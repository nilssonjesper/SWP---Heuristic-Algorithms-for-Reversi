#ifndef REVERSIX_INSTANCE_H
#define REVERSIX_INSTANCE_H

#include <array>
#include <cstdint>
#include <optional>
#include <variant>

#include "map.h"
#include "move.h"

namespace reversi {
  /// Models a game state.
  /** This class simulates the state of a *Reversi plus X* match at any point
   *  in time, acting as a node of a game tree. An #instance object can be
   *  evaluated using predefined heuristics and is extended using move and
   *  disqualification announcements. The turn progression, resulting captures
   *  and player states are inferred automatically by the instance.
   */
  class instance {
    public:
      /// Union of (both) possible map types.
      using map_t = std::variant<micro_map, meso_map, macro_map>;

      /** @name Evaluation
       *  @{ */
      /// Return-type of evaluation functions.
      using eval_t = int_fast64_t;
      /// The minimum possible score which may be attributed to an instance.
      static constexpr eval_t min_eval = INT_FAST64_MIN;
      /// The maximum possible score which may be attributed to an instance.
      static constexpr eval_t max_eval = INT_FAST64_MAX;
      /** @} */

    private:
      /** @name Player-related data
       *  @{ */
      /// The number of players for the instance's map.
      uint_fast8_t player_count_;
      /// A structure holding player state information.
      /// TODO: Create 8 bit int to hold disqualified status of players
      struct player_state {
        public:
          /// The amount of override stones held by a player.
          uint_fast8_t override_stones_count;
          /// The amount of bombs held by a player.
          uint_fast8_t bomb_count;
          /// Whether the player is disqualified or not.
          bool disqualified;
      };
      /// An array holding the state of each player.
      std::array<player_state, max_player_count> players_ {};
      /** @} */

      /// The amount of moves elapsed since the beginning of the game.
      /** Represents the depth at which this instance is located within a game
       *  tree. */
      uint_fast16_t move_number_;

      /// The player who is currently making a move.
      player_t turn_;
      /// The current phase of the instance.
      phase phase_;
      /// The current map state of the instance.
      map_t map_;

      /// Constructs an instance from the given parameters.
      /** @param player_count The number of players.
       *  @param override_stones_count The number of override stones held
       *  by each player at the start of the game.
       *  @param bomb_count The number of bombs held by each player at the
       *  start of the game.
       *  @param map The map which the game takes place on.
       */
      inline instance(uint_fast8_t player_count,
                      uint_fast8_t override_stones_count,
                      uint_fast8_t bomb_count, map_t&& map) noexcept;

      /// Evaluates the adjacency of a player's stones to border and hole
      /// tiles which are not transition domains.
      /** @param player The player whose adjacency to evaluate.
       *  @returns Number of player stones neighboring border or hole
       *  tiles, that are not transition domains.
       */
      [[nodiscard]] eval_t evaluate_adjacency_border_(player_t player) const;
      /// Evaluates the adjacency of a player's stones to opponent stones.
      /** @param player The player whose adjacency to evaluate.
       *  @returns Number of player stones neighboring opponent stones
       */
      [[nodiscard]] eval_t evaluate_adjacency_opponents_(player_t player) const;
      /// Evaluates a player's total stone count relative to other
      /// opponents' stones.
      /** @param player The player whose stone count to evaluate.
       *  @returns The player's stone count minus the total amount of
       *  opponent stones.
       */
      [[nodiscard]] eval_t evaluate_stone_count_(player_t player) const;
      /// Evaluates a player's override stone count relative to other
      /// opponents' override stones.
      /** @param player The player whose override stone count to evaluate.
       *  @returns The player's override stone count minus the total
       *  amount of opponent override stones.
       */
      [[nodiscard]] eval_t
      evaluate_override_stone_count_(player_t player) const;

    public:
      /// Constructs an empty instance.
      inline instance() = default;

      /** @returns The player count of the instance. */
      constexpr uint_fast8_t player_count() const noexcept {
        return player_count_;
      }

      /** @returns The width of the instance's map. */
      constexpr dim_t map_width() const noexcept {
        return std::visit(
          [](auto&& map) constexpr noexcept { return map.width(); }, map_);
      }

      /** @returns An immutable reference to the instance's unified map.
       */
      constexpr const map_t& map() const noexcept { return map_; }

      /** @returns An immutable reference to the id of the player
       *  currently making a move. */
      constexpr const player_t& turn() const noexcept { return turn_; }
      // TODO: Delete this function
      /** @returns A mutable reference to the id of the player currently
       *  making a move. */
      constexpr player_t& turn() noexcept { return turn_; }

      /** @returns True iff the game is currently in the build phase. */
      [[gnu::always_inline]] constexpr bool is_build_phase() const noexcept {
        return phase_ == phase::build;
      }
      /** @returns True iff the game is currently in the elimination
       *  phase. */
      [[gnu::always_inline]] constexpr bool
      is_elimination_phase() const noexcept {
        return phase_ == phase::elimination;
      }

