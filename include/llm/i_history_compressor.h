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

struct CompressionResult {
    std::string summary;                      // Compressed/summarized history
    std::string episode_id;                   // Unique ID for this episode
    int32_t original_token_count = 0;         // Token count before compression
    int32_t compressed_token_count = 0;       // Token count after compression
    float semantic_similarity = 0.0f;         // Similarity score (0.0-1.0)
    int64_t timestamp_ms = 0;                 // Creation timestamp
    std::vector<int32_t> selected_indices;    // Indices of extracted turns (for traceability)
};

struct IHistoryCompressor {
    /**
     * @brief IHistory Compressor.
     * @return Return value.
     */
    virtual ~IHistoryCompressor() = default;

    virtual std::unique_ptr<CompressionResult> compressHistory(
        const std::vector<std::pair<std::string, std::string>>& history,
        int32_t max_tokens,
        float min_similarity = 0.85f) = 0;

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    virtual bool isAvailable() const = 0;

    virtual std::string getStatistics() const { return "{}"; }
};

} // namespace aql
} // namespace themis
