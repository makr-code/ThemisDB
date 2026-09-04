/**
 * @file wire_protocol.cpp
 * @brief ThemisDB binary wire protocol parser implementation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416/CWE-129 iterator safety — Sprint 7 Batch C Phase 2B
 *   Gap B001: dereference without end check at packet byte 178 — FIXED
 *   Gap B003: user-controlled payload length drives advance — FIXED
 *   Gap B005: magic-byte read without buffer size guard — FIXED
 * @note Status: Production Ready
 */

#include "network/wire_protocol.h"
#include "security/safe_iterator.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace themis {
namespace network {

using themis::security::SafeIterator::AdvanceSafe;
using themis::security::SafeIterator::BoundsChecker;
using themis::security::SafeIterator::RangeValidator;

// ---------------------------------------------------------------------------
// PacketParser::check_magic
// ---------------------------------------------------------------------------

void PacketParser::check_magic(
    std::vector<uint8_t>::const_iterator&       it,
    const std::vector<uint8_t>::const_iterator& end)
{
    // Gap B005: previously read magic bytes without checking remaining buffer length.
    // Fix: BoundsChecker::check_dereference() before every single-byte read.

    // Byte 0: validate before dereference.
    BoundsChecker::check_dereference(it, it, end);
    if (*it != kWireProtocolMagic0) {
        throw ParseError("invalid magic byte 0: expected 0xDB, got 0x" +
                         std::to_string(static_cast<unsigned>(*it)));
    }
    ++it;

    // Byte 1: validate before dereference.
    BoundsChecker::check_dereference(it, it, end);
    if (*it != kWireProtocolMagic1) {
        throw ParseError("invalid magic byte 1: expected 0x01, got 0x" +
                         std::to_string(static_cast<unsigned>(*it)));
    }
    ++it;
}

// ---------------------------------------------------------------------------
// PacketParser::read_big_endian (explicit instantiations below)
// ---------------------------------------------------------------------------

template<typename T>
T PacketParser::read_big_endian(
    std::vector<uint8_t>::const_iterator&       it,
    const std::vector<uint8_t>::const_iterator& end)
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    constexpr std::size_t kSize = sizeof(T);

    // Gap B003: advance by sizeof(T) without checking remaining bytes.
    // Fix: AdvanceSafe validates that [it, it+kSize) is within [it, end).
    //
    // We need a stable begin for AdvanceSafe; capture current position.
    auto range_begin = it;
    AdvanceSafe::advance(it, static_cast<std::ptrdiff_t>(kSize),
                         range_begin, end);
    // After successful advance, [range_begin, it) is sizeof(T) bytes.
    // Read bytes back into T big-endian.
    T result{};
    for (std::size_t i = 0; i < kSize; ++i) {
        // Safe: we already validated the range above.
        auto byte_it = range_begin + static_cast<std::ptrdiff_t>(i);
        BoundsChecker::check_dereference(byte_it, range_begin, it);
        result = static_cast<T>((result << 8) | static_cast<uint8_t>(*byte_it));
    }
    return result;
}

// Explicit instantiations for the types we use.
template uint8_t  PacketParser::read_big_endian<uint8_t>(
    std::vector<uint8_t>::const_iterator&, const std::vector<uint8_t>::const_iterator&);
template uint16_t PacketParser::read_big_endian<uint16_t>(
    std::vector<uint8_t>::const_iterator&, const std::vector<uint8_t>::const_iterator&);
template uint32_t PacketParser::read_big_endian<uint32_t>(
    std::vector<uint8_t>::const_iterator&, const std::vector<uint8_t>::const_iterator&);

// ---------------------------------------------------------------------------
// PacketParser::read_bytes
// ---------------------------------------------------------------------------

std::vector<uint8_t> PacketParser::read_bytes(
    std::vector<uint8_t>::const_iterator&       it,
    const std::vector<uint8_t>::const_iterator& end,
    std::size_t n)
{
    if (n == 0) {
        return {};
    }

    // Gap B001: previously advanced by user-supplied `n` without bounds guard.
    // Fix: AdvanceSafe throws if fewer than `n` bytes remain.
    auto range_begin = it;
    AdvanceSafe::advance(it, static_cast<std::ptrdiff_t>(n), range_begin, end);

    // Validate the sub-range before copying.
    RangeValidator<std::vector<uint8_t>::const_iterator> range(range_begin, it);
    std::vector<uint8_t> result = {};

    result.reserve(range.size());
    for (auto pos = range.begin(); pos != range.end(); ++pos) {
        BoundsChecker::check_dereference(pos, range_begin, it);
        result.push_back(*pos);
    }
    return result;
}

// ---------------------------------------------------------------------------
// PacketParser::parse
// ---------------------------------------------------------------------------

