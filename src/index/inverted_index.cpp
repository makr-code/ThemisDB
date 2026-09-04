/**
 * @file inverted_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.26
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=20, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 ThemisDB Contributors

#include "index/inverted_index.h"
#include <stdexcept>
#include "storage/key_schema.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include "utils/normalizer.h"
#include "utils/stemmer.h"
#include "utils/stopwords.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

InvertedIndex::InvertedIndex(RocksDBWrapper& db) : db_(db) {}

// ============================================================================
// Key-schema helpers
// ============================================================================

std::string InvertedIndex::makeMetaKey(std::string_view table,
                                       std::string_view column) {
    return "ftidxmeta:" + std::string(table) + ":" + std::string(column);
}

std::string InvertedIndex::makeIndexKey(std::string_view table,
                                        std::string_view column,
                                        std::string_view token,
                                        std::string_view pk) {
    return "ftidx:" + std::string(table) + ":" + std::string(column) + ":" +
           std::string(token) + ":" + std::string(pk);
}

std::string InvertedIndex::makeIndexPrefix(std::string_view table,
                                           std::string_view column,
                                           std::string_view token) {
    return "ftidx:" + std::string(table) + ":" + std::string(column) + ":" +
           std::string(token) + ":";
}

std::string InvertedIndex::makeTFKey(std::string_view table,
                                     std::string_view column,
                                     std::string_view token,
                                     std::string_view pk) {
    return "fttf:" + std::string(table) + ":" + std::string(column) + ":" +
           std::string(token) + ":" + std::string(pk);
}

std::string InvertedIndex::makeDocLenKey(std::string_view table,
                                         std::string_view column,
                                         std::string_view pk) {
    return "ftdlen:" + std::string(table) + ":" + std::string(column) + ":" +
           std::string(pk);
}

std::string InvertedIndex::makeRevKey(std::string_view table,
                                      std::string_view column,
                                      std::string_view pk) {
    return "ftrev:" + std::string(table) + ":" + std::string(column) + ":" +
           std::string(pk);
}

// ============================================================================
// Index lifecycle
// ============================================================================

InvertedIndex::Status InvertedIndex::create(std::string_view table,
                                            std::string_view column) {
    return create(table, column, Config{});
}

InvertedIndex::Status InvertedIndex::create(std::string_view table,
                                            std::string_view column,
                                            Config config) {
    if (table.empty() || column.empty())
        return Status::Error("InvertedIndex::create: table/column must not be empty");
    if (table.find(':') != std::string_view::npos ||
        column.find(':') != std::string_view::npos)
        return Status::Error(
            "InvertedIndex::create: ':' is not allowed in table/column names");

    nlohmann::json j = {
        {"type", "fulltext"},
        {"stemming_enabled", config.stemming_enabled},
        {"language", config.language},
        {"stopwords_enabled", config.stopwords_enabled},
        {"stopwords", config.stopwords},
        {"normalize_umlauts", config.normalize_umlauts},
    };
    std::string s = j.dump();
    std::vector<uint8_t> bytes(s.begin(), s.end());

    if (!db_.put(makeMetaKey(table, column), bytes))
        return Status::Error("InvertedIndex::create: failed to write meta key");

    THEMIS_INFO("InvertedIndex created: {}.{} (lang={}, stemming={}, stopwords={})",
                table, column, config.language, config.stemming_enabled,
                config.stopwords_enabled);
    return Status::OK();
}

InvertedIndex::Status InvertedIndex::drop(std::string_view table,
                                          std::string_view column) {
    if (table.empty() || column.empty())
        return Status::Error("InvertedIndex::drop: table/column must not be empty");
    if (!db_.del(makeMetaKey(table, column)))
        return Status::Error("InvertedIndex::drop: failed to delete meta key");
    THEMIS_INFO("InvertedIndex dropped: {}.{}", table, column);
    return Status::OK();
}

bool InvertedIndex::exists(std::string_view table,
                           std::string_view column) const {
    return db_.get(makeMetaKey(table, column)).has_value();
}

std::optional<InvertedIndex::Config>
InvertedIndex::getConfig(std::string_view table,
                         std::string_view column) const {
    auto val = db_.get(makeMetaKey(table, column));
    if (!val) {
      return std::nullopt;
    }
    try {
        std::string s(val->begin(), val->end());
        auto j = nlohmann::json::parse(s);
        Config cfg;
        cfg.stemming_enabled  = j.value("stemming_enabled", false);
        cfg.language          = j.value("language", std::string("none"));
        cfg.stopwords_enabled = j.value("stopwords_enabled", false);
        cfg.normalize_umlauts = j.value("normalize_umlauts", false);
        if (j.contains("stopwords") && j["stopwords"].is_array()) {
            for (const auto& w : j["stopwords"])
                if (w.is_string()) {
                  cfg.stopwords.push_back(w.get<std::string>());
                }
        }
        return cfg;
    } catch (...) {
        return Config{}; // legacy or corrupt – return safe defaults
    }
}

// ============================================================================
// Tokenisation
// ============================================================================

std::vector<std::string> InvertedIndex::tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string cur;
    for (unsigned char c : text) {
        if (std::isspace(c) || std::ispunct(c)) {
            if (!cur.empty()) {
                std::transform(cur.begin(), cur.end(), cur.begin(),
                               [](unsigned char ch) { return std::tolower(ch); });
                tokens.push_back(std::move(cur));
            }
        } else {
            cur.push_back(static_cast<char>(c));
        }
    }
    if (!cur.empty()) {
        std::transform(cur.begin(), cur.end(), cur.begin(),
                       [](unsigned char ch) { return std::tolower(ch); });
        tokens.push_back(std::move(cur));
    }
    return tokens;
}

std::vector<std::string> InvertedIndex::tokenize(std::string_view text,
                                                  const Config& config) {
    std::string normalized;
    if (config.normalize_umlauts) {
        normalized = utils::Normalizer::normalizeUmlauts(text);
    }
    auto tokens = tokenize(normalized.empty() ? text
                                              : std::string_view(normalized));

    if (config.stopwords_enabled) {
        auto base = utils::Stopwords::defaults(config.language);
        auto sw   = utils::Stopwords::merge(base, config.stopwords);
        tokens.erase(
            std::remove_if(tokens.begin(), tokens.end(),
                           [&]([[maybe_unused]] const std::string& t) { return sw.count(t) > 0; }),
            tokens.end());
    }

    if (config.stemming_enabled) {
        auto lang = utils::Stemmer::parseLanguage(config.language);
        for (auto& t : tokens)
            t = utils::Stemmer::stem(t, lang);
    }

    return tokens;
}

// ============================================================================
// Document indexing (internal helper)
// ============================================================================

void InvertedIndex::removePostings_(std::string_view table,
                                    std::string_view column,
                                    std::string_view pk) {
    // Read the reverse-index key to discover which tokens are currently stored
    // for this pk, then delete each posting + TF entry.
    auto revKey = makeRevKey(table, column, pk);
    auto val = db_.get(revKey);
    if (val && !val->empty()) {
        try {
            std::string s(val->begin(), val->end());
            auto j = nlohmann::json::parse(s);
            if (j.is_array()) {
                for (const auto& item : j) {
                    if (!item.is_string()) {
                      continue;
                    }
                    const auto tok = item.get<std::string>();
                    if (tok.empty()) {
                      continue;
                    }
                    db_.del(makeIndexKey(table, column, tok, pk));
                    db_.del(makeTFKey(table, column, tok, pk));
                }
            }
        } catch (...) {
            // Log and continue: a corrupt or missing reverse-index key means we
            // cannot clean up stale posting entries for this pk, but we must not
            // crash.  The stale entries will be invisible to callers (unreachable
            // through normal queries) and cleaned up by a future deindex() call.
            THEMIS_WARN("InvertedIndex::removePostings_: failed to parse reverse-index "
                        "key for {}.{} pk={}", table, column, pk);
        }
    }
    db_.del(makeDocLenKey(table, column, pk));
    db_.del(revKey);
}

InvertedIndex::Status InvertedIndex::index(std::string_view table,
                                           std::string_view column,
                                           std::string_view pk,
                                           std::string_view text) {
    if (!exists(table, column))
        return Status::Error("InvertedIndex::index: no index for " +
                             std::string(table) + "." + std::string(column));
    if (pk.empty()) {
      return Status::Error("InvertedIndex::index: pk must not be empty");
    }

    auto config = getConfig(table, column).value_or(Config{});

    // Remove previous posting entries (upsert semantics via reverse-index key)
    removePostings_(table, column, pk);

    if (text.empty()) {
      return Status::OK();
    }

    auto tokens = tokenize(text, config);

    // Build TF map
    std::unordered_map<std::string, uint32_t> tf = {};

    for (const auto& t : tokens) {
      if (!t.empty()) tf[t]++;
    }

    // Write doc-length
    {
        std::string s = std::to_string(tokens.size());
        std::vector<uint8_t> v(s.begin(), s.end());
        db_.put(makeDocLenKey(table, column, pk), v);
    }

    std::vector<uint8_t> pkBytes(pk.begin(), pk.end());

    // Build JSON array of indexed tokens for the reverse-index key
    nlohmann::json revTokens = nlohmann::json::array();
    for (const auto& [tok, cnt] : tf) {
        db_.put(makeIndexKey(table, column, tok, pk), pkBytes);
        std::string s = std::to_string(cnt);
        std::vector<uint8_t> v(s.begin(), s.end());
        db_.put(makeTFKey(table, column, tok, pk), v);
        revTokens.push_back(tok);
    }

    // Persist the reverse-index key so removePostings_ can find these tokens later
    std::string revStr = revTokens.dump();
    std::vector<uint8_t> revBytes(revStr.begin(), revStr.end());
    db_.put(makeRevKey(table, column, pk), revBytes);

    return Status::OK();
}

InvertedIndex::Status InvertedIndex::deindex(std::string_view table,
                                             std::string_view column,
                                             std::string_view pk,
                                             std::string_view /*text_deprecated*/) {
    if (!exists(table, column))
        return Status::Error("InvertedIndex::deindex: no index for " +
                             std::string(table) + "." + std::string(column));
    if (pk.empty()) {
      return Status::Error("InvertedIndex::deindex: pk must not be empty");
    }

    removePostings_(table, column, pk);
    return Status::OK();
}

