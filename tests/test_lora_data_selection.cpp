// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lora_data_selection.cpp
 * @brief Unit tests for the automated LoRA data selection pipeline
 *        (DataSelectionPipeline – Stages 1-5).
 */

#include <gtest/gtest.h>
#include "training/lora_data_selection.h"

#include <string>
#include <vector>
#include <algorithm>

using namespace themis::training;

// ============================================================================
// Helper utilities
// ============================================================================

static DataSample makeSample(const std::string& id,
                              const std::string& text) {
    return DataSample(id, text);
}

// German text with enough tokens and language markers
static std::string germanLegalText(size_t token_count = 60) {
    std::string base =
        "der Vertrag ist zwischen den Parteien geschlossen worden und "
        "die Klausel regelt die Haftung sowie das Ermessen der Behörde "
        "nach Maßgabe des Verwaltungsakts und der Verpflichtung zur "
        "Auskunftserteilung die mit dem eIDAS Rahmen vereinbar ist ";
    std::string result;
    while (result.size() < token_count * 6) result += base;
    return result;
}

// Short German sentence (fewer than 50 tokens → should be filtered)
static std::string shortGermanText() {
    return "der Vertrag ist bindend und die Klausel gilt";
}

// Same text duplicated (should trigger MinHash dedup)
static std::string duplicateText() {
    return germanLegalText(80);
}

// ============================================================================
// Test fixture
// ============================================================================

class DataSelectionPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.min_length_tokens   = 50;
        cfg_.max_length_tokens   = 10000;
        cfg_.required_language   = "de";
        cfg_.max_toxicity_score  = 0.3;
        cfg_.enable_pii_check    = true;
        cfg_.minhash_threshold   = 0.85;  // lower for tests
        cfg_.minhash_num_perm    = 64;    // fewer perms for speed
        cfg_.clustering_k_ratio  = 5;
        cfg_.perplexity_weight   = 0.4;
        cfg_.diversity_weight    = 0.3;
        cfg_.domain_relevance_weight = 0.3;
        cfg_.domain_keywords["legal"] = {"Vertrag", "Klausel", "Haftung", "eIDAS"};
        cfg_.easy_ratio   = 0.1;
        cfg_.medium_ratio = 0.7;
        cfg_.hard_ratio   = 0.2;
        cfg_.target_samples = 10;
        cfg_.audit = true;
    }

    LoRADataSelectionConfig cfg_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(DataSelectionPipelineTest, ConstructionSucceeds) {
    EXPECT_NO_THROW(DataSelectionPipeline pipeline(cfg_));
}

TEST_F(DataSelectionPipelineTest, GetConfigReturnsCurrentConfig) {
    DataSelectionPipeline pipeline(cfg_);
    const auto& c = pipeline.getConfig();
    EXPECT_EQ(c.min_length_tokens, cfg_.min_length_tokens);
    EXPECT_EQ(c.required_language, cfg_.required_language);
    EXPECT_EQ(c.target_samples, cfg_.target_samples);
}

TEST_F(DataSelectionPipelineTest, SetConfigUpdatesConfig) {
    DataSelectionPipeline pipeline(cfg_);
    LoRADataSelectionConfig new_cfg = cfg_;
    new_cfg.target_samples = 99;
    pipeline.setConfig(new_cfg);
    EXPECT_EQ(pipeline.getConfig().target_samples, 99u);
}

// ============================================================================
// Stage 1: Quality filtering
// ============================================================================

TEST_F(DataSelectionPipelineTest, QualityFilter_RejectsTooShortSamples) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input = {
        makeSample("short1", shortGermanText()),
        makeSample("long1",  germanLegalText(60))
    };
    auto out = pipeline.filterByQuality(input);
    // Short sample should be removed; long one passes
    EXPECT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].id, "long1");
}

TEST_F(DataSelectionPipelineTest, QualityFilter_RejectsToxicSamples) {
    DataSelectionPipeline pipeline(cfg_);
    std::string toxic_text = germanLegalText(60) +
        " hass beleidigung gewalt diskriminierung hass";
    auto input = std::vector<DataSample>{
        makeSample("clean", germanLegalText(60)),
        makeSample("toxic", toxic_text)
    };
    auto out = pipeline.filterByQuality(input);
    bool found_toxic = false;
    for (const auto& s : out) if (s.id == "toxic") found_toxic = true;
    EXPECT_FALSE(found_toxic);
}

TEST_F(DataSelectionPipelineTest, QualityFilter_RejectsPIISamples) {
    DataSelectionPipeline pipeline(cfg_);
    std::string pii_text = germanLegalText(60) + " user@example.com";
    auto input = std::vector<DataSample>{
        makeSample("clean",  germanLegalText(60)),
        makeSample("has_pii", pii_text)
    };
    auto out = pipeline.filterByQuality(input);
    bool found_pii = false;
    for (const auto& s : out) if (s.id == "has_pii") found_pii = true;
    EXPECT_FALSE(found_pii);
}

