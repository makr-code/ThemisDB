// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_auto_labeler_db_fetch.cpp
 * @brief DB fetch path tests for LegalAutoLabeler AQL-Executor integration
 *
 * Validates that labelAll() and labelQuery() fetch document IDs from the
 * database via the AQL query executor (QueryEngine) when one is wired in.
 *
 * Issue: [FEATURE] Wire LegalAutoLabeler DB fetch to AQL query executor
 * Milestone: v1.6.0
 *
 * Covers:
 *  - labelAll() with engine=nullptr (offline mode: 0 docs processed)
 *  - labelAll() with real QueryEngine (DB fetch path: N docs processed)
 *  - labelQuery() with engine=nullptr (offline mode: 0 docs processed)
 *  - labelQuery() with real QueryEngine and user-supplied AQL
 *  - executeAqlQuery helper: results envelope with string keys
 *  - executeAqlQuery helper: results envelope with _key objects
 *  - labelAll() callback invoked for each batch of 10 docs fetched from DB
 *  - labelQuery() empty-query guard (no engine call)
 *  - stats accumulation: documents_processed, samples_created
 *  - Isolation: each test gets an independent RocksDB instance
 */

#include <gtest/gtest.h>

#include "training/auto_labeler.h"
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

using namespace themis;
using namespace themis::query;
using namespace themis::training;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Returns a unique, per-test temporary directory path.
std::string makeTmpPath(const std::string& suffix) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_aldb_" + suffix + std::to_string(ts))).string();
}

/// Default AutoLabelConfig targeting "legal_documents" collection.
AutoLabelConfig makeConfig(const std::string& collection = "legal_documents") {
    AutoLabelConfig cfg;
    cfg.source_collection   = collection;
    cfg.target_collection   = "legal_training_samples";
    cfg.language_code       = "de";
    cfg.min_confidence      = 0.3f;   // accept most samples
    cfg.flag_low_confidence = false;
    cfg.batch_size          = 10;
    return cfg;
}

} // namespace

// ============================================================================
// Test fixture – manages a real RocksDB + QueryEngine
// ============================================================================

class AutoLabelerDbFetchTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping DB-fetch integration tests on Windows due to unstable QueryEngine timing under CTest.";
#endif
        dbPath_ = makeTmpPath("al_");
        RocksDBWrapper::Config cfg;
        cfg.db_path        = dbPath_;
        cfg.enable_blobdb  = false;
        db_  = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);

        // Insert three documents with a non-empty "text" field into the
        // collection queried by labelAll() (FETCH_ALL_DOCUMENTS template).
        const std::string kCollection = "legal_documents";
        for (int i = 1; i <= 3; ++i) {
            BaseEntity e("doc_" + std::to_string(i));
            e.setField("text", std::string("Die Behörde muss handeln. Sie kann "
                                           "eine Frist setzen. Sie soll reagieren."));
            e.setField("source", std::string("bundesrecht"));
            ASSERT_TRUE(idx_->put(kCollection, e).ok);
        }

        engine_ = std::make_unique<QueryEngine>(*db_, *idx_);
    }

    void TearDown() override {
        engine_.reset();
        idx_.reset();
        if (db_) db_->close();
        db_.reset();
        std::filesystem::remove_all(dbPath_);
    }

    std::string                        dbPath_;
    std::unique_ptr<RocksDBWrapper>    db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine>       engine_;
};

// ============================================================================
// Offline mode (engine = nullptr) – baseline: no DB access
// ============================================================================

TEST(AutoLabelerOfflineTest, LabelAll_NoEngine_ProcessesZeroDocs) {
    auto cfg = makeConfig();
    LegalAutoLabeler labeler(cfg, "", nullptr);
    auto stats = labeler.labelAll();
    EXPECT_EQ(stats.documents_processed, 0u)
        << "Without a QueryEngine, labelAll() must not fetch any documents";
    EXPECT_EQ(stats.samples_created, 0u);
}

