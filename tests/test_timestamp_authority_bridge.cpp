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
struct EnvUnsetGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    explicit EnvUnsetGuard(const std::string& var_name) : name(var_name) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) previous = existing;
        ::unsetenv(name.c_str());
    }

    ~EnvUnsetGuard() {
        if (had_previous) ::setenv(name.c_str(), previous.c_str(), 1);
        else ::unsetenv(name.c_str());
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