// ============================================================================
// Search – BM25 (internal)
// ============================================================================

std::pair<InvertedIndex::Status, std::vector<InvertedIndex::SearchResult>>
InvertedIndex::computeBM25_(std::string_view table, std::string_view column,
                             std::string_view query, size_t limit) const {
    if (!exists(table, column))
        return {Status::Error("InvertedIndex::search: no index for " +
                              std::string(table) + "." + std::string(column)),
                {}};

    auto config  = getConfig(table, column).value_or(Config{});
    auto tokens  = tokenize(query, config);
    if (tokens.empty()) return {Status::OK(), {}};

    // Retrieve posting sets for each token
    std::vector<std::unordered_set<std::string>> postings;
    postings.reserve(tokens.size());
    for (const auto& tok : tokens) {
        std::unordered_set<std::string> pks;
        db_.scanPrefix(makeIndexPrefix(table, column, tok),
                       [&pks](std::string_view key, std::string_view) {
                           auto pos = key.rfind(':');
                           if (pos != std::string_view::npos)
                               pks.insert(std::string(key.substr(pos + 1)));
                           return true;
                       });
        postings.push_back(std::move(pks));
    }

    // AND intersection
    std::unordered_set<std::string> intersection = postings[0];
    for (size_t i = 1; i < postings.size(); ++i) {
        std::unordered_set<std::string> tmp = {};

        for (const auto& pk : intersection)
            if (postings[i].count(pk)) {
              tmp.insert(pk);
            }
        intersection = std::move(tmp);
    }
    if (intersection.empty()) return {Status::OK(), {}};

    // Build universe for N and avgdl
    std::unordered_set<std::string> universe = {};

    for (const auto& ps : postings)
        for (const auto& pk : ps) {
          universe.insert(pk);
        }

    const double N = static_cast<double>(std::max<size_t>(1, universe.size()));

    std::unordered_map<std::string, double> docLen;
    double totalLen = 0.0;
    for (const auto& pk : universe) {
        auto v = db_.get(makeDocLenKey(table, column, pk));
        double dl = 0.0;
        if (v && !v->empty()) {
            std::string s(v->begin(), v->end());
            try { dl = static_cast<double>(std::stoull(s)); } catch (...) {}
        }
        docLen[pk]  = dl;
        totalLen   += dl;
    }
    const double avgdl =
        universe.empty() ? 1.0 : std::max(1.0, totalLen / N);

    // df per token
    std::vector<double> dfs = {};

    dfs.reserve(postings.size());
    for (const auto& ps : postings)
        dfs.push_back(static_cast<double>(ps.size()));

    // BM25 parameters
    constexpr double k1 = 1.2;
    constexpr double b  = 0.75;

    std::vector<SearchResult> scored = {};

    scored.reserve(intersection.size());

    for (const auto& pk : intersection) {
        double dl = docLen.count(pk) ? docLen.at(pk) : 0.0;
        double score = 0.0;
        for (size_t i = 0; i < tokens.size(); ++i) {
            double df  = std::max(1.0, dfs[i]);
            double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
            auto tfv = db_.get(makeTFKey(table, column, tokens[i], pk));
            double tf = 1.0;
            if (tfv && !tfv->empty()) {
                std::string s(tfv->begin(), tfv->end());
                try { tf = static_cast<double>(std::stoul(s)); } catch (...) {}
            }
            double denom = tf + k1 * (1.0 - b + b * (dl / avgdl));
            if (denom <= 0.0) {
              denom = tf + k1;
            }
            score += idf * ((tf * (k1 + 1.0)) / denom);
        }
        scored.push_back({std::string(pk), score});
    }

    std::sort(scored.begin(), scored.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    if (scored.size() > limit) {
      scored.resize(limit);
    }
    return {Status::OK(), std::move(scored)};
}

// ============================================================================
// Public search API
// ============================================================================

std::pair<InvertedIndex::Status, std::vector<InvertedIndex::SearchResult>>
InvertedIndex::search(std::string_view table, std::string_view column,
                      std::string_view query, size_t limit) const {
    return computeBM25_(table, column, query, limit);
}

// ============================================================================
// Phrase search
// ============================================================================

std::pair<InvertedIndex::Status, std::vector<InvertedIndex::SearchResult>>
InvertedIndex::searchPhrase(std::string_view table, std::string_view column,
                             std::string_view phrase, size_t limit) const {
    if (!exists(table, column))
        return {Status::Error("InvertedIndex::searchPhrase: no index for " +
                              std::string(table) + "." + std::string(column)),
                {}};
    if (phrase.empty()) return {Status::OK(), {}};

    auto config = getConfig(table, column).value_or(Config{});
    auto tokens = tokenize(phrase, config);
    if (tokens.empty()) return {Status::OK(), {}};

    // Candidate documents: AND of all token postings
    std::vector<std::unordered_set<std::string>> postings;
    for (const auto& tok : tokens) {
        std::unordered_set<std::string> pks;
        db_.scanPrefix(makeIndexPrefix(table, column, tok),
                       [&pks](std::string_view key, std::string_view) {
                           auto pos = key.rfind(':');
                           if (pos != std::string_view::npos)
                               pks.insert(std::string(key.substr(pos + 1)));
                           return true;
                       });
        postings.push_back(std::move(pks));
    }

    std::unordered_set<std::string> candidates = postings[0];
    for (size_t i = 1; i < postings.size(); ++i) {
        std::unordered_set<std::string> tmp = {};

        for (const auto& pk : candidates)
            if (postings[i].count(pk)) {
              tmp.insert(pk);
            }
        candidates = std::move(tmp);
    }
    if (candidates.empty()) return {Status::OK(), {}};

    // Verify exact phrase in original field via stored relational key
    std::string phraseNorm = std::string(phrase);
    if (config.normalize_umlauts)
        phraseNorm = utils::Normalizer::normalizeUmlauts(phraseNorm);
    std::transform(phraseNorm.begin(), phraseNorm.end(), phraseNorm.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::vector<SearchResult> results = {};

    for (const auto& pk : candidates) {
        auto blob = db_.get(KeySchema::makeRelationalKey(table, pk));
        if (!blob || blob->empty()) {
          continue;
        }
        try {
            BaseEntity::Blob beBlob(blob->begin(), blob->end());
            auto entity = BaseEntity::deserialize(pk, beBlob);
            auto maybeVal = entity.extractField(column);
            if (!maybeVal) {
              continue;
            }
            std::string field = *maybeVal;
            if (config.normalize_umlauts)
                field = utils::Normalizer::normalizeUmlauts(field);
            std::transform(field.begin(), field.end(), field.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (field.find(phraseNorm) != std::string::npos)
                results.push_back({pk, 1.0});
        } catch (...) {
            // skip unreadable documents
        }
        if (results.size() >= limit) {
          break;
        }
    }
    return {Status::OK(), std::move(results)};
}

// ============================================================================
// Fuzzy search
// ============================================================================

namespace {
int levenshtein(const std::string& s1, const std::string& s2) {
    const size_t m = s1.size(), n = s2.size();
    if (m == 0) {
      return static_cast<int>(n);
    }
    if (n == 0) {
      return static_cast<int>(m);
    }
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (size_t i = 0; i <= m; ++i) {
      dp[i][0] = static_cast<int>(i);
    }
    for (size_t j = 0; j <= n; ++j) {
      dp[0][j] = static_cast<int>(j);
    }
    for (size_t i = 1; i <= m; ++i)
        for (size_t j = 1; j <= n; ++j) {
            int cost      = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1,
                                  dp[i - 1][j - 1] + cost});
        }
    return dp[m][n];
}
} // anonymous namespace

std::pair<InvertedIndex::Status, std::vector<InvertedIndex::SearchResult>>
InvertedIndex::searchFuzzy(std::string_view table, std::string_view column,
                            std::string_view query, int maxDistance,
                            size_t limit) const {
    if (!exists(table, column))
        return {Status::Error("InvertedIndex::searchFuzzy: no index for " +
                              std::string(table) + "." + std::string(column)),
                {}};
    if (query.empty() || maxDistance < 0) return {Status::OK(), {}};

    auto config      = getConfig(table, column).value_or(Config{});
    auto queryTokens = tokenize(query, config);
    if (queryTokens.empty()) return {Status::OK(), {}};

    // Single prefix scan over all entries for this table/column
    std::string prefix =
        "ftidx:" + std::string(table) + ":" + std::string(column) + ":";

    std::unordered_map<std::string, double> pkScores;

    db_.scanPrefix(prefix, [&](std::string_view key, std::string_view) {
        // Key format: ftidx:table:column:token:pk
        // The prefix already contains "ftidx:table:column:", so the remaining
        // part is "token:pk".  Use rfind to locate the last ':' which is the
        // separator between token and pk (tokens never contain ':' because the
        // tokeniser splits on punctuation).
        size_t lastColon = key.rfind(':');
        if (lastColon == std::string_view::npos || lastColon <= prefix.size())
            return true;

        std::string tok(key.substr(prefix.size(), lastColon - prefix.size()));
        std::string pk(key.substr(lastColon + 1));
        if (tok.empty() || pk.empty()) {
          return true;
        }

        for (const auto& qt : queryTokens) {
            int dist = levenshtein(qt, tok);
            if (dist <= maxDistance) {
                double score = 1.0 / (1.0 + dist);
                auto it = pkScores.find(pk);
                if (it == pkScores.end())
                    pkScores[pk] = score;
                else
                    it->second = std::max(it->second, score);
            }
        }
        return true;
    });

    std::vector<SearchResult> results = {};

    results.reserve(pkScores.size());
    for (auto& [pk, score] : pkScores)
        results.push_back({pk, score});

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    if (results.size() > limit) {
      results.resize(limit);
    }
    return {Status::OK(), std::move(results)};
}

} // namespace themis


