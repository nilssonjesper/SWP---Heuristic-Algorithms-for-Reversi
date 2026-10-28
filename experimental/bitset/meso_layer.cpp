#include <gtest/gtest.h>

#include <array>

#include "map.h"

using namespace reversi;

class MesoLayerTest : public ::testing::Test {
  protected:
    meso_map map {};

    static constexpr std::array<enum tile, 13> tile_types {
      tile::hole,         tile::player_one,   tile::player_two,
      tile::player_three, tile::player_four,  tile::player_five,
      tile::player_six,   tile::player_seven, tile::player_eight,
      tile::bonus,        tile::choice,       tile::inversion,
      tile::expansion
    };

    void init(uint_fast8_t player_count, dim_t bomb_radius, dim_t width,
              dim_t height, const char* s) {
      assert(static_cast<pos_t>(width) * height > 64);

      map = meso_map();
      try {
        map.init(std::istringstream(s), player_count, bomb_radius, width,
                 height);
      } catch (const std::exception& e) {
        FAIL() << "Failed initializing map: " << e.what();
      }
    }

    void test(meso_map::layer res, meso_layer expected) {
      EXPECT_EQ(res.data(), expected) << "Got:\n"
                                      << res << "\nMismatch:\n"
                                      << meso_map::layer(res.data() ^ expected);
    }
};

TEST_F(MesoLayerTest, InitMap) {
  init(8, 0, 8, 9,
       "- 1 2 3 4 5 6 7\n8 b c i x - 1 2\n3 4 5 6 7 8 b c\ni x - 1 2 3 4 5\n6 "
       "7 8 b c i x -\n1 2 3 4 5 6 7 8\nb c i x - 1 2 3\n4 5 6 7 8 b c i\nx - "
       "1 2 3 4 5 6");

  meso_layer pattern { 0x10008004002001ull };
  pattern[1] = 0x2;

  meso_layer mask {};
  for (int i = 0; i < 72; ++i) mask.set(i);

  for (int i = 0; i < 13; ++i)
    EXPECT_EQ(map.layer_of(tile_types[i]).data(), (pattern << i) & mask)
      << "Invalid layer " << static_cast<char>(tile_types[i]);

  meso_layer u = map.layer_of(tile_types[0]).data();
  for (int i = 1; i < 13; ++i) u = u ^ map.layer_of(tile_types[i]).data();

  EXPECT_EQ(u, mask);
}

TEST_F(MesoLayerTest, OverrideStandard) {
  init(8, 0, 8, 9,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 1 2 0 0 0\n0 "
       "0 0 2 1 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0");

  const auto [moves_place, moves_override] = map.moves_build(0);
  test(moves_place, { 0x80420100000ull });
  test(moves_override, {});
}

TEST_F(MesoLayerTest, OverrideStar) {
  init(8, 0, 8, 9,
       "2 0 0 2 0 0 2 0\n0 2 0 2 0 2 0 0\n0 0 2 2 2 0 0 0\n2 2 2 1 2 2 2 2\n0 "
       "0 2 2 2 0 0 0\n0 2 0 2 0 2 0 0\n2 0 0 2 0 0 2 0\n0 0 0 2 0 0 0 2\n0 0 "
       "0 0 0 0 0 0");

  test(map.moves_build(0).second, 0x88492A00E3002A49ull);
}

TEST_F(MesoLayerTest, OverrideTransitionPropagatedEntry) {
  init(8, 0, 8, 9,
       "0 2 0 2 0 0 2 0\n0 0 0 2 0 2 0 0\n0 0 2 2 2 0 0 0\n2 2 0 1 2 2 2 2\n0 "
       "0 2 0 0 0 0 0\n0 2 0 2 0 0 2 0\n2 0 0 2 0 0 2 0\n0 0 0 2 0 0 2 0\n0 0 "
       "0 2 0 0 2 0\n3 0 0 <-> 3 8 4\n7 3 2 <-> 0 3 6\n6 0 1 <-> 6 8 4\n0 6 5 "
       "<-> 1 0 0");

  meso_layer expected { 0x48494A00E300284Aull };
  expected[1] = 0x48;
  test(map.moves_build(0).second, expected);
}

TEST_F(MesoLayerTest, OverrideTransitionAdjacentEntry) {
  init(
    8, 0, 8, 9,
    "1 0 0 2 0 2 2 2\n0 0 2 0 0 0 2 2\n0 2 0 0 0 2 0 0\n2 2 2 2 0 0 0 1\n0 2 0 "
    "0 0 0 0 0\n0 0 2 0 0 0 0 2\n0 0 0 2 0 2 0 2\n0 0 0 0 2 0 2 2\n1 0 0 0 0 2 "
    "0 2\n7 3 1 <-> 0 3 5\n7 3 2 <-> 0 3 6\n7 3 3 <-> 0 3 7\n0 8 6 <-> 7 0 "
    "0\n0 8 5 <-> 7 0 1\n0 8 4 <-> 7 0 2\n0 0 0 <-> 7 8 3\n0 0 1 <-> 7 8 4");

  meso_layer expected { 0xd0a884020e22c468ull };
  expected[1] = 0x20;
  test(map.moves_build(0).second, expected);
}

