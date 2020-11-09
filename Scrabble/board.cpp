#include "board.h"

#include "board_square.h"
#include "exceptions.h"
#include "formatting.h"
#include <fstream>
#include <iomanip>

using namespace std;

bool Board::Position::operator==(const Board::Position& other) const {
    return this->row == other.row && this->column == other.column;
}

bool Board::Position::operator!=(const Board::Position& other) const {
    return this->row != other.row || this->column != other.column;
}

Board::Position Board::Position::translate(Direction direction) const { return this->translate(direction, 1); }

Board::Position Board::Position::translate(Direction direction, ssize_t distance) const {
    if (direction == Direction::DOWN) {
        return Board::Position(this->row + distance, this->column);
    } else {
        return Board::Position(this->row, this->column + distance);
    }
}

/* HW5: IMPLEMENT THIS
    Returns the letter at a position.
    Assumes there is a tile at p
    */
char Board::letter_at(Position p) const { return squares[p.row][p.column].get_tile_kind().letter; }

/* HW5: IMPLEMENT THIS
Returns bool indicating whether position p is an anchor spot or not.

A position is an anchor spot if it is
    1) In bounds
    2) Unoccupied
    3) Either adjacent to a placed tile or is the start square
*/
bool Board::is_anchor_spot(Position p) const {
    // In bounds but no tile
    if (is_in_bounds(p) && !in_bounds_and_has_tile(p)) {
        // Check left, right, down, up for tile, if any have tile then anchor
        Position left(p.row, p.column - 1);
        Position right(p.row, p.column + 1);
        Position up(p.row - 1, p.column);
        Position down(p.row + 1, p.column);

        if (in_bounds_and_has_tile(left) || in_bounds_and_has_tile(right) || in_bounds_and_has_tile(up)
            || in_bounds_and_has_tile(down)) {
            return true;
        }
        if (!in_bounds_and_has_tile(start) && p == start) {
            return true;
        }
    }
    return false;
}

/* HW5: IMPLEMENT THIS
Returns a vector of all the Anchors on the board.

For every anchor sqare on the board, it should include two Anchors in the vector.
    One for ACROSS and one for DOWN
The limit for the Anchor is the number of unoccupied, non-anchor squares preceeding the anchor square in question.
*/
std::vector<Board::Anchor> Board::get_anchors() const {

    std::vector<Anchor> anchorList;

    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < columns; c++) {
            Position p(r, c);
            if (is_anchor_spot(p)) {
                // Find number of non-occupied, non-anchor squares preceding the anchor square
                // This is the limit

                // Down direction, so go up to look
                size_t limitUp = 0;
                Position up(p.row - 1, p.column);
                while (!is_anchor_spot(up) && is_in_bounds(up) && !in_bounds_and_has_tile(up)) {
                    limitUp++;
                    up.row--;
                }
                size_t limitLeft = 0;
                Position left(p.row, p.column - 1);
                while (!is_anchor_spot(left) && is_in_bounds(left) && !in_bounds_and_has_tile(left)) {
                    limitLeft++;
                    left.column--;
                }

                // Now create the two anchors, and push them onto vector
                Anchor downAnchor(p, Direction::DOWN, limitUp);
                Anchor rightAnchor(p, Direction::ACROSS, limitLeft);
                anchorList.push_back(downAnchor);
                anchorList.push_back(rightAnchor);
            }
        }
    }
    return anchorList;
}  // Used for testing

Board Board::read(const string& file_path) {
    ifstream file(file_path);
    if (!file) {
        throw FileException("cannot open board file!");
    }

    size_t rows;
    size_t columns;
    size_t starting_row;
    size_t starting_column;
    file >> rows >> columns >> starting_row >> starting_column;
    Board board(rows, columns, starting_row, starting_column);

    // TODO: complete implementation of reading in board from file here.

    // Read in y rows of x chars into board
    string line;
    getline(file, line);
    for (size_t r = 0; r < board.rows; r++) {
        vector<BoardSquare> row;
        board.squares.push_back(row);
        for (size_t c = 0; c < board.columns; c++) {

            char in;
            file >> in;

            if (in == '.') {
                BoardSquare newSquare(1, 1);
                board.squares[r].push_back(newSquare);
            } else if (in == '2') {
                BoardSquare newSquare(2, 1);
                board.squares[r].push_back(newSquare);
            } else if (in == '3') {
                BoardSquare newSquare(3, 1);
                board.squares[r].push_back(newSquare);
            } else if (in == 'd') {
                BoardSquare newSquare(1, 2);
                board.squares[r].push_back(newSquare);
            } else {
                BoardSquare newSquare(1, 3);
                board.squares[r].push_back(newSquare);
            }
        }
    }
    return board;
}

