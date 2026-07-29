/**
 * @file wiki_workspace_orchestrator.h
 * @brief C++ port of the Python MVP workspace orchestrator for the LLM Wiki plugin.
 *
 * Manages the persistent wiki workspace directory structure:
 *  - `raw_sources/`   — immutable copies of ingested source files
 *  - `wiki/pages/`    — per-document and per-query wiki pages
 *  - `wiki/index.md`  — human-readable content catalog
 *  - `wiki/log.md`    — append-only ingest/query/lint timeline
 *  - `wiki/schema.md` — maintenance rules
 *  - `wiki/state.json`— structured state (pages, links, assertions, tasks)
 *
 * ## state.json schema (version "wiki-cpp-1")
 * @code{.json}
 * {
 *   "version":    "wiki-cpp-1",
 *   "pages":      { "<slug>": { "title":"...", "source":"...",
 *                               "created":"...", "updated":"..." } },
 *   "links":      [ { "from":"...", "to":"...", "type":"concept" } ],
 *   "assertions": [ { "id":"...", "text":"...", "source":"..." } ],
 *   "tasks":      [ { "id":"...", "type":"contradiction_review",
 *                     "status":"open", "refs":[...] } ]
 * }
 * @endcode
 *
 * ## Thread safety
 * All public methods acquire `mutex_` exclusively; callers may invoke them from
 * any thread.
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA (Phase 2)
 * @note Edition: enterprise / hyperscaler / military
 */

#pragma once

#include "llm_wiki/llm_wiki_plugin_interface.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace llm_wiki {

// ============================================================================
// Internal state types (not part of the public SDK)
// ============================================================================

/// @brief Metadata for a single wiki page entry in state.json.
struct WikiPageMeta {
    std::string title;    ///< Human-readable page title
    std::string source;   ///< Originating source file path
    std::string created;  ///< ISO-8601 creation timestamp
    std::string updated;  ///< ISO-8601 last-updated timestamp
};

/// @brief A directed conceptual link between two page slugs.
struct WikiLink {
    std::string from; ///< Source page slug
    std::string to;   ///< Target page slug
    std::string type = "concept"; ///< Link type (currently always "concept")
};

/// @brief A factual assertion associated with a source.
struct WikiAssertion {
    std::string id;     ///< Stable identifier
    std::string text;   ///< Assertion text
    std::string source; ///< Source page slug or path
};

/// @brief A review task created by the contradiction detector or manually.
struct WikiTask {
    std::string id;              ///< Unique task identifier
    std::string type;            ///< Task type, e.g. "contradiction_review"
    std::string status = "open"; ///< "open" or "resolved"
    std::vector<std::string> refs; ///< Referenced page slugs
};

/// @brief Full parsed representation of wiki/state.json.
struct WikiState {
    std::string version = "wiki-cpp-1";
    std::map<std::string, WikiPageMeta> pages;
    std::vector<WikiLink>        links;
    std::vector<WikiAssertion>   assertions;
    std::vector<WikiTask>        tasks;
};

// ============================================================================
// WikiWorkspaceOrchestrator
// ============================================================================

/**
 * @brief Manages the file-system lifecycle of a persistent LLM Wiki workspace.
 *
 * Handles directory initialisation, source file copies, wiki page generation,
 * append-only log maintenance, state.json updates (with atomic rename writes),
 * contradiction-task creation, and lint/stats queries.
 *
 * CONTRADICTION_CUES checked during ingestion:
 *   "however", "but", "contradict", "in contrast", "on the other hand"
 */
class WikiWorkspaceOrchestrator {
public:
    WikiWorkspaceOrchestrator()  = default;
    ~WikiWorkspaceOrchestrator() = default;

    // Non-copyable, movable
    WikiWorkspaceOrchestrator(const WikiWorkspaceOrchestrator&)            = delete;
    WikiWorkspaceOrchestrator& operator=(const WikiWorkspaceOrchestrator&) = delete;
    WikiWorkspaceOrchestrator(WikiWorkspaceOrchestrator&&)                 = default;
    WikiWorkspaceOrchestrator& operator=(WikiWorkspaceOrchestrator&&)      = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Initialise a workspace at `workspace_root`.
     *
     * Creates `raw_sources/`, `wiki/`, `wiki/pages/`, and, if absent, the
     * initial scaffold files (`schema.md`, `index.md`, `log.md`, `state.json`).
     * Safe to call on an already-initialised workspace — existing files are
     * not overwritten.
     *
     * @param workspace_root  Absolute path for the workspace root directory.
     * @return                `Status::Ok()` on success; error status otherwise.
     */
    [[nodiscard]] Status init(const std::string& workspace_root);

