/**
 * @file mode_spec_bootstrap.cpp
 * @brief Resolve/load/validate the server ThemisModePack (`--mode-spec`, `llm.orchestrator.mode_spec_path`).
 */

#include "llm/mode_spec_bootstrap.h"

#include <exception>
#include <filesystem>
#include <system_error>
#include <utility>
#include <vector>

namespace themis::llm {

namespace {

[[nodiscard]] std::string joinErrors(const std::vector<std::string>& errors) {
    std::string joined;
    for (const auto& e : errors) {
        if (!joined.empty()) {
            joined += "; ";
        }
        joined += e;
    }
    return joined.empty() ? std::string("unknown validation error") : joined;
}

}  // namespace

const char* modeSpecSourceName(ModeSpecBootstrapResult::Source source) noexcept {
    switch (source) {
        case ModeSpecBootstrapResult::Source::CommandLine:  return "--mode-spec";
        case ModeSpecBootstrapResult::Source::ServerConfig: return "llm.orchestrator.mode_spec_path";
        case ModeSpecBootstrapResult::Source::None:
        default:                                            return "none";
    }
}

ModeSpecBootstrapResult loadModeSpecForServer(const nlohmann::json& server_config,
                                              const std::optional<std::string>& cli_override) {
    ModeSpecBootstrapResult result;
    // The message is fully evaluated before the body runs, so it may safely
    // read result.path.
    auto fail = [&result](std::string message) -> ModeSpecBootstrapResult {
        result.status = ModeSpecBootstrapResult::Status::Failed;
        result.error  = std::move(message);
        return result;
    };

    // Optional orchestrator section (also consulted for default_mode when the CLI wins).
    const nlohmann::json* orch_cfg = nullptr;
    if (server_config.is_object()) {
        const auto llm_it = server_config.find("llm");
        if (llm_it != server_config.end() && llm_it->is_object()) {
            const auto orch_it = llm_it->find("orchestrator");
            if (orch_it != llm_it->end() && orch_it->is_object()) {
                orch_cfg = &*orch_it;
            }
        }
    }

    // ── 1. Resolve the effective path ────────────────────────────────────────
    if (cli_override.has_value()) {
        result.source = ModeSpecBootstrapResult::Source::CommandLine;
        if (cli_override->empty()) {
            return fail("--mode-spec requires a non-empty file path");
        }
        result.path = *cli_override;
    } else {
        if (orch_cfg == nullptr) {
            return result;  // NotConfigured: legacy behaviour unchanged.
        }

        std::optional<bool> enabled;
        if (const auto en_it = orch_cfg->find("enabled"); en_it != orch_cfg->end()) {
            if (!en_it->is_boolean()) {
                result.source = ModeSpecBootstrapResult::Source::ServerConfig;
                return fail("llm.orchestrator.enabled must be a boolean");
            }
            enabled = en_it->get<bool>();
        }
        if (enabled.has_value() && !*enabled) {
            return result;  // Explicitly disabled: path (if any) is ignored.
        }

        const auto path_it = orch_cfg->find("mode_spec_path");
        if (path_it == orch_cfg->end() || path_it->is_null()) {
            if (enabled.value_or(false)) {
                result.source = ModeSpecBootstrapResult::Source::ServerConfig;
                return fail("llm.orchestrator.enabled is true but llm.orchestrator.mode_spec_path is not set");
            }
            return result;  // NotConfigured.
        }

        result.source = ModeSpecBootstrapResult::Source::ServerConfig;
        if (!path_it->is_string() || path_it->get<std::string>().empty()) {
            return fail("llm.orchestrator.mode_spec_path must be a non-empty string");
        }
        result.path = path_it->get<std::string>();
    }

    const std::string origin = std::string(modeSpecSourceName(result.source));

    // ── 2. File must exist and be a regular file ─────────────────────────────
    std::error_code ec;
    const bool is_file = std::filesystem::is_regular_file(result.path, ec);
    if (ec || !is_file) {
        return fail("mode spec file '" + result.path + "' (from " + origin +
                           ") does not exist or is not a regular file");
    }

    // ── 3. Load + validate via ModeSpecLoader ────────────────────────────────
    try {
        result.pack = ModeSpecLoader::loadFromFile(result.path, &result.validation);
    } catch (const std::exception& e) {
        return fail("mode spec '" + result.path + "' (from " + origin + ") could not be loaded: " + e.what());
    }
    if (!result.validation.ok) {
        const std::string details = joinErrors(result.validation.errors);
        return fail("mode spec '" + result.path + "' (from " + origin + ") is invalid: " + details);
    }

    // ── 4. Optional default_mode override from the server config ─────────────
    if (orch_cfg != nullptr) {
        if (const auto dm_it = orch_cfg->find("default_mode"); dm_it != orch_cfg->end() && !dm_it->is_null()) {
            if (!dm_it->is_string() || dm_it->get<std::string>().empty()) {
                return fail("llm.orchestrator.default_mode must be a non-empty string");
            }
            result.pack.default_mode = dm_it->get<std::string>();
            result.validation = ModeSpecLoader::validate(result.pack);
            if (!result.validation.ok) {
                const std::string details = joinErrors(result.validation.errors);
                return fail("llm.orchestrator.default_mode override is invalid for mode spec '" +
                                   result.path + "': " + details);
            }
        }
    }

    result.status = ModeSpecBootstrapResult::Status::Loaded;
    return result;
}

}  // namespace themis::llm
