/**
 * @file wiki_index_store.cpp
 * @brief WikiIndexStore — BM25+, RRF fusion, HNSW stub, persistent cache stub.
 * @version 0.1.0
 * @note Maturity: 🟡 PARTIAL — BM25+ and RRF are production-ready;
 *       HNSW and RocksDB persistent cache are architectural stubs (Wave B).
 */

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE — HNSW vector index backend
// ─────────────────────────────────────────────────────────────────────────────
// Purpose:    Approximate nearest-neighbour search over dense embedding vectors
//             for semantic retrieval in the WikiIndexStore.
// Activation: Enabled when THEMIS_HNSW_BACKEND is defined and a RocksDB
//             column family "hnsw_vectors" is available.
// Production Delta:
//   - Wire hnswlib or faiss HNSW implementation against the embedding column.
//   - Implement upsert / delete / snapshot operations.
//   - Add WAL-backed index persistence.
// Removal Plan: Replace this note with real wiring in Q4 2026 (Wave B RocksDB
//               integration sprint, tracked in ROADMAP.md §Wave-B).
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE — RocksDB persistent embedding cache
// ─────────────────────────────────────────────────────────────────────────────
// Purpose:    Cache dense embeddings keyed by doc_id to avoid re-encoding on
//             restart, using a dedicated RocksDB column family.
// Activation: Enabled when THEMIS_ROCKSDB_CACHE is defined and a valid DB path
//             is provided via WikiIndexStore::Config::cache_db_path.
// Production Delta:
//   - Column family schema: key = SHA-256(doc_id + model_id),
//     value = float32[] (little-endian).
//   - Implement LRU eviction using a TTL compaction filter.
//   - Add prometheus counter for cache hit/miss rate.
// Removal Plan: Replace this note with real wiring in Q4 2026 alongside the
//               HNSW backend (ROADMAP.md §Wave-B).
// ─────────────────────────────────────────────────────────────────────────────

