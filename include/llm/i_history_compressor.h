/**
 * @file i_history_compressor.h
 * @brief History compression interface for AQL conversation context episodic memory.
 * @version 0.1.0-beta
 * @note Maturity: BETA (Phase 2 P2-D03)
 * @note Status: Interface for extractive summarization + agentic memory L2 rotation
 * @note **Plugin Interface**: Abstract interface for history compression implementations.
 *       No .cpp implementation needed. Implementations provided by plugin system.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>

namespace themis {
namespace aql {

/**
 * @brief Compression result containing summary and metadata.
 */
struct CompressionResult {
    std::string summary;                      // Compressed/summarized history
    std::string episode_id;                   // Unique ID for this episode
    int32_t original_token_count = 0;         // Token count before compression
    int32_t compressed_token_count = 0;       // Token count after compression
    float semantic_similarity = 0.0f;         // Similarity score (0.0-1.0)
    int64_t timestamp_ms = 0;                 // Creation timestamp
    std::vector<int32_t> selected_indices;    // Indices of extracted turns (for traceability)
};

/**
 * @brief Interface for conversation history compression strategies.
 *
 * Responsible for reducing conversation history size while preserving semantic
 * meaning. Triggered when conversation exceeds token budget or turn limit.
 *
 * **Design Pattern:**
 * - Hook-based: called by AQLConversationContext on overflow
 * - Strategy-based: different implementations (extractive, abstractive, etc.)
 * - Non-destructive: returns compressed history, doesn't modify input
 *
 * **Usage Example:**
 * @code
 * auto compressor = std::make_unique<LLMExtractiveCompressor>(handler, store);
 * std::vector<std::pair<std::string, std::string>> history = ...;
 * auto result = compressor->compressHistory(history, max_tokens, min_similarity);
 * if (result) {
 *     interaction_store->storeEpisode(result->episode_id, result->summary);
 * }
 * @endcode
 */
struct IHistoryCompressor {
    virtual ~IHistoryCompressor() = default;

    /**
     * @brief Compress conversation history while preserving semantics.
     *
     * Takes full conversation history and produces a compressed version that
     * fits within token budget while maintaining semantic similarity.
     *
     * @param history Vector of {role, content} pairs from conversation context.
     *                Typically includes "system", "user", and "assistant" roles.
     * @param max_tokens Maximum token count for compressed output.
     * @param min_similarity Minimum required semantic similarity (0.0-1.0).
     *                       Returns empty result if similarity cannot be achieved.
     *
     * @return CompressionResult if successful, empty if compression failed.
     *         Empty result indicates: failed to achieve min_similarity within
     *         max_tokens, or internal error occurred.
     *
     * @throws std::invalid_argument if max_tokens < 128 or min_similarity < 0.5
     * @throws std::runtime_error if LLM backend unavailable
     *
     * **Thread Safety:** Not thread-safe on same instance; use synchronization
     * if calling from multiple threads.
     *
     * **Implementation Notes:**
     * - System messages should be preserved at full fidelity
     * - Extractive implementations preserve turn boundaries
     * - Similarity computed using embedding distance or semantic similarity API
     */
    virtual std::unique_ptr<CompressionResult> compressHistory(
        const std::vector<std::pair<std::string, std::string>>& history,
        int32_t max_tokens,
        float min_similarity = 0.85f) = 0;

    /**
     * @brief Validate that compression can proceed.
     *
     * Returns true if the compressor is ready to compress (LLM available,
     * required resources initialized, etc.).
     *
     * @return true if compression is available, false if blocked
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief Optional: Get compression statistics.
     *
     * Some implementations track compression metrics. This method allows
     * inspection without side effects.
     *
     * @return JSON object with compression stats (may be empty for unsupported implementations)
     */
    virtual std::string getStatistics() const { return "{}"; }
};

} // namespace aql
} // namespace themis
