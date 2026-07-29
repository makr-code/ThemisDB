/**
 * @file llm_wiki_plugin_interface.h
 * @brief Public C++ SDK interface for the LLM Wiki enterprise plugin.
 *
 * Exposes the canonical plugin entry point, typed request/response structs, and
 * the `ILLMWikiPlugin` abstract interface that the private plugin implementation
 * (`plugins/private/themisdb_llm_wiki`) implements.
 *
 * ## Plugin identity
 *  - Plugin type: `PluginType::CUSTOM`
 *  - Canonical plugin ID: `"llm_wiki"`
 *  - Allowed editions: `enterprise`, `hyperscaler`, `military`
 *  - Min ThemisDB version: `1.5.0`
 *  - License feature gate: `llm_wiki_enterprise`
 *
 * ## Integration overview
 *
 * @code
 *   // Load via PluginManager (enterprise runtime):
 *   auto& mgr = themis::plugins::PluginManager::instance();
 *   auto plugin = std::dynamic_pointer_cast<ILLMWikiPlugin>(
 *       mgr.load("llm_wiki", "/usr/lib/themisdb/plugins/themisdb_llm_wiki.so"));
 *
 *   // Ingest a Markdown source directory:
 *   WikiIngestOptions ingest_opts;
 *   ingest_opts.recursive        = true;
 *   ingest_opts.splitter_max_tokens   = 220;
 *   ingest_opts.splitter_overlap_tokens = 40;
 *   ingest_opts.embedding_provider = "hash";  // or "sentence-transformers"
 *   auto result = plugin->ingest("/srv/docs", ingest_opts);
 *
 *   // Query:
 *   WikiQueryOptions query_opts;
 *   query_opts.top_k    = 5;
 *   query_opts.min_score = 0.1f;
 *   auto candidates = plugin->query("How does HNSW work?", query_opts);
 * @endcode
 *
 * ## Thread safety
 *
 * All methods are thread-safe after successful `initialize()`. Concurrent
 * `query()` calls are lock-free (shared-read). Concurrent `ingest()` calls
 * serialize writes through an internal `shared_mutex`.
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA — Phase 1 (API contract); implementation in private plugin
 * @note Edition: enterprise / hyperscaler / military
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "llm/wiki_index_store.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace llm_wiki {

// ============================================================================
// Status — lightweight result type for lifecycle operations
// ============================================================================

/**
 * @brief Status result for ILLMWikiPlugin lifecycle operations.
 *
 * Lightweight value type returned by `initialize()`, `wikiInit()`, and similar
 * methods to signal success or failure with a human-readable message.
 */
struct Status {
    /// @brief Status code categories.
    enum class Code {
        Ok,               ///< Operation succeeded.
        Error,            ///< Generic failure.
        PermissionDenied, ///< Sub-feature or edition gate blocked the call.
        InvalidArgument,  ///< Malformed or out-of-range input.
        NotInitialized,   ///< Plugin is not yet initialized.
    };

    Code        code    = Code::Ok;
    std::string message;

    [[nodiscard]] bool ok() const noexcept { return code == Code::Ok; }

    [[nodiscard]] static Status Ok() { return {Code::Ok, {}}; }
    [[nodiscard]] static Status Error(std::string msg)
        { return {Code::Error, std::move(msg)}; }
    [[nodiscard]] static Status PermissionDenied(std::string msg)
        { return {Code::PermissionDenied, std::move(msg)}; }
    [[nodiscard]] static Status InvalidArgument(std::string msg)
        { return {Code::InvalidArgument, std::move(msg)}; }
    [[nodiscard]] static Status NotInitialized()
        { return {Code::NotInitialized, "plugin not initialized; call initialize() first"}; }
};

// ============================================================================
// Forward declarations
// ============================================================================

class ILLMWikiPlugin;

// ============================================================================
// WikiIngestOptions
// ============================================================================

/**
 * @brief Options controlling document ingestion into the LLM Wiki index.
 *
 * Passed to `ILLMWikiPlugin::ingest()` and `ILLMWikiPlugin::wikiIngest()`.
 *
 * @note `embedding_provider` must be one of `"hash"` (default, no deps),
 *       `"mock"` (tests), `"sentence-transformers"` (requires Python bridge
 *       in enterprise builds), or `"openai"` (requires `OPENAI_API_KEY`).
 */
