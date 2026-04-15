/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_query_rewriter.cpp                             ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-04-15 18:10:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "search/llm_query_rewriter.h"
#include "utils/logger.h"
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

LlmQueryRewriter::LlmQueryRewriter(const Config& config, LlmBackend backend)
    : config_(config), backend_(std::move(backend)) {
    if (config_.num_rewrites == 0) {
        throw std::invalid_argument("LlmQueryRewriter: num_rewrites must be > 0");
    }
    if (config_.temperature < 0.0f || config_.temperature > 2.0f) {
        throw std::invalid_argument("LlmQueryRewriter: temperature must be in [0, 2]");
    }
    THEMIS_DEBUG("LlmQueryRewriter initialized (num_rewrites={}, has_backend={})",
                 config_.num_rewrites, backend_ != nullptr);
}

// ============================================================================
// Backend management
// ============================================================================

void LlmQueryRewriter::setBackend(LlmBackend backend) {
    backend_ = std::move(backend);
    THEMIS_DEBUG("LlmQueryRewriter: backend updated (has_backend={})", backend_ != nullptr);
}

// ============================================================================
// Core operation
// ============================================================================

RewrittenQuery LlmQueryRewriter::rewrite(const std::string& query) const {
    RewrittenQuery result;
    result.original = query;

    if (query.empty()) {
        THEMIS_DEBUG("LlmQueryRewriter::rewrite: empty query, returning early");
        return result;
    }

    if (!backend_) {
        THEMIS_DEBUG("LlmQueryRewriter::rewrite: no backend set, using fallback");
        if (config_.fallback_to_original) {
            result.rewrites.push_back(query);
        }
        return result;
    }

    try {
        const std::string prompt = buildPrompt(query);
        const std::string llm_output = backend_(prompt);
        result.llm_used = true;

        auto parsed = parseRewrites(llm_output, query);
        result.rewrites = std::move(parsed);

        THEMIS_DEBUG("LlmQueryRewriter::rewrite('{}') -> {} rewrites",
                     query, result.rewrites.size());
    } catch (const std::exception& e) {
        THEMIS_WARN("LlmQueryRewriter: backend error: {} — falling back to original query",
                    e.what());
        result.llm_used = false;
        result.rewrites.clear();
        if (config_.fallback_to_original) {
            result.rewrites.push_back(query);
        }
    } catch (...) {
        THEMIS_WARN("LlmQueryRewriter: unknown backend error — falling back to original query");
        result.llm_used = false;
        result.rewrites.clear();
        if (config_.fallback_to_original) {
            result.rewrites.push_back(query);
        }
    }

    return result;
}

// ============================================================================
// Private helpers
// ============================================================================

std::string LlmQueryRewriter::buildPrompt(const std::string& query) const {
    // Build a structured prompt that asks the LLM for exactly num_rewrites
    // alternative phrasings of the query, each on a numbered line.
    // Each rewrite should apply a distinct vocabulary strategy to maximise
    // search recall: e.g. synonyms, technical terms, abbreviations/acronyms,
    // broader phrasing, or domain-specific wording.
    // max_tokens and temperature are embedded as guidance for backends that
    // parse prompt metadata, and should be honored by the LlmBackend callable.
    std::ostringstream oss;
    oss << "Rewrite the following search query into "
        << config_.num_rewrites
        << " alternative phrasings to maximise search recall. "
           "Apply a different vocabulary strategy for each rewrite — for example: "
           "synonyms, technical or domain-specific terminology, "
           "common abbreviations or acronyms, broader or narrower phrasing, "
           "or layman's terms. "
           "Output exactly one rewrite per line, numbered starting from 1 "
           "(e.g. \"1. rewrite here\"). "
           "Do not include any other text. "
           "Keep each rewrite concise (recommended max " << config_.max_tokens << " tokens, "
           "temperature " << config_.temperature << ")."
           "\n\n"
           "Query: "
        << query
        << "\n\nRewrites:";
    return oss.str();
}

std::vector<std::string> LlmQueryRewriter::parseRewrites(
    const std::string& llm_output,
    const std::string& original) const {

    std::vector<std::string> rewrites;
    if (llm_output.empty()) {
        if (config_.fallback_to_original) {
            rewrites.push_back(original);
        }
        return rewrites;
    }

    std::istringstream iss(llm_output);
    std::string line;

    while (std::getline(iss, line)) {
        // Strip leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            line = line.substr(0, end + 1);
        }
        if (line.empty()) continue;

        // Strip leading number and period/dot, e.g. "1. " or "1) "
        size_t i = 0;
        while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
            ++i;
        }
        if (i > 0 && i < line.size() &&
            (line[i] == '.' || line[i] == ')' || line[i] == ':')) {
            ++i; // skip separator
            while (i < line.size() && line[i] == ' ') ++i; // skip space(s)
            line = line.substr(i);
        }

        if (line.empty()) continue;

        // Enforce length limit
        if (line.size() > config_.max_rewrite_length) {
            continue;
        }

        // Deduplicate (case-insensitive comparison against existing rewrites
        // and against the original query)
        auto lower = [](std::string s) -> std::string {
            for (char& c : s) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return s;
        };
        const std::string line_lower = lower(line);
        const std::string orig_lower = lower(original);
        if (line_lower == orig_lower) continue;

        bool duplicate = false;
        for (const auto& existing : rewrites) {
            if (lower(existing) == line_lower) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        rewrites.push_back(line);
        if (rewrites.size() >= config_.num_rewrites) break;
    }

    // Fallback: if the LLM produced nothing usable, return the original
    if (rewrites.empty() && config_.fallback_to_original) {
        rewrites.push_back(original);
    }

    return rewrites;
}

}  // namespace themis
