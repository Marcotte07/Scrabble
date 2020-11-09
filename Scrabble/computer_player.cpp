
#include "computer_player.h"

#include "exceptions.h"
#include "formatting.h"
#include "move.h"
#include "place_result.h"
#include "rang.h"
#include "tile_kind.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

void ComputerPlayer::left_part(
        Board::Position anchor_pos,
        std::string partial_word,
        Move partial_move,
        std::shared_ptr<Dictionary::TrieNode> node,
        size_t limit,
        TileCollection& remaining_tiles,
        std::vector<Move>& legal_moves,
        const Board& board) const {
    // This function finds all possible starting prefixes for an anchor of size less than or equal to the anchor’s
    // limit. It does this by searching through the dictionary trie, building possible prefixes based on the tiles still
    // available. For every prefix that can be made, it should call extend_right with that prefix. Starting at node
    // root, build all possible combination of prefixes and extend right start at node, for each and every tile in hand
    // add check if from node there is a path to the next letter,

    // For all members of the node map, recurse if and only if remaining tiles has that letter

    // Base case when limit is 0
    if (limit == 0) {
        return;
    }

    // Iterate through all members of the map in node
    std::map<char, std::shared_ptr<Dictionary::TrieNode>>::iterator letterIterator;
    for (letterIterator = node->nexts.begin(); letterIterator != node->nexts.end(); letterIterator++) {
        // Add character to partial_word, tile to partial_move if tile is in hand
        try {
            TileKind tileInHand = remaining_tiles.lookup_tile(letterIterator->first);

            // Move needs to be updated to start from where first tile is placed
            partial_move.tiles.push_back(tileInHand);
            if (partial_move.direction == Direction::DOWN) {
                partial_move.row--;
            } else {
                partial_move.column--;
            }

            partial_word += letterIterator->first;

            // Remove tile from hand
            remaining_tiles.remove_tile(tileInHand);

            // call extend_right for new prefix
            extend_right(
                    anchor_pos,
                    anchor_pos,
                    partial_word,
                    partial_move,
                    letterIterator->second,
                    remaining_tiles,
                    legal_moves,
                    board);

            // then recurse to get new prefix, with new node, new limit
            left_part(
                    anchor_pos,
                    partial_word,
                    partial_move,
                    letterIterator->second,
                    limit - 1,
                    remaining_tiles,
                    legal_moves,
                    board);

            // Now done with all those prefixes, add tile back to hand and go to next iteration
            remaining_tiles.add_tile(tileInHand);
            partial_move.tiles.pop_back();
            if (partial_move.direction == Direction::DOWN) {
                partial_move.row++;
            } else {
                partial_move.column++;
            }

            partial_word.pop_back();
        } catch (std::exception& e) {
            // Go to next letter in map, only if no blank tiles in hand, if blank tile, then use that as the letter and
            // do same as above
            try {
                TileKind blankInHand = remaining_tiles.lookup_tile('?');
                TileKind newTile(letterIterator->first, blankInHand.points);
                partial_move.tiles.push_back(newTile);
                if (partial_move.direction == Direction::DOWN) {
                    partial_move.row--;
                } else {
                    partial_move.column--;
                }

                partial_word += letterIterator->first;

                // Remove tile from hand
                remaining_tiles.remove_tile(blankInHand);

                // call extend_right for new prefix
                extend_right(
                        anchor_pos,
                        anchor_pos,
                        partial_word,
                        partial_move,
                        letterIterator->second,
                        remaining_tiles,
                        legal_moves,
                        board);

                // then recurse to get new prefix, with new node, new limit
                left_part(
                        anchor_pos,
                        partial_word,
                        partial_move,
                        letterIterator->second,
                        limit - 1,
                        remaining_tiles,
                        legal_moves,
                        board);

                // Now done with all those prefixes, add tile back to hand and go to next iteration
                remaining_tiles.add_tile(blankInHand);
                partial_move.tiles.pop_back();
                if (partial_move.direction == Direction::DOWN) {
                    partial_move.row++;
                } else {
                    partial_move.column++;
                }

                partial_word.pop_back();

            } catch (std::exception& e) {
            }
        }
    }
}

