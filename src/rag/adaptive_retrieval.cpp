/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_retrieval.cpp                             ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:50:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     317                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5f8c6f5fe6  2026-04-12  feat(rag): implement MultiHopReasoner and AdaptiveRetriev... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adaptive_retrieval.cpp
 * @brief Implementation of adaptive retrieval depth (Phase 7).
 */

#include "rag/adaptive_retrieval.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace themis::rag {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AdaptiveRetrieval::AdaptiveRetrieval(const AdaptiveRetrievalConfig& config)
    : config_(config)
{}

const AdaptiveRetrievalConfig& AdaptiveRetrieval::getConfig() const
{
    return config_;
}

void AdaptiveRetrieval::setConfig(const AdaptiveRetrievalConfig& config)
{
    config_ = config;
}

void AdaptiveRetrieval::setScorer(IComplexityScorer* scorer)
{
    scorer_ = scorer;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

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
    while (ss >> tok) tokens.push_back(tok);
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
    if (analysis.is_long_query) score += 0.15;

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

QueryComplexity AdaptiveRetrieval::scoreToComplexity(double raw_score)
{
    if (raw_score < 0.30) return QueryComplexity::SIMPLE;
    if (raw_score < 0.55) return QueryComplexity::MODERATE;
    if (raw_score < 0.75) return QueryComplexity::COMPLEX;
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
    const double scale = config_.complexity_scaling;
    size_t k = config_.base_top_k;

    switch (complexity) {
        case QueryComplexity::SIMPLE:
            break;
        case QueryComplexity::MODERATE:
            k = static_cast<size_t>(std::round(k * scale));
            break;
        case QueryComplexity::COMPLEX:
            k = static_cast<size_t>(std::round(k * scale * scale));
            break;
        case QueryComplexity::VERY_COMPLEX:
            k = static_cast<size_t>(std::round(k * scale * scale * scale));
            break;
    }

    return std::min(k, config_.max_top_k);
}

double AdaptiveRetrieval::complexityToThreshold(
    QueryComplexity complexity) const
{
    // Linear interpolation between base_similarity_threshold (SIMPLE)
    // and min_similarity_threshold (VERY_COMPLEX) over 4 tiers (0-3).
    const int tier = static_cast<int>(complexity);  // 0..3
    const double base  = config_.base_similarity_threshold;
    const double floor = config_.min_similarity_threshold;
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
