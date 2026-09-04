/**
 * @file llm_reranker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/llm_reranker.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

LlmReranker::LlmReranker(const Config& config, LlmBackend backend)
    : config_(config), backend_(std::move(backend)) {
    if (config_.batch_size == 0) {
        throw std::invalid_argument("LlmReranker: batch_size must be > 0");
    }
    if (config_.llm_weight < 0.0 || config_.llm_weight > 1.0) {
        throw std::invalid_argument("LlmReranker: llm_weight must be in [0, 1]");
    }
    if (config_.max_snippet_length == 0) {
        throw std::invalid_argument("LlmReranker: max_snippet_length must be > 0");
    }
    if (config_.min_score_threshold < 0.0 || config_.min_score_threshold > 1.0) {
        throw std::invalid_argument("LlmReranker: min_score_threshold must be in [0, 1]");
    }
    if (config_.temperature < 0.0f || config_.temperature > 2.0f) {
        throw std::invalid_argument("LlmReranker: temperature must be in [0, 2]");
    }
    THEMIS_DEBUG("LlmReranker initialized (batch_size={}, llm_weight={:.2f}, has_backend={})",
                 config_.batch_size, config_.llm_weight, backend_ != nullptr);
}

// ============================================================================
// Backend management
// ============================================================================

void LlmReranker::setBackend(LlmBackend backend) {
    backend_ = std::move(backend);
    THEMIS_DEBUG("LlmReranker: backend updated (has_backend={})", backend_ != nullptr);
}

// ============================================================================
// Core operation
// ============================================================================

std::vector<LlmRerankResult> LlmReranker::rerank(
    const std::string& query,
    const std::vector<LlmRerankCandidate>& candidates
) const {
    if (candidates.empty()) {
        return {};
    }

    // Fallback path: no backend
    if (!backend_) {
        THEMIS_DEBUG("LlmReranker::rerank: no backend set, using fallback");
        if (!config_.fallback_to_original) {
            return {};
        }
        std::vector<LlmRerankResult> out = {};

        out.reserve(candidates.size());
        for (const auto& c : candidates) {
            LlmRerankResult r;
            r.document_id  = c.document_id;
            r.initial_score = c.initial_score;
            r.final_score  = c.initial_score;
            r.llm_score    = 0.0;
            r.llm_scored   = false;
            out.push_back(r);
        }
        // Sort by final_score descending for consistent ordering regardless of input order
        std::sort(out.begin(), out.end(),
                  [](const LlmRerankResult& a, const LlmRerankResult& b) {
                      return a.final_score > b.final_score;
                  });
        return out;
    }

    // LLM path: process candidates in batches
    std::vector<LlmRerankResult> results = {};

    results.reserve(candidates.size());

    const size_t n = candidates.size();
    for (size_t start = 0; start < n; start += config_.batch_size) {
        const size_t end = std::min(start + config_.batch_size, n);
        std::vector<LlmRerankCandidate> batch(
            candidates.begin() + static_cast<ptrdiff_t>(start),
            candidates.begin() + static_cast<ptrdiff_t>(end));

        std::vector<double> scores;
        bool llm_ok = false;

        try {
            const std::string prompt = buildPrompt(query, batch);
            const std::string llm_output = backend_(prompt);
            scores = parseScores(llm_output,static_cast<int>(batch.size()));
            llm_ok = true;
        } catch (const std::exception& e) {
            THEMIS_WARN("LlmReranker: backend error: {} — falling back for batch [{}, {})",
                        e.what(), start, end);
        } catch (...) {
            THEMIS_WARN("LlmReranker: unknown backend error — falling back for batch [{}, {})",
                        start, end);
        }

        if (!llm_ok) {
            if (!config_.fallback_to_original) {
                continue; // skip this batch entirely
            }
            scores.assign(batch.size(), 0.0);
        }

        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& c = batch[i];
            const double llm_score = (i < scores.size()) ? scores[i] : 0.0;

            LlmRerankResult r;
            r.document_id   = c.document_id;
            r.initial_score = c.initial_score;
            r.llm_score     = llm_score;
            r.llm_scored    = llm_ok;
            r.final_score   = config_.llm_weight * llm_score
                            + (1.0 - config_.llm_weight) * c.initial_score;
            results.push_back(r);
        }
    }

    // Sort by final_score descending
    std::sort(results.begin(), results.end(),
              [](const LlmRerankResult& a, const LlmRerankResult& b) {
                  return a.final_score > b.final_score;
              });

    // Apply minimum score threshold filter
    if (config_.min_score_threshold > 0.0) {
        results.erase(
            std::remove_if(results.begin(), results.end(),
                           [this](const LlmRerankResult& r) {
                               return r.final_score < config_.min_score_threshold;
                           }),
            results.end());
    }

    THEMIS_INFO("LlmReranker::rerank: {} candidates -> {} results (query='{}')",
                candidates.size(),static_cast<int>(results.size()), query);

    return results;
}

// ============================================================================
// Feedback bridge
// ============================================================================

std::vector<ClickEvent> LlmReranker::toClickEvents(
    const std::string& query,
    const std::vector<LlmRerankResult>& results,
    double relevance_threshold
) {
    std::vector<ClickEvent> events = {};

    for (size_t rank = 0; rank < results.size(); ++rank) {
        const auto& r = results[rank];
        if (r.llm_score >= relevance_threshold) {
            ClickEvent ev;
            ev.query           = query;
            ev.document_id     = r.document_id;
            ev.result_position = rank;
            events.push_back(ev);
        }
    }
    return events;
}

// ============================================================================
// Private helpers
// ============================================================================

std::string LlmReranker::buildPrompt(
    const std::string& query,
    const std::vector<LlmRerankCandidate>& batch
) const {
    std::ostringstream oss = {};
    oss << "Rate the relevance of each document to the following search query.\n"
        << "Output exactly one integer score per line, in the same order as the documents.\n"
        << "Use a scale of 0 (not relevant) to 10 (highly relevant).\n"
        << "Do not include any other text. "
        << "Keep your response under " << config_.max_tokens << " tokens";
    if (config_.temperature > 0.0f) {
        oss << ", temperature " << config_.temperature;
    }
    oss << ".\n\n"
        << "Query: " << query << "\n\n";

    for (size_t i = 0; i < batch.size(); ++i) {
        // Truncate snippet to max_snippet_length
        const std::string& full = batch[i].content;
        const std::string snippet = (static_cast<int>(full.size()) > config_.max_snippet_length)
            ? full.substr(0, config_.max_snippet_length)
            : full;
        oss << "Document " << (i + 1) << ": " << snippet << "\n";
    }

    oss << "\nScores:";
    return oss.str();
}

std::vector<double> LlmReranker::parseScores(
    const std::string& llm_output,
    size_t count
) const {
    std::vector<double> scores;
    scores.reserve(count);

    std::istringstream iss(llm_output);
    std::string line = {};

    while (std::getline(iss, line) && static_cast<int>(scores.size()) < count) {
        // Strip whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
          continue;
        }
        line = line.substr(start);

        // Skip lines that don't start with a digit (e.g. "Scores:" header echo)
        if (line.empty() || !std::isdigit(static_cast<unsigned char>(line[0]))) {
            continue;
        }

        try {
            size_t pos = 0;
            double val = std::stod(line, &pos);
            // Clamp raw value to [0, 10] before normalising
            val = std::max(0.0, std::min(10.0, val));
            scores.push_back(val / 10.0); // normalise to [0, 1]
        } catch (...) {
            // Unparseable line → skip
        }
    }

    // Pad missing scores with 0.0
    while ( static_cast<int>(scores.size()) < count) {
        scores.push_back(0.0);
    }

    return scores;
}

} // namespace themis