TEST_F(MesoLayerTest, OverridePlayer) {
  init(8, 0, 8, 9,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n1 0 0 0 0 0 1 2\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0\n0 3 6 <-> 7 3 2");

  test(map.moves_build(0).second, 0x41000000ull);
}

TEST_F(MesoLayerTest, OverridePlayerTransitionAdjacentEntry) {
  init(8, 0, 8, 9,
       "0 0 0 1 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n1 0 0 0 0 0 1 2\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 1 1 1 0 0 0\n0 0 "
       "0 2 0 0 0 0\n0 3 6 <-> 7 3 2\n3 0 0 <-> 3 8 4\n3 0 1 <-> 3 8 5\n3 0 7 "
       "<-> 3 8 3\n");

  test(map.moves_build(0).second, { 0x1C00000041000008ull });
}

TEST_F(MesoLayerTest, OverridePlayerTransitionAdjacentEntrySelf) {
  init(8, 0, 8, 9,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n2 2 2 2 2 2 2 1\n0 0 "
       "0 0 0 0 0 0 \n0 7 6 <-> 7 7 2");
  test(map.moves_build(0).second, { 0x7F00000000000000ull });
}

TEST_F(MesoLayerTest, OverridePlayerTransitionAdjacentSelf) {
  init(8, 0, 8, 9,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n1 0 0 0 0 0 0 1\n0 0 "
       "0 0 0 0 0 0\n0 7 6 <-> 7 7 2");
  test(map.moves_build(0).second, {});
}

TEST_F(MesoLayerTest, OverrideSelfMapTransition) {
  init(8, 0, 8, 9,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 1 - 0 0 0\n0 0 0 0 2 0 0 0\n0 0 "
       "0 0 0 0 0 0\n4 7 3 <-> 4 7 3");
  test(map.moves_build(0).second, {});
}

TEST_F(MesoLayerTest, OverrideNestedTransitionOppositeDirection) {
  init(8, 0, 8, 9,
       "2 2 2 2 2 2 2 0\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n1 "
       "2 2 2 2 2 2 2\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0\n7 4 2 <-> 0 3 6\n7 3 2 <-> 0 2 6\n7 2 2 <-> 0 1 6\n7 1 2 "
       "<-> 0 0 6");
  test(map.moves_build(0).second, { 0xFCFFFFFF7Full });
}

TEST_F(MesoLayerTest, OverrideNestedTransitionsEqualDirection) {
  init(8, 0, 8, 9,
       "2 2 2 2 2 2 2 0\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n2 2 2 2 2 2 2 2\n1 "
       "2 2 2 2 2 2 2\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0\n7 4 2 <-> 7 3 2\n0 3 6 <-> 0 2 6\n7 2 2 <-> 7 1 2\n0 1 6 "
       "<-> 0 0 6");
  test(map.moves_build(0).second, { 0xFCFFFFFF7Full });
}

TEST_F(MesoLayerTest, OverridePlayerSelfEqualDirection) {
  init(8, 0, 8, 9,
       "0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 "
       "0 0 0 0 0 0 0\n2 1 2 2 2 2 2 2\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0\n0 5 6 <-> 7 5 2");
  test(map.moves_build(0).second, { 0xFD0000000000ull });
}

TEST_F(MesoLayerTest, OverridePlayerSelf) {
  init(8, 0, 8, 9,
       "0 2 0 0 0 0 0 0\n0 2 0 0 0 0 0 0\n0 2 0 0 0 0 0 0\n0 2 0 0 0 0 0 0\n0 "
       "2 0 0 0 0 0 0\n0 1 2 2 2 2 2 2\n0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0\n1 0 0 <-> 7 5 2");
  test(map.moves_build(0).second, { 0xFC0202020202ull });
}

TEST_F(MesoLayerTest, OverridePlayerSelf2) {
  init(8, 0, 8, 9,
       "0 2 0 0 0 0 0 0\n0 2 0 0 0 0 0 0\n0 2 0 0 0 0 0 0\n0 2 0 0 0 0 0 0\n0 "
       "2 0 0 0 0 0 0\n0 1 2 2 2 2 2 2\n0 2 0 0 0 0 0 0\n0 1 0 0 0 0 0 0\n0 0 "
       "0 0 0 0 0 0\n1 0 0 <-> 7 5 2");
  test(map.moves_build(0).second, { 0x200FE0202020202ull });
}

TEST_F(MesoLayerTest, CountLeadingZeroFull) {
  init(8, 0, 9, 8,
       "1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 "
       "1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 "
       "1 1 1 1 1");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 0);
}

TEST_F(MesoLayerTest, CountLeadingZeroMax) {
  init(8, 0, 9, 8,
       "0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 1");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 71);
}

TEST_F(MesoLayerTest, CountLeadingZeroOne) {
  init(8, 0, 9, 8,
       "1 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 0");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 0);
}

TEST_F(MesoLayerTest, CountLeadingZeroRand) {
  init(8, 0, 9, 8,
       "0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 1 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 0");
  EXPECT_EQ(map.layer_of(tile::player_one).first(), 21);
}

TEST_F(MesoLayerTest, IteratorBegin) {
  init(8, 0, 9, 8,
       "1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 "
       "1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 1 1 1 1 1\n1 1 1 1 "
       "1 1 1 1 1");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 71);

  init(8, 0, 9, 8,
       "0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 1");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 71);

  init(8, 0, 9, 8,
       "1 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 0");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 0);

  init(8, 0, 9, 8,
       "0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 1 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 0");
  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), 21);
}

TEST_F(MesoLayerTest, IteratorEnd) {
  EXPECT_EQ(map.layer_of(tile::player_one).end(), -1);
}

TEST_F(MesoLayerTest, EmptyIterator) {
  init(8, 0, 9, 8,
       "0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 "
       "0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0 0 0 0 "
       "0 0 0 0 0");

  EXPECT_EQ(*map.layer_of(tile::player_one).begin(), -1);
}

