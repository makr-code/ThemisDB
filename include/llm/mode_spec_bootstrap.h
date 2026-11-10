/**
 * @file mode_spec_bootstrap.h
 * @brief Server-startup wiring of the YAML ThemisModePack (mode spec) into the AIOrchestrator.
 *
 * Resolves the mode-spec path from the `--mode-spec <path>` command-line flag
 * (highest precedence) or the server configuration key
 * `llm.orchestrator.mode_spec_path`, loads it with ModeSpecLoader::loadFromFile()
 * and validates it with ModeSpecLoader::validate(). The function is side-effect
 * free apart from reading the file, so the startup policy (fail-fast on an
 * explicitly configured but invalid spec) can be unit-tested without a server.
 */
#pragma once

#include "llm/ai_orchestrator.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace themis::llm {

/**
 * @brief Outcome of resolving and loading the server mode spec.
 *
 * Ownership: plain value type; the caller owns the contained ModePack and
 * typically moves it into an AIOrchestrator.
 */
struct ModeSpecBootstrapResult {
    /// @brief Overall outcome.
    enum class Status {
        NotConfigured,  ///< Neither CLI flag nor config key set (or orchestrator disabled); keep legacy behaviour.
        Loaded,         ///< Spec loaded and validated; `pack` is usable.
        Failed          ///< Spec explicitly requested but missing/unreadable/invalid; startup must abort.
    };

    /// @brief Where the effective mode-spec path came from.
    enum class Source {
        None,          ///< No path was configured.
        CommandLine,   ///< `--mode-spec <path>` (overrides the config key).
        ServerConfig   ///< `llm.orchestrator.mode_spec_path`.
    };

    Status status = Status::NotConfigured;
    Source source = Source::None;
    std::string path;                     ///< Effective path (as configured; relative paths use the working directory).
    ModePack pack;                        ///< Valid only when status == Loaded.
    ModeSpecValidationResult validation;  ///< Loader/validator diagnostics (warnings are kept on success).
    std::string error;                    ///< Human-readable reason; non-empty iff status == Failed.

    /// @brief True when a validated pack is available.
    [[nodiscard]] bool loaded() const noexcept { return status == Status::Loaded; }
    /// @brief True when startup must be aborted.
    [[nodiscard]] bool failed() const noexcept { return status == Status::Failed; }
};

/**
 * @brief Human-readable name of a ModeSpecBootstrapResult::Source value.
 * @param[in] source Source enumerator.
 * @return "none", "--mode-spec" or "llm.orchestrator.mode_spec_path".
 */
[[nodiscard]] const char* modeSpecSourceName(ModeSpecBootstrapResult::Source source) noexcept;

/**
 * @brief Resolve, load and validate the server mode spec.
 *
 * Resolution rules:
 *  - A present @p cli_override always wins. An empty override string is an error.
 *  - Otherwise `llm.orchestrator` of @p server_config is consulted:
 *    - missing `llm` / `llm.orchestrator` (or non-object) → NotConfigured;
 *    - `enabled: false` → NotConfigured (the path is ignored);
 *    - `enabled` present but not a boolean → Failed;
 *    - `mode_spec_path` absent → NotConfigured, unless `enabled: true` → Failed;
 *    - `mode_spec_path` not a non-empty string → Failed.
 *  - Optional `llm.orchestrator.default_mode` (non-empty string) overrides the
 *    pack's `default_mode`; the result is re-validated so an unknown mode id fails.
 *
 * A configured file that does not exist, cannot be parsed, or fails
 * ModeSpecLoader::validate() yields Status::Failed with all validation errors
 * joined into `error`.
 *
 * @param[in] server_config Parsed server configuration (JSON view of the YAML/JSON file);
 *                          pass an empty object when no config file was loaded.
 * @param[in] cli_override  Value of `--mode-spec`, if given on the command line.
 * @return Bootstrap result; never throws for configuration or file errors.
 */
[[nodiscard]] ModeSpecBootstrapResult loadModeSpecForServer(
    const nlohmann::json& server_config,
    const std::optional<std::string>& cli_override);

}  // namespace themis::llm
