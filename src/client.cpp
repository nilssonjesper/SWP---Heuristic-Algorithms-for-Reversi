#include "client.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <sstream>

#include "move.h"

bool client::wait_for_readable_() const {
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(sockfd_, &read_fds);

  return select(sockfd_ + 1, &read_fds, nullptr, nullptr, NULL) > 0;
}

bool client::send_bulk_(std::vector<uint8_t>&& data) const {
  size_t total_sent = 0;

  while (total_sent < data.size()) {
    ssize_t sent =
      send(sockfd_, data.data() + total_sent, data.size() - total_sent, 0);
    if (sent <= 0) return false;

    total_sent += sent;
  }
  return true;
}

bool client::receive_bulk_(uint8_t* buffer, size_t size) const {
  size_t total_received = 0;

  while (total_received < size) {
    if (!wait_for_readable_()) return false;

    ssize_t received =
      recv(sockfd_, buffer + total_received, size - total_received, 0);
    if (received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      return false;
    }
    if (received == 0) return false;

    total_received += received;
  }
  return true;
}

bool client::send_message_(uint8_t type, const uint8_t* data,
                           uint32_t len) const {
  std::vector<uint8_t> buffer;
  buffer.reserve(message_header_size_);

  buffer.push_back(type);

  uint32_t nlen = htonl(len);

  uint8_t* len_ptr = static_cast<uint8_t*>(static_cast<void*>(&nlen));
  buffer.insert(buffer.end(), len_ptr, len_ptr + sizeof(nlen));

  buffer.insert(buffer.end(), data, data + len);

  return send_bulk_(std::move(buffer));
}

bool client::receive_message_(std::vector<uint8_t>& message) const {
  uint8_t header[message_header_size_];
  if (!receive_bulk_(header, message_header_size_)) return false;

  uint32_t nlen;
  std::memcpy(&nlen, &header[message_type_size_], message_length_size_);
  const uint32_t len = ntohl(nlen);

  message.resize(len + message_header_size_);
  std::memcpy(message.data(), header, message_header_size_);

  return receive_bulk_(message.data() + message_header_size_, len);
}

void client::handle_move_request_() {
  uint32_t ntime_limit {};
  std::memcpy(&ntime_limit, in_.data() + client::message_header_size_, 4);
  time_t time_limit = ntohl(ntime_limit);

  typename reversi::engine::depth_t depth;
  std::memcpy(&depth, in_.data() + client::message_header_size_ + 4, 1);

  std::optional<reversi::move> m = depth > 0
    ? engine_.search_move(depth)
    : engine_.iterative_deepening(time_limit);

  if (!m.has_value()) {
    std::cerr << "out of moves\n";
    exit(EXIT_FAILURE);
  }

  const auto [x, y] = engine_.coordinate(m->pos());
  out_ = { 0, x, 0, y, m->special() };

  if (!send_message_(5, out_.data(), 5)) {
    std::cerr << "failed sending message\n";
    exit(EXIT_FAILURE);
  }
}

void client::handle_move_() {
  uint16_t nx;
  std::memcpy(&nx, in_.data() + client::message_header_size_, 2);
  const uint8_t x = ntohs(nx);

  uint16_t ny;
  std::memcpy(&ny, in_.data() + client::message_header_size_ + 2, 2);
  const uint8_t y = ntohs(ny);

  reversi::move::special_t special;
  std::memcpy(&special, in_.data() + client::message_header_size_ + 4, 1);

  uint8_t player;
  std::memcpy(&player, in_.data() + client::message_header_size_ + 5, 1);

  assert(engine_.turn() == player_(player));

  engine_.apply_move({ engine_.turn(), engine_.position(x, y), special });
  engine_.next_turn();
}

void client::handle_disqualification_() {
  uint8_t player;
  std::memcpy(&player, in_.data() + client::message_header_size_, 1);

  if (player_(player) == engine_.me()) exit(EXIT_FAILURE);

  engine_.disqualify(player_(player));
}

void client::connect() {
  addrinfo hints {};
#ifdef __APPLE__
  hints.ai_family = AF_INET; // force IPv4
#else
  hints.ai_family = AF_UNSPEC;
#endif
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(hostname_.c_str(), port_.c_str(), &hints, &addrinfo_) != 0)
    throw std::runtime_error("getaddrinfo failed");

  sockfd_ = socket(addrinfo_->ai_family, addrinfo_->ai_socktype,
                   addrinfo_->ai_protocol);
  if (sockfd_ < 0) {
    freeaddrinfo(addrinfo_);
    addrinfo_ = nullptr;
    throw std::runtime_error("socket creation failed");
  }

  if (::connect(sockfd_, addrinfo_->ai_addr, addrinfo_->ai_addrlen) != 0) {
    close(sockfd_);
    freeaddrinfo(addrinfo_);
    addrinfo_ = nullptr;
    throw std::runtime_error("connection failed");
  }

  // Set socket to nb
  fcntl(sockfd_, F_SETFL, fcntl(sockfd_, F_GETFL, 0) | O_NONBLOCK);

  // Send group id
  out_ = { group_id_ };
  if (!send_message_(1, out_.data(), 1))
    throw std::runtime_error("sending gid failed");

  // Receive map data
  if (!receive_message_(in_)) throw std::runtime_error("receiving map failed");
  std::istringstream instance_data(
    std::string(static_cast<const char*>(static_cast<const void*>(
                  in_.data() + client::message_header_size_)),
                in_.size() - client::message_header_size_));

  // Receive pid
  if (!receive_message_(in_)) throw std::runtime_error("receiving pid failed");

  engine_.init(std::move(instance_data),
               player_(in_[client::message_header_size_]));
}

void client::listen() {
  while (true) {
    if (!receive_message_(in_)) {
      std::cerr << "failed receiving message\n";
      exit(EXIT_FAILURE);
    }
    switch (in_[0]) {
      case 4: handle_move_request_(); break;
      case 6: handle_move_(); break;
      case 7: handle_disqualification_(); break;
      case 8: assert(engine_.is_elimination_phase()); break;
      case 9: handle_game_over_(); break;
    }
  }
}

