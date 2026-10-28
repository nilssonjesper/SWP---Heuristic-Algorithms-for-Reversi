#ifndef REVERSIX_REVERSI_H
#define REVERSIX_REVERSI_H

#include <cassert>
#include <cstdint>

/// Generic *Reversi plus X* interfaces, static variables and implementations
namespace reversi {
  /// Alias for player id type ranging from 0 to 7.
  using player_t = uint_fast8_t;

  /// Encodes map dimensions (width or height.)
  using dim_t = uint_fast8_t;

  /// Encodes an arbitrary location on the board.
  using pos_t = uint_fast16_t;

  /// The enumeration of game phases.
  enum class phase {
    build,      ///< Phase during which players can place stones.
    elimination ///< Phase during which players can place bombs.
  };

  /// The maximum amount of players in a session.
  static constexpr uint_fast8_t max_player_count = 8;
  /// The maximum permitted width of a map.
  static constexpr dim_t max_map_width = 50;
  /// The maximum permitted height of a map.
  static constexpr dim_t max_map_height = 50;

  /// The enumeration of all possible tile types.
  enum class tile {
    hole = '-',       ///< Hole
    player_one = '1', ///< Tile occupied by stone of player one
    player_two,       ///< Tile occupied by stone of player two
    player_three,     ///< Tile occupied by stone of player three
    player_four,      ///< Tile occupied by stone of player four
    player_five,      ///< Tile occupied by stone of player five
    player_six,       ///< Tile occupied by stone of player six
    player_seven,     ///< Tile occupied by stone of player seven
    player_eight,     ///< Tile occupied by stone of player eight
    bonus = 'b',      ///< Bonus tile
    choice = 'c',     ///< Choice tile
    inversion = 'i',  ///< Inversion tile
    expansion = 'x'   ///< Tile occupied by expansion stone
  };
  /// Converts a tile to the corresponding player id using the identity map.
  /** @param tile Player-occupied tile of which to extract the occupying
   *  player's id from.
   *  @note Does not return the player id based on the color map resulting from
   *  inversions, etc. but rather the layer's index that is responsible for
   *  encoding the given tile type.
   *  @returns The index of the layer encoding the given tile type as a player
   *  id.
   */
  static constexpr player_t pid(enum tile tile) {
    assert('1' <= static_cast<char>(tile) and '8' >= static_cast<char>(tile));

    return static_cast<player_t>(tile) - 49;
  }
  /// Converts a player id to the corresponding tile using the identity map.
  /** @param player Player of whom to return the tile type.
   *  @note Does not return the tile type based on the color map resulting from
   *  inversions, etc. but rather the layer's index that is responsible for
   *  encoding the player's tile type.
   *  @returns The index of the layer encoding the given player's tile type.
   */
  static constexpr enum tile tile(player_t player) {
    assert(player < 8);

    return static_cast<enum tile>(player + 49);
  }
}

#endif