#include "rag/wiki_index_store.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Lowercase-tokenise a string on whitespace boundaries.
std::vector<std::string> tokenise(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream ss(text);
    std::string word;
    while (ss >> word) {
        std::string lower;
        lower.reserve(word.size());
        for (unsigned char c : word) {
            lower += static_cast<char>(std::tolower(c));
        }
        tokens.push_back(std::move(lower));
    }
    return tokens;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM25+ scoring — production implementation
// ─────────────────────────────────────────────────────────────────────────────

float bm25PlusScore(
    const std::vector<std::string>&               query_terms,
    const std::string&                            doc_text,
    float                                         avg_doc_len,
    const std::unordered_map<std::string, float>& idf_map)
{
    constexpr float k1    = 1.5f;
    constexpr float b     = 0.75f;
    constexpr float delta = 1.0f;

    // Tokenise document and compute term-frequency map.
    const auto doc_tokens = tokenise(doc_text);
    const float dl = static_cast<float>(doc_tokens.size());

    std::unordered_map<std::string, float> tf_map;
    tf_map.reserve(doc_tokens.size());
    for (const auto& tok : doc_tokens) {
        tf_map[tok] += 1.0f;
    }

    const float norm = (avg_doc_len > 0.0f) ? dl / avg_doc_len : 1.0f;

    float score = 0.0f;
    for (const auto& term : query_terms) {
        auto idf_it = idf_map.find(term);
        if (idf_it == idf_map.end() || idf_it->second <= 0.0f) {
            continue; // Term not in corpus or zero IDF.
        }
        const float idf = idf_it->second;

        float tf = 0.0f;
        auto tf_it = tf_map.find(term);
        if (tf_it != tf_map.end()) {
            tf = tf_it->second;
        }

        // BM25+ numerator / denominator
        const float numerator   = tf * (k1 + 1.0f);
        const float denominator = tf + k1 * (1.0f - b + b * norm);
        const float bm25_term   = (denominator > 0.0f)
                                      ? (numerator / denominator)
                                      : 0.0f;

        score += idf * (bm25_term + delta);
    }
    return score;
}

// ─────────────────────────────────────────────────────────────────────────────
// RRF fusion — production implementation
// ─────────────────────────────────────────────────────────────────────────────

std::vector<IndexResult> rrfFusion(
    const std::vector<std::vector<std::string>>& ranked_lists,
    int k)
{
    if (k <= 0) {
        throw std::invalid_argument("rrfFusion: k must be > 0");
    }

    std::unordered_map<std::string, float> rrf_scores;

    for (const auto& list : ranked_lists) {
        int rank = 1; // 1-based rank within this list.
        for (const auto& doc_id : list) {
            rrf_scores[doc_id] += 1.0f / static_cast<float>(k + rank);
            ++rank;
        }
    }

    std::vector<IndexResult> results;
    results.reserve(rrf_scores.size());
    for (auto& [doc_id, score] : rrf_scores) {
        results.push_back(IndexResult{doc_id, score});
    }

    // Sort descending by RRF score, then ascending by doc_id for stable ties.
    std::sort(results.begin(), results.end(),
              [](const IndexResult& a, const IndexResult& b) {
                  if (a.score != b.score) return a.score > b.score;
                  return a.doc_id < b.doc_id;
              });

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::Impl
// ─────────────────────────────────────────────────────────────────────────────

struct WikiIndexStore::Impl {
    Config config;

    // Inverted index: doc_id → raw text (for on-the-fly BM25+ scoring).
    mutable std::mutex                          idx_mutex;
    std::unordered_map<std::string, std::string> docs; // Thread-safety: protected by idx_mutex (Wave 5)

    // Corpus-level IDF cache; rebuilt on addDocument.
    // Thread-safety: protected by idx_mutex (Wave 5)
    std::unordered_map<std::string, float>      idf_cache;

    explicit Impl(Config cfg) : config(std::move(cfg)) {}

    /// Rebuild IDF from the current document set (call under idx_mutex held).
    void rebuildIDF() {
        // df_map: term → number of documents containing the term.
        std::unordered_map<std::string, int> df_map;
        for (const auto& [id, text] : docs) {
            auto tokens = tokenise(text);
            // Unique tokens per document for DF counting.
            std::unordered_map<std::string, bool> seen;
            for (const auto& tok : tokens) {
                if (!seen[tok]) {
                    seen[tok] = true;
                    df_map[tok]++;
                }
            }
        }

        const float N = static_cast<float>(docs.size());
        idf_cache.clear();
        for (const auto& [term, df] : df_map) {
            // BM25 IDF: log((N - df + 0.5) / (df + 0.5) + 1)
            idf_cache[term] = std::log(
                (N - static_cast<float>(df) + 0.5f) /
                (static_cast<float>(df) + 0.5f) + 1.0f);
        }
    }

    /// Compute corpus average document length (call under idx_mutex held).
    float computeAvgDocLen() const {
        if (docs.empty()) return config.avg_doc_len;
        float total = 0.0f;
        for (const auto& [id, text] : docs) {
            total += static_cast<float>(tokenise(text).size());
        }
        return total / static_cast<float>(docs.size());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — public interface
// ─────────────────────────────────────────────────────────────────────────────

WikiIndexStore::WikiIndexStore(Config cfg)
    : impl_(std::make_unique<Impl>(std::move(cfg)))
{}

WikiIndexStore::~WikiIndexStore() = default;

void WikiIndexStore::addDocument(const std::string& doc_id,
                                 const std::string& text) {
    if (doc_id.empty()) {
        THEMIS_WARN("WikiIndexStore::addDocument: empty doc_id ignored");
        return;
    }
    std::lock_guard<std::mutex> lk(impl_->idx_mutex); // Thread-safety: protected by idx_mutex (Wave 5)
    impl_->docs[doc_id] = text;
    impl_->rebuildIDF();
    THEMIS_DEBUG("WikiIndexStore: indexed doc '{}' ({} total)", doc_id, impl_->docs.size());
}

std::vector<IndexResult> WikiIndexStore::searchBM25(
    const std::vector<std::string>& query_terms,
    size_t top_k) const
{
    std::lock_guard<std::mutex> lk(impl_->idx_mutex); // Thread-safety: protected by idx_mutex (Wave 5)

    const float avg_len = impl_->computeAvgDocLen();
    std::vector<IndexResult> results;
    results.reserve(impl_->docs.size());

    for (const auto& [doc_id, text] : impl_->docs) {
        const float score = bm25PlusScore(query_terms, text, avg_len, impl_->idf_cache);
        results.push_back(IndexResult{doc_id, score});
    }

    // Partial sort: only the top_k highest scores needed.
    const size_t k = std::min(top_k, results.size());
    std::partial_sort(results.begin(),
                      results.begin() + static_cast<std::ptrdiff_t>(k),
                      results.end(),
                      [](const IndexResult& a, const IndexResult& b) {
                          return a.score > b.score;
                      });
    results.resize(k);
    return results;
}

std::vector<IndexResult> WikiIndexStore::fuseRRF(
    const std::vector<std::vector<std::string>>& ranked_lists) const
{
    return rrfFusion(ranked_lists, impl_->config.rrf_k);
}

void WikiIndexStore::clear() {
    std::lock_guard<std::mutex> lk(impl_->idx_mutex); // Thread-safety: protected by idx_mutex (Wave 5)
    impl_->docs.clear();
    impl_->idf_cache.clear();
}

size_t WikiIndexStore::size() const {
    std::lock_guard<std::mutex> lk(impl_->idx_mutex); // Thread-safety: protected by idx_mutex (Wave 5)
    return impl_->docs.size();
}

} // namespace themis::rag
