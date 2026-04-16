/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            toolbox_builder.h                                  ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-16                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/ingestion_sinks.h"
#include "ingestion/inference_backend.h"
#include "ingestion/workflow_engine.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxBuilder — fluent API for constructing IngestionToolbox instances
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fluent builder for constructing production `IngestionToolbox`
 *        instances with custom workflow profiles, sinks, and backends.
 *
 * ## Motivation
 *
 * `IngestionToolbox::createDefault()` is suitable for simple integration
 * scenarios.  Production deployments need finer control: loading workflow
 * profiles from disk, injecting a real LLM text-generation backend, and
 * attaching graph-writer or vector-writer sinks.  `ToolboxBuilder` provides
 * this control without exposing internal construction details.
 *
 * ## Usage
 * @code
 * auto toolbox = themis::toolbox::ToolboxBuilder()
 *     .withWorkflowProfile("/etc/themis/profiles/legal.yaml")
 *     .withTextBackend(llm_bridge)
 *     .withGraphWriter(graph_sink)
 *     .build();
 * @endcode
 *
 * Each `with*()` call returns a reference to `*this` for chaining.
 * A builder instance can only be used to call `build()` once; calling
 * `build()` again throws `std::logic_error`.
 *
 * ## Dependency direction
 * @code
 *   toolbox/  →  ingestion/
 * @endcode
 * `toolbox/` must never be imported by `ingestion/`.
 *
 * ## Thread-safety
 * `ToolboxBuilder` instances are not thread-safe; all configuration calls
 * and the final `build()` call must happen on the same thread.  The
 * resulting `IngestionToolbox` is fully thread-safe.
 */
class ToolboxBuilder {
public:
    ToolboxBuilder();
    ~ToolboxBuilder();

    // Non-copyable, movable
    ToolboxBuilder(const ToolboxBuilder&) = delete;
    ToolboxBuilder& operator=(const ToolboxBuilder&) = delete;
    ToolboxBuilder(ToolboxBuilder&&) noexcept;
    ToolboxBuilder& operator=(ToolboxBuilder&&) noexcept;

    // ── Configuration ─────────────────────────────────────────────────────────

    /**
     * @brief Add a workflow profile file to load during `build()`.
     *
     * Multiple profiles can be added; they are loaded in registration order.
     * Each profile is loaded via `WorkflowEngine::loadProfile(path)`.
     *
     * @param profile_path  Absolute or relative path to a YAML workflow
     *                      profile file.
     * @return `*this` for chaining.
     * @throws std::invalid_argument if @p profile_path is empty.
     */
    ToolboxBuilder& withWorkflowProfile(std::string profile_path);

    /**
     * @brief Attach a graph-writer sink.
     *
     * When set, the `AQLIngestionBridge` and `RAGIngestionBridge` that wrap
     * this toolbox will write extracted entity nodes and relation edges to
     * the provided sink.
     *
     * @param writer  Graph-store sink.  May be null to clear a previously
     *                set writer.
     * @return `*this` for chaining.
     */
    ToolboxBuilder& withGraphWriter(
        std::shared_ptr<ingestion::IGraphWriter> writer);

    /**
     * @brief Inject a text-generation backend.
     *
     * Replaces the default `NullTextGenerationBackend`.  The backend is
     * forwarded to `IngestionToolbox::setTextBackend()` which propagates it
     * to all registered NER / LLM steps.
     *
     * @param backend  LLM text-generation implementation.  Passing
     *                 `nullptr` reinstates the null backend.
     * @return `*this` for chaining.
     */
    ToolboxBuilder& withTextBackend(
        std::shared_ptr<ingestion::ITextGenerationBackend> backend);

    /**
     * @brief Use a pre-constructed `WorkflowEngine` instead of the default.
     *
     * Useful for test isolation or for engines pre-loaded with custom step
     * implementations.  Profile paths added via `withWorkflowProfile()` are
     * still loaded on top of the injected engine.
     *
     * @param engine  Custom engine.  Must not be null.
     * @return `*this` for chaining.
     * @throws std::invalid_argument if @p engine is null.
     */
    ToolboxBuilder& withWorkflowEngine(
        std::shared_ptr<ingestion::WorkflowEngine> engine);

    // ── Build ─────────────────────────────────────────────────────────────────

    /**
     * @brief Construct and return a fully configured `IngestionToolbox`.
     *
     * Steps performed in order:
     *  1. Create or reuse a `WorkflowEngine` (from `withWorkflowEngine()` or
     *     via `IngestionToolbox::createDefault()`).
     *  2. Load each workflow profile added via `withWorkflowProfile()`.
     *  3. Inject the text-generation backend if one was provided.
     *  4. Return the configured toolbox instance.
     *
     * Profile load failures are logged at `warn` level; all loadable profiles
     * are applied and the toolbox is returned even when some profiles fail.
     *
     * @return Fully configured `shared_ptr<IngestionToolbox>`.
     * @throws std::logic_error if `build()` has already been called on this
     *         builder instance.
     */
    [[nodiscard]] std::shared_ptr<IngestionToolbox> build();

    // ── Accessors (for inspection / testing) ─────────────────────────────────

    /**
     * @brief Return the graph-writer that was set via `withGraphWriter()`.
     * May return null if no writer was configured.
     */
    std::shared_ptr<ingestion::IGraphWriter> graphWriter() const;

    /**
     * @brief Return the number of profile paths registered.
     */
    std::size_t profileCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace toolbox
} // namespace themis
