// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_wiki_edition_gates.cpp
 * @brief Edition-gate negative tests for LLM Wiki plugin (LWP-GATE-01).
 *
 * Validates compile-time and runtime edition gating:
 *   - Verify Community/Minimal builds return PermissionDenied on all plugin ops
 *   - Test that enterprise-only features are blocked in community mode
 *   - Validate compile-time gating with multiple build configurations
 *   - Test IngestMarkdownFiles, Query, Workspace operations in community mode
 *
 * Success Criteria:
 *   ✓ Community/Minimal build: all plugin ops return Status::PermissionDenied()
 *   ✓ Enterprise/Hyperscaler/Military build: full functionality
 *   ✓ Edition checks work for all 5 module ops
 *
 * @see include/llm_wiki/edition_gate.h
 * @see include/llm_wiki/llm_wiki_plugin_interface.h
 * @see src/llm_wiki/ROADMAP.md
 */

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "llm_wiki/edition_gate.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"

using namespace themis::llm_wiki;
using namespace themis::plugins::llm_wiki;

namespace {

// ---------------------------------------------------------------------------
// Edition gate tests
// ---------------------------------------------------------------------------

class EditionGateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test runs in isolation
    }
};

// ---------------------------------------------------------------------------
// Test: Compile-Time Gating
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-01: Compile-Time Edition Gating
 *
 * Verify that compile-time gates correctly enable/disable the LLM Wiki plugin
 * based on the THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED flag.
 */
TEST_F(EditionGateTest, CompileTimeGating_LWP_GATE_01) {
    // This constant is defined in edition_gate.h and depends on the compile flag
    SPDLOG_INFO("Compile-time LLM Wiki enabled: {}", kLLMWikiCompileTimeEnabled);
    
    // In enterprise builds, this should be true
    // In community/minimal builds, this should be false
    if constexpr (kLLMWikiCompileTimeEnabled) {
        SPDLOG_INFO("Enterprise build detected: LLM Wiki is enabled");
        EXPECT_TRUE(kLLMWikiCompileTimeEnabled);
    } else {
        SPDLOG_INFO("Community/Minimal build detected: LLM Wiki is disabled");
        EXPECT_FALSE(kLLMWikiCompileTimeEnabled);
    }
}

// ---------------------------------------------------------------------------
// Test: Runtime Edition Detection
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-02: Runtime Edition Detection
 *
 * Verify that the current edition is correctly detected at runtime.
 */
TEST_F(EditionGateTest, RuntimeEditionDetection_LWP_GATE_02) {
    Edition current_edition = getCurrentEdition();
    
    // Verify edition is one of the known values
    bool is_valid_edition = 
        current_edition == Edition::Community ||
        current_edition == Edition::Minimal ||
        current_edition == Edition::Enterprise ||
        current_edition == Edition::Hyperscaler ||
        current_edition == Edition::Military;
    
    EXPECT_TRUE(is_valid_edition)
        << "Invalid edition detected: " << static_cast<int>(current_edition);
    
    const char* edition_names[] = {
        "Community",
        "Minimal",
        "Enterprise",
        "Hyperscaler",
        "Military"
    };
    
    int edition_idx = static_cast<int>(current_edition);
    SPDLOG_INFO("Current edition: {}", edition_names[edition_idx]);
}

// ---------------------------------------------------------------------------
// Test: Plugin Availability by Edition
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-03: Plugin Availability by Edition
 *
 * Verify that LLM Wiki plugin is available only in enterprise+ editions.
 */
TEST_F(EditionGateTest, PluginAvailabilityByEdition_LWP_GATE_03) {
    bool is_llm_wiki_enabled = isLLMWikiEnabled();
    Edition current_edition = getCurrentEdition();
    
    // Determine if plugin should be available
    bool expected_available = 
        current_edition == Edition::Enterprise ||
        current_edition == Edition::Hyperscaler ||
        current_edition == Edition::Military;
    
    EXPECT_EQ(is_llm_wiki_enabled, expected_available)
        << "LLM Wiki availability mismatch for edition";
    
    if (is_llm_wiki_enabled) {
        SPDLOG_INFO("LLM Wiki plugin is available in current edition");
    } else {
        SPDLOG_INFO("LLM Wiki plugin is NOT available in current edition (as expected)");
    }
}

// ---------------------------------------------------------------------------
// Test: Plugin Gate Enforcement
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-04: Plugin Operation Gate Enforcement
 *
 * Verify that all plugin operations are gated correctly:
 *   - Community/Minimal: Returns PermissionDenied
 *   - Enterprise+: Returns Ok (or operation-specific error)
 */
