/**
 * @file workflow_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/ingestion_step.h"
#include "ingestion/extraction_context.h"
#include "ingestion/base_entity.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// FilePattern — file selection criteria for a workflow profile
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Criteria that determine which files activate a `WorkflowProfile`.
 *
 * A file matches when:
 *  - Its MIME type appears in `mime_types` (or `mime_types` is empty), AND
 *  - Its filename matches at least one glob in `filename_patterns` (or
 *    `filename_patterns` is empty).
 *
 * An empty `FilePattern` matches every file (used by `default.yaml`).
 */
struct FilePattern {
    std::vector<std::string> mime_types;
    ///< e.g. ["application/pdf", "text/html"]
    ///< Prefix wildcard supported: "application/vnd.openxmlformats*"

    std::vector<std::string> filename_patterns;
    ///< Shell-style glob patterns, e.g. ["*Gesetz*", "*Verordnung*"]

    /// Returns true when the file described by the arguments matches.
    bool matches(const std::string& mime, const std::string& filename) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowProfile — a loaded YAML ingestion workflow
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A parsed, validated ingestion workflow profile.
 *
 * Loaded from a YAML file that conforms to the `IngestionWorkflow` schema
 * (apiVersion: themis.ingestion/v1, kind: IngestionWorkflow).
 */
struct WorkflowProfile {
    std::string api_version;        ///< "themis.ingestion/v1"
    std::string kind;               ///< "IngestionWorkflow"
    std::string name;               ///< Profile name, e.g. "legal-document-de"
    std::string description;

    FilePattern file_patterns;      ///< Which files activate this profile

    std::vector<StepConfig> steps;  ///< Ordered list of steps

    // ── Output / quality gate ──────────────────────────────────────────────
    bool output_graph{true};
    bool output_vector{true};
    bool output_document_store{true};
    std::uint32_t quality_gate_min_entities{1};
    double        quality_gate_min_quality_score{0.0};

    // ── Internal metadata ──────────────────────────────────────────────────
    std::string source_path;        ///< Filesystem path of the YAML file (for diagnostics)
};

// ─────────────────────────────────────────────────────────────────────────────
// StepPluginManifest — sandbox configuration for DLL step plugins
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Sandbox manifest for DLL step plugin loading.
 *
 * Implements §Phase 3 DLL step plugin sandbox (ROADMAP Q3 2026).
 * Before `loadStepPlugin()` dlopen()s any library, the manifest is
 * validated:
 *   1. The library path must be under one of the `allowed_paths` prefixes.
 *   2. If `allowed_mime_types` is non-empty, the plugin's reported MIME
 *      type (declared in the manifest, read from a sidecar `.manifest.json`
 *      file next to the .so) must be in the allowed set.
 *
 * An empty `StepPluginManifest` (default) is permissive — all paths and
 * MIME types are accepted (backwards compatibility).
 */
struct StepPluginManifest {
    /// Directory prefixes that are allowed for plugin .so files.
    /// Matched as path prefix (canonical). Empty → any path allowed.
    std::vector<std::string> allowed_paths;

    /// MIME type allowlist for the plugin. Matched against
    /// the "mime_type" key in the sidecar <plugin>.manifest.json.
    /// Empty → any MIME type allowed.
    std::vector<std::string> allowed_mime_types;
};

// ─────────────────────────────────────────────────────────────────────────────
// StepRegistry — name → IIngestionStep factory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe registry that maps step plugin names to instances.
 *
 * Built-in steps are registered at startup by `WorkflowEngine`.  Dynamic
 * steps (`.so` / `.dll`) are loaded on demand via `loadStepPlugin()`.
 *
 * Naming convention: `builtin.<name>` for bundled steps,
 * `<vendor>.<name>` for third-party plugins.
 */
class StepRegistry {
public:
    StepRegistry();
    ~StepRegistry();

    // Delete copy
    StepRegistry(const StepRegistry&) = delete;
    StepRegistry& operator=(const StepRegistry&) = delete;

    /**
     * @brief Register a pre-built step instance.
     *
     * @param plugin_name  Fully qualified name (e.g. "builtin.parse_text")
     * @param step         Shared pointer to the step implementation.
     * @return Error if a step with that name is already registered.
     */
    Result<void> registerStep(const std::string& plugin_name,
                               std::shared_ptr<IIngestionStep> step);

    /**
     * @brief Load a dynamic step plugin from a shared library.
     *
     * Calls `themis_create_step()` from the .so/.dll.
     * If @p manifest is provided, the library path and MIME type are
     * validated before dlopen() (§Phase 3 DLL sandbox).
     *
     * @param plugin_name  Logical name to register the plugin under.
     * @param library_path Absolute filesystem path to the shared library.
     * @param manifest     Optional sandbox constraints.  Empty manifest
     *                     (default) permits any path and MIME type.
     * @return Error when the library cannot be loaded, the entry point is
     *         missing, or sandbox validation fails.
     */
    Result<void> loadStepPlugin(const std::string& plugin_name,
                                const std::string& library_path,
                                const StepPluginManifest& manifest = {});

