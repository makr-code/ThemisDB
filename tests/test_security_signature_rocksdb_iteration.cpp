// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Focused tests for SecuritySignatureManager: RocksDB Iteration (v1.8.0, Issue #206)
//
// Acceptance criteria covered:
//   AC-1  RocksDBWrapper::iterateRange(start_key, end_key, callback) uses a
//          rocksdb::Iterator under the hood and honours start/end bounds.
//   AC-2  SecuritySignatureManager::verifyAll() scans all document keys via
//          iterateRange and reports per-signature verification outcomes.

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "storage/security_signature_manager.h"
#include "storage/security_signature.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::storage;

// ---------------------------------------------------------------------------
// Helper: open a temporary RocksDB instance
// ---------------------------------------------------------------------------

static std::shared_ptr<RocksDBWrapper> openTempDB(const std::string& path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path    = path;
    cfg.enable_wal = true;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    if (!db->open()) return nullptr;
    return db;
}

static std::string uniqueTmpPath(const std::string& tag) {
    return (fs::temp_directory_path() /
            ("themis_ssig_" + tag + "_" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count())))
               .string();
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class SecuritySignatureRocksDBIterationTests : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = uniqueTmpPath("iter");
        fs::remove_all(db_path_);
        db_ = openTempDB(db_path_);
        ASSERT_NE(db_, nullptr) << "Failed to open test RocksDB at " << db_path_;
        mgr_ = std::make_unique<SecuritySignatureManager>(db_);
    }

    void TearDown() override {
        mgr_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string                              db_path_;
    std::shared_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<SecuritySignatureManager> mgr_;
};

// ---------------------------------------------------------------------------
// AC-1: RocksDBWrapper::iterateRange
// ---------------------------------------------------------------------------

TEST_F(SecuritySignatureRocksDBIterationTests, IterateRange_ReturnsKeysWithinBounds) {
    // Put keys: a_1, a_2, b_1, b_2, c_1
    db_->put("a_1", "v1");
    db_->put("a_2", "v2");
    db_->put("b_1", "v3");
    db_->put("b_2", "v4");
    db_->put("c_1", "v5");

    // Iterate [a_, b_) — should yield a_1, a_2 only
    std::vector<std::string> collected_keys;
    db_->iterateRange("a_", "b_", [&](std::string_view key, std::string_view /*value*/) -> bool {
        collected_keys.emplace_back(key);
        return true;
    });

    ASSERT_EQ(collected_keys.size(), 2u);
    EXPECT_EQ(collected_keys[0], "a_1");
    EXPECT_EQ(collected_keys[1], "a_2");
}

TEST_F(SecuritySignatureRocksDBIterationTests, IterateRange_EndKeyIsExclusive) {
    db_->put("x_1", "v1");
    db_->put("x_2", "v2");

    // end_key == "x_2" should exclude "x_2"
    std::vector<std::string> keys;
    db_->iterateRange("x_1", "x_2", [&](std::string_view k, std::string_view) -> bool {
        keys.emplace_back(k);
        return true;
    });

    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "x_1");
}

TEST_F(SecuritySignatureRocksDBIterationTests, IterateRange_EmptyRange_NoCallback) {
    db_->put("z_1", "v1");

    int count = 0;
    db_->iterateRange("m_", "n_", [&](std::string_view, std::string_view) -> bool {
        ++count;
        return true;
    });

    EXPECT_EQ(count, 0);
}

TEST_F(SecuritySignatureRocksDBIterationTests, IterateRange_EarlyTermination) {
    db_->put("t_1", "v1");
    db_->put("t_2", "v2");
    db_->put("t_3", "v3");

    int count = 0;
    db_->iterateRange("t_", "u_", [&](std::string_view, std::string_view) -> bool {
        ++count;
        return false; // stop after first entry
    });

    EXPECT_EQ(count, 1);
}

TEST_F(SecuritySignatureRocksDBIterationTests, IterateRange_ReturnsValues) {
    db_->put("k_1", "hello");
    db_->put("k_2", "world");

    std::vector<std::string> values;
    db_->iterateRange("k_", "l_", [&](std::string_view, std::string_view v) -> bool {
        values.emplace_back(v);
        return true;
    });

    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0], "hello");
    EXPECT_EQ(values[1], "world");
}

// ---------------------------------------------------------------------------
// AC-1 (continued): listAllSignatures uses iterateRange
// ---------------------------------------------------------------------------

