/**
 * @file negative_keyword_filter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/negative_keyword_filter.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <unordered_set>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

NegativeKeywordFilter::NegativeKeywordFilter(SecondaryIndexManager* index)
    : NegativeKeywordFilter(index, Config{}) {}

NegativeKeywordFilter::NegativeKeywordFilter(SecondaryIndexManager* index,
                                             const Config& config)
    : index_(index), config_(config) {}

// ============================================================================
// Static helpers
// ============================================================================

NegativeKeywordFilter::ParsedQuery
NegativeKeywordFilter::parseQuery(const std::string& raw_query) {
    ParsedQuery result;

    std::istringstream iss(raw_query);
    std::string token;
    bool next_is_negative = false;
    std::string positive_buf;

    while (iss >> token) {
        // Check for the `NOT` keyword (case-insensitive)
        std::string token_upper;
        token_upper.reserve(token.size());
        for (unsigned char c : token) {
            token_upper += static_cast<char>(std::toupper(c));
        }

        if (token_upper == "NOT") {
            next_is_negative = true;
            continue;
        }

        if (next_is_negative) {
            // The previous token was `NOT` — this token is excluded
            std::string neg;
            neg.reserve(token.size());
            for (unsigned char c : token) {
                neg += static_cast<char>(std::tolower(c));
            }
            if (!neg.empty()) {
                result.negative_terms.push_back(std::move(neg));
            }
            next_is_negative = false;
            continue;
        }

        if (token.size() >= 2 && token[0] == '-') {
            // Minus-prefix syntax: `-term`
            std::string neg(token.begin() + 1, token.end());
            std::transform(neg.begin(), neg.end(), neg.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (!neg.empty()) {
                result.negative_terms.push_back(std::move(neg));
            }
            continue;
        }

        // Regular positive token
        if (!positive_buf.empty()) positive_buf += ' ';
        positive_buf += token;
        next_is_negative = false;
    }

    result.positive_query = std::move(positive_buf);
    return result;
}

// ============================================================================
// filter()
// ============================================================================

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
NegativeKeywordFilter::filter(
    const std::string& table,
    const std::string& column,
    const std::vector<std::string>& candidate_pks,
    const std::vector<std::string>& negative_terms
) const {
    if (negative_terms.empty()) {
        return {SecondaryIndexManager::Status::OK(), candidate_pks};
    }

    if (!index_) {
        return {SecondaryIndexManager::Status::Error(
                    "NegativeKeywordFilter: null SecondaryIndexManager"),
                candidate_pks};
    }

    if (candidate_pks.empty()) {
        return {SecondaryIndexManager::Status::OK(), {}};
    }

    // Build the set of PKs to exclude: union of all documents that contain
    // any of the negative terms.
    std::unordered_set<std::string> excluded;
    SecondaryIndexManager::Status last_error = SecondaryIndexManager::Status::OK();

    for (const auto& term : negative_terms) {
        if (term.empty()) continue;

        try {
            auto [status, neg_results] = index_->scanFulltext(
                table, column, term,
                config_.max_exclude_scan == 0
                    ? std::numeric_limits<size_t>::max()
                    : config_.max_exclude_scan);

            if (!status.ok) {
                THEMIS_WARN(
                    "NegativeKeywordFilter: scanFulltext failed for term '{}': {}",
                    term, status.message);
                last_error = status;
                continue;
            }

            for (const auto& pk : neg_results) {
                excluded.insert(pk);
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("NegativeKeywordFilter: exception scanning term '{}': {}",
                         term, e.what());
            last_error = SecondaryIndexManager::Status::Error(e.what());
        }
    }

    // Remove excluded PKs from the candidate set while preserving order.
    std::vector<std::string> filtered;
    filtered.reserve(candidate_pks.size());
    for (const auto& pk : candidate_pks) {
        if (excluded.find(pk) == excluded.end()) {
            filtered.push_back(pk);
        }
    }

    THEMIS_DEBUG(
        "NegativeKeywordFilter: {}/{} candidates survived NOT filter "
        "(excluded {} docs for {} negative terms)",
        filtered.size(), candidate_pks.size(),
        excluded.size(), negative_terms.size());

    return {last_error, std::move(filtered)};
}

} // namespace themis
