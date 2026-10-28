#include <chrono>
#include <cstdint>

#include "instance.h"

namespace reversi {
  /// Manages game tree and performs move search
  /** The `engine` class provides a Reversi plus X engine to a #client object.
   */
  class engine {
    public:
      /// Alias for unsigned integer encoding the depth of a game tree.
      using depth_t = uint_fast8_t;

      /// Type used for storing elapsed milliseconds.
      using time_t = int_fast64_t;

    private:
      /// The engine's internal game state.
      instance instance_;
      /// The player for which the engine is to maximize.
      player_t me_;

      /// The maximum search depth.
      static constexpr uint_fast8_t max_depth_ = 30;

      /** @name Aspiration
       *  @{ */
      /// Delta of the aspiration window
      static constexpr instance::eval_t aspiration_delta_ = 100;
      /// Aspiration window guess
      instance::eval_t aspiration_window_;
      /** @} */

      /** @name Iterative deepening
       *  @{ */
      /// Clock used to measure elapsed search time.
      using clock = std::chrono::steady_clock;
      /// Time unit used by #clock.
      using clock_time_t = std::chrono::milliseconds;

      /// Tolerance applied to iterative deepening deadline.
      static constexpr time_t abort_time_buffer_ = 100;

      /// Abort signal for iterative deepening.
      bool abort_ = false;
      /// Deadline for iterative deepening.
      std::chrono::time_point<clock> abort_time_;
      /** @} */

      /** @name History heuristic
       *  @{ */
      /// Maps a move to a value determined by the history heuristic.
      /** Special table assigning a value to each move based on the depth at
       * which it produced a cutoff. */
      std::unordered_map<move, uint_fast16_t> history_;

      /// Evaluates a given move for move ordering.
      /** @param move Move to evaluate.
       *	@param st_score Stone-tile score of \p move.
       *  @returns Linear combination of \p st_score and historic value of move.
       */
      template <typename T>
      constexpr instance::eval_t
      move_evaluate_(class move move,
                     typename reversi::map<T>::st_score_t st_score) noexcept {
        return st_score + history_[move];
      }
      /** @} */

      /// Minimax implementation using optional Alpha-beta Pruning and / or
      /// Move Sorting.
      /** @param root Root node of subtree.
       *  @param depth Maximum search depth.
       *  @param alpha Highest guaranteed heuristic value for maximizing
       * player.
       *  @param beta Lowest guaranteed heuristic value for minimizing player.
       *  @returns Heuristic value of optimal leaf node.
       */
      [[gnu::hot]] instance::eval_t
      search_move_(instance root, depth_t depth,
                   instance::eval_t alpha = instance::min_eval,
                   instance::eval_t beta = instance::max_eval) {
        if (depth == 0) return root.evaluate(me_);

        return std::visit(
          [&](auto&& map) -> instance::eval_t {
            using T = std::decay_t<decltype(map)>;

            const auto [moves_place, moves_override] =
              root.next_turn<typename std::decay_t<decltype(map)>::layer_t>();

            const bool max = me_ == root.turn();

            const auto move_positions = is_elimination_phase()
              ? moves_place
              : (max ? (moves_place ? moves_place : moves_override)
                     : (moves_place | moves_override));

            if (!move_positions) return root.evaluate(me_);

            instance::eval_t top_score =
              max ? instance::min_eval : instance::max_eval;

            // Function for evaluating a game tree node and updating alpha and
            // beta parameters based on result
            const auto search_node =
              [&](instance&& node, class move move)
                __attribute__((hot, always_inline)) -> bool {
              node.apply_move(move);

              if (clock::now() >= abort_time_) return abort_ = true;

              instance::eval_t score =
                search_move_(node, depth - 1, alpha, beta);

              if (abort_) return true;

              if (max) {
                top_score = std::max(top_score, score);
                if (top_score >= beta) {
                  // Store depth of cutoff in history table
                  history_[move] += depth * depth;
                  return true;
                }
                alpha = std::max(top_score, alpha);
              } else {
                top_score = std::min(top_score, score);
                if (top_score <= alpha) return true;
                beta = std::min(top_score, beta);
              }

              return false;
            };

            if (instance_.is_build_phase()) {
              std::vector<std::pair<move, instance::eval_t>> moves;
              moves.reserve(move_positions.count());

              const auto& st = T::st();

              // Move evaluation
              for (int i = T::st_max_tile_score - 1; i >= 0; --i)
                for (pos_t pos : (move_positions & st[i])) {
                  if (map.layer_of(tile::choice).test(pos))
                    for (player_t j = 1; j <= root.player_count(); ++j) {
                      class move move { root.turn(), pos, j };
                      moves.push_back(
                        { move, move_evaluate_<typename T::layer_t>(move, i) });
                    }
                  else if (map.layer_of(tile::bonus).test(pos)) {
                    class move move { root.turn(), pos,
                                      static_cast<move::special_t>(
                                        move::bonus::override_stone) };
                    moves.push_back(
                      { move, move_evaluate_<typename T::layer_t>(move, i) });
                  } else {
                    class move move { root.turn(), pos };
                    moves.push_back(
                      { move, move_evaluate_<typename T::layer_t>(move, i) });
                  };
                }

              // Move ordering
              std::sort(moves.begin(), moves.end(), [&](auto&& a, auto&& b) {
                return a.second > b.second;
              });

              for (auto& [move, move_score] : moves)
                if (search_node(instance(root), move)) {
                  if (abort_) return instance::min_eval;
                  break;
                }
            } else
              for (pos_t pos : move_positions)
                if (search_node(instance(root), move(instance_.turn(), pos))) {
                  if (abort_) return instance::min_eval;
                  break;
                }

            return top_score;
          },
          root.map());
      }

