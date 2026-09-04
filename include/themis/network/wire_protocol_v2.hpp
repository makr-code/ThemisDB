/**
 * @file wire_protocol_v2.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol V2 – Multiplexed Binary Protocol
//
// V2 extends V1 with:
//   • Stream multiplexing (multiple logical streams on one TCP connection)
//   • Server push   (unsolicited data from server to client)
//   • Flow control  (per-stream and connection-level window sizes)
//
// Frame layout (V2 fixed header – 16 bytes):
//   ┌──────────┬──────────┬──────────┬──────────┐
//   │  magic   │ ver|type │  flags   │ stream_id│
//   │  4 bytes │  2 bytes │  2 bytes │  4 bytes │
//   ├──────────┴──────────┴──────────┴──────────┤
//   │         payload_length  (4 bytes)         │
//   └───────────────────────────────────────────┘
//   Followed by `payload_length` bytes of payload.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace wire {

// =============================================================================
// V2 Constants
// =============================================================================

constexpr uint32_t WIRE_V2_MAGIC        = 0x544D4432; // "TMD2"
constexpr uint8_t  WIRE_VERSION_2       = 0x02;
constexpr size_t   V2_HEADER_SIZE       = 16;
constexpr uint32_t V2_MAX_STREAM_ID     = 0x7FFFFFFF;
constexpr size_t   V2_DEFAULT_WINDOW    = 64 * 1024;   // 64 KiB initial window
constexpr size_t   V2_MAX_PAYLOAD       = 16 * 1024 * 1024; // 16 MiB

// =============================================================================
// V2 Frame Types
// =============================================================================

enum class V2FrameType : uint8_t {
    DATA          = 0x00, ///< Request / response data
    HEADERS       = 0x01, ///< Stream metadata (method, namespace, auth token)
    PRIORITY      = 0x02, ///< Stream priority hint
    RST_STREAM    = 0x03, ///< Abruptly terminate a stream
    SETTINGS      = 0x04, ///< Connection-level settings negotiation
    PUSH_PROMISE  = 0x05, ///< Server announces upcoming push stream
    PING          = 0x06, ///< Keep-alive / round-trip measurement
    GOAWAY        = 0x07, ///< Graceful connection shutdown
    WINDOW_UPDATE = 0x08, ///< Increase flow-control window
    CONTINUATION  = 0x09, ///< Continuation of a HEADERS frame
};

// =============================================================================
// V2 Frame Flags
// =============================================================================

enum class V2FrameFlags : uint16_t {
    NONE           = 0x0000,
    END_STREAM     = 0x0001, ///< Last frame for this stream
    END_HEADERS    = 0x0004, ///< This HEADERS frame ends the header block
    PADDED         = 0x0008, ///< DATA frame carries padding
    PRIORITY_FLAG  = 0x0020, ///< HEADERS frame has priority fields
    ACK            = 0x0001, ///< SETTINGS or PING acknowledgement
    COMPRESSED     = 0x0100, ///< Payload is LZ4-compressed
    ZSTD_COMPRESSED = 0x0200, ///< Payload is Zstd-compressed
};

// =============================================================================
// V2 Wire Frame Header (16 bytes, packed)
// =============================================================================

#pragma pack(push, 1)
struct V2FrameHeader {
    uint32_t magic = 0;          ///< Must equal WIRE_V2_MAGIC
    uint8_t  version;        ///< Must equal WIRE_VERSION_2
    uint8_t  frame_type;     ///< V2FrameType cast to uint8_t
    uint16_t flags;          ///< Bitfield of V2FrameFlags
    uint32_t stream_id;      ///< 0 = connection-level frame; >0 = stream frame
    uint32_t payload_length; ///< Length of payload in bytes

    bool is_valid() const noexcept {
        return magic == WIRE_V2_MAGIC && version == WIRE_VERSION_2;
    }

    V2FrameType get_type() const noexcept {
        return static_cast<V2FrameType>(frame_type);
    }

    bool has_flag(V2FrameFlags f) const noexcept {
        return (flags & static_cast<uint16_t>(f)) != 0;
    }
};
#pragma pack(pop)

static_assert(sizeof(V2FrameHeader) == V2_HEADER_SIZE,
              "V2FrameHeader must be exactly 16 bytes");

// =============================================================================
// V2 Settings Parameters
// =============================================================================

enum class V2SettingId : uint16_t {
    HEADER_TABLE_SIZE      = 0x0001,
    ENABLE_PUSH            = 0x0002,
    MAX_CONCURRENT_STREAMS = 0x0003,
    INITIAL_WINDOW_SIZE    = 0x0004,
    MAX_FRAME_SIZE         = 0x0005,
    MAX_HEADER_LIST_SIZE   = 0x0006,
};

struct V2Setting {
    V2SettingId id;
    uint32_t    value;
};

// =============================================================================
// V2 Stream State Machine
// =============================================================================

enum class V2StreamState {
    IDLE,            ///< Not yet opened
    OPEN,            ///< Request/response in progress
    HALF_CLOSED_LOCAL,  ///< Local side sent END_STREAM
    HALF_CLOSED_REMOTE, ///< Remote side sent END_STREAM
    CLOSED,          ///< Stream fully closed
    RESERVED_LOCAL,  ///< Promised by local PUSH_PROMISE
    RESERVED_REMOTE, ///< Promised by remote PUSH_PROMISE
};

struct V2Stream {
    uint32_t      stream_id            = 0;
    V2StreamState state                = V2StreamState::IDLE;
    int32_t       send_window          = static_cast<int32_t>(V2_DEFAULT_WINDOW);
    int32_t       recv_window          = static_cast<int32_t>(V2_DEFAULT_WINDOW);
    uint8_t       priority             = 16; ///< 0 (highest) – 255 (lowest)
    uint32_t      stream_dependency    = 0;  ///< ID of the parent stream (0 = root)
    bool          exclusive_dependency = false; ///< Exclusive dependency flag (RFC 7540 §5.3)

    bool is_open() const noexcept {
        return state == V2StreamState::OPEN ||
               state == V2StreamState::HALF_CLOSED_REMOTE;
    }
};

// =============================================================================
// V2 Connection Configuration
// =============================================================================

struct V2ConnectionConfig {
    uint32_t max_concurrent_streams  = 100;
    uint32_t initial_window_size     = static_cast<uint32_t>(V2_DEFAULT_WINDOW);
    uint32_t max_frame_size          = static_cast<uint32_t>(V2_MAX_PAYLOAD);
    bool     enable_server_push      = true;
    bool     enable_flow_control     = true;
    bool     enable_lz4_compression  = true;
    bool     enable_zstd_compression = false;  ///< Zstd overrides LZ4 when both enabled
    int      zstd_compression_level  = 3;      ///< Zstd compression level (1–22)
    uint32_t min_compression_payload_size = 256; ///< Skip compression for smaller payloads
    uint16_t port                    = 7890; ///< Default V2 protocol port
    size_t   num_io_threads          = 4;
};

// =============================================================================
// Forward declarations
// =============================================================================

class V2Session;
class V2Server;

// =============================================================================
// Callback / handler types
// =============================================================================

/// Called when a complete DATA frame arrives on a stream.
/// @param stream_id  ID of the stream carrying the data.
/// @param payload    Raw payload bytes (may be decompressed already).
/// @param end_stream True if this is the last frame on the stream.
using V2DataHandler = std::function<void(
    uint32_t stream_id,
    const std::vector<uint8_t>& payload,
    bool end_stream)>;

/// Called when a HEADERS frame arrives (new stream or trailer).
using V2HeadersHandler = std::function<void(
    uint32_t stream_id,
    const std::unordered_map<std::string, std::string>& headers,
    bool end_stream)>;

/// Called when a stream is reset (RST_STREAM received).
using V2RstStreamHandler = std::function<void(
    uint32_t stream_id, uint32_t error_code)>;

// =============================================================================
// V2Session – per-connection multiplexed session
// =============================================================================

/**
 * @brief Represents one multiplexed V2 connection.
 *
 * A single V2Session manages many logical streams (V2Stream) over a
 * single TCP socket.  It is created by V2Server when a new client
 * connects and destroyed when the connection is torn down.
 */
