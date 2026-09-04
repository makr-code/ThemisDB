/**
 * @file prompt_quality_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/prompt_quality_evaluator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace prompt_engineering {

// ── Helper: tokenize ─────────────────────────────────────────────────────────

std::vector<std::string> PromptQualityEvaluator::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string word = {};
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else {
            if (!word.empty()) {
                tokens.push_back(std::move(word));
            }
        }
    }
    if (!word.empty()) {
        tokens.push_back(std::move(word));
    }
    return tokens;
}

// ── Helper: case-insensitive contains ─────────────────────────────────────────

bool PromptQualityEvaluator::containsIgnoreCase(const std::string& haystack,
                                                 const std::string& needle) {
    if (needle.empty()) {
      return false;
    }
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });
    return it != haystack.end();
}

// ── Injection check ───────────────────────────────────────────────────────────

void PromptQualityEvaluator::checkInjection(
    const std::string&              text,
    const std::vector<std::string>& blocklist,
    std::vector<QualityCheck>&      failed) const {
    int pattern_idx = 1;
    for (const auto& pattern : blocklist) {
        if (containsIgnoreCase(text, pattern)) {
            std::string id = "INJ-" + std::to_string(pattern_idx);
            failed.push_back(QualityCheck{
                id,
                "Injection pattern detected: '" + pattern + "'",
                false,
                "Text contains blocklisted injection pattern: '" + pattern + "'"
            });
        }
        ++pattern_idx;
    }
}

// ── Token diversity check ─────────────────────────────────────────────────────

void PromptQualityEvaluator::checkTokenDiversity(
    const std::string&         text,
    double                     min_diversity,
    std::vector<QualityCheck>& failed) const {
    const auto tokens = tokenize(text);
    if (tokens.empty()) {
        // Empty text cannot meet any diversity threshold; warn but don't fail.
        return;
    }
    const std::unordered_set<std::string> distinct(tokens.begin(), tokens.end());
    const double diversity = static_cast<double>(distinct.size()) /
                             static_cast<double>(tokens.size());
    if (diversity < min_diversity) {
        failed.push_back(QualityCheck{
            "DIV-001",
            "Token diversity below threshold",
            false,
            "Diversity ratio " +
                std::to_string(diversity).substr(0, 6) +
                " < required " +
                std::to_string(min_diversity).substr(0, 6)
        });
    }
}

// ── Bigram repetition check ───────────────────────────────────────────────────

void PromptQualityEvaluator::checkRepetition(
    const std::string&         text,
    double                     max_repetition,
    std::vector<QualityCheck>& failed) const {
    const auto tokens = tokenize(text);
    if (static_cast<int>(tokens.size()) < 2) {
        return;  // not enough tokens to form bigrams
    }

    // Build consecutive bigrams and count repetitions.
    std::unordered_map<std::string, size_t> bigram_counts = {};

    for (size_t i = 0; i + 1 <static_cast<int>(tokens.size()); ++i) {
        bigram_counts[tokens[i] + '\0' + tokens[i + 1]]++;
    }

    const size_t total_bigrams = static_cast<int>(tokens.size()) - 1;
    size_t repeated = 0;
    for (const auto& [bigram, count] : bigram_counts) {
        if (count > 1) {
            repeated += (count - 1);  // each repeated occurrence beyond first
        }
    }

    const double ratio = static_cast<double>(repeated) /
                         static_cast<double>(total_bigrams);
    if (ratio > max_repetition) {
        failed.push_back(QualityCheck{
            "REP-001",
            "Consecutive bigram repetition ratio above threshold",
            false,
            "Repetition ratio " +
                std::to_string(ratio).substr(0, 6) +
                " > allowed " +
                std::to_string(max_repetition).substr(0, 6)
        });
    }
}

// ── evaluateText ──────────────────────────────────────────────────────────────

QualityReport PromptQualityEvaluator::evaluateText(
    const std::string&   text,
    const QualityConfig& config) const {
    QualityReport report;
    report.threshold = config.min_score_threshold;

    // Count total checks: one per injection pattern + 1 diversity + 1 repetition.
    const size_t total_checks = static_cast<int>(config.injection_blocklist.size()) + 2;

    checkInjection(text, config.injection_blocklist, report.failed_checks);
    checkTokenDiversity(text, config.min_token_diversity, report.failed_checks);
    checkRepetition(text, config.max_repetition_ratio, report.failed_checks);

    if (total_checks == 0) {
        report.score = 1.0;
    } else {
        const size_t passed = total_checks - report.failed_checks.size();
        report.score = static_cast<double>(passed) /
                       static_cast<double>(total_checks);
    }

    return report;
}

// ── evaluate (IPromptTemplate) ────────────────────────────────────────────────

QualityReport PromptQualityEvaluator::evaluate(
    const IPromptTemplate& tmpl,
    const QualityConfig&   config) const {
    // Render the template against an empty context.  Missing required slots
    // are treated as empty strings; rendering errors become structural failures.
    std::string rendered = {};
    try {
        rendered = tmpl.render({});
    } catch (const std::exception& ex) {
        // Structural integrity failure — treat the raw error message as the text.
        QualityReport report;
        report.threshold = config.min_score_threshold;
        report.failed_checks.push_back(QualityCheck{
            "STR-001",
            "Template rendering failed",
            false,
            std::string("Render error: ") + ex.what()
        });
        report.score = 0.0;
        return report;
    }
    return evaluateText(rendered, config);
}

} // namespace prompt_engineering
} // namespace themis