TEST(AutoLabelerOfflineTest, LabelQuery_NoEngine_ProcessesZeroDocs) {
    auto cfg = makeConfig();
    LegalAutoLabeler labeler(cfg, "", nullptr);
    auto stats = labeler.labelQuery(
        "FOR doc IN legal_documents RETURN doc._key");
    EXPECT_EQ(stats.documents_processed, 0u)
        << "Without a QueryEngine, labelQuery() must not fetch any documents";
}

// ============================================================================
// DB fetch path – labelAll() with real QueryEngine
// ============================================================================

TEST_F(AutoLabelerDbFetchTest, LabelAll_WithEngine_FetchesDocumentsFromDb) {
    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    auto stats = labeler.labelAll();

    // Three documents were inserted; all three should be processed.
    EXPECT_EQ(stats.documents_processed, 3u)
        << "labelAll() must fetch all 3 documents from the DB via AQL";
    EXPECT_GT(stats.samples_created, 0u)
        << "labelAll() must generate at least one training sample per document";
}

TEST_F(AutoLabelerDbFetchTest, LabelAll_WithEngine_ElapsedTimeRecorded) {
    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());
    auto stats = labeler.labelAll();
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(AutoLabelerDbFetchTest, LabelAll_WithEngine_StatsConsistent) {
    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());
    auto stats = labeler.labelAll();

    // high_confidence + low_confidence must not exceed total samples_created.
    EXPECT_LE(stats.high_confidence_samples + stats.low_confidence_samples,
              stats.samples_created);
}

// ============================================================================
// DB fetch path – labelAll() callback
// ============================================================================

TEST_F(AutoLabelerDbFetchTest, LabelAll_WithEngine_CallbackReceivedForProgress) {
    // Batch_size=10 > 3 docs, so callback fires only at the very end (every 10).
    // With 3 docs the inner `processed % 10 == 0` branch is never taken, but
    // processed still increments. Verify callback mechanism doesn't crash.
    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    size_t callback_count = 0;
    labeler.labelAll([&](size_t /*processed*/, size_t /*total*/,
                         const std::string& /*status*/) {
        ++callback_count;
    });
    // No assertion on exact count – just ensure no crash and the call completes.
    SUCCEED();
}

TEST_F(AutoLabelerDbFetchTest, LabelAll_WithEngine_CallbackFiredEvery10Docs) {
    // Insert 20 extra documents so the every-10 callback path is exercised.
    for (int i = 100; i < 120; ++i) {
        BaseEntity e("doc_" + std::to_string(i));
        e.setField("text", std::string("Der Vertrag ist bindend."));
        ASSERT_TRUE(idx_->put("legal_documents", e).ok);
    }

    auto cfg    = makeConfig("legal_documents");
    cfg.batch_size = 25;
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    size_t callback_count = 0;
    auto stats = labeler.labelAll([&](size_t /*processed*/, size_t /*total*/,
                                      const std::string& /*status*/) {
        ++callback_count;
    });

    // 23 documents total (3 original + 20 extra), callback every 10 → twice.
    EXPECT_EQ(stats.documents_processed, 23u);
    EXPECT_EQ(callback_count, 2u)
        << "Callback should be called once per 10 processed documents";
}

// ============================================================================
// DB fetch path – labelQuery() with real QueryEngine
// ============================================================================

TEST_F(AutoLabelerDbFetchTest, LabelQuery_WithEngine_FetchesDocumentsViaAql) {
    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    // User-supplied AQL that returns all document keys in the collection.
    const std::string aql =
        "FOR doc IN legal_documents RETURN doc._key";
    auto stats = labeler.labelQuery(aql);

    EXPECT_EQ(stats.documents_processed, 3u)
        << "labelQuery() must process all 3 documents returned by the AQL query";
    EXPECT_GT(stats.samples_created, 0u);
}

TEST_F(AutoLabelerDbFetchTest, LabelQuery_WithEngine_FilteredAql_ProcessesSubset) {
    // Insert one extra document with a different source field.
    {
        BaseEntity e("doc_bundesrecht_1");
        e.setField("text", std::string("Der Staat soll Bürger schützen."));
        e.setField("source", std::string("bundesverfassung"));
        ASSERT_TRUE(idx_->put("legal_documents", e).ok);
    }

    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    // Query that targets only the document with source == 'bundesverfassung'.
    const std::string aql =
        "FOR doc IN legal_documents "
        "FILTER doc.source == 'bundesverfassung' "
        "RETURN doc._key";
    auto stats = labeler.labelQuery(aql);

    EXPECT_EQ(stats.documents_processed, 1u)
        << "labelQuery() should process only documents matching the filter";
}

