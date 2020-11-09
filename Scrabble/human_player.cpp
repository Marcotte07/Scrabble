#include "human_player.h"

#include "exceptions.h"
#include "formatting.h"
#include "move.h"
#include "place_result.h"
#include "rang.h"
#include "tile_kind.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

// This method is fully implemented.
inline string& to_upper(string& str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

// this encapsulates how a human being playing the game would interact with the game.
// This means that the get_move function must query the user for input and parse that input
// into a move. Since get_move should only return valid moves, this method should use the Board
// and Dictionary to check that the move is legal.

Move HumanPlayer::get_move(const Board& board, const Dictionary& dictionary) const {

    // vector of all string commands given
    board.print(std::cout);
    print_hand(std::cout);
    vector<string> commandsVec;
    string command;
    cout << "Enter your move, " << this->get_name() << ":  ";

    std::string line;
    std::getline(cin, line);
    std::istringstream iss(line);
    while (iss >> line) {
        commandsVec.push_back(line);
    }

    Move playerMove = parse_move(commandsVec);

    // Move is place and commands are valid, need to check if word can be placed on board(using board and dictionary),
    // and if move vector works for hand

    // PLACE
    if (playerMove.kind == MoveKind::PLACE) {

        // If move is too large, then throw exception
        if (playerMove.tiles.size() > this->get_hand_size()) {
            throw MoveException("Too many letters placed");
        }

        PlaceResult result = board.test_place(playerMove);

        if (!result.valid) {
            throw MoveException(result.error);
        }

        // result contains words and amount of points
        for (size_t i = 0; i < result.words.size(); i++) {
            if (!dictionary.is_word(result.words[i])) {
                throw MoveException("1 or more words formed are not valid");
            }
        }

        // All words valid and move is placed on the board
        return playerMove;
    }

    // EXCHANGE
    else if (playerMove.kind == MoveKind::EXCHANGE) {
        // Anything else to do here?
        if (playerMove.tiles.size() > this->get_hand_size()) {
            throw MoveException("Too many letters to swap");
        }
        return playerMove;
    }

    // Pass, so dont need to check for anything
    else {
        return playerMove;
    }
}

// Pass string of letter that user inputted to here,
// return vector that will be used in move

// This function is private, and called only by parse_move, which handles making sure everything is valid.

// Need to also check if tiles used are in hand, if not throw invalid move exception

vector<TileKind> HumanPlayer::parse_tiles(string& letters, MoveKind kind) const {

    vector<TileKind> returnVec;

    // Use TileCollection method count_tiles(TileKind) with duplicates, this is a map of TileKind and each duplicate
    map<TileKind, int> lettersDuplicateMap;

    letters = to_upper(letters);

    // Convert string letters to tileKinds and if found in map, update count. If not found, insert and set count to 1
    for (size_t i = 0; i < letters.size(); i++) {

        if (letters[i] == '?') {

            try {
                TileKind newTile = tiles.lookup_tile(letters[i]);
                map<TileKind, int>::iterator mapIt;
                mapIt = lettersDuplicateMap.find(newTile);
                // Duplicate found
                if (mapIt != lettersDuplicateMap.end()) {
                    mapIt->second++;
                }
                // Not found, enter into map
                else {
                    lettersDuplicateMap.insert(pair<TileKind, int>(newTile, 1));
                }
                i++;
            }

            catch (out_of_range& e) {
                throw MoveException("Invalid Move, tile not found in your hand");
            }
        }

        else {
            try {
                TileKind newTile = tiles.lookup_tile(letters[i]);
                map<TileKind, int>::iterator mapIt;
                mapIt = lettersDuplicateMap.find(newTile);
                // Duplicate found
                if (mapIt != lettersDuplicateMap.end()) {
                    mapIt->second++;
                }
                // Not found, enter into map
                else {
                    lettersDuplicateMap.insert(pair<TileKind, int>(newTile, 1));
                }
            }

            catch (out_of_range& e) {
                throw MoveException("Invalid Move, letter not found in your hand");
            }
        }
    }

    // Now all tiles in letters are in Map except for ones after ?, need to check if all duplicates actually exist in
    // players hand and then add to returnVec, Converting ? to their actuall letters

    // use count_tiles, if player.count_tiles(it->first) != it->second then throw invalid move, letter not found in hand

    // Check for duplicates, doesn't remove anything!
    for (map<TileKind, int>::iterator it = lettersDuplicateMap.begin(); it != lettersDuplicateMap.end(); ++it) {

        // Using more tiles than whats in your inventory
        if (this->tiles.count_tiles(it->first) < it->second) {
            throw MoveException("Invalid Move, tile not found in your hand");
        }
    }

    if (kind == MoveKind::PLACE) {
        // Now go through letters and becuase its known there are no errors, add to returnVec each tile, but leave out ?
        // symbol
        for (size_t i = 0; i < letters.size(); i++) {
            if (letters[i] == '?') {
                continue;
            }

            // If letters[i-1] is ?, then dont lookup tile but just add letters[i] to vector
            if (i > 0) {
                if (letters[i - 1] == '?') {
                    TileKind newTile(letters[i], 0);
                    returnVec.push_back(newTile);
                    continue;
                }
            }

            TileKind newTile = tiles.lookup_tile(letters[i]);
            returnVec.push_back(newTile);
        }
    }
    // Move Kind is EXCHANGE, so just go through letters and add everything to returnVec
    else {
        for (size_t i = 0; i < letters.size(); i++) {
            TileKind newTile = tiles.lookup_tile(letters[i]);
            returnVec.push_back(newTile);
        }
    }

    return returnVec;
}

// Used to take in string but now vector

// Takes in vector of commands from get_move, parses it into a valid move or throws commandException.
// Calls parse_tiles to get move.tiles

Move HumanPlayer::parse_move(vector<string>& commandsVec) const {
    // If no command given
    if (commandsVec.size() == 0) {
        throw CommandException("Invalid Command");
    }

    // PLACE
    if (to_upper(commandsVec[0]) == "PLACE") {
        if (commandsVec.size() != 5) {
            throw CommandException("Invalid Command");
        }
        string strDirection = commandsVec[1];
        Direction direction;
        if (strDirection == "-") {
            direction = Direction::ACROSS;
        } else if (strDirection == "|") {
            direction = Direction::DOWN;
        } else {
            throw CommandException("Invalid Command");
        }

        size_t row = stoi(commandsVec[2]) - 1;
        size_t column = stoi(commandsVec[3]) - 1;

        string letters = to_upper(commandsVec[4]);

        // Pass letters into parse_tiles and return tile vector,
        // Create a move object with all info of action place and return, only check if valid move in main part of
        // get_move
        MoveKind kind = MoveKind::PLACE;
        vector<TileKind> tileVec = parse_tiles(letters, kind);
        // Need to remove ? from tileVec only if letter after it

        Move playerMove(tileVec, row, column, direction);
        return playerMove;
    }

    // PASS
    else if (to_upper(commandsVec[0]) == "PASS") {
        if (commandsVec.size() != 1) {
            throw CommandException("Invalid Command");
        }
        Move playerMove;
        return playerMove;
    }

    // EXCHANGE
    else if (to_upper(commandsVec[0]) == "EXCHANGE") {
        if (commandsVec.size() != 2) {
            throw CommandException("Invalid Command");
        }
        string letters = to_upper(commandsVec[1]);
        MoveKind kind = MoveKind::EXCHANGE;
        vector<TileKind> tileVec = parse_tiles(letters, kind);
        Move playerMove(tileVec);

        return playerMove;

    }

    else {
        throw CommandException("Invalid Command");
    }
}

// This function is fully implemented.
void HumanPlayer::print_hand(ostream& out) const {
    const size_t tile_count = tiles.count_tiles();
    const size_t empty_tile_count = this->get_hand_size() - tile_count;
    const size_t empty_tile_width = empty_tile_count * (SQUARE_OUTER_WIDTH - 1);

    for (size_t i = 0; i < HAND_TOP_MARGIN - 2; ++i) {
        out << endl;
    }

    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_HEADING << "Your Hand: " << endl << endl;

    // Draw top line
    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE;
    print_horizontal(tile_count, L_TOP_LEFT, T_DOWN, L_TOP_RIGHT, out);
    out << repeat(SPACE, empty_tile_width) << BG_COLOR_OUTSIDE_BOARD << endl;

    // Draw middle 3 lines
    for (size_t line = 0; line < SQUARE_INNER_HEIGHT; ++line) {
        out << FG_COLOR_LABEL << BG_COLOR_OUTSIDE_BOARD << repeat(SPACE, HAND_LEFT_MARGIN);
        for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL << BG_COLOR_PLAYER_HAND;

            // Print letter
            if (line == 1) {
                out << repeat(SPACE, 2) << FG_COLOR_LETTER << (char)toupper(it->letter) << repeat(SPACE, 2);

                // Print score in bottom right
            } else if (line == SQUARE_INNER_HEIGHT - 1) {
                out << FG_COLOR_SCORE << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << setw(2) << it->points;

            } else {
                out << repeat(SPACE, SQUARE_INNER_WIDTH);
            }
        }
        if (tiles.count_tiles() > 0) {
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL;
            out << repeat(SPACE, empty_tile_width) << BG_COLOR_OUTSIDE_BOARD << endl;
        }
    }

    // Draw bottom line
    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE;
    print_horizontal(tile_count, L_BOTTOM_LEFT, T_UP, L_BOTTOM_RIGHT, out);
    out << repeat(SPACE, empty_tile_width) << rang::style::reset << endl;
}