void ComputerPlayer::extend_right(
        Board::Position square,
        Board::Position anchor_pos,
        std::string partial_word,
        Move partial_move,
        std::shared_ptr<Dictionary::TrieNode> node,
        TileCollection& remaining_tiles,
        std::vector<Move>& legal_moves,
        const Board& board) const {

    // Words need to make it back to the anchor position at least, and be final
    if (node->is_final) {
        if (partial_move.direction == Direction::ACROSS) {
            if (partial_move.column + partial_word.size() > anchor_pos.column) {
                legal_moves.push_back(partial_move);
            }
        } else {
            if (partial_move.row + partial_word.size() > anchor_pos.row) {
                legal_moves.push_back(partial_move);
            }
        }
    }

    // BASE CASE if map is empty or if position is out of bounds
    if (node->nexts.empty() || !board.is_in_bounds(square)) {
        return;
    }

    // TILE ON BOARD IN THAT POSITION
    if (board.in_bounds_and_has_tile(square)) {
        // Already a tile on board in that position, if that letter is not in the node->map, then return
        char c = board.letter_at(square);
        if (node->nexts.find(c) == node->nexts.end()) {
            // Not a valid word
            return;
        } else {
            // Add tile and letter to move and word respectively, because it is a valid tile, could be words after
            partial_word += c;
            // Recurse again but with new node
            // Direction is down
            if (partial_move.direction == Direction::DOWN) {
                Board::Position newPos(square.row + 1, square.column);
                extend_right(
                        newPos,
                        anchor_pos,
                        partial_word,
                        partial_move,
                        node->nexts.find(c)->second,
                        remaining_tiles,
                        legal_moves,
                        board);
            }
            // Direction is across
            else {
                Board::Position newPos(square.row, square.column + 1);
                extend_right(
                        newPos,
                        anchor_pos,
                        partial_word,
                        partial_move,
                        node->nexts.find(c)->second,
                        remaining_tiles,
                        legal_moves,
                        board);
            }
        }
    }

    // NO TILE ON BOARD IN THAT POSITION
    else {
        std::map<char, std::shared_ptr<Dictionary::TrieNode>>::iterator letterIterator;
        for (letterIterator = node->nexts.begin(); letterIterator != node->nexts.end(); letterIterator++) {
            // iterate through map, recurse for each tile just like in left side
            try {
                TileKind tileInHand = remaining_tiles.lookup_tile(letterIterator->first);
                partial_move.tiles.push_back(tileInHand);
                partial_word += letterIterator->first;

                // Remove tile from hand
                remaining_tiles.remove_tile(tileInHand);

                // call extend_right for new prefix
                if (partial_move.direction == Direction::DOWN) {
                    Board::Position newPos(square.row + 1, square.column);
                    extend_right(
                            newPos,
                            anchor_pos,
                            partial_word,
                            partial_move,
                            letterIterator->second,
                            remaining_tiles,
                            legal_moves,
                            board);
                } else {
                    Board::Position newPos(square.row, square.column + 1);
                    extend_right(
                            newPos,
                            anchor_pos,
                            partial_word,
                            partial_move,
                            letterIterator->second,
                            remaining_tiles,
                            legal_moves,
                            board);
                }
                // Now done with recursive calls, add tile just removed back to hand and move to next letter in nexts
                // map
                remaining_tiles.add_tile(tileInHand);
                partial_move.tiles.pop_back();
                partial_word.pop_back();
            }

            catch (std::exception& e) {
                // The letter not found in hand, see if blank tile
                try {
                    TileKind blankInHand = remaining_tiles.lookup_tile('?');
                    TileKind newTile(letterIterator->first, blankInHand.points);
                    partial_move.tiles.push_back(newTile);
                    partial_word += letterIterator->first;

                    // Remove tile from hand
                    remaining_tiles.remove_tile(blankInHand);

                    // call extend_right for new prefix
                    if (partial_move.direction == Direction::DOWN) {
                        Board::Position newPos(square.row + 1, square.column);
                        extend_right(
                                newPos,
                                anchor_pos,
                                partial_word,
                                partial_move,
                                letterIterator->second,
                                remaining_tiles,
                                legal_moves,
                                board);
                    } else {
                        Board::Position newPos(square.row, square.column + 1);
                        extend_right(
                                newPos,
                                anchor_pos,
                                partial_word,
                                partial_move,
                                letterIterator->second,
                                remaining_tiles,
                                legal_moves,
                                board);
                    }
                    // Now done with recursive calls, add tile just removed back to hand and move to next letter in
                    // nexts map
                    remaining_tiles.add_tile(blankInHand);
                    partial_move.tiles.pop_back();
                    partial_word.pop_back();
                } catch (std::exception& e) {
                }
            }
        }
    }
}