class V2Session : public std::enable_shared_from_this<V2Session> {
public:
    virtual ~V2Session() = default;

    /// @brief Unique connection identifier (UUID or sequential number).
    virtual const std::string& connection_id() const = 0;

    /// @brief Send a DATA frame on @p stream_id.
    virtual void send_data(uint32_t stream_id,
                           const std::vector<uint8_t>& data,
                           bool end_stream = true) = 0;

    /// @brief Initiate a server-push stream (server → client unsolicited).
    virtual uint32_t push_promise(
        uint32_t associated_stream_id,
        const std::unordered_map<std::string, std::string>& headers) = 0;

    /// @brief Gracefully close a stream with RST_STREAM.
    virtual void reset_stream(uint32_t stream_id, uint32_t error_code = 0) = 0;

    /// @brief Send a GOAWAY and close the connection after in-flight frames.
    virtual void go_away(uint32_t last_stream_id, uint32_t error_code = 0) = 0;

    /// @brief Number of currently open streams on this connection.
    virtual size_t open_stream_count() const = 0;

    /// @brief Current send-window size for @p stream_id (bytes).
    virtual int32_t send_window(uint32_t stream_id) const = 0;

    /// @brief Update the connection-level send-window (WINDOW_UPDATE).
    virtual void update_connection_window(uint32_t increment) = 0;

