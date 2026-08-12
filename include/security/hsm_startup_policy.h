/**
 * @file hsm_startup_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=21; TODO=1, Stub=19, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/hsm_provider.h"
#include "security/hsm_security_checker.h"

#include <nlohmann/json.hpp>

#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace themis::security {

struct HSMStartupPolicyResult {
    HSMConfig    config;
    bool         explicit_stub_opt_in{false};
    bool         hsm_config_present{false};
    std::string  config_source;
    std::string  error;

    [[nodiscard]] bool ok() const noexcept { return error.empty(); }
};

/**
 * @brief Evaluates the effective startup security class after HSM initialization.
 *
 * This maps policy intent (`resolveHSMStartupPolicy`) and the observed runtime
 * provider state (real PKCS#11 vs. stub) into a hard gate decision.
 *
 * Failure/edge cases:
 * - If a stub provider is active without explicit insecure opt-in, startup must
 *   be blocked.
 * - If PKCS#11 was configured but runtime still fell back to stub, startup must
 *   be blocked unless explicit insecure opt-in is present.
 */
struct [[nodiscard]] HSMRuntimeSecurityDecision {
    /// True when startup may continue; false means startup must abort.
    bool allow_startup{false};
    /// True when the active runtime provider is the software stub.
    bool runtime_stub_active{true};
    /// Stable security class string (e.g. HSM-HARDENED-PKCS11, HSM-DEGRADED-EXPLICIT-STUB).
    std::string security_classification;
    /// Human-readable audit trail message for logs/audit sinks.
    std::string audit_event;
};

[[nodiscard]] inline bool isHSMStubOptInEnabled(int argc, char* argv[]) {
    if (HSMSecurityChecker::hasAllowStubFlag(argc, argv)) {
        return true;
    }

    const auto* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
    if (allow_stub == nullptr) {
        return false;
    }

    std::string value{allow_stub};
    for (auto& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    return value == "1" || value == "true" || value == "yes" || value == "on";
}

/**
 * @brief Evaluate runtime HSM security posture against resolved startup policy.
 *
 * @param policy Policy intent resolved from config/CLI/environment.
 * @param runtime_stub_active Observed runtime provider state (`true` when stub is active).
 * @param runtime_error Provider initialization/runtime error string used for audit context.
 * @return Startup decision including hard gate result, classification, and audit event.
 */
[[nodiscard]] inline HSMRuntimeSecurityDecision evaluateHSMRuntimeSecurity(
    const HSMStartupPolicyResult& policy,
    bool                          runtime_stub_active,
    const std::string&            runtime_error)
{
    HSMRuntimeSecurityDecision decision;
    decision.runtime_stub_active = runtime_stub_active;

    if (!runtime_stub_active) {
        decision.allow_startup = true;
        decision.security_classification = "HSM-HARDENED-PKCS11";
        decision.audit_event = "HSM runtime classified as hardened PKCS#11.";
        return decision;
    }

    if (policy.explicit_stub_opt_in) {
        decision.allow_startup = true;
        decision.security_classification = "HSM-DEGRADED-EXPLICIT-STUB";
        decision.audit_event =
            "HSM runtime uses explicit insecure stub override; hardware key protection is disabled.";
        return decision;
    }

    decision.allow_startup = false;
    if (!policy.config.library_path.empty()) {
        decision.security_classification = "HSM-BLOCKED-PKCS11-FALLBACK";
        decision.audit_event =
            "HSM policy requires PKCS#11 but runtime fell back to stub without explicit override; startup blocked.";
    } else {
        decision.security_classification = "HSM-BLOCKED-IMPLICIT-STUB";
        decision.audit_event =
            "HSM runtime attempted implicit stub startup without explicit insecure override.";
    }

    if (!runtime_error.empty()) {
        decision.audit_event += " Provider error: " + runtime_error;
    }
    return decision;
}

[[nodiscard]] inline std::optional<std::string> applyConfiguredHSMProvider(
    const nlohmann::json& hsm,
    bool                  allow_stub_opt_in,
    HSMConfig&            config)
{
    if (!hsm.contains("provider") || !hsm["provider"].is_string()) {
        return std::string("HSM configuration requires string field `hsm.provider`.");
    }

    const auto provider = hsm["provider"].get<std::string>();
    if (provider == "pkcs11") {
        if (!hsm.contains("pkcs11") || !hsm["pkcs11"].is_object()) {
            return std::string(
                "`hsm.provider: pkcs11` requires object `hsm.pkcs11`.");
        }

        const auto& pkcs11 = hsm["pkcs11"];
        config.library_path = pkcs11.value("library_path", std::string{});
        if (config.library_path.empty()) {
            return std::string(
                "`hsm.provider: pkcs11` requires non-empty `hsm.pkcs11.library_path`.");
        }

        config.slot_id = pkcs11.value("slot_id", 0u);
        config.pin = pkcs11.value("pin", std::string{});
        config.token_label = pkcs11.value("token_label", std::string{});
        config.key_label = pkcs11.value("key_label", std::string("themis-signing-key"));
        return std::nullopt;
    }

    if (provider == "stub") {
        if (!allow_stub_opt_in) {
            return std::string(
                "`hsm.provider: stub` requires explicit development opt-in via "
                "`--allow-stub-hsm` or `THEMIS_ALLOW_HSM_STUB=1`.");
        }

        config.library_path.clear();
        config.pin.clear();
        config.token_label.clear();
        config.key_label = "themis-signing-key";
        return std::nullopt;
    }

    return std::string("Unsupported HSM provider `") + provider
           + "`. Supported providers are `pkcs11` and `stub`.";
}

[[nodiscard]] inline HSMStartupPolicyResult resolveHSMStartupPolicy(
    const std::optional<nlohmann::json>& security_config,
    const std::optional<nlohmann::json>& main_config,
    int                                  argc,
    char*                                argv[])
{
    HSMStartupPolicyResult result;
    result.explicit_stub_opt_in = isHSMStubOptInEnabled(argc, argv);

    const auto applyConfigIfPresent = [&](const std::optional<nlohmann::json>& cfg,
                                          const std::string&                  source) -> bool {
        if (!cfg || !cfg->contains("hsm")) {
            return false;
        }

        result.hsm_config_present = true;
        result.config_source = source;
        if (auto error = applyConfiguredHSMProvider((*cfg)["hsm"],
                                                    result.explicit_stub_opt_in,
                                                    result.config)) {
            result.error = *error;
        }
        return true;
    };

    if (!applyConfigIfPresent(security_config, "security config")
        && !applyConfigIfPresent(main_config, "main config")) {
        if (!result.explicit_stub_opt_in) {
            result.error =
                "No HSM configuration found. Configure `hsm.provider: pkcs11` with a real "
                "`hsm.pkcs11.library_path`, or explicitly opt in to the development stub via "
                "`--allow-stub-hsm` or `THEMIS_ALLOW_HSM_STUB=1`.";
            return result;
        }

        result.config_source = "explicit stub opt-in";
        result.config.library_path.clear();
        result.config.key_label = "themis-signing-key";
    }

    return result;
}

} // namespace themis::security
