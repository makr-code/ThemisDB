/**
 * @file wiki_index_store.cpp
 * @brief Full production implementation of WikiIndexStore and JsonWikiIndexReader.
 *
 * WikiIndexStore:
 *   - Manages a fulltext (BM25) secondary index on the `content` column and
 *     a regular secondary index on `doc_id` via SecondaryIndexManager.
 *   - Manages a COSINE HNSW vector index via VectorIndexManager.
 *   - Embeds text through EmbeddedLLM (single or batch).
 *   - Fuses BM25 and KNN candidate lists through HybridRetriever (RRF).
 *   - Thread-safe: shared_mutex guards the chunk embedding cache (exclusive
 *     for writes, shared for reads); a separate query_embed_mutex_ guards the
 *     query embedding cache used by const query() callers.
 *
 * JsonWikiIndexReader:
 *   - Reads the Python-MVP index.json format (array of chunk objects).
 *   - Performs in-memory BM25-style TF token-overlap scoring for queries.
 *   - Self-contained: no RocksDB dependency.
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "llm/wiki_index_store.h"
#include "core/concerns/adapter_signing.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <limits>
#include <list>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace themis {
namespace llm {

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::string makeCacheMaterial(const WikiChunk& chunk) {
    return chunk.doc_id + '\n' + chunk.text;
}

double applyBm25PlusFloor(double bm25_score, double delta) noexcept {
    return bm25_score + std::max(0.0, delta);
}

/// Build a SecondaryIndexManager / VectorIndexManager-ready BaseEntity from a WikiChunk.
///
/// Fields written:
///  - Primary key: chunk.chunk_id
///  - "content"        → chunk text (indexed by the BM25 fulltext index)
///  - "doc_id"         → source document identifier
///  - "section_title"  → heading of the containing section
///  - "source_path"    → originating file path
///  - "line_start" / "line_end" → 1-based line range within the source file
///  - "embedding"      → dense float vector (stored in the vector index when non-empty)
///
/// @param chunk  Source wiki chunk.
/// @return       `themis::BaseEntity` ready for `SecondaryIndexManager::put()` and
///               `VectorIndexManager::addEntity()`.
themis::BaseEntity chunkToEntity(const WikiChunk& chunk) {
    themis::BaseEntity e(chunk.chunk_id);
    e.setField("content",       chunk.text);
    e.setField("doc_id",        chunk.doc_id);
    e.setField("section_title", chunk.section_title);
    e.setField("source_path",   chunk.source_path);
    e.setField("line_start",    static_cast<int64_t>(chunk.line_start));
    e.setField("line_end",      static_cast<int64_t>(chunk.line_end));
    if (!chunk.embedding.empty()) {
        e.setField("embedding", chunk.embedding);
    }
    return e;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — constructor
// ─────────────────────────────────────────────────────────────────────────────

WikiIndexStore::WikiIndexStore(SecondaryIndexManager& sim,
                               VectorIndexManager&    vim,
                               EmbeddedLLM&           llm,
                               WikiIndexConfig        config)
    : sim_(sim)
    , vim_(vim)
    , llm_(llm)
    , config_(std::move(config))
    , llm_ptr_(&llm)
    , emb_cache_table_(config_.embedding_cache_table)
    , legacy_emb_cache_table_(config_.table_name + "_emb_cache")
{
    // Pre-allocate the latency ring buffer so query() never allocates on the
    // hot path.  kLatencyRingSize slots are inserted at construction.
    latency_ring_.resize(kLatencyRingSize, 0.0);
    if (!config_.enable_phase_b) {
        spdlog::warn("[WikiIndexStore] Phase B gate disabled (THEMIS_WIKI_PHASE_B=OFF). "
                     "Store remains inactive.");
        ready_.store(false, std::memory_order_release);
        return;
    }

    // Initialise HybridRetriever with configured RRF parameters
    rag::HybridRetrieverConfig rrf_cfg;
    rrf_cfg.use_rrf        = true;
    rrf_cfg.rrf_k          = config_.rrf_k;
    rrf_cfg.bm25_weight    = config_.enable_bm25 ? 0.5 : 0.0;
    rrf_cfg.vector_weight  = config_.enable_vector ? 0.5 : 0.0;
    rrf_cfg.top_k          = static_cast<std::size_t>(config_.top_k);
    retriever_ = rag::HybridRetriever(rrf_cfg);

    // Fulltext index on "content" column for BM25 retrieval
    if (config_.enable_bm25) {
        auto s = sim_.createFulltextIndex(config_.table_name, "content");
        if (!s.ok) {
            spdlog::warn("[WikiIndexStore] createFulltextIndex: {}", s.message);
        }
    }

    // Regular index on "doc_id" for doc-scoped lookup
    auto s2 = sim_.createIndex(config_.table_name, "doc_id");
    if (!s2.ok) {
        spdlog::warn("[WikiIndexStore] createIndex(doc_id): {}", s2.message);
    }

    // Vector index: COSINE HNSW
    if (config_.enable_vector) {
        auto s3 = vim_.init(config_.table_name,
                            config_.embedding_dim,
                            VectorIndexManager::Metric::COSINE,
                            config_.hnsw_m,
                            config_.hnsw_ef_construction,
                            config_.hnsw_ef_search);
        if (!s3.ok) {
            spdlog::warn("[WikiIndexStore] VectorIndexManager::init: {}", s3.message);
        } else {
            auto ef_s = vim_.setEfSearch(config_.hnsw_ef_search);
            if (!ef_s.ok) {
                spdlog::warn("[WikiIndexStore] setEfSearch({}): {}",
                             config_.hnsw_ef_search, ef_s.message);
            }
        }
    }

    ready_.store(true, std::memory_order_release);
    spdlog::debug("[WikiIndexStore] initialised table={} dim={}",
                  config_.table_name, config_.embedding_dim);

    // Load persisted embedding cache from RocksDB (must run after ready_ is set)
    if (config_.enable_persistent_cache) {
        auto idx_s = sim_.createIndex(emb_cache_table_, "cache_key");
        if (!idx_s.ok) {
            spdlog::debug("[WikiIndexStore] emb_cache createIndex(cache_key): {}",
                          idx_s.message);
        }
        auto idx_chunk = sim_.createIndex(emb_cache_table_, "chunk_id");
        if (!idx_chunk.ok) {
            spdlog::debug("[WikiIndexStore] emb_cache createIndex(chunk_id): {}",
                          idx_chunk.message);
        }
        if (config_.enable_phase_a_cache_migration) {
            auto legacy_idx = sim_.createIndex(legacy_emb_cache_table_, "chunk_id");
            if (!legacy_idx.ok) {
                spdlog::debug("[WikiIndexStore] legacy emb_cache createIndex(chunk_id): {}",
                              legacy_idx.message);
            }
        }
        loadPersistentEmbedCache();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// IWikiIndexWriter — writeChunk
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::writeChunk(WikiChunk chunk) {
    std::unique_lock lock(mutex_);
    if (!config_.enable_phase_b) {
        throw std::runtime_error("[WikiIndexStore] Phase B is disabled by THEMIS_WIKI_PHASE_B gate");
    }

    // Auto-probe embedding dimensionality on first write
    if (config_.auto_probe_dim) {
        probeEmbeddingDim();
    }

    // Compute embedding if missing
    if (chunk.embedding.empty()) {
        if (!tryResolveEmbeddingFromCaches(chunk, &chunk.embedding)) {
            chunk.embedding = llm_.embed(chunk.text);
            const auto cache_key = makeEmbeddingCacheKey(chunk);
            upsertEmbeddingCacheEntry(cache_key, chunk.embedding);
            persistEmbedding(cache_key, chunk.chunk_id, chunk.embedding);
        }
    }

    themis::BaseEntity entity = chunkToEntity(chunk);
    auto s1 = sim_.put(config_.table_name, entity);
    if (!s1.ok) {
        throw std::runtime_error("[WikiIndexStore] sim_.put failed: " + s1.message);
    }

    // Write to vector index (only when an embedding was stored in the entity)
    if (config_.enable_vector && !chunk.embedding.empty()) {
        auto s2 = vim_.addEntity(entity);
        if (!s2.ok) {
            spdlog::warn("[WikiIndexStore] vim_.addEntity: {}", s2.message);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// IWikiIndexWriter — writeBatch
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::writeBatch(std::vector<WikiChunk> chunks) {
    std::unique_lock lock(mutex_);
    if (!config_.enable_phase_b) {
        throw std::runtime_error("[WikiIndexStore] Phase B is disabled by THEMIS_WIKI_PHASE_B gate");
    }

    // Auto-probe embedding dimensionality on first write
    if (config_.auto_probe_dim) {
        probeEmbeddingDim();
    }

    const int effective_batch_size = std::max(1, config_.batch_size);

    // Collect indices of chunks that still need embeddings
    std::vector<std::size_t> need_embed;
    for (std::size_t i = 0; i < chunks.size(); ++i) {
        if (chunks[i].embedding.empty()) {
            if (!tryResolveEmbeddingFromCaches(chunks[i], &chunks[i].embedding)) {
                need_embed.push_back(i);
            }
        }
    }

    // Embed in configurable batches
    for (std::size_t b = 0; b < need_embed.size();
         b += static_cast<std::size_t>(effective_batch_size)) {
        std::size_t end = std::min(b + static_cast<std::size_t>(effective_batch_size),
                                   need_embed.size());
        std::vector<std::string> texts;
        texts.reserve(end - b);
        for (std::size_t k = b; k < end; ++k) {
            texts.push_back(chunks[need_embed[k]].text);
        }

        auto vecs = llm_.embedBatch(texts);
        for (std::size_t k = b; k < end; ++k) {
            std::size_t ci = need_embed[k];
            if (k - b < vecs.size()) {
                chunks[ci].embedding = vecs[k - b];
                const auto cache_key = makeEmbeddingCacheKey(chunks[ci]);
                upsertEmbeddingCacheEntry(cache_key, chunks[ci].embedding);
                persistEmbedding(cache_key, chunks[ci].chunk_id, chunks[ci].embedding);
            }
        }
    }

    // Write all chunks
    for (const auto& chunk : chunks) {
        themis::BaseEntity entity = chunkToEntity(chunk);

        auto s1 = sim_.put(config_.table_name, entity);
        if (!s1.ok) {
            throw std::runtime_error("[WikiIndexStore] writeBatch sim_.put: " + s1.message);
        }
        if (config_.enable_vector && !chunk.embedding.empty()) {
            auto s2 = vim_.addEntity(entity);
            if (!s2.ok) {
                spdlog::warn("[WikiIndexStore] writeBatch vim_.addEntity: {}", s2.message);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// IWikiIndexWriter — flush
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::flush() {
    // RocksDB WAL provides durability; explicit flush is not required.
    // Provided for API completeness and potential future override.
    spdlog::debug("[WikiIndexStore] flush() called (no-op for RocksDB WAL)");
}

// ─────────────────────────────────────────────────────────────────────────────
// IWikiIndexReader — query
// ─────────────────────────────────────────────────────────────────────────────

std::vector<WikiChunk> WikiIndexStore::query(const std::string& query_text,
                                              int                top_k,
                                              float              min_score) const {
    const auto t_start = std::chrono::steady_clock::now();

    std::shared_lock lock(mutex_);
    if (!config_.enable_phase_b) {
        spdlog::warn("[WikiIndexStore] query requested while Phase B gate is disabled");
        return {};
    }

    const int k = (top_k > 0) ? top_k : config_.top_k;

    std::vector<rag::judge::RetrievedDocument> bm25_docs;
    std::vector<rag::judge::RetrievedDocument> vec_docs;

    // ── BM25 path ──────────────────────────────────────────────────────────
    if (config_.enable_bm25) {
        auto [s, results] = sim_.scanFulltextWithScores(
            config_.table_name, "content", query_text, k * 2);
        if (s.ok) {
            for (const auto& r : results) {
                rag::judge::RetrievedDocument d;
                d.id               = r.pk;
                d.similarity_score = applyBm25PlusFloor(r.score, config_.bm25_delta);
                bm25_docs.push_back(std::move(d));
            }
        } else {
            spdlog::warn("[WikiIndexStore] BM25 scan failed: {}", s.message);
        }
    }

    // ── Vector KNN path ────────────────────────────────────────────────────
    if (config_.enable_vector) {
        // Embed the query text.  Results are cached in query_embed_cache_ under
        // query_embed_mutex_ (a dedicated exclusive lock independent of mutex_).
        // This avoids both a const_cast and a race window: concurrent query()
        // calls all hold the shared mutex_ but serialise embedding work through
        // query_embed_mutex_.
        std::vector<float> qvec;
        {
            std::lock_guard<std::mutex> qlock(query_embed_mutex_);
            if (auto it = query_embed_cache_.find(query_text);
                it != query_embed_cache_.end()) {
                qvec = it->second;
            } else {
                qvec = llm_ptr_->embed(query_text);
                query_embed_cache_.emplace(query_text, qvec);
            }
        }

        if (!qvec.empty()) {
            auto [s, results] = vim_.searchKnn(qvec, k * 2);
            if (s.ok) {
                for (const auto& r : results) {
                    rag::judge::RetrievedDocument d;
                    d.id               = r.pk;
                    // VectorIndexManager distance: smaller = better for COSINE (1-cos)
                    // Convert to similarity: sim = 1 - distance
                    d.similarity_score = static_cast<double>(1.0f - r.distance);
                    vec_docs.push_back(std::move(d));
                }
            } else {
                spdlog::warn("[WikiIndexStore] KNN search failed: {}", s.message);
            }
        }
    }

    // ── Fusion ─────────────────────────────────────────────────────────────
    rag::HybridFusionResult fused = retriever_.fuse(bm25_docs, vec_docs);

    // ── Convert to WikiChunks ──────────────────────────────────────────────
    std::vector<WikiChunk> out;
    out.reserve(fused.documents.size());

    for (std::size_t i = 0; i < fused.documents.size(); ++i) {
        const auto& doc = fused.documents[i];
        float score = (i < fused.scores.size())
                      ? static_cast<float>(fused.scores[i].hybrid_score)
                      : static_cast<float>(doc.similarity_score);

        if (score < min_score) {
            continue;
        }

        WikiChunk c;
        c.chunk_id = doc.id;
        c.score    = score;
        // Populate remaining fields from doc metadata if available
        for (const auto& meta_entry : doc.metadata) {
            const auto& meta_key = meta_entry.first;
            const auto& meta_value = meta_entry.second;
            if      (meta_key == "doc_id") {
              c.doc_id        = meta_value;
            }
            else if (meta_key == "section_title") c.section_title = meta_value;
            else if (meta_key == "source_path")   c.source_path   = meta_value;
            else if (meta_key == "content")       c.text          = meta_value;
        }
        if (c.text.empty()) {
          c.text = doc.content;
        }

        out.push_back(std::move(c));
        if (static_cast<int>(out.size()) >= k) {
          break;
        }
    }

    // ── Record query latency ───────────────────────────────────────────────
    {
        const auto t_end = std::chrono::steady_clock::now();
        const double latency_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        std::lock_guard<std::mutex> elk(eval_mutex_);
        recordLatencyLocked(latency_ms);
        ++total_query_count_;
    }

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — isReady
// ─────────────────────────────────────────────────────────────────────────────

bool WikiIndexStore::isReady() const noexcept {
    return ready_.load(std::memory_order_acquire);
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — Evaluation API (Recall@k / MRR / p95)
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::recordLatencyLocked([[maybe_unused]] double latency_ms) const noexcept {
    // Ring buffer: overwrite oldest entry when full.
    latency_ring_[latency_ring_head_] = latency_ms;
    latency_ring_head_ = (latency_ring_head_ + 1) % kLatencyRingSize;
    if (latency_ring_count_ < kLatencyRingSize) {
        ++latency_ring_count_;
    }
}

std::vector<WikiChunk> WikiIndexStore::evaluateQuery(
    const std::string&              query_text,
    int                             top_k,
    float                           min_score,
    const std::vector<std::string>& relevant_doc_ids) const {

    // query() records latency and increments total_query_count_ internally.
    auto results = query(query_text, top_k, min_score);

    if (!relevant_doc_ids.empty()) {
        const std::unordered_set<std::string> rel_set(
            relevant_doc_ids.begin(), relevant_doc_ids.end());

        // ── Recall@k ────────────────────────────────────────────────────────
        // For each k: count how many of the top-k returned results have a
        // doc_id in the ground-truth set, normalised by ground-truth size.
        auto recall_at = [&]([[maybe_unused]] int k) -> double {
            const int n = std::min(k, static_cast<int>(results.size()));
            int hits = 0;
            for (int i = 0; i < n; ++i) {
                if (rel_set.count(results[static_cast<std::size_t>(i)].doc_id)) {
                    ++hits;
                }
            }
            return static_cast<double>(hits) / static_cast<double>(rel_set.size());
        };

        // ── MRR ─────────────────────────────────────────────────────────────
        // Reciprocal rank of the first relevant result; 0 if none in results.
        double rr = 0.0;
        for (std::size_t i = 0; i < results.size(); ++i) {
            if (rel_set.count(results[i].doc_id)) {
                rr = 1.0 / static_cast<double>(i + 1);
                break;
            }
        }

        // ── Online mean update ──────────────────────────────────────────────
        std::lock_guard<std::mutex> lk(eval_mutex_);
        const double n_prev = static_cast<double>(eval_query_count_);
        const double n_new  = n_prev + 1.0;
        eval_recall_at_1_  = (eval_recall_at_1_  * n_prev + recall_at(1))  / n_new;
        eval_recall_at_3_  = (eval_recall_at_3_  * n_prev + recall_at(3))  / n_new;
        eval_recall_at_5_  = (eval_recall_at_5_  * n_prev + recall_at(5))  / n_new;
        eval_recall_at_10_ = (eval_recall_at_10_ * n_prev + recall_at(10)) / n_new;
        eval_mrr_          = (eval_mrr_           * n_prev + rr)             / n_new;
        ++eval_query_count_;
    }

    return results;
}

WikiEvalStats WikiIndexStore::getEvaluationStats() const {
    std::lock_guard<std::mutex> lk(eval_mutex_);
    WikiEvalStats s;
    s.recall_at_k1        = eval_recall_at_1_;
    s.recall_at_k3        = eval_recall_at_3_;
    s.recall_at_k5        = eval_recall_at_5_;
    s.recall_at_k10       = eval_recall_at_10_;
    s.mrr                 = eval_mrr_;
    s.query_count         = eval_query_count_;
    s.total_query_count   = total_query_count_;

    // Compute p95 from the valid portion of the ring buffer.
    if (latency_ring_count_ > 0) {
        // Copy valid samples — ring may have wrapped; copy count_ items starting
        // from the oldest (= head - count, wrapping around).
        std::vector<double> samples;
        samples.reserve(latency_ring_count_);
        const std::size_t oldest =
            (latency_ring_head_ + kLatencyRingSize - latency_ring_count_) % kLatencyRingSize;
        for (std::size_t i = 0; i < latency_ring_count_; ++i) {
            samples.push_back(latency_ring_[(oldest + i) % kLatencyRingSize]);
        }
        std::sort(samples.begin(), samples.end());
        // p95 index: ceil(0.95 * N) - 1, clamped to [0, N-1].
        const std::size_t p95_idx = static_cast<std::size_t>(
            std::ceil(0.95 * static_cast<double>(samples.size())));
        const std::size_t clamped = (p95_idx == 0) ? 0 : p95_idx - 1;
        s.p95_query_latency_ms = samples[std::min(clamped, samples.size() - 1)];
    }

    return s;
}

void WikiIndexStore::resetEvaluationStats() noexcept {
    std::lock_guard<std::mutex> lk(eval_mutex_);
    eval_recall_at_1_  = 0.0;
    eval_recall_at_3_  = 0.0;
    eval_recall_at_5_  = 0.0;
    eval_recall_at_10_ = 0.0;
    eval_mrr_          = 0.0;
    eval_query_count_  = 0;
    total_query_count_ = 0;
    latency_ring_head_  = 0;
    latency_ring_count_ = 0;
    // Wipe ring buffer values so stale data isn't accidentally exposed.
    std::fill(latency_ring_.begin(), latency_ring_.end(), 0.0);
}


// ─────────────────────────────────────────────────────────────────────────────

themis::BaseEntity WikiIndexStore::toEntity(const WikiChunk& chunk) {
    return chunkToEntity(chunk);
}

std::string WikiIndexStore::makeEmbeddingCacheKey(const WikiChunk& chunk) {
    const auto material = makeCacheMaterial(chunk);
    auto key = core::concerns::SignedAdapterValidator::sha256Hex(material);
    if (key.empty()) {
        const auto fallback = std::hash<std::string>{}(material);
        std::ostringstream oss;
        oss << "fallback-hash:" << std::hex << fallback;
        key = oss.str();
    }
    return key;
}

std::size_t WikiIndexStore::estimateEmbeddingBytes(const std::string& cache_key,
                                                   const std::vector<float>& embedding) noexcept {
    return cache_key.size() + embedding.size() * sizeof(float);
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — Phase 3 helpers: persistent cache, auto-probe dim
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::persistEmbedding(const std::string&        cache_key,
                                      const std::string&        chunk_id,
                                      const std::vector<float>& embedding) {
    if (!config_.enable_persistent_cache) {
        return;
    }
    themis::BaseEntity e;
    e.setPrimaryKey(cache_key);
    e.setField("cache_key", cache_key);
    e.setField("chunk_id", chunk_id);
    e.setField("embedding", embedding);
    auto s = sim_.put(emb_cache_table_, e);
    if (!s.ok) {
        spdlog::warn("[WikiIndexStore] persistEmbedding: {}", s.message);
    }
}

std::optional<std::vector<float>> WikiIndexStore::fetchPersistedEmbedding(
    const std::string& cache_key) const
{
    if (!config_.enable_persistent_cache || cache_key.empty()) {
        return std::nullopt;
    }
    auto [status, entities] = sim_.scanEntitiesEqual(emb_cache_table_, "cache_key", cache_key);
    if (!status.ok || entities.empty()) {
        return std::nullopt;
    }
    auto maybe_vec = entities[0].getFieldAsVector("embedding");
    if (!maybe_vec.has_value() || maybe_vec->empty()) {
        return std::nullopt;
    }
    return maybe_vec;
}

std::optional<std::vector<float>> WikiIndexStore::fetchLegacyPersistedEmbeddingByChunkId(
    const std::string& chunk_id) const
{
    if (!config_.enable_persistent_cache || chunk_id.empty()) {
        return std::nullopt;
    }
    auto [status, entities] = sim_.scanEntitiesEqual(legacy_emb_cache_table_, "chunk_id", chunk_id);
    if (!status.ok || entities.empty()) {
        return std::nullopt;
    }
    auto maybe_vec = entities[0].getFieldAsVector("embedding");
    if (!maybe_vec.has_value() || maybe_vec->empty()) {
        return std::nullopt;
    }
    return maybe_vec;
}

void WikiIndexStore::loadPersistentEmbedCache() {
    spdlog::debug("[WikiIndexStore] loadPersistentEmbedCache: persistent cache enabled; "
                  "embeddings will be fetched lazily from '{}' on cache miss; "
                  "legacy migration table='{}' migration_enabled={}",
                  emb_cache_table_, legacy_emb_cache_table_,
                  config_.enable_phase_a_cache_migration);
}

void WikiIndexStore::touchEmbeddingCacheEntry(const std::string& cache_key) const {
    auto it = embed_cache_lru_pos_.find(cache_key);
    if (it == embed_cache_lru_pos_.end()) {
        return;
    }
    embed_cache_lru_.splice(embed_cache_lru_.begin(), embed_cache_lru_, it->second);
    it->second = embed_cache_lru_.begin();
}

void WikiIndexStore::upsertEmbeddingCacheEntry(const std::string&        cache_key,
                                               const std::vector<float>& embedding) const {
    auto it = embed_cache_.find(cache_key);
    const auto new_bytes = estimateEmbeddingBytes(cache_key, embedding);
    if (it != embed_cache_.end()) {
        auto old_it = embed_cache_entry_bytes_.find(cache_key);
        if (old_it != embed_cache_entry_bytes_.end() && embed_cache_bytes_ >= old_it->second) {
            embed_cache_bytes_ -= old_it->second;
        }
        it->second = embedding;
        embed_cache_entry_bytes_[cache_key] = new_bytes;
        embed_cache_bytes_ += new_bytes;
        touchEmbeddingCacheEntry(cache_key);
        enforceEmbeddingCacheLimit();
        return;
    }

    embed_cache_.emplace(cache_key, embedding);
    embed_cache_lru_.push_front(cache_key);
    embed_cache_lru_pos_[cache_key] = embed_cache_lru_.begin();
    embed_cache_entry_bytes_[cache_key] = new_bytes;
    embed_cache_bytes_ += new_bytes;
    enforceEmbeddingCacheLimit();
}

void WikiIndexStore::enforceEmbeddingCacheLimit() const {
    if (config_.embedding_cache_max_bytes == 0) {
        return;
    }
    while (embed_cache_bytes_ > config_.embedding_cache_max_bytes &&
           !embed_cache_lru_.empty()) {
        const auto victim_key = embed_cache_lru_.back();
        embed_cache_lru_.pop_back();
        embed_cache_lru_pos_.erase(victim_key);
        embed_cache_.erase(victim_key);
        auto bytes_it = embed_cache_entry_bytes_.find(victim_key);
        std::size_t victim_bytes = 0;
        if (bytes_it != embed_cache_entry_bytes_.end()) {
            victim_bytes = bytes_it->second;
            embed_cache_entry_bytes_.erase(bytes_it);
        }
        if (embed_cache_bytes_ >= victim_bytes) {
            embed_cache_bytes_ -= victim_bytes;
        } else {
            embed_cache_bytes_ = 0;
        }
        spdlog::info("[WikiIndexStore] embedding_cache LRU eviction key={} bytes={} current_bytes={} limit_bytes={}",
                     victim_key, victim_bytes, embed_cache_bytes_, config_.embedding_cache_max_bytes);
    }
}

void WikiIndexStore::migrateLegacyEntryIfNeeded(const WikiChunk&              chunk,
                                                const std::vector<float>& embedding) {
    if (!config_.enable_phase_a_cache_migration) {
        return;
    }
    const auto cache_key = makeEmbeddingCacheKey(chunk);
    spdlog::info("[WikiIndexStore] migrating legacy Phase-A embedding cache entry chunk_id={} -> cache_key={}",
                 chunk.chunk_id, cache_key);
    persistEmbedding(cache_key, chunk.chunk_id, embedding);
}

bool WikiIndexStore::tryResolveEmbeddingFromCaches(const WikiChunk& chunk,
                                                   std::vector<float>* out_embedding) {
    if (out_embedding == nullptr) {
        return false;
    }
    const auto cache_key = makeEmbeddingCacheKey(chunk);
    if (auto it = embed_cache_.find(cache_key); it != embed_cache_.end()) {
        *out_embedding = it->second;
        touchEmbeddingCacheEntry(cache_key);
        return true;
    }
    if (auto persisted = fetchPersistedEmbedding(cache_key); persisted.has_value()) {
        *out_embedding = *persisted;
        upsertEmbeddingCacheEntry(cache_key, *persisted);
        return true;
    }
    if (config_.enable_phase_a_cache_migration) {
        if (auto legacy = fetchLegacyPersistedEmbeddingByChunkId(chunk.chunk_id);
            legacy.has_value()) {
            *out_embedding = *legacy;
            upsertEmbeddingCacheEntry(cache_key, *legacy);
            migrateLegacyEntryIfNeeded(chunk, *legacy);
            return true;
        }
    }
    return false;
}

/**
 * @brief Probe the LLM to determine the actual embedding dimensionality.
 *
 * Embeds a short, deterministic sentinel string (`"__dim_probe__"`) and uses
 * the returned vector size as the authoritative `embedding_dim`.  If the
 * probed dimension differs from `config_.embedding_dim`, the vector index is
 * re-initialised with the correct size.
 *
 * This method is idempotent: subsequent calls are no-ops once `dim_probed_`
 * is set.  Call sites must hold the write lock.
 */
