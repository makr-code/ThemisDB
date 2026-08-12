// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_pipeline_hardening.cpp
 * @brief Unit tests for per-stage error recovery and retry logic in the
 *        content ingestion pipeline (Issue #1701 / roadmap Phase 2).
 *
 * Covers:
 *  - StageConfig default values and retry field round-trip via JSON
 *  - ContentTypePipelineConfig now includes a storage stage
 *  - ProcessorChainConfig serialisation with retry object syntax
 *  - Backward compatibility: old boolean stage values still parse correctly
 *  - continue_on_error and max_retries fields survive JSON round-trip
 */

#include <gtest/gtest.h>
#include "content/processor_chain_config.h"
#include "content/content_type.h"

using namespace themis::content;

// ============================================================================
// StageConfig – default values
// ============================================================================

TEST(PipelineHardeningTest, StageConfig_Defaults) {
    StageConfig s;
    EXPECT_TRUE(s.enabled);
    EXPECT_EQ(s.max_retries, 0);
    EXPECT_EQ(s.retry_delay_ms, 100);
    EXPECT_FALSE(s.continue_on_error);
}

TEST(PipelineHardeningTest, StageConfig_CanBeModified) {
    StageConfig s;
    s.enabled = false;
    s.max_retries = 3;
    s.retry_delay_ms = 250;
    s.continue_on_error = true;

    EXPECT_FALSE(s.enabled);
    EXPECT_EQ(s.max_retries, 3);
    EXPECT_EQ(s.retry_delay_ms, 250);
    EXPECT_TRUE(s.continue_on_error);
}

// ============================================================================
// ContentTypePipelineConfig – storage stage present
// ============================================================================

TEST(PipelineHardeningTest, ContentTypePipelineConfig_HasStorageStage) {
    ContentTypePipelineConfig cfg;
    // storage stage must exist and default to enabled with no retries
    EXPECT_TRUE(cfg.storage.enabled);
    EXPECT_EQ(cfg.storage.max_retries, 0);
    EXPECT_FALSE(cfg.storage.continue_on_error);
}

TEST(PipelineHardeningTest, ContentTypePipelineConfig_AllStagesDefaultEnabled) {
    ContentTypePipelineConfig cfg;
    EXPECT_TRUE(cfg.extraction.enabled);
    EXPECT_TRUE(cfg.chunking.enabled);
    EXPECT_TRUE(cfg.embedding.enabled);
    EXPECT_TRUE(cfg.deduplication.enabled);
    EXPECT_TRUE(cfg.storage.enabled);
}

// ============================================================================
// ProcessorChainConfig JSON round-trip with retry fields
// ============================================================================

TEST(PipelineHardeningTest, JsonRoundTrip_StorageStage_Default) {
    ProcessorChainConfig cfg;
    auto j   = cfg.toJson();
    auto cfg2 = ProcessorChainConfig::fromJson(j);

    auto eff = cfg2.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_TRUE(eff.storage.enabled);
    EXPECT_EQ(eff.storage.max_retries, 0);
    EXPECT_FALSE(eff.storage.continue_on_error);
}

TEST(PipelineHardeningTest, JsonRoundTrip_RetryFields) {
    ProcessorChainConfig cfg;
    cfg.default_config.extraction.max_retries      = 2;
    cfg.default_config.extraction.retry_delay_ms   = 200;
    cfg.default_config.extraction.continue_on_error = true;
    cfg.default_config.storage.max_retries          = 3;
    cfg.default_config.storage.retry_delay_ms       = 500;

    auto j    = cfg.toJson();
    auto cfg2  = ProcessorChainConfig::fromJson(j);
    auto eff   = cfg2.getEffectiveConfig("text/html", ContentCategory::TEXT);

    EXPECT_EQ(eff.extraction.max_retries, 2);
    EXPECT_EQ(eff.extraction.retry_delay_ms, 200);
    EXPECT_TRUE(eff.extraction.continue_on_error);
    EXPECT_EQ(eff.storage.max_retries, 3);
    EXPECT_EQ(eff.storage.retry_delay_ms, 500);
}

TEST(PipelineHardeningTest, JsonRoundTrip_MimeTypeOverrideWithRetry) {
    ProcessorChainConfig cfg;
    ContentTypePipelineConfig html_cfg;
    html_cfg.extraction.max_retries      = 1;
    html_cfg.extraction.continue_on_error = true;
    html_cfg.storage.max_retries          = 2;
    cfg.mime_type_configs["text/html"] = html_cfg;

    auto j    = cfg.toJson();
    auto cfg2  = ProcessorChainConfig::fromJson(j);

    auto eff = cfg2.getEffectiveConfig("text/html", ContentCategory::TEXT);
    EXPECT_EQ(eff.extraction.max_retries, 1);
    EXPECT_TRUE(eff.extraction.continue_on_error);
    EXPECT_EQ(eff.storage.max_retries, 2);

    // Other MIME types fall back to default (no retry)
    auto plain = cfg2.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_EQ(plain.extraction.max_retries, 0);
    EXPECT_FALSE(plain.extraction.continue_on_error);
}

TEST(PipelineHardeningTest, JsonRoundTrip_CategoryOverrideWithRetry) {
    ProcessorChainConfig cfg;
    ContentTypePipelineConfig img_cfg;
    img_cfg.storage.max_retries    = 5;
    img_cfg.storage.retry_delay_ms = 1000;
    cfg.category_configs[ContentCategory::IMAGE] = img_cfg;

    auto j    = cfg.toJson();
    auto cfg2  = ProcessorChainConfig::fromJson(j);

    auto eff = cfg2.getEffectiveConfig("image/jpeg", ContentCategory::IMAGE);
    EXPECT_EQ(eff.storage.max_retries, 5);
    EXPECT_EQ(eff.storage.retry_delay_ms, 1000);
}

// ============================================================================
// Backward compatibility: old boolean JSON values still parse correctly
// ============================================================================

TEST(PipelineHardeningTest, BackwardCompat_BooleanStagesStillParse) {
    // Old-format JSON (booleans only, no storage key)
    nlohmann::json j = {
        {"default", {
            {"extraction", true},
            {"chunking", true},
            {"embedding", false},
            {"deduplication", true}
        }},
        {"mime_types", nlohmann::json::object()},
        {"categories", nlohmann::json::object()}
    };

    EXPECT_NO_THROW({
        auto cfg = ProcessorChainConfig::fromJson(j);
        auto eff = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);
        EXPECT_TRUE(eff.extraction.enabled);
        EXPECT_TRUE(eff.chunking.enabled);
        EXPECT_FALSE(eff.embedding.enabled);
        EXPECT_TRUE(eff.deduplication.enabled);
        // storage not in old JSON → defaults to enabled, 0 retries
        EXPECT_TRUE(eff.storage.enabled);
        EXPECT_EQ(eff.storage.max_retries, 0);
    });
}

