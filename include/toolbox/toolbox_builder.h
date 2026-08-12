/**
 * @file toolbox_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.2.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/ingestion_sinks.h"
#include "ingestion/inference_backend.h"
#include "ingestion/workflow_engine.h"
#include "ingestion/format_extractor.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations — full types provided by the bridge headers only inside
// toolbox_builder.cpp.  This avoids a cyclic include:
//   toolbox_builder.h -> aql/aql_ingestion_bridge.h -> toolbox/ingestion_toolbox.h
//   toolbox_builder.h -> rag/rag_ingestion_bridge.h -> toolbox/ingestion_toolbox.h
namespace themis {
namespace aql   { class AQLIngestionBridge; }
namespace rag   { class RAGIngestionBridge; }
} // namespace themis

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
    // ─────────────────────────────────────────────────────────────────────────
    // BuiltToolbox — result of buildWithBridges()
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Aggregates the toolbox and its auto-wired bridge instances.
     *
     * Fields are set according to the sinks provided to the builder:
     *
     * | Field         | Condition                                               |
     * |---------------|---------------------------------------------------------|
     * | `toolbox`     | Always non-null.                                        |
     * | `aql_bridge`  | Non-null when a graph-writer was registered.            |
     * | `rag_bridge`  | Non-null when a vector-writer or graph-writer was set.  |
     *
     * ## Destructor
     * Declared out-of-line in `toolbox_builder.cpp` where the full
     * `AQLIngestionBridge` / `RAGIngestionBridge` types are visible.
     */
    struct BuiltToolbox {
        std::shared_ptr<IngestionToolbox>             toolbox;
        std::shared_ptr<aql::AQLIngestionBridge>      aql_bridge;
        std::shared_ptr<rag::RAGIngestionBridge>      rag_bridge;

        BuiltToolbox();
        ~BuiltToolbox();
        BuiltToolbox(BuiltToolbox&&) noexcept;
        BuiltToolbox& operator=(BuiltToolbox&&) noexcept;
        BuiltToolbox(const BuiltToolbox&) = default;
        BuiltToolbox& operator=(const BuiltToolbox&) = default;
    };

    // ─────────────────────────────────────────────────────────────────────────

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
     * @brief Attach a vector-writer sink.
     *
     * When set and `buildWithBridges()` is used, the resulting
     * `RAGIngestionBridge` will write embedding chunks to this sink.
     *
     * @param writer  Vector-store sink.  May be null to clear a previously
     *                set writer.
     * @return `*this` for chaining.
     */
    ToolboxBuilder& withVectorWriter(
        std::shared_ptr<ingestion::IVectorWriter> writer);

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

    /**
     * @brief Register a format extractor and wire it into the corresponding
     *        builtin parse step.
     *
     * Calling this multiple times with different extractors registers all of
     * them.  For each extractor, the matching builtin step is created and
     * registered in the `StepRegistry` during `build()`:
     *
     *  - `IFormatExtractor` supporting `application/pdf`    → `builtin.parse_pdf`
     *  - Supporting `application/vnd.openxmlformats-*`      → `builtin.parse_office`
     *  - Supporting `image/\*`                               → `builtin.parse_image`
     *  - Supporting `application/zip` / `application/x-tar` → `builtin.parse_archive`
     *  - Supporting `audio/\*`                               → `builtin.parse_audio`
     *
     * When a `FormatExtractorFactory` (from `content/adapters/`) is available,
     * use `withFormatExtractorFactory()` to register all extractors at once.
     *
     * @param extractor  Format extractor to register.  Must not be null.
     * @return `*this` for chaining.
     * @throws std::invalid_argument if @p extractor is null.
     */
    ToolboxBuilder& withFormatExtractor(
        std::shared_ptr<ingestion::IFormatExtractor> extractor);

    /**
     * @brief Register all format extractors from a factory at once.
     *
     * Iterates over `factory->registeredMimeTypes()` and calls
     * `withFormatExtractor()` for each distinct extractor.  This is the
     * preferred way to wire a `content::adapters::FormatExtractorFactory`
     * into the toolbox.
     *
     * @param factory  Pre-populated factory.  Must not be null.
     * @return `*this` for chaining.
     */
    ToolboxBuilder& withFormatExtractorFactory(
        std::shared_ptr<ingestion::IFormatExtractorFactory> factory);

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

    /**
     * @brief Construct the toolbox and auto-wire `AQLIngestionBridge` and
     *        `RAGIngestionBridge` from the registered sinks.
     *
     * Performs the same steps as `build()`, then creates:
     *
     * - `AQLIngestionBridge(toolbox, graph_writer)` when a graph-writer has
     *   been set via `withGraphWriter()`.
     * - `RAGIngestionBridge(toolbox, vector_writer, graph_writer)` when a
     *   vector-writer or graph-writer has been set.
     *
     * @return `BuiltToolbox` with all applicable fields populated.
     * @throws std::logic_error if `build()` or `buildWithBridges()` has
     *         already been called on this builder instance.
     */
    [[nodiscard]] BuiltToolbox buildWithBridges();

    // ── Accessors (for inspection / testing) ─────────────────────────────────

    /**
     * @brief Return the graph-writer that was set via `withGraphWriter()`.
     * May return null if no writer was configured.
     */
    std::shared_ptr<ingestion::IGraphWriter> graphWriter() const;

    /**
     * @brief Return the vector-writer that was set via `withVectorWriter()`.
     * May return null if no writer was configured.
     */
    std::shared_ptr<ingestion::IVectorWriter> vectorWriter() const;

    /**
     * @brief Return the number of profile paths registered.
     */
    std::size_t profileCount() const noexcept;

    /**
     * @brief Inject a real `ITensorDecompositionBackend` for TT-core production.
     *
     * Re-registers the `builtin.chunk_tt_decompose` step with the supplied
     * backend, replacing the default `NullTensorDecompositionBackend`.
     * Pass `nullptr` to restore the no-op fallback.
     *
     * @param backend  A `TensorIngestionBridge` (or any custom implementation).
     * @return `*this` for chaining.
     */
    ToolboxBuilder& withTensorDecompositionBackend(
        std::shared_ptr<ingestion::ITensorDecompositionBackend> backend);

    /**
     * @brief Inject a real `ITensorCoreBridge` for durable TT-core storage.
     *
     * Re-registers the `builtin.tensor_core_bridge` step with the supplied
     * sink, replacing the default no-op fallback.
     * Pass `nullptr` to restore the no-op fallback.
     *
     * @param sink  A `TensorCoreStorageBridge` (or any custom implementation).
     * @return `*this` for chaining.
     */
    ToolboxBuilder& withTensorCoreSink(
        std::shared_ptr<ingestion::ITensorCoreBridge> sink);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace toolbox
} // namespace themis