Move ComputerPlayer::get_move(const Board& board, const Dictionary& dictionary) const {
    std::vector<Move> legal_moves;
    std::vector<Board::Anchor> anchors = board.get_anchors();

    board.print(std::cout);
    print_hand(std::cout);

    for (size_t i = 0; i < anchors.size(); i++) {
        // Call left part on each and every anchor, need to initialize a move

        std::vector<TileKind> tiles;
        size_t row = anchors[i].position.row;
        size_t column = anchors[i].position.column;
        Move buildMove(tiles, row, column, anchors[i].direction);

        // First move, limit is hand size - 1
        if (row == board.start.row && column == board.start.column) {
            anchors[i].limit = get_hand_size() - 1;
        }

        TileCollection copyTiles = this->tiles;

        // Limit is 0, no call to left_part, find if tiles to left or up (depending on direction) and call extend_right
        // on that
        if (anchors[i].limit == 0) {
            // Build move from tiles already on board
            std::string partial_word = "";
            if (anchors[i].direction == Direction::DOWN) {
                Board::Position up(row - 1, column);
                while (board.in_bounds_and_has_tile(up)) {
                    up.row--;
                }
                // Now at last position with tile, need to get full string and pass to extend_right
                up.row++;
                while (board.in_bounds_and_has_tile(up)) {
                    partial_word = partial_word + board.letter_at(up);
                    up.row++;
                }
            }
            // Direction Across
            else {
                Board::Position left(row, column - 1);
                while (board.in_bounds_and_has_tile(left)) {
                    left.column--;
                }
                // Now at last position with tile, need to get full string and pass to extend_right
                left.column++;
                while (board.in_bounds_and_has_tile(left)) {
                    partial_word = partial_word + board.letter_at(left);
                    left.column++;
                }
            }
            // Now have prefix in partial_word, need to make sure its valid in trie before passing to extend_right
            if (dictionary.find_prefix(partial_word) != nullptr) {
                extend_right(
                        anchors[i].position,
                        anchors[i].position,
                        partial_word,
                        buildMove,
                        dictionary.find_prefix(partial_word),
                        copyTiles,
                        legal_moves,
                        board);
            }
        }

        // Limit is not 0, need to get all valid prefixes before extending right
        else {
            left_part(
                    anchors[i].position,
                    "",
                    buildMove,
                    dictionary.find_prefix(""),
                    anchors[i].limit,
                    copyTiles,
                    legal_moves,
                    board);
        }
    }

    return get_best_move(legal_moves, board, dictionary);
}

Move ComputerPlayer::get_best_move(
        std::vector<Move> legal_moves, const Board& board, const Dictionary& dictionary) const {
    Move best_move = Move();  // Pass if no move found
    size_t mostPoints = 0;
    for (size_t i = 0; i < legal_moves.size(); i++) {
        // test place all moves on board to find which has most points
        PlaceResult testPlace = board.test_place(legal_moves[i]);

        // Need to check if all words formed in testPlace are valid, if so then valid move
        bool validPlace = true;
        for (size_t j = 0; j < testPlace.words.size(); j++) {
            if (!dictionary.is_word(testPlace.words[j])) {
                validPlace = false;
            }
        }
        if (testPlace.points > mostPoints && validPlace && legal_moves[i].tiles.size() >= 2) {
            mostPoints = testPlace.points;
            best_move = legal_moves[i];
        }
        // First move, make it start at center
        if (!board.in_bounds_and_has_tile(board.start)) {
            best_move.row = board.start.row;
            best_move.column = board.start.column;
            best_move.direction = Direction::ACROSS;
        }
    }
    return best_move;
}

void ComputerPlayer::print_hand(std::ostream& out) const {
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
