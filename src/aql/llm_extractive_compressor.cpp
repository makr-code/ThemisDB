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
    : handler_([[maybe_unused]] handler), store_(store), config_(config) {}

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

/// @brief Build a term-frequency bag-of-words from a flat message collection.
/// @details Tokenises each message body into lowercase words and accumulates
///          counts.  Punctuation is stripped and empty tokens are discarded.
/// @param msgs Collection of (role, content) pairs to aggregate.
/// @return Mapping from word to occurrence count.
static std::unordered_map<std::string, float> buildTermFrequency(
    const std::vector<std::pair<std::string, std::string>>& msgs) {
    std::unordered_map<std::string, float> tf = {};

    for (const auto& [role, content] : msgs) {
        (void)role; // role is not used for term frequency
        std::string token;
        for (unsigned char ch : content) {
            if (std::isalnum(ch)) {
                token.push_back(static_cast<char>(std::tolower(ch)));
            } else {
                if (!token.empty()) {
                    tf[token] += 1.0f;
                    token.clear();
                }
            }
        }
        if (!token.empty()) {
            tf[token] += 1.0f;
        }
    }
    return tf;
}

/// @brief Compute the L2 norm of a term-frequency vector.
static float l2Norm(const std::unordered_map<std::string, float>& tf) {
    float sum = 0.0f;
    for (const auto& [_, v] : tf) {
        sum += v * v;
    }
    return std::sqrt(sum);
}

float LLMExtractiveCompressor::computeSimilarity(
    const std::vector<std::pair<std::string, std::string>>& original,
    const std::vector<std::pair<std::string, std::string>>& compressed) {
    
    if (!config_.validate_similarity) {
        return -1.0f;
    }

    if (original.empty()) {
        return 1.0f;
    }

    // Cosine similarity over bag-of-words term frequencies.
    // This is a deterministic, dependency-free approximation of semantic
    // overlap that replaces the previous turn-count heuristic.  When a real
    // embedding service is wired in this can be upgraded without changing the
    // call sites.
    const auto orig_tf = buildTermFrequency(original);
    const auto comp_tf = buildTermFrequency(compressed);

    if (orig_tf.empty() || comp_tf.empty()) {
        // No text content — treat as fully similar to avoid spurious rejection.
        return 1.0f;
    }

    // Dot product (iterate over the smaller vector for efficiency)
    const auto& iter_tf = (orig_tf.size() <= comp_tf.size()) ? orig_tf : comp_tf;
    const auto& lookup_tf = (orig_tf.size() <= comp_tf.size()) ? comp_tf : orig_tf;
    float dot = 0.0f;
    for (const auto& [term, value] : iter_tf) {
        auto it = lookup_tf.find(term);
        if (it != lookup_tf.end()) {
            dot += value * it->second;
        }
    }

    const float norm_orig = l2Norm(orig_tf);
    const float norm_comp = l2Norm(comp_tf);

    if (norm_orig <= 0.0f || norm_comp <= 0.0f) {
        return 0.0f;
    }

    const float cosine = dot / (norm_orig * norm_comp);
    return std::min(std::max(cosine, 0.0f), 1.0f);
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
