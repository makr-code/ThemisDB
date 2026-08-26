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
#include <list>
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

/// Lowercase-tokenise a string on whitespace+punctuation boundaries.
/// Strips leading/trailing punctuation from each word so that "hello,"
/// and "hello" index identically, keeping tokenisation consistent between
/// addDocument and query paths.
std::vector<std::string> tokenise(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream ss(text);
    std::string word;
    while (ss >> word) {
        std::string lower;
        lower.reserve(word.size());
        for (unsigned char c : word) {
            if (std::isalnum(c) || c == '\'') {
                lower += static_cast<char>(std::tolower(c));
            } else {
                lower += ' '; // Treat punctuation as a separator.
            }
        }
        // Split on internal spaces introduced by punctuation above.
        std::istringstream inner(lower);
        std::string part;
        while (inner >> part) {
            if (!part.empty()) {
                tokens.push_back(std::move(part));
            }
        }
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
    constexpr float delta = 0.5f;

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

    // Positional index: term → doc_id → sorted list of 0-based token positions.
    // Thread-safety: protected by idx_mutex (same lock as docs/idf_cache, Wave 7).
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<size_t>>> positional_index_;

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

    /// Rebuild the positional index from the current document set
    /// (call under idx_mutex held).
    void rebuildPositionalIndex() {
        positional_index_.clear();
        for (const auto& [doc_id, text] : docs) {
            const auto tokens = tokenise(text);
            for (size_t pos = 0; pos < tokens.size(); ++pos) {
                positional_index_[tokens[pos]][doc_id].push_back(pos);
            }
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

    // ─── [W8-18] HNSW injection bridge (in-memory fallback) ──────────────
    // STUB/SIMULATION NOTE:
    // Purpose:    Approximate nearest-neighbour search over dense embeddings.
    //             This in-memory cosine-similarity exhaustive scan is the
    //             injection-bridge placeholder until hnswlib/FAISS is wired.
    // Activation: config.enable_hnsw == true (per-instance config flag).
    // Production Delta:
    //   - Replace exhaustive scan with hnswlib HierarchicalNSW or FAISS HNSW.
    //   - Add WAL-backed index snapshot for persistence across restarts.
    //   - Support upsert and delete operations on the live index.
    // Removal Plan: Wire real HNSW library in Q4 2026 (ROADMAP.md §Wave-B).

    /// Stored embeddings: doc_id → unit-norm float vector.
    std::unordered_map<std::string, std::vector<float>> hnsw_vectors;

    /// Dimension of stored vectors; 0 = not yet set.
    size_t hnsw_dim{0};

    /// Cosine similarity: inner product of two unit-norm vectors.
    static float cosineSim(const std::vector<float>& a,
                            const std::vector<float>& b) {
        float dot = 0.0f;
        const size_t n = a.size();
        for (size_t i = 0; i < n; ++i) dot += a[i] * b[i];
        return dot;
    }

    /// Return a unit-norm copy of @p v (safe: returns @p v if near-zero norm).
    static std::vector<float> unitNorm(const std::vector<float>& v) {
        float norm = 0.0f;
        for (float x : v) norm += x * x;
        norm = std::sqrt(norm);
        if (norm < 1e-9f) return v;
        std::vector<float> out;
        out.reserve(v.size());
        for (float x : v) out.push_back(x / norm);
        return out;
    }

    // ─── [W8-19] In-memory embedding cache bridge ────────────────────────
    // STUB/SIMULATION NOTE:
    // Purpose:    Avoid re-encoding documents on restart by caching dense
    //             embeddings keyed by doc_id (or content hash).
    // Activation: config.cache_dir non-empty (RocksDB persistence) OR
    //             always active as in-memory LRU when max_cache_size > 0.
    // Production Delta:
    //   - Replace in-memory map with RocksDB column family writes
    //     (key = SHA-256(doc_id + model_id), value = float32[] LE).
    //   - Add TTL compaction filter for expired entries.
    //   - Add prometheus counters for hit/miss rate.
    // Removal Plan: Wire RocksDB CF in Q4 2026 alongside HNSW (ROADMAP §Wave-B).

    /// LRU-ordered list of cache keys (front = most recent).
    std::list<std::string> cache_lru;
    /// Embedding cache: doc_id → embedding vector (unit-norm).
    std::unordered_map<std::string, std::vector<float>> embedding_cache;

    /// Insert into LRU cache with eviction if above max_cache_size.
    void cacheInsert(const std::string& key, std::vector<float> emb) {
        // Remove existing entry from LRU order if present.
        auto it = embedding_cache.find(key);
        if (it != embedding_cache.end()) {
            cache_lru.remove(key);
        }
        // Evict LRU entry if at capacity.
        if (config.max_cache_size > 0 &&
            embedding_cache.size() >= config.max_cache_size) {
            embedding_cache.erase(cache_lru.back());
            cache_lru.pop_back();
        }
        cache_lru.push_front(key);
        embedding_cache[key] = std::move(emb);
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
    impl_->rebuildPositionalIndex();
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

// ─────────────────────────────────────────────────────────────────────────────
// BM25+ Positional scorer — production implementation (Wave 7)
// ─────────────────────────────────────────────────────────────────────────────

/// @internal Check whether all query_terms appear within a sliding window of
/// @p window_size tokens anywhere in the positional index entry for @p doc_id.
static bool termsWithinWindow(
    const std::vector<std::string>& query_terms,
    const std::string&              doc_id,
    const std::unordered_map<std::string,
          std::unordered_map<std::string, std::vector<size_t>>>& pos_idx,
    size_t window_size)
{
    // Gather the positions for each term in this document.
    std::vector<const std::vector<size_t>*> term_pos_lists;
    term_pos_lists.reserve(query_terms.size());
    for (const auto& term : query_terms) {
        auto it_term = pos_idx.find(term);
        if (it_term == pos_idx.end()) return false;
        auto it_doc = it_term->second.find(doc_id);
        if (it_doc == it_term->second.end()) return false;
        term_pos_lists.push_back(&it_doc->second);
    }

    // Sweep anchor positions of the first term and test whether every other
    // term has at least one position inside [anchor, anchor + window_size).
    for (size_t anchor : *term_pos_lists[0]) {
        bool all_in_window = true;
        for (size_t ti = 1; ti < term_pos_lists.size(); ++ti) {
            bool found = false;
            for (size_t p : *term_pos_lists[ti]) {
                if (p >= anchor && p < anchor + window_size) {
                    found = true;
                    break;
                }
            }
            if (!found) { all_in_window = false; break; }
        }
        if (all_in_window) return true;
    }
    return false;
}

/// @brief BM25+ with positional proximity bonus.
///
/// Standard BM25+ score multiplied by 1.5 when all query_terms co-occur
/// within a window of @p window_size tokens in the document.
static float computePositionalBM25Score(
    const std::vector<std::string>&               query_terms,
    const std::string&                            doc_id,
    const std::string&                            doc_text,
    float                                         avg_len,
    const std::unordered_map<std::string, float>& idf_cache,
    const std::unordered_map<std::string,
          std::unordered_map<std::string, std::vector<size_t>>>& pos_idx,
    size_t window_size = 8)
{
    float score = bm25PlusScore(query_terms, doc_text, avg_len, idf_cache);
    if (query_terms.size() > 1 &&
        termsWithinWindow(query_terms, doc_id, pos_idx, window_size)) {
        score *= 1.5f;
    }
    return score;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::searchPhrase — phrase query (Wave 7)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<IndexResult> WikiIndexStore::searchPhrase(
    const std::string& phrase,
    size_t top_k) const
{
    if (phrase.empty()) {
        THEMIS_WARN("WikiIndexStore::searchPhrase: empty phrase, returning empty");
        return {};
    }

    const auto phrase_terms = tokenise(phrase);
    if (phrase_terms.empty()) return {};

    // Single-term phrase → delegate to standard BM25.
    if (phrase_terms.size() == 1) {
        return searchBM25(phrase_terms, top_k);
    }

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    // Collect candidate docs: those containing ALL phrase terms.
    // Start from the first term's posting list and intersect.
    auto it_first = impl_->positional_index_.find(phrase_terms[0]);
    if (it_first == impl_->positional_index_.end()) return {};

    std::vector<std::string> candidates;
    candidates.reserve(it_first->second.size());
    for (const auto& [doc_id, _] : it_first->second) {
        candidates.push_back(doc_id);
    }

    for (size_t ti = 1; ti < phrase_terms.size(); ++ti) {
        auto it = impl_->positional_index_.find(phrase_terms[ti]);
        if (it == impl_->positional_index_.end()) return {};
        const auto& posting = it->second;
        candidates.erase(
            std::remove_if(candidates.begin(), candidates.end(),
                [&posting](const std::string& d) {
                    return posting.find(d) == posting.end();
                }),
            candidates.end());
        if (candidates.empty()) return {};
    }

    // Filter by consecutive-position constraint.
    const float avg_len = impl_->computeAvgDocLen();
    std::vector<IndexResult> results;
    results.reserve(candidates.size());

    for (const auto& doc_id : candidates) {
        // Check positions of phrase_terms[0] in this doc.
        const auto& pos0 = impl_->positional_index_.at(phrase_terms[0]).at(doc_id);
        bool phrase_found = false;
        for (size_t anchor : pos0) {
            bool consecutive = true;
            for (size_t ti = 1; ti < phrase_terms.size(); ++ti) {
                const auto& pos_ti =
                    impl_->positional_index_.at(phrase_terms[ti]).at(doc_id);
                size_t expected = anchor + ti;
                bool has_pos = std::binary_search(pos_ti.begin(), pos_ti.end(), expected);
                if (!has_pos) { consecutive = false; break; }
            }
            if (consecutive) { phrase_found = true; break; }
        }
        if (!phrase_found) continue;

        const float score = bm25PlusScore(
            phrase_terms, impl_->docs.at(doc_id), avg_len, impl_->idf_cache);
        results.push_back(IndexResult{doc_id, score});
    }

    const size_t k = std::min(top_k, results.size());
    std::partial_sort(results.begin(),
                      results.begin() + static_cast<std::ptrdiff_t>(k),
                      results.end(),
                      [](const IndexResult& a, const IndexResult& b) {
                          return a.score > b.score;
                      });
    results.resize(k);
    THEMIS_INFO("WikiIndexStore::searchPhrase: '{}' → {} result(s)", phrase, results.size());
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::searchProximity — proximity query (Wave 7)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<IndexResult> WikiIndexStore::searchProximity(
    const std::string& term1,
    const std::string& term2,
    size_t distance,
    size_t top_k) const
{
    if (term1.empty() || term2.empty()) {
        THEMIS_WARN("WikiIndexStore::searchProximity: empty term(s), returning empty");
        return {};
    }

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    auto it1 = impl_->positional_index_.find(term1);
    auto it2 = impl_->positional_index_.find(term2);
    if (it1 == impl_->positional_index_.end() ||
        it2 == impl_->positional_index_.end()) {
        THEMIS_WARN("WikiIndexStore::searchProximity: term '{}' or '{}' not in index",
                    term1, term2);
        return {};
    }

    const auto& posting1 = it1->second;
    const auto& posting2 = it2->second;

    const float avg_len = impl_->computeAvgDocLen();
    std::vector<IndexResult> results;

    for (const auto& [doc_id, pos_list1] : posting1) {
        auto it_doc2 = posting2.find(doc_id);
        if (it_doc2 == posting2.end()) continue;
        const auto& pos_list2 = it_doc2->second;

        // Find minimum distance between any pair of positions.
        bool within = false;
        if (term1 == term2) {
            // Same term: need ≥2 positions within distance of each other.
            for (size_t i = 0; i + 1 < pos_list1.size() && !within; ++i) {
                if (pos_list1[i + 1] - pos_list1[i] <= distance) within = true;
            }
        } else {
            // Two-pointer scan (both lists are sorted).
            size_t i = 0, j = 0;
            while (i < pos_list1.size() && j < pos_list2.size() && !within) {
                size_t p1 = pos_list1[i];
                size_t p2 = pos_list2[j];
                size_t d  = (p1 <= p2) ? (p2 - p1) : (p1 - p2);
                if (d <= distance) { within = true; }
                else if (p1 < p2) { ++i; } else { ++j; }
            }
        }
        if (!within) continue;

        const std::vector<std::string> query_terms{term1, term2};
        const float score = computePositionalBM25Score(
            query_terms, doc_id, impl_->docs.at(doc_id),
            avg_len, impl_->idf_cache, impl_->positional_index_);
        results.push_back(IndexResult{doc_id, score});
    }

    const size_t k = std::min(top_k, results.size());
    std::partial_sort(results.begin(),
                      results.begin() + static_cast<std::ptrdiff_t>(k),
                      results.end(),
                      [](const IndexResult& a, const IndexResult& b) {
                          return a.score > b.score;
                      });
    results.resize(k);
    THEMIS_INFO("WikiIndexStore::searchProximity: '{}'~'{}' dist={} → {} result(s)",
                term1, term2, distance, results.size());
    return results;
}

std::vector<IndexResult> WikiIndexStore::fuseRRF(
    const std::vector<std::vector<std::string>>& ranked_lists) const
{
    return rrfFusion(ranked_lists, impl_->config.rrf_k);
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::addVector — [W8-18] HNSW injection bridge
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::addVector(const std::string& doc_id,
                                const std::vector<float>& embedding)
{
    if (embedding.empty()) {
        throw std::invalid_argument(
            "WikiIndexStore::addVector: embedding must not be empty");
    }
    if (!impl_->config.enable_hnsw) {
        THEMIS_WARN("WikiIndexStore::addVector: HNSW is disabled (enable_hnsw=false); "
                    "ignoring vector for '{}'", doc_id);
        return;
    }

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    // Enforce consistent dimensionality after the first insertion.
    if (impl_->hnsw_dim == 0) {
        impl_->hnsw_dim = embedding.size();
    } else if (embedding.size() != impl_->hnsw_dim) {
        throw std::invalid_argument(
            "WikiIndexStore::addVector: dimension mismatch — expected " +
            std::to_string(impl_->hnsw_dim) + ", got " +
            std::to_string(embedding.size()));
    }

    impl_->hnsw_vectors[doc_id] = Impl::unitNorm(embedding);
    THEMIS_DEBUG("WikiIndexStore::addVector: stored dim={} vector for '{}'",
                 impl_->hnsw_dim, doc_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::searchHNSW — [W8-18] exhaustive cosine scan (bridge impl)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<IndexResult> WikiIndexStore::searchHNSW(
    const std::vector<float>& query_embedding,
    size_t top_k) const
{
    if (query_embedding.empty()) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: empty query embedding");
        return {};
    }
    if (!impl_->config.enable_hnsw || impl_->hnsw_vectors.empty()) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: HNSW disabled or no vectors indexed");
        return {};
    }

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    if (query_embedding.size() != impl_->hnsw_dim) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: query dim={} != index dim={}; "
                    "returning empty", query_embedding.size(), impl_->hnsw_dim);
        return {};
    }

    const auto q_unit = Impl::unitNorm(query_embedding);

    std::vector<IndexResult> results;
    results.reserve(impl_->hnsw_vectors.size());
    for (const auto& [id, vec] : impl_->hnsw_vectors) {
        results.push_back(IndexResult{id, Impl::cosineSim(q_unit, vec)});
    }

    const size_t k = std::min(top_k, results.size());
    std::partial_sort(results.begin(),
                      results.begin() + static_cast<std::ptrdiff_t>(k),
                      results.end(),
                      [](const IndexResult& a, const IndexResult& b) {
                          return a.score > b.score;
                      });
    results.resize(k);
    THEMIS_INFO("WikiIndexStore::searchHNSW: top_k={} → {} result(s)", top_k, results.size());
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::cacheEmbedding / retrieveEmbedding — [W8-19]
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::cacheEmbedding(const std::string& key,
                                     const std::vector<float>& embedding)
{
    if (key.empty() || embedding.empty()) return;
    if (impl_->config.max_cache_size == 0) return; // cache disabled

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);
    impl_->cacheInsert(key, Impl::unitNorm(embedding));
    THEMIS_DEBUG("WikiIndexStore::cacheEmbedding: stored embedding for '{}'", key);
}

std::vector<float> WikiIndexStore::retrieveEmbedding(const std::string& key) const
{
    if (key.empty()) return {};

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);
    auto it = impl_->embedding_cache.find(key);
    if (it == impl_->embedding_cache.end()) {
        THEMIS_DEBUG("WikiIndexStore::retrieveEmbedding: cache miss for '{}'", key);
        return {};
    }
    // Promote to front (LRU).
    impl_->cache_lru.remove(key);
    impl_->cache_lru.push_front(key);
    THEMIS_DEBUG("WikiIndexStore::retrieveEmbedding: cache hit for '{}'", key);
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::searchHybrid — [W8-21] BM25+ ⊕ HNSW fused with RRF
// ─────────────────────────────────────────────────────────────────────────────

std::vector<IndexResult> WikiIndexStore::searchHybrid(
    const std::vector<std::string>& query_terms,
    const std::vector<float>&       query_embedding,
    size_t                          top_k) const
{
    // BM25+ lexical list (always available).
    const auto bm25_results = searchBM25(query_terms, top_k * 2);

    std::vector<std::string> bm25_ids;
    bm25_ids.reserve(bm25_results.size());
    for (const auto& r : bm25_results) bm25_ids.push_back(r.doc_id);

    // HNSW semantic list (only when backend is enabled and query has a vector).
    if (!impl_->config.enable_hnsw || query_embedding.empty()) {
        // Hybrid degrades gracefully to pure BM25+ when HNSW is unavailable.
        THEMIS_DEBUG("WikiIndexStore::searchHybrid: HNSW disabled/no-embedding — "
                     "falling back to BM25+");
        auto results = bm25_results;
        if (results.size() > top_k) results.resize(top_k);
        return results;
    }

    const auto hnsw_results = searchHNSW(query_embedding, top_k * 2);

    std::vector<std::string> hnsw_ids;
    hnsw_ids.reserve(hnsw_results.size());
    for (const auto& r : hnsw_results) hnsw_ids.push_back(r.doc_id);

    // Fuse both ranked lists with RRF.
    auto fused = fuseRRF({bm25_ids, hnsw_ids});

    if (fused.size() > top_k) fused.resize(top_k);
    THEMIS_INFO("WikiIndexStore::searchHybrid: bm25={} hnsw={} fused={}",
                bm25_ids.size(), hnsw_ids.size(), fused.size());
    return fused;
}

void WikiIndexStore::clear() {
    std::lock_guard<std::mutex> lk(impl_->idx_mutex); // Thread-safety: protected by idx_mutex (Wave 5)
    impl_->docs.clear();
    impl_->idf_cache.clear();
    impl_->positional_index_.clear();
}

size_t WikiIndexStore::size() const {
    std::lock_guard<std::mutex> lk(impl_->idx_mutex); // Thread-safety: protected by idx_mutex (Wave 5)
    return impl_->docs.size();
}

} // namespace themis::rag