TEST_F(SecuritySignatureRocksDBIterationTests, ListAllSignatures_EmptyDB_ReturnsEmpty) {
    auto sigs = mgr_->listAllSignatures();
    EXPECT_TRUE(sigs.empty());
}

TEST_F(SecuritySignatureRocksDBIterationTests, ListAllSignatures_ReturnsAllStoredSignatures) {
    SecuritySignature s1;
    s1.resource_id = "file_a";
    s1.hash        = std::string(64, 'a');
    s1.algorithm   = "sha256";
    s1.created_at  = 1000;

    SecuritySignature s2;
    s2.resource_id = "file_b";
    s2.hash        = std::string(64, 'b');
    s2.algorithm   = "sha256";
    s2.created_at  = 2000;

    ASSERT_TRUE(mgr_->storeSignature(s1));
    ASSERT_TRUE(mgr_->storeSignature(s2));

    auto sigs = mgr_->listAllSignatures();
    EXPECT_EQ(sigs.size(), 2u);

    // Verify resource_ids are present (order is undefined)
    std::vector<std::string> ids;
    for (const auto& s : sigs) ids.push_back(s.resource_id);
    EXPECT_NE(std::find(ids.begin(), ids.end(), "file_a"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "file_b"), ids.end());
}

TEST_F(SecuritySignatureRocksDBIterationTests, ListAllSignatures_UnrelatedKeysNotIncluded) {
    // Store a signature under the expected prefix
    SecuritySignature sig;
    sig.resource_id = "myfile";
    sig.hash        = std::string(64, 'c');
    sig.algorithm   = "sha256";
    sig.created_at  = 0;
    ASSERT_TRUE(mgr_->storeSignature(sig));

    // Store an unrelated key directly in the DB
    db_->put("unrelated_key", "some_value");
    db_->put("other:prefix:stuff", "noise");

    auto sigs = mgr_->listAllSignatures();
    EXPECT_EQ(sigs.size(), 1u);
    EXPECT_EQ(sigs[0].resource_id, "myfile");
}

// ---------------------------------------------------------------------------
// AC-2: SecuritySignatureManager::verifyAll
// ---------------------------------------------------------------------------

TEST_F(SecuritySignatureRocksDBIterationTests, VerifyAll_EmptyDB_SucceedsWithZeroTotal) {
    auto result = mgr_->verifyAll();
    EXPECT_EQ(result.total, 0);
    EXPECT_EQ(result.verified, 0);
    EXPECT_EQ(result.failed, 0);
    EXPECT_TRUE(result.success());
}

TEST_F(SecuritySignatureRocksDBIterationTests, VerifyAll_MissingFile_ReportedAsFailed) {
    SecuritySignature sig;
    sig.resource_id = "/nonexistent/path/file.txt";
    sig.hash        = std::string(64, 'd');
    sig.algorithm   = "sha256";
    sig.created_at  = 0;
    ASSERT_TRUE(mgr_->storeSignature(sig));

    auto result = mgr_->verifyAll();
    EXPECT_EQ(result.total, 1);
    EXPECT_EQ(result.verified, 0);
    EXPECT_EQ(result.failed, 1);
    EXPECT_FALSE(result.success());
    ASSERT_EQ(result.failed_resource_ids.size(), 1u);
    EXPECT_EQ(result.failed_resource_ids[0], sig.resource_id);
}

TEST_F(SecuritySignatureRocksDBIterationTests, VerifyAll_CorrectHash_ReportedAsVerified) {
    // Create a real temporary file
    std::string tmp_file = uniqueTmpPath("verify_correct") + ".txt";
    {
        std::ofstream f(tmp_file);
        f << "ThemisDB test content";
    }

    // Compute its hash via the manager
    std::string hash = SecuritySignatureManager::computeFileHash(tmp_file);
    ASSERT_FALSE(hash.empty());

    SecuritySignature sig;
    sig.resource_id = tmp_file;
    sig.hash        = hash;
    sig.algorithm   = "sha256";
    sig.created_at  = 0;
    ASSERT_TRUE(mgr_->storeSignature(sig));

    auto result = mgr_->verifyAll();
    EXPECT_EQ(result.total, 1);
    EXPECT_EQ(result.verified, 1);
    EXPECT_EQ(result.failed, 0);
    EXPECT_TRUE(result.success());

    fs::remove(tmp_file);
}

