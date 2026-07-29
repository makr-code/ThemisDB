// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_plugins_contract_hardening_focused.cpp
 * @brief Phase 4 focused contract-hardening tests for the plugins module.
 *
 * Test IDs: PLG-01 through PLG-08
 * No file I/O, no network, deterministic only.
 *
 * @see include/plugins/plugins_api_contract.h
 * @see src/plugins/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "plugins/plugins_api_contract.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace themis {
namespace plugins {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// PLG-01 — Error code uniqueness
// ============================================================================

TEST(PluginsContractHardening, PLG01_ErrorCodeUniqueness) {
    std::set<int32_t> seen;
    const int32_t codes[] = {
        static_cast<int32_t>(PluginsError::kPluginNotFound),
        static_cast<int32_t>(PluginsError::kManifestInvalid),
        static_cast<int32_t>(PluginsError::kSignatureVerifyFailed),
        static_cast<int32_t>(PluginsError::kLifecycleTransition),
        static_cast<int32_t>(PluginsError::kCapabilityDenied),
        static_cast<int32_t>(PluginsError::kRegistryConflict),
        static_cast<int32_t>(PluginsError::kHealthCheckFailed),
        static_cast<int32_t>(PluginsError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_TRUE(seen.insert(c).second) << "Duplicate error code: " << c;
    }
    EXPECT_EQ(seen.size(), 8u);
    (void)kSeed;
}

// ============================================================================
// PLG-02 — Error code range [8200, 8299]
// ============================================================================

TEST(PluginsContractHardening, PLG02_ErrorCodeRange) {
    const int32_t codes[] = {
        static_cast<int32_t>(PluginsError::kPluginNotFound),
        static_cast<int32_t>(PluginsError::kManifestInvalid),
        static_cast<int32_t>(PluginsError::kSignatureVerifyFailed),
        static_cast<int32_t>(PluginsError::kLifecycleTransition),
        static_cast<int32_t>(PluginsError::kCapabilityDenied),
        static_cast<int32_t>(PluginsError::kRegistryConflict),
        static_cast<int32_t>(PluginsError::kHealthCheckFailed),
        static_cast<int32_t>(PluginsError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_GE(c, 8200) << "Code " << c << " below reserved base 8200";
        EXPECT_LE(c, 8299) << "Code " << c << " above reserved max 8299";
    }
}

// ============================================================================
// PLG-03 — Switch dispatch: all cases must be handled
// ============================================================================

TEST(PluginsContractHardening, PLG03_SwitchDispatch) {
    auto describe = [](PluginsError e) -> const char* {
        switch (e) {
            case PluginsError::kSuccess:               return "success";
            case PluginsError::kPluginNotFound:        return "plugin_not_found";
            case PluginsError::kManifestInvalid:       return "manifest_invalid";
            case PluginsError::kSignatureVerifyFailed: return "signature_verify_failed";
            case PluginsError::kLifecycleTransition:   return "lifecycle_transition";
            case PluginsError::kCapabilityDenied:      return "capability_denied";
            case PluginsError::kRegistryConflict:      return "registry_conflict";
            case PluginsError::kHealthCheckFailed:     return "health_check_failed";
            case PluginsError::kInternalError:         return "internal_error";
        }
        return "unknown";
    };

    EXPECT_STREQ(describe(PluginsError::kSuccess),               "success");
    EXPECT_STREQ(describe(PluginsError::kPluginNotFound),        "plugin_not_found");
    EXPECT_STREQ(describe(PluginsError::kSignatureVerifyFailed), "signature_verify_failed");
    EXPECT_STREQ(describe(PluginsError::kInternalError),         "internal_error");
}

// ============================================================================
// PLG-04 — PluginState enum values are distinct
// ============================================================================

TEST(PluginsContractHardening, PLG04_PluginStateDistinct) {
    std::set<int32_t> states = {
        static_cast<int32_t>(PluginState::Unloaded),
        static_cast<int32_t>(PluginState::Loading),
        static_cast<int32_t>(PluginState::Active),
        static_cast<int32_t>(PluginState::Degraded),
        static_cast<int32_t>(PluginState::Unloading),
    };
    EXPECT_EQ(states.size(), 5u);
}

// ============================================================================
// PLG-05 — PluginRegistrationDescriptor default values
// ============================================================================

TEST(PluginsContractHardening, PLG05_RegistrationDescriptorDefaults) {
    PluginRegistrationDescriptor desc;
    EXPECT_TRUE(desc.plugin_id.empty());
    EXPECT_TRUE(desc.version.empty());
    EXPECT_TRUE(desc.manifest_path.empty());
    EXPECT_TRUE(desc.signature.empty());
    EXPECT_FALSE(desc.hot_plug_eligible);
}

// ============================================================================
// PLG-06 — Copy semantics for PluginRegistrationDescriptor
// ============================================================================

TEST(PluginsContractHardening, PLG06_RegistrationDescriptorCopy) {
    PluginRegistrationDescriptor src;
    src.plugin_id         = "plugin-42";
    src.version           = "1.0.0";
    src.hot_plug_eligible = true;

    PluginRegistrationDescriptor copy = src;
    EXPECT_EQ(copy.plugin_id,         src.plugin_id);
    EXPECT_EQ(copy.version,           src.version);
    EXPECT_EQ(copy.hot_plug_eligible, src.hot_plug_eligible);
}

// ============================================================================
// PLG-07 — Move semantics for PluginRegistrationDescriptor
// ============================================================================

TEST(PluginsContractHardening, PLG07_RegistrationDescriptorMove) {
    PluginRegistrationDescriptor src;
    src.plugin_id = "plugin-move";
    src.version   = "2.0.0";

    PluginRegistrationDescriptor moved = std::move(src);
    EXPECT_EQ(moved.plugin_id, "plugin-move");
    EXPECT_EQ(moved.version,   "2.0.0");
}

// ============================================================================
// PLG-08 — isPluginsFailClosed predicate
// ============================================================================

TEST(PluginsContractHardening, PLG08_FailClosedPredicate) {
    // Must be fail-closed.
    EXPECT_TRUE(isPluginsFailClosed(PluginsError::kSignatureVerifyFailed));
    EXPECT_TRUE(isPluginsFailClosed(PluginsError::kInternalError));
    EXPECT_TRUE(isPluginsFailClosed(PluginsError::kHealthCheckFailed));

    // Must NOT be fail-closed.
    EXPECT_FALSE(isPluginsFailClosed(PluginsError::kSuccess));
    EXPECT_FALSE(isPluginsFailClosed(PluginsError::kPluginNotFound));
    EXPECT_FALSE(isPluginsFailClosed(PluginsError::kManifestInvalid));
    EXPECT_FALSE(isPluginsFailClosed(PluginsError::kLifecycleTransition));
    EXPECT_FALSE(isPluginsFailClosed(PluginsError::kCapabilityDenied));
    EXPECT_FALSE(isPluginsFailClosed(PluginsError::kRegistryConflict));
}

} // namespace test
} // namespace plugins
} // namespace themis
