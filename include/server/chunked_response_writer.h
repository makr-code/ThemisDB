/**
 * @file chunked_response_writer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct ChunkedWriterConfig {
    size_t chunk_size = 100;
    size_t max_items = 0;
    std::string content_type = "application/x-ndjson";
};

class ChunkedResponseWriter {
public:
    /**
     * @brief Encode Chunked Body.
     * @param[in] fragments Input parameter.
     * @return Return value.
     */
    static std::string encodeChunkedBody(const std::vector<std::string>& fragments);

    static http::response<http::string_body> fromFragments(
        const http::request<http::string_body>& req,
        http::status status,
        const std::vector<std::string>& fragments,
        const std::string& content_type = "application/x-ndjson");

    static http::response<http::string_body> fromJsonVector(
        const http::request<http::string_body>& req,
        http::status status,
        const std::vector<nlohmann::json>& items,
        const ChunkedWriterConfig& config = ChunkedWriterConfig{});

    static http::response<http::string_body> fromStream(
        const http::request<http::string_body>& req,
        http::status status,
        std::shared_ptr<query::ResultStream<nlohmann::json>> stream,
        const ChunkedWriterConfig& config = ChunkedWriterConfig{});

    static bool shouldUseChunkedTransfer(
        const http::request<http::string_body>& req,
        size_t item_count,
        size_t threshold = 1000);

    /**
     * @brief Decode Chunked Body.
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static std::string decodeChunkedBody(const std::string& encoded);

private:
    /**
     * @brief Append Chunk.
     * @param[in,out] out Input/output parameter.
     * @param[in] data Input parameter.
     */
    static void appendChunk(std::string& out, const std::string& data);
};

} // namespace server
} // namespace themis
