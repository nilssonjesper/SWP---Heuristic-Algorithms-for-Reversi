#ifndef REVERSIX_MAP_H
#define REVERSIX_MAP_H

#include <algorithm>
#include <cstdint>
#include <iostream>

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "move.h"

namespace reversi {
  /// Type used to represent layers of maps with (less than) 64 tiles.
  using micro_layer = uint64_t;
  /// Type used to represent layers of maps with 65 - 1024 tiles.
  using meso_layer = std::bitset<1024>;
  /// Type used to represent layers of maps with more than 1024 tiles.
  using macro_layer = std::bitset<2560>;

  /// Represents the full game board state using layered tile encodings.
  /** This class is a generic container designed to efficiently encode the state
   *  of a tile-based game board using multiple binary layers. Each layer is
   *  used to represent a distinct category of a tile, such as player stones,
   *  special tiles and holes.
   *  @tparam T The type used to represent individual layers. This type is
   *  constrained to either a micro_layer or a macro_layer, which both support
   *  bitwise operations and efficient indexing.
   */
  template <typename T>
  class map {
    public:
      /// The type used to represent a layer's internal data.
      using layer_t = T;

      /// Represents a single binary layer in the map's layered encoding.
      /** A `layer` provides a 2-dimensional view over a sequence of bits and
       *  supports classic bitwise and utility operations, as well as
       *  2-dimensional spatial shifts and range checks.\n Each layer encodes
       *  the presence or absence of a specific feature at every position on the
       *  board. Therefore, layers shall always be sized accordingly to the
       *  parent map's dimensions and should not be manipulated independently of
       *  the map.
       */
      class layer {
        private:
          /// Internal data
          /** All bits out-of-bounds are defined as 0. */
          layer_t data_;

          /** @name Data Sanitization
           *  @{ */
          // TODO: Only compute masks required by the map (make mask(s) static
          // member of map and compute during first move)
          /// @cond DOXYGEN_SHOULD_SKIP_THIS
          /// Masks for defined bits of micro layers computed prior to server
          /// connection.
          /*static micro_layer micro_data_mask_;
          static meso_layer meso_data_mask_;*/
          /// Masks for defined bits of macro layers computed prior to server
          /// connection.
          // static macro_layer macro_data_mask_;
          /// @endcond

          /// Sanitizes out-of-bound bits
          /** Resets all bits which exceed the defined area.
           *  O(1).
           */
          [[gnu::always_inline]] constexpr void shrink_() noexcept {
            data_ &= data_mask_;
          }
          /** @} */

          /** @name Border-wrap Masking
           *  @{ */
          /// @cond DOXYGEN_SHOULD_SKIP_THIS

          /// Generates left-edge-wrap mask.
          /** O(1)
           *  @param s Exponent to the base of 2 of width of mask.
           *  @param width The width of the mask.
           *  @param height The height of the mask.
           */
          [[nodiscard]] [[gnu::always_inline]] static constexpr layer_t
          wrap_mask_r_(dim_t s) noexcept {
            assert(0 < s and s <= 64);

            return wrap_masks_r_[31 - __builtin_clz(s)];
          }
          /// Generates right-edge-wrap mask.
          /** O(1)
           *  @param s Exponent to the base of 2 of width of mask.
           *  @param width The width of the mask.
           *  @param height The height of the mask.
           */
          [[nodiscard]] [[gnu::always_inline]] static constexpr layer_t
          wrap_mask_l_(dim_t s) noexcept {
            assert(0 < s and s <= 64);

            return wrap_masks_l_[31 - __builtin_clz(s)];
          }
          /// @endcond
          /** @} */

        public:
          /// Defines the capacity of tiles which the type of layer can encode.
          static constexpr uint_fast16_t capacity =
            std::is_same_v<layer_t, micro_layer>  ? 64
            : std::is_same_v<layer_t, meso_layer> ? 1024
                                                  : 2500;

          /// Constructs an empty layer.
          layer() noexcept : data_(0ull) {}
          /// Constructs a layer from the given data.
          /** @param data Internal data. */
          constexpr layer(layer_t data) noexcept : data_(data) {}

          /** @returns The internal data of the layer. */
          [[nodiscard]] [[gnu::always_inline]] constexpr const layer_t&
          data() const noexcept {
            return data_;
          }

          /** @name Iterators
           *  @{ */
          /// Provides a fast iterator over the set bits of a layer.
          class iterator {
            private:
              /// Pointer to layer's internal data.
              const layer_t* data_ptr_;

              /// Current location of the bit pointed at by the iterator.
              int_fast32_t pos_;

              /// Advances iterator to the first set bit starting from the
              /// iterator's current position.
              /** Yields if sentinel is reached. */
              constexpr void next_() noexcept {
                if constexpr (std::is_same_v<layer_t, macro_layer> ||
                              std::is_same_v<layer_t, meso_layer>)
                  while (pos_ >= 0 && !(*data_ptr_).test(pos_)) --pos_;
                else if constexpr (std::is_same_v<layer_t, micro_layer>) {
                  const uint64_t data =
                    (*data_ptr_ & ((1ull << (pos_ + 1)) - 1));
                  pos_ = data == 0 ? sentinel : 63 - __builtin_clzll(data);
                }
              }

            public:
              /// Sentinel location marking the end of the layer.
              static constexpr int_fast32_t sentinel = -1;

              /// Constructs an iterator for a given layer.
              /** @param data Pointer to the internal data of the layer.
               *  @param pos The position of the starting bit.
               */
              constexpr iterator(const layer_t* data, pos_t pos) noexcept
                  : data_ptr_(data), pos_(pos) {
                if constexpr (std::is_same_v<layer_t, micro_layer>) {
                  if (pos_ == 63)
                    pos_ = *data_ptr_ == 0 ? sentinel
                                           : 63 - __builtin_clzll(*data_ptr_);
                  else next_();
                } else next_();
              }

              /** @returns The position of the bit currently pointed at by the
               *  iterator.
               */
              [[nodiscard]] constexpr pos_t operator*() const noexcept {
                return pos_;
              }

