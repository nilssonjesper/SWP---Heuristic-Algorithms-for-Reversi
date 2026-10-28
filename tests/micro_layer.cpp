#include <gtest/gtest.h>

#include <array>

#include "map.h"

using namespace reversi;

class MicroLayerTest : public ::testing::Test {
  protected:
    micro_map map {};

    static constexpr std::array<enum tile, 13> tile_types {
      tile::hole,         tile::player_one,   tile::player_two,
      tile::player_three, tile::player_four,  tile::player_five,
      tile::player_six,   tile::player_seven, tile::player_eight,
      tile::bonus,        tile::choice,       tile::inversion,
      tile::expansion
    };

    void init(uint_fast8_t player_count, dim_t bomb_radius, dim_t width,
              dim_t height, const char* s) {
      assert(static_cast<pos_t>(width) * height < 64);

      map = micro_map();
      try {
        map.init(std::istringstream(s), player_count, bomb_radius, width,
                 height);
      } catch (const std::exception& e) {
        FAIL() << "Failed initializing map: " << e.what();
      }
    }

    void test(micro_map::layer res, micro_layer expected) {
      EXPECT_EQ(res.data(), expected)
        << "Got:\n"
        << res << "\nMismatch:\n"
        << micro_map::layer(res.data() ^ expected);
    }
};

TEST_F(MicroLayerTest, InitMap) {
  init(8, 0, 4, 4, "- 1 2 3\n4 5 6 7\n8 b c i\nx - 1 2");

  constexpr micro_layer pattern = 0x2001ull;

  for (int i = 0; i < 13; ++i)
    EXPECT_EQ(map.layer_of(tile_types[i]).data(), (pattern << i) & 0xFFFF)
      << "Invalid layer " << static_cast<char>(tile_types[i]);

  micro_map::layer u = map.layer_of(tile_types[0]);
  for (int i = 1; i < 13; ++i) u = u ^ map.layer_of(tile_types[i]);

  EXPECT_EQ(u.data(), 0xFFFF);
}

TEST_F(MicroLayerTest, OverrideStandard) {
  init(8, 0, 4, 4, "0 0 0 0\n0 1 2 0\n0 2 1 0\n0 0 0 0");
  test(map.moves_build(0).second, 0ull);
}

TEST_F(MicroLayerTest, OverrideStar) {
  init(8, 0, 8, 7,
       "2 0 0 2 0 0 2 0\n0 2 0 2 0 2 0 0\n0 0 2 2 2 0 0 0\n2 2 2 1 2 2 2 2\n0 "
       "0 2 2 2 0 0 0\n0 2 0 2 0 2 0 0\n2 0 0 2 0 0 2 0");
  test(map.moves_build(0).second, 0x492A00E3002A49ull);
}
/*
TEST_F(MicroLayerTest, OverrideTransitionPropagatedEntry) {
  init(8, 0, 8, 8,
       "0 2 0 2 0 0 2 0\n0 0 0 2 0 2 0 0\n0 0 2 2 2 0 0 0\n2 2 0 1 2 2 2 2\n0 "
       "0 2 0 0 0 0 0\n0 2 0 2 0 0 2 0\n2 0 0 2 0 0 2 0\n0 0 0 2 0 0 2 0\n3 0 "
       "0 <-> 3 7 4\n7 3 2 <-> 0 3 6\n6 0 1 <-> 6 7 4\n0 6 5 <-> 1 0 0");
  test(map.moves_build(0).second, 0x48494A00E300284Aull);
}

TEST_F(MicroLayerTest, OverrideTransitionAdjacentEntry) {
  init(8, 0, 8, 8,
       "1 0 0 2 0 2 2 2\n0 0 2 0 0 0 2 2\n0 2 0 0 0 2 0 0\n2 2 2 2 0 0 0 1\n0 "
       "2 0 0 0 0 0 0\n0 0 2 0 0 2 0 2\n0 0 0 2 0 0 2 2\n1 0 0 0 2 0 0 2\n7 3 "
       "1 <-> 0 3 5\n7 3 2 <-> 0 3 6\n7 3 3 <-> 0 3 7\n0 7 6 <-> 7 0 0\n0 7 5 "
       "<-> 7 0 1\n0 7 4 <-> 7 0 2\n0 0 0 <-> 7 7 3\n0 0 1 <-> 7 7 4");
  test(map.moves_build(0).second, 0x10C8A4020E22C468ull);
}

TEST_F(MicroLayerTest, OverridePlayer) {
  init(8, 0, 8, 8,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n1 0 0 0 0 0 1 2\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 3 "
       "6 <-> 7 3 2");
  test(map.moves_build(0).second, 0x00000041000000ull);
}

TEST_F(MicroLayerTest, OverridePlayerTransitionAdjacentEntry) {
  init(8, 0, 8, 8,
       "0 0 0 1 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n1 0 0 0 0 0 1 2\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 1 1 1 0 0 0\n0 0 0 2 0 0 0 0\n0 3 "
       "6 <-> 7 3 2\n3 0 0 <-> 3 7 4\n3 0 1 <-> 3 7 5 \n3 0 7 <-> 3 7 3");
  test(map.moves_build(0).second, 0x1C000041000008ull);
}

TEST_F(MicroLayerTest, OverridePlayerTransitionAdjacentEntrySelf) {
  init(8, 0, 8, 8,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n2 2 2 2 2 2 2 1\n0 7 "
       "6 <-> 7 7 2");
  test(map.moves_build(0).second, 0x7F00000000000000ull);
}

TEST_F(MicroLayerTest, OverridePlayerTransitionAdjacentSelf) {
  init(8, 0, 8, 8,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n1 0 0 0 0 0 0 1\n0 7 "
       "6 <-> 7 7 2");
  test(map.moves_build(0).second, 0ull);
}

TEST_F(MicroLayerTest, OverrideSelfMapTransition) {
  init(8, 0, 8, 8,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 1 - 0 0 0\n0 0 0 0 2 0 0 0\n4 7 "
       "3 <-> 4 7 3");
  test(map.moves_build(0).second, 0ull);
}

TEST_F(MicroLayerTest, OverrideNestedTransitionOppositeDirection) {
  init(8, 0, 8, 8,
       "2 2 2 2 2 2 2 0\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n1 "
       "2 2 2 2 2 2 2\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n7 4 "
       "2 <-> 0 3 6\n7 3 2 <-> 0 2 6\n7 2 2 <-> 0 1 6\n7 1 2 <-> 0 0 6");
  test(map.moves_build(0).second, 0xFCFFFFFF7Full);
}

TEST_F(MicroLayerTest, OverrideNestedTransitionsEqualDirection) {
  init(8, 0, 8, 8,
       "2 2 2 2 2 2 2 0\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n1 "
       "2 2 2 2 2 2 2\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n7 4 "
       "2 <-> 7 3 2\n0 3 6 <-> 0 2 6\n7 2 2 <-> 7 1 2\n0 1 6 <-> 0 0 6");
  test(map.moves_build(0).second, 0xFCFFFFFF7Full);
}
*/
TEST_F(MicroLayerTest, CountLeadingZeroFull) {
  init(8, 0, 4, 4, "1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 0);
}

TEST_F(MicroLayerTest, CountLeadingZeroMax) {
  init(8, 0, 4, 4, "0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 1");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 15);
}

