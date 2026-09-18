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

class LLMExtractiveCompressor final : public IHistoryCompressor {
public:
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

    std::vector<int32_t> rankTurnsByImportance(
        const std::vector<std::pair<std::string, std::string>>& history);

    std::vector<int32_t> selectTopTurns(
        const std::vector<int32_t>& ranked_indices,
        const std::vector<std::pair<std::string, std::string>>& history,
        int32_t max_tokens);

    float computeSimilarity(
        const std::vector<std::pair<std::string, std::string>>& original,
        const std::vector<std::pair<std::string, std::string>>& compressed);

    /**
     * @brief Store Episode.
     * @param[in] result Input parameter.
     */
    void storeEpisode(const CompressionResult& result);

    std::string formatTurnsForPrompt(
        const std::vector<std::pair<std::string, std::string>>& history,
        const std::vector<int32_t>& selected_indices);

    /**
     * @brief Generate UUID.
     * @return Return value.
     */
    std::string generateUUID();
};

} // namespace aql
} // namespace themis
