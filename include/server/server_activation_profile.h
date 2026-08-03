/**
 * @file server_activation_profile.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Header-Only Configuration**: Defines server activation profiles (enum + configuration).
 *       No .cpp implementation needed. Consumers pass the profile to server initialization.
 */

/*
 * ThemisDB | File: server_activation_profile.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::server {

/// @brief Runtime activation profile used to enforce startup feature contracts.
enum class ServerActivationProfile {
    Minimal,
    Standard,
    Enterprise
};

/// @brief Result of server profile resolution from CLI/config/environment.
struct ProfileResolutionResult {
    bool ok = false;
    ServerActivationProfile profile = ServerActivationProfile::Standard;
    std::string source;
    std::string raw_value;
    std::string error;
};

/// @brief Compile-time capabilities that are relevant for server profile checks.
struct ServerBuildCapabilities {
    bool http_server = false;
    bool grpc = false;
    bool prometheus = false;
    bool llm = false;
    bool mimalloc = false;
    bool hsm_real = false;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"http_server", http_server},
            {"grpc", grpc},
            {"prometheus", prometheus},
            {"llm", llm},
            {"mimalloc", mimalloc},
            {"hsm_real", hsm_real},
        };
    }
};

/// @brief Runtime feature intent extracted from configuration and CLI opts.
struct ServerRuntimeFeatureRequests {
    bool http_enabled = true;
    bool grpc_enabled = false;
    bool prometheus_enabled = false;
    bool llm_enabled = false;
    bool mimalloc_enabled = false;
    bool hsm_stub_opt_in = false;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"http_enabled", http_enabled},
            {"grpc_enabled", grpc_enabled},
            {"prometheus_enabled", prometheus_enabled},
            {"llm_enabled", llm_enabled},
            {"mimalloc_enabled", mimalloc_enabled},
            {"hsm_stub_opt_in", hsm_stub_opt_in},
        };
    }
};

/// @brief Validation result for startup profile and capability compatibility.
struct ServerProfileValidationResult {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool ok() const { return errors.empty(); }

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"ok", ok()},
            {"errors", errors},
            {"warnings", warnings},
        };
    }
};

[[nodiscard]] inline std::string toString(ServerActivationProfile profile) {
    switch (profile) {
        case ServerActivationProfile::Minimal:
            return "minimal";
        case ServerActivationProfile::Standard:
            return "standard";
        case ServerActivationProfile::Enterprise:
            return "enterprise";
    }
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#endif
    return "standard";
}

[[nodiscard]] inline std::string normalizeProfileValue(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

[[nodiscard]] inline std::optional<ServerActivationProfile> parseProfileValue(const std::string& value) {
    const std::string normalized = normalizeProfileValue(value);
    if (normalized == "minimal") {
        return ServerActivationProfile::Minimal;
    }
    if (normalized == "standard") {
        return ServerActivationProfile::Standard;
    }
    if (normalized == "enterprise") {
        return ServerActivationProfile::Enterprise;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> jsonBoolAtPath(
    const nlohmann::json& root,
    std::initializer_list<std::string_view> path
) {
    const nlohmann::json* current = &root;
    for (const auto key : path) {
        if (!current->is_object() || !current->contains(std::string(key))) {
            return std::nullopt;
        }
        current = &(*current)[std::string(key)];
    }
    if (current->is_boolean()) {
        return current->get<bool>();
    }
    if (current->is_number_integer()) {
        return current->get<int>() != 0;
    }
    if (current->is_string()) {
        const std::string value = normalizeProfileValue(current->get<std::string>());
        if (value == "1" || value == "true" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "0" || value == "false" || value == "no" || value == "off") {
            return false;
        }
    }
    return std::nullopt;
}

/**
 * @brief Resolve the effective server activation profile.
 *
 * Resolution precedence is: CLI argument > config file > environment variable >
 * compile-time default profile.
 *
 * @param cli_profile Optional CLI profile override (e.g. --server-profile).
 * @param config Optional parsed runtime config.
 * @param env_profile Optional profile from THEMIS_SERVER_PROFILE.
 * @param default_profile Compile-time default profile from build configuration.
 * @return Resolution metadata with source/value or a descriptive error.
 */
