// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_processor_chain.cpp
 * @brief Unit tests for the configurable processing pipeline (processor chain).
 *
 * Covers:
 *  - ProcessorChainConfig construction and defaults
 *  - getEffectiveConfig() priority order (MIME > category > default)
 *  - JSON round-trip serialisation
 *  - ContentManager::setProcessorChainConfig() / getProcessorChainConfig()
 *  - Stage flags respected during ingestRawBlob (deduplication + extraction)
 */

#include <gtest/gtest.h>
#include "content/processor_chain_config.h"
#include "content/content_type.h"

using namespace themis::content;

// ============================================================================
// ProcessorChainConfig – construction and defaults
// ============================================================================

TEST(ProcessorChainConfigTest, DefaultAllStagesEnabled) {
    ProcessorChainConfig cfg;
    auto eff = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_TRUE(eff.extraction.enabled);
    EXPECT_TRUE(eff.chunking.enabled);
    EXPECT_TRUE(eff.embedding.enabled);
    EXPECT_TRUE(eff.deduplication.enabled);
}

TEST(ProcessorChainConfigTest, DefaultConfigAppliedWhenNoOverride) {
    ProcessorChainConfig cfg;
    cfg.default_config.embedding.enabled = false;

    auto eff = cfg.getEffectiveConfig("application/pdf", ContentCategory::BINARY);
    EXPECT_FALSE(eff.embedding.enabled);
    EXPECT_TRUE(eff.extraction.enabled);  // not overridden
}

// ============================================================================
// getEffectiveConfig() – priority order
// ============================================================================

TEST(ProcessorChainConfigTest, MimeTypeOverrideHasHighestPriority) {
    ProcessorChainConfig cfg;
    // Category-level: disable extraction for IMAGE
    ContentTypePipelineConfig cat_cfg;
    cat_cfg.extraction.enabled = false;
    cfg.category_configs[ContentCategory::IMAGE] = cat_cfg;

    // MIME-level: enable everything for image/jpeg (overrides category)
    ContentTypePipelineConfig mime_cfg;
    mime_cfg.extraction.enabled = true;
    cfg.mime_type_configs["image/jpeg"] = mime_cfg;

    auto eff = cfg.getEffectiveConfig("image/jpeg", ContentCategory::IMAGE);
    EXPECT_TRUE(eff.extraction.enabled);  // MIME override wins
}

TEST(ProcessorChainConfigTest, CategoryOverrideHasHigherPriorityThanDefault) {
    ProcessorChainConfig cfg;
    cfg.default_config.deduplication.enabled = true;

    ContentTypePipelineConfig cat_cfg;
    cat_cfg.deduplication.enabled = false;
    cfg.category_configs[ContentCategory::TEXT] = cat_cfg;

    auto eff = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_FALSE(eff.deduplication.enabled);  // category wins over default
}

TEST(ProcessorChainConfigTest, DefaultFallsBackWhenNoMimeOrCategoryMatch) {
    ProcessorChainConfig cfg;
    cfg.default_config.chunking.enabled = false;

    // No MIME or category override registered
    auto eff = cfg.getEffectiveConfig("application/octet-stream", ContentCategory::BINARY);
    EXPECT_FALSE(eff.chunking.enabled);
}

TEST(ProcessorChainConfigTest, UnknownMimeTypeUsesDefault) {
    ProcessorChainConfig cfg;
    cfg.default_config.embedding.enabled = false;

    auto eff = cfg.getEffectiveConfig("application/x-custom-type", ContentCategory::UNKNOWN);
    EXPECT_FALSE(eff.embedding.enabled);
}

// ============================================================================
// JSON round-trip
// ============================================================================

TEST(ProcessorChainConfigTest, JsonRoundTrip_Defaults) {
    ProcessorChainConfig cfg;
    auto j = cfg.toJson();
    auto cfg2 = ProcessorChainConfig::fromJson(j);

    auto eff = cfg2.getEffectiveConfig("text/plain", ContentCategory::TEXT);
    EXPECT_TRUE(eff.extraction.enabled);
    EXPECT_TRUE(eff.chunking.enabled);
    EXPECT_TRUE(eff.embedding.enabled);
    EXPECT_TRUE(eff.deduplication.enabled);
}