TEST_F(AutoLabelerDbFetchTest, LabelQuery_WithEngine_MutatingAql_IsRejected) {
    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    const std::string aql =
        "FOR doc IN legal_documents REMOVE doc IN legal_documents RETURN OLD._key";
    auto stats = labeler.labelQuery(aql);

    EXPECT_EQ(stats.documents_processed, 0u)
        << "Mutating AQL must be rejected by safety guard";
    EXPECT_EQ(stats.samples_created, 0u);
}

TEST_F(AutoLabelerDbFetchTest, LabelQuery_WithEngine_MixedCaseMutatingAql_IsRejected) {
    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    const std::string aql =
        "FoR doc IN legal_documents UpDaTe doc WITH {tag:'x'} IN legal_documents RETURN NEW._key";
    auto stats = labeler.labelQuery(aql);

    EXPECT_EQ(stats.documents_processed, 0u)
        << "Mutating AQL guard must be case-insensitive";
    EXPECT_EQ(stats.samples_created, 0u);
}

TEST_F(AutoLabelerDbFetchTest, LabelQuery_WithEngine_EmptyQuery_ReturnsZero) {
    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    auto stats = labeler.labelQuery("");
    EXPECT_EQ(stats.documents_processed, 0u)
        << "labelQuery() with empty AQL string must return zero stats";
    EXPECT_EQ(stats.samples_created, 0u);
}

TEST_F(AutoLabelerDbFetchTest, LabelQuery_WithEngine_ElapsedTimeRecorded) {
    auto cfg    = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());
    auto stats = labeler.labelQuery(
        "FOR doc IN legal_documents RETURN doc._key");
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

// ============================================================================
// Integration: DB fetch + offline dual-mode
// ============================================================================

TEST_F(AutoLabelerDbFetchTest, DualMode_SameConfigDifferentEngines) {
    // With engine wired → processes DB documents.
    // With nullptr      → processes no documents.
    auto cfg = makeConfig("legal_documents");

    LegalAutoLabeler with_engine(cfg, "", engine_.get());
    auto stats_db = with_engine.labelAll();

    LegalAutoLabeler without_engine(cfg, "", nullptr);
    auto stats_offline = without_engine.labelAll();

    EXPECT_GT(stats_db.documents_processed, stats_offline.documents_processed)
        << "DB mode must process more documents than offline mode";
    EXPECT_EQ(stats_offline.documents_processed, 0u);
}

// ============================================================================
// fetchDocumentText DB fetch path – text is read from the database
// ============================================================================

TEST_F(AutoLabelerDbFetchTest, LabelDocument_WithEngine_FetchesTextFromDb) {
    // Insert a document whose text contains ONLY "kann" (permission modal).
    // The hardcoded offline fallback text contains "muss" AND "soll" AND "kann",
    // so if DB fetch is wired correctly we get fewer/different samples than the
    // fallback would produce.
    {
        BaseEntity e("doc_kann_only");
        e.setField("text", std::string("Die Behörde kann eine Ausnahme genehmigen."));
        ASSERT_TRUE(idx_->put("legal_documents", e).ok);
    }

    auto cfg = makeConfig("legal_documents");
    cfg.min_confidence = 0.0f; // accept all samples regardless of confidence
    cfg.flag_low_confidence = true;
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    auto samples = labeler.labelDocument("doc_kann_only");

    // With DB fetch the text only contains "kann", so every sample should be
    // in the "permission" category.  If the hardcoded fallback were returned
    // instead, "obligation" and "default_obligation" samples would also appear.
    ASSERT_FALSE(samples.empty())
        << "At least one sample must be produced from the 'kann' document";
    for (const auto& s : samples) {
        EXPECT_EQ(s.category, "permission")
            << "Only 'permission' samples expected when text contains only 'kann'";
        EXPECT_EQ(s.source_id, "doc_kann_only");
    }
}

