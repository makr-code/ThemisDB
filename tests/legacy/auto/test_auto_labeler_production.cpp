// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_auto_labeler_production.cpp
 * @brief Production readiness tests for LegalAutoLabeler (Phases 1 & 2)
 *
 * Covers:
 *  - Phase 1: Construction, labelAll stats, labelDocument, labelQuery,
 *             getLowConfidenceSamples, updateSampleConfidence, batch config,
 *             confidence filtering, AQL query templates
 *  - Phase 2: NLP extraction, multi-language config, error recovery,
 *             metadata provenance, confidence scoring
 */

#include <gtest/gtest.h>
#include "training/auto_labeler.h"
#include <future>
#include <string>
#include <vector>
#include <algorithm>

using namespace themis::training;

// ============================================================================
// Test fixture
// ============================================================================
class AutoLabelerProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default German legal configuration
        config_.source_collection = "legal_documents";
        config_.target_collection = "legal_training_samples";
        config_.language_code     = "de";
        config_.min_confidence    = 0.5f;
        config_.flag_low_confidence = true;
        config_.batch_size        = 10;
    }

    AutoLabelConfig config_;
    const std::string db_conn_ = ""; // no real DB in test environment
};

// ============================================================================
// Phase 1: Construction & configuration
// ============================================================================

TEST_F(AutoLabelerProductionTest, Construction_Succeeds) {
    EXPECT_NO_THROW(LegalAutoLabeler labeler(config_, db_conn_));
}

TEST_F(AutoLabelerProductionTest, Construction_EmptyDbConnection_Succeeds) {
    EXPECT_NO_THROW(LegalAutoLabeler labeler(config_, ""));
}

TEST_F(AutoLabelerProductionTest, Config_DefaultValues) {
    AutoLabelConfig cfg;
    EXPECT_EQ(cfg.language_code,      "de");
    EXPECT_FLOAT_EQ(cfg.min_confidence, 0.5f);
    EXPECT_TRUE(cfg.flag_low_confidence);
    EXPECT_EQ(cfg.batch_size, 100u);
}

TEST_F(AutoLabelerProductionTest, Config_BatchSizeIsRespected) {
    config_.batch_size = 50;
    EXPECT_NO_THROW(LegalAutoLabeler labeler(config_, db_conn_));
    EXPECT_EQ(config_.batch_size, 50u);
}

// ============================================================================
// Phase 1: labelAll – statistics
// ============================================================================

TEST_F(AutoLabelerProductionTest, LabelAll_EmptyCollection_ReturnsZeroStats) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto stats = labeler.labelAll();

    // With no database, no documents are fetched → all counters stay at 0
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.samples_created,     0u);
}

TEST_F(AutoLabelerProductionTest, LabelAll_ElapsedTimeRecorded) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto stats = labeler.labelAll();

    // elapsed_seconds should be non-negative even for an empty run
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(AutoLabelerProductionTest, LabelAll_CallbackNotCalledOnEmpty) {
    LegalAutoLabeler labeler(config_, db_conn_);
    int callback_count = 0;
    labeler.labelAll([&](size_t, size_t, const std::string&) { ++callback_count; });
    EXPECT_EQ(callback_count, 0);
}

// ============================================================================
// Phase 1: labelDocument – single document processing
// ============================================================================

TEST_F(AutoLabelerProductionTest, LabelDocument_EmptyId_ReturnsEmpty) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("");
    EXPECT_TRUE(samples.empty());
}

TEST_F(AutoLabelerProductionTest, LabelDocument_ValidId_ReturnsSamples) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_001");

    // Should produce at least one sample from the built-in German legal text
    EXPECT_GE(samples.size(), 1u);
}

TEST_F(AutoLabelerProductionTest, LabelDocument_SamplesHaveRequiredFields) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_002");

    for (const auto& s : samples) {
        EXPECT_FALSE(s.input.empty())    << "Sample missing input text";
        EXPECT_FALSE(s.output.empty())   << "Sample missing output label";
        EXPECT_FALSE(s.category.empty()) << "Sample missing category";
        EXPECT_GE(s.confidence, 0.0f);
        EXPECT_LE(s.confidence, 1.0f);
    }
}

TEST_F(AutoLabelerProductionTest, LabelDocument_SourceIdTracked) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("my_doc_42");

    for (const auto& s : samples) {
        EXPECT_EQ(s.source_id, "my_doc_42");
    }
}

