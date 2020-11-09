#include "scrabble.h"

#include "formatting.h"
#include <iomanip>
#include <iostream>
#include <map>

using namespace std;

// Given to you. this does not need to be changed
Scrabble::Scrabble(const ScrabbleConfig& config)
        : hand_size(config.hand_size),
          minimum_word_length(config.minimum_word_length),
          tile_bag(TileBag::read(config.tile_bag_file_path, config.seed)),
          board(Board::read(config.board_file_path)),
          dictionary(Dictionary::read(config.dictionary_file_path)) {}

void Scrabble::add_players() {
    // Go through and add players to vector of shared pointers
    size_t n;
    cout << "How many players? ";
    cin >> n;

    if (n < 1) {
        throw FileException("Error, not enough players");
    }
    if (n > 8) {
        throw FileException("Error, too many players");
    }

    for (size_t i = 0; i < n; i++) {

        string name;
        cout << "Player " << i + 1 << ", what is your name? ";
        cin >> name;

        string computer;
        cout << "Is " << name << " a computer? (y or n): ";
        cin >> computer;

        bool humanPlayer = true;
        if (computer != "y" && computer != "Y" && computer != "n" && computer != "N") {
            cout << "Error, command not valid, default to human player" << endl;
            humanPlayer = true;
        }

        if (computer == "Y" || computer == "y") {
            humanPlayer = false;
        }

        if (humanPlayer) {
            shared_ptr<Player> newPlayer(new HumanPlayer(name, hand_size));
            players.push_back(newPlayer);
        }

        else {
            shared_ptr<Player> newPlayer(new ComputerPlayer(name, hand_size));
            players.push_back(newPlayer);
        }

        // Remove hand_size random tiles from bag and add to newPlayer

        players[i]->add_tiles(tile_bag.remove_random_tiles(hand_size));
    }
    cin.ignore();
}
// Game Loop should cycle through players and get and execute that players move
// until the game is over. Game is over when tileBag is out and one player plays all of their letters
void Scrabble::game_loop() {

    bool gameRun = true;
    size_t numHumansPassed = 0;
    size_t numHumans = 0;

    for (size_t i = 0; i < players.size(); i++) {
        if (players[i]->is_human()) {
            numHumans++;
        }
    }
    while (gameRun) {

        // loop through players, get their move and remove tiles of move + add new ones

        for (size_t i = 0; i < players.size(); i++) {

            bool noValidMove = true;

            while (noValidMove) {

                try {

                    Move playerMove = players[i]->get_move(board, dictionary);

                    // PASS
                    if (playerMove.kind == MoveKind::PASS) {

                        if (players[i]->is_human()) {
                            numHumansPassed++;
                        }

                        // Game ends if all human players pass
                        if (numHumansPassed == numHumans) {
                            return;
                        }
                    }
                    // EXCHANGE
                    else if (playerMove.kind == MoveKind::EXCHANGE) {
                        if (players[i]->is_human()) {
                            numHumansPassed = 0;
                        }
                        players[i]->remove_tiles(playerMove.tiles);

                        for (size_t i = 0; i < playerMove.tiles.size(); i++) {
                            tile_bag.add_tile(playerMove.tiles[i]);
                        }

                        players[i]->add_tiles(tile_bag.remove_random_tiles(playerMove.tiles.size()));
                    }

                    // PLACE
                    else {
                        if (players[i]->is_human()) {
                            numHumansPassed = 0;
                        }
                        if (playerMove.tiles.size() < minimum_word_length) {
                            throw MoveException("Word too short");
                        }

                        players[i]->remove_tiles(playerMove.tiles);

                        PlaceResult actuallyPlaced = board.place(playerMove);

                        // add points to player score and refresh new tiles from tileBag
                        players[i]->add_points(actuallyPlaced.points);

                        if (playerMove.tiles.size() == hand_size) {
                            players[i]->add_points(50);
                        }

                        cout << "You gained " << SCORE_COLOR << actuallyPlaced.points << rang::style::reset
                             << " points!" << endl;

                        // Game over
                        if (players[i]->count_tiles() == 0 && tile_bag.count_tiles() == 0) {
                            gameRun = false;
                            return;
                        } else {
                            players[i]->add_tiles(tile_bag.remove_random_tiles(playerMove.tiles.size()));
                        }
                    }
                    cout << "Your current score: " << SCORE_COLOR << players[i]->get_points() << rang::style::reset
                         << endl;
                    noValidMove = false;
                    cout << endl << "Press [enter] to continue.";
                    cin.ignore();
                }

                catch (MoveException& e) {
                    // How to print out error message??
                    cerr << e.what();
                    cout << endl << "Press [enter] to try again.";
                    cin.ignore();
                    continue;
                }

                catch (CommandException& e) {

                    cerr << e.what();
                    cout << endl << "Press [enter] to try again.";
                    cin.ignore();
                    continue;
                }
            }
        }
    }

    // Useful cout expressions with fancy colors. Expressions in curly braces, indicate values you supply.
    // cout << "You gained " << SCORE_COLOR << {points} << rang::style::reset << " points!" << endl;
    // cout << "Your current score: " << SCORE_COLOR << {points} << rang::style::reset << endl;
    // cout << endl << "Press [enter] to continue.";

    // Use tile_bag
}

// Performs final score subtraction. Players lose points for each tile in their
// hand. The player who cleared their hand receives all the points lost by the
// other players.
void Scrabble::final_subtraction(vector<shared_ptr<Player>>& plrs) {

    int scoreAdded = 0;
    int winIndex;
    int countNotEmpty = 0;

    for (size_t i = 0; i < plrs.size(); i++) {
        // Still has tiles in hand
        if (plrs[i]->count_tiles() != 0) {
            // Score caps at 0
            scoreAdded += plrs[i]->get_hand_value();
            plrs[i]->subtract_points(plrs[i]->get_hand_value());
            countNotEmpty++;
        } else {
            winIndex = i;
        }
    }

    // Nobody cleared hand
    if (countNotEmpty == plrs.size()) {
        return;
    }
    // player at winindex cleared hand
    else {
        plrs[winIndex]->add_points(scoreAdded);
    }
}

// You should not need to change this function.
void Scrabble::print_result() {
    // Determine highest score
    size_t max_points = 0;
    for (auto player : this->players) {
        if (player->get_points() > max_points) {
            max_points = player->get_points();
        }
    }

    // Determine the winner(s) indexes
    vector<shared_ptr<Player>> winners;
    for (auto player : this->players) {
        if (player->get_points() >= max_points) {
            winners.push_back(player);
        }
    }

    cout << (winners.size() == 1 ? "Winner:" : "Winners: ");
    for (auto player : winners) {
        cout << SPACE << PLAYER_NAME_COLOR << player->get_name();
    }
    cout << rang::style::reset << endl;

    // now print score table
    cout << "Scores: " << endl;
    cout << "---------------------------------" << endl;

    // Justify all integers printed to have the same amount of character as the high score, left-padding with spaces
    cout << setw(static_cast<uint32_t>(floor(log10(max_points) + 1)));

    for (auto player : this->players) {
        cout << SCORE_COLOR << player->get_points() << rang::style::reset << " | " << PLAYER_NAME_COLOR
             << player->get_name() << rang::style::reset << endl;
    }
}

// You should not need to change this.
void Scrabble::main() {
    add_players();
    game_loop();
    final_subtraction(this->players);
    print_result();
}
