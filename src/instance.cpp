#include <istream>
#include <utility>
#include <variant>

#include "instance.h"
#include "map.h"

using namespace reversi;

instance::instance(uint_fast8_t player_count,
                   uint_fast8_t override_stones_count, uint_fast8_t bomb_count,
                   map_t&& map) noexcept
    : player_count_(player_count), map_(std::move(map)) {
  for (player_t i = 0; i < player_count; ++i)
    players_[i] = { .override_stones_count = override_stones_count,
                    .bomb_count = bomb_count,
                    .disqualified = false };

  phase_ = phase::build;

  turn_ = 0;

  // Elect starting player
  std::visit(
    [&](const auto& m) constexpr noexcept -> void {
      using T = typename std::decay_t<decltype(m)>::layer_t;

      for (player_t i = 0; i < player_count_; ++i) {
        const auto [moves_place, moves_override] = moves<T>();
        if (moves_place || moves_override) return;
        turn_ = (turn_ + 1) % player_count_;
      }
      next_phase();
      for (player_t i = 0; i < player_count_; ++i) {
        const auto [moves_place, moves_override] = moves<T>();
        if (moves_place || moves_override) return;
        turn_ = (turn_ + 1) % player_count_;
      }
    },
    map_);
}

void instance::apply_move(class move move) {
  std::visit(
    [&](auto& map) {
      // Subtract items if necessary
      if (is_elimination_phase()) {
        assert(players_[turn_].bomb_count > 0);
        players_[turn_].bomb_count -= 1;
      } else if ((map.layer_of(turn_) | map.opponent_layer(turn_))
                   .test(move.pos())) {
        assert(players_[turn_].override_stones_count > 0);
        players_[turn_].override_stones_count -= 1;
      }

      // Assert correctness of special field
      assert(((move.special() & 0x10) &&
              map.layer_of(tile::bonus).test(move.pos())) ||
             ((move.special() & 0x10) == 0 && move.special() != 0 &&
              map.layer_of(tile::choice).test(move.pos())) ||
             move.special() == 0);

      map.apply_move(move, phase_);

      // Handle special field
      if (move.special() & 0x10) {
        if (move.special() == 21 && players_[turn_].override_stones_count < 255)
          players_[turn_].override_stones_count += 1;
        else if (move.special() == 20 && players_[turn_].bomb_count < 255)
          players_[turn_].bomb_count += 1;
      } else if (move.special()) map.apply_choice(turn_, move.special() - 1);
    },
    map_);
}

instance::eval_t instance::evaluate_adjacency_border_(player_t player) const {
  return std::visit(
    [&](auto&& map) constexpr noexcept -> instance::eval_t {
      using T = std::decay_t<decltype(map)>;

      instance::eval_t score = 0;

      // Use stone-tile table
      const auto& st = T::st();
      for (typename T::st_score_t i = 1; i < T::st_max_tile_score; ++i)
        score += (map.layer_of(player) & st[i]).count() << i;

      return score;
    },
    map_);
}

instance::eval_t
instance::evaluate_adjacency_opponents_(player_t player) const {
  return std::visit(
    [&](auto&& map) constexpr noexcept -> instance::eval_t {
      return (map.layer_of(player).neighbors() & map.opponent_layer(player))
        .count();
    },
    map_);
}

instance::eval_t instance::evaluate_stone_count_(player_t player) const {
  return std::visit(
    [&](const auto& map) constexpr noexcept -> instance::eval_t {
      instance::eval_t player_stone_count = map.layer_of(player).count(),
                       opponent_stone_count = (map.opponent_layer(player) &
                                               ~map.layer_of(tile::expansion))
                                                .count();

      return (player_stone_count * player_stone_count) -
        (opponent_stone_count * opponent_stone_count);
    },
    map_);
}

instance::eval_t
instance::evaluate_override_stone_count_(player_t player) const {
  instance::eval_t opponent_stone_count = 0;
  for (player_t o = 0; o < player_count_; ++o)
    if (o != player) opponent_stone_count += players_[o].override_stones_count;

  return std::visit(
    [&](const auto& map) -> instance::eval_t {
      return (player_count_ * players_[player].override_stones_count -
              opponent_stone_count) *
        (map.area() - map.layer_of(player).count() -
         (map.opponent_layer(player) & ~map.layer_of(tile::expansion)).count());
    },
    map_);
}

instance instance::create(std::istream&& in) {
  std::string s;
  auto parse_parameter = [&](const std::string& name, uint_fast8_t min,
                             uint_fast8_t max,
                             char delim = ' ') -> uint_fast8_t {
    if (!std::getline(in, s, delim)) [[unlikely]]
      throw std::invalid_argument("missing value for " + name);

    uint_fast8_t value = std::stoi(s);
    if (value < min || value > max) [[unlikely]]
      throw std::invalid_argument(name + " must be within range " +
                                  std::to_string(min) + ".." +
                                  std::to_string(max));

    return value;
  };

  uint_fast8_t player_count = parse_parameter("player count", 2, 8, '\n'),
               override_stones_count =
                 parse_parameter("override stones count", 0, 255, '\n'),
               bomb_count = parse_parameter("bomb count", 0, 255),
               bomb_radius = parse_parameter("bomb radius", 0, 255, '\n'),
               map_height = parse_parameter("map height", 0, max_map_height),
               map_width = parse_parameter("map width", 0, max_map_width, '\n');

  // Create map variant based on runtime parameters
  map_t map_variant = [&]() -> map_t {
    if (map_width * map_height <= 64) return micro_map();
    else if (map_width * map_height <= 1024) return meso_map();
    else return macro_map();
  }();
  // Initialize map
  std::visit(
    [&](auto&& map) {
      map.init(std::move(in), player_count, bomb_radius, map_width, map_height);
    },
    map_variant);

  return { player_count, override_stones_count, bomb_count,
           std::move(map_variant) };
}

