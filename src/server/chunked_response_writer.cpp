/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chunked_response_writer.cpp                        ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/chunked_response_writer.h"

#include <iomanip>
#include <sstream>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

void ChunkedResponseWriter::appendChunk(std::string& out, const std::string& data) {
    // chunk-size in hex
    std::ostringstream hex;
    hex << std::hex << data.size();
    out += hex.str();
    out += "\r\n";
    out += data;
    out += "\r\n";
}

// ---------------------------------------------------------------------------
// encodeChunkedBody
// ---------------------------------------------------------------------------

std::string ChunkedResponseWriter::encodeChunkedBody(
    const std::vector<std::string>& fragments)
{
    std::string body;
    // Pre-allocate: rough estimate (hex digits + CRLF overhead per chunk)
    for (const auto& frag : fragments) {
        body.reserve(body.size() + frag.size() + 16);
        if (!frag.empty()) {
            appendChunk(body, frag);
        }
    }
    // Terminal chunk
    body += "0\r\n\r\n";
    return body;
}

// ---------------------------------------------------------------------------
// fromFragments
// ---------------------------------------------------------------------------

http::response<http::string_body> ChunkedResponseWriter::fromFragments(
    const http::request<http::string_body>& req,
    http::status status,
    const std::vector<std::string>& fragments,
    const std::string& content_type)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, content_type);
    res.set(http::field::transfer_encoding, "chunked");
    res.keep_alive(req.keep_alive());

    res.body() = encodeChunkedBody(fragments);
    // Intentionally do NOT call prepare_payload() – that would add
    // Content-Length and remove Transfer-Encoding: chunked.
    return res;
}

// ---------------------------------------------------------------------------
// fromJsonVector
// ---------------------------------------------------------------------------

http::response<http::string_body> ChunkedResponseWriter::fromJsonVector(
    const http::request<http::string_body>& req,
    http::status status,
    const std::vector<nlohmann::json>& items,
    const ChunkedWriterConfig& config)
{
    const size_t chunk_size = config.chunk_size > 0 ? config.chunk_size : 100;
    const size_t max_items  = config.max_items;

    std::vector<std::string> fragments;

    std::string current_chunk;
    size_t count = 0;

    for (const auto& item : items) {
        if (max_items > 0 && count >= max_items) {
            break;
        }

        current_chunk += item.dump();
        current_chunk += '\n';
        ++count;

        if (count % chunk_size == 0) {
            fragments.push_back(std::move(current_chunk));
            current_chunk.clear();
        }
    }

    // Flush any remaining items
    if (!current_chunk.empty()) {
        fragments.push_back(std::move(current_chunk));
    }

    return fromFragments(req, status, fragments, config.content_type);
}

// ---------------------------------------------------------------------------
// fromStream
// ---------------------------------------------------------------------------

http::response<http::string_body> ChunkedResponseWriter::fromStream(
    const http::request<http::string_body>& req,
    http::status status,
    std::shared_ptr<query::ResultStream<nlohmann::json>> stream,
    const ChunkedWriterConfig& config)
{
    const size_t chunk_size = config.chunk_size > 0 ? config.chunk_size : 100;
    const size_t max_items  = config.max_items;

    std::vector<std::string> fragments;
    size_t total = 0;

    while (stream->hasNext()) {
        if (max_items > 0 && total >= max_items) {
            break;
        }

        size_t remaining = (max_items > 0) ? (max_items - total) : chunk_size;
        size_t batch     = std::min(chunk_size, remaining);

        auto result = stream->nextBatch(batch);
        if (!result) {
            break;
        }

        if (result->items.empty()) {
            break;
        }

        std::string chunk_data;
        for (const auto& item : result->items) {
            chunk_data += item.dump();
            chunk_data += '\n';
        }
        total += result->items.size();
        fragments.push_back(std::move(chunk_data));
    }

    return fromFragments(req, status, fragments, config.content_type);
}

// ---------------------------------------------------------------------------
// shouldUseChunkedTransfer
// ---------------------------------------------------------------------------

bool ChunkedResponseWriter::shouldUseChunkedTransfer(
    const http::request<http::string_body>& req,
    size_t item_count,
    size_t threshold)
{
    // HTTP/1.0 does not support chunked transfer encoding
    if (req.version() < 11) {
        return false;
    }
    return item_count >= threshold;
}

} // namespace server
} // namespace themis
