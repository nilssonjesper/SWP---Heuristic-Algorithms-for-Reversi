#ifndef REVERSI_TRANSPOSITION_TABLE_H
#define REVERSI_TRANSPOSITION_TABLE_H

#include <cstdint>

#include "reversi.h"

class transposition_table {
  public:
    using hash_t = uint64_t;

  private:
    hash_t zobrist_[reversi::tile_count]
                   [reversi::max_map_width * reversi::max_map_height]
                   [reversi::max_player_count];

    void init();

  public:
};

#endif

