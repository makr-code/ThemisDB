/**
 * @file llm_extractive_compressor.cpp
 * @brief Extractive summarization implementation for conversation history compression.
 * @version 0.1.0-beta
 */

#include "aql/llm_extractive_compressor.h"
#include "aql/llm_token_estimator.h"
#include "llm/prompt_manager.h"
#include "utils/uuid.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <numeric>
#include <spdlog/spdlog.h>
#include <sstream>

namespace themis {
namespace aql {

namespace {

std::vector<int32_t> buildRecencyRanking(std::size_t history_size) {
    std::vector<int32_t> indices;
    indices.reserve(history_size);
    for (int32_t i = static_cast<int32_t>(history_size); i-- > 0;) {
        indices.push_back(i);
    }
    return indices;
}

std::vector<int32_t> parseRankedIndices(const std::string& response, std::size_t history_size) {
    std::vector<int32_t> indices;
    std::string current_number;
    std::vector<bool> seen(history_size, false);

    auto flush_number = [&]() {
        if (current_number.empty()) {
            return;
        }
        try {
            const auto idx = std::stoi(current_number);
            if (idx >= 0 && idx < static_cast<int32_t>(history_size) && !seen[static_cast<std::size_t>(idx)]) {
                indices.push_back(idx);
                seen[static_cast<std::size_t>(idx)] = true;
            }
        } catch (const std::exception&) {
            // Ignore malformed fragments.
        }
        current_number.clear();
    };

    for (char ch : response) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            current_number.push_back(ch);
        } else {
            flush_number();
        }
    }
    flush_number();

    if (indices.size() == history_size) {
        return indices;
    }

    for (int32_t i = static_cast<int32_t>(history_size); i-- > 0;) {
        if (!seen[static_cast<std::size_t>(i)]) {
            indices.push_back(i);
        }
    }
    return indices;
}

} // namespace

LLMExtractiveCompressor::LLMExtractiveCompressor(
    LLMAQLHandler& handler,
    LLMInteractionStore* store,
    const Config& config)
    : handler_(handler), store_(store), config_(config) {}

std::unique_ptr<CompressionResult> LLMExtractiveCompressor::compressHistory(
    const std::vector<std::pair<std::string, std::string>>& history,
    int32_t max_tokens,
    float min_similarity) {
    
    if (max_tokens < 128 || min_similarity < 0.5f) {
        throw std::invalid_argument("max_tokens >= 128 and min_similarity >= 0.5 required");
    }

    if (history.empty()) {
        return nullptr;
    }

    // Step 1: Identify system message (always preserve)
    std::vector<std::pair<std::string, std::string>> system_messages;
    std::vector<std::pair<std::string, std::string>> conversation_turns;
    
    for (const auto& msg : history) {
        if (msg.first == "system") {
            system_messages.push_back(msg);
        } else {
            conversation_turns.push_back(msg);
        }
    }

    // Step 2: Rank turns by importance
    auto ranked_indices = rankTurnsByImportance(conversation_turns);
    if (ranked_indices.empty()) {
        ++failed_compressions_;
        return nullptr;
    }

    // Step 3: Select top K turns within budget
    auto selected_indices = selectTopTurns(ranked_indices, conversation_turns, max_tokens);
    if (selected_indices.empty()) {
        ++failed_compressions_;
        return nullptr;
    }

    // Step 4: Build compressed history
    std::vector<std::pair<std::string, std::string>> compressed_history = system_messages;
    for (int32_t idx : selected_indices) {
        if (idx >= 0 && idx < static_cast<int32_t>(conversation_turns.size())) {
            compressed_history.push_back(conversation_turns[idx]);
        }
    }

    // Step 5: Estimate tokens
    CharDivisionEstimator estimator;
    int32_t original_tokens = 0;
    for (const auto& msg : history) {
        original_tokens += estimator.estimate(msg.second);
    }

    int32_t compressed_tokens = 0;
    for (const auto& msg : compressed_history) {
        compressed_tokens += estimator.estimate(msg.second);
    }

    // Step 6: Validate similarity
    float similarity = 1.0f;
    if (config_.validate_similarity) {
        similarity = computeSimilarity(history, compressed_history);
        if (similarity < 0.0f) {
            // Validation disabled/failed
            similarity = -1.0f;
        } else if (similarity < min_similarity) {
            ++failed_compressions_;
            return nullptr;
        }
    }

    // Step 7: Create result
    auto result = std::make_unique<CompressionResult>();
    result->episode_id = generateUUID();
    result->summary = formatTurnsForPrompt(history, selected_indices);
    result->original_token_count = original_tokens;
    result->compressed_token_count = compressed_tokens;
    result->semantic_similarity = similarity;
    result->timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    result->selected_indices = selected_indices;

    // Step 8: Store episode if store available
    if (store_) {
        storeEpisode(*result);
    }

    ++successful_compressions_;
    ++total_compressions_;
    if (compressed_tokens > 0) {
        total_compression_ratio_ += static_cast<double>(original_tokens) / compressed_tokens;
    }

    return result;
}

bool LLMExtractiveCompressor::isAvailable() const {
    auto llm_client = handler_.getLLMClient();
    return llm_client != nullptr && llm_client->isReady();
}

std::string LLMExtractiveCompressor::getStatistics() const {
    nlohmann::json stats;
    stats["total_compressions"] = total_compressions_;
    stats["successful_compressions"] = successful_compressions_;
    stats["failed_compressions"] = failed_compressions_;
    if (successful_compressions_ > 0) {
        stats["avg_compression_ratio"] = 
            total_compression_ratio_ / successful_compressions_;
    }
    return stats.dump();
}

