/**
 * @file chunked_response_writer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstddef>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "query/result_stream.h"

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Configuration for chunked HTTP response streaming.
 */
struct ChunkedWriterConfig {
    /// Number of result items per HTTP chunk (default: 100).
    size_t chunk_size = 100;
    /// Maximum number of items to stream in a single response (0 = unlimited).
    size_t max_items = 0;
    /// MIME type of each chunk's content (default: application/x-ndjson).
    std::string content_type = "application/x-ndjson";
};

/**
 * @brief Utility for building HTTP/1.1 chunked transfer-encoded responses.
 *
 * `ChunkedResponseWriter` converts an ordered collection of string fragments
 * (or a `ResultStream<nlohmann::json>`) into a properly RFC 7230-encoded
 * chunked body and returns an `http::response<http::string_body>` with
 *   - `Transfer-Encoding: chunked`
 *   - no `Content-Length` header
 *
 * ### Why chunked transfer?
 * Large query result sets may be megabytes in size.  Clients that support
 * HTTP/1.1 streaming (curl, browsers using the Fetch streaming API, etc.)
 * can begin processing the first batch of records while the server is still
 * serialising the remaining ones – reducing perceived first-byte latency.
 *
 * ### Body format
 * Each chunk contains one or more newline-delimited JSON records (NDJSON),
 * where every record is one JSON object followed by `\n`.  The final `0\r\n\r\n`
 * terminator is appended automatically.
 *
 * ### Usage example
 * ```cpp
 * // Stream query results
 * auto stream = createKeyStream(keys);
 * auto response = ChunkedResponseWriter::fromStream(
 *     req, http::status::ok, stream, config);
 * return response;
 *
 * // Stream a pre-built list of JSON objects
 * std::vector<nlohmann::json> items = { ... };
 * auto response = ChunkedResponseWriter::fromJsonVector(
 *     req, http::status::ok, items, ChunkedWriterConfig{});
 * return response;
 * ```
 *
 * @note The current implementation pre-encodes the entire body in memory
 *       before writing, which is consistent with the existing server
 *       architecture (all handlers return a complete `string_body`).
 *       True async streaming (writing chunks as they become available) is
 *       tracked in a follow-up issue.
 */
class ChunkedResponseWriter {
public:
    /**
     * @brief RFC 7230 §4.1 chunk encoding.
     *
     * Encodes `fragments` into a single chunked-transfer body string:
     * ```
     * <hex-length>\r\n<data>\r\n
     * ...
     * 0\r\n\r\n
     * ```
     * Empty fragments are skipped to avoid zero-size intermediate chunks.
     *
     * @param fragments  Non-empty data fragments; each becomes one chunk.
     * @return Encoded body string including the terminal empty chunk.
     */
    static std::string encodeChunkedBody(const std::vector<std::string>& fragments);

    /**
     * @brief Build a chunked HTTP response from pre-built string fragments.
     *
     * @param req        Originating HTTP request (used for version / keep-alive).
     * @param status     HTTP status code.
     * @param fragments  Each element becomes one HTTP chunk in the body.
     * @param content_type  Value for the `Content-Type` response header.
     * @return HTTP response with `Transfer-Encoding: chunked`.
     */
    static http::response<http::string_body> fromFragments(
        const http::request<http::string_body>& req,
        http::status status,
        const std::vector<std::string>& fragments,
        const std::string& content_type = "application/x-ndjson");

    /**
     * @brief Build a chunked HTTP response from a vector of JSON objects.
     *
     * Items are serialised as NDJSON (one JSON object per line) and batched
     * into chunks of `config.chunk_size` items each.
     *
     * @param req     Originating HTTP request.
     * @param status  HTTP status code.
     * @param items   JSON objects to stream.
     * @param config  Streaming configuration.
     * @return HTTP response with `Transfer-Encoding: chunked`.
     */
    static http::response<http::string_body> fromJsonVector(
        const http::request<http::string_body>& req,
        http::status status,
        const std::vector<nlohmann::json>& items,
        const ChunkedWriterConfig& config = ChunkedWriterConfig{});

    /**
     * @brief Build a chunked HTTP response by consuming a `ResultStream<nlohmann::json>`.
     *
     * The stream is drained in batches of `config.chunk_size` items.  Each
     * batch becomes one HTTP chunk of NDJSON text.  Up to `config.max_items`
     * items are consumed (unlimited when 0).
     *
     * @param req     Originating HTTP request.
     * @param status  HTTP status code.
     * @param stream  Result stream to drain (must not be nullptr).
     * @param config  Streaming configuration.
     * @return HTTP response with `Transfer-Encoding: chunked`.
     */
    static http::response<http::string_body> fromStream(
        const http::request<http::string_body>& req,
        http::status status,
        std::shared_ptr<query::ResultStream<nlohmann::json>> stream,
        const ChunkedWriterConfig& config = ChunkedWriterConfig{});

    /**
     * @brief Decide whether chunked transfer should be used for `item_count` items.
     *
     * Returns `true` when:
     *  - the request is HTTP/1.1, AND
     *  - `item_count` exceeds `threshold` (default 1000).
     *
     * HTTP/1.0 clients do not understand chunked encoding and must receive a
     * regular content-length response.
     *
     * @param req         Incoming request.
     * @param item_count  Number of result items that will be returned.
     * @param threshold   Minimum item count to trigger chunked mode.
     */
    static bool shouldUseChunkedTransfer(
        const http::request<http::string_body>& req,
        size_t item_count,
        size_t threshold = 1000);

    /**
     * @brief Decode an RFC 7230 chunked body back to plain bytes.
     *
     * Used by protocol adapters (e.g. HTTP/2 session) that need to strip
     * chunked framing before forwarding the body over a transport that does
     * not support `Transfer-Encoding: chunked`.
     *
     * @param encoded  Raw chunked body (as produced by encodeChunkedBody).
     * @return Decoded body.  Returns an empty string if `encoded` is empty.
     */
    static std::string decodeChunkedBody(const std::string& encoded);

private:
    /// Append one chunk to `out` using RFC 7230 encoding.
    static void appendChunk(std::string& out, const std::string& data);
};

} // namespace server
} // namespace themis