    // ── Ingestion ─────────────────────────────────────────────────────────────

    /**
     * @brief Record a source ingestion into the workspace.
     *
     * Copies the source file to `raw_sources/<slug>`, creates a wiki page at
     * `wiki/pages/<slug>.md` with a source summary and chunk excerpts, updates
     * `wiki/state.json` (page entry, concept links from heading adjacency,
     * contradiction tasks if CONTRADICTION_CUES found), refreshes `wiki/index.md`,
     * and appends a log entry.
     *
     * @param source_path  Path of the file that was ingested.
     * @param opts         Ingestion options (used for `page_title` override).
     * @param chunks       Chunks produced by `WikiChunkSplitter::split()`.
     * @return             `WikiIngestResult` with page/chunk counts.
     */
    [[nodiscard]] WikiIngestResult ingest(
        const std::string&                     source_path,
        const WikiIngestOptions&               opts,
        const std::vector<themis::llm::WikiChunk>& chunks);

    // ── Retrieval ─────────────────────────────────────────────────────────────

    /**
     * @brief Retrieve wiki chunks and optionally persist the result as a page.
     *
     * Calls `reader.query(query_text, opts.top_k, opts.min_score)` to obtain
     * candidates.  When `opts.save_as_page = true`, writes the result to
     * `wiki/pages/<slug>.md` and sets `WikiQueryResult::saved_page_path`.
     * Appends a log entry regardless.
     *
     * @param query_text  Natural-language query string.
     * @param opts        Query options; must have `workspace_root` set or
     *                    `workspace_root_` initialised by `init()`.
     * @param reader      Active index reader (Phase A or Phase B).
     * @return            `WikiQueryResult` with retrieved chunks.
     */
    [[nodiscard]] WikiQueryResult query(
        const std::string&           query_text,
        const WikiQueryOptions&      opts,
        themis::llm::IWikiIndexReader& reader);

    // ── Lint ──────────────────────────────────────────────────────────────────

    /**
     * @brief Run lint checks on the workspace at `workspace_root`.
     *
     * Checks (all loaded from `wiki/state.json`):
     *  - Orphan pages: pages with no inbound links
     *  - Missing backlinks: link targets not present in `pages`
     *  - Stale synthesis pages: `updated` timestamp older than `max_staleness_days`
     *  - Unresolved tasks: tasks with `status == "open"`
     *
     * @param workspace_root     Workspace root path (need not be the same as
     *                           the workspace this instance was `init()`-ed with).
     * @param max_staleness_days Pages older than this are flagged stale.
     * @return                   `WikiLintResult` with categorised findings.
     */
    [[nodiscard]] WikiLintResult lint(
        const std::string& workspace_root,
        int max_staleness_days = 30);

    // ── Stats ─────────────────────────────────────────────────────────────────

    /**
     * @brief Return workspace-level statistics (page/task/orphan counts).
     *
     * @param workspace_root  Workspace root to query.
     * @return                Partial `WikiWorkspaceStats` (index-level fields
     *                        such as `total_chunks` are left at zero; the caller
     *                        fills them in).
     */
    [[nodiscard]] WikiWorkspaceStats stats(const std::string& workspace_root);

private:
    // ── Helpers ───────────────────────────────────────────────────────────────

    /// @brief Derive a URL-safe slug from a title or file path.
    [[nodiscard]] static std::string slugify(const std::string& text);

    /// @brief Return the current UTC time formatted as ISO-8601 (YYYY-MM-DDTHH:MM:SSZ).
    [[nodiscard]] static std::string isoTimestamp();

    /// @brief Load and parse `<root>/wiki/state.json`; returns default state on error.
    [[nodiscard]] static WikiState loadState(const std::string& root);

    /// @brief Serialise `state` to `<root>/wiki/state.json` atomically (write+rename).
    static void saveState(const std::string& root, const WikiState& state);

    /// @brief Append a single log entry line to `<root>/wiki/log.md`.
    static void appendLog(const std::string& root, const std::string& entry);

    /// @brief Rebuild `<root>/wiki/index.md` from the current state.
    static void rebuildIndex(const std::string& root, const WikiState& state);

    /// @brief Check whether `text` contains any CONTRADICTION_CUE (case-insensitive).
    [[nodiscard]] static bool hasContradictionCue(const std::string& text);

    mutable std::mutex mutex_;   ///< Serialises all public method calls
    std::string workspace_root_; ///< Workspace root set by init()
};

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