struct WikiIngestOptions {
    bool        recursive               = true;   ///< Recurse into subdirectories
    std::string file_glob               = "*.md"; ///< File glob filter (Markdown default)
    int         splitter_max_tokens     = 220;    ///< Max tokens per chunk
    int         splitter_overlap_tokens = 40;     ///< Overlap tokens between consecutive chunks
    std::string embedding_provider      = "hash"; ///< Embedding provider name
    std::string embedding_model;                  ///< Model name (used by sentence-transformers/openai)
    int         embedding_dim           = 128;    ///< Dimension for hash provider
    bool        skip_existing           = false;  ///< Skip files already present in the index
    std::optional<std::string> workspace_root;    ///< Persistent workspace root (wiki-workspace mode)
    std::optional<std::string> page_title;        ///< Display title for persisted wiki page
};

// ============================================================================
// WikiQueryOptions
// ============================================================================

/**
 * @brief Options controlling retrieval from the LLM Wiki index.
 *
 * Passed to `ILLMWikiPlugin::query()` and `ILLMWikiPlugin::wikiQuery()`.
 */
struct WikiQueryOptions {
    int   top_k              = 5;     ///< Maximum number of chunks to return
    float min_score          = 0.0f;  ///< Minimum score threshold; lower results are dropped
    bool  save_as_page       = false; ///< Persist query result as a wiki page (workspace mode)
    std::optional<std::string> page_title;   ///< Title for the persisted page (if save_as_page)
    std::optional<std::string> workspace_root; ///< Workspace root for persistent query mode
};

// ============================================================================
// WikiIngestResult
// ============================================================================

/**
 * @brief Result returned by `ILLMWikiPlugin::ingest()`.
 */
struct WikiIngestResult {
    int         files_processed  = 0;   ///< Number of source files processed
    int         chunks_written   = 0;   ///< Number of chunks added to the index
    int         chunks_skipped   = 0;   ///< Chunks skipped (already present when skip_existing=true)
    int         errors           = 0;   ///< Number of files that failed to ingest
    std::vector<std::string> failed_files; ///< File paths that produced errors
    std::chrono::milliseconds duration{0}; ///< Wall-clock ingestion time
};

// ============================================================================
// WikiQueryResult
// ============================================================================

/**
 * @brief Result returned by `ILLMWikiPlugin::query()`.
 */
struct WikiQueryResult {
    std::vector<themis::llm::WikiChunk> candidates;  ///< Retrieved and scored chunks
    bool    query_flagged_for_prompt_injection = false; ///< True if the query matched a guardrail pattern
    int     filtered_unsafe_chunks             = 0;    ///< Chunks excluded by content guardrails
    std::string saved_page_path;                       ///< Non-empty when save_as_page=true and page was persisted
    std::chrono::milliseconds duration{0};             ///< Wall-clock query time
};

// ============================================================================
// WikiWorkspaceStats
// ============================================================================

/**
 * @brief Index and workspace statistics returned by `ILLMWikiPlugin::stats()`.
 */
struct WikiWorkspaceStats {
    int    total_chunks     = 0;   ///< Total chunks in the index
    int    total_docs       = 0;   ///< Unique source documents indexed
    int    wiki_pages       = 0;   ///< Wiki pages in persistent workspace (0 if no workspace)
    int    open_tasks       = 0;   ///< Unresolved review/contradiction tasks in workspace
    int    orphan_pages     = 0;   ///< Pages with no inbound links (lint result)
    bool   rocksdb_backed   = false; ///< True when WikiIndexStore Phase B is active
    std::string embedding_provider; ///< Active embedding provider name
};

// ============================================================================
// WikiLintResult
// ============================================================================

/**
 * @brief Result of a workspace lint check (`ILLMWikiPlugin::wikiLint()`).
 *
 * Lint is only meaningful when a persistent workspace is configured.
 */
struct WikiLintResult {
    std::vector<std::string> orphan_pages;           ///< Pages with no inbound links
    std::vector<std::string> missing_backlinks;      ///< Pages that reference non-existent targets
    std::vector<std::string> stale_synthesis_pages;  ///< Synthesis pages with outdated source refs
    std::vector<std::string> unresolved_tasks;       ///< Open contradiction-review tasks
};

// ============================================================================
// WikiDumpIngestOptions  (enterprise / Wikipedia ingestion)
// ============================================================================

/**
 * @brief Options for ingesting a Wikipedia XML dump into the wiki index.
 *
 * Wraps the C++ Wikipedia ingestion pipeline in `src/importers/wikipedia_*.cpp`.
 * This is an enterprise-only feature (not available in community / minimal builds).
 *
 * @note Dump paths may be local files or remote URIs (`http://`, `https://`).
 *       The pipeline supports resumable checkpointing via
 *       `WikipediaCheckpoint` when `checkpoint_dir` is set.
 */
