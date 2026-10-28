#include "map.h"

using namespace reversi;

template <typename T>
pos_t map<T>::winning_opponent_stone_count(player_t player) const {
  assert(player < player_count_);

  // TODO: optimize this loop
  pos_t c = 0;
  for (player_t i = 0; i < player_count_; ++i)
    if (i != player && map_[color_[i]].count() > c) c = map_[color_[i]].count();
  return c;
}

template <typename T>
[[maybe_unused]] std::ostream& reversi::operator<<(std::ostream& o,
                                                   const map<T>& m) {
  std::string s;
  s.resize(2 * (m.height_ + 1) * m.width_ - 1);

  for (dim_t i = 0; i < m.height_; ++i) {
    for (dim_t j = 0; j < m.width_; ++j) {
      s[2 * (i * m.width_ + j)] = '0';
      s[2 * (i * m.width_ + j) + 1] = ' ';
      for (enum tile l : m.tile_types_) {
        if (!(m.layer_of(l < tile::player_one || l > tile::player_eight
                           ? l
                           : tile(m.color_[pid(l)] - 1))
                .test(i * m.width_ + j)))
          continue;

        s[2 * (i * m.width_ + j)] = static_cast<char>(l);
        break;
      }
    }
    s[2 * (i + 1) * m.width_ - 1] = '\n';
  }
  s[2 * (m.height_ + 1) * m.width_ - 1] = '\n';

  return o << s;
}

template std::ostream&
reversi::operator<<(std::ostream&, const reversi::map<reversi::micro_layer>&);
template std::ostream&
reversi::operator<<(std::ostream&, const reversi::map<reversi::meso_layer>&);
template std::ostream&
reversi::operator<<(std::ostream&, const reversi::map<reversi::macro_layer>&);

template class reversi::map<micro_layer>;
template class reversi::map<meso_layer>;
template class reversi::map<macro_layer>;