size_t Board::get_move_index() const { return this->move_index; }

// Test_place should verify that the move given as an argument can be placed on the board
// It should return a valid PlaceResult object with appropriate words if so
// and an invalid PlaceResult object with error message otherwise.
PlaceResult Board::test_place(const Move& move) const {

    int points = 0;
    int pointsOfOtherWords = 0;
    int wordMultiplier = 1;

    bool firstMove = true;
    bool validPlacement = false;
    bool playedAtStart = false;

    std::vector<std::string> words;
    // This will be words[0] to build main word out of
    words.push_back("");

    // Check if first move has been played already
    if (in_bounds_and_has_tile(this->start)) {
        firstMove = false;
    }

    // First letter placed already has a tile in that poistion, bad overlap
    if (squares[move.row][move.column].has_tile()) {
        PlaceResult error("ERROR: OVERLAP");
        return error;
    }

    // if not first move, has to be placed next to already existing word or words, add those words to vector words
    // Check if in any direction, directly next to the first tile placed there is a tile already there, if not return
    // invalid PlaceResult
    if (!firstMove) {
        Position left(move.row, move.column - 1);
        Position right(move.row, move.column + 1);
        Position up(move.row - 1, move.column);
        Position down(move.row + 1, move.column);

        // None of these positions have a tile, cant place a tile if no tile next to it
        if (in_bounds_and_has_tile(left) || in_bounds_and_has_tile(right) || in_bounds_and_has_tile(up)
            || in_bounds_and_has_tile(down)) {
            validPlacement = true;
        }
    }

    if (move.direction == Direction::DOWN) {

        size_t r = move.row;
        size_t tilesPlaced = 0;

        // If only 1 tile in move and nothing up or down from tile, so only way to be valid is if placed at start or end
        // of horizontal word
        Position up(move.row - 1, move.column);
        Position down(move.row + 1, move.column);
        if (move.tiles.size() == 1 && !in_bounds_and_has_tile(up) && !in_bounds_and_has_tile(down)) {
            Position right(r, move.column + 1);
            Position left(r, move.column - 1);

            if (this->in_bounds_and_has_tile(left)) {
                validPlacement = true;
                // Need to go left until tile with no letter, then starting there add letters to the right and add word
                // to vector of words
                while (this->in_bounds_and_has_tile(left)) {
                    left.column--;
                }

                // Now newLeft is start of word, need to go through and add letters to word and add points from new word
                Position newWordPosition(r, left.column + 1);

                // if new tile just placed is on word multiplier
                while (this->is_in_bounds(newWordPosition)) {

                    if (newWordPosition.column == move.column) {
                        points += move.tiles[tilesPlaced].points * squares[move.row][move.column].letter_multiplier;
                    }
                    // That tile has been used before so score doesnt include letter multiplier
                    else {
                        if (!in_bounds_and_has_tile(newWordPosition)) {
                            break;
                        }
                        points += squares[newWordPosition.row][newWordPosition.column].get_points()
                                  / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                        words[0] += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                    }
                    newWordPosition.column++;
                }
                words[0] += move.tiles[tilesPlaced].letter;
                // If tile just "placed" is a word multiplier, multiply score of other word created by this
                points *= squares[move.row][move.column].word_multiplier;
                PlaceResult result(words, points);
                return result;
            }

            // no tiles to the left of pos, start word from pos and go right
            else if (this->in_bounds_and_has_tile(right)) {
                validPlacement = true;
                Position newWordPosition(move.row, move.column);

                while (this->is_in_bounds(newWordPosition)) {
                    if (newWordPosition.column == move.column) {
                        points += move.tiles[tilesPlaced].points * squares[move.row][move.column].letter_multiplier;
                    } else {
                        if (!in_bounds_and_has_tile(newWordPosition)) {
                            break;
                        }
                        points += squares[newWordPosition.row][newWordPosition.column].get_points()
                                  / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                        words[0] += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                    }
                    newWordPosition.column++;
                }

                words[0] += move.tiles[tilesPlaced].letter;
                // Add on multiplier of tile placed
                points *= squares[move.row][move.column].word_multiplier;
                PlaceResult result(words, points);
                return result;
            }

            else {
                PlaceResult error("ERROR: Invalid Placement");
                return error;
            }
        }

        // Check upwards for any tiles
        Position pos(move.row - 1, move.column);
        while (this->in_bounds_and_has_tile(pos)) {
            validPlacement = true;
            points += squares[pos.row][pos.column].get_points() / squares[pos.row][pos.column].letter_multiplier;
            pos.row--;
        }

        // Now pos is at first, need to go until back at first tile placed, adding all to words[0]
        pos.row++;
        while (pos.row != move.row) {
            words[0] += squares[pos.row][pos.column].get_tile_kind().letter;
            pos.row++;
        }

        // Loop will run until all tiles in move are "placed"
        while (tilesPlaced != move.tiles.size()) {
            // Position that new tile could go in
            Position pos(r, move.column);

            // If new position is in bounds
            if (this->is_in_bounds(pos)) {
                // Already tile there, dont add any multiplier but add points and letter to word formed, also makes
                // placement valid
                if (squares[r][move.column].has_tile()) {
                    validPlacement = true;
                    points += squares[r][move.column].get_points() / squares[r][move.column].letter_multiplier;
                    words[0] += squares[r][move.column].get_tile_kind().letter;
                }

                // No tile on this square, increment tilesPlaced and add to score, depending on value of tile and
                // multipliers
                else {
                    if (pos.row == start.row && pos.column == start.column) {
                        playedAtStart = true;
                    }
                    points += move.tiles[tilesPlaced].points * squares[r][move.column].letter_multiplier;
                    wordMultiplier *= squares[r][move.column].word_multiplier;
                    words[0] += move.tiles[tilesPlaced].letter;

                    // If letter next to the letter placed, then new word!
                    Position right(r, move.column + 1);
                    Position left(r, move.column - 1);

                    if (this->in_bounds_and_has_tile(left)) {
                        validPlacement = true;
                        // Need to go left until tile with no letter, then starting there add letters to the right and
                        // add word to vector of words
                        while (this->in_bounds_and_has_tile(left)) {
                            left.column--;
                        }

                        // Now newLeft is start of word, need to go through and add letters to word and add points from
                        // new word
                        Position newWordPosition(r, left.column + 1);
                        std::string newWord = "";
                        int pointsAdded = 0;

                        // if new tile just placed is on word multiplier
                        while (this->is_in_bounds(newWordPosition)) {

                            if (newWordPosition == pos) {
                                pointsAdded += move.tiles[tilesPlaced].points
                                               * squares[pos.row][pos.column].letter_multiplier;
                                char lastLetter = move.tiles[tilesPlaced].letter;
                                newWord += lastLetter;
                            }
                            // That tile has been used before so score doesnt include letter multiplier
                            else {
                                if (!in_bounds_and_has_tile(newWordPosition)) {
                                    break;
                                }
                                pointsAdded += squares[newWordPosition.row][newWordPosition.column].get_points()
                                               / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                                newWord += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                            }

                            newWordPosition.column++;
                        }

                        // If tile just "placed" is a word multiplier, multiply score of other word created by this
                        pointsAdded *= squares[pos.row][pos.column].word_multiplier;
                        pointsOfOtherWords += pointsAdded;
                        // newWord is done and points have been totalled for that word, add word to vector of words
                        words.push_back(newWord);

                    }
                    // no tiles to the left of pos, start word from pos and go right
                    else if (this->in_bounds_and_has_tile(right)) {
                        validPlacement = true;
                        Position newWordPosition = pos;
                        std::string newWord = "";
                        int pointsAdded = 0;

                        while (this->is_in_bounds(newWordPosition)) {
                            if (newWordPosition == pos) {
                                pointsAdded += move.tiles[tilesPlaced].points
                                               * squares[pos.row][pos.column].letter_multiplier;
                                char lastLetter = move.tiles[tilesPlaced].letter;
                                newWord += lastLetter;

                            } else {
                                if (!in_bounds_and_has_tile(newWordPosition)) {
                                    break;
                                }
                                pointsAdded += squares[newWordPosition.row][newWordPosition.column].get_points()
                                               / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                                newWord += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                            }

                            newWordPosition.column++;
                        }

                        // Add on multiplier of tile placed
                        pointsAdded *= squares[pos.row][pos.column].word_multiplier;
                        pointsOfOtherWords += pointsAdded;

                        words.push_back(newWord);
                    }

                    else {
                        // No new words for that tile placement, go to next tile placement!
                    }

                    tilesPlaced++;
                }
                r++;
            }
            // Position is NOT in bounds, make new place_result of ERROR
            else {
                PlaceResult error("ERROR: out of bounds");
                return error;
            }
        }

        // Still could be tiles already placed vertically below last tile placed! These need to be added to words[0]!
        pos.row = r;
        pos.column = move.column;
        while (this->in_bounds_and_has_tile(pos)) {
            validPlacement = true;
            points += squares[pos.row][pos.column].get_points() / squares[pos.row][pos.column].letter_multiplier;
            words[0] += squares[pos.row][pos.column].get_tile_kind().letter;
            pos.row++;
        }

        // All tiles have been placed now, if validPlacement is false then it is an invalid move, if true then valid
        // move and
        if (!validPlacement && !firstMove) {
            PlaceResult error("ERROR: Not valid placement");
            return error;
        }

        if (firstMove && !playedAtStart) {
            PlaceResult error("ERROR: FIRST MOVE MUST BE PLACED AT START");
            return error;
        }

        // Loop has completed without error, so all tiles in move have been placed, calculate total points and return
        // PlaceResult object Points is just words[0] and points of other words is all other words formed.
        points *= wordMultiplier;
        points += pointsOfOtherWords;
        PlaceResult placement(words, points);
        return placement;
    }

    // ACROSS
    // Now do the same for a move across
    else {
        // c is index inside board
        size_t c = move.column;
        size_t tilesPlaced = 0;

        // If only 1 tile in move and nothing left or right from tile, so only way to be valid is if placed at start or
        // end of horizontal word
        Position left(move.row, move.column - 1);
        Position right(move.row, move.column + 1);
        if (move.tiles.size() == 1 && !in_bounds_and_has_tile(left) && !in_bounds_and_has_tile(right)) {
            Position up(move.row - 1, c);
            Position down(move.row + 1, c);

            if (this->in_bounds_and_has_tile(up)) {
                validPlacement = true;
                // Need to go left until tile with no letter, then starting there add letters to the right and add word
                // to vector of words
                while (this->in_bounds_and_has_tile(up)) {
                    up.row--;
                }

                // Now newLeft is start of word, need to go through and add letters to word and add points from new word
                Position newWordPosition(up.row + 1, c);

                // if new tile just placed is on word multiplier
                while (this->is_in_bounds(newWordPosition)) {

                    if (newWordPosition.row == move.row) {
                        points += move.tiles[tilesPlaced].points * squares[move.row][move.column].letter_multiplier;
                    }
                    // That tile has been used before so score doesnt include letter multiplier
                    else {
                        if (!in_bounds_and_has_tile(newWordPosition)) {
                            break;
                        }
                        points += squares[newWordPosition.row][newWordPosition.column].get_points()
                                  / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                        words[0] += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                    }
                    newWordPosition.row++;
                }
                words[0] += move.tiles[tilesPlaced].letter;
                // If tile just "placed" is a word multiplier, multiply score of other word created by this
                points *= squares[move.row][move.column].word_multiplier;
                PlaceResult result(words, points);
                return result;
            }

            else if (this->in_bounds_and_has_tile(down)) {
                validPlacement = true;
                Position newWordPosition(move.row, move.column);

                while (this->is_in_bounds(newWordPosition)) {
                    if (newWordPosition.column == move.column) {
                        points += move.tiles[tilesPlaced].points * squares[move.row][move.column].letter_multiplier;
                    } else {
                        if (!in_bounds_and_has_tile(newWordPosition)) {
                            break;
                        }
                        points += squares[newWordPosition.row][newWordPosition.column].get_points()
                                  / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                        words[0] += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                    }
                    newWordPosition.row++;
                }

                words[0] += move.tiles[tilesPlaced].letter;
                // Add on multiplier of tile placed
                points *= squares[move.row][move.column].word_multiplier;
                PlaceResult result(words, points);
                return result;
            }

            else {
                PlaceResult error("ERROR: Invalid Placement");
                return error;
            }
        }

        // Check left for any tiles to add to words[0] and score to add
        Position pos(move.row, move.column - 1);
        while (this->in_bounds_and_has_tile(pos)) {
            validPlacement = true;
            points += squares[pos.row][pos.column].get_points() / squares[pos.row][pos.column].letter_multiplier;
            pos.column--;
        }

        pos.column++;
        while (pos.column != move.column) {
            words[0] += squares[pos.row][pos.column].get_tile_kind().letter;
            pos.column++;
        }

        // Loop will run until all tiles in move are "placed"
        while (tilesPlaced != move.tiles.size()) {
            // Position that new tile could go in
            Position pos(move.row, c);

            // If new position is in bounds
            if (this->is_in_bounds(pos)) {
                // Already tile there, dont add any multiplier but add points and letter to word formed
                if (squares[pos.row][pos.column].has_tile()) {
                    validPlacement = true;
                    points += squares[pos.row][pos.column].get_points()
                              / squares[pos.row][pos.column].letter_multiplier;
                    words[0] += squares[pos.row][pos.column].get_tile_kind().letter;
                }

                // No tile on this square, increment tilesPlaced and add to score, depending on value of tile and
                // multipliers, and look for new words up and down
                else {
                    if (pos.row == start.row && pos.column == start.column) {
                        playedAtStart = true;
                    }
                    points += move.tiles[tilesPlaced].points * squares[pos.row][pos.column].letter_multiplier;
                    wordMultiplier *= squares[pos.row][pos.column].word_multiplier;
                    words[0] += move.tiles[tilesPlaced].letter;

                    // If letter next to the letter placed, then new word!
                    Position up(pos.row - 1, pos.column);
                    Position down(pos.row + 1, pos.column);

                    if (this->in_bounds_and_has_tile(up)) {
                        validPlacement = true;
                        // Need to go left until tile with no letter, then starting there add letters to the right and
                        // add word to vector of words
                        while (this->in_bounds_and_has_tile(up)) {
                            up.row--;
                        }

                        // Now newWordPosition is start of word, need to go dowm and add letters to word and add points
                        // from new word

                        Position newWordPosition(up.row + 1, up.column);
                        std::string newWord = "";
                        int pointsAdded = 0;

                        while (this->is_in_bounds(newWordPosition)) {

                            if (newWordPosition == pos) {
                                // HAVENT ACTUALLY PLACED ON BOARD YET, CANT Access board because not on board
                                pointsAdded += move.tiles[tilesPlaced].points
                                               * squares[pos.row][pos.column].letter_multiplier;
                                char lastLetter = move.tiles[tilesPlaced].letter;
                                newWord += lastLetter;
                            }

                            // That tile has been used before so score doesnt include letter multiplier
                            else {
                                if (!in_bounds_and_has_tile(newWordPosition)) {
                                    break;
                                }
                                pointsAdded += squares[newWordPosition.row][newWordPosition.column].get_points()
                                               / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                                newWord += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                            }
                            // Regardless of whether placed before or not, add letter to newWord

                            newWordPosition.row++;
                        }
                        // If tile just "placed" is a word multiplier, multiply score of other word created by this
                        pointsAdded *= squares[pos.row][pos.column].word_multiplier;
                        pointsOfOtherWords += pointsAdded;
                        // newWord is done and points have been totalled for that word, add word to vector of words
                        words.push_back(newWord);

                    }
                    // no tiles to above of pos, start word from pos and look down
                    else if (this->in_bounds_and_has_tile(down)) {
                        validPlacement = true;
                        Position newWordPosition = pos;
                        std::string newWord = "";
                        int pointsAdded = 0;

                        while (this->is_in_bounds(newWordPosition)) {
                            if (newWordPosition == pos) {
                                pointsAdded += move.tiles[tilesPlaced].points
                                               * squares[pos.row][pos.column].letter_multiplier;
                                char lastLetter = move.tiles[tilesPlaced].letter;
                                newWord += lastLetter;
                            } else {
                                if (!in_bounds_and_has_tile(newWordPosition)) {
                                    break;
                                }
                                pointsAdded += squares[newWordPosition.row][newWordPosition.column].get_points()
                                               / squares[newWordPosition.row][newWordPosition.column].letter_multiplier;
                                newWord += squares[newWordPosition.row][newWordPosition.column].get_tile_kind().letter;
                            }

                            newWordPosition.row++;
                        }

                        // Add on multiplier of tile placed
                        pointsAdded *= squares[pos.row][pos.column].word_multiplier;
                        pointsOfOtherWords += pointsAdded;

                        words.push_back(newWord);
                    }

                    else {
                        // No new words for that tile placement, go to next tile placement!
                    }

                    tilesPlaced++;
                }
                c++;
            }
            // Position is NOT in bounds, make new place_result of ERROR
            else {
                PlaceResult error("ERROR: OUT OF BOUNDS");
                return error;
            }
        }

        // This only runs when all tiles have been placed
        // Still could be tiles already placed horizontally to the right of last tile placed! These need to be added to
        // words[0]!
        pos.row = move.row;
        pos.column = c;
        while (this->in_bounds_and_has_tile(pos)) {
            validPlacement = true;
            points += squares[pos.row][pos.column].get_points() / squares[pos.row][pos.column].letter_multiplier;
            words[0] += squares[pos.row][pos.column].get_tile_kind().letter;
            pos.column++;
        }

        if (!validPlacement && !firstMove) {
            PlaceResult error("ERROR: Not valid placement");
            return error;
        }

        if (firstMove && !playedAtStart) {
            PlaceResult error("ERROR: FIRST MOVE MUST BE PLACED AT START");
            return error;
        }

        // Loop has completed without error, so all tiles in move have been placed, calculate total points and return
        // PlaceResult object Points is just words[0] and points of other words is all other words formed.
        points *= wordMultiplier;
        points += pointsOfOtherWords;
        PlaceResult placement(words, points);
        return placement;
    }
}

