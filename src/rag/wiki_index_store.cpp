/**
 * @file wiki_index_store.cpp
 * @brief WikiIndexStore — BM25+, RRF fusion, HNSW (hnswlib when available),
 *        and RocksDB-backed persistent embedding cache.
 * @version 0.2.0
 * @note Maturity: 🟢 PRODUCTION-READY (BM25+, RRF, HNSW, cache all wired)
 */

// HNSW backend — wired against hnswlib when THEMIS_HNSW_ENABLED is set by the
// build system (cmake/CMakeLists.txt detects hnswlib via find_package).
// Falls back to exhaustive cosine scan otherwise.
#ifdef THEMIS_HNSW_ENABLED
#include <hnswlib/hnswlib.h>
#endif

// RocksDB persistence for embedding cache — wired when rocksdb headers are
// available (always present in community/enterprise builds).
#ifdef THEMIS_ROCKSDB_AVAILABLE
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#endif

#include "rag/wiki_index_store.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <openssl/evp.h>
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

    // ─── [W8-18] HNSW backend — hnswlib wired under THEMIS_HNSW_ENABLED ──────
    //
    // When THEMIS_HNSW_ENABLED is set by the build system (hnswlib found via
    // find_package), this section owns a real hnswlib::HierarchicalNSW<float>
    // index using InnerProductSpace (cosine after unit-norm).
    //
    // When THEMIS_HNSW_ENABLED is NOT set (hnswlib not installed), the Impl
    // falls back to the exhaustive cosine scan using hnsw_vectors_fallback.
    // The fallback is sufficient for development/test loads.

#ifdef THEMIS_HNSW_ENABLED
    std::unique_ptr<hnswlib::InnerProductSpace>       hnsw_space;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>>  hnsw_index;
    /// label → internal hnswlib label (== sequential insertion order).
    std::unordered_map<std::string, size_t>           hnsw_label_map;
    /// reverse map: label → doc_id.
    std::unordered_map<size_t, std::string>           hnsw_id_map;
    size_t                                            hnsw_next_id{0};
#else
    /// Fallback: stored embeddings doc_id → unit-norm float vector.
    std::unordered_map<std::string, std::vector<float>> hnsw_vectors_fallback;
#endif

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

    // ─── [W8-19] RocksDB-backed + in-memory LRU embedding cache ─────────────
    //
    // When config.cache_dir is non-empty AND THEMIS_ROCKSDB_AVAILABLE is set,
    // embeddings are persisted to a dedicated RocksDB instance (column family
    // "embedding_cache").  Key = SHA-256(input), value = raw float32[] LE.
    //
    // The in-memory LRU (cache_lru / embedding_cache) operates in all cases as
    // a write-through / read-through cache layer in front of RocksDB.

    /// LRU-ordered list of cache keys (front = most recent).
    std::list<std::string> cache_lru;
    /// In-memory embedding cache: sha256key → unit-norm vector.
    std::unordered_map<std::string, std::vector<float>> embedding_cache;

#ifdef THEMIS_ROCKSDB_AVAILABLE
    /// RocksDB instance for persistent embedding cache (nullptr when disabled).
    rocksdb::DB*                   cache_db{nullptr};
    rocksdb::ColumnFamilyHandle*   cache_cf{nullptr};

    /// Open (or create) the RocksDB embedding cache at config.cache_dir.
    /// Called lazily on first cacheEmbedding() call with a non-empty cache_dir.
    bool openCacheDB() {
        if (cache_db != nullptr) return true;
        if (config.cache_dir.empty()) return false;

        rocksdb::Options opts;
        opts.create_if_missing                = true;
        opts.create_missing_column_families   = true;

        std::vector<rocksdb::ColumnFamilyDescriptor> cf_descs{
            {rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions{}},
            {"embedding_cache",                 rocksdb::ColumnFamilyOptions{}}
        };
        std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
        rocksdb::DB* raw_db_instance = nullptr;
        const rocksdb::Status s = rocksdb::DB::Open(
            opts, config.cache_dir, cf_descs, &cf_handles, &raw_db_instance);
        if (!s.ok()) {
            THEMIS_WARN("WikiIndexStore: failed to open RocksDB cache at '{}': {}",
                        config.cache_dir, s.ToString());
            return false;
        }
        cache_db = raw_db_instance;
        // cf_handles[0] = default CF (not used); cf_handles[1] = embedding_cache.
        if (cf_handles.size() >= 2) {
            cache_cf = cf_handles[1];
            // Default CF handle: close immediately (we don't need it).
            delete cf_handles[0];
        }
        return cache_cf != nullptr;
    }

    void closeCacheDB() {
        if (cache_cf) { delete cache_cf; cache_cf = nullptr; }
        if (cache_db) { delete cache_db; cache_db = nullptr; }
    }