      /// Initiates the elimination phase.
      constexpr void next_phase() noexcept {
        assert(phase_ != phase::elimination);

        phase_ = phase::elimination;
      }

      /// Determines whether a given player has remaining override stones.
      /** @param player Player of whom to check the override stone count.
       *  @returns True iff the player has remaining override stones. */
      [[gnu::always_inline]] constexpr bool
      has_override_stones(player_t player) const {
        return players_[player].override_stones_count > 0;
      }

      /// Determines whether a given player has remaining bombs.
      /** @param player Player of whom to check the bomb count.
       *  @returns True iff the player has remaining bombs. */
      [[gnu::always_inline]] constexpr bool has_bombs(player_t player) const {
        return players_[player].bomb_count > 0;
      }

      /** @returns Layer marking all applicable moves of the attacking
       *  player encoded in #turn_. */
      template <typename T>
      constexpr std::pair<typename reversi::map<T>::layer,
                          typename reversi::map<T>::layer>
      moves() const {
        using U = typename reversi::map<T>;

        const U& map = std::get<U>(map_);

        // Delegate appropriate move generation to map object
        if (is_build_phase())
          return has_override_stones(turn_)
            ? map.moves_build(turn_)
            : std::pair(map.moves_place(turn_), typename U::layer());
        else if (has_bombs(turn_))
          return { has_bombs(turn_) ? ~map.layer_of(tile::hole)
                                    : typename U::layer(),
                   typename U::layer() };
        else return { typename U::layer(), typename U::layer() };
      }

      /// Elects first player with applicable moves and returns them.
      /** @note Exhibits undefined behaviour once the game has ended.
       *  @returns The valid moves of the newly elected player.
       */
      template <typename T>
      constexpr std::pair<typename reversi::map<T>::layer,
                          typename reversi::map<T>::layer>
      next_turn() noexcept {
        player_t caller = turn_;

        std::pair<typename reversi::map<T>::layer,
                  typename reversi::map<T>::layer>
          move_positions;
        for (player_t i = 0; i < player_count_; ++i) {
          turn_ = (turn_ + 1) % player_count_;
          move_positions = moves<T>();

          if (players_[turn_].disqualified) continue;
          if (move_positions.first || move_positions.second)
            return move_positions;
        }

        // Out-of-moves
        if (is_elimination_phase()) return move_positions;

        next_phase();
        turn_ = caller;
        return next_turn<T>();
      }

      /// @brief Modifies the current instance object based on a given
      /// move containing a choice for a special field
      /** @param move Move to be applied. */
      [[gnu::hot]] void apply_move(class move move);

      /// Disqualifies a given player from the game.
      /** @param player Player to disqualify. */
      constexpr void disqualify(player_t player) noexcept {
        assert(!players_[player].disqualified);

        players_[player].disqualified = true;

        // Elect new player if disqualified player is currently taking turn.
        if (player == turn_)
          std::visit(
            [&](auto&& map) constexpr noexcept {
              next_turn<typename std::decay_t<decltype(map)>::layer_t>();
            },
            map_);
      }

      /** @returns If possible, a random move applicable by the currently
       *  attacking player encoded in #turn_. */
      [[maybe_unused]] std::optional<move> random_move() const;

      /// Evaluates a player's position.
      /** Linear combination of partial evaluation functions based on 5
       *  heuristics:
       *    - current stone count on the map (map domination)
       *    - amount of occupants on neighboring tiles (cluster formation)
       *    - amount of stones bordering the map (border adjacency)
       *    - amount of applicable moves (mobility)
       *    - amount of override stones available
       *
       *  @param player Player whose position to evaluate.
       *  @returns Positive floating point value in relation to the
       *  predicted win-rate.
       */
      [[nodiscard]] [[gnu::always_inline]] eval_t
      evaluate(player_t player) const {
        const eval_t eval_stone_count = evaluate_stone_count_(player);

        return evaluate_override_stone_count_(player) * eval_stone_count +
          4 * eval_stone_count + evaluate_adjacency_opponents_(player) +
          evaluate_adjacency_border_(player);
      }

      /// Initializes an instance based on an input stream.
      /** @param stream Input stream containing map configuration.
       *  @returns Initialized instance.
       */
      static instance create(std::istream&& stream);

      /// @cond DOXYGEN_SHOULD_SKIP_THIS
      [[maybe_unused]] constexpr friend std::ostream&
      operator<<(std::ostream& o, const instance& i) {
        return std::visit(
          [&](auto&& map) constexpr noexcept -> std::ostream& {
            return o << map;
          },
          i.map_);
      }
      /// @endcond
  };

  /// @cond DOXYGEN_SHOULD_SKIP_THIS
  constexpr std::ostream& operator<<(std::ostream&, const instance&);
  /// @endcond
}

#endif