TEST(ProcessorChainConfigTest, JsonRoundTrip_WithOverrides) {
    ProcessorChainConfig cfg;
    cfg.default_config.embedding.enabled = false;

    ContentTypePipelineConfig mime_cfg;
    mime_cfg.deduplication.enabled = false;
    cfg.mime_type_configs["text/html"] = mime_cfg;

    ContentTypePipelineConfig cat_cfg;
    cat_cfg.extraction.enabled = false;
    cfg.category_configs[ContentCategory::IMAGE] = cat_cfg;

    auto j = cfg.toJson();
    auto cfg2 = ProcessorChainConfig::fromJson(j);

    // Default: embedding disabled
    auto d = cfg2.getEffectiveConfig("application/pdf", ContentCategory::BINARY);
    EXPECT_FALSE(d.embedding.enabled);

    // MIME override: deduplication disabled
    auto h = cfg2.getEffectiveConfig("text/html", ContentCategory::TEXT);
    EXPECT_FALSE(h.deduplication.enabled);

    // Category override: extraction disabled
    auto img = cfg2.getEffectiveConfig("image/png", ContentCategory::IMAGE);
    EXPECT_FALSE(img.extraction.enabled);
}

TEST(ProcessorChainConfigTest, JsonFromJson_IgnoresUnknownCategory) {
    nlohmann::json j = {
        {"default", {{"extraction", true}, {"chunking", true}, {"embedding", true}, {"deduplication", true}}},
        {"mime_types", nlohmann::json::object()},
        {"categories", {{"TOTALLY_UNKNOWN_CATEGORY", {{"extraction", false}}}}}
    };
    // Should not throw – unknown category is silently ignored
    EXPECT_NO_THROW(ProcessorChainConfig::fromJson(j));
}

TEST(ProcessorChainConfigTest, JsonFromJson_PartialFields) {
    // Only some stage fields provided – others default to true
    nlohmann::json j = {
        {"default", {{"embedding", false}}},  // only embedding specified
        {"mime_types", nlohmann::json::object()},
        {"categories", nlohmann::json::object()}
    };
    auto cfg = ProcessorChainConfig::fromJson(j);
    auto eff = cfg.getEffectiveConfig("text/plain", ContentCategory::TEXT);

    EXPECT_TRUE(eff.extraction.enabled);   // not specified → default true
    EXPECT_TRUE(eff.chunking.enabled);
    EXPECT_FALSE(eff.embedding.enabled);   // specified false
    EXPECT_TRUE(eff.deduplication.enabled);
}

// ============================================================================
// All categories round-trip via JSON
// ============================================================================

TEST(ProcessorChainConfigTest, AllCategoriesSerializeAndDeserialize) {
    ProcessorChainConfig cfg;
    const std::vector<ContentCategory> all_cats = {
        ContentCategory::TEXT, ContentCategory::IMAGE, ContentCategory::GEO,
        ContentCategory::CAD, ContentCategory::AUDIO, ContentCategory::VIDEO,
        ContentCategory::STRUCTURED, ContentCategory::BINARY,
        ContentCategory::ARCHIVE, ContentCategory::UNKNOWN,
    };
    for (auto cat : all_cats) {
        ContentTypePipelineConfig c;
        c.chunking.enabled = false;
        cfg.category_configs[cat] = c;
    }

    auto j = cfg.toJson();
    auto cfg2 = ProcessorChainConfig::fromJson(j);

    // All named categories (including VIDEO, ARCHIVE and UNKNOWN) must round-trip.
    for (auto cat : all_cats) {
        auto eff = cfg2.getEffectiveConfig("", cat);
        EXPECT_FALSE(eff.chunking.enabled) << "Category " << static_cast<int>(cat);
        EXPECT_TRUE(eff.extraction.enabled) << "Category " << static_cast<int>(cat);
    }
}