// If the move given is valid and can be placed on the board, this method
// will make the changes to the board for the move and return the
// PlaceResult object. The result returned by this method should be exactly
// what test_place would return, so consider simply using test_place then
// modifying the board.
PlaceResult Board::place(const Move& move) {

    PlaceResult placement = test_place(move);

    // Not valid move, so return
    if (!placement.valid) {
        return placement;
    }

    // Same overall loop as test_place, modifying the board
    if (move.direction == Direction::DOWN) {
        // r is index inside board
        size_t r = move.row;
        size_t tilesPlaced = 0;

        // Loop will run until all tiles in move are "placed"
        while (tilesPlaced != move.tiles.size()) {
            Position pos(r, move.column);

            // If new position is in bounds and doesnt have a tile
            if (this->is_in_bounds(pos)) {
                // Already tile there, dont add any multiplier but add points
                if (squares[r][move.column].has_tile()) {

                }

                // No tile on this square, increment tilesPlaced and add to score, depending on value of tile and
                // multipliers
                else {
                    // Add tile to board, go to next tile in move
                    squares[r][move.column].set_tile_kind(move.tiles[tilesPlaced]);
                    tilesPlaced++;
                }
                r++;
            }
            // Position is NOT in bounds, make new place_result of ERROR, should never be called but just in case
            else {
                PlaceResult error("ERROR: OUT OF BOUNDS");
                return error;
            }
        }

        // returns what test_place returned
        return placement;
    }

    // Now do the same for a move across
    else if (move.direction == Direction::ACROSS) {
        // r is index inside board
        size_t c = move.column;
        size_t tilesPlaced = 0;

        // Loop will run until all tiles in move are "placed"
        while (tilesPlaced != move.tiles.size()) {
            Position pos(move.row, c);

            // If new position is in bounds
            if (this->is_in_bounds(pos)) {
                // Already tile there
                if (squares[move.row][c].has_tile()) {

                }

                // No tile on this square, increment tilesPlaced and actually place tile, depending on value of tile and
                // multipliers
                else {
                    squares[move.row][c].set_tile_kind(move.tiles[tilesPlaced]);
                    tilesPlaced++;
                }
                c++;
            }
            // Position is NOT in bounds, make new place_result of ERROR, shouldnt need this but just in case
            else {
                PlaceResult error("ERROR: OUT OF BOUNDS");
                return error;
            }
        }

        return placement;
    }

    // Direction is None, dont know what to do
    else {
        std::vector<std::string> empty;
        PlaceResult placement(empty, 0);
        return placement;
    }
}