void WikiIndexStore::probeEmbeddingDim() {
    // Double-checked pattern inside the held write lock.
    if (dim_probed_.load(std::memory_order_acquire)) {
        return;
    }

    static constexpr const char* kProbeSentinel = "__dim_probe__";
    auto probe_vec = llm_.embed(kProbeSentinel);
    if (probe_vec.empty()) {
        spdlog::warn("[WikiIndexStore] probeEmbeddingDim: embed() returned empty vector; "
                     "keeping configured dim={}", config_.embedding_dim);
        dim_probed_.store(true, std::memory_order_release);
        return;
    }

    const int probed_dim = static_cast<int>(probe_vec.size());
    if (probed_dim != config_.embedding_dim) {
        spdlog::info("[WikiIndexStore] probeEmbeddingDim: dim {} → {} (re-initialising vector index)",
                     config_.embedding_dim, probed_dim);
        config_.embedding_dim = probed_dim;

        if (config_.enable_vector) {
            auto s = vim_.init(config_.table_name,
                               config_.embedding_dim,
                               VectorIndexManager::Metric::COSINE,
                               config_.hnsw_m,
                               config_.hnsw_ef_construction,
                               config_.hnsw_ef_search);
            if (!s.ok) {
                spdlog::warn("[WikiIndexStore] probeEmbeddingDim: vim_.init: {}", s.message);
            } else {
                auto ef_s = vim_.setEfSearch(config_.hnsw_ef_search);
                if (!ef_s.ok) {
                    spdlog::warn("[WikiIndexStore] probeEmbeddingDim: setEfSearch({}): {}",
                                 config_.hnsw_ef_search, ef_s.message);
                }
            }
        }
    }

    dim_probed_.store(true, std::memory_order_release);
    spdlog::debug("[WikiIndexStore] probeEmbeddingDim: effective dim={}", config_.embedding_dim);
}

