/**
 * @file http3_datagram.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – HTTP/3 Datagram Support (RFC 9221 + RFC 9297)
 *
 * Provides Http3DatagramDispatcher: context registration, receive dispatch,
 * and outbound frame encoding for HTTP/3 datagrams over QUIC.
 *
 * Design notes:
 *   - Each HTTP/3 datagram carries a Quarter Stream ID (stream_id / 4) as a
 *     QUIC variable-length integer, followed by the application payload
 *     (RFC 9297 §2).
 *   - The dispatcher maps Quarter Stream IDs (context IDs) to registered
 *     DatagramHandler callbacks.
 *   - Datagrams for unknown context IDs are silently dropped and counted.
 *   - Thread-safe: all public methods may be called from any I/O thread.
 *
 * Guarded by THEMIS_ENABLE_HTTP3 (requires ngtcp2 + nghttp3 + OpenSSL).
 */

#pragma once

#ifdef THEMIS_ENABLE_HTTP3

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace themis {
namespace server {

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for HTTP/3 datagram support.
 */
struct Http3DatagramConfig {
    /// Enable datagram receive and send.  When false the dispatcher rejects
    /// all registrations and silently discards all incoming datagrams.
    bool enable = true;

    /// Maximum datagram payload size advertised to the remote peer via the
    /// QUIC max_datagram_frame_size transport parameter.  The actual per-packet
    /// limit is the minimum of this value and the QUIC MTU.
    uint64_t max_datagram_frame_size = 65535;

    Http3DatagramConfig() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// Callback type
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Callback invoked when an HTTP/3 datagram arrives for a registered
 *        context ID.
 *
 * @param context_id  The Quarter Stream ID (stream_id / 4) that identifies
 *                    the logical datagram channel.
 * @param data        Pointer to the application payload (Quarter Stream ID
 *                    prefix already stripped).
 * @param len         Length of @p data in bytes.
 */
using DatagramHandler = std::function<void(uint64_t context_id,
                                           const uint8_t* data,
                                           size_t len)>;

// ─────────────────────────────────────────────────────────────────────────────
// Context
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A registered HTTP/3 datagram context (one per logical stream / flow).
 */
struct Http3DatagramContext {
    uint64_t        context_id;  ///< Quarter Stream ID (stream_id / 4)
    DatagramHandler handler;     ///< Called on each incoming datagram
    bool            active = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// Dispatcher
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief HTTP/3 datagram dispatcher.
 *
 * Manages context registrations and routes incoming QUIC datagrams to the
 * appropriate application handler.  Also provides helpers for encoding
 * outbound HTTP/3 datagram frames.
 *
 * Usage (server-side):
 * @code
 *   Http3DatagramDispatcher dispatcher;
 *
 *   // Register a handler for real-time telemetry on stream 4 (context 1).
 *   dispatcher.registerContext(1, [](uint64_t ctx, const uint8_t* d, size_t n) {
 *       processTelemetry(ctx, d, n);
 *   });
 *
 *   // In the ngtcp2 recv_datagram callback:
 *   dispatcher.dispatch(data, datalen);
 *
 *   // Build an outbound datagram for context 1:
 *   auto frame = Http3DatagramDispatcher::encode(1, payload.data(), payload.size());
 *   // frame is ready to pass to ngtcp2_conn_write_datagram().
 * @endcode
 */
class Http3DatagramDispatcher {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    explicit Http3DatagramDispatcher(
        const Http3DatagramConfig& config = Http3DatagramConfig{});

    ~Http3DatagramDispatcher() = default;

    // Non-copyable; moveable.
    Http3DatagramDispatcher(const Http3DatagramDispatcher&)            = delete;
    Http3DatagramDispatcher& operator=(const Http3DatagramDispatcher&) = delete;
    Http3DatagramDispatcher(Http3DatagramDispatcher&&)                 noexcept noexcept = default;
    Http3DatagramDispatcher& operator=(Http3DatagramDispatcher&&)      noexcept noexcept = default;

    // ── Context Management ───────────────────────────────────────────────────

    /**
     * @brief Register a handler for a given context ID.
     *
     * Replaces any existing handler for the same context ID.
     *
     * @param context_id  Quarter Stream ID (stream_id / 4).
     * @param handler     Callback invoked on each incoming datagram.
     * @return true on success, false when datagrams are disabled in config.
     */
    bool registerContext(uint64_t context_id, DatagramHandler handler);

    /**
     * @brief Remove a previously registered context.
     *
     * @return true if the context existed and was removed.
     */
    bool unregisterContext(uint64_t context_id);

    /**
     * @brief Return true if a handler is registered for @p context_id.
     */
    bool hasContext(uint64_t context_id) const;

    // ── Receive Path ─────────────────────────────────────────────────────────

    /**
     * @brief Dispatch a raw HTTP/3 datagram received from the QUIC layer.
     *
     * Decodes the Quarter Stream ID prefix (RFC 9297 §2), looks up the
     * registered handler, and invokes it with the remaining payload.
     * Datagrams for unknown context IDs are dropped and counted.
     *
     * This method is designed to be called directly from the ngtcp2
     * @c recv_datagram callback on an I/O thread.
     *
     * @param data    Pointer to the raw datagram payload (as passed by ngtcp2).
     * @param len     Total length in bytes.
     */
    void dispatch(const uint8_t* data, size_t len);

    // ── Send Path ─────────────────────────────────────────────────────────────

    /**
     * @brief Encode an HTTP/3 datagram frame ready for ngtcp2_conn_write_datagram().
     *
     * Prepends the Quarter Stream ID as a QUIC variable-length integer
     * (RFC 9000 §16) to the application payload.
     *
     * @param context_id  Quarter Stream ID (stream_id / 4).
     * @param payload     Application payload bytes.
     * @param paylen      Length of @p payload.
     * @return Encoded frame bytes (Quarter Stream ID varint + payload).
     */
    static std::vector<uint8_t> encode(uint64_t       context_id,
                                       const uint8_t* payload,
                                       size_t         paylen);

    /**
     * @brief Record that one outbound datagram was successfully written.
     *
     * Called by the session layer after ngtcp2_conn_write_datagram() succeeds.
     */
    void recordSent();

    // ── Codec Helpers (public for testing) ───────────────────────────────────

    /**
     * @brief Encode a QUIC variable-length integer into @p buf.
     *
     * @param value  Value to encode (must be < 2^62).
     * @param buf    Output buffer (must have at least 8 bytes of capacity).
     * @return Number of bytes written, or 0 if @p value is out of range.
     */
    static size_t encodeVarint(uint64_t value, uint8_t* buf);

    /**
     * @brief Decode a QUIC variable-length integer from @p data.
     *
     * @param data      Input buffer.
     * @param len       Available bytes.
     * @param value_out Decoded value on success.
     * @return Bytes consumed, or 0 on error (buffer too short / invalid).
     */
    static size_t decodeVarint(const uint8_t* data, size_t len,
                               uint64_t& value_out);

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t datagrams_received   = 0;  ///< Total datagrams dispatched to decode
        uint64_t datagrams_dispatched = 0;  ///< Datagrams delivered to a handler
        uint64_t datagrams_dropped    = 0;  ///< Unknown context_id or malformed
        uint64_t datagrams_sent       = 0;  ///< Successfully sent outbound datagrams
    };

    Stats getStats() const;

    // ── Config access ─────────────────────────────────────────────────────────

    const Http3DatagramConfig& config() const { return config_; }

private:
    Http3DatagramConfig config_;

    mutable std::mutex                                      contexts_mutex_;
    std::unordered_map<uint64_t, Http3DatagramContext>      contexts_;

    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

}  // namespace server
}  // namespace themis

#endif  // THEMIS_ENABLE_HTTP3