struct WikiDumpIngestOptions {
    bool        recursive              = false;    ///< (unused for dump ingestion)
    std::string lang                   = "en";     ///< Wikipedia language code
    int         batch_size             = 1000;     ///< Articles per write batch
    int         max_articles           = 0;        ///< Hard limit (0 = unlimited)
    bool        project_vector         = true;     ///< Populate vector projections
    bool        project_graph          = true;     ///< Build graph link projections
    bool        project_timeseries     = false;    ///< Build temporal access projections
    std::optional<std::string> checkpoint_dir;     ///< Resumable-checkpoint working directory
    std::optional<std::string> filter_category;    ///< Restrict to a top-level Wikipedia category
};

// ============================================================================
// ILLMWikiPlugin
// ============================================================================

/**
 * @brief Primary interface for the LLM Wiki enterprise plugin.
 *
 * Extends `IThemisPlugin` with wiki-specific ingestion, retrieval, workspace
 * management, and Wikipedia dump ingestion capabilities.
 *
 * ### Edition gating
 *
 * All methods are available in `enterprise`, `hyperscaler`, and `military`
 * builds.  `ingestWikipediaDump()` additionally requires the optional
 * `llm_wiki_wikipedia` sub-feature (licensed separately for high-volume dumps).
 *
 * ### Lifecycle
 *
 * 1. Construct via plugin factory / `PluginManager::load()`.
 * 2. Call `initialize(config_json)` — sets up RocksDB table, embedding
 *    provider, and optional workspace root.
 * 3. Use `ingest()` / `query()` (or workspace variants).
 * 4. Call `shutdown()` before unloading.
 */
class ILLMWikiPlugin : public IThemisPlugin {
public:
    ~ILLMWikiPlugin() override = default;

    // ── IThemisPlugin metadata ────────────────────────────────────────────

    /// @return "llm_wiki"
    [[nodiscard]] virtual const char* pluginId()      const noexcept = 0;
    /// @return Human-readable name, e.g., "ThemisDB LLM Wiki Plugin v0.1.0"
    [[nodiscard]] virtual const char* pluginVersion() const noexcept = 0;
    [[nodiscard]] PluginType getType() const override { return PluginType::CUSTOM; }

    // ── Lifecycle ─────────────────────────────────────────────────────────

    /**
     * @brief Initialize the plugin from a JSON configuration blob.
     *
     * Required fields (all have defaults):
     *  - `"embedding_provider"` (`string`, default `"hash"`)
     *  - `"embedding_dim"` (`int`, default `128`)
     *  - `"table_name"` (`string`, default `"wiki_chunks"`)
     *  - `"workspace_root"` (`string`, optional)
     *  - `"rocksdb_dir"` (`string`, optional — activates Phase B if set)
     *
     * @param config_json  JSON object with plugin configuration.
     * @return             `Status::Ok()` on success; error status otherwise.
     */
    [[nodiscard]] virtual Status initialize(const std::string& config_json) = 0;

    /**
     * @brief Gracefully shut down the plugin and flush pending writes.
     *
     * Must be called before the shared library is unloaded or the plugin
     * pointer is released.
     */
    virtual void shutdown() noexcept = 0;

    // ── Core ingestion ─────────────────────────────────────────────────────

    /**
     * @brief Ingest documents from `source_path` into the wiki index.
     *
     * @param source_path  File or directory path to ingest.
     *                     Directories are traversed according to `opts.recursive`
     *                     and `opts.file_glob`.
     * @param opts         Ingestion options; defaults are production-safe.
     * @return             `WikiIngestResult` with counts and error list.
     */
    [[nodiscard]] virtual WikiIngestResult ingest(
        const std::string&    source_path,
        const WikiIngestOptions& opts = {}) = 0;

    // ── Core retrieval ─────────────────────────────────────────────────────

    /**
     * @brief Query the wiki index for chunks relevant to `query_text`.
     *
     * Retrieval strategy depends on the active backend:
     *  - Phase A (JSON/in-memory): BM25 TF token overlap + cosine KNN.
     *  - Phase B (RocksDB-native): `WikiIndexStore` BM25 + HNSW + RRF fusion.
     *
     * Guardrails:
     *  - Query is screened for prompt-injection patterns before retrieval.
     *  - Individual chunks are filtered if they match unsafe content patterns.
     *
     * @param query_text  Natural-language query string.
     * @param opts        Query options.
     * @return            `WikiQueryResult` with scored candidates and guardrail flags.
     */
    [[nodiscard]] virtual WikiQueryResult query(
        const std::string&    query_text,
        const WikiQueryOptions& opts = {}) = 0;

    // ── Persistent workspace ───────────────────────────────────────────────

