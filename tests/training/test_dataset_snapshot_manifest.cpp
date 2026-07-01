/**
 * @file test_dataset_snapshot_manifest.cpp
 * @brief Tests for dataset snapshot manifest and related components
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "training/dataset_snapshot_manifest.h"
#include "training/eligibility_policy_engine.h"
#include "training/dataset_split_manager.h"
#include <memory>
#include <chrono>

using namespace themis::training;

// ============================================================================
// DatasetSnapshotManifest Tests
// ============================================================================

class DatasetSnapshotManifestTest : public ::testing::Test {
protected:
    DatasetSnapshotManifest createTestManifest() {
        DatasetSnapshotManifest manifest;
        manifest.snapshot_id = "snapshot-test-001";
        manifest.name = "Test Dataset v1";
        manifest.description = "Test dataset for unit testing";
        manifest.total_samples = 1000;
        manifest.train_samples = 700;
        manifest.validation_samples = 150;
        manifest.test_samples = 150;
        manifest.avg_quality_score = 0.85;
        manifest.avg_difficulty_score = 0.55;
        return manifest;
    }
};

TEST_F(DatasetSnapshotManifestTest, SerializeToJSON) {
    auto manifest = createTestManifest();
    std::string json = manifest.toJSON();
    
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("snapshot-test-001"), std::string::npos);
    EXPECT_NE(json.find("Test Dataset v1"), std::string::npos);
}

TEST_F(DatasetSnapshotManifestTest, SerializeToYAML) {
    auto manifest = createTestManifest();
    std::string yaml = manifest.toYAML();
    
    EXPECT_FALSE(yaml.empty());
    EXPECT_NE(yaml.find("snapshot_id:"), std::string::npos);
    EXPECT_NE(yaml.find("train_samples:"), std::string::npos);
}

TEST_F(DatasetSnapshotManifestTest, ComputeChecksum) {
    auto manifest = createTestManifest();
    manifest.updateChecksum();
    
    EXPECT_FALSE(manifest.content_checksum.empty());
    EXPECT_EQ(manifest.content_checksum.length(), 64); // SHA-256 hex string
}

TEST_F(DatasetSnapshotManifestTest, VerifyIntegrity) {
    auto manifest = createTestManifest();
    manifest.updateChecksum();
    
    EXPECT_TRUE(manifest.verifyIntegrity());
}

TEST_F(DatasetSnapshotManifestTest, GetSplitStatistics) {
    auto manifest = createTestManifest();
    std::string stats = manifest.getSplitStatistics();
    
    EXPECT_NE(stats.find("Training Set:"), std::string::npos);
    EXPECT_NE(stats.find("700"), std::string::npos);
}

TEST_F(DatasetSnapshotManifestTest, GetDomainStatistics) {
    auto manifest = createTestManifest();
    manifest.domain_distribution["legal"] = 300;
    manifest.domain_distribution["medical"] = 700;
    
    std::string stats = manifest.getDomainStatistics();
    
    EXPECT_NE(stats.find("legal"), std::string::npos);
    EXPECT_NE(stats.find("medical"), std::string::npos);
}

// ============================================================================
// EligibilityPolicy Tests
// ============================================================================

class EligibilityPolicyTest : public ::testing::Test {
protected:
    EligibilityPolicy createTestPolicy() {
        EligibilityPolicy policy;
        policy.policy_version = "1.0";
        policy.min_quality_score = 0.5;
        policy.max_difficulty_score = 0.95;
        policy.pii_handling = "reject";
        return policy;
    }
};

TEST_F(EligibilityPolicyTest, SerializeToJSON) {
    auto policy = createTestPolicy();
    std::string json = policy.toJSON();
    
    EXPECT_NE(json.find("policy_version"), std::string::npos);
    EXPECT_NE(json.find("1.0"), std::string::npos);
}

// ============================================================================
// SampleLineage Tests
// ============================================================================

class SampleLineageTest : public ::testing::Test {
protected:
    SampleLineage createTestLineage() {
        SampleLineage lineage;
        lineage.sample_id = "sample-001";
        lineage.source_document_id = "doc-001";
        lineage.processing_version = "v1.0";
        lineage.modality = "text";
        return lineage;
    }
};

TEST_F(SampleLineageTest, SerializeToJSON) {
    auto lineage = createTestLineage();
    std::string json = lineage.toJSON();
    
    EXPECT_NE(json.find("sample-001"), std::string::npos);
    EXPECT_NE(json.find("doc-001"), std::string::npos);
}

// ============================================================================
// EligibilityPolicyEngine Tests
// ============================================================================

class EligibilityPolicyEngineTest : public ::testing::Test {
protected:
    EligibilityPolicy createTestPolicy() {
        EligibilityPolicy policy;
        policy.min_quality_score = 0.5;
        policy.max_difficulty_score = 0.95;
        policy.required_languages = {"en"};
        return policy;
    }

    DataSample createTestSample(std::string id = "test-001",
                               double quality = 0.8,
                               double difficulty = 0.5) {
        DataSample sample(id, "Test content");
        sample.language = "en";
        sample.quality_score = quality;
        sample.difficulty_score = difficulty;
        return sample;
    }
};

TEST_F(EligibilityPolicyEngineTest, EvaluateSamplePassesQualityCheck) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    auto sample = createTestSample("test-001", 0.8, 0.5);
    auto result = engine.evaluateSample(sample);
    
    EXPECT_TRUE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, "");
}

TEST_F(EligibilityPolicyEngineTest, EvaluateSampleFailsQualityCheck) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    auto sample = createTestSample("test-002", 0.3, 0.5); // Below threshold
    auto result = engine.evaluateSample(sample);
    
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, "quality_score_too_low");
}

TEST_F(EligibilityPolicyEngineTest, EvaluateSampleFailsDifficultyCheck) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    auto sample = createTestSample("test-003", 0.8, 0.98); // Above threshold
    auto result = engine.evaluateSample(sample);
    
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, "difficulty_score_too_high");
}

TEST_F(EligibilityPolicyEngineTest, RecordSampleLineage) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    SampleLineage lineage;
    lineage.sample_id = "test-001";
    lineage.source_document_id = "doc-001";
    
    EXPECT_TRUE(engine.recordSampleLineage("test-001", lineage));
    EXPECT_FALSE(engine.recordSampleLineage("test-001", lineage)); // Duplicate
}

TEST_F(EligibilityPolicyEngineTest, GetLineageHistory) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    SampleLineage lineage;
    lineage.sample_id = "test-001";
    lineage.source_document_id = "doc-001";
    
    engine.recordSampleLineage("test-001", lineage);
    
    auto history = engine.getLineageHistory("test-001");
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].sample_id, "test-001");
}

TEST_F(EligibilityPolicyEngineTest, ValidateSampleIdUniqueness) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    SampleLineage lineage;
    lineage.sample_id = "test-001";
    
    engine.recordSampleLineage("test-001", lineage);
    
    EXPECT_FALSE(engine.validateSampleIdUniqueness("test-001"));
    EXPECT_TRUE(engine.validateSampleIdUniqueness("test-002"));
}

TEST_F(EligibilityPolicyEngineTest, UpdatePolicy) {
    auto policy1 = createTestPolicy();
    EligibilityPolicyEngine engine(policy1);
    
    EligibilityPolicy policy2;
    policy2.min_quality_score = 0.7; // Stricter
    
    engine.updatePolicy(policy2);
    
    EXPECT_EQ(engine.getCurrentPolicy().min_quality_score, 0.7);
}

TEST_F(EligibilityPolicyEngineTest, GetAuditLog) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    auto sample1 = createTestSample("test-001", 0.8, 0.5);
    auto sample2 = createTestSample("test-002", 0.3, 0.5);
    
    engine.evaluateSample(sample1);
    engine.evaluateSample(sample2);
    
    auto log = engine.getAuditLog();
    EXPECT_GE(log.size(), 2);
}

TEST_F(EligibilityPolicyEngineTest, GetStatistics) {
    auto policy = createTestPolicy();
    EligibilityPolicyEngine engine(policy);
    
    auto sample1 = createTestSample("test-001", 0.8, 0.5); // Pass
    auto sample2 = createTestSample("test-002", 0.3, 0.5); // Fail quality
    auto sample3 = createTestSample("test-003", 0.8, 0.98); // Fail difficulty
    
    engine.evaluateSample(sample1);
    engine.evaluateSample(sample2);
    engine.evaluateSample(sample3);
    
    auto stats = engine.getEligibilityStatistics();
    EXPECT_GT(stats["quality_score_too_low"], 0);
    EXPECT_GT(stats["difficulty_score_too_high"], 0);
}

// ============================================================================
// DatasetSplitManager Tests
// ============================================================================

class DatasetSplitManagerTest : public ::testing::Test {
protected:
    SplitConfig createTestConfig() {
        SplitConfig config;
        config.train_ratio = 0.7;
        config.validation_ratio = 0.15;
        config.test_ratio = 0.15;
        config.random_seed = 42; // Fixed seed for reproducibility
        return config;
    }

    std::vector<DataSample> createTestSamples(size_t count) {
        std::vector<DataSample> samples;
        for (size_t i = 0; i < count; ++i) {
            samples.emplace_back(
                "sample-" + std::to_string(i),
                "Content for sample " + std::to_string(i)
            );
            samples.back().quality_score = 0.8;
            samples.back().difficulty_score = 0.5;
        }
        return samples;
    }
};

TEST_F(DatasetSplitManagerTest, SplitConfigValidate) {
    SplitConfig config;
    config.train_ratio = 0.7;
    config.validation_ratio = 0.15;
    config.test_ratio = 0.15;
    
    EXPECT_TRUE(config.validate());
}

TEST_F(DatasetSplitManagerTest, SplitConfigValidateFail) {
    SplitConfig config;
    config.train_ratio = 0.7;
    config.validation_ratio = 0.15;
    config.test_ratio = 0.2; // Sum > 1.0
    
    EXPECT_FALSE(config.validate());
}

TEST_F(DatasetSplitManagerTest, GenerateSplits) {
    auto config = createTestConfig();
    DatasetSplitManager manager(config);
    auto samples = createTestSamples(100);
    
    auto result = manager.generateSplits(samples);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.assignments.size(), 100);
}

TEST_F(DatasetSplitManagerTest, GenerateSplitsCorrectRatios) {
    auto config = createTestConfig();
    DatasetSplitManager manager(config);
    auto samples = createTestSamples(1000);
    
    auto result = manager.generateSplits(samples);
    
    EXPECT_TRUE(result.success);
    
    auto stats = manager.getSplitStatistics(result);
    EXPECT_NEAR(stats["train"] / 1000.0, 0.7, 0.02);
    EXPECT_NEAR(stats["validation"] / 1000.0, 0.15, 0.02);
    EXPECT_NEAR(stats["test"] / 1000.0, 0.15, 0.02);
}

TEST_F(DatasetSplitManagerTest, VerifySplitIntegrity) {
    auto config = createTestConfig();
    DatasetSplitManager manager(config);
    auto samples = createTestSamples(100);
    
    auto result = manager.generateSplits(samples);
    
    EXPECT_TRUE(manager.verifySplitIntegrity(result));
}

TEST_F(DatasetSplitManagerTest, GetSamplesInSplit) {
    auto config = createTestConfig();
    DatasetSplitManager manager(config);
    auto samples = createTestSamples(100);
    
    auto result = manager.generateSplits(samples);
    
    auto train_samples = manager.getSamplesInSplit(result, "train");
    EXPECT_EQ(train_samples.size(), 70);
    
    auto val_samples = manager.getSamplesInSplit(result, "validation");
    EXPECT_EQ(val_samples.size(), 15);
    
    auto test_samples = manager.getSamplesInSplit(result, "test");
    EXPECT_EQ(test_samples.size(), 15);
}

TEST_F(DatasetSplitManagerTest, DeterministicSeeding) {
    auto config1 = createTestConfig();
    config1.random_seed = 42;
    
    auto config2 = createTestConfig();
    config2.random_seed = 42;
    
    DatasetSplitManager manager1(config1);
    DatasetSplitManager manager2(config2);
    
    auto samples = createTestSamples(100);
    
    auto result1 = manager1.generateSplits(samples);
    auto result2 = manager2.generateSplits(samples);
    
    EXPECT_EQ(result1.checksum, result2.checksum);
}

TEST_F(DatasetSplitManagerTest, DifferentSeeding) {
    auto config1 = createTestConfig();
    config1.random_seed = 42;
    
    auto config2 = createTestConfig();
    config2.random_seed = 123;
    
    DatasetSplitManager manager1(config1);
    DatasetSplitManager manager2(config2);
    
    auto samples = createTestSamples(100);
    
    auto result1 = manager1.generateSplits(samples);
    auto result2 = manager2.generateSplits(samples);
    
    EXPECT_NE(result1.checksum, result2.checksum);
}

TEST_F(DatasetSplitManagerTest, GenerateSplitsFromIds) {
    auto config = createTestConfig();
    DatasetSplitManager manager(config);
    
    std::vector<std::string> sample_ids;
    for (int i = 0; i < 100; ++i) {
        sample_ids.push_back("sample-" + std::to_string(i));
    }
    
    auto result = manager.generateSplitsFromIds(sample_ids);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.assignments.size(), 100);
}

// ============================================================================
// Integration Tests
// ============================================================================

class DatasetSnapshotIntegrationTest : public ::testing::Test {
protected:
    DataSample createTestSample(std::string id,
                               double quality = 0.8,
                               double difficulty = 0.5,
                               std::string domain = "general") {
        DataSample sample(id, "Test content for " + id);
        sample.language = "en";
        sample.quality_score = quality;
        sample.difficulty_score = difficulty;
        sample.domain = domain;
        return sample;
    }
};

TEST_F(DatasetSnapshotIntegrationTest, FullPipeline) {
    // 1. Create eligibility policy
    EligibilityPolicy policy;
    policy.min_quality_score = 0.5;
    policy.max_difficulty_score = 0.95;
    
    // 2. Evaluate samples with policy engine
    EligibilityPolicyEngine engine(policy);
    std::vector<DataSample> eligible_samples;
    
    for (int i = 0; i < 100; ++i) {
        auto sample = createTestSample("sample-" + std::to_string(i));
        auto result = engine.evaluateSample(sample);
        if (result.is_eligible) {
            eligible_samples.push_back(sample);
        }
    }
    
    EXPECT_GT(eligible_samples.size(), 0);
    
    // 3. Generate splits
    SplitConfig split_config;
    split_config.train_ratio = 0.7;
    split_config.validation_ratio = 0.15;
    split_config.test_ratio = 0.15;
    split_config.random_seed = 42;
    
    DatasetSplitManager split_manager(split_config);
    auto split_result = split_manager.generateSplits(eligible_samples);
    
    EXPECT_TRUE(split_result.success);
    EXPECT_TRUE(split_manager.verifySplitIntegrity(split_result));
    
    // 4. Create manifest
    DatasetSnapshotManifest manifest;
    manifest.snapshot_id = "snapshot-001";
    manifest.name = "Test Snapshot";
    manifest.eligibility_policy = policy;
    manifest.total_samples = eligible_samples.size();
    
    auto train_count = split_manager.getSamplesInSplit(split_result, "train").size();
    auto val_count = split_manager.getSamplesInSplit(split_result, "validation").size();
    auto test_count = split_manager.getSamplesInSplit(split_result, "test").size();
    
    manifest.train_samples = train_count;
    manifest.validation_samples = val_count;
    manifest.test_samples = test_count;
    
    manifest.updateChecksum();
    
    EXPECT_TRUE(manifest.verifyIntegrity());
    
    // 5. Verify statistics
    auto split_stats = split_manager.getSplitStatistics(split_result);
    EXPECT_EQ(split_stats["train"], manifest.train_samples);
}

} // namespace
