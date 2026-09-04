/**
 * @file test_llm_wiki_edge_cases_focused.cpp
 * @brief Group WE — WorkspaceStateManager edge-case and corruption tests.
 */

#include <gtest/gtest.h>
#include "llm_wiki/workspace_state_manager.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis::llm_wiki;

// ── fixture ───────────────────────────────────────────────────────────────────

class LlmWikiEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / ("test_llm_wiki_edge_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tmp_dir_ / "wiki");
    }

    void TearDown() override {
        std::error_code ec = {};
        fs::remove_all(tmp_dir_, ec);
    }

    fs::path tmp_dir_;
};

// ── Group WE — Edge Cases ─────────────────────────────────────────────────────

// WE1: Load on a file with invalid JSON returns InvalidJson or CorruptState
TEST_F(LlmWikiEdgeCaseTest, WE1_LoadCorruptJson_ReturnsErrorCode) {
    auto state_file = tmp_dir_ / "wiki" / "state.json";
    {
        std::ofstream f(state_file);
        f << "NOT VALID JSON }{";
    }

    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState state;
    auto status = mgr.load(state);
    EXPECT_FALSE(status.ok());
    // Must be either InvalidJson or CorruptState — not OK
    bool expected_code =
        (status.code == WorkspaceStatus::Code::InvalidJson ||
         status.code == WorkspaceStatus::Code::CorruptState);
    EXPECT_TRUE(expected_code) << "Unexpected code: " << static_cast<int>(status.code);
}

// WE2: Load on a file with valid JSON but mismatched checksum returns ChecksumMismatch
TEST_F(LlmWikiEdgeCaseTest, WE2_LoadChecksumMismatch_ReturnsChecksumMismatch) {
    auto state_file = tmp_dir_ / "wiki" / "state.json";
    {
        // Write syntactically valid JSON with a wrong checksum
        std::ofstream f(state_file);
        f << R"({"version":"1.0.0","workspace_root":"/tmp","checksum":"deadbeef"})";
    }

    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState state;
    auto status = mgr.load(state);
    EXPECT_FALSE(status.ok());
    // Must indicate checksum failure or corrupt state
    bool expected_code =
        (status.code == WorkspaceStatus::Code::ChecksumMismatch ||
         status.code == WorkspaceStatus::Code::CorruptState);
    EXPECT_TRUE(expected_code) << "Unexpected code: " << static_cast<int>(status.code);
}

// WE3: Save with an extremely large links map succeeds
TEST_F(LlmWikiEdgeCaseTest, WE3_SaveLargeLinksMap_Succeeds) {
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState state;
    state.workspace_root = tmp_dir_.string();
    for (int i = 0; i < 500; ++i) {
        std::string page = "page_" + std::to_string(i);
        state.links[page] = {"refA", "refB", "refC"};
    }
    auto status = mgr.save(state);
    EXPECT_TRUE(status.ok()) << status.message;
}

// WE4: validateChecksum on a non-existent file returns error
TEST_F(LlmWikiEdgeCaseTest, WE4_ValidateChecksum_NonExistentFile_ReturnsError) {
    auto non_existent = tmp_dir_ / "wiki" / "no_such_file.json";
    auto status = WorkspaceStateManager::validateChecksum(non_existent);
    EXPECT_FALSE(status.ok());
}

// WE5: WorkspaceStatus factory methods produce consistent state
TEST_F(LlmWikiEdgeCaseTest, WE5_WorkspaceStatus_FactoryMethods_ConsistentState) {
    auto ok = WorkspaceStatus::Ok();
    EXPECT_TRUE(ok.ok());
    EXPECT_EQ(ok.code, WorkspaceStatus::Code::Ok);

    auto err = WorkspaceStatus::Error("some error");
    EXPECT_FALSE(err.ok());
    EXPECT_FALSE(err.message.empty());

    auto corrupt = WorkspaceStatus::CorruptState("state corrupted");
    EXPECT_FALSE(corrupt.ok());
    EXPECT_EQ(corrupt.code, WorkspaceStatus::Code::CorruptState);
}

// WE6: Load after corrupt-then-valid-save recovers correctly
TEST_F(LlmWikiEdgeCaseTest, WE6_CorruptThenValidSave_RecoversByLoad) {
    // First write a valid state
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState original;
    original.workspace_root = tmp_dir_.string();
    original.version        = "1.0.0";
    ASSERT_TRUE(mgr.save(original).ok());

    // Corrupt the state file
    auto state_file = tmp_dir_ / "wiki" / "state.json";
    {
        std::ofstream f(state_file, std::ios::trunc);
        f << "CORRUPTED";
    }

    // Now save again — should overwrite the corrupt file
    WorkspaceState fresh;
    fresh.workspace_root = tmp_dir_.string();
    fresh.version        = "2.0.0";
    ASSERT_TRUE(mgr.save(fresh).ok());

    // Load should now succeed
    WorkspaceState loaded;
    auto status = mgr.load(loaded);
    EXPECT_TRUE(status.ok()) << status.message;
    EXPECT_EQ(loaded.version, "2.0.0");
}