    /**
     * @brief Initialize a persistent LLM Wiki workspace.
     *
     * Creates the canonical workspace directory structure:
     *  - `raw_sources/` — immutable source copies
     *  - `wiki/pages/` — LLM-maintained wiki pages
     *  - `wiki/index.md` — content catalog
     *  - `wiki/log.md` — append-only ingest/query/lint timeline
     *  - `wiki/schema.md` — maintenance rules
     *  - `wiki/state.json` — structured state (links, assertions, tasks)
     *
     * @param workspace_root  Absolute path for the workspace directory.
     * @return                `Status::Ok()` on success; error if the directory
     *                        cannot be created or is already incompatible.
     */
    [[nodiscard]] virtual Status wikiInit(const std::string& workspace_root) = 0;

    /**
     * @brief Ingest a single source into an initialized workspace.
     *
     * Appends a log entry, generates a summary page, and creates concept links
     * in `wiki/state.json`.  A raw copy is placed in `raw_sources/`.
     *
     * @param source_path    Source file path.
     * @param opts           Ingestion options; `opts.workspace_root` is mandatory.
     * @return               `WikiIngestResult`.
     */
    [[nodiscard]] virtual WikiIngestResult wikiIngest(
        const std::string&    source_path,
        const WikiIngestOptions& opts) = 0;

    /**
     * @brief Query an initialized workspace and optionally persist the answer.
     *
     * When `opts.save_as_page = true` a new page is written to
     * `wiki/pages/<slug>.md` and indexed.  The returned
     * `WikiQueryResult::saved_page_path` contains the path.
     *
     * @param query_text  Natural-language query string.
     * @param opts        Query options; `opts.workspace_root` is mandatory.
     * @return            `WikiQueryResult`.
     */
    [[nodiscard]] virtual WikiQueryResult wikiQuery(
        const std::string&    query_text,
        const WikiQueryOptions& opts) = 0;

    /**
     * @brief Run lint checks on a workspace and return diagnostic findings.
     *
     * Checks:
     *  - Orphan pages (no inbound links)
     *  - Missing backlinks
     *  - Stale synthesis pages (source refs older than `max_staleness_days`)
     *  - Unresolved contradiction-review tasks
     *
     * @param workspace_root     Absolute workspace root path.
     * @param max_staleness_days Pages whose source refs are older than this
     *                           are flagged as stale (default 30).
     * @return                   `WikiLintResult` with categorised findings.
     */
    [[nodiscard]] virtual WikiLintResult wikiLint(
        const std::string& workspace_root,
        int max_staleness_days = 30) = 0;

    // ── Wikipedia dump ingestion (enterprise sub-feature) ─────────────────

    /**
     * @brief Ingest a Wikipedia XML dump into the wiki index.
     *
     * Delegates to the C++ Wikipedia ingestion pipeline
     * (`src/importers/wikipedia_pipeline.cpp`).  Resumable via checkpoint
     * when `opts.checkpoint_dir` is set.
     *
     * @note Requires license sub-feature `"llm_wiki_wikipedia"`.
     *
     * @param dump_path  Local path or `http(s)://` URI of the compressed
     *                   Wikipedia dump file (`*-pages-articles.xml.bz2`).
     * @param opts       Dump ingestion options.
     * @return           `WikiIngestResult` with article/chunk counts.
     */
    [[nodiscard]] virtual WikiIngestResult ingestWikipediaDump(
        const std::string&         dump_path,
        const WikiDumpIngestOptions& opts = {}) = 0;

    // ── Statistics ────────────────────────────────────────────────────────

    /**
     * @brief Return current index and workspace statistics.
     *
     * @param workspace_root  Optional workspace root path; when empty, only
     *                        index-level stats are returned.
     * @return                `WikiWorkspaceStats`.
     */
    [[nodiscard]] virtual WikiWorkspaceStats stats(
        const std::string& workspace_root = {}) = 0;
};

// ============================================================================
// Factory helpers
// ============================================================================

/**
 * @brief Shared-library entry point symbol for the LLM Wiki plugin.
 *
 * Every shared library built from the private plugin repository MUST export
 * this function with C linkage:
 *
 * @code
 *   extern "C" THEMIS_PLUGIN_EXPORT
 *   std::shared_ptr<ILLMWikiPlugin> themisdb_llm_wiki_create();
 * @endcode
 */
using LLMWikiPluginFactory = std::shared_ptr<ILLMWikiPlugin> (*)();

/// @brief Canonical export symbol name expected by the plugin loader.
inline constexpr const char* kLLMWikiPluginFactorySymbol =
    "themisdb_llm_wiki_create";

/// @brief Canonical plugin identifier passed to `PluginManager::load()`.
inline constexpr const char* kLLMWikiPluginId = "llm_wiki";

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