[[nodiscard]] inline ProfileResolutionResult resolveServerActivationProfile(
    const std::optional<std::string>& cli_profile,
    const std::optional<nlohmann::json>& config,
    const std::optional<std::string>& env_profile,
    const std::string& default_profile
) {
    ProfileResolutionResult result;

    auto resolve_one = [&](const std::optional<std::string>& value, const char* source) -> bool {
        if (!value || value->empty()) {
            return false;
        }
        const auto parsed = parseProfileValue(*value);
        if (!parsed) {
            result.ok = false;
            result.source = source;
            result.raw_value = *value;
            result.error = "invalid server profile '" + *value +
                           "' (expected minimal|standard|enterprise)";
            return true;
        }
        result.ok = true;
        result.profile = *parsed;
        result.source = source;
        result.raw_value = *value;
        return true;
    };

    if (resolve_one(cli_profile, "cli")) {
        return result;
    }

    if (config) {
        if (config->contains("server") && (*config)["server"].is_object() &&
            (*config)["server"].contains("profile") && (*config)["server"]["profile"].is_string()) {
            const std::optional<std::string> from_cfg = (*config)["server"]["profile"].get<std::string>();
            if (resolve_one(from_cfg, "config.server.profile")) {
                return result;
            }
        }
        if (config->contains("profile") && (*config)["profile"].is_object() &&
            (*config)["profile"].contains("server") && (*config)["profile"]["server"].is_string()) {
            const std::optional<std::string> from_cfg = (*config)["profile"]["server"].get<std::string>();
            if (resolve_one(from_cfg, "config.profile.server")) {
                return result;
            }
        }
    }

    if (resolve_one(env_profile, "env.THEMIS_SERVER_PROFILE")) {
        return result;
    }

    const std::optional<std::string> fallback = default_profile;
    resolve_one(fallback, "build_default");
    return result;
}

/**
 * @brief Extract runtime feature activation requests from config/CLI context.
 *
 * Missing paths are treated as "not requested", and malformed values are
 * ignored to keep extraction non-throwing.
 *
 * @param config Optional parsed runtime config.
 * @param hsm_stub_opt_in Whether explicit HSM stub opt-in was requested.
 * @return Runtime request snapshot used for capability validation/reporting.
 */
[[nodiscard]] inline ServerRuntimeFeatureRequests extractRuntimeFeatureRequests(
    const std::optional<nlohmann::json>& config,
    bool hsm_stub_opt_in
) {
    ServerRuntimeFeatureRequests requests;
    requests.hsm_stub_opt_in = hsm_stub_opt_in;

    if (!config) {
        return requests;
    }

    if (const auto value = jsonBoolAtPath(*config, {"network", "http", "enabled"})) {
        requests.http_enabled = *value;
    }
    if (const auto value = jsonBoolAtPath(*config, {"distributed", "enabled"})) {
        requests.grpc_enabled = *value;
    }
    if (const auto value = jsonBoolAtPath(*config, {"protocols", "grpc", "enabled"})) {
        requests.grpc_enabled = *value;
    }
    if (const auto value = jsonBoolAtPath(*config, {"llm", "enabled"})) {
        requests.llm_enabled = *value;
    }
    if (const auto value = jsonBoolAtPath(*config, {"metrics", "prometheus", "enabled"})) {
        requests.prometheus_enabled = *value;
    }
    if (const auto value = jsonBoolAtPath(*config, {"observability", "prometheus", "enabled"})) {
        requests.prometheus_enabled = *value;
    }
    if (const auto value = jsonBoolAtPath(*config, {"optimizations", "mimalloc", "enabled"})) {
        requests.mimalloc_enabled = *value;
    }
    return requests;
}

/**
 * @brief Validate profile requirements against build/runtime capability signals.
 *
 * This enforces the profile feature matrix and rejects contradictory runtime
 * requests (feature requested but not compiled in). Missing required features
 * become warnings only when @p allow_degraded_build is true.
 *
 * @param profile Selected activation profile.
 * @param build_capabilities Compile-time feature capability state.
 * @param runtime_requests Runtime configuration requests.
 * @param allow_degraded_build Enables warning-only handling for missing required features.
 * @return Validation result containing errors (startup blockers) and warnings.
 */