              /** @param rhs Position to check against.
               *  @returns True iff the position of the bit currently pointed at
               *  by the iterator does not equal the right-hand side value.
               */
              constexpr bool operator!=(const int_fast32_t rhs) const noexcept {
                return pos_ != rhs;
              }

              /// Advances the iterator to the next set bit.
              /** Yields if sentinel is reached.
               *  @returns Iterator pointing to the next set bit.
               */
              constexpr iterator& operator++() noexcept {
                if (pos_ < 0) return *this;

                --pos_;
                next_();
                return *this;
              }
          };
          /** @returns Iterator to the beginning of the layer. */
          [[nodiscard]] [[gnu::always_inline]] constexpr iterator
          begin() const noexcept {
            return iterator(&data_, area() - 1);
          }
          /** @returns Iterator pointing to #iterator::sentinel. */
          [[nodiscard]] [[gnu::always_inline]] constexpr int_fast32_t
          end() const noexcept {
            return iterator::sentinel;
          }
          /** @} */

          /** @name Modifiers
           *  @{ */
          /// Sets all defined bits to `1`.
          [[gnu::always_inline]] constexpr void set() noexcept {
            if constexpr (std::is_same_v<layer_t, macro_layer> ||
                          std::is_same_v<layer_t, meso_layer>)
              data_.set();
            else data_ = ~0ull;

            shrink_();
          }
          /// Sets bit at given position to `1`.
          /** @param position Position at which to set bit. */
          [[gnu::always_inline]] constexpr void set(pos_t position) noexcept {
            assert(position < area());

            if constexpr (std::is_same_v<layer_t, macro_layer> ||
                          std::is_same_v<layer_t, meso_layer>)
              data_.set(position);
            else data_ |= layer_t(1) << position;
          }

          /// Sets bit at given position to `0`.
          /** @param position Position at which to set bit. */
          [[gnu::always_inline]] constexpr void reset(pos_t position) noexcept {
            assert(position < area());

            if constexpr (std::is_same_v<layer_t, macro_layer> ||
                          std::is_same_v<layer_t, meso_layer>)
              data_.reset(position);
            else data_ &= ~(layer_t(1) << position);
          }

          /// @cond DOXYGEN_SHOULD_SKIP_THIS
          [[gnu::always_inline]] constexpr bool
          operator==(const layer& rhs) const noexcept {
            return data_ == rhs.data_;
          }
          [[gnu::always_inline]] constexpr layer&
          operator|=(const layer_t& rhs) noexcept {
            data_ |= rhs;
            return *this;
          }
          [[gnu::always_inline]] constexpr layer&
          operator&=(const layer_t& rhs) noexcept {
            data_ &= rhs;
            return *this;
          }
          [[gnu::always_inline]] constexpr layer&
          operator|=(const layer& rhs) noexcept {
            data_ |= rhs.data_;
            return *this;
          }
          [[gnu::always_inline]] constexpr layer&
          operator&=(const layer& rhs) noexcept {
            data_ &= rhs.data_;
            return *this;
          }
          [[gnu::always_inline]] constexpr layer
          operator|(const layer& rhs) const noexcept {
            return { data_ | rhs.data_ };
          }
          [[gnu::always_inline]] constexpr layer
          operator&(const layer& rhs) const noexcept {
            return { data_ & rhs.data_ };
          }
          [[gnu::always_inline]] constexpr layer
          operator^(const layer& rhs) const noexcept {
            return { data_ ^ rhs.data_ };
          }
          [[gnu::always_inline]] constexpr layer&
          operator<<=(pos_t s) noexcept {
            data_ <<= s;

            shrink_();
            return *this;
          }
          [[gnu::always_inline]] constexpr layer&
          operator>>=(pos_t s) noexcept {
            data_ >>= s;
            return *this;
          }
          [[gnu::always_inline]] constexpr layer operator~() const noexcept {
            layer res { ~data_ };

            res.shrink_();
            return res;
          }
          [[gnu::always_inline]] constexpr layer_t
          operator|(const layer_t& data) const noexcept {
            return data_ | data;
          }
          [[gnu::always_inline]] constexpr layer_t
          operator&(const layer_t& data) const noexcept {
            return data_ & data;
          }
          [[gnu::always_inline]] constexpr layer_t
          operator<<(pos_t s) const noexcept {
            return (data_ << s) & data_mask_;

            __builtin_unreachable();
          }
          [[gnu::always_inline]] constexpr layer_t
          operator>>(pos_t s) const noexcept {
            return data_ >> s;
          }
          /// @endcond
          /** @} */

          /** @name Element Access
           *  @{ */
          /// Tests bit at given position.
          /** @param position Position of bit to test.
           *  @return Value of bit.
           */
          [[nodiscard]] [[gnu::always_inline]] constexpr bool
          test(pos_t position) const noexcept {
            assert(position < area());

            if constexpr (std::is_same_v<layer_t, micro_layer>)
              return (data_ & layer_t(1) << position) != 0;
            else return data_.test(position);
          }

          /// Tests if any bits of layer are set.
          /** @return True iff any defined bit of layer is set */
          [[gnu::always_inline]] explicit constexpr operator bool() const {
            if constexpr (std::is_same_v<layer_t, micro_layer>)
              return data_ != 0ull;
            else return data_.any();
          }

          /** @returns The amount of defined bits that are set. */
          [[nodiscard]] [[gnu::always_inline]] constexpr pos_t count() const {
            if constexpr (std::is_same_v<layer_t, micro_layer>)
              return __builtin_popcountll(data_);
            else return data_.count();
          }

          /** @returns Position of the first (defined) set bit.
           *  @note Exhibits undefined behaviour if the underlying data is equal
           *  to zero.
           */
          [[nodiscard]] constexpr pos_t first() const {
            assert(*this);

            if constexpr (std::is_same_v<layer_t, micro_layer>)
              if (area() == capacity) return __builtin_ctzll(data_);
              else return __builtin_ctzll(data_ & ((1ull << area()) - 1));
            else
              for (pos_t i = 0; i < area(); ++i)
                if (data_[i]) return i;

            __builtin_unreachable();
          }