TEST_F(SecuritySignatureRocksDBIterationTests, VerifyAll_WrongHash_ReportedAsFailed) {
    // Create a real temporary file
    std::string tmp_file = uniqueTmpPath("verify_wrong") + ".txt";
    {
        std::ofstream f(tmp_file);
        f << "original content";
    }

    SecuritySignature sig;
    sig.resource_id = tmp_file;
    sig.hash        = std::string(64, 'f'); // deliberately wrong hash
    sig.algorithm   = "sha256";
    sig.created_at  = 0;
    ASSERT_TRUE(mgr_->storeSignature(sig));

    auto result = mgr_->verifyAll();
    EXPECT_EQ(result.total, 1);
    EXPECT_EQ(result.verified, 0);
    EXPECT_EQ(result.failed, 1);
    EXPECT_FALSE(result.success());

    fs::remove(tmp_file);
}

TEST_F(SecuritySignatureRocksDBIterationTests, VerifyAll_MixedResults) {
    // File that exists with correct hash
    std::string good_file = uniqueTmpPath("good") + ".txt";
    {
        std::ofstream f(good_file);
        f << "good content";
    }
    std::string good_hash = SecuritySignatureManager::computeFileHash(good_file);

    SecuritySignature good_sig;
    good_sig.resource_id = good_file;
    good_sig.hash        = good_hash;
    good_sig.algorithm   = "sha256";
    good_sig.created_at  = 0;
    ASSERT_TRUE(mgr_->storeSignature(good_sig));

    // File that does not exist
    SecuritySignature bad_sig;
    bad_sig.resource_id = "/no/such/file.bin";
    bad_sig.hash        = std::string(64, '0');
    bad_sig.algorithm   = "sha256";
    bad_sig.created_at  = 0;
    ASSERT_TRUE(mgr_->storeSignature(bad_sig));

    auto result = mgr_->verifyAll();
    EXPECT_EQ(result.total, 2);
    EXPECT_EQ(result.verified, 1);
    EXPECT_EQ(result.failed, 1);
    EXPECT_FALSE(result.success());
    ASSERT_EQ(result.failed_resource_ids.size(), 1u);
    EXPECT_EQ(result.failed_resource_ids[0], bad_sig.resource_id);

    fs::remove(good_file);
}

// ---------------------------------------------------------------------------
// In-memory fallback path (no RocksDB)
// ---------------------------------------------------------------------------

TEST(SecuritySignatureVerifyAllFallbackTests, NullBackendFailsClosedWithoutExplicitFallback) {
    SecuritySignatureManager mgr(nullptr);
    SecuritySignature sig;
    sig.resource_id = "fb_file";
    sig.hash        = std::string(64, 'e');
    sig.algorithm   = "sha256";
    sig.created_at  = 42;

    EXPECT_FALSE(mgr.isUsingFallbackMemoryStore());
    EXPECT_FALSE(mgr.hasPersistentBackend());
    EXPECT_FALSE(mgr.storeSignature(sig));
    EXPECT_TRUE(mgr.listAllSignatures().empty());

    auto result = mgr.verifyAll();
    EXPECT_EQ(result.total, 0);
    EXPECT_FALSE(result.backend_available);
    EXPECT_FALSE(result.used_fallback_memory_store);
    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.error_message.empty());
}

TEST(SecuritySignatureVerifyAllFallbackTests, ExplicitFallbackListAllSignatures_ReturnsStoredSigs) {
    SecuritySignatureManager mgr(
        nullptr,
        SecuritySignatureManager::Options{.allow_in_memory_fallback = true});

    SecuritySignature sig;
    sig.resource_id = "fb_file";
    sig.hash        = std::string(64, 'e');
    sig.algorithm   = "sha256";
    sig.created_at  = 42;
    ASSERT_TRUE(mgr.storeSignature(sig));

    auto sigs = mgr.listAllSignatures();
    ASSERT_EQ(sigs.size(), 1u);
    EXPECT_EQ(sigs[0].resource_id, "fb_file");
    EXPECT_TRUE(mgr.isUsingFallbackMemoryStore());
    EXPECT_FALSE(mgr.hasPersistentBackend());
}

TEST(SecuritySignatureVerifyAllFallbackTests, ExplicitFallbackVerifyAll_EmptyStore) {
    SecuritySignatureManager mgr(
        nullptr,
        SecuritySignatureManager::Options{.allow_in_memory_fallback = true});
    auto result = mgr.verifyAll();
    EXPECT_EQ(result.total, 0);
    EXPECT_TRUE(result.backend_available);
    EXPECT_TRUE(result.used_fallback_memory_store);
    EXPECT_TRUE(result.success());
}