[[nodiscard]] inline ServerProfileValidationResult validateServerActivationProfile(
    ServerActivationProfile profile,
    const ServerBuildCapabilities& build_capabilities,
    const ServerRuntimeFeatureRequests& runtime_requests,
    bool allow_degraded_build
) {
    ServerProfileValidationResult result;

    auto require_feature = [&](bool available, const std::string& name) {
        if (available) {
            return;
        }
        if (allow_degraded_build) {
            result.warnings.push_back("missing required feature '" + name +
                                      "' overridden via --allow-degraded-build");
        } else {
            result.errors.push_back("missing required feature '" + name + "'");
        }
    };

    require_feature(build_capabilities.http_server, "THEMIS_ENABLE_HTTP_SERVER");

    if (profile == ServerActivationProfile::Standard || profile == ServerActivationProfile::Enterprise) {
        require_feature(build_capabilities.prometheus, "THEMIS_HAS_PROMETHEUS");
        require_feature(build_capabilities.llm, "THEMIS_ENABLE_LLM");
        require_feature(build_capabilities.mimalloc, "THEMIS_ENABLE_MIMALLOC");
    }

    if (profile == ServerActivationProfile::Enterprise) {
        require_feature(build_capabilities.grpc, "THEMIS_ENABLE_GRPC");
        require_feature(build_capabilities.hsm_real, "THEMIS_ENABLE_HSM_REAL");
    }

    auto require_runtime_compatibility = [&](bool requested, bool available, const std::string& name) {
        if (requested && !available) {
            result.errors.push_back("runtime config requests '" + name +
                                    "' but binary was built without it");
        }
    };

    require_runtime_compatibility(runtime_requests.http_enabled, build_capabilities.http_server, "http");
    require_runtime_compatibility(runtime_requests.grpc_enabled, build_capabilities.grpc, "grpc");
    require_runtime_compatibility(runtime_requests.prometheus_enabled, build_capabilities.prometheus, "prometheus");
    require_runtime_compatibility(runtime_requests.llm_enabled, build_capabilities.llm, "llm");
    require_runtime_compatibility(runtime_requests.mimalloc_enabled, build_capabilities.mimalloc, "mimalloc");

    return result;
}

/**
 * @brief Enforce HSM runtime policy for the selected profile.
 *
 * Stub HSM operation requires explicit opt-in in all profiles, and Enterprise
 * profile always requires hardware-backed HSM.
 *
 * @param profile Selected activation profile.
 * @param stub_provider_active True when runtime uses stub HSM provider.
 * @param explicit_stub_opt_in True when operator explicitly opted in to stub mode.
 * @return Validation result with blocking errors for policy violations.
 */
[[nodiscard]] inline ServerProfileValidationResult validateHsmRuntimeForProfile(
    ServerActivationProfile profile,
    bool stub_provider_active,
    bool explicit_stub_opt_in
) {
    ServerProfileValidationResult result;
    if (!stub_provider_active) {
        return result;
    }

    if (!explicit_stub_opt_in) {
        result.errors.push_back("HSM stub provider is active without explicit opt-in (--allow-stub-hsm or THEMIS_ALLOW_HSM_STUB=1)");
    }

    if (profile == ServerActivationProfile::Enterprise) {
        result.errors.push_back("enterprise profile requires hardware-backed HSM provider");
    }

    return result;
}

/**
 * @brief Build machine-readable startup capability report payload.
 *
 * The report captures selected profile, build capabilities, runtime requests,
 * validation status, and startup mode flags and is intended for structured logs.
 *
 * @param profile Active profile.
 * @param profile_resolution Profile resolution metadata.
 * @param build_capabilities Compile-time capabilities.
 * @param runtime_requests Runtime feature requests.
 * @param validation Validation output for the current startup stage.
 * @param allow_degraded_build Whether degraded override was requested.
 * @param production_mode Whether production mode detection is active.
 * @return JSON object suitable for structured startup logging.
 */
[[nodiscard]] inline nlohmann::json makeStartupCapabilityReport(
    ServerActivationProfile profile,
    const ProfileResolutionResult& profile_resolution,
    const ServerBuildCapabilities& build_capabilities,
    const ServerRuntimeFeatureRequests& runtime_requests,
    const ServerProfileValidationResult& validation,
    bool allow_degraded_build,
    bool production_mode
) {
    return {
        {"profile", {
            {"active", toString(profile)},
            {"source", profile_resolution.source},
            {"raw_value", profile_resolution.raw_value},
        }},
        {"mode", {
            {"production", production_mode},
            {"allow_degraded_build", allow_degraded_build},
        }},
        {"build_capabilities", build_capabilities.toJson()},
        {"runtime_requests", runtime_requests.toJson()},
        {"validation", validation.toJson()},
    };
}

} // namespace themis::server