          // TODO: make this computationally faster
          /// Computes the set bits of this layer neighboring the set
          /// bits of a separate layer.
          /** O(1).
           *  @returns Set bits neighboring set bits of \p rhs.
           */
          [[nodiscard]] constexpr layer neighbors() const {
            layer mask = *this;
            mask |= mask.shift_x_l(1) | mask.shift_x_r(1) |
              mask.shift_tlbr_l(width_ - 1, width_ - 1) |
              mask.shift_tlbr_r(width_ - 1, width_ - 1) |
              mask.shift_trbl_l(width_ + 1, width_ + 1) |
              mask.shift_trbl_r(width_ + 1, width_ + 1) | mask << width_ |
              mask >> width_;

            return mask & ~(*this);
          }
          /** @} */

          /** @name 2-dimensional Shifting
           *  @{ */
          /// @cond DOXYGEN_SHOULD_SKIP_THIS
          /// 2-dimensional horizontal right shift.
          /** Shifts bits of layer to the right and applies right edge-wrap
           *  mask.
           *  @param distance Distance by which to shift.
           *  @returns Resulting layer
           */
          [[using gnu: always_inline]] constexpr layer
          shift_x_r(pos_t distance) const noexcept {
            return { data_ >> distance & wrap_mask_r_(distance) };
          }
          /// 2-dimensional horizontal left shift.
          /** Shifts bits of layer to the left and applies left edge-wrap mask.
           *  @param distance Distance by which to shift.
           *  @returns Resulting layer
           */
          [[using gnu: always_inline, hot]] constexpr layer
          shift_x_l(pos_t distance) const noexcept {
            return { data_ << distance & wrap_mask_l_(distance) };
          }
          /// \todo Remove branching!!!
          /// 2-dimensional diagonal bottom-right shift.
          /** Shifts bits of layer to the right and applies top-right
           *  bottom-left edge-wrap mask.
           *  @param distance Distance by which to shift.
           *  @param limit Normalization value for the width of the edge-wrap
           *  mask.
           *  @returns Resulting layer
           */
          [[using gnu: always_inline, hot]] constexpr layer
          shift_tlbr_r(pos_t distance, dim_t limit) const noexcept {
            return { distance < limit
                       ? (data_ >> distance)
                       : (data_ >> distance & wrap_mask_l_(distance / limit)) };
          }
          /// \todo Remove branching!!!
          /// 2-dimensional diagonal top-left shift.
          /** Shifts bits of layer to the left and applies top-right bottom-left
           *  edge-wrap mask.
           *  @param distance Distance by which to shift.
           *  @param limit Normalization value for the width of the edge-wrap
           *  mask.
           *  @returns Resulting layer
           */
          [[using gnu: always_inline, hot]] constexpr layer
          shift_tlbr_l(pos_t distance, dim_t limit) const noexcept {
            return { distance < limit
                       ? (data_ << distance)
                       : (data_ << distance & wrap_mask_r_(distance / limit)) };
          }
          /// \todo Remove branching!!!
          /// 2-dimensional diagonal top-right shift.
          /** Shifts bits of layer to the right and applies top-left
           *  bottom-right edge-wrap mask.
           *  @param distance Distance by which to shift.
           *  @param limit Normalization value for the width of the edge-wrap
           *  mask.
           *  @returns Resulting layer
           */
          [[using gnu: always_inline, hot]] constexpr layer
          shift_trbl_r(pos_t distance, dim_t limit) const noexcept {
            return { distance < limit
                       ? (data_ >> distance)
                       : (data_ >> distance & wrap_mask_r_(distance / limit)) };
          }
          /// \todo Remove branching!!!
          /// 2-dimensional diagonal bottom-left shift.
          /** Shifts bits of layer to the left and applies top-left bottom-right
           * edge-wrap mask.
           *  @param distance Distance by which to shift.
           *  @param limit Normalization value for the width of the edge-wrap
           *  mask.
           *  @returns Resulting layer
           */
          [[using gnu: always_inline, hot]] constexpr layer
          shift_trbl_l(pos_t distance, dim_t limit) const noexcept {
            return { distance < limit
                       ? (data_ << distance)
                       : (data_ << distance & wrap_mask_l_(distance / limit)) };
          }
          /// @endcond
          /** @} */

          /// @cond DOXYGEN_SHOULD_SKIP_THIS
          [[maybe_unused]] friend std::ostream& operator<<(std::ostream& o,
                                                           const layer& l) {
            std::string s;
            s.resize(2 * (height_ + 1) * width_ - 1);

            for (dim_t i = 0; i < height_; ++i) {
              for (dim_t j = 0; j < width_; ++j) {
                s[2 * (i * width_ + j)] = l.test(i * width_ + j) ? '1' : '0';
                s[2 * (i * width_ + j) + 1] = ' ';
              }
              s[2 * (i + 1) * width_ - 1] = '\n';
            }
            s[2 * (height_ + 1) * width_ - 1] = '\n';

            return o << s;
          }
          /// @endcond
      };

      /** @name Stone-tile table
       *  @{ */
      /// Alias for tile score type.
      using st_score_t = int_fast8_t;
      /// Maximum score which may be attributed to a tile.
      static constexpr st_score_t st_max_tile_score = 5;
      /** @} */

    private:
      /** @name Data Sanitization
       *  @{ */
      /// Mask for sanitizing out-of-bound bits of layers.
      static inline layer_t data_mask_ {};
      /// @cond DOXYGEN_SHOULD_SKIP_THIS
      static inline std::array<layer_t, 6> wrap_masks_r_ {}, wrap_masks_l_ {};
      /// @endcond
      /** @} */

      /// Layer marking all border tiles.
      static inline layer border_ {};

      /// The number of players for the map.
      static inline uint_fast8_t player_count_ = 0;

      /// The area-of-effect (strength) of each individual bomb.
      static inline dim_t bomb_radius_ = 0;

      /// The width of the map.
      static inline dim_t width_ = 0;