// ─────────────────────────────────────────────────────────────────────────────
// JsonWikiIndexReader
// ─────────────────────────────────────────────────────────────────────────────

JsonWikiIndexReader::JsonWikiIndexReader(std::string index_path, bool auto_load)
    : index_path_(std::move(index_path))
{
    if (auto_load) {
        load();
    }
}

void JsonWikiIndexReader::load() {
    std::ifstream f(index_path_);
    if (!f.is_open()) {
        throw std::runtime_error("[JsonWikiIndexReader] cannot open: " + index_path_);
    }

    nlohmann::json j;
    try {
        f >> j;
    } catch (const nlohmann::json::exception& ex) {
        throw std::runtime_error(
            std::string("[JsonWikiIndexReader] JSON parse error: ") + ex.what());
    }

    if (!j.is_array()) {
        throw std::runtime_error(
            "[JsonWikiIndexReader] expected JSON array at top level");
    }

    chunks_.clear();
    chunks_.reserve(j.size());

    for (const auto& obj : j) {
        WikiChunk c;
        c.chunk_id      = obj.value("chunk_id",      "");
        c.source_path   = obj.value("file_path",      "");
        c.doc_id        = c.source_path;
        c.section_title = obj.value("section_title",  "");
        c.line_start    = obj.value("line_start",      0);
        c.line_end      = obj.value("line_end",        0);
        c.text          = obj.value("text",            "");
        // Optional score_override (no[[maybe_unused]] t i[[maybe_unused]] n basi[[maybe_unused]] c MV[[maybe_unused]] P forma[[maybe_unused]] t, bu[[maybe_unused]] t accepte[[maybe_unused]] d)
        if (ob[[maybe_unused]] j.contain[[maybe_unused]] s("score_overrid[[maybe_unused]] e") && obj["score_override"].is_number()) {
            c.score = obj["score_override"].get<float>();
        }
        if (!c.chunk_id.empty()) {
            chunks_.push_back(std::move(c));
        }
    }

    loaded_ = true;
    spdlog::debug("[JsonWikiIndexReader] loaded {} chunks from {}", chunks_.size(), index_path_);
}

