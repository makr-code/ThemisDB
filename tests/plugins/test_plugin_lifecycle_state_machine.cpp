// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_plugin_lifecycle_state_machine.cpp
 * @brief Phase 2A focused tests for plugin lifecycle state machine.
 *
 * Test IDs: PLG-09 through PLG-16
 * Validates explicit state transitions and transition contract enforcement.
 *
 * @see include/plugins/plugin_interface.h (PluginLifecycleState enum)
 * @see src/plugins/ROADMAP.md — Phase 2A implementation
 */

#include "gtest/gtest.h"
#include "plugins/plugin_interface.h"

#include <cstdint>
#include <vector>

namespace themis {
namespace plugins {
namespace test {

// ============================================================================
// PLG-09 — Lifecycle state enum values
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG09_StateEnumValues) {
    // Verify enum values are as expected
    EXPECT_EQ(static_cast<uint8_t>(PluginLifecycleState::UNLOADED), 0u);
    EXPECT_EQ(static_cast<uint8_t>(PluginLifecycleState::LOADING), 1u);
    EXPECT_EQ(static_cast<uint8_t>(PluginLifecycleState::LOADED), 2u);
    EXPECT_EQ(static_cast<uint8_t>(PluginLifecycleState::UNLOADING), 3u);
    EXPECT_EQ(static_cast<uint8_t>(PluginLifecycleState::UNKNOWN), 255u);
}

// ============================================================================
// PLG-10 — State to string conversion
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG10_StateToString) {
    EXPECT_STREQ(lifecycleStateToString(PluginLifecycleState::UNLOADED), "UNLOADED");
    EXPECT_STREQ(lifecycleStateToString(PluginLifecycleState::LOADING), "LOADING");
    EXPECT_STREQ(lifecycleStateToString(PluginLifecycleState::LOADED), "LOADED");
    EXPECT_STREQ(lifecycleStateToString(PluginLifecycleState::UNLOADING), "UNLOADING");
    EXPECT_STREQ(lifecycleStateToString(PluginLifecycleState::UNKNOWN), "UNKNOWN");
    
    // Invalid state should return safe fallback
    EXPECT_STREQ(lifecycleStateToString(static_cast<PluginLifecycleState>(99)), "INVALID");
}

// ============================================================================
// PLG-11 — Valid transition from UNLOADED state
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG11_TransitionsFromUnloaded) {
    const auto unloaded = PluginLifecycleState::UNLOADED;
    const auto loading = PluginLifecycleState::LOADING;
    const auto loaded = PluginLifecycleState::LOADED;
    const auto unloading = PluginLifecycleState::UNLOADING;

    // Only UNLOADED → LOADING is valid
    EXPECT_TRUE(isValidLifecycleTransition(unloaded, loading));
    EXPECT_FALSE(isValidLifecycleTransition(unloaded, loaded));
    EXPECT_FALSE(isValidLifecycleTransition(unloaded, unloading));
    EXPECT_FALSE(isValidLifecycleTransition(unloaded, unloaded));
}

// ============================================================================
// PLG-12 — Valid transitions from LOADING state
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG12_TransitionsFromLoading) {
    const auto unloaded = PluginLifecycleState::UNLOADED;
    const auto loading = PluginLifecycleState::LOADING;
    const auto loaded = PluginLifecycleState::LOADED;
    const auto unloading = PluginLifecycleState::UNLOADING;

    // LOADING → LOADED (success) or LOADING → UNLOADED (rollback)
    EXPECT_TRUE(isValidLifecycleTransition(loading, loaded));
    EXPECT_TRUE(isValidLifecycleTransition(loading, unloaded));
    EXPECT_FALSE(isValidLifecycleTransition(loading, unloading));
    EXPECT_FALSE(isValidLifecycleTransition(loading, loading));
}

// ============================================================================
// PLG-13 — Valid transitions from LOADED state
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG13_TransitionsFromLoaded) {
    const auto unloaded = PluginLifecycleState::UNLOADED;
    const auto loading = PluginLifecycleState::LOADING;
    const auto loaded = PluginLifecycleState::LOADED;
    const auto unloading = PluginLifecycleState::UNLOADING;

    // LOADED → UNLOADING (unload) or LOADED → LOADED (reload)
    EXPECT_TRUE(isValidLifecycleTransition(loaded, unloading));
    EXPECT_TRUE(isValidLifecycleTransition(loaded, loaded)); // Reload case
    EXPECT_FALSE(isValidLifecycleTransition(loaded, loading));
    EXPECT_FALSE(isValidLifecycleTransition(loaded, unloaded));
}

// ============================================================================
// PLG-14 — Valid transitions from UNLOADING state
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG14_TransitionsFromUnloading) {
    const auto unloaded = PluginLifecycleState::UNLOADED;
    const auto loading = PluginLifecycleState::LOADING;
    const auto loaded = PluginLifecycleState::LOADED;
    const auto unloading = PluginLifecycleState::UNLOADING;

    // Only UNLOADING → UNLOADED is valid
    EXPECT_TRUE(isValidLifecycleTransition(unloading, unloaded));
    EXPECT_FALSE(isValidLifecycleTransition(unloading, loading));
    EXPECT_FALSE(isValidLifecycleTransition(unloading, loaded));
    EXPECT_FALSE(isValidLifecycleTransition(unloading, unloading));
}

// ============================================================================
// PLG-15 — Complete valid lifecycle path
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG15_CompleteValidPath) {
    // Simulate a complete plugin lifecycle:
    // UNLOADED → LOADING → LOADED → UNLOADING → UNLOADED
    
    std::vector<PluginLifecycleState> path = {
        PluginLifecycleState::UNLOADED,
        PluginLifecycleState::LOADING,
        PluginLifecycleState::LOADED,
        PluginLifecycleState::UNLOADING,
        PluginLifecycleState::UNLOADED,
    };

    // Verify each transition in the path is valid
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        EXPECT_TRUE(isValidLifecycleTransition(path[i], path[i + 1]))
            << "Transition from " << lifecycleStateToString(path[i])
            << " to " << lifecycleStateToString(path[i + 1]) << " should be valid";
    }
}

// ============================================================================
// PLG-16 — Reload path (LOADED → LOADED)
// ============================================================================

TEST(PluginLifecycleStateMachine, PLG16_ReloadPath) {
    // Plugin reload allows direct LOADED → LOADED transition
    const auto loaded = PluginLifecycleState::LOADED;
    
    EXPECT_TRUE(isValidLifecycleTransition(loaded, loaded));
    
    // But reload followed by unload should also work
    EXPECT_TRUE(isValidLifecycleTransition(loaded, PluginLifecycleState::UNLOADING));
}

} // namespace test
} // namespace plugins
} // namespace themis
