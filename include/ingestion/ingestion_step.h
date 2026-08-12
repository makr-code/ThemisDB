/**
 * @file ingestion_step.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/extraction_context.h"
#include "plugins/plugin_interface.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// StepConfig — per-step YAML configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Runtime configuration for a single workflow step.
 *
 * In the YAML profile each step entry looks like:
 * @code
 * - name: parse_text
 *   plugin: builtin.parse_text
 *   condition: ""        # optional JMESPath-like expression
 *   on_failure: skip     # "skip" | "abort" | "quarantine"
 *   parallel: false
 *   config:
 *     ocr_enabled: true
 *     ocr_language: "deu+eng"
 * @endcode
 *
 * The `config` sub-object is passed as raw JSON to the step so that each step
 * can parse only the fields it needs without a shared coupling to a central
 * config type.
 */
struct StepConfig {
    std::string  name;           ///< Step name (unique within the workflow)
    std::string  plugin;         ///< Fully qualified plugin name (e.g. "builtin.parse_text")
    std::string  condition;      ///< Optional guard expression; empty means "always run"
    std::string  on_failure;     ///< "skip" | "abort" | "quarantine" (default: "abort")
    bool         parallel{false};///< True when this step may run concurrently with siblings
    nlohmann::json config;       ///< Raw JSON config block forwarded to the step

    /// Returns true when `on_failure == "skip"`.
    bool skipOnFailure() const { return on_failure == "skip"; }
    /// Returns true when `on_failure == "quarantine"`.
    bool quarantineOnFailure() const { return on_failure == "quarantine"; }
};

// ─────────────────────────────────────────────────────────────────────────────
// IIngestionStep — the plugin interface every step implements
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Base interface for all workflow step plugins.
 *
 * Every step — whether built-in or provided as a dynamic plugin (.so/.dll) —
 * implements this interface.  The `WorkflowEngine` calls steps in order,
 * passing the same `ExtractionContext` by reference so each step can both read
 * prior results and append its own.
 *
 * Plugin type
 * ──────────
 * `getType()` must return `PluginType::INGESTION_STEP`.
 *
 * Capability check
 * ────────────────
 * The engine calls `canHandle()` before `execute()`.  If `canHandle()` returns
 * false the step is skipped (no error).  This lets a step declare itself as
 * "MIME-specific" without requiring a `condition:` expression in the YAML.
 *
 * Thread-safety
 * ─────────────
 * Implementations must be thread-safe if `parallel: true` is used.  The
 * context passed to parallel steps is a deep copy; the engine merges results
 * after all siblings complete.
 *
 * C ABI / dynamic loading
 * ───────────────────────
 * Dynamic step plugins export:
 * @code
 *   extern "C" IIngestionStep* themis_create_step();
 *   extern "C" void            themis_destroy_step(IIngestionStep*);
 * @endcode
 * The `StepRegistry` uses these symbols when loading `.so` / `.dll` files.
 */
class IIngestionStep : public themis::plugins::IThemisPlugin {
public:
    ~IIngestionStep() override = default;

    // ── IThemisPlugin overrides ────────────────────────────────────────────────

    /**
     * @brief Must return `PluginType::INGESTION_STEP`.
     */
    themis::plugins::PluginType getType() const override {
        return themis::plugins::PluginType::INGESTION_STEP;
    }

    // ── Step-specific interface ────────────────────────────────────────────────

    /**
     * @brief Execute the step, enriching `ctx` in-place.
     *
     * The step reads any fields it needs from `ctx` and appends results to the
     * appropriate collections (e.g. `ctx.entities`, `ctx.chunks`).  It must
     * not clear or overwrite data written by preceding steps.
     *
     * @param ctx     Shared pipeline context — modified in-place.
     * @param config  Step configuration parsed from the YAML profile.
     * @return `Result<void>` — success, or an error with an `ErrorCode` from
     *         the `ERR_WORKFLOW_*` range.
     */
    [[nodiscard]] virtual Result<void> execute(ExtractionContext& ctx,
                                 const StepConfig& config) = 0;

    /**
     * @brief Returns the MIME types this step can process.
     *
     * An empty vector means "any MIME type".  Used by `canHandle()` default
     * implementation.  Override `canHandle()` directly for more complex logic.
     */
    [[nodiscard]] virtual std::vector<std::string> supportedMimeTypes() const = 0;

    /**
     * @brief Returns true when this step can process the current context.
     *
     * Default: returns true when `supportedMimeTypes()` is empty (all MIMEs
     * accepted) or when `ctx.manifest.detected_mime` matches any entry in
     * `supportedMimeTypes()`.
     *
     * Override for custom capability checks (e.g. "only run if raw_text is
     * empty", "only run for files < 10 MB").
     */
    virtual bool canHandle(const ExtractionContext& ctx) const {
        const auto& mimes = supportedMimeTypes();
        if (mimes.empty()) return true;
        for (const auto& m : mimes) {
            if (m == ctx.manifest.detected_mime) return true;
        }
        return false;
    }
};

} // namespace ingestion
} // namespace themis