      /// The height of the map.
      static inline dim_t height_ = 0;

      /// An array holding an entry for every tile type.
      static constexpr std::array<enum tile, 13> tile_types_ {
        tile::hole,         tile::player_one,   tile::player_two,
        tile::player_three, tile::player_four,  tile::player_five,
        tile::player_six,   tile::player_seven, tile::player_eight,
        tile::bonus,        tile::choice,       tile::inversion,
        tile::expansion
      };

      /** @name Orientation
       *  @{ */
      /// The enumeration of all possible propagation directions.
      enum class direction {
        N,  ///< North
        NE, ///< North East
        E,  ///< East
        SE, ///< South East
        S,  ///< South
        SW, ///< South West
        W,  ///< West
        NW  ///< North West
      };

      /** @param direction Direction encoded as 8-bit unsigned integer.
       *  @returns 8-bit encoding of mirrored direction.
       *  @code
       *  mirror_direction_(0); // = 4
       *  @endcode
       */
      [[gnu::always_inline]] static constexpr uint_fast8_t
      mirror_direction_(uint_fast8_t direction) noexcept {
        return (direction + 4) & 7;
      }

      /** @param direction Direction to mirror.
       *  @returns Mirrored direction.
       *  @code
       *  mirror_direction_(direction::N); // = direction::S
       *  @endcode
       */
      [[gnu::always_inline]] static constexpr direction
      mirror_direction_(direction direction) noexcept {
        return static_cast<enum direction>(
          mirror_direction_(static_cast<uint_fast8_t>(direction)));
      }
      /** @} */

      /** @name Transitions
       *  @{ */
      /// An utility class for transition codec.
      /** Provides functions for extracting positions and directions from
       *  transition domains. */
      class transition {
        public:
          /// Encodes a transition domain.
          /** Unsigned 16-bit integer where most significant bit is reserved, 12
           *  bits encode the position and 3 bits encode the direction of the
           *  transition domain.
           */
          using domain_t = uint_fast16_t;

          /// Encodes an arbitrary transition domain.
          /** @param position Position of the transition domain.
           *  @param direction Direction of the transition domain.
           *  @returns #domain_t value encoding the transition domain.
           */
          [[gnu::always_inline]] static constexpr domain_t
          encode_domain(pos_t position, direction direction) noexcept {
            return (static_cast<domain_t>(position)) |
              (static_cast<domain_t>(direction) << 12);
          }

          /** @param domain The domain from which to extract the position.
           *  @returns The position encoded within a transition domain.
           */
          [[gnu::always_inline]] static constexpr pos_t
          domain_position(domain_t domain) noexcept {
            return domain & 0xFFF;
          }

          /** @param domain The domain to decode.
           *  @returns A pair containing the position and direction of a
           * transition domain
           */
          [[gnu::always_inline]] static constexpr std::pair<pos_t, direction>
          decode_domain(domain_t domain) noexcept {
            return { domain_position(domain),
                     static_cast<direction>(domain >> 12) };
          }
      };

      /// Alias for the type of the map's transition lookup table for a single
      /// direction.
      /** An array where entry `i` represents the (optional) transition domain
       *  at position `i` on the map. */
      using transition_lut_t =
        std::array<typename transition::domain_t, layer::capacity>;
      /// Alias for the type of the map's transition lookup table.
      /** An array where entry `i` represents the transition lookup table for
       *  the `i`-th direction. */
      using oriented_transition_lut_t = std::array<transition_lut_t, 8>;
      /// The map's transition lookup table.
      /** A 2-dimensional array where the first index represents the direction
       *  and the second index represents the position of the transition domain
       *  for which the codomain is to be looked up.\n This member is
       *  copy-on-write and remains unmodified after #map::init.
       */
      static inline oriented_transition_lut_t t_lut_ {};

      /// @brief A layer encoding the position of every tile without any
      /// incoming or outgoing transitions in any direction.
      static inline layer n_t_domains_ {};
      /// @brief An array where entry `i` represents the layer encoding the
      /// position of every tile without any incoming or outgoing transitions in
      /// the `i`-th direction.
      static inline std::array<layer, 8> n_oriented_t_domains_ {};
      /** @} */

      /// An array mapping each player to a color.
      /** Entry at position `i` denotes the position of layer holding tiles of
       *  player with id `i`. */
      std::array<uint_fast8_t, max_player_count> color_ {
        1, 2, 3, 4, 5, 6, 7, 8
      };

      /** @name Stone-tile table
       *  @{ */
      /// Stone-tile table.
      /** Attributes a score to each map tile based on adjacency to other
       *  tiles and the map border. */
      static inline std::array<layer, st_max_tile_score> st_ {};
      /** @} */

      // TODO: use reversi::tile_count
      /// Internal map representation using binary layers for every tile type.
      std::array<layer, tile_types_.size()> map_ {};

      /// Masks every layer of this map object except for the given tile's
      /// corresponding layer.
      /** @param tile Tile whose complement layers to mask.
       *  @param mask Mask used to perform bitwise `and`.
       */
      [[gnu::hot]]
      void reset_complementary_tiles_(enum tile tile, const layer& mask) {
        for (enum tile t : tile_types_)
          if (t != tile) layer_of(t) &= mask;
      }

      /// Masks every layer of this map object except for the given player's
      /// corresponding layer.
      /** @param player Player whose complementary layers to mask.
       *  @param mask Mask used to perform bitwise `and`.
       */
      [[gnu::hot]] void reset_complementary_tiles_(player_t player,
                                                   const layer& mask) {
        assert(player < player_count_);

        for (enum tile t : tile_types_)
          if (t != tile(color_[player] - 1)) layer_of(t) &= mask;
      }

