/**
 * @file adaptive_retrieval.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/adaptive_retrieval.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace themis::rag {

namespace {

AdaptiveRetrievalConfig sanitizeConfig(const AdaptiveRetrievalConfig& cfg)
{
    AdaptiveRetrievalConfig out = cfg;

    if (out.base_top_k == 0u) {
        out.base_top_k = 1u;
    }
    if (out.max_top_k < out.base_top_k) {
        out.max_top_k = out.base_top_k;
    }

    if (!std::isfinite(out.complexity_scaling) || out.complexity_scaling < 1.0) {
        out.complexity_scaling = 1.0;
    }

    if (!std::isfinite(out.base_similarity_threshold)) {
        out.base_similarity_threshold = 0.75;
    }
    if (!std::isfinite(out.min_similarity_threshold)) {
        out.min_similarity_threshold = 0.40;
    }

    out.base_similarity_threshold =
        std::clamp(out.base_similarity_threshold, 0.0, 1.0);
    out.min_similarity_threshold =
        std::clamp(out.min_similarity_threshold, 0.0, 1.0);

    if (out.min_similarity_threshold > out.base_similarity_threshold) {
        out.min_similarity_threshold = out.base_similarity_threshold;
    }

    return out;
}

/** Lowercase a string (ASCII only). */
std::string toLower(const std::string& s)
{
    std::string r = s;
    for (auto& c : r) {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
    }
    return r;
}

/** Split a string into whitespace-delimited tokens. */
std::vector<std::string> tokenize(const std::string& s)
{
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string tok;
    while (ss >> tok) {
      tokens.push_back(tok);
    }
    return tokens;
}

/** Strip punctuation from a token for comparison. */
std::string stripPunct(const std::string& s)
{
    std::string r;
    for (char c : s) {
        if (std::isalpha(static_cast<unsigned char>(c)) ||
            std::isdigit(static_cast<unsigned char>(c))) {
            r += c;
        }
    }
    return r;
}

// Clause connective words that indicate a more complex query
const char* kConnectives[] = {
    "and", "or", "but", "because", "while", "since", "although",
    "however", "therefore", "furthermore", "moreover", "whereas",
    "unless", "whether", nullptr
};

// Question words (beyond the first token)
const char* kQuestionWords[] = {
    "who", "what", "when", "where", "why", "how", "which", nullptr
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AdaptiveRetrieval::AdaptiveRetrieval(const AdaptiveRetrievalConfig& config)
    : config_(sanitizeConfig(config))
{}

const AdaptiveRetrievalConfig& AdaptiveRetrieval::getConfig() const
{
    return config_;
}

void AdaptiveRetrieval::setConfig(const AdaptiveRetrievalConfig& config)
{
    config_ = sanitizeConfig(config);
}

void AdaptiveRetrieval::setScorer(IComplexityScorer* scorer)
{
    scorer_ = scorer;
}

// ---------------------------------------------------------------------------
// heuristicAnalyze
// ---------------------------------------------------------------------------

ComplexityAnalysis AdaptiveRetrieval::heuristicAnalyze(
    const std::string& query) const
{
    ComplexityAnalysis analysis;

    if (query.empty()) {
        analysis.explanation = "empty query";
        return analysis;
    }

    const std::string lower = toLower(query);
    const std::vector<std::string> tokens = tokenize(lower);

    // 1. Count connectives
    for (const auto& tok : tokens) {
        const std::string t = stripPunct(tok);
        for (size_t i = 0; kConnectives[i]; ++i) {
            if (t == kConnectives[i]) {
                ++analysis.connective_count;
                break;
            }
        }
    }

    // 2. Count question words (skip the first token — that's the question word)
    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string t = stripPunct(tokens[i]);
        for (size_t j = 0; kQuestionWords[j]; ++j) {
            if (t == kQuestionWords[j]) {
                ++analysis.question_word_count;
                break;
            }
        }
    }

    // 3. Long query flag
    analysis.is_long_query =
        (config_.long_query_threshold > 0 &&
         query.size() > config_.long_query_threshold);

    // 4. Assemble raw score
    // Connectives contribute the most: each connective ~ 0.15
    // Question words mid-query: each ~ 0.10
    // Long query: +0.15
    double score = 0.0;
    score += static_cast<double>(analysis.connective_count) * 0.15;
    score += static_cast<double>(analysis.question_word_count) * 0.10;
    if (analysis.is_long_query) {
      score += 0.15;
    }

    // Clamp to [0, 1]
    analysis.raw_score = std::min(1.0, score);

    analysis.complexity = scoreToComplexity(analysis.raw_score);

    // Build explanation
    std::ostringstream expl;
    expl << "connectives=" << analysis.connective_count
         << " question_words=" << analysis.question_word_count
         << " long=" << (analysis.is_long_query ? "yes" : "no")
         << " score=" << analysis.raw_score
         << " tier=" << complexityToString(analysis.complexity);
    analysis.explanation = expl.str();

    return analysis;
}

// ---------------------------------------------------------------------------
// analyzeComplexity
// ---------------------------------------------------------------------------

