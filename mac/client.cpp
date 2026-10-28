#include "client.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <variant>

#include "move.h"

#define GROUP_ID 7

bool client::_wait_for_readable() const {
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(_sockfd, &read_fds);

  return select(_sockfd + 1, &read_fds, nullptr, nullptr, NULL) > 0;
}

bool client::_send_bulk(std::vector<uint8_t>&& data) const {
  size_t total_sent = 0;

  while (total_sent < data.size()) {
    ssize_t sent =
      send(_sockfd, data.data() + total_sent, data.size() - total_sent, 0);
    if (sent <= 0) return false;

    total_sent += sent;
  }
  return true;
}

bool client::_receive_bulk(uint8_t* buffer, size_t size) const {
  size_t total_received = 0;

  while (total_received < size) {
    if (!_wait_for_readable()) return false;

    ssize_t received =
      recv(_sockfd, buffer + total_received, size - total_received, 0);
    if (received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      return false;
    }
    if (received == 0) return false;

    total_received += received;
  }
  return true;
}

bool client::_send_message(uint8_t type, const uint8_t* data,
                           uint32_t len) const {
  std::vector<uint8_t> buffer;
  buffer.reserve(_message_header_size);

  buffer.push_back(type);

  uint32_t nlen = htonl(len);

  uint8_t* len_ptr = static_cast<uint8_t*>(static_cast<void*>(&nlen));
  buffer.insert(buffer.end(), len_ptr, len_ptr + sizeof(nlen));

  buffer.insert(buffer.end(), data, data + len);

  return _send_bulk(std::move(buffer));
}

bool client::_receive_message(std::vector<uint8_t>& message) const {
  uint8_t header[_message_header_size];
  if (!_receive_bulk(header, _message_header_size)) return false;

  uint32_t nlen;
  std::memcpy(&nlen, &header[_message_type_size], _message_length_size);
  const uint32_t len = ntohl(nlen);

  message.resize(len + _message_header_size);
  std::memcpy(message.data(), header, _message_header_size);

  return _receive_bulk(message.data() + _message_header_size, len);
}

void client::_handle_move_request() {
  std::visit(
    [&](auto& engine) constexpr -> void {
      uint32_t ntime_limit {};
      std::memcpy(&ntime_limit, _in.data() + client::_message_header_size, 4);
      time_t time_limit = ntohl(ntime_limit);

      typename std::decay_t<decltype(engine)>::depth_t depth;
      std::memcpy(&depth, _in.data() + client::_message_header_size + 4, 1);

      std::optional<reversi::move> m = depth > 0
        ? engine.search_move(depth)
        : engine.iterative_deepening(time_limit);

      if (!m.has_value()) {
        // this should never happen as the server expects a move
        std::cerr << "out of moves\n";
        exit(EXIT_FAILURE);
      }

      const auto&& [x, y] = engine.coordinate(m.value().pos());
      _out = { 0, y, 0, x };

      std::visit(
        [&](auto&& special) constexpr noexcept {
          using T = std::decay_t<decltype(special)>;

          if constexpr (std::is_same_v<T, std::monostate>) _out.push_back(0);
          else if constexpr (std::is_same_v<T, reversi::move::bonus>)
            _out.push_back(static_cast<uint8_t>(special) | 0x14);
          else _out.push_back(special + 1);
        },
        m.value().special());

      if (!_send_message(5, _out.data(), 5)) {
        std::cerr << "failed sending message\n";
        exit(EXIT_FAILURE);
      }
    },
    _engine);
}

void client::_handle_move() {
  uint16_t nx;
  std::memcpy(&nx, _in.data() + client::_message_header_size, 2);
  const uint8_t x = ntohs(nx);

  uint16_t ny;
  std::memcpy(&ny, _in.data() + client::_message_header_size + 2, 2);
  const uint8_t y = ntohs(ny);

  uint8_t nspecial;
  std::memcpy(&nspecial, _in.data() + client::_message_header_size + 4, 1);
  reversi::move::special_t special;
  if (nspecial) {
    if (nspecial & 0x10)
      special.emplace<reversi::move::bonus>(
        static_cast<reversi::move::bonus>(nspecial & 0x1));
    else
      special.emplace<reversi::player_t>(
        static_cast<reversi::player_t>(nspecial - 1));
  }

  uint8_t player;
  std::memcpy(&player, _in.data() + client::_message_header_size + 5, 1);

  std::visit(
    [&](auto& engine) constexpr -> void {
      assert(engine.turn() == _player(player));

      engine.apply_move(x, y, special);
      engine.next_turn();
    },
    _engine);
}

void client::_handle_disqualification() {
  uint8_t player;
  std::memcpy(&player, _in.data() + client::_message_header_size, 1);

  std::visit(
    [&](auto& engine) constexpr -> void {
      // TODO: Delete this
      if (_player(player) == engine.me()) {
        std::cerr << "disqualified\n";
        exit(EXIT_FAILURE);
      }

      engine.disqualify(_player(player));
    },
    _engine);
}

void client::connect() {
  addrinfo hints {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(_hostname.c_str(), _port.c_str(), &hints, &_addrinfo) != 0)
    throw std::runtime_error("getaddrinfo failed");

  _sockfd = socket(_addrinfo->ai_family, _addrinfo->ai_socktype,
                   _addrinfo->ai_protocol);
  if (_sockfd < 0) {
    freeaddrinfo(_addrinfo);
    _addrinfo = nullptr;
    throw std::runtime_error("socket creation failed");
  }

  if (::connect(_sockfd, _addrinfo->ai_addr, _addrinfo->ai_addrlen) != 0) {
    close(_sockfd);
    freeaddrinfo(_addrinfo);
    _addrinfo = nullptr;
    throw std::runtime_error("connection failed");
  }

  // set socket to nb
  fcntl(_sockfd, F_SETFL, fcntl(_sockfd, F_GETFL, 0) | O_NONBLOCK);

  // send gid
  _out = { GROUP_ID };
  if (!_send_message(1, _out.data(), 1))
    throw std::runtime_error("sending gid failed");

  // receive map data
  if (!_receive_message(_in)) throw std::runtime_error("receiving map failed");
  std::istringstream instance_data(
    std::string(static_cast<const char*>(static_cast<const void*>(
                  _in.data() + client::_message_header_size)),
                _in.size() - client::_message_header_size));

  // receive pid
  if (!_receive_message(_in)) throw std::runtime_error("receiving pid failed");

  std::visit(
    [&](auto& engine) constexpr -> void {
      engine.init(std::move(instance_data),
                  _player(_in[client::_message_header_size]));
    },
    _engine);
}

void client::listen() {
  while (true) {
    if (!_receive_message(_in)) {
      std::cerr << "failed receiving message\n";
      exit(EXIT_FAILURE);
    }
    switch (_in[0]) {
    case 4:
      _handle_move_request();
      break;
    case 6:
      _handle_move();
      break;
    case 7:
      _handle_disqualification();
      break;
    case 8:
      assert(std::visit([&](auto& engine) constexpr
                          -> bool { return engine.is_elimination_phase(); },
                        _engine));
      break;
    case 9:
      _handle_game_over();
      break;
    }
  }
}