TEST(PipelineHardeningTest, BackwardCompat_MixedBoolAndObjectPerStage) {
    // Mixed: some stages as booleans, one as full object
    nlohmann::json j = {
        {"default", {
            {"extraction", {{"enabled", true}, {"max_retries", 2}, {"continue_on_error", true}}},
            {"chunking", true},
            {"embedding", false},
            {"deduplication", true},
            {"storage", {{"enabled", true}, {"max_retries", 3}, {"retry_delay_ms", 300}}}
        }},
        {"mime_types", nlohmann::json::object()},
        {"categories", nlohmann::json::object()}
    };

    auto cfg = ProcessorChainConfig::fromJson(j);
    auto eff  = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);

    EXPECT_TRUE(eff.extraction.enabled);
    EXPECT_EQ(eff.extraction.max_retries, 2);
    EXPECT_TRUE(eff.extraction.continue_on_error);

    EXPECT_TRUE(eff.chunking.enabled);
    EXPECT_EQ(eff.chunking.max_retries, 0);  // boolean → defaults apply

    EXPECT_FALSE(eff.embedding.enabled);

    EXPECT_TRUE(eff.storage.enabled);
    EXPECT_EQ(eff.storage.max_retries, 3);
    EXPECT_EQ(eff.storage.retry_delay_ms, 300);
}

// ============================================================================
// getEffectiveConfig priority still respected with retry fields
// ============================================================================

TEST(PipelineHardeningTest, PriorityOrder_MimeOverrideTrumpsCategory) {
    ProcessorChainConfig cfg;

    ContentTypePipelineConfig cat_cfg;
    cat_cfg.storage.max_retries = 10;
    cfg.category_configs[ContentCategory::TEXT] = cat_cfg;

    ContentTypePipelineConfig mime_cfg;
    mime_cfg.storage.max_retries = 1;
    cfg.mime_type_configs["text/plain"] = mime_cfg;

    auto eff = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_EQ(eff.storage.max_retries, 1);  // MIME wins
}

TEST(PipelineHardeningTest, PriorityOrder_CategoryTrumpsDefault) {
    ProcessorChainConfig cfg;
    cfg.default_config.extraction.max_retries = 5;

    ContentTypePipelineConfig cat_cfg;
    cat_cfg.extraction.max_retries = 0;  // no retry for this category
    cfg.category_configs[ContentCategory::BINARY] = cat_cfg;

    auto eff_binary = cfg.getEffectiveConfig("application/octet-stream", ContentCategory::BINARY);
    EXPECT_EQ(eff_binary.extraction.max_retries, 0);  // category wins

    auto eff_text = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_EQ(eff_text.extraction.max_retries, 5);  // default applies
}

// ============================================================================
// Serialisation: stages with non-default retry fields produce object JSON
// ============================================================================

TEST(PipelineHardeningTest, ToJson_NonDefaultRetryProducesObject) {
    ProcessorChainConfig cfg;
    cfg.default_config.storage.max_retries = 3;

    auto j = cfg.toJson();
    // storage should be serialized as an object (not just a boolean)
    ASSERT_TRUE(j["default"].contains("storage"));
    EXPECT_TRUE(j["default"]["storage"].is_object());
    EXPECT_EQ(j["default"]["storage"]["max_retries"], 3);
}

TEST(PipelineHardeningTest, ToJson_DefaultRetryProducesBoolean) {
    ProcessorChainConfig cfg;
    // All default: max_retries=0, retry_delay_ms=100, continue_on_error=false
    auto j = cfg.toJson();

    // All stage values should be simple booleans (backward-compatible)
    ASSERT_TRUE(j["default"].contains("extraction"));
    EXPECT_TRUE(j["default"]["extraction"].is_boolean());
    ASSERT_TRUE(j["default"].contains("storage"));
    EXPECT_TRUE(j["default"]["storage"].is_boolean());
}