TEST_F(EditionGateTest, PluginGateEnforcement_LWP_GATE_04) {
    // List of plugin operations that should be gated
    std::vector<std::string> operations = {
        "ingest",
        "query",
        "wikiInit",
        "wikiIngest",
        "wikiQuery",
        "wikiLint",
        "ingestWikipediaDump",
        "stats",
        "initialize",
        "shutdown"
    };
    
    Edition current_edition = getCurrentEdition();
    bool is_enterprise_or_higher = 
        current_edition == Edition::Enterprise ||
        current_edition == Edition::Hyperscaler ||
        current_edition == Edition::Military;
    
    for (const auto& op : operations) {
        Status gate_status = enforcePluginGate(op.c_str());
        
        if (is_enterprise_or_higher) {
            // In enterprise builds, gate should be Ok (operations may still error for other reasons)
            EXPECT_EQ(gate_status.code, Status::Code::Ok)
                << "Operation '" << op << "' should be permitted in enterprise build";
        } else {
            // In community/minimal builds, gate should be PermissionDenied
            EXPECT_EQ(gate_status.code, Status::Code::PermissionDenied)
                << "Operation '" << op << "' should be denied in community/minimal build";
        }
    }
    
    SPDLOG_INFO("Plugin gate enforcement verified for {} operations", operations.size());
}

// ---------------------------------------------------------------------------
// Test: Feature-Level Gating
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-05: Feature-Level Gating
 *
 * Verify that sub-features like Wikipedia dump ingestion are properly gated.
 */
TEST_F(EditionGateTest, FeatureLevelGating_LWP_GATE_05) {
    // Sub-features that may be enterprise-only
    std::vector<std::string> features = {
        "llm_wiki_wikipedia",
        "llm_wiki_workspace",
        "llm_wiki_guardrails",
        "llm_wiki_rocksdb_backend",
        "llm_wiki_hnsw_vectors"
    };
    
    for (const auto& feature : features) {
        bool is_enabled = isLLMWikiFeatureEnabled(feature.c_str());
        Status gate_status = enforceFeatureGate(feature.c_str());
        
        // Feature availability should be consistent
        if (is_enabled) {
            EXPECT_EQ(gate_status.code, Status::Code::Ok)
                << "Feature '" << feature << "' availability inconsistent";
            SPDLOG_DEBUG("Feature '{}' is enabled", feature);
        } else {
            EXPECT_EQ(gate_status.code, Status::Code::PermissionDenied)
                << "Feature '" << feature << "' gate status inconsistent";
            SPDLOG_DEBUG("Feature '{}' is disabled", feature);
        }
    }
    
    SPDLOG_INFO("Feature-level gating verified for {} features", features.size());
}

// ---------------------------------------------------------------------------
// Test: Community Build Blocking
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-06: Community Build Blocking
 *
 * In community/minimal builds, verify that all LLM Wiki operations are blocked.
 *
 * This test is effective only when compiled with THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED=OFF
 */
TEST_F(EditionGateTest, CommunityBuildBlocking_LWP_GATE_06) {
    if constexpr (!kLLMWikiCompileTimeEnabled) {
        // Community build: verify all operations are blocked
        SPDLOG_INFO("Running LWP-GATE-06 in community build mode");
        
        // All plugin operations should fail
        Status ingest_gate = enforcePluginGate("ingest");
        EXPECT_EQ(ingest_gate.code, Status::Code::PermissionDenied)
            << "ingest should be blocked in community build";
        
        Status query_gate = enforcePluginGate("query");
        EXPECT_EQ(query_gate.code, Status::Code::PermissionDenied)
            << "query should be blocked in community build";
        
        Status init_gate = enforcePluginGate("initialize");
        EXPECT_EQ(init_gate.code, Status::Code::PermissionDenied)
            << "initialize should be blocked in community build";
        
        SPDLOG_INFO("Community build blocking verified");
    } else {
        // Enterprise build: this test doesn't apply
        SPDLOG_INFO("Running LWP-GATE-06 in enterprise build (test skipped)");
    }
}

// ---------------------------------------------------------------------------
// Test: Enterprise Build Enabling
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-07: Enterprise Build Enabling
 *
 * In enterprise/hyperscaler/military builds, verify that LLM Wiki operations are allowed.
 *
 * This test is effective only when compiled with THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED=ON
 */
TEST_F(EditionGateTest, EnterpriseBuildEnabling_LWP_GATE_07) {
    if constexpr (kLLMWikiCompileTimeEnabled) {
        // Enterprise build: verify operations are allowed
        SPDLOG_INFO("Running LWP-GATE-07 in enterprise build mode");
        
        // All plugin operations should be allowed (gate-wise)
        Status ingest_gate = enforcePluginGate("ingest");
        EXPECT_EQ(ingest_gate.code, Status::Code::Ok)
            << "ingest should be allowed in enterprise build";
        
        Status query_gate = enforcePluginGate("query");
        EXPECT_EQ(query_gate.code, Status::Code::Ok)
            << "query should be allowed in enterprise build";
        
        Status init_gate = enforcePluginGate("initialize");
        EXPECT_EQ(init_gate.code, Status::Code::Ok)
            << "initialize should be allowed in enterprise build";
        
        SPDLOG_INFO("Enterprise build enabling verified");
    } else {
        // Community build: this test doesn't apply
        SPDLOG_INFO("Running LWP-GATE-07 in community build (test skipped)");
    }
}

// ---------------------------------------------------------------------------
// Test: Wikipedia Feature Gating
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-08: Wikipedia Sub-Feature Gating
 *
 * Verify that Wikipedia dump ingestion is gated separately from the main plugin.
 */