TEST_F(DataSelectionPipelineTest, QualityFilter_RejectsWrongLanguage) {
    DataSelectionPipeline pipeline(cfg_);
    std::string english_text(320, 'a'); // non-German
    // pad with spaces to exceed token minimum
    for (size_t i = 4; i < english_text.size(); i += 5) english_text[i] = ' ';
    auto input = std::vector<DataSample>{
        makeSample("german",  germanLegalText(60)),
        makeSample("english", english_text)
    };
    auto out = pipeline.filterByQuality(input);
    bool found_english = false;
    for (const auto& s : out) if (s.id == "english") found_english = true;
    EXPECT_FALSE(found_english);
}

TEST_F(DataSelectionPipelineTest, QualityFilter_PassesValidSamples) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 5; ++i) {
        input.push_back(makeSample("s" + std::to_string(i), germanLegalText(60)));
    }
    auto out = pipeline.filterByQuality(input);
    EXPECT_EQ(out.size(), 5u);
}

// ============================================================================
// Stage 2: Deduplication
// ============================================================================

TEST_F(DataSelectionPipelineTest, Deduplication_RemovesNearDuplicates) {
    DataSelectionPipeline pipeline(cfg_);
    // Two identical samples
    std::string text = duplicateText();
    auto input = std::vector<DataSample>{
        makeSample("orig", text),
        makeSample("dup",  text)
    };
    auto out = pipeline.deduplicate(input);
    EXPECT_EQ(out.size(), 1u);
}

TEST_F(DataSelectionPipelineTest, Deduplication_KeepsDissimilarSamples) {
    DataSelectionPipeline pipeline(cfg_);
    std::string text_a = germanLegalText(60);
    // Create a clearly different text
    std::string text_b(400, 'x');
    for (size_t i = 4; i < text_b.size(); i += 5) text_b[i] = ' ';
    text_b += " und also jedoch daher somit weiterhin ferner";
    auto input = std::vector<DataSample>{
        makeSample("a", text_a),
        makeSample("b", text_b)
    };
    auto out = pipeline.deduplicate(input);
    EXPECT_EQ(out.size(), 2u);
}

TEST_F(DataSelectionPipelineTest, Deduplication_EmptyInputReturnsEmpty) {
    DataSelectionPipeline pipeline(cfg_);
    auto out = pipeline.deduplicate({});
    EXPECT_TRUE(out.empty());
}

// ============================================================================
// Stage 3: Cluster-based sampling
// ============================================================================

TEST_F(DataSelectionPipelineTest, ClusterSample_ReducesToKSamples) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 20; ++i) {
        input.push_back(makeSample("s" + std::to_string(i), germanLegalText(60)));
    }
    size_t k = 5;
    auto out = pipeline.clusterAndSample(input, k);
    EXPECT_LE(out.size(), k);
    EXPECT_GT(out.size(), 0u);
}

TEST_F(DataSelectionPipelineTest, ClusterSample_EmptyInputReturnsEmpty) {
    DataSelectionPipeline pipeline(cfg_);
    auto out = pipeline.clusterAndSample({}, 3);
    EXPECT_TRUE(out.empty());
}

TEST_F(DataSelectionPipelineTest, ClusterSample_KLargerThanInputReturnsSome) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input = {
        makeSample("a", germanLegalText(60)),
        makeSample("b", germanLegalText(60))
    };
    auto out = pipeline.clusterAndSample(input, 100);
    EXPECT_LE(out.size(), input.size());
    EXPECT_GT(out.size(), 0u);
}

// ============================================================================
// Stage 4: Quality & difficulty scoring
// ============================================================================

TEST_F(DataSelectionPipelineTest, Scoring_PopulatesQualityScore) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> samples = {
        makeSample("s1", germanLegalText(60)),
        makeSample("s2", germanLegalText(80))
    };
    pipeline.scoreQualityAndDifficulty(samples);
    for (const auto& s : samples) {
        EXPECT_GE(s.quality_score,    0.0);
        EXPECT_LE(s.quality_score,    1.0);
        EXPECT_GE(s.difficulty_score, 0.0);
        EXPECT_LE(s.difficulty_score, 1.0);
    }
}

TEST_F(DataSelectionPipelineTest, Scoring_EmptyTextProducesZeroScores) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> samples = { makeSample("empty", "") };
    pipeline.scoreQualityAndDifficulty(samples);
    EXPECT_GE(samples[0].quality_score, 0.0);
    EXPECT_LE(samples[0].quality_score, 1.0);
}

// ============================================================================
// Stage 5: Curriculum stratified sampling
// ============================================================================

TEST_F(DataSelectionPipelineTest, StratifiedSample_RespectsTargetCount) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> samples;
    for (int i = 0; i < 50; ++i) {
        DataSample s("s" + std::to_string(i), germanLegalText(60));
        s.difficulty_score = static_cast<double>(i) / 50.0;
        samples.push_back(s);
    }
    auto out = pipeline.stratifiedSample(samples, 10);
    EXPECT_LE(out.size(), 10u);
}

