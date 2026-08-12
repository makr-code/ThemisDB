#include <gtest/gtest.h>

#include "server/server_activation_profile.h"
#include <utility>

using json = nlohmann::json;

namespace {

TEST(ServerActivationProfile, ResolveDefaultsToBuildProfileWhenUnset) {
    const auto resolved = themis::server::resolveServerActivationProfile(
        std::nullopt, std::nullopt, std::nullopt, "standard");

    ASSERT_TRUE(resolved.ok);
    EXPECT_EQ(resolved.source, "build_default");
    EXPECT_EQ(resolved.profile, themis::server::ServerActivationProfile::Standard);
}

TEST(ServerActivationProfile, ResolveRejectsInvalidProfileValue) {
    const auto resolved = themis::server::resolveServerActivationProfile(
        std::optional<std::string>{"invalid-profile"}, std::nullopt, std::nullopt, "standard");

    EXPECT_FALSE(resolved.ok);
    EXPECT_NE(resolved.error.find("invalid server profile"), std::string::npos);
}

TEST(ServerActivationProfile, StandardProfileRequiresCoreProductionFlags) {
    themis::server::ServerBuildCapabilities caps{};
    caps.http_server = true;
    caps.prometheus = true;
    caps.llm = true;
    caps.mimalloc = false;  // Missing required feature

    const auto result = themis::server::validateServerActivationProfile(
        themis::server::ServerActivationProfile::Standard,
        caps,
        themis::server::ServerRuntimeFeatureRequests{},
        false);

    EXPECT_FALSE(result.ok());
    EXPECT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.front().find("THEMIS_ENABLE_MIMALLOC"), std::string::npos);
}

TEST(ServerActivationProfile, StandardProfileAllowsExplicitDegradedOverride) {
    themis::server::ServerBuildCapabilities caps{};
    caps.http_server = true;

    const auto result = themis::server::validateServerActivationProfile(
        themis::server::ServerActivationProfile::Standard,
        caps,
        themis::server::ServerRuntimeFeatureRequests{},
        true);

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.warnings.empty());
}

TEST(ServerActivationProfile, EnterpriseProfileRequiresRealHsmBuildCapability) {
    themis::server::ServerBuildCapabilities caps{};
    caps.http_server = true;
    caps.grpc = true;
    caps.prometheus = true;
    caps.llm = true;
    caps.mimalloc = true;
    caps.hsm_real = false;

    const auto result = themis::server::validateServerActivationProfile(
        themis::server::ServerActivationProfile::Enterprise,
        caps,
        themis::server::ServerRuntimeFeatureRequests{},
        false);

    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.back().find("THEMIS_ENABLE_HSM_REAL"), std::string::npos);
}

TEST(ServerActivationProfile, RuntimeConfigMismatchFailsFast) {
    themis::server::ServerBuildCapabilities caps{};
    caps.http_server = true;

    auto requests = themis::server::ServerRuntimeFeatureRequests{};
    requests.grpc_enabled = true;

    const auto result = themis::server::validateServerActivationProfile(
        themis::server::ServerActivationProfile::Minimal,
        caps,
        requests,
        false);

    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.front().find("runtime config requests 'grpc'"), std::string::npos);
}

TEST(ServerActivationProfile, ExtractsRuntimeRequestsFromConfigPaths) {
    json cfg = {
        {"llm", {{"enabled", true}}},
        {"distributed", {{"enabled", true}}},
        {"metrics", {{"prometheus", {{"enabled", true}}}}},
        {"optimizations", {{"mimalloc", {{"enabled", true}}}}}
    };

    const auto requests = themis::server::extractRuntimeFeatureRequests(std::make_optional<json>(std::move(cfg)), true);

    EXPECT_TRUE(requests.llm_enabled);
    EXPECT_TRUE(requests.grpc_enabled);
    EXPECT_TRUE(requests.prometheus_enabled);
    EXPECT_TRUE(requests.mimalloc_enabled);
    EXPECT_TRUE(requests.hsm_stub_opt_in);
}

TEST(ServerActivationProfile, EnterpriseProfileRejectsStubHsmAtRuntime) {
    const auto result = themis::server::validateHsmRuntimeForProfile(
        themis::server::ServerActivationProfile::Enterprise,
        true,
        true);

    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.front().find("enterprise profile"), std::string::npos);
}

TEST(ServerActivationProfile, StubHsmRequiresExplicitOptIn) {
    const auto result = themis::server::validateHsmRuntimeForProfile(
        themis::server::ServerActivationProfile::Minimal,
        true,
        false);

    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.front().find("explicit opt-in"), std::string::npos);
}

} // namespace
