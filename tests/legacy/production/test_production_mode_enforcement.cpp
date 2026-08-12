#include <gtest/gtest.h>
#include "core/security_initialization.h"
#include "core/production_mode.h"
#include "core/config_validator.h"
#include "core/concerns/concerns_context.h"
#include <cstdlib>

#ifdef _WIN32
static int setenv(const char* name, const char* value, int overwrite) {
    if (!overwrite && std::getenv(name) != nullptr) {
        return 0;
    }
    return _putenv_s(name, value ? value : "");
}

static int unsetenv(const char* name) {
    return _putenv_s(name, "");
}
#endif

using namespace themis;
using namespace themis::core;
using namespace themis::core::concerns;

/**
 * @brief Helper class to manage environment variables during tests
 */
class EnvGuard {
public:
    EnvGuard(const std::string& var_name, const std::string& value) : name_(var_name) {
        const char* old_val = std::getenv(name_.c_str());
        if (old_val) {
            old_value_ = old_val;
            has_old_ = true;
        } else {
            has_old_ = false;
        }
        setenv(name_.c_str(), value.c_str(), 1);
    }
    
    ~EnvGuard() {
        if (has_old_) {
            setenv(name_.c_str(), old_value_.c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
    }
    
private:
    std::string name_;
    std::string old_value_;
    bool has_old_;
};

/**
 * @brief Test Production Mode Detection
 */
class ProductionModeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear env vars before each test
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
    
    void TearDown() override {
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
};

TEST_F(ProductionModeTest, DetectsProductionModeFromEnvVar) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    EXPECT_TRUE(ProductionMode::isEnabled());
    EXPECT_EQ(ProductionMode::modeName(), "production");
}

TEST_F(ProductionModeTest, DetectsProductionFromEnvironmentVar) {
    EnvGuard guard("THEMIS_ENVIRONMENT", "production");
    EXPECT_TRUE(ProductionMode::isEnabled());
}

TEST_F(ProductionModeTest, RecognizesDevelopmentMode) {
    // No env vars set
    EXPECT_FALSE(ProductionMode::isEnabled());
    EXPECT_EQ(ProductionMode::modeName(), "development");
}

TEST_F(ProductionModeTest, EnforceThrowsInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    EXPECT_THROW(
        ProductionMode::enforce(false, "Test error"),
        std::runtime_error
    );
}

TEST_F(ProductionModeTest, EnforceDoesNotThrowInDevelopment) {
    EXPECT_NO_THROW(
        ProductionMode::enforce(false, "Test error")
    );
}

/**
 * @brief Test Config Validator
 */
class ConfigValidatorTest : public ::testing::Test {};

TEST_F(ConfigValidatorTest, ValidVaultConfigPasses) {
    nlohmann::json config = {
        {"vault_addr", "https://vault.example.com"},
        {"vault_token", "s.test123"},
        {"kv_mount_path", "secret"}
    };
    
    auto result = ConfigValidator::validateVaultConfig(config);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConfigValidatorTest, InvalidVaultConfigFails) {
    nlohmann::json config = {
        {"vault_addr", ""},
        {"vault_token", "s.test123"}
    };
    
    auto result = ConfigValidator::validateVaultConfig(config);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigValidatorTest, VaultConfigWarnsOnInsecureSettings) {
    nlohmann::json config = {
        {"vault_addr", "https://vault.example.com"},
        {"vault_token", "s.test123"},
        {"tls_skip_verify", true}
    };
    
    auto result = ConfigValidator::validateVaultConfig(config);
    EXPECT_TRUE(result.valid);
    EXPECT_FALSE(result.warnings.empty());
}

TEST_F(ConfigValidatorTest, JWTConfigRequiresIssuerInProduction) {
    auth::JWTValidatorConfig config;
    config.jwks_url = "https://auth.example.com/jwks";
    config.expected_issuer = std::nullopt;  // No issuer configured
    
    auto result = ConfigValidator::validateJWTConfig(config, true);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigValidatorTest, JWTConfigWarnsInDevelopment) {
    auth::JWTValidatorConfig config;
    config.jwks_url = "";
    
    auto result = ConfigValidator::validateJWTConfig(config, false);
    EXPECT_TRUE(result.valid);  // Valid but with warnings
    EXPECT_FALSE(result.warnings.empty());
}

TEST_F(ConfigValidatorTest, LogConfigValidatesLevel) {
    auto result = ConfigValidator::validateLogConfig("invalid_level", "[%l] %v");
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test SecurityLayerBuilder Production Mode Enforcement
 */
class SecurityLayerBuilderProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
    
    void TearDown() override {
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
};

TEST_F(SecurityLayerBuilderProductionTest, RejectsMockProviderInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    auto builder = SecurityLayerBuilder()
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}");
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST_F(SecurityLayerBuilderProductionTest, AllowsMockProviderInDevelopment) {
    // No production mode set
    auto builder = SecurityLayerBuilder()
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}");
    
    EXPECT_NO_THROW(builder.build());
}

TEST_F(SecurityLayerBuilderProductionTest, RequiresKeyProviderInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    auto builder = SecurityLayerBuilder();  // No key provider set
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST_F(SecurityLayerBuilderProductionTest, RequiresJWTInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    // Valid vault config but no JWT
    nlohmann::json vault_config = {
        {"vault_addr", "https://vault.example.com"},
        {"vault_token", "s.test123"}
    };
    
    auto builder = SecurityLayerBuilder()
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::VAULT, vault_config.dump());
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST_F(SecurityLayerBuilderProductionTest, ValidatesVaultConfig) {
    // Invalid vault config (missing token)
    nlohmann::json vault_config = {
        {"vault_addr", "https://vault.example.com"}
    };
    
    auto builder = SecurityLayerBuilder();
    
    EXPECT_THROW(
        builder.withKeyProvider(SecurityLayerBuilder::KeyProviderType::VAULT, vault_config.dump()),
        std::runtime_error
    );
}

TEST_F(SecurityLayerBuilderProductionTest, AcceptsValidProductionConfig) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    nlohmann::json vault_config = {
        {"vault_addr", "https://vault.example.com"},
        {"vault_token", "s.test123"},
        {"kv_mount_path", "secret"}
    };
    
    auth::JWTValidatorConfig jwt_config;
    jwt_config.jwks_url = "https://auth.example.com/jwks";
    jwt_config.expected_issuer = "https://auth.example.com";
    jwt_config.expected_audience = "my-app";
    
    auto builder = SecurityLayerBuilder()
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::VAULT, vault_config.dump())
        .withJWT(jwt_config);
    
