/**
 * @file rag_prompt_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/rag_prompt_builder.h"
#include <algorithm>
#include <sstream>

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// Constructor / configuration
// ---------------------------------------------------------------------------

RAGPromptBuilder::RAGPromptBuilder(const RAGPromptConfig& config)
    : config_(config) {}

const RAGPromptConfig& RAGPromptBuilder::getConfig() const {
    return config_;
}

void RAGPromptBuilder::setConfig(const RAGPromptConfig& config) {
    config_ = config;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string RAGPromptBuilder::formatChunk(const RetrievedChunk& chunk,
                                          size_t index) const {
    std::ostringstream out = {};

    if (config_.include_source_citations && !chunk.source.empty()) {
        out << "[Source " << (index + 1) << ": " << chunk.source << "]\n";
    }

    out << chunk.content;
    return out.str();
}

// ---------------------------------------------------------------------------
// Chunk selection
// ---------------------------------------------------------------------------

std::vector<RetrievedChunk> RAGPromptBuilder::selectChunks(
    const std::vector<RetrievedChunk>& candidates,
    size_t max_total_length) const {

    // Optionally sort by relevance descending
    std::vector<const RetrievedChunk*> ordered = {};

    ordered.reserve(candidates.size());
    for (const auto& c : candidates) {
        ordered.push_back(&c);
    }

    if (config_.rank_by_relevance) {
        std::stable_sort(ordered.begin(), ordered.end(),
            [](const RetrievedChunk* a, const RetrievedChunk* b) {
                return a->relevance_score > b->relevance_score;
            });
    }

    std::vector<RetrievedChunk> selected;
    size_t total = 0;

    for (size_t i = 0; i < ordered.size(); ++i) {
        const auto& chunk = *ordered[i];
        std::string formatted = formatChunk(chunk, selected.size());
        size_t chunk_len = static_cast<int>(formatted.size()) + config_.chunk_separator.size();

        if (total + chunk_len > max_total_length && !selected.empty()) {
            break; // budget exhausted
        }
        total += chunk_len;
        selected.push_back(chunk);
    }

    return selected;
}

// ---------------------------------------------------------------------------
// Context section assembly
// ---------------------------------------------------------------------------

std::string RAGPromptBuilder::buildContextSection(
    const std::vector<RetrievedChunk>& chunks) const {

    if (chunks.empty()) {
        return {};
    }

    std::ostringstream out = {};

    if (!config_.context_header.empty()) {
        out << config_.context_header << "\n";
    }

    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i > 0) {
            out << config_.chunk_separator;
        }
        out << formatChunk(chunks[i], i);
    }

    if (!config_.context_footer.empty()) {
        out << "\n" << config_.context_footer;
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// Template-injection build
// ---------------------------------------------------------------------------

std::string RAGPromptBuilder::build(
    const std::string& base_template,
    const std::string& query,
    const std::vector<RetrievedChunk>& chunks) const {

    auto selected = selectChunks(chunks, config_.max_context_length);
    std::string context_block = buildContextSection(selected);

    std::string result = base_template;

    // Replace context placeholder
    auto pos = result.find(config_.template_placeholder);
    if (pos != std::string::npos) {
        result.replace(pos, config_.template_placeholder.size(), context_block);
    }

    // Replace {query} placeholder
    const std::string query_placeholder = "{query}";
    pos = result.find(query_placeholder);
    if (pos != std::string::npos) {
        result.replace(pos, query_placeholder.size(), query);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Full-prompt assembly
// ---------------------------------------------------------------------------

std::string RAGPromptBuilder::buildFullPrompt(
    const std::string& system_instruction,
    const std::string& query,
    const std::vector<RetrievedChunk>& chunks) const {

    auto selected = selectChunks(chunks, config_.max_context_length);
    std::string context_block = buildContextSection(selected);

    std::ostringstream out = {};

    if (!system_instruction.empty()) {
        out << system_instruction << "\n\n";
    }

    if (!context_block.empty()) {
        out << context_block << "\n\n";
    }

    out << "Question: " << query << "\n";
    out << "Answer:";

    return out.str();
}

} // namespace prompt_engineering
} // namespace themis