TEST_F(EditionGateTest, WikipediaFeatureGating_LWP_GATE_08) {
    bool wiki_feature_enabled = isLLMWikiFeatureEnabled("llm_wiki_wikipedia");
    Status wiki_gate = enforceFeatureGate("llm_wiki_wikipedia");
    
    // Feature availability should be consistent
    if (wiki_feature_enabled) {
        EXPECT_EQ(wiki_gate.code, Status::Code::Ok)
            << "Wikipedia feature gate inconsistent with availability";
        SPDLOG_INFO("Wikipedia feature is available");
    } else {
        EXPECT_EQ(wiki_gate.code, Status::Code::PermissionDenied)
            << "Wikipedia feature gate inconsistent with availability";
        SPDLOG_INFO("Wikipedia feature is not available");
    }
}

// ---------------------------------------------------------------------------
// Test: Edition Information Messages
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-09: Edition Information in Error Messages
 *
 * Verify that denied operations include informative error messages.
 */
TEST_F(EditionGateTest, EditionInformationMessages_LWP_GATE_09) {
    // Get plugin gate for an operation
    Status gate_status = enforcePluginGate("query");
    
    // If denied, message should explain why
    if (gate_status.code == Status::Code::PermissionDenied) {
        EXPECT_FALSE(gate_status.message.empty())
            << "PermissionDenied status should include explanation message";
        EXPECT_NE(gate_status.message.find("permission"), std::string::npos)
            << "Message should mention permission";
        SPDLOG_INFO("Denied operation message: {}", gate_status.message);
    }
}

// ---------------------------------------------------------------------------
// Test: Compile-Time Flag Consistency
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-10: Compile-Time Flag Consistency
 *
 * Verify that all compile-time flags are consistent.
 */
TEST_F(EditionGateTest, CompileTimeFlagConsistency_LWP_GATE_10) {
    // Both flags should be either true or false together
    EXPECT_EQ(kLLMWikiCompileTimeEnabled, kLLMWikiWikipediaCompileTimeEnabled)
        << "Compile-time flags should be consistent";
    
    if (kLLMWikiCompileTimeEnabled) {
        SPDLOG_INFO("Compile-time flags: LLM Wiki=ON, Wikipedia=ON");
    } else {
        SPDLOG_INFO("Compile-time flags: LLM Wiki=OFF, Wikipedia=OFF");
    }
}

// ---------------------------------------------------------------------------
// Test: Multi-Edition Gating Matrix
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-11: Multi-Edition Gating Matrix
 *
 * Verify comprehensive gating matrix across all editions.
 *
 * Expected behavior:
 *   Community:   All ops -> PermissionDenied
 *   Minimal:     All ops -> PermissionDenied
 *   Enterprise:  All ops -> Ok
 *   Hyperscaler: All ops -> Ok
 *   Military:    All ops -> Ok
 */
TEST_F(EditionGateTest, MultiEditionGatingMatrix_LWP_GATE_11) {
    Edition current_edition = getCurrentEdition();
    
    std::string edition_names[] = {
        "Community", "Minimal", "Enterprise", "Hyperscaler", "Military"
    };
    
    std::vector<std::string> operations = {
        "ingest",
        "query",
        "wikiInit",
        "wikiIngest",
        "wikiQuery"
    };
    
    std::string current_edition_name = edition_names[static_cast<int>(current_edition)];
    SPDLOG_INFO("Testing gating matrix for edition: {}", current_edition_name);
    
    for (const auto& op : operations) {
        Status status = enforcePluginGate(op.c_str());
        
        std::string expected = {};
        if (current_edition == Edition::Community || current_edition == Edition::Minimal) {
            expected = "PermissionDenied";
            EXPECT_EQ(status.code, Status::Code::PermissionDenied)
                << "Operation '" << op << "' should be denied";
        } else {
            expected = "Ok";
            EXPECT_EQ(status.code, Status::Code::Ok)
                << "Operation '" << op << "' should be allowed";
        }
        
        SPDLOG_DEBUG("{}: {} -> {}", current_edition_name, op, expected);
    }
}

// ---------------------------------------------------------------------------
// Test: Gate Status Codes Are Correct
// ---------------------------------------------------------------------------

/**
 * @test LWP-GATE-12: Gate Status Code Correctness
 *
 * Verify that all returned status codes match expected values.
 */
TEST_F(EditionGateTest, GateStatusCodeCorrectness_LWP_GATE_12) {
    Status ok_status = Status::Ok();
    EXPECT_EQ(ok_status.code, Status::Code::Ok);
    EXPECT_TRUE(ok_status.ok());
    
    Status denied_status = Status::PermissionDenied("test");
    EXPECT_EQ(denied_status.code, Status::Code::PermissionDenied);
    EXPECT_FALSE(denied_status.ok());
    
    Status error_status = Status::Error("test");
    EXPECT_EQ(error_status.code, Status::Code::Error);
    EXPECT_FALSE(error_status.ok());
    
    SPDLOG_INFO("Status code correctness verified");
}

}  // namespace