#endif // THEMIS_ROCKSDB_AVAILABLE

    /// Compute SHA-256 hex of @p input using the EVP API (OpenSSL 3.x compatible).
    static std::string sha256Hex(const std::string& input) {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int  digest_len = 0;
        if (EVP_Digest(input.data(), input.size(),
                       digest, &digest_len,
                       EVP_sha256(), nullptr) != 1) {
            return std::string(64, '0'); // unreachable in practice
        }
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (unsigned int i = 0; i < digest_len; ++i) {
            oss << std::setw(2) << static_cast<int>(digest[i]);
        }
        return oss.str();
    }

    /// Insert into in-memory LRU cache with eviction if above max_cache_size.
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

    ~Impl() {
#ifdef THEMIS_ROCKSDB_AVAILABLE
        closeCacheDB();
#endif
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — public interface
// ─────────────────────────────────────────────────────────────────────────────

WikiIndexStore::WikiIndexStore(Config cfg)
    : impl_(std::make_unique<Impl>(std::move(cfg)))
{}

WikiIndexStore::~WikiIndexStore() = default;

WikiIndexStore::WikiIndexStore(WikiIndexStore&&) noexcept = default;

WikiIndexStore& WikiIndexStore::operator=(WikiIndexStore&&) noexcept = default;

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
// WikiIndexStore::addVector — [W8-18] hnswlib wiring + exhaustive fallback
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

    const auto unit = Impl::unitNorm(embedding);

#ifdef THEMIS_HNSW_ENABLED
    // Initialise the hnswlib index on first insertion.
    if (!impl_->hnsw_space) {
        impl_->hnsw_space = std::make_unique<hnswlib::InnerProductSpace>(impl_->hnsw_dim);
        // Reserve space for at least 1024 elements; the index auto-resizes.
        impl_->hnsw_index = std::make_unique<hnswlib::HierarchicalNSW<float>>(
            impl_->hnsw_space.get(),
            /*max_elements=*/1024,
            static_cast<size_t>(impl_->config.hnsw_m),
            static_cast<size_t>(impl_->config.hnsw_ef_construction));
        impl_->hnsw_index->ef_ = static_cast<int>(impl_->config.hnsw_ef);
    }

    // Resize the internal index if needed (hnswlib requires explicit reserve).
    if (impl_->hnsw_index->getCurrentCount() + 1 >
        impl_->hnsw_index->maxelements_) {
        impl_->hnsw_index->resizeIndex(
            impl_->hnsw_index->maxelements_ * 2);
    }

    const size_t label = impl_->hnsw_next_id++;
    impl_->hnsw_index->addPoint(unit.data(), label);
    impl_->hnsw_label_map[doc_id] = label;
    impl_->hnsw_id_map[label]    = doc_id;
    THEMIS_DEBUG("WikiIndexStore::addVector[hnswlib]: stored dim={} vec for '{}' (label={})",
                 impl_->hnsw_dim, doc_id, label);
#else
    impl_->hnsw_vectors_fallback[doc_id] = unit;
    THEMIS_DEBUG("WikiIndexStore::addVector[fallback]: stored dim={} vec for '{}'",
                 impl_->hnsw_dim, doc_id);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::searchHNSW — [W8-18] hnswlib ANN + exhaustive fallback
// ─────────────────────────────────────────────────────────────────────────────

std::vector<IndexResult> WikiIndexStore::searchHNSW(
    const std::vector<float>& query_embedding,
    size_t top_k) const
{
    if (query_embedding.empty()) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: empty query embedding");
        return {};
    }

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    if (!impl_->config.enable_hnsw) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: HNSW disabled (enable_hnsw=false)");
        return {};
    }

    if (impl_->hnsw_dim == 0) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: no vectors indexed");
        return {};
    }

    if (query_embedding.size() != impl_->hnsw_dim) {
        THEMIS_WARN("WikiIndexStore::searchHNSW: query dim={} != index dim={}; returning empty",
                    query_embedding.size(), impl_->hnsw_dim);
        return {};
    }

    const auto q_unit = Impl::unitNorm(query_embedding);
    std::vector<IndexResult> results;