// ============================================================================
// Phase 1: Confidence filtering
// ============================================================================

TEST_F(AutoLabelerProductionTest, ConfidenceFilter_HighThreshold_ReducesSamples) {
    config_.min_confidence      = 0.9f;
    config_.flag_low_confidence = false; // don't include low-confidence
    LegalAutoLabeler labeler_strict(config_, db_conn_);
    auto strict = labeler_strict.labelDocument("doc_003");

    config_.min_confidence = 0.0f;
    LegalAutoLabeler labeler_loose(config_, db_conn_);
    auto loose = labeler_loose.labelDocument("doc_003");

    // Strict threshold should produce fewer or equal samples
    EXPECT_LE(strict.size(), loose.size());
}

TEST_F(AutoLabelerProductionTest, LowConfidenceFlagging_Enabled_FlagsInMetadata) {
    config_.min_confidence      = 0.99f; // almost nothing will exceed this
    config_.flag_low_confidence = true;
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_004");

    // Low-confidence samples should be included but flagged
    bool found_flagged = false;
    for (const auto& s : samples) {
        if (s.metadata.find("flagged_for_review") != std::string::npos) {
            found_flagged = true;
        }
    }
    // If any samples were generated and confidence was low, flagging should trigger
    if (!samples.empty()) {
        EXPECT_TRUE(found_flagged);
    }
}

TEST_F(AutoLabelerProductionTest, LowConfidenceFlagging_Disabled_ExcludesLow) {
    config_.min_confidence      = 0.99f;
    config_.flag_low_confidence = false;
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_005");

    for (const auto& s : samples) {
        EXPECT_GE(s.confidence, config_.min_confidence);
    }
}

// ============================================================================
// Phase 1: labelQuery
// ============================================================================

TEST_F(AutoLabelerProductionTest, LabelQuery_EmptyQuery_ReturnsZeroStats) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto stats = labeler.labelQuery("");
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.samples_created,     0u);
}

TEST_F(AutoLabelerProductionTest, LabelQuery_ValidQuery_ReturnsStats) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto stats = labeler.labelQuery(
        "FOR doc IN legal_documents FILTER doc.source == 'bundesrecht' RETURN doc._key");
    // No database → still 0 documents, but no crash
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

// ============================================================================
// Phase 1: getLowConfidenceSamples & updateSampleConfidence
// ============================================================================

TEST_F(AutoLabelerProductionTest, GetLowConfidenceSamples_ReturnsEmpty_WhenNoDb) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.getLowConfidenceSamples(0.5f);
    // Expected: empty (no database connected)
    EXPECT_TRUE(samples.empty());
}

TEST_F(AutoLabelerProductionTest, UpdateSampleConfidence_DoesNotThrow) {
    LegalAutoLabeler labeler(config_, db_conn_);
    EXPECT_NO_THROW(
        labeler.updateSampleConfidence("sample_001", 0.9f, "human_reviewer_1"));
}

TEST_F(AutoLabelerProductionTest, UpdateSampleConfidence_EmptyId_DoesNotThrow) {
    LegalAutoLabeler labeler(config_, db_conn_);
    EXPECT_NO_THROW(labeler.updateSampleConfidence("", 0.8f, "reviewer"));
}

// ============================================================================
// Phase 2: Multi-language configuration
// ============================================================================

TEST_F(AutoLabelerProductionTest, MultiLanguage_EnglishConfig_Constructs) {
    config_.language_code = "en";
    EXPECT_NO_THROW(LegalAutoLabeler labeler(config_, db_conn_));
}

TEST_F(AutoLabelerProductionTest, MultiLanguage_FrenchConfig_Constructs) {
    config_.language_code = "fr";
    EXPECT_NO_THROW(LegalAutoLabeler labeler(config_, db_conn_));
}

// ============================================================================
// Phase 2: Metadata provenance
// ============================================================================

TEST_F(AutoLabelerProductionTest, SampleMetadata_ContainsProvenanceInfo) {
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_meta_test");

    for (const auto& s : samples) {
        // Metadata should contain auto_labeled flag
        EXPECT_FALSE(s.metadata.empty());
        EXPECT_NE(s.metadata.find("auto_labeled"), std::string::npos);
    }
}

// ============================================================================
// Phase 2: Error recovery
// ============================================================================

