/**
 * @file llm_wiki_plugin_impl.h
 * @brief Private implementation header for `LLMWikiPluginImpl`.
 *
 * NOT part of the public SDK. Include only from within the private plugin
 * repository (`plugins/private/themisdb_llm_wiki`).
 *
 * ## Architecture
 *
 * ### Phase A (default — no `rocksdb_dir`)
 * - In-memory `std::vector<WikiChunk> chunks_`
 * - Optional `JsonWikiIndexReader` loaded from `json_index_path_`
 * - Hash embedding (FNV-1a deterministic, no external deps)
 * - In-memory BM25 token-overlap scoring as retrieval fallback
 *
 * ### Phase B (`rocksdb_dir` set AND compiled with `THEMISDB_WIKI_PHASE_B`)
 * - `RocksDBWrapper` + `SecondaryIndexManager` + `VectorIndexManager`
 * - `WikiIndexStore` (BM25 + HNSW + RRF fusion)
 * - `EmbeddedLLM` with injected hash `EmbedFn`
 * - Falls back to Phase A with a logged warning if RocksDB unavailable
 *
 * ## Thread safety
 * - `std::shared_mutex mutex_` guards all state
 * - `query()` takes a shared lock (concurrent reads allowed)
 * - `ingest()`, `initialize()`, `shutdown()` take an exclusive lock
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA (Phase 2)
 * @note Edition: enterprise / hyperscaler / military
 */

#pragma once

#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "wikipedia/wiki_workspace_orchestrator.h"

#include "llm/wiki_chunk_splitter.h"
#include "llm/wiki_index_store.h"

