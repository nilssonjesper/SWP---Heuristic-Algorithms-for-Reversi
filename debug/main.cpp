/*
#include <cstdlib>
#include <iostream>
#include <regex>

#include <fstream>

#include "engine.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Missing filename\n";
    return EXIT_FAILURE;
  }

  std::ifstream in(argv[1]);
  if (!in.is_open()) {
    std::cerr << "Error opening file\n";
    return EXIT_FAILURE;
  }

  reversi::engine engine {};
  engine.init(std::move(in), argc > 2 ? std::stoi(argv[2]) : 0);

  std::string line;
  std::regex move_pattern { R"(^\s*(\d+)\s+(\d+)\s+(\d+)(?:\s+(\d+))?\s*$)" },
    disqualify_pattern { R"(^\s*disqualify\s+(\d+)\s*$)" };
  std::smatch move_match, disqualify_match;

  std::ifstream file("../experimental/moves.txt");
  if (!file.is_open()) {
    std::cerr << "Error opening moves\n";
    return EXIT_FAILURE;
  }

  std::cin.rdbuf(file.rdbuf());

  int move = 11;

  std::visit(
    [&](auto&& map) {
      while (true) {
        std::getline(std::cin, line);
        const bool is_disqualification = std::regex_match(
                     line, disqualify_match, disqualify_pattern),
                   is_move = std::regex_match(line, move_match, move_pattern);

        if (!(is_disqualification || is_move)) {
          std::cerr << "Invalid input, try again.\n";
          abort();
        }

        if (is_disqualification) {
          reversi::player_t player = std::stoi(disqualify_match[1]);

          if (player > engine.i().player_count()) {
            std::cerr << "Invalid input, try again.\n";
            abort();
          }

          engine.disqualify(player - 1);
          std::cout << "Disqualified " << std::to_string(player - 1) << '\n';
        } else if (is_move) {
          reversi::player_t player = std::stoi(move_match[1]);
          reversi::pos_t x = std::stoi(move_match[2]),
                         y = std::stoi(move_match[3]);
          reversi::move::special_t s =
            move_match[4].matched ? std::stoi(move_match[4]) : 0;

          std::cout << move << ' ' << std::to_string(player) << ' '
                    << std::to_string(x) << ' ' << std::to_string(y) << ' '
                    << std::to_string(s) << '\n';

          if (player > engine.i().player_count() || x >= map.width() ||
              y >= map.width() ||
              (s > engine.i().player_count() && !(s == 20 || s == 21))) {
            std::cerr << "Invalid input, try again.\n";
            abort();
          }

          if (player - 1 != engine.turn()) {
            std::cerr << "DESYNC: turn\n";
            std::cout << std::to_string(engine.i().bomb_count(4)) << ' '
                      << std::to_string(engine.turn()) << '\n';
            abort();
          }

          const auto [moves_place, moves_override] =
            engine.i().moves<typename std::decay_t<decltype(map)>::layer_t>();
          const auto moves = engine.i().has_override_stones(engine.turn())
            ? (moves_place | moves_override)
            : moves_place;
          if (!moves.test(x + y * map.width())) {
            std::cerr << "DESYNC: move validity\n";
            std::cout << moves << '\n';
            abort();
          }

          engine.apply_move({ engine.turn(), x + y * map.width(), s });
          engine.next_turn();
        }

        if (move == 1221) {
          std::cout << std::to_string(engine.turn()) << ' '
                    << engine.i()
                         .moves<typename std::decay_t<decltype(map)>::layer_t>()
                         .second.test(6 + 38 * 50)
                    << '\n';
          return;
        }
        ++move;
        continue;
      }
    },
    engine.i().map());

  return EXIT_SUCCESS;
}*/

/*#include <cstdlib>
#include <iostream>

#include <fstream>

#include "instance.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Missing filename\n";
    return EXIT_FAILURE;
  }

  std::ifstream in(argv[1]);
  if (!in.is_open()) {
    std::cerr << "Error opening file\n";
    return EXIT_FAILURE;
  }

  reversi::instance i = reversi::instance::create(std::move(in));
  i.turn() = argc > 2 ? std::stoi(argv[2]) : 0;

  std::visit(
    [&](auto&& map) {
      std::cout << map << '\n';

      // SET TILE!!!
      const auto a = map.layer_of(reversi::tile::player_eight);
      i.apply_move({ i.turn(), 39 + 28 * 50, 0 });
      const auto b = map.layer_of(reversi::tile::player_eight);

      std::cout << (a ^ b) << '\n';

      const auto moves =
        i.moves<typename std::decay_t<decltype(map)>::layer_t>().second;

      std::cout << moves << '\n' << moves.test(1439) << '\n';
    },
    i.map());

  return EXIT_SUCCESS;
}
*/