std::vector<int32_t> LLMExtractiveCompressor::rankTurnsByImportance(
    const std::vector<std::pair<std::string, std::string>>& history) {
    
    if (history.empty()) {
        return {};
    }

    // Format turns for LLM prompt
    std::ostringstream turn_format;
    for (size_t i = 0; i < history.size(); ++i) {
        turn_format << "[" << i << "] " << history[i].first << ": " 
                   << history[i].second.substr(0, 100) << "...\n";
    }

    // Create ranking prompt
    std::string ranking_prompt = config_.importance_ranking_prompt_template;
    size_t pos = ranking_prompt.find("{turns}");
    if (pos != std::string::npos) {
        ranking_prompt.replace(pos, 7, turn_format.str());
    }

    try {
        if (!isAvailable()) {
            return buildRecencyRanking(history.size());
        }

        std::unordered_map<std::string, std::string> options;
        options["max_tokens"] = "256";
        options["temperature"] = "0.0";
        const std::string ranking_response = handler_.executeInfer(ranking_prompt, "", "", options);
        if (ranking_response.empty()) {
            return buildRecencyRanking(history.size());
        }
        return parseRankedIndices(ranking_response, history.size());
    } catch (const std::exception& ex) {
        spdlog::warn("LLMExtractiveCompressor: ranking via LLM failed; using recency fallback: {}", ex.what());
        return buildRecencyRanking(history.size());
    }
}

std::vector<int32_t> LLMExtractiveCompressor::selectTopTurns(
    const std::vector<int32_t>& ranked_indices,
    const std::vector<std::pair<std::string, std::string>>& history,
    int32_t max_tokens) {
    
    CharDivisionEstimator estimator;
    std::vector<int32_t> selected;
    int32_t accumulated_tokens = 0;

    for (int32_t idx : ranked_indices) {
        if (idx < 0 || idx >= static_cast<int32_t>(history.size())) {
            continue;
        }

        int32_t turn_tokens = estimator.estimate(history[idx].second);
        if (accumulated_tokens + turn_tokens <= max_tokens) {
            selected.push_back(idx);
            accumulated_tokens += turn_tokens;
        } else if (selected.size() < static_cast<size_t>(config_.top_k_turns)) {
            // Keep adding even if over budget, up to top_k
            selected.push_back(idx);
            accumulated_tokens += turn_tokens;
        }
    }

    // Ensure minimum turns preserved
    if (selected.size() < static_cast<size_t>(config_.min_preserved_turns) && 
        selected.size() < history.size()) {
        // Add more turns to reach minimum
        for (int32_t idx : ranked_indices) {
            if (std::find(selected.begin(), selected.end(), idx) == selected.end()) {
                selected.push_back(idx);
                if (selected.size() >= static_cast<size_t>(config_.min_preserved_turns)) {
                    break;
                }
            }
        }
    }

    // Sort by original position for readability
    std::sort(selected.begin(), selected.end());
    return selected;
}

float LLMExtractiveCompressor::computeSimilarity(
    const std::vector<std::pair<std::string, std::string>>& original,
    const std::vector<std::pair<std::string, std::string>>& compressed) {
    
    if (!config_.validate_similarity) {
        return -1.0f;
    }

    // TODO: Implement actual embedding-based similarity computation
    // For now, use heuristic: preserve ratio of turn count
    if (original.empty()) {
        return 1.0f;
    }

    float turn_ratio = static_cast<float>(compressed.size()) / original.size();
    // Map turn ratio to similarity (conservative estimate)
    float base_similarity = 0.7f + (turn_ratio * 0.3f);
    return std::min(base_similarity, 1.0f);
}

void LLMExtractiveCompressor::storeEpisode(const CompressionResult& result) {
    if (!store_) {
        return;
    }

    try {
        LLMInteractionStore::Interaction episode;
        episode.id = result.episode_id;
        episode.prompt = "Episodic History Compression";
        episode.response = result.summary;
        episode.timestamp_ms = result.timestamp_ms;
        episode.metadata["semantic_similarity"] = result.semantic_similarity;
        episode.metadata["original_tokens"] = result.original_token_count;
        episode.metadata["compressed_tokens"] = result.compressed_token_count;
        episode.metadata["type"] = "episodic_summary";

        store_->createInteraction(episode);
    } catch (const std::exception& ex) {
        // Log and continue - failure to store shouldn't break compression.
        spdlog::warn("LLMExtractiveCompressor: failed to persist episode {}: {}",
                     result.episode_id, ex.what());
    } catch (...) {
        spdlog::warn("LLMExtractiveCompressor: failed to persist episode {} due to unknown exception",
                     result.episode_id);
    }
}

std::string LLMExtractiveCompressor::formatTurnsForPrompt(
    const std::vector<std::pair<std::string, std::string>>& history,
    const std::vector<int32_t>& selected_indices) {
    
    std::ostringstream result;
    result << "=== Episodic Memory Summary ===\n\n";

    for (int32_t idx : selected_indices) {
        if (idx >= 0 && idx < static_cast<int32_t>(history.size())) {
            const auto& msg = history[idx];
            result << "[" << msg.first << "]\n" << msg.second << "\n\n";
        }
    }

    return result.str();
}

std::string LLMExtractiveCompressor::generateUUID() {
    return utils::generate_uuid_v4();
}

} // namespace aql
} // namespace themis