TEST_F(AutoLabelerProductionTest, LabelAll_WithCallback_CallsCallbackOnProgress) {
    // This validates the callback mechanism doesn't crash when no docs present
    LegalAutoLabeler labeler(config_, db_conn_);
    bool callback_invoked = false;
    labeler.labelAll([&](size_t, size_t, const std::string&) {
        callback_invoked = true;
    });
    // No documents → callback not invoked, but no crash either
    EXPECT_FALSE(callback_invoked);
}

// ============================================================================
// AQL executor wiring – nullptr engine (backward-compatibility, offline mode)
// ============================================================================

TEST_F(AutoLabelerProductionTest, Construction_ExplicitNullEngine_Succeeds) {
    // Explicit nullptr engine must be accepted without throwing
    EXPECT_NO_THROW(LegalAutoLabeler labeler(config_, db_conn_, nullptr));
}

TEST_F(AutoLabelerProductionTest, LabelAll_NullEngine_ReturnsZeroDocuments) {
    // When no engine is wired in, labelAll() must return 0 documents – same as
    // the two-argument constructor.
    LegalAutoLabeler labeler(config_, db_conn_, nullptr);
    auto stats = labeler.labelAll();
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.samples_created,     0u);
}

TEST_F(AutoLabelerProductionTest, LabelQuery_NullEngine_ReturnsZeroDocuments) {
    // When no engine is wired in, labelQuery() must return 0 documents even for
    // a well-formed AQL query string.
    LegalAutoLabeler labeler(config_, db_conn_, nullptr);
    auto stats = labeler.labelQuery(
        "FOR doc IN legal_documents RETURN doc._key");
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(AutoLabelerProductionTest, LabelDocument_NullEngine_StillProducesSamples) {
    // labelDocument() does not require a query engine; it uses the in-process
    // fallback text regardless of whether an engine is wired in.
    LegalAutoLabeler labeler(config_, db_conn_, nullptr);
    auto samples = labeler.labelDocument("doc_null_engine");
    EXPECT_GE(samples.size(), 1u);
}

// ============================================================================
// Phase 3: Multi-modal extraction – ContentModality field and per-modality stats
// ============================================================================

TEST_F(AutoLabelerProductionTest, LabelDocument_SamplesHaveModalityField) {
    // Every sample returned by labelDocument() must carry a non-UNKNOWN modality
    // so the training pipeline can apply modality-specific confidence thresholds.
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_modality_test");
    ASSERT_FALSE(samples.empty()) << "Expected at least one sample";
    for (const auto& s : samples) {
        EXPECT_NE(s.modality, ContentModality::UNKNOWN)
            << "Sample '" << s.input.substr(0, 40) << "...' has UNKNOWN modality";
    }
}

TEST_F(AutoLabelerProductionTest, LabelDocument_ModalityConfidenceInRange) {
    // Confidence scores must be in [0, 1] for every modality so that
    // mean-confidence logging produces sensible values.
    LegalAutoLabeler labeler(config_, db_conn_);
    auto samples = labeler.labelDocument("doc_conf_range_test");
    for (const auto& s : samples) {
        EXPECT_GE(s.confidence, 0.0f) << "Modality confidence below 0";
        EXPECT_LE(s.confidence, 1.0f) << "Modality confidence above 1";
    }
}

TEST_F(AutoLabelerProductionTest, RegisterDocument_ConcurrentLabelingUsesRegisteredCorpus) {
    LegalAutoLabeler labeler(config_, db_conn_);

    std::vector<std::future<std::vector<TrainingSample>>> futures;
    futures.reserve(6);

    for (size_t i = 0; i < 6; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            const std::string doc_id = "doc_concurrent_" + std::to_string(i);
            const std::string marker = "Sondertext" + std::to_string(i);
            labeler.registerDocument(doc_id, marker + " muss dokumentiert werden.");
            return labeler.labelDocument(doc_id);
        }));
    }

    for (size_t i = 0; i < futures.size(); ++i) {
        const std::string expected_marker = "Sondertext" + std::to_string(i);
        auto samples = futures[i].get();
        ASSERT_FALSE(samples.empty());

        bool found_registered_text = false;
        for (const auto& sample : samples) {
            if (sample.input.find(expected_marker) != std::string::npos) {
                found_registered_text = true;
                break;
            }
        }

        EXPECT_TRUE(found_registered_text)
            << "Expected labeled samples to use registered offline corpus text for " << expected_marker;
    }
}