TEST_F(MicroLayerTest, CountLeadingZeroOne) {
  init(8, 0, 4, 4, "1 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 0);
}

TEST_F(MicroLayerTest, CountLeadingZeroRand) {
  init(8, 0, 4, 4, "0 0 0 0\n0 0 0 0\n0 0 1 0\n0 0 0 0");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 10);
}

TEST_F(MicroLayerTest, IteratorBegin) {
  init(8, 0, 4, 4, "1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 15);

  init(8, 0, 4, 4, "0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 1");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 15);

  init(8, 0, 4, 4, "1 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 0);

  init(8, 0, 4, 4, "0 0 0 0\n0 0 0 0\n0 0 1 0\n0 0 0 0");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 10);
}

TEST_F(MicroLayerTest, IteratorEnd) {
  EXPECT_EQ(map.layer_of(tile::player_one).end(), -1);
}
/*
TEST_F(MicroLayerTest, Iterator) {
  init(8, 0, 8, 8,
       "1 1 1 1 1 1 1 0\n1 1 1 1 1 1 1 0\n1 1 1 1 1 1 1 0\n1 1 1 1 1 1 1 0\n1 "
       "1 1 1 1 1 1 0\n1 1 1 1 1 1 1 0\n1 1 1 1 1 1 1 0\n1 1 1 1 1 1 1 0");

  pos_t i = 62;
  for (pos_t it : map.layer_of(tile::player_one)) {
    EXPECT_EQ(it, i);
    EXPECT_TRUE(map.layer_of(tile::player_one).test(it));

    i -= (i % 8 == 0 ? 2 : 1);
  }

  i = 63;
  for (pos_t it : ~map.layer_of(tile::player_one)) {
    EXPECT_EQ(it, i);
    EXPECT_FALSE(map.layer_of(tile::player_one).test(it));

    i -= 8;
  }

  init(8, 0, 8, 8,
       "0 1 1 1 1 1 1 1\n0 1 1 1 1 1 1 1\n0 1 1 1 1 1 1 1\n0 1 1 1 1 1 1 1\n0 "
       "1 1 1 1 1 1 1\n0 1 1 1 1 1 1 1\n0 1 1 1 1 1 1 1\n0 1 1 1 1 1 1 1");

  i = 63;
  for (pos_t it : map.layer_of(tile::player_one)) {
    EXPECT_EQ(it, i);
    EXPECT_TRUE(map.layer_of(tile::player_one).test(it));

    i -= ((i - 1) % 8 == 0 ? 2 : 1);
  }

  i = 56;
  for (pos_t it : ~map.layer_of(tile::player_one)) {
    EXPECT_EQ(it, i);
    EXPECT_FALSE(map.layer_of(tile::player_one).test(it));

    i -= 8;
  }
}
*/
TEST_F(MicroLayerTest, EmptyIterator) {
  init(8, 0, 4, 4, "0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), -1);
}

