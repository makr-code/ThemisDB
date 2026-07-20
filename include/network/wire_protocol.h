/**
 * @file wire_protocol.h
 * @brief ThemisDB binary wire protocol packet parser with iterator-safe buffer access.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416 iterator safety applied (Sprint 7 Batch C, Phase 2B)
 * @note Status: Production Ready
 *
 * Implements the low-level packet framing and field-parsing layer of the
 * ThemisDB binary wire protocol.  All buffer iterators are validated through
 * `themis::security::SafeIterator` before every dereference or advance,
 * eliminating the CWE-129/CWE-416 vulnerabilities identified in the Sprint 7
 * gap scan (gap IDs B001–B007).
 *
 * **Protocol framing:**
 * ```
 * ┌──────────┬──────────┬──────────┬────────────────┐
 * │ magic[2] │  type[1] │  len[4]  │  payload[len]  │
 * └──────────┴──────────┴──────────┴────────────────┘
 * ```
 * - magic: 0xDB 0x01
 * - type:  @ref PacketType
 * - len:   payload length in network byte order (big-endian)
 * - payload: type-dependent fields
 *
 * **CWE Remediations applied:**
 * - CWE-129 (Improper Array Index Validation): every `advance()` call goes
 *   through `AdvanceSafe::advance()` so user-supplied lengths cannot walk the
 *   iterator past `end()`.
 * - CWE-416 (Use-After-Free): `BoundsChecker::check_dereference()` guards
 *   every single-byte read.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "security/safe_iterator.h"

namespace themis {
namespace network {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Protocol magic bytes (first two bytes of every packet).
inline constexpr std::uint8_t kWireProtocolMagic0 = 0xDB;
inline constexpr std::uint8_t kWireProtocolMagic1 = 0x01;

/// Minimum packet size (magic + type + length-field).
inline constexpr std::size_t kWireProtocolMinPacketSize = 7;

/// Maximum allowed payload length (64 MiB) — prevents OOM via malformed packets.
inline constexpr std::size_t kWireProtocolMaxPayloadBytes = 64u * 1024u * 1024u;

// ---------------------------------------------------------------------------
// PacketType
// ---------------------------------------------------------------------------

/**
 * @brief Enumeration of all supported protocol packet types.
 *
 * Values are encoded as a single byte in the framing header.
 */
enum class PacketType : std::uint8_t {
    kHandshake    = 0x01, ///< Client→Server connection setup
    kQuery        = 0x02, ///< Client→Server AQL query
    kQueryResult  = 0x03, ///< Server→Client query result rows
    kPut          = 0x04, ///< Client→Server key/value write
    kGet          = 0x05, ///< Client→Server key/value read
    kDelete       = 0x06, ///< Client→Server delete
    kError        = 0x10, ///< Server→Client error response
    kPing         = 0x20, ///< Keep-alive ping
    kPong         = 0x21, ///< Keep-alive pong
    kUnknown      = 0xFF,
};

// ---------------------------------------------------------------------------
// ParseError
// ---------------------------------------------------------------------------

/**
 * @brief Exception type raised on any protocol parsing failure.
 *
 * Thrown by `PacketParser` when a malformed or truncated packet is presented.
 * Callers should catch and close the connection.
 */
class ParseError : public std::runtime_error {
public:
    /**
     * @brief Construct with a descriptive message.
     * @param msg Human-readable description of the parse failure.
     */
    explicit ParseError(const std::string& msg)
        : std::runtime_error("WireProtocol parse error: " + msg) {}
};

// ---------------------------------------------------------------------------
// WirePacket
// ---------------------------------------------------------------------------

/**
 * @brief Represents one fully-parsed wire protocol packet.
 *
 * Immutable value type; constructed exclusively by `PacketParser::parse()`.
 */
struct WirePacket {
    PacketType           type;     ///< Decoded packet type.
    std::vector<uint8_t> payload;  ///< Raw payload bytes (may be empty).
};

// ---------------------------------------------------------------------------
// PacketParser
// ---------------------------------------------------------------------------

