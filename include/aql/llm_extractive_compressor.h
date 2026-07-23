/**
 * @file llm_extractive_compressor.h
 * @brief Extractive summarization compressor for conversation history (L2 episodic memory).
 * @version 0.1.0-beta
 * @note Maturity: BETA (Phase 2 P2-D03)
 */

#pragma once

#include "llm/i_history_compressor.h"
#include "aql/llm_aql_handler.h"
#include "llm/llm_interaction_store.h"
#include "llm/prompt_manager.h"

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace themis {
namespace aql {

/**
 * @brief Extractive summarization compressor using LLM-based turn selection.
 *
 * Implements IHistoryCompressor by:
 * 1. Using LLM to rank conversation turns by importance
 * 2. Selecting top-K most important turns
 * 3. Concatenating selected turns while respecting token budget
 * 4. Validating semantic similarity via embedding distance
 * 5. Storing episode in LLMInteractionStore
 *
 * **Performance Characteristics:**
 * - Time: O(turn_count) LLM calls + O(selected_count) token counting
 * - VRAM: Embeddings for original + compressed history
 * - Typical compression ratio: 3-5x for 10-50 turn conversations
 *
 * **Activation:**
 * - Triggered by AQLConversationContext on token budget overflow
 * - Configurable via AQLConversationContext::Config::enable_episodic_compaction
 * - Default minimum similarity: 0.85 (P2-GATE-03)
 */
class LLMExtractiveCompressor final : public IHistoryCompressor {
public:
    /**
     * @brief Configuration for extractive compressor.
     */
    struct Config {
        // Prompt template for turn importance ranking
        std::string importance_ranking_prompt_template =
            "Rank these conversation turns by importance for context preservation (1=most, N=least):\n{turns}\nRankings:";
        
        // Number of top turns to extract
        int32_t top_k_turns = 5;
        
        // Minimum turns to preserve (system + at least N user/assistant pairs)
        int32_t min_preserved_turns = 3;
        
        // Enable embedding-based similarity validation
        bool validate_similarity = true;
        
        // Embedding model to use for similarity (if empty, skip embedding validation)
        std::string embedding_model = "default";
        
        // Timeout for LLM importance ranking call (milliseconds)
        int32_t ranking_timeout_ms = 5000;
    };

    /**
     * @brief Construct an extractive compressor.
     *
     * @param handler LLM handler for generating importance rankings and similarity embeddings
     * @param store Optional LLMInteractionStore for persisting episodes
     * @param config Runtime configuration (uses defaults if not provided)
     */
    explicit LLMExtractiveCompressor(
        LLMAQLHandler& handler,
        LLMInteractionStore* store = nullptr,
        const Config& config = Config());

    // IHistoryCompressor implementation
    std::unique_ptr<CompressionResult> compressHistory(
        const std::vector<std::pair<std::string, std::string>>& history,
        int32_t max_tokens,
        float min_similarity = 0.85f) override;

    bool isAvailable() const override;
    std::string getStatistics() const override;

private:
    LLMAQLHandler& handler_;
    LLMInteractionStore* store_;  // Optional; may be nullptr
    Config config_;
    
    // Statistics for getStatistics()
    int64_t total_compressions_ = 0;
    int64_t successful_compressions_ = 0;
    int64_t failed_compressions_ = 0;
    double total_compression_ratio_ = 0.0;

    /**
     * @brief Rank turns by importance using LLM.
     *
     * Calls LLM with importance_ranking_prompt to get relative ranking of turns.
     *
     * @param history Original conversation history
     * @return Vector of turn indices ranked by importance (descending)
     *         Empty vector if ranking failed
     */
    std::vector<int32_t> rankTurnsByImportance(
        const std::vector<std::pair<std::string, std::string>>& history);

    /**
     * @brief Select top K turns while respecting token budget.
     *
     * @param ranked_indices Turn indices in importance order
     * @param history Original conversation history
     * @param max_tokens Maximum token budget
     * @return Indices of selected turns, always including system message
     */
    std::vector<int32_t> selectTopTurns(
        const std::vector<int32_t>& ranked_indices,
        const std::vector<std::pair<std::string, std::string>>& history,
        int32_t max_tokens);

    /**
     * @brief Compute semantic similarity between original and compressed history.
     *
     * Uses embedding distance or LLM-based similarity scoring.
     *
     * @param original Original history
     * @param compressed Compressed history
     * @return Similarity score (0.0-1.0), or -1.0 if validation disabled/failed
     */
    float computeSimilarity(
        const std::vector<std::pair<std::string, std::string>>& original,
        const std::vector<std::pair<std::string, std::string>>& compressed);

    /**
     * @brief Store episode in LLMInteractionStore if available.
     */
    void storeEpisode(const CompressionResult& result);

    /**
     * @brief Format turns for LLM prompt.
     */
    std::string formatTurnsForPrompt(
        const std::vector<std::pair<std::string, std::string>>& history,
        const std::vector<int32_t>& selected_indices);

    /**
     * @brief Generate a UUID v4 for episode identification.
     */
    std::string generateUUID();
};

} // namespace aql
} // namespace themis