// The rest of this file is provided for you. No need to make changes.

BoardSquare& Board::at(const Board::Position& position) { return this->squares.at(position.row).at(position.column); }

const BoardSquare& Board::at(const Board::Position& position) const {
    return this->squares.at(position.row).at(position.column);
}

bool Board::is_in_bounds(const Board::Position& position) const {
    return position.row < this->rows && position.column < this->columns;
}

bool Board::in_bounds_and_has_tile(const Position& position) const {
    return is_in_bounds(position) && at(position).has_tile();
}

void Board::print(ostream& out) const {
    // Draw horizontal number labels
    for (size_t i = 0; i < BOARD_TOP_MARGIN - 2; ++i) {
        out << std::endl;
    }
    out << FG_COLOR_LABEL << repeat(SPACE, BOARD_LEFT_MARGIN);
    const size_t right_number_space = (SQUARE_OUTER_WIDTH - 3) / 2;
    const size_t left_number_space = (SQUARE_OUTER_WIDTH - 3) - right_number_space;
    for (size_t column = 0; column < this->columns; ++column) {
        out << repeat(SPACE, left_number_space) << std::setw(2) << column + 1 << repeat(SPACE, right_number_space);
    }
    out << std::endl;

    // Draw top line
    out << repeat(SPACE, BOARD_LEFT_MARGIN);
    print_horizontal(this->columns, L_TOP_LEFT, T_DOWN, L_TOP_RIGHT, out);
    out << endl;

    // Draw inner board
    for (size_t row = 0; row < this->rows; ++row) {
        if (row > 0) {
            out << repeat(SPACE, BOARD_LEFT_MARGIN);
            print_horizontal(this->columns, T_RIGHT, PLUS, T_LEFT, out);
            out << endl;
        }

        // Draw insides of squares
        for (size_t line = 0; line < SQUARE_INNER_HEIGHT; ++line) {
            out << FG_COLOR_LABEL << BG_COLOR_OUTSIDE_BOARD;

            // Output column number of left padding
            if (line == 1) {
                out << repeat(SPACE, BOARD_LEFT_MARGIN - 3);
                out << std::setw(2) << row + 1;
                out << SPACE;
            } else {
                out << repeat(SPACE, BOARD_LEFT_MARGIN);
            }

            // Iterate columns
            for (size_t column = 0; column < this->columns; ++column) {
                out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL;
                const BoardSquare& square = this->squares.at(row).at(column);
                bool is_start = this->start.row == row && this->start.column == column;

                // Figure out background color
                if (square.word_multiplier == 2) {
                    out << BG_COLOR_WORD_MULTIPLIER_2X;
                } else if (square.word_multiplier == 3) {
                    out << BG_COLOR_WORD_MULTIPLIER_3X;
                } else if (square.letter_multiplier == 2) {
                    out << BG_COLOR_LETTER_MULTIPLIER_2X;
                } else if (square.letter_multiplier == 3) {
                    out << BG_COLOR_LETTER_MULTIPLIER_3X;
                } else if (is_start) {
                    out << BG_COLOR_START_SQUARE;
                }

                // Text
                if (line == 0 && is_start) {
                    out << "  \u2605  ";
                } else if (line == 0 && square.word_multiplier > 1) {
                    out << FG_COLOR_MULTIPLIER << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << 'W' << std::setw(1)
                        << square.word_multiplier;
                } else if (line == 0 && square.letter_multiplier > 1) {
                    out << FG_COLOR_MULTIPLIER << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << 'L' << std::setw(1)
                        << square.letter_multiplier;
                } else if (line == 1 && square.has_tile()) {
                    char l = square.get_tile_kind().letter == TileKind::BLANK_LETTER ? square.get_tile_kind().assigned
                                                                                     : ' ';
                    out << repeat(SPACE, 2) << FG_COLOR_LETTER << square.get_tile_kind().letter << l
                        << repeat(SPACE, 1);
                } else if (line == SQUARE_INNER_HEIGHT - 1 && square.has_tile()) {
                    out << repeat(SPACE, SQUARE_INNER_WIDTH - 1) << FG_COLOR_SCORE << square.get_points();
                } else {
                    out << repeat(SPACE, SQUARE_INNER_WIDTH);
                }
            }

            // Add vertical line
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL << BG_COLOR_OUTSIDE_BOARD << std::endl;
        }
    }

    // Draw bottom line
    out << repeat(SPACE, BOARD_LEFT_MARGIN);
    print_horizontal(this->columns, L_BOTTOM_LEFT, T_UP, L_BOTTOM_RIGHT, out);
    out << endl << rang::style::reset << std::endl;
}