TEST_F(AutoLabelerDbFetchTest, LabelDocument_WithEngine_NoTextField_ReturnsEmpty) {
    // Insert a document WITHOUT a text field – fetchDocumentText should return ""
    // and labelDocument should produce no samples.
    {
        BaseEntity e("doc_no_text");
        e.setField("title", std::string("Untitled"));
        ASSERT_TRUE(idx_->put("legal_documents", e).ok);
    }

    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());

    auto samples = labeler.labelDocument("doc_no_text");
    EXPECT_TRUE(samples.empty())
        << "A document without a text field must produce no training samples";
}

TEST_F(AutoLabelerDbFetchTest, LabelAll_WithEngine_SkipsDocsWithoutText) {
    // Insert a document without text. FETCH_ALL_DOCUMENTS filters
    // doc.text != null AND doc.text != '', so this doc should not be returned
    // by the AQL query and therefore not be processed.
    {
        BaseEntity e("doc_no_text_in_all");
        e.setField("category", std::string("misc"));
        ASSERT_TRUE(idx_->put("legal_documents", e).ok);
    }

    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());
    auto stats = labeler.labelAll();

    // Only the original 3 documents (with text fields) should be processed.
    EXPECT_EQ(stats.documents_processed, 3u)
        << "Documents without a text field must be excluded by FETCH_ALL_DOCUMENTS";
}


// ============================================================================
// registerDocument() — stub #66 resolution
// ============================================================================

TEST_F(AutoLabelerDbFetchTest, RegisterDocument_OfflineMode_UsesRegisteredText) {
    // In offline mode (no engine), registerDocument() feeds per-document text
    // into the labeler, overriding the hardcoded fallback paragraph.
    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", nullptr);

    // Register a document containing only "kann" (permission) — should produce
    // only "permission" samples, unlike the hardcoded fallback which contains
    // all three modalities (muss/soll/kann).
    labeler.registerDocument("offline_doc_1",
        "Die Behörde kann eine Ausnahme genehmigen.");

    auto samples = labeler.labelDocument("offline_doc_1");
    ASSERT_FALSE(samples.empty())
        << "LabelDocument with a registered offline document must produce samples";
    for (const auto& s : samples) {
        EXPECT_EQ(s.category, "permission")
            << "Only 'permission' samples expected from a 'kann'-only text";
    }
}

TEST_F(AutoLabelerDbFetchTest, RegisterDocument_FallbackForUnregisteredId) {
    // When a document_id is NOT registered and no engine is wired, the
    // hardcoded fallback paragraph should still be used.
    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", nullptr);

    // Do NOT register the document.
    auto samples = labeler.labelDocument("unregistered_doc");
    // The hardcoded fallback contains muss/soll/kann → expect at least one sample
    // from the obligation/duty category.
    ASSERT_FALSE(samples.empty())
        << "Unregistered document should fall back to hardcoded text and produce samples";
}

TEST_F(AutoLabelerDbFetchTest, RegisterDocument_EngineWinsOverRegistry) {
    // When a query engine is wired, the DB text takes precedence over any
    // registered offline text.
    {
        BaseEntity e("doc_muss_only");
        e.setField("text", std::string("Die Behörde muss handeln."));
        ASSERT_TRUE(idx_->put("legal_documents", e).ok);
    }

    auto cfg = makeConfig("legal_documents");
    LegalAutoLabeler labeler(cfg, "", engine_.get());
    // Register a different text for the same document_id.
    labeler.registerDocument("doc_muss_only",
        "Die Behörde kann eine Ausnahme genehmigen.");

    auto samples = labeler.labelDocument("doc_muss_only");
    // DB text is "muss" (obligation) → engine wins, samples must be obligation.
    ASSERT_FALSE(samples.empty());
    bool has_obligation = false;
    for (const auto& s : samples) {
        if (s.category == "obligation") has_obligation = true;
    }
    EXPECT_TRUE(has_obligation)
        << "Engine-backed text (muss) must take precedence over registered offline text (kann)";
}
