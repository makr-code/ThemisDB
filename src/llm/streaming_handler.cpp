/**
 * @file streaming_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/streaming_handler.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// SSE helpers
// ═══════════════════════════════════════════════════════════

/**
 * @brief Escape a string for safe embedding inside a JSON string value.
 *
 * Replaces the minimal set of characters that would break JSON string
 * parsing: reverse solidus, double-quote, and the C0 control characters
 * mandated by RFC 8259 §7.
 *
 * @param s Raw input string.
 * @return JSON-safe escaped string (without surrounding quotes).
 */
static std::string escapeJsonString(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    // Encode other control characters as \uXXXX
                    std::ostringstream esc = {};
                    esc << "\\u" << std::hex << std::setw(4)
                        << std::setfill('0') << static_cast<int>(c);
                    out += esc.str();
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

// ═══════════════════════════════════════════════════════════
// StreamingHandler — public static methods
// ═══════════════════════════════════════════════════════════

std::string StreamingHandler::formatSseEvent(
    const std::string& token,
    const std::string& request_id,
    size_t index,
    bool done)
{
    // Build the JSON payload inline to avoid pulling in a full JSON library
    // for this lightweight, hot-path helper.
    std::string payload = {};
    payload.reserve(128);
    payload += "{\"id\":\"";
    payload += escapeJsonString(request_id);
    payload += "\",\"token\":\"";
    payload += escapeJsonString(token);
    payload += "\",\"index\":";
    payload += std::to_string(index);
    payload += ",\"done\":";
    payload += done ? "true" : "false";
    payload += '}';

    // SSE wire format: "data: <payload>\n\n"
    return "data: " + payload + "\n\n";
}

std::string StreamingHandler::formatDoneEvent(
    const std::string& request_id)
{
    // OpenAI-compatible terminal sentinel — request_id is not part of the
    // wire format but is logged for traceability.
    spdlog::debug("StreamingHandler: emitting [DONE] for request {}", request_id);
    return "data: [DONE]\n\n";
}

std::string StreamingHandler::formatChunkedData([[maybe_unused]] const std::string& data) {
    // HTTP/1.1 chunked-transfer encoding:
    //   <hex-length>\r\n<data>\r\n
    // An empty data string produces the terminal zero-length chunk.
    std::ostringstream oss = {};
    oss << std::hex <<static_cast<int>(data.size()) << "\r\n" << data << "\r\n";
    return oss.str();
}

std::function<void(const std::string&)> StreamingHandler::makeStreamCallback(
    std::function<void(const std::string&)> sink,
    const std::string& request_id)
{
    if (!sink) {
        throw std::invalid_argument(
            "StreamingHandler::makeStreamCallback: sink must not be null");
    }

    // Share the counter between the lambda and potential copies.
    auto counter = std::make_shared<std::atomic<size_t>>(0);
    std::string req_id = request_id;

    return [sink = std::move(sink), counter, req_id](const std::string& token) {
        size_t idx = counter->fetch_add(1, std::memory_order_relaxed);
        sink(StreamingHandler::formatSseEvent(token, req_id, idx, /*done=*/false));
    };
}

} // namespace llm
} // namespace themis