    // This should not throw - all production requirements met
    // Note: It may fail due to actual Vault connection, but should not fail validation
    try {
        auto layer = builder.build();
        SUCCEED();
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        // Should not be a production mode violation
        EXPECT_EQ(msg.find("Production mode violation"), std::string::npos);
    }
}

/**
 * @brief Test ConcernsContext Production Mode Enforcement
 */
class ConcernsContextProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
    
    void TearDown() override {
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
};

TEST_F(ConcernsContextProductionTest, RejectsNoOpInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    EXPECT_THROW(ConcernsContext::createNoOp(), std::runtime_error);
}

TEST_F(ConcernsContextProductionTest, AllowsNoOpInDevelopment) {
    EXPECT_NO_THROW(ConcernsContext::createNoOp());
}

TEST_F(ConcernsContextProductionTest, RequiresTracingInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    ConcernsContext::Config config;
    config.tracingEnabled = false;  // Tracing disabled
    config.metricsEnabled = true;
    
    EXPECT_THROW(ConcernsContext::create(config), std::runtime_error);
}

TEST_F(ConcernsContextProductionTest, RequiresMetricsInProduction) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    ConcernsContext::Config config;
    config.tracingEnabled = true;
    config.tracingEndpoint = "http://localhost:4318";
    config.metricsEnabled = false;  // Metrics disabled
    
    EXPECT_THROW(ConcernsContext::create(config), std::runtime_error);
}

