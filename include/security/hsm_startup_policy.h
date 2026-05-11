#pragma once

#include "security/hsm_provider.h"
#include "security/hsm_security_checker.h"

#include <nlohmann/json.hpp>

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

[[nodiscard]] inline bool isHSMStubOptInEnabled(int argc, char* argv[]) {
    if (HSMSecurityChecker::hasAllowStubFlag(argc, argv)) {
        return true;
    }

    const auto* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
    return allow_stub != nullptr && std::string_view{allow_stub} == "1";
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

    const auto try_apply = [&](const std::optional<nlohmann::json>& cfg,
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

    if (!try_apply(security_config, "security config")
        && !try_apply(main_config, "main config")) {
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
