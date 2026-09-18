#pragma once

/**
 * @file streaming_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace themis {
namespace llm {

class StreamingHandler {
public:
    static std::string formatSseEvent(
        const std::string& token,
        const std::string& request_id,
        size_t index,
        bool done = false);

    /**
     * @brief Format Done Event.
     * @param[in] request_id Identifier of the request.
     * @return Return value.
     */
    static std::string formatDoneEvent(const std::string& request_id);

    /**
     * @brief Format Chunked Data.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::string formatChunkedData(const std::string& data);

    static std::function<void(const std::string&)> makeStreamCallback(
        std::function<void(const std::string&)> sink,
        const std::string& request_id);
};

} // namespace llm
} // namespace themis