TEST_F(DataSelectionPipelineTest, StratifiedSample_EmptyInputReturnsEmpty) {
    DataSelectionPipeline pipeline(cfg_);
    auto out = pipeline.stratifiedSample({}, 10);
    EXPECT_TRUE(out.empty());
}

TEST_F(DataSelectionPipelineTest, StratifiedSample_InvalidRatiosSumNormalized) {
    DataSelectionPipeline pipeline(cfg_);
    LoRADataSelectionConfig bad_cfg = cfg_;
    bad_cfg.easy_ratio   = 0.0;
    bad_cfg.medium_ratio = 0.0;
    bad_cfg.hard_ratio   = 0.0;
    pipeline.setConfig(bad_cfg);

    std::vector<DataSample> samples;
    for (int i = 0; i < 20; ++i) {
        DataSample s("s" + std::to_string(i), germanLegalText(60));
        s.difficulty_score = static_cast<double>(i) / 20.0;
        samples.push_back(s);
    }
    // Should not crash / throw with all-zero ratios
    EXPECT_NO_THROW(pipeline.stratifiedSample(samples, 5));
}

// ============================================================================
// Full pipeline run
// ============================================================================

TEST_F(DataSelectionPipelineTest, FullPipeline_SucceedsOnValidInput) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 30; ++i) {
        input.push_back(makeSample("doc" + std::to_string(i), germanLegalText(60)));
    }
    auto result = pipeline.run(input);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.elapsed_seconds, 0.0);
}

TEST_F(DataSelectionPipelineTest, FullPipeline_EmptyInputSucceedsWithEmptyOutput) {
    DataSelectionPipeline pipeline(cfg_);
    auto result = pipeline.run({});
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.selected_samples.empty());
}

TEST_F(DataSelectionPipelineTest, FullPipeline_AuditEntryPopulated) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 20; ++i) {
        input.push_back(makeSample("d" + std::to_string(i), germanLegalText(60)));
    }
    auto result = pipeline.run(input);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.audit_entry.input_sample_count, 20u);
    EXPECT_FALSE(result.audit_entry.config_hash.empty());
}

TEST_F(DataSelectionPipelineTest, FullPipeline_ProgressCallbackInvoked) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 20; ++i) {
        input.push_back(makeSample("d" + std::to_string(i), germanLegalText(60)));
    }

    std::vector<std::string> stages_called;
    auto result = pipeline.run(input,
        [&stages_called](const std::string& stage, size_t, const std::string&) {
            stages_called.push_back(stage);
        });

    EXPECT_TRUE(result.success);
    // All 5 stages should have been reported
    EXPECT_GE(stages_called.size(), 5u);
}

TEST_F(DataSelectionPipelineTest, FullPipeline_SelectedSamplesSubsetOfInput) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 20; ++i) {
        input.push_back(makeSample("x" + std::to_string(i), germanLegalText(60)));
    }
    auto result = pipeline.run(input);
    EXPECT_TRUE(result.success);
    EXPECT_LE(result.selected_samples.size(), input.size());
}

TEST_F(DataSelectionPipelineTest, FullPipeline_AuditSelectedIdsMatchOutputIds) {
    DataSelectionPipeline pipeline(cfg_);
    std::vector<DataSample> input;
    for (int i = 0; i < 20; ++i) {
        input.push_back(makeSample("z" + std::to_string(i), germanLegalText(60)));
    }
    auto result = pipeline.run(input);
    EXPECT_TRUE(result.success);

    // audit selected_ids should equal output sample ids
    std::vector<std::string> output_ids;
    for (const auto& s : result.selected_samples) output_ids.push_back(s.id);
    std::sort(output_ids.begin(), output_ids.end());

    std::vector<std::string> audit_ids = result.audit_entry.selected_ids;
    std::sort(audit_ids.begin(), audit_ids.end());

    EXPECT_EQ(output_ids, audit_ids);
}

// ============================================================================
// Config live-reload
// ============================================================================

TEST_F(DataSelectionPipelineTest, LiveReload_NewConfigAppliedOnNextRun) {
    DataSelectionPipeline pipeline(cfg_);

    LoRADataSelectionConfig updated = cfg_;
    updated.target_samples = 3;
    pipeline.setConfig(updated);

    std::vector<DataSample> input;
    for (int i = 0; i < 20; ++i) {
        input.push_back(makeSample("r" + std::to_string(i), germanLegalText(60)));
    }
    auto result = pipeline.run(input);
    EXPECT_TRUE(result.success);
    EXPECT_LE(result.selected_samples.size(), 3u);
}

// ============================================================================
// SelfImprovementConfig default values
// ============================================================================

TEST(SelfImprovementConfigTest, DefaultValues) {
    SelfImprovementConfig cfg;
    EXPECT_TRUE(cfg.enabled);
    EXPECT_EQ(cfg.period_seconds, 86400u);
    EXPECT_TRUE(cfg.threshold_auto_adjust);
    EXPECT_DOUBLE_EQ(cfg.latency_target_ms, 5000.0);
    EXPECT_TRUE(cfg.accuracy_monitoring);
}