      /** @param direction Propagation direction.
       *  @returns Set of parameters uesd for *Kogge-Stone* propagation.
       */
      [[gnu::always_inline]] static constexpr std::tuple<
        layer (*)(const layer&, pos_t) noexcept, dim_t, pos_t>
      propagation_parameters_(direction direction) noexcept {
        switch (direction) {
          case direction::W:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept { return l.shift_x_r(s); },
              1, width_);
          case direction::E:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept { return l.shift_x_l(s); },
              1, width_);
          case direction::N:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept -> layer { return l >> s; },
              width_, width_* height_);
          case direction::S:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept -> layer {
                return { l << s };
              },
              width_, width_* height_);
          case direction::SW:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept {
                return l.shift_tlbr_l(s, width_ - 1);
              },
              width_ - 1, (width_ - 1) * std::max(width_, height_));
          case direction::NE:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept {
                return l.shift_tlbr_r(s, width_ - 1);
              },
              width_ - 1, (width_ - 1) * std::max(width_, height_));
          case direction::SE:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept {
                return l.shift_trbl_l(s, width_);
              },
              width_ + 1, (width_ + 1) * std::max(width_, height_));
          case direction::NW:
            return std::forward_as_tuple(
              [](const layer& l, pos_t s) noexcept {
                return l.shift_trbl_r(s, width_);
              },
              width_ + 1, (width_ + 1) * std::max(width_, height_));
        }
        __builtin_unreachable();
      }

      /// Modifies the current map object based on a given move and assumes the
      /// build phase.
      /** @param player The player performing the move.
       *  @param position The position on which to place a player stone.
       */
      [[gnu::hot]] void apply_move_build_(player_t player, pos_t position) {
        assert(player < player_count_);

        // One layer for each direction used to mark non-visited transitions.
        std::array<layer, 8> n_t_visited {};
        for (layer& n_oriented_t_visited : n_t_visited)
          n_oriented_t_visited.set();

        const layer o = opponent_layer(player), start = { T(1) << position };
        layer res {};

        // Recursive propagation algorithm for move resolution. Commences search
        // for player stone starting from target tile.
        const auto propagate = [&](const auto& self, uint_fast8_t d,
                                   const layer&& mask, const layer& p,
                                   bool transition = false) -> bool {
          // Retrieve relevant propagation parameters
          const auto [shift, bits, limit] =
            propagation_parameters_(direction(d));

          // *Kogge-Stone* algorithm
          layer gen = mask | o & shift(mask, bits);
          layer pro = o & shift(o, bits);
          for (pos_t dist = bits << 1; pro && dist < limit; dist <<= 1) {
            gen |= pro & shift(gen, dist);
            pro &= shift(pro, dist);
          }
          // Discard on capture loop
          if (transition && gen.test(position)) return false;

          bool is_enclosed_through_transition = false;

          // Retrieve non-visited transition domains encountered during
          // propagation
          const transition_lut_t& transition_mask = t_lut_[d];
          layer& n_oriented_t_visited = n_t_visited[d];

          // Handle every newly visited transition domain
          for (pos_t i :
               (gen & n_oriented_t_visited & ~(n_oriented_t_domains_)[d])) {
            const auto [codomain_position, codomain_direction] =
              transition::decode_domain(transition_mask[i]);

            n_oriented_t_visited.reset(i);

            if ((p & ~mask).test(codomain_position)) {
              is_enclosed_through_transition = true;
              break;
            }
            if (!o.test(codomain_position)) break;

            // Commence recursion
            is_enclosed_through_transition =
              self(self, uint_fast8_t(mirror_direction_(codomain_direction)),
                   { layer_t(1) << codomain_position }, p, true);
            break;
          }

          // Validate path
          if ((shift(gen, bits) & p & ~start) ||
              is_enclosed_through_transition) {
            res |= gen & o;
            return true;
          }

          return false;
        };

        /// Commence search in every direction
        for (uint_fast8_t i = 0; i < 8; ++i)
          propagate(propagate, i, std::move(start), layer_of(player));

        const layer modified = (start | res) & ~layer_of(tile::hole);

        bool inversion = layer_of(tile::inversion).test(position);

        layer_of(player) |= modified;
        reset_complementary_tiles_(player, ~modified);

        if (inversion) apply_inversion();
      }

      /// Modifies the current map object based on a given move and assumes the
      /// elimination phase.
      /** @param position The position on which to place a bomb. */
      [[gnu::hot]] void apply_move_elimination_(pos_t position) {
        assert(position < area());

        layer res = { T(1) << position }, front = res, temp_front {},
              holes = layer_of(tile::hole);

        for (dim_t i = 0; i < bomb_radius_; ++i) {
          for (pos_t j : ~(n_t_domains_ | holes))
            if (front.test(j))
              for (uint_fast8_t d = 0; d < 8; ++d)
                temp_front.set(transition::domain_position(t_lut_[d][j]));

          // Flood-fill
          temp_front |= front.shift_x_l(1) | front.shift_x_r(1) |
            front.shift_tlbr_l(width_ - 1, width_ - 1) |
            front.shift_tlbr_r(width_ - 1, width_ - 1) |
            front.shift_trbl_l(width_ + 1, width_ + 1) |
            front.shift_trbl_r(width_ + 1, width_ + 1) | front << width_ |
            front >> width_;

          temp_front &= ~(holes | res);
          res |= temp_front;

          if (!temp_front) break;
          // Shift frontier
          front = temp_front;
        }

        layer_of(tile::hole) |= res;
        reset_complementary_tiles_(tile::hole, ~res);
      }

    public:
      constexpr map() = default;

      /** @returns The width of the map. */
      [[nodiscard]] static constexpr dim_t width() noexcept { return width_; }
      /** @returns The height of the map. */
      [[nodiscard]] static constexpr dim_t height() noexcept { return height_; }
      /** @returns The defined area of the map. */
      [[nodiscard]] [[gnu::always_inline]] static constexpr pos_t
      area() noexcept {
        return static_cast<pos_t>(width_) * static_cast<pos_t>(height_);
      }

      /** @returns An immutable reference to the map's stone-tile table. */
      [[nodiscard]] [[gnu::always_inline]] static constexpr const std::array<
        layer, st_max_tile_score>&
      st() noexcept {
        return st_;
      }

      /// Casts an arbitrary position on the map to it's corresponding
      /// coordinate value.
      /** @param position Position to cast.
       *  @returns A pair containing `x` and `y` coordinate values based on the
       *  map's dimensions.
       */
      [[nodiscard]] static constexpr std::pair<dim_t, dim_t>
      coordinate(pos_t position) noexcept {
        return { static_cast<dim_t>(position % width_),
                 static_cast<dim_t>(position / width_) };
      }

      /** @returns Set containing all possible tile types */
      [[nodiscard]] static std::set<enum tile> tile_types() noexcept {
        return { std::begin(tile_types_), std::end(tile_types_) };
      }

      /** @param player Player of whom to return the corresponding layer.
       *  @returns A mutable reference to the layer belonging to the given
       *  player.
       */
      [[nodiscard]] [[gnu::always_inline]] constexpr layer&
      layer_of(player_t player) {
        assert(player < player_count_);

        return map_[color_[player]];
      }
      /** @param player Player of whom to return the corresponding layer.
       *  @returns An immutable reference to the layer belonging to the given
       *  player.
       */
      [[nodiscard]] [[gnu::always_inline]] constexpr const layer&
      layer_of(player_t player) const {
        assert(player < player_count_);

        return map_[color_[player]];
      }

      /** @param tile Tile type of which to return the corresponding layer of.
       *  @returns A mutable reference to the layer belonging to the given tile
       *  type.
       */
      [[nodiscard]] [[using gnu: always_inline, hot]] constexpr layer&
      layer_of(enum tile tile) {
        assert(std::any_of(tile_types_.begin(), tile_types_.end(),
                           [=](enum tile t) { return t == tile; }));

        switch (tile) {
          case tile::hole: return map_[0];
          case tile::player_one... tile::player_eight:
            return map_[pid(tile) + 1];
          case tile::bonus: return map_[9];
          case tile::choice: return map_[10];
          case tile::inversion: return map_[11];
          case tile::expansion: return map_[12];
        }
        __builtin_unreachable();
      }
      /** @param tile Tile type of which to return the corresponding layer of.
       *  @returns An immutable reference to the layer belonging to the given
       *  tile type.
       */
      [[nodiscard]] [[using gnu: always_inline, hot]] constexpr const layer&
      layer_of(enum tile tile) const {
        assert(std::any_of(tile_types_.begin(), tile_types_.end(),
                           [=](enum tile t) { return t == tile; }));

        switch (tile) {
          case tile::hole: return map_[0];
          case tile::player_one... tile::player_eight:
            return map_[pid(tile) + 1];
          case tile::bonus: return map_[9];
          case tile::choice: return map_[10];
          case tile::inversion: return map_[11];
          case tile::expansion: return map_[12];
        }
        __builtin_unreachable();
      }

      /** @returns An immutable reference to the layer marking every tile which
       *  is not part of any transition domain. */
      [[nodiscard]] static constexpr const layer& n_t_domains() {
        return n_t_domains_;
      }

      /// Joins all layers &mdash; except for the layer belonging to the given
      /// player &mdash; to a singular layer.
      /** @param player Player of whom to return opponent layer.
       *  @returns Consolidation of all layers belonging to opponents.
       */
      [[nodiscard]] [[gnu::hot]] layer opponent_layer(player_t player) const {
        assert(player < player_count_);

        layer_t o = map_[12].data();
        for (player_t i = 0; i < player_count_; ++i)
          if (i != player) o |= map_[color_[i]].data();
        return { o };
      }

      /** @param player Player of whom to return the winning opponent's layer.
       *  @returns Layers belonging to the winning opponent of \p player.
       */
      [[nodiscard]] [[gnu::hot]] pos_t
      winning_opponent_stone_count(player_t player) const;

      /// Generates place moves (exclusively) for a given player.
      /** Omits generating override moves. Uses a fast parallel prefix algorithm
       *  for move generation.
       *  @param player Player who to generate moves for.
       *  @returns A layer marking all legal place moves.
       */
      layer moves_place(player_t player) const {
        assert(player < player_count_);

        const layer o = opponent_layer(player);
        const layer& p = layer_of(player);

        layer moves_place {};
        // One layer for each direction used to mark activate transition
        // domains
        std::array<layer, 8> oat_domains {};

        // One layer for each direction used to mark non-visited transitions.
        std::array<layer, 8> n_t_visited {};
        for (layer& n_oriented_t_visited : n_t_visited)
          n_oriented_t_visited.set();

        /// Unsigned 8-bit integer where the i-th bit being set represents the
        /// i-th direction being unverified.
        uint8_t unverified_directions = UINT8_MAX;
        // Fast search algorithm propagating from all player stones
        // simultaneously in a given direction. Handles unverified transitions
        // iteratively
        const auto search = [&](uint_fast8_t d) -> void {
          /// Mark propagation direction as verified
          unverified_directions &= ~(1 << d);

          // Retrieve relevant propagation parameters
          const auto [shift, bits, limit] =
            propagation_parameters_(direction(d));

          // Retrieve active transition domains.
          const layer& at_domains = oat_domains[mirror_direction_(d)];

          // Parallel *Kogge-Stone* algorithm
          layer gen = p | (o & (shift(p | at_domains, bits) | at_domains)),
                pro = o & (shift(o, bits) | at_domains);
          for (pos_t dist = bits << 1; pro && dist < limit; dist <<= 1) {
            gen |= pro & shift(gen, dist);
            pro &= shift(pro, dist);
          }
          if (!gen) return;

          layer res = gen & o, shifted_res = shift(res, bits);
          moves_place |= shifted_res;

          // Retrieve non-visited transition domains encountered during
          // propagation
          const transition_lut_t& transition_mask = t_lut_[d];
          layer& n_oriented_t_visited = n_t_visited[d];
          if (n_oriented_t_visited == n_oriented_t_domains_[d]) return;

          // Handle every newly visited transition domain
          for (pos_t i :
               (gen & n_oriented_t_visited & ~(n_oriented_t_domains_[d]))) {
            const auto [codomain_position, codomain_direction] =
              transition::decode_domain(transition_mask[i]);

            n_oriented_t_visited.reset(i);

            if (!o.test(codomain_position)) {
              if (o.test(i)) moves_place.set(codomain_position);
              continue;
            }

            oat_domains[static_cast<uint_fast8_t>(codomain_direction)].set(
              codomain_position);

            // Mark codomain direction as unverified
            unverified_directions |= 1 << static_cast<uint_fast8_t>(
                                       mirror_direction_(codomain_direction));
          }
        };

        /// Commence search in every direction
        for (uint_fast8_t i = 0; i < 8; ++i) search(i);
        /// Continue search until no new transitions are activated / discovered
        while (unverified_directions)
          search(__builtin_ctz(unverified_directions));

        return moves_place & ~p & ~o & ~layer_of(tile::hole);
      }

      /// Generates build phase moves for a given player.
      /** @param player Player to generate moves for.
       *  @returns A pair of layers where the first element marks all legal
       *  `place` moves and the second element marks all legal `override` moves.
       */
      [[nodiscard]] [[gnu::hot]]
      std::pair<layer, layer> moves_build(player_t player) const {
        assert(player < player_count_);

        const layer o = opponent_layer(player);
        const layer& p = layer_of(player);

        // Initialize result layers
        layer moves_place {}, moves_override {};

        // Search algorithm propagating from a given player stone in a given
        // direction. Uses a stack for recursion
        auto search = [&](pos_t i_0, uint_fast8_t d_0) -> void {
          // Layer holding player stone from which the propagation commences
          layer start { layer_t(1) << i_0 };
          const auto [a, b, _] = propagation_parameters_(direction(d_0));
          // Stone directly adjacent to starting stone in direction of
          // propagation
          layer adjacent = ~((a)(start, b));

          struct search_t {
            public:
              pos_t i;          // < position
              uint_fast8_t d;   // < propagation direction
              bool is_adjacent; // < whether the starting stone is located on a
                                //   transition
              bool is_propagated; // < whether i encodes a transition codomain
          };
          std::vector<search_t> stack;
          // Expect 16 nested transitions
          stack.reserve(0x10);
          stack.push_back({ i_0, d_0, false, false });

          // One layer for each direction used to mark visited transition
          // codomains
          std::array<layer, 8> t_visited {};

          while (!stack.empty()) {
            const auto [i, d, is_adjacent, is_propagated] = stack.back();
            stack.pop_back();

            // Mark transition codomain / starting position as visited
            t_visited[d].set(i);

            // Retrieve relevant propagation parameters
            const auto [shift, bits, limit] =
              propagation_parameters_(direction(d));

            // *Kogge-Stone* algorithm
            layer mask { layer_t(1) << i },
              gen = mask | (o & shift(mask, bits)), pro = o & shift(o, bits);
            for (pos_t dist = bits << 1; pro && dist < limit; dist <<= 1) {
              gen |= pro & shift(gen, dist);
              pro &= shift(pro, dist);
            }
            if (!gen) continue;

            layer res = gen & o, shifted_res = shift(res, bits);
            moves_place |= shifted_res;

            if (is_adjacent)
              moves_override |=
                ((res & ~mask) | (shifted_res & p & ~start)) & adjacent;
            else if (is_propagated)
              moves_override |= (res | (shifted_res & p & ~start)) & adjacent;
            // Case for starting position
            else moves_override |= (res | (shifted_res & p)) & adjacent;

            // Retrieve transition domain encountered during propagation
            layer domain_mask = gen & ~(n_oriented_t_domains_[d]);
            if (!domain_mask) continue;

            // Decode corresponding codomain
            pos_t domain_position = domain_mask.first();
            const auto [codomain_position, codomain_direction] =
              transition::decode_domain(t_lut_[d][domain_position]);

            if (!o.test(codomain_position)) {
              if (o.test(domain_position)) {
                moves_place.set(codomain_position);
                if (i != domain_position) {
                  layer codomain_mask { layer_t(1) << codomain_position };
                  moves_override |= p & codomain_mask & ~start;
                }
              }
              continue;
            }

            if (t_visited[uint_fast8_t(mirror_direction_(codomain_direction))]
                  .test(codomain_position))
              continue;

            /// Push codomain on the stack.
            stack.push_back(
              { codomain_position,
                uint_fast8_t(mirror_direction_(codomain_direction)),
                i_0 == domain_position, true });
          }
        };

        /// Commence search at every player stone.
        for (uint_fast8_t d = 0; d < 8; ++d)
          for (pos_t i : p) search(i, d);

        return { moves_place & ~p & ~o & ~layer_of(tile::hole),
                 moves_override |
                   (layer_of(tile::expansion) & ~layer_of(tile::hole)) };
      }

      /// Modifies the current map object based on a given move.
      /** @param move The move to be applied.
       *  @param phase The phase during which the move is performed.
       */
      constexpr void apply_move(const move& move, enum phase phase) {
        assert(move.pos() < area());

        if (phase == phase::build) apply_move_build_(move.player(), move.pos());
        else apply_move_elimination_(move.pos());
      }

      /// Applies an inversion effect.
      constexpr void apply_inversion() {
        // [[assume(_player_count > 0)]];
        if (player_count_ == 0) __builtin_unreachable();

        player_t s = color_[player_count_ - 1];
        for (player_t i = player_count_ - 2; i > 0; --i)
          color_[i + 1] = color_[i];
        color_[1] = color_[0];
        color_[0] = s;
      }

      /// Applies a choice effect by swapping stones of player two players.
      /** @param a Player A.
       *  @param b Player B.
       */
      constexpr void apply_choice(player_t a, player_t b) {
        std::swap(color_[a], color_[b]);
      }

      /// Initializes the map.
      /** @param stream Input stream containing map configuration.
       *  @param player_count The map's player count.
       *  @param bomb_radius The map's bomb radius.
       *  @param width The map's width.
       *  @param height The map's width. */
      void init(std::istream&& stream, uint_fast8_t player_count,
                dim_t bomb_radius, dim_t width, dim_t height) {
        player_count_ = player_count;
        bomb_radius_ = bomb_radius;
        width_ = width;
        height_ = height;

        // Initializes data mask
        if constexpr (std::is_same_v<layer_t, micro_layer>)
          data_mask_ =
            (area() < layer::capacity) ? ((1ull << area()) - 1) : ~0ull;
        else
          for (pos_t i = 0; i < area(); ++i) data_mask_.set(i);

        // Initializes wrap masks
        for (dim_t i = 1; i <= std::min(width_, height_); i <<= 1) {
          layer_t l {}, r {};
          for (dim_t j = 0; j < height_; ++j) {
            l |= l << width_ |
              layer_t(~((1ull << i) - 1) & ((1ull << width_) - 1));
            r |= r << width_ |
              layer_t(((1ull << (width_ - i)) - 1) & ((1ull << width_) - 1));
          }
          wrap_masks_l_[31 - __builtin_clz(i)] = l;
          wrap_masks_r_[31 - __builtin_clz(i)] = r;
        }

        // Initializes border layer
        border_ = { (1ull << width) - 1 };
        border_ <<= width * (height - 1);
        border_ |= layer_t((1ull << width) - 1);
        for (int i = 1; i < height - 1; ++i)
          border_ |= layer_t((1ull << (width - 1)) + 1) << (width * i);

        // Parses map data
        std::string s;
        char tile;
        for (dim_t i = 0; i < height_; ++i)
          for (dim_t j = 0; j < width_; ++j) {
            if (!(std::getline(stream, s, j == width_ - 1 ? '\n' : ' ') &&
                  (s.length() == 1 || (s.length() == 2 && j == width_ - 1)) &&
                  (tile_types().count(static_cast<enum tile>(tile = s[0])) ||
                   tile == '0')))
              throw std::invalid_argument("invalid formatting");

            if (tile == '0') continue;

            if (tile <= (max_player_count + '0') && (tile - 48) > player_count_)
              throw std::invalid_argument("invalid tile type");

            layer_of(static_cast<enum tile>(tile)).set(i * width_ + j);
          }

        auto parse_transition_parameter =
          [&](const std::string& name, uint_fast8_t max,
              char delim = ' ') -> uint_fast8_t {
          if (!std::getline(stream, s, delim)) [[unlikely]]
            throw std::invalid_argument("missing value for " + name);

          uint_fast8_t value = std::stoi(s);
          if (value > max) [[unlikely]]
            throw std::invalid_argument("invalid " + name);

          return value;
        };

        // Initializes transition lookup tables and layers
        for (uint_fast8_t i = 0; i < 8; ++i) n_oriented_t_domains_[i].set();
        n_t_domains_.set();

        // Parses transitions
        uint_fast8_t x1, y1, x2, y2, d1, d2;
        pos_t pos1, pos2;
        while (stream.peek() != EOF) {
          x1 = parse_transition_parameter("domain x coordinate", width_ - 1);
          y1 = parse_transition_parameter("domain y coordinate", height_ - 1);
          pos1 = static_cast<pos_t>(static_cast<pos_t>(y1) * width_ + x1);

          d1 = parse_transition_parameter("domain direction", 7);

          std::getline(stream, s, ' ');
          if (s != "<->")
            throw std::invalid_argument("expected transition symbol <->");

          x2 = parse_transition_parameter("codomain x coordinate", width_ - 1);
          y2 = parse_transition_parameter("codomain y coordinate", height_ - 1);
          pos2 = static_cast<pos_t>(static_cast<pos_t>(y2) * width_ + x2);

          d2 = parse_transition_parameter("codomain direction", 7, '\n');

          if (pos1 == pos2 && d1 == d2) continue;

          t_lut_[d1][pos1] =
            transition::encode_domain(pos2, static_cast<direction>(d2));
          n_oriented_t_domains_[d1].reset(pos1);
          n_t_domains_.reset(pos1);

          t_lut_[d2][pos2] =
            transition::encode_domain(pos1, static_cast<direction>(d1));
          n_oriented_t_domains_[d2].reset(pos2);
          n_t_domains_.reset(pos2);
        }

        auto score = [&](pos_t i) -> st_score_t {
          layer mask = { layer_t(1) << i };

          // +1 for every neighboring hole
          st_score_t score =
            (mask.neighbors() & (layer_of(tile::hole))).count();

          // +3 if adjacent to border
          if (mask & border_) score += 3;
          // +3 if corner stone
          if (mask.test(0) || mask.test(width_ - 1) ||
              mask.test((width_ - 1) * height_) || mask.test(area() - 1))
            score += 2;

          // -1 for each incoming/outgoing transition
          for (uint_fast8_t j = 0; j < 4; ++j)
            score -= !(n_oriented_t_domains_[j + 4]).test(i) ? 1 : 0;

          return std::min(score,
                          static_cast<st_score_t>(st_max_tile_score - 1));
        };

        // Initializes stone-tile table.
        for (pos_t i = 0; i < area(); ++i) st_[score(i)].set(i);

        // Asserts that stone-tile table is disjoint.
        assert((std::accumulate(
                  st_.begin(), st_.end(), layer(),
                  [](const layer& a, const layer& c) { return a ^ c; }))
                 .count() == area());
      }

      /// @cond DOXYGEN_SHOULD_SKIP_THIS
      template <typename U>
      friend std::ostream& operator<<(std::ostream&,
                                      const class map<U>::layer&);

      template <typename U>
      friend std::ostream& operator<<(std::ostream&, const map<U>&);
      /// @endcond
  };

  /// @cond DOXYGEN_SHOULD_SKIP_THIS
  template <typename T>
  [[maybe_unused]] std::ostream& operator<<(std::ostream&,
                                            const class map<T>::layer&);

  template <typename T>
  [[maybe_unused]] std::ostream& operator<<(std::ostream&, const map<T>&);
  /// @endcond

  /// Map using #micro_layer's to store tile data.
  using micro_map = map<micro_layer>;
  /// Map using #meso_layer's to store tile data.
  using meso_map = map<meso_layer>;
  /// Map using #macro_layer's to store tile data.
  using macro_map = map<macro_layer>;
}

#endif

