#pragma once

/**
 * @file streaming_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief Stateless helper class for SSE / chunked-response token streaming
 *
 * All methods are static so the class can be used without instantiation.
 * The `makeStreamCallback` factory manages token-index state through a
 * captured shared counter, making the returned callback thread-safe for
 * single-producer scenarios.
 */
class StreamingHandler {
public:
    /**
     * @brief Format a single token as an SSE `data:` line.
     *
     * Produces a JSON payload embedded in the SSE wire format:
     * @code
     *   data: {"id":"<request_id>","token":"<token>","index":<idx>,"done":false}\n\n
     * @endcode
     *
     * When @p done is true, the token field is empty and `"done":true` is set
     * instead of emitting the separate [DONE] sentinel.  For the canonical
     * OpenAI-style termination sentinel use formatDoneEvent().
     *
     * @param token      The token text to stream (may be empty when done=true).
     * @param request_id Identifier of the originating request.
     * @param index      Zero-based position of this token in the stream.
     * @param done       Set to true to mark the last event of the stream.
     * @return SSE-formatted string ready to write to the HTTP response body.
     *
     * @note The returned string already contains the double newline (`\n\n`)
     *       required by the SSE specification.
     */
    static std::string formatSseEvent(
        const std::string& token,
        const std::string& request_id,
        size_t index,
        bool done = false);

    /**
     * @brief Format the terminal [DONE] SSE event.
     *
     * Produces the canonical OpenAI-compatible stream termination marker:
     * @code
     *   data: [DONE]\n\n
     * @endcode
     *
     * @param request_id Identifier of the originating request (unused in the
     *                   wire format but kept for API symmetry and logging).
     * @return SSE-formatted termination string.
     */
    static std::string formatDoneEvent(const std::string& request_id);

    /**
     * @brief Wrap raw bytes in HTTP chunked-transfer encoding.
     *
     * Produces one chunk frame:
     * @code
     *   <hex-length>\r\n<data>\r\n
     * @endcode
     *
     * To signal end-of-stream, pass an empty string — this produces the
     * terminal zero-length chunk `0\r\n\r\n`.
     *
     * @param data Raw bytes to wrap (may be empty to signal end-of-stream).
     * @return Chunked-encoded string.
     */
    static std::string formatChunkedData(const std::string& data);

    /**
     * @brief Build an InferenceRequest::stream_callback that emits SSE events.
     *
     * The returned lambda maintains an internal token-index counter.  Each
     * invocation formats the received token as an SSE event (via
     * formatSseEvent()) and forwards the result to @p sink.
     *
     * @param sink       Output callable that accepts a pre-formatted SSE
     *                   string.  Called once per token on the inference
     *                   worker thread.
     * @param request_id Request identifier embedded in each SSE event.
     * @return Callback suitable for InferenceRequest::stream_callback.
     *
     * @note The counter shared inside the lambda is atomic so the callback
     *       is safe for single-producer / single-consumer use cases.
     */
    static std::function<void(const std::string&)> makeStreamCallback(
        std::function<void(const std::string&)> sink,
        const std::string& request_id);
};

} // namespace llm
} // namespace themis

