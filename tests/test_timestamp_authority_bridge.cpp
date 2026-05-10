/**
 * @file test_timestamp_authority_bridge.cpp
 * @brief Unit tests for TSA stub bridge callbacks (STUB #213/#214) and
 *        stateful non-OpenSSL Impl replacement (STUB #223).
 */

#include <gtest/gtest.h>
#include "security/timestamp_authority.h"

#include <cstdlib>
#include <stdexcept>

using namespace themis::security;

namespace {
void unsetEnvVar(const std::string& name) {
#ifdef _WIN32
    _putenv_s(name.c_str(), "");
#else
    ::unsetenv(name.c_str());
#endif
}

void setEnvVar(const std::string& name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    ::setenv(name.c_str(), value.c_str(), 1);
#endif
}

struct EnvUnsetGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    explicit EnvUnsetGuard(const std::string& var_name) : name(var_name) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) previous = existing;
        unsetEnvVar(name);
    }

    ~EnvUnsetGuard() {
        if (had_previous) setEnvVar(name, previous);
        else unsetEnvVar(name);
    }
};
} // namespace

// TSA-BRIDGE-01: Injected ValidateFn is called in non-OpenSSL build.
TEST(TimestampAuthorityBridgeTest, ValidateFnInjectionIsUsed) {
#ifdef THEMIS_USE_OPENSSL_TSA
    GTEST_SKIP() << "THEMIS_USE_OPENSSL_TSA is ON; stub bridge path not compiled.";
#endif
    EnvUnsetGuard no_stub("THEMIS_ALLOW_TSA_STUB");

    eIDASTimestampValidator::setValidateFn(
        [](const TimestampToken& token,
           const std::vector<std::string>& anchors,
           std::vector<std::string>& errors) {
            EXPECT_EQ(token.serial_number, "BRIDGE");
            EXPECT_EQ(anchors.size(), 2u);
            errors.clear();
            return true;
        });

    eIDASTimestampValidator v;
    TimestampToken token;
    token.success = false; // callback decides result
    token.serial_number = "BRIDGE";
    EXPECT_TRUE(v.validateeIDASTimestamp(token, {"A1", "A2"}));
    EXPECT_TRUE(v.getValidationErrors().empty());

    eIDASTimestampValidator::setValidateFn({});
}

// TSA-BRIDGE-02: Injected QualifiedTSAFn is called and exceptions fail closed.
TEST(TimestampAuthorityBridgeTest, QualifiedTSAFnInjectionAndFailClosed) {
#ifdef THEMIS_USE_OPENSSL_TSA
    GTEST_SKIP() << "THEMIS_USE_OPENSSL_TSA is ON; stub bridge path not compiled.";
#endif
    EnvUnsetGuard no_stub("THEMIS_ALLOW_TSA_STUB");

    eIDASTimestampValidator::setQualifiedTSAFn(
        [](const std::string& cert,
           const std::vector<std::string>& qtsp,
           std::vector<std::string>& errors) {
            EXPECT_EQ(cert, "CERT");
            EXPECT_EQ(qtsp.size(), 1u);
            errors.clear();
            return true;
        });
    {
        eIDASTimestampValidator v;
        EXPECT_TRUE(v.isQualifiedTSA("CERT", {"QTSP"}));
        EXPECT_TRUE(v.getValidationErrors().empty());
    }

    eIDASTimestampValidator::setQualifiedTSAFn(
        [](const std::string&, const std::vector<std::string>&, std::vector<std::string>&) -> bool {
            throw std::runtime_error("qualified-tsa callback failed");
        });
    {
        eIDASTimestampValidator v;
        EXPECT_FALSE(v.isQualifiedTSA("CERT", {"QTSP"}));
        auto errs = v.getValidationErrors();
        ASSERT_FALSE(errs.empty());
    }

    eIDASTimestampValidator::setQualifiedTSAFn({});
}

// TSA-IMPL-01: non-OpenSSL TimestampAuthority stub remains operational and
// getTSACertificate() is stable (stateful pimpl cache path).
TEST(TimestampAuthorityBridgeTest, StubImplStatefulCertificateCachePathWorks) {
#ifdef THEMIS_USE_OPENSSL_TSA
    GTEST_SKIP() << "THEMIS_USE_OPENSSL_TSA is ON; non-OpenSSL Impl test skipped.";
#endif
    EnvUnsetGuard no_stub("THEMIS_ALLOW_TSA_STUB");

    TSAConfig cfg;
    cfg.url = "stub://tsa";
    TimestampAuthority tsa(cfg);

    auto cert1 = tsa.getTSACertificate();
    auto cert2 = tsa.getTSACertificate();
    ASSERT_TRUE(cert1.has_value());
    ASSERT_TRUE(cert2.has_value());
    EXPECT_EQ(*cert1, *cert2);
    EXPECT_NE(cert1->find("BEGIN CERTIFICATE"), std::string::npos);
}

TEST(TimestampAuthorityBridgeTest, GetTimestampForHashBridgeIsUsed) {
#ifdef THEMIS_USE_OPENSSL_TSA
    GTEST_SKIP() << "THEMIS_USE_OPENSSL_TSA is ON; stub bridge path not compiled.";
#endif
    EnvUnsetGuard no_stub("THEMIS_ALLOW_TSA_STUB");

    TimestampAuthority::setGetTimestampForHashFn(
        [](const std::vector<uint8_t>& hash, const TSAConfig& cfg) {
            TimestampToken token;
            token.success = true;
            token.serial_number = "BRIDGED";
            token.token_b64 = "hash:" + std::to_string(hash.size());
            token.policy_oid = cfg.policy_oid;
            return token;
        });

    TSAConfig cfg;
    cfg.policy_oid = "1.2.3";
    TimestampAuthority tsa(cfg);
    auto token = tsa.getTimestampForHash({1, 2, 3, 4});
    EXPECT_TRUE(token.success);
    EXPECT_EQ(token.serial_number, "BRIDGED");
    EXPECT_EQ(token.policy_oid, "1.2.3");

    TimestampAuthority::setGetTimestampForHashFn({});
}

TEST(TimestampAuthorityBridgeTest, VerifyTimestampForHashBridgeIsUsedAndFailsClosed) {
#ifdef THEMIS_USE_OPENSSL_TSA
    GTEST_SKIP() << "THEMIS_USE_OPENSSL_TSA is ON; stub bridge path not compiled.";
#endif
    EnvUnsetGuard no_stub("THEMIS_ALLOW_TSA_STUB");

    TimestampAuthority::setVerifyTimestampForHashFn(
        [](const std::vector<uint8_t>& hash, const TimestampToken& token, const TSAConfig&) {
            return hash.size() == 2 && token.serial_number == "OK";
        });

    TSAConfig cfg;
    TimestampAuthority tsa(cfg);
    TimestampToken token;
    token.serial_number = "OK";
    EXPECT_TRUE(tsa.verifyTimestampForHash({9, 9}, token));

    TimestampAuthority::setVerifyTimestampForHashFn(
        [](const std::vector<uint8_t>&, const TimestampToken&, const TSAConfig&) -> bool {
            throw std::runtime_error("verify failure");
        });
    EXPECT_FALSE(tsa.verifyTimestampForHash({9, 9}, token));

    TimestampAuthority::setVerifyTimestampForHashFn({});
}
