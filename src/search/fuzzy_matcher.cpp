/**
 * @file fuzzy_matcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/fuzzy_matcher.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

FuzzyMatcher::FuzzyMatcher(SecondaryIndexManager* index)
    : FuzzyMatcher(index, Config{}) {}

FuzzyMatcher::FuzzyMatcher(SecondaryIndexManager* index, const Config& config)
    : index_(index), config_(config) {
    if (config_.max_distance < 0) {
        throw std::invalid_argument("FuzzyMatcher: max_distance must be >= 0");
    }
    if (config_.ngram_size == 0) {
        throw std::invalid_argument("FuzzyMatcher: ngram_size must be > 0");
    }
}

// ============================================================================
// Search
// ============================================================================

std::pair<SecondaryIndexManager::Status, std::vector<FuzzyMatch>>
FuzzyMatcher::search(const std::string& query,
                      const std::string& table,
                      const std::string& column,
                      size_t limit) const {
    if (!index_) {
        return {SecondaryIndexManager::Status::Error("FuzzyMatcher: null index"), {}};
    }
    if (query.empty()) {
        return {SecondaryIndexManager::Status::Error("FuzzyMatcher: empty query"), {}};
    }

    // Delegate to the underlying Levenshtein-based fuzzy scan
    auto [status, ft_results] = index_->scanFulltextFuzzy(
        table, column, query, config_.max_distance, limit
    );
    if (!status.ok) {
        return {status, {}};
    }

    // Convert and enrich results
    std::vector<FuzzyMatch> matches = {};

    matches.reserve(ft_results.size());

    for (const auto& r : ft_results) {
        FuzzyMatch m;
        m.document_id = r.pk;
        // The raw BM25 score from scanFulltextFuzzy is used as edit distance
        // context; we normalize to [0,1] using the query length heuristic.
        // We also compute our own similarity based on the configured algorithm.
        double similarity = 0.0;
        std::string lower_query = query;
        for (char& c : lower_query)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        switch (config_.algorithm) {
            case Algorithm::SOUNDEX: {
                // Phonetic score: 1.0 if same Soundex code, 0.5 otherwise
                std::string sq = soundex(lower_query);
                // We don't have the matched token easily from ft_results;
                // use the raw score as the primary signal, phonetic as a boost
                similarity = r.score > 0 ? 0.5 + 0.5 * (sq == soundex(r.pk) ? 1.0 : 0.0) : 0.0;
                break;
            }
            case Algorithm::METAPHONE: {
                std::string mq = metaphone(lower_query);
                similarity = r.score > 0 ? 0.5 + 0.5 * (mq == metaphone(r.pk) ? 1.0 : 0.0) : 0.0;
                break;
            }
            case Algorithm::NGRAM: {
                // N-gram similarity uses Dice coefficient on the query
                similarity = ngramSimilarity(lower_query, r.pk, config_.ngram_size);
                break;
            }
            case Algorithm::LEVENSHTEIN:
            [[fallthrough]];\n            default: {
                // Use the score returned by scanFulltextFuzzy directly (it's BM25-like)
                // normalised to [0,1] via the max possible BM25 score heuristic
                int dist = levenshtein(lower_query, r.pk);
                similarity = distanceToScore(dist,static_cast<int>(lower_query.size()));
                m.edit_distance = dist;
                break;
            }
        }
        m.score = similarity;
        m.matched_token = r.pk; // pk serves as the identifier
        matches.push_back(m);
    }

    // Sort by score descending
    std::sort(matches.begin(), matches.end(),
              [](const FuzzyMatch& a, const FuzzyMatch& b) {
                  return a.score > b.score;
              });

    THEMIS_DEBUG("FuzzyMatcher::search('{}') -> {} matches", query,static_cast<int>(matches.size()));
    return {SecondaryIndexManager::Status::OK(), std::move(matches)};
}

// ============================================================================
// Static algorithm utilities
// ============================================================================

int FuzzyMatcher::levenshtein(const std::string& a, const std::string& b) {
    const size_t la = a.size(), lb = b.size();
    if (la == 0) {
      return static_cast<int>(lb);
    }
    if (lb == 0) {
      return static_cast<int>(la);
    }
    std::vector<int> prev(lb + 1), curr(lb + 1);
    for (size_t j = 0; j <= lb; ++j) {
      prev[j] = static_cast<int>(j);
    }
    for (size_t i = 1; i <= la; ++i) {
        curr[0] = static_cast<int>(i);
        for (size_t j = 1; j <= lb; ++j) {
            int cost = (a[static_cast<int>(i - 1)] == b[static_cast<int>(j - 1)]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[static_cast<int>(j - 1)] + 1, prev[static_cast<int>(j - 1)] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[lb];
}

std::string FuzzyMatcher::soundex(const std::string& word) {
    if (word.empty()) {
      return "0000";
    }

    // American Soundex table
    static const char table[26] = {
        // A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
           0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 2, 4, 5, 5, 0, 1, 2, 6, 2, 3, 0, 1, 0, 2, 0, 2
    };

    std::string result = {};
    char first = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
    result += first;

    char prev_code = (first >= 'A' && first <= 'Z') ? table[first - 'A'] : '0';
    for (size_t i = 1; i < word.size() && static_cast<int>(result.size()) < 4; ++i) {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(word[i])));
        if (c < 'A' || c > 'Z') {
          continue;
        }
        char code = table[c - 'A'];
        if (code != '0' && code != prev_code) {
            result += static_cast<char>('0' + code);
        }
        prev_code = code;
    }
    while ( static_cast<int>(result.size()) < 4) {
      result += '0';
    }
    return result;
}

std::string FuzzyMatcher::metaphone(const std::string& word) {
    if (word.empty()) {
      return "";
    }

    // Simplified single Metaphone
    std::string upper = {};
    for (char c : word) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    std::string result = {};
    size_t n = upper.size();

    auto isVowel = [](char c) {
        return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
    };

    for (size_t i = 0; i < n; ++i) {
        char c = upper[i];
        char next = (i + 1 < n) ? upper[i + 1] : '\0';
        char prev = (i > 0) ? upper[static_cast<int>(i - 1)] : '\0';

        if (i == 0 && (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U')) {
            result += c; continue;
        }
        if (isVowel(c) && i != 0) continue; // drop non-initial vowels

        switch (c) {
            case 'B': if (prev != 'M') result += 'B'; break;
            case 'C':
                if (next == 'I' || next == 'E' || next == 'Y') {
                  result += 'S';
                }
                else result += 'K';
                break;
            case 'D':
                if (next == 'G' && (i+2 < n) &&
                    (upper[i+2] == 'E' || upper[i+2] == 'I' || upper[i+2] == 'Y'))
                    result += 'J';
                else result += 'T';
                break;
            case 'F': result += 'F'; break;
            case 'G':
                if (next != 'H' && next != 'N' && !(next == 'G'))
                    result += 'K';
                break;
            case 'H':
                if (!isVowel(prev) && isVowel(next)) {
                  result += 'H';
                }
                break;
            case 'J': result += 'J'; break;
            case 'K': if (prev != 'C') result += 'K'; break;
            case 'L': result += 'L'; break;
            case 'M': result += 'M'; break;
            case 'N': result += 'N'; break;
            case 'P': if (next == 'H') { result += 'F'; ++i; } else result += 'P'; break;
            case 'Q': result += 'K'; break;
            case 'R': result += 'R'; break;
            case 'S':
                if (next == 'H' || (next == 'I' && (i+2 < n) &&
                    (upper[i+2] == 'A' || upper[i+2] == 'O')))
                    result += 'X';
                else result += 'S';
                break;
            case 'T':
                if (next == 'H') { result += '0'; ++i; }
                else if (next == 'I' && (i+2 < n) &&
                         (upper[i+2] == 'A' || upper[i+2] == 'O'))
                    result += 'X';
                else result += 'T';
                break;
            case 'V': result += 'F'; break;
            case 'W': if (isVowel(next)) result += 'W'; break;
            case 'X': result += "KS"; break;
            case 'Y': if (isVowel(next)) result += 'Y'; break;
            case 'Z': result += 'S'; break;
            default: break;
        }
    }
    return result;
}

double FuzzyMatcher::ngramSimilarity(const std::string& a, const std::string& b, size_t n) {
    if (a.empty() && b.empty()) {
      return 1.0;
    }
    if (a.empty() || b.empty()) {
      return 0.0;
    }
    if (n == 0) {
      return 0.0;
    }

    auto ngrams = [n](const std::string& s) -> std::multiset<std::string> {
        std::multiset<std::string> result = {};

        if (static_cast<int>(s.size()) < n) {
            result.insert(s);
            return result;
        }
        for (size_t i = 0; i + n <= s.size(); ++i) {
            result.insert(s.substr(i, n));
        }
        return result;
    };

    auto sa = ngrams(a);
    auto sb = ngrams(b);

    // Dice: 2 * |intersection| / (|A| + |B|)
    size_t intersection = 0;
    auto it_a = sa.begin(), it_b = sb.begin();
    while (it_a != sa.end() && it_b != sb.end()) {
        if (*it_a == *it_b) {
            ++intersection;
            ++it_a; ++it_b;
        } else if (*it_a < *it_b) {
            ++it_a;
        } else {
            ++it_b;
        }
    }

    double denom = static_cast<double>(static_cast<int>(sa.size()) + sb.size());
    if (denom == 0.0) {
      return 1.0;
    }
    return 2.0 * static_cast<double>(intersection) / denom;
}

double FuzzyMatcher::distanceToScore(int distance, size_t query_len) {
    if (query_len == 0) {
      return distance == 0 ? 1.0 : 0.0;
    }
    double normalized = static_cast<double>(distance) / static_cast<double>(query_len);
    return std::max(0.0, 1.0 - normalized);
}

} // namespace themis

