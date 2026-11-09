#include <gtest/gtest.h>

#include "security/timestamp_authority.h"

#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#ifndef THEMIS_USE_OPENSSL_TSA
namespace {

class ScopedEnvironment {
public:
    explicit ScopedEnvironment(const char* name)
        : name_(name), previous_(std::getenv(name) ? std::optional<std::string>(std::getenv(name))
                                                   : std::nullopt) {}

    ScopedEnvironment(const ScopedEnvironment&) = delete;
    ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

    ~ScopedEnvironment() {
        if (previous_) {
            set(*previous_);
        } else {
            unset();
        }
    }

    void set(const std::string& value) const {
#ifdef _WIN32
        _putenv_s(name_.c_str(), value.c_str());
#else
        ::setenv(name_.c_str(), value.c_str(), 1);
#endif
    }

    void unset() const {
#ifdef _WIN32
        _putenv_s(name_.c_str(), "");
#else
        ::unsetenv(name_.c_str());
#endif
    }

private:
    std::string name_;
    std::optional<std::string> previous_;
};

class ScopedCallbackReset {
public:
    ~ScopedCallbackReset() {
        themis::security::TimestampAuthority::setGetTimestampForHashFn({});
        themis::security::TimestampAuthority::setVerifyTimestampForHashFn({});
        themis::security::eIDASTimestampValidator::setValidateFn({});
        themis::security::eIDASTimestampValidator::setQualifiedTSAFn({});
    }
};

} // namespace

TEST(TimestampAuthorityProductionGating, OptInAndCallbacksCannotEnableFallbackInProduction) {
    using namespace themis::security;

    ScopedEnvironment production_mode("THEMIS_PRODUCTION_MODE");
    ScopedEnvironment environment("THEMIS_ENVIRONMENT");
    ScopedEnvironment stub_opt_in("THEMIS_ALLOW_TSA_STUB");
    ScopedCallbackReset callback_reset;
    production_mode.set("1");
    environment.unset();
    stub_opt_in.set("1");

    int stamp_callback_calls = 0;
    int verify_callback_calls = 0;
    int validate_callback_calls = 0;
    int qualified_callback_calls = 0;
    TimestampAuthority::setGetTimestampForHashFn(
        [&stamp_callback_calls](const std::vector<uint8_t>&, const TSAConfig&) {
            ++stamp_callback_calls;
            TimestampToken token;
            token.success = true;
            return token;
        });
    TimestampAuthority::setVerifyTimestampForHashFn(
        [&verify_callback_calls](const std::vector<uint8_t>&, const TimestampToken&,
                                 const TSAConfig&) {
            ++verify_callback_calls;
            return true;
        });
    eIDASTimestampValidator::setValidateFn(
        [&validate_callback_calls](const TimestampToken&, const std::vector<std::string>&,
                                   std::vector<std::string>&) {
            ++validate_callback_calls;
            return true;
        });
    eIDASTimestampValidator::setQualifiedTSAFn(
        [&qualified_callback_calls](const std::string&, const std::vector<std::string>&,
                                    std::vector<std::string>&) {
            ++qualified_callback_calls;
            return true;
        });

    TimestampAuthority authority(TSAConfig{});
    const std::vector<uint8_t> bytes{0x01, 0x02};
    TimestampToken supplied_token;
    supplied_token.success = true;

    EXPECT_FALSE(authority.getTimestamp(bytes).success);
    EXPECT_FALSE(authority.getTimestampForHash(bytes).success);
    EXPECT_FALSE(authority.verifyTimestamp(bytes, supplied_token));
    EXPECT_FALSE(authority.verifyTimestampForHash(bytes, supplied_token));
    EXPECT_FALSE(authority.parseToken(bytes).success);
    EXPECT_FALSE(authority.parseToken(std::string("hex:0102")).success);
    EXPECT_FALSE(authority.getTSACertificate().has_value());
    EXPECT_FALSE(authority.isAvailable());

    eIDASTimestampValidator validator;
    EXPECT_FALSE(validator.validateeIDASTimestamp(supplied_token, {}));
    EXPECT_FALSE(validator.validateAge(supplied_token));
    EXPECT_FALSE(validator.isQualifiedTSA("stub-certificate", {}));

    EXPECT_EQ(stamp_callback_calls, 0);
    EXPECT_EQ(verify_callback_calls, 0);
    EXPECT_EQ(validate_callback_calls, 0);
    EXPECT_EQ(qualified_callback_calls, 0);
    EXPECT_FALSE(validator.getValidationErrors().empty());
}

TEST(TimestampAuthorityProductionGating, CachedFallbackCertificateIsHiddenInProduction) {
    using namespace themis::security;

    ScopedEnvironment production_mode("THEMIS_PRODUCTION_MODE");
    ScopedEnvironment environment("THEMIS_ENVIRONMENT");
    ScopedEnvironment stub_opt_in("THEMIS_ALLOW_TSA_STUB");
    production_mode.unset();
    environment.unset();
    stub_opt_in.set("1");

    TimestampAuthority authority(TSAConfig{});
    ASSERT_TRUE(authority.getTSACertificate().has_value());

    production_mode.set("1");
    EXPECT_FALSE(authority.getTSACertificate().has_value());
}

#endif // !THEMIS_USE_OPENSSL_TSA