ComplexityAnalysis AdaptiveRetrieval::analyzeComplexity(
    const std::string& query) const
{
    if (scorer_) {
        const double llm_score = scorer_->score(query);
        if (llm_score > 0.0) {
            ComplexityAnalysis a;
            a.raw_score   = std::min(1.0, llm_score);
            a.complexity  = scoreToComplexity(a.raw_score);
            a.explanation = "llm_scorer score=" + std::to_string(a.raw_score) +
                            " tier=" + complexityToString(a.complexity);
            return a;
        }
    }
    return heuristicAnalyze(query);
}

// ---------------------------------------------------------------------------
// scoreToComplexity
// ---------------------------------------------------------------------------

QueryComplexity AdaptiveRetrieval::scoreToComplexity([[maybe_unused]] double raw_score)
{
    if (raw_score < 0.30) {
      return QueryComplexity::SIMPLE;
    }
    if (raw_score < 0.55) {
      return QueryComplexity::MODERATE;
    }
    if (raw_score < 0.75) {
      return QueryComplexity::COMPLEX;
    }
    return QueryComplexity::VERY_COMPLEX;
}

const char* AdaptiveRetrieval::complexityToString(QueryComplexity complexity)
{
    switch (complexity) {
        case QueryComplexity::SIMPLE:       return "SIMPLE";
        case QueryComplexity::MODERATE:     return "MODERATE";
        case QueryComplexity::COMPLEX:      return "COMPLEX";
        case QueryComplexity::VERY_COMPLEX: return "VERY_COMPLEX";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// complexityToTopK / complexityToThreshold
// ---------------------------------------------------------------------------

size_t AdaptiveRetrieval::complexityToTopK(QueryComplexity complexity) const
{
    const auto cfg = sanitizeConfig(config_);
    const long double base = static_cast<long double>(cfg.base_top_k);
    const long double scale = static_cast<long double>(cfg.complexity_scaling);

    int exponent = 0;
    switch (complexity) {
        case QueryComplexity::SIMPLE:       exponent = 0; break;
        case QueryComplexity::MODERATE:     exponent = 1; break;
        case QueryComplexity::COMPLEX:      exponent = 2; break;
        case QueryComplexity::VERY_COMPLEX: exponent = 3; break;
    }

    long double raw = base;
    for (int i = 0; i < exponent; ++i) {
        raw *= scale;
    }

    if (!std::isfinite(static_cast<double>(raw)) ||
        raw > static_cast<long double>(std::numeric_limits<size_t>::max())) {
        return cfg.max_top_k;
    }

    const size_t k = static_cast<size_t>(std::llround(raw));
    return std::clamp(k, cfg.base_top_k, cfg.max_top_k);
}

double AdaptiveRetrieval::complexityToThreshold(
    QueryComplexity complexity) const
{
    const auto cfg = sanitizeConfig(config_);

    // Linear interpolation between base_similarity_threshold (SIMPLE)
    // and min_similarity_threshold (VERY_COMPLEX) over 4 tiers (0-3).
    const int tier = static_cast<int>(complexity);  // 0..3
    const double base  = cfg.base_similarity_threshold;
    const double floor = cfg.min_similarity_threshold;
    const double range = base - floor;
    // tier 0 → base, tier 3 → floor
    return std::max(floor, base - (static_cast<double>(tier) / 3.0) * range);
}

// ---------------------------------------------------------------------------
// computeParams
// ---------------------------------------------------------------------------

AdaptiveRetrievalParams AdaptiveRetrieval::computeParams(
    const std::string& query) const
{
    AdaptiveRetrievalParams params;
    params.analysis             = analyzeComplexity(query);
    params.top_k                = complexityToTopK(params.analysis.complexity);
    params.similarity_threshold = complexityToThreshold(params.analysis.complexity);

    spdlog::info(
        "AdaptiveRetrieval::computeParams query_chars={} complexity={} raw_score={:.3f} top_k={} similarity_threshold={:.3f}",
        query.size(),
        complexityToString(params.analysis.complexity),
        params.analysis.raw_score,
        params.top_k,
        params.similarity_threshold);

    return params;
}

// ---------------------------------------------------------------------------
// AdaptiveRetrievalFactory
// ---------------------------------------------------------------------------

std::unique_ptr<AdaptiveRetrieval> AdaptiveRetrievalFactory::createLightweight()
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k               = 3;
    cfg.max_top_k                = 8;
    cfg.base_similarity_threshold = 0.80;
    cfg.min_similarity_threshold  = 0.55;
    cfg.complexity_scaling        = 1.4;
    return std::make_unique<AdaptiveRetrieval>(cfg);
}

std::unique_ptr<AdaptiveRetrieval> AdaptiveRetrievalFactory::createBalanced()
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k               = 5;
    cfg.max_top_k                = 15;
    cfg.base_similarity_threshold = 0.75;
    cfg.min_similarity_threshold  = 0.40;
    cfg.complexity_scaling        = 1.5;
    return std::make_unique<AdaptiveRetrieval>(cfg);
}

std::unique_ptr<AdaptiveRetrieval> AdaptiveRetrievalFactory::createHighRecall()
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k               = 8;
    cfg.max_top_k                = 30;
    cfg.base_similarity_threshold = 0.60;
    cfg.min_similarity_threshold  = 0.25;
    cfg.complexity_scaling        = 1.6;
    return std::make_unique<AdaptiveRetrieval>(cfg);
}

} // namespace themis::rag