TEST_F(ConcernsContextProductionTest, AcceptsValidProductionConfig) {
    EnvGuard guard("THEMIS_PRODUCTION_MODE", "1");
    
    ConcernsContext::Config config;
    config.tracingEnabled = true;
    config.tracingEndpoint = "http://localhost:4318";
    config.metricsEnabled = true;
    config.logLevel = "info";
    
    EXPECT_NO_THROW(ConcernsContext::create(config));
}

TEST_F(ConcernsContextProductionTest, ValidatesLogLevel) {
    ConcernsContext::Config config;
    config.logLevel = "invalid_level";
    
    EXPECT_THROW(ConcernsContext::create(config), std::runtime_error);
}

/**
 * @brief Test HSM Provider Feature Flag
 */
class HSMProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        unsetenv("THEMIS_HSM_ENABLED");
    }
    
    void TearDown() override {
        unsetenv("THEMIS_HSM_ENABLED");
    }
};

TEST_F(HSMProviderTest, RejectsHSMWhenNotEnabled) {
    nlohmann::json hsm_config = {
        {"library_path", "/usr/lib/softhsm/libsofthsm2.so"},
        {"slot_id", "0"},
        {"pin", "1234"}
    };
    
    auto builder = SecurityLayerBuilder();
    builder.withKeyProvider(SecurityLayerBuilder::KeyProviderType::HSM, hsm_config.dump());
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST_F(HSMProviderTest, AllowsHSMWhenEnabled) {
    EnvGuard guard("THEMIS_HSM_ENABLED", "1");
    
    nlohmann::json hsm_config = {
        {"library_path", "/usr/lib/softhsm/libsofthsm2.so"},
        {"slot_id", "0"},
        {"pin", "1234"}
    };
    
    auto builder = SecurityLayerBuilder();
    builder.withKeyProvider(SecurityLayerBuilder::KeyProviderType::HSM, hsm_config.dump());
    
    // May throw due to actual HSM not available, but not due to feature flag
    try {
        auto layer = builder.build();
        SUCCEED();
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        // Should not be about feature flag
        EXPECT_EQ(msg.find("not enabled"), std::string::npos);
    }
}

TEST(HSMProviderStandaloneTest, RejectsMissingHSMLibraryPathBeforeInitialization) {
    EnvGuard guard("THEMIS_HSM_ENABLED", "1");

    nlohmann::json hsm_config = {
        {"library_path", "Z:/themis/nonexistent/pkcs11-provider.dll"},
        {"slot_id", "0"},
        {"pin", "1234"}
    };

    auto builder = SecurityLayerBuilder();
    builder.withKeyProvider(SecurityLayerBuilder::KeyProviderType::HSM, hsm_config.dump());

    try {
        auto layer = builder.build();
        (void)layer;
        FAIL() << "Expected missing HSM library_path to fail before initialization";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("library_path does not point to an existing file"), std::string::npos);
    }
}

TEST(HSMProviderStandaloneTest, RejectsInvalidHSMSlotIdBeforeInitialization) {
    EnvGuard guard("THEMIS_HSM_ENABLED", "1");

    nlohmann::json hsm_config = {
        {"library_path", __FILE__},
        {"slot_id", "slot-zero"},
        {"pin", "1234"}
    };

    auto builder = SecurityLayerBuilder();
    builder.withKeyProvider(SecurityLayerBuilder::KeyProviderType::HSM, hsm_config.dump());

    try {
        auto layer = builder.build();
        (void)layer;
        FAIL() << "Expected invalid HSM slot_id to fail before initialization";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("slot_id must be an unsigned integer"), std::string::npos);
    }
}