    public:
      /// Constructs an engine
      engine() = default;

      /** @returns An immutable reference to the id of the player currently
       *  making a move. */
      [[gnu::always_inline]] constexpr const player_t& turn() const noexcept {
        return instance_.turn();
      }
      /** @returns An immutable reference to the id of the player for which the
       *  engine is maximizing. */
      [[gnu::always_inline]] constexpr const player_t& me() const noexcept {
        return me_;
      }

      /** @returns True iff the engine's game instance is currently in the build
       *  phase. */
      [[gnu::always_inline]] constexpr bool is_build_phase() const noexcept {
        return instance_.is_build_phase();
      }
      /** @returns True iff the engine's game instance is currently in the
       *  elimination phase. */
      [[gnu::always_inline]] constexpr bool
      is_elimination_phase() const noexcept {
        return instance_.is_elimination_phase();
      }

      /// Progresses the game state by electing the next player with applicable
      /// moves.
      constexpr void next_turn() noexcept {
        std::visit(
          [&](auto&& map) constexpr noexcept {
            instance_
              .next_turn<typename std::decay_t<decltype(map)>::layer_t>();
          },
          instance_.map());
      }

      /// Disqualifies a given player from the engine's game instance.
      /** @param player Player to disqualify. */
      [[gnu::always_inline]] constexpr void
      disqualify(player_t player) noexcept {
        instance_.disqualify(player);
      }

      /// Casts an arbitrary position on the engine's map to it's corresponding
      /// coordinate value.
      /** @param position Position to cast.
       *  @returns A pair containing `x` and `y` coordinate values based on the
       *  map's dimensions.
       */
      constexpr std::pair<dim_t, dim_t> coordinate(pos_t position) const {
        return std::visit(
          [&](auto&& map) constexpr noexcept -> std::pair<dim_t, dim_t> {
            return map.coordinate(position);
          },
          instance_.map());
      }

      /// Casts a given coordinate to a corresponding #pos_t value.
      /** @param x x-coordinate
       *  @param y y-coordinate
       *  @returns The equivalent position as a #pos_t value.
       */
      constexpr pos_t position(dim_t x, dim_t y) const {
        return std::visit(
          [&](auto&& map) constexpr noexcept -> pos_t {
            return x + y * map.width();
          },
          instance_.map());
      }

      /// Initiates the elimination phase of the engine's game instance.
      [[gnu::always_inline]] constexpr void next_phase() {
        instance_.next_phase();
      }

      /// Modifies the engine's instance based on a given move.
      /** @param move The move to be applied. */
      [[gnu::always_inline]] void apply_move(class move move) {
        instance_.apply_move(move);
      }