#ifdef THEMISDB_WIKI_PHASE_B
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "llm/embedded_llm.h"
#include "storage/rocksdb_wrapper.h"
#endif

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace plugins {
namespace llm_wiki {

/**
 * @brief Concrete implementation of `ILLMWikiPlugin`.
 *
 * Implements all pure virtuals from both `IThemisPlugin` and `ILLMWikiPlugin`.
 *
 * ### Shutdown contract
 * `shutdown() noexcept` satisfies both `IThemisPlugin::shutdown()` and
 * `ILLMWikiPlugin::shutdown() noexcept`; adding `noexcept` strengthens the
 * contract without violating either base.
 *
 * ### initialize() overloads
 * - `bool initialize(const char*)` from `IThemisPlugin` — delegates to the
 *   string overload and maps the `Status` result to `bool`.
 * - `Status initialize(const std::string&)` from `ILLMWikiPlugin` — performs
 *   the real initialisation.
 */
class LLMWikiPluginImpl final : public ILLMWikiPlugin {
public:
    LLMWikiPluginImpl()  = default;
    ~LLMWikiPluginImpl() override = default;

    // Non-copyable, non-movable (shared ownership via shared_ptr from factory)
    LLMWikiPluginImpl(const LLMWikiPluginImpl&)            = delete;
    LLMWikiPluginImpl& operator=(const LLMWikiPluginImpl&) = delete;
    LLMWikiPluginImpl(LLMWikiPluginImpl&&)                 = delete;
    LLMWikiPluginImpl& operator=(LLMWikiPluginImpl&&)      = delete;

    // ── IThemisPlugin metadata ────────────────────────────────────────────────

    /// @return "LLM Wiki Plugin"
    [[nodiscard]] const char* getName()    const override { return "LLM Wiki Plugin"; }
    /// @return "0.1.0"
    [[nodiscard]] const char* getVersion() const override { return "0.1.0"; }
    /// @return `PluginType::CUSTOM`
    [[nodiscard]] PluginType  getType()    const override { return PluginType::CUSTOM; }

    /**
     * @brief Returns capabilities for this plugin.
     * @return Capabilities with `thread_safe = true`, `supports_batching = true`.
     */
    [[nodiscard]] PluginCapabilities getCapabilities() const override;

    /**
     * @brief `IThemisPlugin::initialize` — delegates to the string overload.
     *
     * @param config_json  Null-terminated JSON configuration string.
     * @return             `true` on success, `false` on parse or init error.
     */
    [[nodiscard]] bool initialize(const char* config_json) override;

    /**
     * @brief Graceful shutdown — flushes pending writes and releases resources.
     *
     * Satisfies both `IThemisPlugin::shutdown()` and
     * `ILLMWikiPlugin::shutdown() noexcept`.
     */
    void shutdown() noexcept override;

    /**
     * @brief Return `this` cast to `void*` for type-specific downcast by the
     *        plugin manager.
     */
    [[nodiscard]] void* getInstance() override { return static_cast<void*>(this); }

    // ── ILLMWikiPlugin identity ───────────────────────────────────────────────

    [[nodiscard]] const char* pluginId()      const noexcept override { return "llm_wiki"; }
    [[nodiscard]] const char* pluginVersion() const noexcept override { return "0.1.0"; }

    // ── ILLMWikiPlugin lifecycle ──────────────────────────────────────────────

    /**
     * @brief Initialise from a JSON configuration blob.
     *
     * Recognised fields (all optional, shown with defaults):
     * ```json
     * {
     *   "embedding_provider":      "hash",
     *   "embedding_dim":           128,
     *   "table_name":              "wiki_chunks",
     *   "rocksdb_dir":             "",
     *   "workspace_root":          "",
     *   "json_index_path":         "",
     *   "retrieval_top_k":         5,
     *   "retrieval_min_score":     0.0,
     *   "fail_open":               false,
     *   "lint_max_staleness_days": 30,
     *   "llm_wiki_wikipedia":      false,
     *   "splitter_max_tokens":     220,
     *   "splitter_overlap_tokens": 40
     * }
     * ```
     *
     * @param config_json  JSON object string.
     * @return             `Status::Ok()` on success; error status on failure.
     */
    [[nodiscard]] Status initialize(const std::string& config_json) override;

    // ── ILLMWikiPlugin core ops ───────────────────────────────────────────────

    /**
     * @brief Ingest documents from `source_path` into the wiki index.
     * @param source_path  File or directory to ingest.
     * @param opts         Ingestion options.
     * @return             `WikiIngestResult`.
     */
    [[nodiscard]] WikiIngestResult ingest(
        const std::string&       source_path,
        const WikiIngestOptions& opts = {}) override;

    /**
     * @brief Query the wiki index for chunks relevant to `query_text`.
     *
     * Applies prompt-injection guardrails to the query and content guardrails
     * to retrieved chunks.
     *
     * @param query_text  Natural-language query.
     * @param opts        Query options.
     * @return            `WikiQueryResult` with scored candidates and guardrail flags.
     */
    [[nodiscard]] WikiQueryResult query(
        const std::string&      query_text,
        const WikiQueryOptions& opts = {}) override;

    // ── ILLMWikiPlugin workspace ops ─────────────────────────────────────────

    /**
     * @brief Initialise a persistent wiki workspace at `workspace_root`.
     * @param workspace_root  Workspace directory path.
     * @return                `Status::Ok()` on success.
     */
    [[nodiscard]] Status wikiInit(const std::string& workspace_root) override;

    /**
     * @brief Ingest a source file into the initialized workspace.
     * @param source_path  Source file path.
     * @param opts         Ingestion options; `workspace_root` used if set.
     * @return             `WikiIngestResult`.
     */
    [[nodiscard]] WikiIngestResult wikiIngest(
        const std::string&       source_path,
        const WikiIngestOptions& opts) override;

    /**
     * @brief Query an initialized workspace, optionally persisting the result.
     * @param query_text  Natural-language query.
     * @param opts        Query options; `workspace_root` used if set.
     * @return            `WikiQueryResult`.
     */
    [[nodiscard]] WikiQueryResult wikiQuery(
        const std::string&      query_text,
        const WikiQueryOptions& opts) override;

    /**
     * @brief Run lint checks on the workspace.
     * @param workspace_root      Workspace root path.
     * @param max_staleness_days  Staleness threshold in days.
     * @return                    `WikiLintResult`.
     */
    [[nodiscard]] WikiLintResult wikiLint(
        const std::string& workspace_root,
        int max_staleness_days = 30) override;

    /**
     * @brief Ingest a Wikipedia XML dump (requires `llm_wiki_wikipedia` license).
     * @param dump_path  Local path or HTTP(S) URI.
     * @param opts       Dump ingestion options.
     * @return           `WikiIngestResult` with article/chunk counts.
     */
    [[nodiscard]] WikiIngestResult ingestWikipediaDump(
        const std::string&          dump_path,
        const WikiDumpIngestOptions& opts = {}) override;

    /**
     * @brief Return current index and workspace statistics.
     * @param workspace_root  Optional workspace root for workspace-level stats.
     * @return                `WikiWorkspaceStats`.
     */
    [[nodiscard]] WikiWorkspaceStats stats(
        const std::string& workspace_root = {}) override;

    // ── Diagnostics (not part of SDK interface) ───────────────────────────────

    /// @return True when `initialize()` has succeeded.
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_.load(); }

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /**
     * @brief Write `chunks_` to `json_index_path_` atomically.
     *
     * Caller must hold the exclusive lock on `mutex_`.
     */
    void persistJsonIndex_locked();

    /**
     * @brief BM25 token-overlap scoring over the in-memory `chunks_` vector.
     *
     * Used as the Phase A fallback when `json_reader_` is not ready.
     *
     * @param query_text  Query string (tokenised with `[A-Za-z0-9_\\-]+`).
     * @param top_k       Maximum candidates to return (0 = unlimited).
     * @param min_score   Minimum score threshold.
     * @return            Scored chunks, descending order.
     */
    [[nodiscard]] std::vector<themis::llm::WikiChunk> inMemoryBm25(
        const std::string& query_text,
        int   top_k,
        float min_score) const;

    // ── State ─────────────────────────────────────────────────────────────────

    std::atomic<bool>          initialized_{false};
    mutable std::shared_mutex  mutex_;

    // Configuration (set during initialize())
    std::string embedding_provider_{"hash"};
    int         embedding_dim_{128};
    std::string table_name_{"wiki_chunks"};
    std::string rocksdb_dir_;
    std::string workspace_root_;
    std::string json_index_path_;
    int         retrieval_top_k_{5};
    float       retrieval_min_score_{0.0f};
    bool        fail_open_{false};
    int         lint_max_staleness_days_{30};
    bool        has_wikipedia_license_{false};

    // Phase A in-memory store
    std::vector<themis::llm::WikiChunk>               chunks_;
    std::unique_ptr<themis::llm::JsonWikiIndexReader>  json_reader_;
    themis::llm::WikiChunkSplitter                     splitter_{220, 40};

    // Persistent workspace orchestrator (active when workspace_root_ is non-empty)
    std::unique_ptr<WikiWorkspaceOrchestrator> workspace_;

#ifdef THEMISDB_WIKI_PHASE_B
    bool phase_b_active_{false};
    std::unique_ptr<themis::RocksDBWrapper>            rocksdb_;
    std::unique_ptr<themis::SecondaryIndexManager>     sim_;
    std::unique_ptr<themis::VectorIndexManager>        vim_;
    std::unique_ptr<themis::llm::EmbeddedLLM>          llm_b_;
    std::unique_ptr<themis::llm::WikiIndexStore>       wiki_store_;
#endif
};

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
