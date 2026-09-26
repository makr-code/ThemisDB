/**
 * @file test_audit_log_store.cpp
 * @brief Tests for AuditLogStore
 *
 * Test cases: AUDIT-01..06
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

#include "security/audit_log_store.h"

using namespace themis::security;

class AuditLogStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() / "test_audit_store_XXXXXX";
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override {
    if (std::filesystem::exists(temp_dir_)) {
      std::filesystem::remove_all(temp_dir_);
    }
  }

  std::filesystem::path temp_dir_;

  std::unique_ptr<AuditLogStore> CreateStore() {
    return AuditLogStore::Open(temp_dir_.string());
  }

  AuditEntry CreateTestEntry(const std::string& query_id, const std::string& principal_id,
                              bool allowed) {
    AuditEntry entry;
    entry.query_id = query_id;
    entry.principal_id = principal_id;
    entry.resource_id = "index_1";
    entry.action = 0;  // Read
    entry.query_text = "SELECT * FROM index_1";
    entry.timestamp = std::chrono::system_clock::now();
    entry.allowed = allowed;
    entry.decision_reason = allowed ? "Principal has read permission" : "Access denied";
    entry.execution_time_ms = 100;
    entry.result_count = allowed ? 42 : 0;
    entry.success = true;
    return entry;
  }
};

/**
 * @test AUDIT-01: Append and retrieve entries
 */
TEST_F(AuditLogStoreTest, AUDIT_01_AppendAndRetrieve) {
  auto store = CreateStore();

  AuditEntry entry1 = CreateTestEntry("q1", "user_1", true);
  uint64_t id1 = store->AppendEntry(entry1);
  EXPECT_GT(id1, 0);

  AuditEntry retrieved = store->GetEntry(id1);
  EXPECT_EQ(retrieved.query_id, "q1");
  EXPECT_EQ(retrieved.principal_id, "user_1");
  EXPECT_TRUE(retrieved.allowed);
  EXPECT_EQ(retrieved.entry_id, id1);
}

/**
 * @test AUDIT-02: Entry counter increments
 */
TEST_F(AuditLogStoreTest, AUDIT_02_EntryCounter) {
  auto store = CreateStore();

  uint64_t id1 = store->AppendEntry(CreateTestEntry("q1", "user_1", true));
  uint64_t id2 = store->AppendEntry(CreateTestEntry("q2", "user_2", true));
  uint64_t id3 = store->AppendEntry(CreateTestEntry("q3", "user_3", false));

  EXPECT_EQ(id1, 1);
  EXPECT_EQ(id2, 2);
  EXPECT_EQ(id3, 3);
  EXPECT_EQ(store->GetEntryCount(), 3);
}

/**
 * @test AUDIT-03: Query by principal
 */
TEST_F(AuditLogStoreTest, AUDIT_03_QueryByPrincipal) {
  auto store = CreateStore();

  store->AppendEntry(CreateTestEntry("q1", "user_1", true));
  store->AppendEntry(CreateTestEntry("q2", "user_1", false));
  store->AppendEntry(CreateTestEntry("q3", "user_2", true));
  store->AppendEntry(CreateTestEntry("q4", "user_1", true));

  auto results = store->QueryByPrincipal("user_1");
  EXPECT_EQ(results.size(), 3);

  for (const auto& entry : results) {
    EXPECT_EQ(entry.principal_id, "user_1");
  }
}

/**
 * @test AUDIT-04: Query by resource
 */
TEST_F(AuditLogStoreTest, AUDIT_04_QueryByResource) {
  auto store = CreateStore();

  AuditEntry e1 = CreateTestEntry("q1", "user_1", true);
  AuditEntry e2 = CreateTestEntry("q2", "user_2", true);
  e2.resource_id = "index_2";

  store->AppendEntry(e1);
  store->AppendEntry(e2);
  store->AppendEntry(e1);  // Another entry for index_1

  auto results = store->QueryByResource("index_1");
  EXPECT_EQ(results.size(), 2);
  for (const auto& entry : results) {
    EXPECT_EQ(entry.resource_id, "index_1");
  }
}

/**
 * @test AUDIT-05: Query denied decisions
 */
TEST_F(AuditLogStoreTest, AUDIT_05_QueryDeniedDecisions) {
  auto store = CreateStore();

  store->AppendEntry(CreateTestEntry("q1", "user_1", true));
  store->AppendEntry(CreateTestEntry("q2", "user_2", false));
  store->AppendEntry(CreateTestEntry("q3", "user_1", true));
  store->AppendEntry(CreateTestEntry("q4", "user_3", false));
  store->AppendEntry(CreateTestEntry("q5", "user_2", false));

  auto denied = store->QueryDeniedDecisions();
  EXPECT_EQ(denied.size(), 3);

  for (const auto& entry : denied) {
    EXPECT_FALSE(entry.allowed);
  }
}

/**
 * @test AUDIT-06: List entries in range
 */
TEST_F(AuditLogStoreTest, AUDIT_06_ListEntriesInRange) {
  auto store = CreateStore();

  for (int i = 1; i <= 10; ++i) {
    store->AppendEntry(CreateTestEntry("q" + std::to_string(i), "user_1", i % 2 == 0));
  }

  auto results = store->ListEntries(3, 7);
  EXPECT_EQ(results.size(), 5);
  EXPECT_EQ(results[0].entry_id, 3);
  EXPECT_EQ(results[4].entry_id, 7);
}
