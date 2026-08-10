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

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace themis {
namespace llm {

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

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
    , emb_cache_table_(config_.table_name + "_emb_cache")
{
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
                            VectorIndexManager::Metric::COSINE);
        if (!s3.ok) {
            spdlog::warn("[WikiIndexStore] VectorIndexManager::init: {}", s3.message);
        }
    }

    ready_.store(true, std::memory_order_release);
    spdlog::debug("[WikiIndexStore] initialised table={} dim={}",
                  config_.table_name, config_.embedding_dim);

    // Load persisted embedding cache from RocksDB (must run after ready_ is set)
    if (config_.enable_persistent_cache) {
        // Create an index on chunk_id field to support per-entry lookup.
        // The emb_cache table stores chunk_id both as PK and as an indexed field
        // so scanEntitiesEqual() can locate entries by chunk_id.
        auto idx_s = sim_.createIndex(emb_cache_table_, "chunk_id");
        if (!idx_s.ok) {
            spdlog::debug("[WikiIndexStore] emb_cache createIndex(chunk_id): {}", idx_s.message);
        }
        loadPersistentEmbedCache();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// IWikiIndexWriter — writeChunk
// ─────────────────────────────────────────────────────────────────────────────

void WikiIndexStore::writeChunk(WikiChunk chunk) {
    std::unique_lock lock(mutex_);

    // Auto-probe embedding dimensionality on first write
    if (config_.auto_probe_dim) {
        probeEmbeddingDim();
    }

    // Compute embedding if missing
    if (chunk.embedding.empty()) {
        if (auto it = embed_cache_.find(chunk.chunk_id); it != embed_cache_.end()) {
            chunk.embedding = it->second;
        } else if (auto persisted = fetchPersistedEmbedding(chunk.chunk_id);
                   persisted.has_value()) {
            chunk.embedding = std::move(*persisted);
            embed_cache_[chunk.chunk_id] = chunk.embedding;
        } else {
            chunk.embedding = llm_.embed(chunk.text);
            embed_cache_[chunk.chunk_id] = chunk.embedding;
            persistEmbedding(chunk.chunk_id, chunk.embedding);
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

    // Auto-probe embedding dimensionality on first write
    if (config_.auto_probe_dim) {
        probeEmbeddingDim();
    }

    const int effective_batch_size = std::max(1, config_.batch_size);

    // Collect indices of chunks that still need embeddings
    std::vector<std::size_t> need_embed;
    for (std::size_t i = 0; i < chunks.size(); ++i) {
        if (chunks[i].embedding.empty()) {
            if (auto it = embed_cache_.find(chunks[i].chunk_id); it != embed_cache_.end()) {
                chunks[i].embedding = it->second;
            } else if (auto persisted = fetchPersistedEmbedding(chunks[i].chunk_id);
                       persisted.has_value()) {
                chunks[i].embedding = std::move(*persisted);
                embed_cache_[chunks[i].chunk_id] = chunks[i].embedding;
            } else {
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
                embed_cache_[chunks[ci].chunk_id] = chunks[ci].embedding;
                persistEmbedding(chunks[ci].chunk_id, chunks[ci].embedding);
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
    std::shared_lock lock(mutex_);

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
                d.similarity_score = r.score;
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
        for (const auto& [k, v] : doc.metadata) {
            if      (k == "doc_id")        c.doc_id        = v;
            else if (k == "section_title") c.section_title = v;
            else if (k == "source_path")   c.source_path   = v;
            else if (k == "content")       c.text          = v;
        }
        if (c.text.empty()) c.text = doc.content;

        out.push_back(std::move(c));
        if (static_cast<int>(out.size()) >= k) break;
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
// WikiIndexStore — toEntity (static)
// ─────────────────────────────────────────────────────────────────────────────

themis::BaseEntity WikiIndexStore::toEntity(const WikiChunk& chunk) {
    return chunkToEntity(chunk);
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — Phase 3 helpers: persistent cache, auto-probe dim
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Persist a single embedding to RocksDB under the emb_cache table.
 *
 * Stores the embedding as a `BaseEntity` with primary key `chunk_id`, a
 * float-vector field `"embedding"`, and a string field `"chunk_id"` used as
 * the lookup key in `fetchPersistedEmbedding`.  Call sites must hold the
 * write lock.
 *
 * No-op when `config_.enable_persistent_cache` is `false`.
 *
 * @param chunk_id  Stable chunk identifier (used as primary key and lookup field).
 * @param embedding Dense embedding vector to persist.
 */
void WikiIndexStore::persistEmbedding(const std::string&        chunk_id,
                                      const std::vector<float>& embedding) {
    if (!config_.enable_persistent_cache) {
        return;
    }
    themis::BaseEntity e;
    e.setPrimaryKey(chunk_id);
    e.setField("chunk_id", chunk_id);   // indexed field for scanEntitiesEqual lookup
    e.setField("embedding", embedding);
    auto s = sim_.put(emb_cache_table_, e);
    if (!s.ok) {
        spdlog::warn("[WikiIndexStore] persistEmbedding: {}", s.message);
    }
}

/**
 * @brief Attempt to load a single embedding from RocksDB by chunk_id.
 *
 * Performs an indexed lookup using `SecondaryIndexManager::scanEntitiesEqual`
 * on the `"chunk_id"` field of the emb_cache table.  Returns the embedding
 * from the first matching entry, or `std::nullopt` if not found.
 *
 * Returns `std::nullopt` immediately when `config_.enable_persistent_cache`
 * is `false`.
 *
 * @param chunk_id  Chunk identifier to look up.
 * @return          The embedding vector, or `std::nullopt`.
 */
std::optional<std::vector<float>> WikiIndexStore::fetchPersistedEmbedding(
    const std::string& chunk_id) const
{
    if (!config_.enable_persistent_cache || chunk_id.empty()) {
        return std::nullopt;
    }
    auto [status, entities] = sim_.scanEntitiesEqual(emb_cache_table_, "chunk_id", chunk_id);
    if (!status.ok || entities.empty()) {
        return std::nullopt;
    }
    auto maybe_vec = entities[0].getFieldAsVector("embedding");
    if (!maybe_vec.has_value() || maybe_vec->empty()) {
        return std::nullopt;
    }
    return maybe_vec;
}

/**
 * @brief Load previously persisted embeddings from RocksDB into embed_cache_.
 *
 * Called once during construction when `config_.enable_persistent_cache` is
 * `true`.
 *
 * @note Since `SecondaryIndexManager` does not expose a full-table scan,
 *       this function logs a debug note that embeddings are fetched lazily
 *       (on demand in `writeChunk` / `writeBatch`) instead of eagerly.
 *       The in-memory `embed_cache_` is therefore empty after construction;
 *       individual entries are fetched from RocksDB on first access.
 */
void WikiIndexStore::loadPersistentEmbedCache() {
    spdlog::debug("[WikiIndexStore] loadPersistentEmbedCache: persistent cache enabled; "
                  "embeddings will be fetched lazily from '{}' on cache miss",
                  emb_cache_table_);
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
                               VectorIndexManager::Metric::COSINE);
            if (!s.ok) {
                spdlog::warn("[WikiIndexStore] probeEmbeddingDim: vim_.init: {}", s.message);
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
        // Optional score_override (not in basic MVP format, but accepted)
        if (obj.contains("score_override") && obj["score_override"].is_number()) {
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
