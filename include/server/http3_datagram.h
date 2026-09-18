/**
 * @file http3_datagram.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct Http3DatagramConfig {
    bool enable = true;

    uint64_t max_datagram_frame_size = 65535;

    Http3DatagramConfig() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// Callback type
// ─────────────────────────────────────────────────────────────────────────────

using DatagramHandler = std::function<void(uint64_t context_id,
                                           const uint8_t* data,
                                           size_t len)>;

// ─────────────────────────────────────────────────────────────────────────────
// Context
// ─────────────────────────────────────────────────────────────────────────────

struct Http3DatagramContext {
    uint64_t        context_id = 0;  ///< Quarter Stream ID (stream_id / 4)
    DatagramHandler handler;     ///< Called on each incoming datagram
    bool            active = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// Dispatcher
// ─────────────────────────────────────────────────────────────────────────────

class Http3DatagramDispatcher {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    explicit Http3DatagramDispatcher(
        const Http3DatagramConfig& config = Http3DatagramConfig{});

    ~Http3DatagramDispatcher() = default;

    // Non-copyable; moveable.
    Http3DatagramDispatcher(const Http3DatagramDispatcher&)            = delete;
    Http3DatagramDispatcher& operator=(const Http3DatagramDispatcher&) = delete;
    Http3DatagramDispatcher(Http3DatagramDispatcher&&)                 noexcept = default;
    Http3DatagramDispatcher& operator=(Http3DatagramDispatcher&&)      noexcept = default;


    /**
     * @brief Register Context.
     * @param[in] context_id Identifier of the context.
     * @param[in] handler Input parameter.
     * @return True when the operation succeeds.
     */
    bool registerContext(uint64_t context_id, DatagramHandler handler);

    /**
     * @brief Unregister Context.
     * @param[in] context_id Identifier of the context.
     * @return True when the operation succeeds.
     */
    bool unregisterContext(uint64_t context_id);

    /**
     * @brief Has Context.
     * @param[in] context_id Identifier of the context.
     * @return True when the operation succeeds.
     */
    bool hasContext(uint64_t context_id) const;


    /**
     * @brief Dispatch.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     */
    void dispatch(const uint8_t* data, size_t len);


    /**
     * @brief Encode.
     * @param[in] context_id Identifier of the context.
     * @param[in] payload Input parameter.
     * @param[in] paylen Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> encode(uint64_t       context_id,
                                       const uint8_t* payload,
                                       size_t         paylen);

    /**
     * @brief Record Sent.
     */
    void recordSent();


    /**
     * @brief Encode Varint.
     * @param[in] value Input parameter.
     * @param[in,out] buf Input/output parameter.
     * @return Return value.
     */
    static size_t encodeVarint(uint64_t value, uint8_t* buf);

    /**
     * @brief Decode Varint.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @param[in,out] value_out Input/output parameter.
     * @return Return value.
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

    /**
     * @brief Get Stats.
     * @return Return value.
     */
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