      /// Performs an iterative deepening search
      /** @param time_limit The time restriction of the search.
       *  @returns The best move as determined by the engine.
       */
      std::optional<move> iterative_deepening(time_t time_limit) {
        abort_ = false;

        std::chrono::time_point<clock> depth_start_time = clock::now();
        abort_time_ =
          depth_start_time + clock_time_t(time_limit - abort_time_buffer_);

        // Search depth 1
        std::optional<move> top_move =
          search_move(1, aspiration_window_ - aspiration_delta_,
                      aspiration_window_ + aspiration_delta_);

        time_t time_per_depth = std::chrono::duration_cast<clock_time_t>(
                                  clock::now() - depth_start_time)
                                  .count();
        time_limit -= time_per_depth;

        // Check for timeout at depth 2
        if (abort_ || time_per_depth * 50 > time_limit) {
          std::cout << "search halted at depth 2\n";
          return top_move;
        }

        // Search depth 2
        depth_start_time = clock::now();
        std::optional<move> temp_move =
          search_move(2, aspiration_window_ - aspiration_delta_,
                      aspiration_window_ + aspiration_delta_);

        // Compute time ratio
        double depth_expansion_time_ratio =
          static_cast<double>(std::chrono::duration_cast<clock_time_t>(
                                clock::now() - depth_start_time)
                                .count()) /
          (time_per_depth + 1);

        // Perform iterative deepening
        for (depth_t i = 3; i < max_depth_; ++i) {
          time_limit -= time_per_depth =
            std::chrono::duration_cast<clock_time_t>(clock::now() -
                                                     depth_start_time)
              .count();

          if (abort_ ||
              std::chrono::duration_cast<clock_time_t>(clock::now() -
                                                       depth_start_time)
                    .count() *
                  depth_expansion_time_ratio >
                time_limit) {
            std::cout << "search halted at depth " << std::to_string(i)
                      << (abort_ ? " due to faulty estimation\n" : "\n");
            return top_move;
          }
          top_move = temp_move;

          depth_start_time = clock::now();
          temp_move = search_move(i, aspiration_window_ - aspiration_delta_,
                                  aspiration_window_ + aspiration_delta_);

          // Update time ratio
          depth_expansion_time_ratio =
            static_cast<double>(std::chrono::duration_cast<clock_time_t>(
                                  clock::now() - depth_start_time)
                                  .count()) /
            (time_per_depth + 1);
        }

        std::cout << "search halted at depth 30\n";

        return top_move;
      }
      /**
       *  @param alpha Highest guaranteed heuristic value for maximizing player.
       *  @param beta Lowest guaranteed heuristic value for minimizing player.
       *  @param depth Maximum search depth.
       *  @returns The best move as determined by the engine.
       */
      constexpr std::optional<move>
      search_move(depth_t depth, instance::eval_t alpha = instance::min_eval,
                  instance::eval_t beta = instance::max_eval) {
        assert(instance_.turn() == me_);

        instance::eval_t top_score = instance::min_eval;
        move top_move;

        // Function for evaluating top-level game tree node and updating top
        // move based on result
        const auto search_node = [&](instance&& i, class move move)
                                   __attribute__((hot, always_inline)) -> bool {
          i.apply_move(move);

          if (clock::now() >= abort_time_) return abort_ = true;

          instance::eval_t score = search_move_(i, depth - 1, alpha, beta);

          if (abort_) return true;

          if (score > top_score) {
            top_score = score;
            top_move = move;
          }

          return false;
        };

        return std::visit(
          [&](auto&& map) -> std::optional<move> {
            using T = std::decay_t<decltype(map)>;

            const auto [moves_place, moves_override] =
              instance_.moves<typename T::layer_t>();

            const auto move_positions =
              moves_place ? moves_place : moves_override;

            if (!move_positions) return std::nullopt;

            if (instance_.is_build_phase()) {
              std::vector<std::pair<move, instance::eval_t>> moves;
              moves.reserve(move_positions.count());

              const auto& st = T::st();

              // Move evaluation
              for (int i = T::st_max_tile_score - 1; i >= 0; --i)
                for (pos_t pos : (move_positions & st[i])) {
                  if (map.layer_of(tile::choice).test(pos))
                    for (player_t j = 1; j <= instance_.player_count(); ++j) {
                      class move move { instance_.turn(), pos, j };
                      moves.push_back(
                        { move, move_evaluate_<typename T::layer_t>(move, i) });
                    }
                  else if (map.layer_of(tile::bonus).test(pos)) {
                    class move move { instance_.turn(), pos,
                                      static_cast<move::special_t>(
                                        move::bonus::override_stone) };
                    moves.push_back(
                      { move, move_evaluate_<typename T::layer_t>(move, i) });
                  } else {
                    class move move { instance_.turn(), pos };
                    moves.push_back(
                      { move, move_evaluate_<typename T::layer_t>(move, i) });
                  };
                }

              // Move ordering
              std::sort(moves.begin(), moves.end(), [&](auto&& a, auto&& b) {
                return a.second > b.second;
              });

              for (auto& [move, move_score] : moves)
                if (search_node(instance(instance_), move)) return std::nullopt;
            } else
              for (pos_t pos : move_positions)
                if (search_node(instance(instance_),
                                move(instance_.turn(), pos)))
                  return std::nullopt;

            if (top_score <= alpha)
              return search_move(depth, instance::min_eval, beta);
            else if (top_score >= beta)
              return search_move(depth, alpha, instance::max_eval);

            aspiration_window_ = top_score;

            return std::make_optional(top_move);
          },
          instance_.map());
      }

      /// Initializes the engine based on an input stream.
      /** @param stream Input stream containing map configuration.
       *  @param player The player for which the engine is to maximize.
       */
      void init(std::istream&& stream, player_t player) {
        me_ = player;
        // Initialize internal instance
        instance_ = reversi::instance::create(std::move(stream));

        aspiration_window_ = instance_.evaluate(me_);

        assert(player < instance_.player_count());
      }
  };
}
