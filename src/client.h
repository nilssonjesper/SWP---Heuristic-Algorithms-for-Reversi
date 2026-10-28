#ifndef REVERSIX_CLIENT_H
#define REVERSIX_CLIENT_H

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <vector>

#include "engine.h"

/// Establishes server connection and communicates game data.
/** This class implements the client-server protocol over a TCP socket and
 *  processes the received data to manage the internal game state. It therefore
 *  does not require further synchronization apart from move notifications and
 *  requests. An object of this class should be instantiated only once and is
 *  classified as `noreturn`.
 */
class client {
  private:
    /** @name Protocol Specification
     *  @{ */
    /// Unique group id used to identify our client.
    static constexpr uint_fast8_t group_id_ = 7;

    /// Byte-size of client-server protocol message type encoding.
    static constexpr uint8_t message_type_size_ = 1;
    /// Byte-size of client-server protocol message length encoding.
    static constexpr uint8_t message_length_size_ = 4;
    /// Total byte-size of client-server protocol message header.
    static constexpr uint8_t message_header_size_ =
      message_type_size_ + message_length_size_;
    /** @} */

    /** @name Socket Information
     *  @{ */
    /// Hostname of the socket.
    const std::string hostname_;
    /// Port of the socket.
    const std::string port_;
    /// `addrinfo` structure used for establishing the socket connection.
    addrinfo* addrinfo_ = nullptr;
    /// The socket file descriptor.
    int sockfd_;
    /** @} */

    /// The client's *Reversi plus X* engine.
    reversi::engine engine_ {};

    /** @name I/O Buffers
     *  @{ */
    /// Buffer used for storing bytes received over the socket.
    std::vector<uint8_t> in_ {};
    /// Buffer used for storing bytes to send over the socket.
    std::vector<uint8_t> out_ {};
    /** @} */

    /** @name Socket I/O Functions
     *  @{ */
    /// Yields until the socket is readable.
    /** @returns True iff the select system call was successful. */
    bool wait_for_readable_() const;
    /// Sends a buffer of bytes over the socket.
    /** Sends bytes in host order (Big-endian).
     *  @param data The bytes to send (Little-endian).
     *  @returns True iff the send was successful.
     */
    bool send_bulk_(std::vector<uint8_t>&& data) const;
    /// Loads a buffer with `size` many bytes received sequentially over the
    /// socket.
    /** @param[out] data Buffer to load.
     *  @param[in] size Amount of bytes to load into the buffer.
     *  @returns True iff receiving was successful. */
    bool receive_bulk_(uint8_t* data, size_t size) const;
    // TODO: Make `type` an enum
    /// Sends a custom message to the server.
    /** @param type The type of message to send.
     *  @param data The payload to be sent (Little-endian).
     *  @param len The length of the payload in bytes.
     *  @returns True iff the send was successful.
     */
    bool send_message_(uint8_t type, const uint8_t* data, uint32_t len) const;
    /// Loads a buffer with the latest message received from the server.
    /** @param data The buffer which to fill.
     *  @returns True iff receiving was successful.
     */
    bool receive_message_(std::vector<uint8_t>& data) const;
    /** @} */

    /** @name Event Handlers
     *  @{ */
    /// Handles a move request from the server.
    void handle_move_request_();
    /// Handles a move notification from the server.
    void handle_move_();
    /// Handles a notification about a disqualified player from the server.
    void handle_disqualification_();
    /// Handles reaching the game's end state.
    [[noreturn]] void handle_game_over_() const {
      // std::cout << "game over\n";
      exit(EXIT_SUCCESS);
    }
    /** @} */

    /// Converts a network protocol player id into a #reversi::player_t value.
    /** @param player The network protocol player id which to convert.
     *  @returns A player id in range [0; 7].
     */
    constexpr reversi::player_t player_(uint_fast8_t player) const noexcept {
      assert(0 < player and player <= 8);
      return player - 1;
    }

  public:
    /// Constructs a **ReversiAI** client.
    /** @param hostname The hostname of the server to establish the connection
     *  with.
     *  @param port The port over which to establish the connection.
     */
    inline client(const char* hostname, const char* port) noexcept
        : hostname_(hostname), port_(port) {}
    /// Handles the destruction of the **ReversiAI** client object.
    /** Closes file descriptor and frees all allocated memory. */
    ~client() {
      if (addrinfo_) freeaddrinfo(addrinfo_);
      close(sockfd_);
    }

    /// Attempts to establish connection to the server using the provided
    /// hostname and port.
    /** After establishing the connection, the client sends the `group id` and
     *  awaits the `map data` and its assigned `player id`. These parameters are
     *  then used to initialize the internal instance.
     */
    void connect();

    /// Starts listening on the socket.
    /** @attention Does **not** return control flow. */
    [[noreturn]] void listen();
};

#endif

