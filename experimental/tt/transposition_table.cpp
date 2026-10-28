#include <random>

#include "reversi.h"
#include "transposition_table.h"

void transposition_table::init() {
  std::mt19937_64 rng(0xC15C0D06F00D);
  std::uniform_int_distribution<hash_t> dist;

  for (reversi::pos_t i = 0;
       i < reversi::max_map_width * reversi::max_map_height; ++i)
    for (uint_fast8_t j = 0; j < reversi::tile_count; ++j)
      for (reversi::player_t k = 0; k < reversi::max_player_count; ++k)
        zobrist_[i][j][k] = dist(rng);
}