/**
 * @brief Stateless binary packet parser with iterator-safe buffer access.
 *
 * All buffer traversal is performed through `themis::security::SafeIterator`
 * primitives.  Parsing rejects packets that are:
 * - Too short to contain the framing header.
 * - Missing the expected magic bytes.
 * - Carrying a payload longer than `kWireProtocolMaxPayloadBytes`.
 * - Shorter than the header-declared payload length.
 *
 * **Usage:**
 * ```cpp
 * std::vector<uint8_t> raw = receive_bytes();
 * WirePacket pkt = PacketParser::parse(raw);
 * ```
 *
 * @note This class is stateless; all methods are static.
 */
class PacketParser {
public:
    PacketParser() = delete;

    /**
     * @brief Parse one wire packet from a raw byte buffer.
     *
     * @param buf Raw bytes received from the network.
     * @return Fully decoded `WirePacket`.
     * @throws ParseError if the packet is malformed, truncated, or oversized.
     *
     * **Iterator safety:** Every buffer position is validated through
     * `BoundsChecker::check_dereference()` before the byte is read, and
     * every multi-byte skip uses `AdvanceSafe::advance()`.
     */
    static WirePacket parse(const std::vector<uint8_t>& buf);

    /**
     * @brief Read one field of type T from a buffer iterator.
     *
     * @tparam T  Trivially-copyable integer type (uint8_t, uint16_t, uint32_t …).
     * @param it  Current buffer position (advanced by `sizeof(T)` on return).
     * @param end One-past-the-end sentinel.
     * @return Value decoded in host byte order (big-endian source assumed).
     * @throws ParseError if fewer than `sizeof(T)` bytes remain.
     *
     * **Iterator safety:** uses `AdvanceSafe::advance()` internally.
     */
    template<typename T>
    static T read_big_endian(std::vector<uint8_t>::const_iterator& it,
                             const std::vector<uint8_t>::const_iterator& end);

    /**
     * @brief Read exactly `n` payload bytes starting at `it`.
     *
     * @param it   Current position (advanced by `n` on return).
     * @param end  Buffer sentinel.
     * @param n    Number of bytes to read.
     * @return     Byte span copied into a new vector.
     * @throws ParseError if fewer than `n` bytes remain.
     */
    static std::vector<uint8_t> read_bytes(
        std::vector<uint8_t>::const_iterator&       it,
        const std::vector<uint8_t>::const_iterator& end,
        std::size_t n);

private:
    /// Validate magic header bytes; throws ParseError on mismatch.
    static void check_magic(std::vector<uint8_t>::const_iterator& it,
                            const std::vector<uint8_t>::const_iterator& end);
};

// ---------------------------------------------------------------------------
// PacketBuilder
// ---------------------------------------------------------------------------

/**
 * @brief Constructs outgoing wire packets with safe buffer appending.
 *
 * Provides type-safe methods for emitting each `PacketType`, handling
 * big-endian encoding and payload length framing automatically.
 */
class PacketBuilder {
public:
    /**
     * @brief Build an error response packet.
     * @param code   Application-level error code.
     * @param message Human-readable description (UTF-8, max 4096 bytes).
     * @return Fully framed packet bytes ready for transmission.
     */
    static std::vector<uint8_t> build_error(uint32_t code, std::string_view message);

    /**
     * @brief Build a pong keep-alive response.
     * @return Framed PONG packet (header only, no payload).
     */
    static std::vector<uint8_t> build_pong();

    /**
     * @brief Build a query-result packet from a row vector.
     * @param rows  Serialised result rows; each element is one row's wire bytes.
     * @return Framed packet with all rows concatenated in the payload.
     * @throws std::length_error if total payload would exceed
     *         `kWireProtocolMaxPayloadBytes`.
     */
    static std::vector<uint8_t> build_query_result(
        const std::vector<std::vector<uint8_t>>& rows);

private:
    /// Append a uint32_t in big-endian order to `buf`.
    static void append_be32(std::vector<uint8_t>& buf, uint32_t v);

    /// Append the common 7-byte framing header.
    static void append_header(std::vector<uint8_t>& buf,
                              PacketType type, uint32_t payload_len);
};

}  // namespace network
}  // namespace themis
