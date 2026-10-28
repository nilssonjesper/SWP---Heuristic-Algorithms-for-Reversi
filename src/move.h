#ifndef REVERSIX_MOVE_H
#define REVERSIX_MOVE_H

#include <cstdint>
#include <functional>

#include "reversi.h"

namespace reversi {
  /// A structure representing an arbitrary move.
  struct move {
    public:
      /// Encodes a choice for a special field.
      using special_t = uint_fast8_t;

      /// The enumeration of possible bonus choices.
      enum class bonus {
        bomb = 0x14,           ///< Choice of receiving an extra bomb.
        override_stone = 0x15, ///< Choice of receiving an extra override stone.
      };

    private:
      /// Encodes position and player of move.
      /** Unsigned 32-bit integer with the following structure:
       *  - 3 LSBs encode the attacking player
       *  - 12 bits are used to encode the position of the target tile
       *  - 5 bits are reserved for the special field
       */
      uint_fast32_t data_ = 0;

    public:
      constexpr move() = default;

      /// Constructs a move.
      /** @param player Player applying the move.
       *  @param pos Location of the target tile.
       *  @param special Special field data.
       */
      constexpr move(player_t player, pos_t pos, special_t special = 0) noexcept
          : data_(player | pos << 0x3 | special << 0xF) {}

      /** @returns Player applying the move. */
      [[gnu::always_inline]] constexpr player_t player() const noexcept {
        return static_cast<player_t>(data_ & 0x7);
      }

      /** @returns Location of the target tile. */
      [[gnu::always_inline]] constexpr pos_t pos() const noexcept {
        return static_cast<pos_t>(data_ >> 0x3) & 0xFFF;
      }

      /** @returns Special field data. */
      [[gnu::always_inline]] constexpr special_t special() const noexcept {
        return static_cast<special_t>(data_ >> 0xF) & 0x1F;
      }

      /// @cond DOXYGEN_SHOULD_SKIP_THIS
      [[gnu::always_inline]] constexpr bool
      operator==(const move& rhs) const noexcept {
        return data_ == rhs.data_;
      }

      friend std::hash<move>;
      /// @endcond
  };
}

/// Hashing utility for moves.
template <>
struct std::hash<reversi::move> {
  public:
    /** @param move Move which to hash.
     *  @returns Unique hash of move.
     */
    size_t operator()(reversi::move move) const noexcept { return move.data_; }
};

#endif