// ─────────────────────────────────────────────────────────────────────────────
// JsonWikiIndexReader — query (in-memory BM25-style)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<WikiChunk> JsonWikiIndexReader::query(const std::string& query_text,
                                                   int                top_k,
                                                   float              min_score) const {
    if (!loaded_ || chunks_.empty()) {
        return {};
    }

    static const std::regex tok_re(R"([A-Za-z0-9_\-]+)", std::regex::ECMAScript);

    // Tokenise query
    std::unordered_map<std::string, int> query_tf;
    {
        auto it  = std::sregex_iterator(query_text.begin(), query_text.end(), tok_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string tok = it->str();
            // Lowercase for case-insensitive matching
            std::transform(tok.begin(), tok.end(), tok.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            ++query_tf[tok];
        }
    }

    if (query_tf.empty()) {
        return {};
    }

    // Score each chunk
    struct Scored { std::size_t idx; float score; };
    std::vector<Scored> scored;
    scored.reserve(chunks_.size());

    for (std::size_t ci = 0; ci < chunks_.size(); ++ci) {
        const WikiChunk& chunk = chunks_[ci];

        // Build chunk token frequencies
        std::unordered_map<std::string, int> chunk_tf;
        {
            auto it  = std::sregex_iterator(chunk.text.begin(), chunk.text.end(), tok_re);
            auto end = std::sregex_iterator();
            for (; it != end; ++it) {
                std::string tok = it->str();
                std::transform(tok.begin(), tok.end(), tok.begin(),
                               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                ++chunk_tf[tok];
            }
        }

        // TF overlap: sum TF(query_tok in chunk) * TF(query_tok in query)
        float sc = 0.0f;
        for (const auto& [qtok, qtf] : query_tf) {
            auto cit = chunk_tf.find(qtok);
            if (cit != chunk_tf.end()) {
                sc += static_cast<float>(1 + std::log(static_cast<float>(cit->second)))
                    * static_cast<float>(qtf);
            }
        }

        // Normalise by chunk length (avoid favouring very long chunks)
        if (!chunk_tf.empty()) {
            sc /= static_cast<float>(std::sqrt(static_cast<double>(chunk_tf.size())));
        }

        if (sc >= min_score) {
            scored.push_back({ci, sc});
        }
    }

    // Sort descending by score
    std::sort(scored.begin(), scored.end(),
              [](const Scored& a, const Scored& b){ return a.score > b.score; });

    // Build result
    const int limit = (top_k > 0)
                      ? std::min(static_cast<int>(scored.size()), top_k)
                      : static_cast<int>(scored.size());

    std::vector<WikiChunk> out;
    out.reserve(static_cast<std::size_t>(limit));
    for (int i = 0; i < limit; ++i) {
        WikiChunk c = chunks_[scored[i].idx];
        c.score = scored[i].score;
        out.push_back(std::move(c));
    }
    return out;
}

bool JsonWikiIndexReader::isReady() const noexcept {
    return loaded_;
}

std::size_t JsonWikiIndexReader::size() const noexcept {
    return chunks_.size();
}

} // namespace llm
} // namespace themis