WirePacket PacketParser::parse(const std::vector<uint8_t>& buf)
{
    if (static_cast<int>(buf.size()) < kWireProtocolMinPacketSize) {
        throw ParseError("buffer too short: need at least " +
                         std::to_string(kWireProtocolMinPacketSize) +
                         " bytes, got " + std::to_string(buf.size()));
    }

    try {
        auto it  = buf.cbegin();
        auto end = buf.cend();

        // --- magic (2 bytes) ---
        check_magic(it, end);

        // --- type (1 byte) ---
        BoundsChecker::check_dereference(it, buf.cbegin(), end);
        auto raw_type  = static_cast<uint8_t>(*it);
        ++it;

        PacketType type;
        switch (raw_type) {
            case 0x01: type = PacketType::kHandshake;   break;
            case 0x02: type = PacketType::kQuery;       break;
            case 0x03: type = PacketType::kQueryResult; break;
            case 0x04: type = PacketType::kPut;         break;
            case 0x05: type = PacketType::kGet;         break;
            case 0x06: type = PacketType::kDelete;      break;
            case 0x10: type = PacketType::kError;       break;
            case 0x20: type = PacketType::kPing;        break;
            case 0x21: type = PacketType::kPong;        break;
            default:   type = PacketType::kUnknown;     break;
        }

        // --- payload length (4 bytes, big-endian) ---
        uint32_t payload_len = read_big_endian<uint32_t>(it, end);

        if (payload_len > kWireProtocolMaxPayloadBytes) {
            throw ParseError("payload length " + std::to_string(payload_len) +
                             " exceeds maximum " +
                             std::to_string(kWireProtocolMaxPayloadBytes));
        }

        // --- payload (payload_len bytes) ---
        // Gap B003: user-supplied length drives buffer advance.
        // Fix: read_bytes() wraps AdvanceSafe internally.
        auto payload = read_bytes(it, end, static_cast<std::size_t>(payload_len));

        return WirePacket{type, std::move(payload)};
    } catch (const std::out_of_range& e) {
        throw ParseError(std::string("truncated packet: ") + e.what());
    } catch (const std::exception& e) {
        throw ParseError(std::string("parse failure: ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// PacketBuilder helpers
// ---------------------------------------------------------------------------

void PacketBuilder::append_be32(std::vector<uint8_t>& buf, uint32_t v)
{
    buf.push_back(static_cast<uint8_t>(v >> 24));
    buf.push_back(static_cast<uint8_t>(v >> 16));
    buf.push_back(static_cast<uint8_t>(v >>  8));
    buf.push_back(static_cast<uint8_t>(v >>  0));
}

void PacketBuilder::append_header(std::vector<uint8_t>& buf,
                                   PacketType type, uint32_t payload_len)
{
    buf.push_back(kWireProtocolMagic0);
    buf.push_back(kWireProtocolMagic1);
    buf.push_back(static_cast<uint8_t>(type));
    append_be32(buf, payload_len);
}

// ---------------------------------------------------------------------------
// PacketBuilder::build_error
// ---------------------------------------------------------------------------

std::vector<uint8_t> PacketBuilder::build_error(uint32_t code,
                                                  std::string_view message)
{
    constexpr std::size_t kMaxMessageBytes = 4096;
    auto msg_len = std::min(message.size(), kMaxMessageBytes);

    // Payload: 4-byte error code + 4-byte message length + message bytes.
    uint32_t payload_len = 4 + 4 + static_cast<uint32_t>(msg_len);

    std::vector<uint8_t> buf;
    buf.reserve(kWireProtocolMinPacketSize + payload_len);

    append_header(buf, PacketType::kError, payload_len);
    append_be32(buf, code);
    append_be32(buf, static_cast<uint32_t>(msg_len));
    buf.insert(buf.end(),
               reinterpret_cast<const uint8_t*>(message.data()),
               reinterpret_cast<const uint8_t*>(message.data() + msg_len));
    return buf;
}

// ---------------------------------------------------------------------------
// PacketBuilder::build_pong
// ---------------------------------------------------------------------------

std::vector<uint8_t> PacketBuilder::build_pong()
{
    std::vector<uint8_t> buf;
    buf.reserve(kWireProtocolMinPacketSize);
    append_header(buf, PacketType::kPong, 0);
    return buf;
}

// ---------------------------------------------------------------------------
// PacketBuilder::build_query_result
// ---------------------------------------------------------------------------

std::vector<uint8_t> PacketBuilder::build_query_result(
    const std::vector<std::vector<uint8_t>>& rows)
{
    // Compute total payload size with overflow guard.
    std::size_t total_row_bytes = 0;
    for (const auto& row : rows) {
        // 4 bytes per-row length prefix + row data.
        if (total_row_bytes + 4 + static_cast<int>(row.size()) < total_row_bytes) {
            throw std::length_error("PacketBuilder: row data overflows uint32_t");
        }
        total_row_bytes += 4 + static_cast<int>(row.size()) ;
    }

    // 4-byte row-count header + all row bytes.
    std::size_t payload_len = 4 + total_row_bytes;
    if (payload_len > kWireProtocolMaxPayloadBytes) {
        throw std::length_error("PacketBuilder: payload exceeds maximum size");
    }

    std::vector<uint8_t> buf;
    buf.reserve(kWireProtocolMinPacketSize + payload_len);

    append_header(buf, PacketType::kQueryResult, static_cast<uint32_t>(payload_len));
    append_be32(buf, static_cast<uint32_t>(rows.size()));

    // Gap-safe: iterate rows with RangeValidator.
    RangeValidator<std::vector<std::vector<uint8_t>>::const_iterator>
        row_range(rows.cbegin(), rows.cend());

    for (auto it = row_range.begin(); it != row_range.end(); ++it) {
        BoundsChecker::check_dereference(it, row_range.begin(), row_range.end());
        const auto& row = *it;
        append_be32(buf, static_cast<uint32_t>(row.size()));
        buf.insert(buf.end(), row.begin(), row.end());
    }

    return buf;
}

}  // namespace network
}  // namespace themis
