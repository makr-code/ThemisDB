/**
 * @file llm_query_rewriter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/llm_query_rewriter.h"
#include "utils/logger.h"
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
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

        // Semantic output validator (Gap 2 — search/FUTURE_ENHANCEMENTS.md §Gap 2):
        // discard rewrites whose Jaccard token-overlap with the original query
        // falls below Config::min_token_overlap_ratio.
        if (config_.min_token_overlap_ratio > 0.0f && !parsed.empty()) {
            const bool any_survived = applyOverlapFilter(parsed, query);
            if (!any_survived) {
                THEMIS_WARN("LlmQueryRewriter: all {} rewrite(s) failed overlap "
                            "threshold ({:.2f}) — falling back to original query",
                            result.rewrites.size(),
                            static_cast<double>(config_.min_token_overlap_ratio));
                result.quality = RewriteQuality::FALLBACK;
                parsed.clear();
                if (config_.fallback_to_original) {
                    parsed.push_back(query);
                }
            }
        }

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

    std::vector<std::string> rewrites = {};

    if (llm_output.empty()) {
        if (config_.fallback_to_original) {
            rewrites.push_back(original);
        }
        return rewrites;
    }

    std::istringstream iss(llm_output);
    std::string line = {};

    while (std::getline(iss, line)) {
        // Strip leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
          continue;
        }
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            line = line.substr(0, end + 1);
        }
        if (line.empty()) {
          continue;
        }

        // Strip leading number and period/dot, e.g. "1. " or "1) "
        size_t i = 0;
        while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
            ++i;
        }
        if (i > 0  && static_cast<size_t>(i) < line.size() &&
            (line[i] == '.' || line[i] == ')' || line[i] == ':')) {
            ++i; // skip separator
            while (i < line.size() && line[i] == ' ') ++i; // skip space(s)
            line = line.substr(i);
        }

        if (line.empty()) {
          continue;
        }

        // Enforce length limit
        if (static_cast<int>(line.size()) > config_.max_rewrite_length) {
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
        if (line_lower == orig_lower) {
          continue;
        }

        bool duplicate = false;
        for (const auto& existing : rewrites) {
            if (lower(existing) == line_lower) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
          continue;
        }

        rewrites.push_back(line);
        if (static_cast<int>(rewrites.size()) > = config_.num_rewrites) {
          break;
        }
    }

    // Fallback: if the LLM produced nothing usable, return the original
    if (rewrites.empty() && config_.fallback_to_original) {
        rewrites.push_back(original);
    }

    return rewrites;
}

// ============================================================================
// Semantic output validator helpers (Gap 2)
// ============================================================================

float LlmQueryRewriter::jaccardTokenOverlap(const std::string& a,
                                             const std::string& b)
{
    // Tokenise by splitting on whitespace; normalise to lower-case.
    auto tokenise = [](const std::string& s) -> std::unordered_set<std::string> {
        std::unordered_set<std::string> tokens;
        std::istringstream iss(s);
        std::string tok = {};
        while (iss >> tok) {
            std::string lc = {};
            lc.reserve(tok.size());
            for (char c : tok) {
                lc += static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
            tokens.insert(std::move(lc));
        }
        return tokens;
    };

    const auto ta = tokenise(a);
    const auto tb = tokenise(b);

    if (ta.empty() && tb.empty()) {
      return 1.0f;
    }
    if (ta.empty() || tb.empty()) {
      return 0.0f;
    }

    size_t intersection = 0;
    for (const auto& tok : ta) {
        if (tb.count(tok)) {
          ++intersection;
        }
    }
    // |A ∪ B| = |A| + |B| - |A ∩ B|
    const size_t union_size = static_cast<int>(ta.size()) + static_cast<int>(tb.size()) - intersection;
    return static_cast<float>(intersection) / static_cast<float>(union_size);
}

bool LlmQueryRewriter::applyOverlapFilter(std::vector<std::string>& rewrites,
                                           const std::string& original) const
{
    std::vector<std::string> kept = {};

    kept.reserve(rewrites.size());
    for (auto& r : rewrites) {
        const float overlap = jaccardTokenOverlap(r, original);
        if (overlap >= config_.min_token_overlap_ratio) {
            kept.push_back(std::move(r));
        } else {
            THEMIS_DEBUG("LlmQueryRewriter: discarding rewrite (overlap={:.3f} < {:.3f}): '{}'",
                         static_cast<double>(overlap),
                         static_cast<double>(config_.min_token_overlap_ratio),
                         r);
        }
    }
    rewrites = std::move(kept);
    return !rewrites.empty();
}

}  // namespace themis


