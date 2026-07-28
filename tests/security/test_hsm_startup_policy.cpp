/*
 * ThemisDB | File: test_hsm_startup_policy.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB — HSM startup policy tests
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "security/hsm_startup_policy.h"

#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;
using ::testing::HasSubstr;

namespace themis { namespace security { namespace test { 

namespace {

// RAII helper for environment-variable based tests.
// Captures the original value (if any) and restores it on destruction so tests
// remain isolated across platforms.
class ScopedEnvVar {
public:
    explicit ScopedEnvVar(const char* name)
        : name_(name) {
        if (const auto* value = std::getenv(name_)) {
            old_value_ = std::string(value);
        }
    }

    ~ScopedEnvVar() {
#ifdef _WIN32
        if (old_value_) {
            _putenv_s(name_, old_value_->c_str());
        } else {
            _putenv_s(name_, "");
        }
#else
        if (old_value_) {
            setenv(name_, old_value_->c_str(), 1);
        } else {
            unsetenv(name_);
        }
#endif
    }

    void set(const char* value) const {
#ifdef _WIN32
        _putenv_s(name_, value);
#else
        setenv(name_, value, 1);
#endif
    }

    void unset() const {
#ifdef _WIN32
        _putenv_s(name_, "");
#else
        unsetenv(name_);
#endif
    }

private:
    const char*               name_;
    std::optional<std::string> old_value_;
};

std::vector<char*> makeArgv(std::initializer_list<char*> args) {
    return std::vector<char*>(args);
}

std::optional<json> asOptionalJson(const json& value) {
    return std::optional<json>{value};
}

} // namespace

TEST(HSMStartupPolicy, NoConfigRequiresExplicitStubOptIn) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    char arg0[] = "themis_server";
    auto argv = makeArgv({arg0});

    const auto result = resolveHSMStartupPolicy(std::nullopt, std::nullopt,
                                                static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(result.ok());
    EXPECT_THAT(result.error, HasSubstr("No HSM configuration found"));
}

TEST(HSMStartupPolicy, NoConfigAllowsExplicitStubOptInFromFlag) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    char arg0[] = "themis_server";
    char arg1[] = "--allow-stub-hsm";
    auto argv = makeArgv({arg0, arg1});

    const auto result = resolveHSMStartupPolicy(std::nullopt, std::nullopt,
                                                static_cast<int>(argv.size()), argv.data());
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_TRUE(result.explicit_stub_opt_in);
    EXPECT_EQ(result.config_source, "explicit stub opt-in");
    EXPECT_TRUE(result.config.library_path.empty());
}

TEST(HSMStartupPolicy, StubProviderRequiresExplicitOptIn) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    const json security_cfg = {
        {"hsm", {
            {"provider", "stub"}
        }}
    };

    char arg0[] = "themis_server";
    auto argv = makeArgv({arg0});

    const auto result = resolveHSMStartupPolicy(asOptionalJson(security_cfg), std::nullopt,
                                                static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(result.ok());
    EXPECT_THAT(result.error, HasSubstr("requires explicit development opt-in"));
}

TEST(HSMStartupPolicy, StubProviderAllowsEnvironmentOptIn) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.set("1");

    const json security_cfg = {
        {"hsm", {
            {"provider", "stub"}
        }}
    };

    char arg0[] = "themis_server";
    auto argv = makeArgv({arg0});

    const auto result = resolveHSMStartupPolicy(asOptionalJson(security_cfg), std::nullopt,
                                                static_cast<int>(argv.size()), argv.data());
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_TRUE(result.explicit_stub_opt_in);
    EXPECT_EQ(result.config_source, "security config");
    EXPECT_TRUE(result.config.library_path.empty());
}

TEST(HSMStartupPolicy, Pkcs11ProviderRequiresLibraryPath) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    const json security_cfg = {
        {"hsm", {
            {"provider", "pkcs11"},
            {"pkcs11", {
                {"slot_id", 7}
            }}
        }}
    };

    char arg0[] = "themis_server";
    auto argv = makeArgv({arg0});

    const auto result = resolveHSMStartupPolicy(asOptionalJson(security_cfg), std::nullopt,
                                                static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(result.ok());
    EXPECT_THAT(result.error, HasSubstr("library_path"));
}

TEST(HSMStartupPolicy, Pkcs11ProviderParsesConfiguredFields) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    const json security_cfg = {
        {"hsm", {
            {"provider", "pkcs11"},
            {"pkcs11", {
                {"library_path", "/usr/lib/libCryptoki2_64.so"},
                {"slot_id", 3},
                {"pin", "1234"},
                {"token_label", "themis"},
                {"key_label", "master-key"}
            }}
        }}
    };

    char arg0[] = "themis_server";
    auto argv = makeArgv({arg0});

    const auto result = resolveHSMStartupPolicy(asOptionalJson(security_cfg), std::nullopt,
                                                static_cast<int>(argv.size()), argv.data());
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_FALSE(result.explicit_stub_opt_in);
    EXPECT_EQ(result.config.library_path, "/usr/lib/libCryptoki2_64.so");
    EXPECT_EQ(result.config.slot_id, 3u);
    EXPECT_EQ(result.config.pin, "1234");
    EXPECT_EQ(result.config.token_label, "themis");
    EXPECT_EQ(result.config.key_label, "master-key");
}

TEST(HSMStartupPolicy, SecurityConfigTakesPrecedenceOverMainConfig) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    const json security_cfg = {
        {"hsm", {
            {"provider", "pkcs11"},
            {"pkcs11", {
                {"library_path", "/security/lib.so"},
                {"slot_id", 1}
            }}
        }}
    };
    const json main_cfg = {
        {"hsm", {
            {"provider", "pkcs11"},
            {"pkcs11", {
                {"library_path", "/main/lib.so"},
                {"slot_id", 9}
            }}
        }}
    };

    char arg0[] = "themis_server";
    auto argv = makeArgv({arg0});

    const auto result = resolveHSMStartupPolicy(asOptionalJson(security_cfg), asOptionalJson(main_cfg),
                                                static_cast<int>(argv.size()), argv.data());
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.config.library_path, "/security/lib.so");
    EXPECT_EQ(result.config.slot_id, 1u);
}

TEST(HSMStartupPolicy, RuntimeSecurityAllowsRealPkcs11Classification) {
    HSMStartupPolicyResult policy;
    policy.config.library_path = "/usr/lib/libpkcs11.so";
    policy.explicit_stub_opt_in = false;

    const auto decision = evaluateHSMRuntimeSecurity(policy, /*runtime_stub_active=*/false, "");
    EXPECT_TRUE(decision.allow_startup);
    EXPECT_EQ(decision.security_classification, "HSM-HARDENED-PKCS11");
    EXPECT_THAT(decision.audit_event, HasSubstr("hardened PKCS#11"));
}

TEST(HSMStartupPolicy, RuntimeSecurityBlocksImplicitStubFallbackForPkcs11Policy) {
    HSMStartupPolicyResult policy;
    policy.config.library_path = "/usr/lib/libpkcs11.so";
    policy.explicit_stub_opt_in = false;

    const auto decision = evaluateHSMRuntimeSecurity(policy, /*runtime_stub_active=*/true,
                                                     "Failed to load PKCS#11 library");
    EXPECT_FALSE(decision.allow_startup);
    EXPECT_EQ(decision.security_classification, "HSM-BLOCKED-PKCS11-FALLBACK");
    EXPECT_THAT(decision.audit_event, HasSubstr("Provider error"));
}

TEST(HSMStartupPolicy, RuntimeSecurityAllowsExplicitStubOverride) {
    HSMStartupPolicyResult policy;
    policy.explicit_stub_opt_in = true;

    const auto decision = evaluateHSMRuntimeSecurity(policy, /*runtime_stub_active=*/true, "");
    EXPECT_TRUE(decision.allow_startup);
    EXPECT_EQ(decision.security_classification, "HSM-DEGRADED-EXPLICIT-STUB");
}
} } } // namespace themis::security::test
