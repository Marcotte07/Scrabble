#include "player.h"

using namespace std;

void Player::add_points(size_t points) { this->points += points; }

// Subtracts points from player's score
void Player::subtract_points(size_t points) {
    if (points > this->points) {
        this->points = 0;
    } else {
        this->points -= points;
    }
}

size_t Player::get_points() const { return points; }

const std::string& Player::get_name() const { return name; }

size_t Player::count_tiles() const { return this->tiles.count_tiles(); }

// Removes tiles from player's hand
// Will pass in list of tiles without any ?. if there is a tile thats
// not in hand, it is a ? so remove that
void Player::remove_tiles(const std::vector<TileKind>& tiles) {

    for (size_t i = 0; i < tiles.size(); i++) {
        // If tile isnt in hand  but there is a ? in tiles, remove ? tile from hand
        try {
            // the players tile collection doesn't have the tile
            if (!this->has_tile(tiles[i])) {
                // This will throw exception if not found
                this->tiles.remove_tile(this->tiles.lookup_tile('?'));
            } else {
                this->tiles.remove_tile(tiles[i]);
            }
        }

        catch (out_of_range& e) {
            throw MoveException("Error in tiles played");
        }
    }
}

// Adds tiles to player's hand.
void Player::add_tiles(const std::vector<TileKind>& tiles) {
    for (size_t i = 0; i < tiles.size(); i++) {
        this->tiles.add_tile(tiles[i]);
    }
}

bool Player::has_tile(TileKind tile) {
    try {
        TileKind returnTile = tiles.lookup_tile(tile.letter);
        return true;
    } catch (out_of_range& e) {
        return false;
    }
}

unsigned int Player::get_hand_value() const { return tiles.total_points(); }

size_t Player::get_hand_size() const { return this->hand_size; }