#ifdef THEMIS_HNSW_ENABLED
    if (!impl_->hnsw_index || impl_->hnsw_index->getCurrentCount() == 0) {
        THEMIS_WARN("WikiIndexStore::searchHNSW[hnswlib]: index empty");
        return {};
    }
    const size_t k = std::min(top_k, static_cast<size_t>(impl_->hnsw_index->getCurrentCount()));
    auto res = impl_->hnsw_index->searchKnn(q_unit.data(), k);
    results.reserve(res.size());
    while (!res.empty()) {
        auto [dist, label] = res.top(); res.pop();
        auto it = impl_->hnsw_id_map.find(label);
        if (it != impl_->hnsw_id_map.end()) {
            // hnswlib InnerProductSpace returns 1 - cos_sim as "distance".
            results.push_back(IndexResult{it->second, 1.0f - dist});
        }
    }
    // hnswlib returns in ascending distance order; reverse for descending score.
    std::sort(results.begin(), results.end(),
              [](const IndexResult& a, const IndexResult& b) {
                  return a.score > b.score;
              });
    THEMIS_INFO("WikiIndexStore::searchHNSW[hnswlib]: top_k={} → {} result(s)", top_k, results.size());
#else
    // Fallback: exhaustive cosine scan.
    if (impl_->hnsw_vectors_fallback.empty()) {
        THEMIS_WARN("WikiIndexStore::searchHNSW[fallback]: no vectors indexed");
        return {};
    }
    results.reserve(impl_->hnsw_vectors_fallback.size());
    for (const auto& [id, vec] : impl_->hnsw_vectors_fallback) {
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
    THEMIS_INFO("WikiIndexStore::searchHNSW[fallback]: top_k={} → {} result(s)", top_k, results.size());
#endif
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore::cacheEmbedding / retrieveEmbedding — [W8-19]
// RocksDB CF persistence + in-memory LRU
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::cacheEmbedding(const std::string& key,
                                     const std::vector<float>& embedding)
{
    if (key.empty() || embedding.empty()) return;
    if (impl_->config.max_cache_size == 0) return; // cache disabled

    const auto unit    = Impl::unitNorm(embedding);
    const auto db_key  = Impl::sha256Hex(key);

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    // 1. In-memory LRU insert (always).
    impl_->cacheInsert(db_key, unit);

#ifdef THEMIS_ROCKSDB_AVAILABLE
    // 2. RocksDB persistence (when cache_dir is configured).
    if (!impl_->config.cache_dir.empty() && impl_->openCacheDB()) {
        const char* raw  = reinterpret_cast<const char*>(unit.data());
        const size_t len = unit.size() * sizeof(float);
        rocksdb::WriteOptions wo;
        const auto s = impl_->cache_db->Put(
            wo, impl_->cache_cf,
            rocksdb::Slice(db_key),
            rocksdb::Slice(raw, len));
        if (!s.ok()) {
            THEMIS_WARN("WikiIndexStore::cacheEmbedding: RocksDB put failed for '{}': {}",
                        key, s.ToString());
        } else {
            THEMIS_DEBUG("WikiIndexStore::cacheEmbedding: persisted embedding for '{}'", key);
        }
    }
#endif
}

std::vector<float> WikiIndexStore::retrieveEmbedding(const std::string& key) const
{
    if (key.empty()) return {};

    const auto db_key = Impl::sha256Hex(key);

    std::lock_guard<std::mutex> lk(impl_->idx_mutex);

    // 1. Check in-memory LRU first (fastest path).
    auto it = impl_->embedding_cache.find(db_key);
    if (it != impl_->embedding_cache.end()) {
        // Promote to MRU position.
        impl_->cache_lru.remove(db_key);
        impl_->cache_lru.push_front(db_key);
        THEMIS_DEBUG("WikiIndexStore::retrieveEmbedding: LRU hit for '{}'", key);
        return it->second;
    }

#ifdef THEMIS_ROCKSDB_AVAILABLE
    // 2. Fall through to RocksDB when in-memory cache misses.
    if (!impl_->config.cache_dir.empty() &&
        const_cast<Impl*>(impl_.get())->openCacheDB()) {
        std::string raw_val;
        rocksdb::ReadOptions ro;
        const auto s = impl_->cache_db->Get(
            ro, impl_->cache_cf,
            rocksdb::Slice(db_key), &raw_val);
        if (s.ok() && (raw_val.size() % sizeof(float)) == 0) {
            const size_t n = raw_val.size() / sizeof(float);
            std::vector<float> emb(n);
            std::memcpy(emb.data(), raw_val.data(), raw_val.size());
            // Warm the in-memory LRU cache.
            const_cast<Impl*>(impl_.get())->cacheInsert(db_key, emb);
            THEMIS_DEBUG("WikiIndexStore::retrieveEmbedding: RocksDB hit for '{}'", key);
            return emb;
        }
    }
#endif

    THEMIS_DEBUG("WikiIndexStore::retrieveEmbedding: cache miss for '{}'", key);
    return {};
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