    /// @brief Send a PRIORITY frame to inform the remote side of stream ordering.
    ///
    /// The PRIORITY frame payload follows RFC 7540 §6.3:
    ///   - E (1 bit)             – exclusive dependency flag
    ///   - Stream Dependency (31 bits) – ID of the parent stream (0 = root)
    ///   - Weight (8 bits)       – priority weight 0–255 (maps to HTTP/2 weight 1–256)
    ///
    /// @param stream_id   Stream to reprioritise.
    /// @param dependency  Parent stream ID this stream depends on (0 = root).
    /// @param weight      Priority weight 0–255.
    /// @param exclusive   When true the stream exclusively depends on @p dependency.
    virtual void set_stream_priority(uint32_t stream_id,
                                     uint32_t dependency,
                                     uint8_t  weight,
                                     bool     exclusive = false) = 0;

    // ── Statistics ────────────────────────────────────────────────────────
    virtual uint64_t frames_received()  const = 0;
    virtual uint64_t frames_sent()      const = 0;
    virtual uint64_t bytes_received()   const = 0;
    virtual uint64_t bytes_sent()       const = 0;
};

// =============================================================================
// V2Server – multiplexed wire protocol server
// =============================================================================

/**
 * @brief Listens for incoming connections and manages V2Session instances.
 *
 * Usage:
 * @code
 *   V2ConnectionConfig cfg;
 *   cfg.port = 7890;
 *   cfg.max_concurrent_streams = 200;
 *
 *   V2Server server(cfg);
 *   server.set_data_handler([](uint32_t sid, const auto& payload, bool eos) {
 *       // handle incoming data
 *   });
 *   server.start();
 *   // ... run event loop ...
 *   server.stop();
 * @endcode
 */
class V2Server {
public:
    explicit V2Server(const V2ConnectionConfig& config);
    ~V2Server();

    // Non-copyable, non-movable (owns I/O threads)
    V2Server(const V2Server&)            = delete;
    V2Server& operator=(const V2Server&) = delete;

    // ── Lifecycle ────────────────────────────────────────────────────────

    /// @brief Start accepting connections.  Non-blocking; spawns I/O threads.
    void start();

    /// @brief Stop accepting connections and wait for in-flight work to finish.
    void stop();

    bool is_running() const;

    // ── Handler registration ──────────────────────────────────────────────

    void set_data_handler(V2DataHandler handler);
    void set_headers_handler(V2HeadersHandler handler);
    void set_rst_stream_handler(V2RstStreamHandler handler);

    // ── Server-push API ───────────────────────────────────────────────────

    /**
     * @brief Push data to a connected client on a new server-initiated stream.
     *
     * @param connection_id   Identifies the target V2Session.
     * @param associated_sid  Client stream that triggered the push.
     * @param headers         Headers for the pushed resource.
     * @param data            Payload to push.
     * @return true if the push was enqueued successfully.
     */
    bool push_to_client(const std::string& connection_id,
                        uint32_t associated_sid,
                        const std::unordered_map<std::string, std::string>& headers,
                        const std::vector<uint8_t>& data);

    // ── Statistics ────────────────────────────────────────────────────────

    size_t   active_connections()   const;
    uint64_t total_streams_opened() const;
    uint64_t total_frames_sent()    const;
    uint64_t total_frames_received() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace wire
} // namespace themis
