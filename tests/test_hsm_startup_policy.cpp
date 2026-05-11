/*
 * ThemisDB — HSM startup policy tests
 */

#include <gtest/gtest.h>

#include "security/hsm_startup_policy.h"

#include <cstdlib>
#include <optional>
#include <string>

using nlohmann::json;

namespace themis::security::test {

namespace {

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

} // namespace

TEST(HSMStartupPolicy, NoConfigRequiresExplicitStubOptIn) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    char arg0[] = "themis_server";
    char* argv[] = {arg0};

    const auto result = resolveHSMStartupPolicy(std::nullopt, std::nullopt, 1, argv);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error.find("No HSM configuration found"), std::string::npos);
}

TEST(HSMStartupPolicy, NoConfigAllowsExplicitStubOptInFromFlag) {
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    allow_stub.unset();

    char arg0[] = "themis_server";
    char arg1[] = "--allow-stub-hsm";
    char* argv[] = {arg0, arg1};

    const auto result = resolveHSMStartupPolicy(std::nullopt, std::nullopt, 2, argv);
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
    char* argv[] = {arg0};

    const auto result = resolveHSMStartupPolicy(security_cfg, std::nullopt, 1, argv);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error.find("requires explicit development opt-in"), std::string::npos);
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
    char* argv[] = {arg0};

    const auto result = resolveHSMStartupPolicy(security_cfg, std::nullopt, 1, argv);
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
    char* argv[] = {arg0};

    const auto result = resolveHSMStartupPolicy(security_cfg, std::nullopt, 1, argv);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error.find("library_path"), std::string::npos);
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
    char* argv[] = {arg0};

    const auto result = resolveHSMStartupPolicy(security_cfg, std::nullopt, 1, argv);
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
    char* argv[] = {arg0};

    const auto result = resolveHSMStartupPolicy(security_cfg, main_cfg, 1, argv);
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.config.library_path, "/security/lib.so");
    EXPECT_EQ(result.config.slot_id, 1u);
}

} // namespace themis::security::test