    /**
     * @brief Retrieve a registered step by name.
     *
     * @return Shared pointer or nullptr when not found.
     */
    std::shared_ptr<IIngestionStep> getStep(const std::string& plugin_name) const;

    /**
     * @brief Returns true when a step with the given name is registered.
     */
    bool hasStep(const std::string& plugin_name) const;

    /**
     * @brief List all registered step names.
     */
    std::vector<std::string> listSteps() const;

    /**
     * @brief Unload and remove a step from the registry.
     *
     * For dynamically loaded plugins this also calls `themis_destroy_step()`
     * and closes the library handle.
     */
    Result<void> unloadStep(const std::string& plugin_name);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowEngine — the orchestrator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief YAML-driven workflow orchestrator for the ingestion pipeline.
 *
 * The `WorkflowEngine` is the core of the v2.0 ingestion architecture.  It:
 *  1. Loads and validates YAML workflow profiles from disk.
 *  2. Selects the best-matching profile for each incoming file.
 *  3. Executes the ordered step list, passing an `ExtractionContext` through
 *     each step.
 *  4. Returns a `BaseEntitySet` ready for writing to the sinks.
 *
 * Built-in steps are registered automatically during construction.  Additional
 * built-ins and DLL steps can be registered at any time before execution.
 *
 * Dependency injection
 * ────────────────────
 * The engine itself has no knowledge of LLM backends, document stores, or
 * graph writers.  Steps receive what they need through `StepConfig::config`
 * (for static YAML parameters) or through their own DI mechanism.  The
 * `IngestionManager` wires the LLM backend into steps that require it via
 * `setTextGenerationBackend()` before calling `WorkflowEngine::execute()`.
 *
 * Thread-safety
 * ─────────────
 * `loadProfile()` and `execute()` are safe to call concurrently from multiple
 * threads.  Profile loading is idempotent when called with the same path
 * (the second call is a no-op).
 *
 * Profile selection
 * ─────────────────
 * Profiles are evaluated in registration order; the first matching profile
 * wins.  If none match, the profile named "default" is used.  If no default
 * is loaded, `execute()` returns `ERR_WORKFLOW_NO_MATCHING_PROFILE`.
 *
 * Example bootstrap
 * ─────────────────
 * @code
 * auto engine = std::make_shared<WorkflowEngine>();
 * engine->loadProfile("/config/ingestion/workflows/legal-document-de.yaml");
 * engine->loadProfile("/config/ingestion/workflows/default.yaml");
 * mgr.setWorkflowEngine(engine);
 * @endcode
 */
class WorkflowEngine {
public:
    WorkflowEngine();
    ~WorkflowEngine();

    // Delete copy
    WorkflowEngine(const WorkflowEngine&) = delete;
    WorkflowEngine& operator=(const WorkflowEngine&) = delete;

    // ── Profile management ─────────────────────────────────────────────────

    /**
     * @brief Load and validate a YAML workflow profile.
     *
     * If a profile with the same name is already loaded the call is a no-op.
     *
     * @param yaml_path  Absolute filesystem path to the profile YAML.
     * @return Error on I/O failure, YAML parse failure, or schema violation.
     */
    Result<void> loadProfile(const std::string& yaml_path);

    /**
     * @brief Load all `*.yaml` profile files from a directory.
     *
     * Calls `loadProfile()` for each file; errors per-file are logged as
     * warnings and do not abort the batch.
     *
     * @param directory_path  Directory to scan.
     * @return Number of profiles successfully loaded.
     */
    std::size_t loadProfilesFromDirectory(const std::string& directory_path);

    /**
     * @brief Return the loaded profile that best matches `mime` and `filename`.
     *
     * Returns nullptr when no profile matches and no "default" profile exists.
     */
    const WorkflowProfile* selectProfile(const std::string& mime,
                                          const std::string& filename) const;

    /**
     * @brief List the names of all loaded profiles.
     */
    std::vector<std::string> listProfiles() const;

    // ── Step registry access ───────────────────────────────────────────────

    /**
     * @brief Access the underlying step registry for custom registrations.
     *
     * Primarily used by tests and bootstrap code.
     */
    StepRegistry& stepRegistry();
    const StepRegistry& stepRegistry() const;

    // ── Execution ─────────────────────────────────────────────────────────

    /**
     * @brief Run the workflow against an already-constructed `ExtractionContext`.
     *
     * The engine selects a profile based on `ctx.manifest.detected_mime` and
     * `ctx.manifest.filename_stem + ctx.manifest.extension`, then executes
     * each step in order.
     *
     * @param ctx  Pipeline context carrying the `FileManifest`.  Modified
     *             in-place by each executed step.
     * @return Assembled `BaseEntitySet` on success, or an error.
     */
    Result<BaseEntitySet> execute(ExtractionContext& ctx);

    /**
     * @brief Run the workflow using an explicitly named profile (bypass auto-select).
     *
     * @param profile_name  Name of the profile to use (must be loaded).
     * @param ctx           Pipeline context.
     * @return Assembled `BaseEntitySet` on success, or an error.
     */
    Result<BaseEntitySet> executeWithProfile(const std::string& profile_name,
                                              ExtractionContext& ctx);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis

