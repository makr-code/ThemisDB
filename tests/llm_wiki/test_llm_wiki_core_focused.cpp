/**
 * @file test_llm_wiki_core_focused.cpp
 * @brief Group WS — WorkspaceStateManager core tests (save / load / round-trip).
 *
 * Uses a temporary directory for all I/O so tests are hermetic.
 */

#include <gtest/gtest.h>
#include "llm_wiki/workspace_state_manager.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace themis::llm_wiki;

// ── fixture ───────────────────────────────────────────────────────────────────

class LlmWikiCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / ("test_llm_wiki_core_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tmp_dir_ / "wiki");
    }

    void TearDown() override {
        std::error_code ec = {};
        fs::remove_all(tmp_dir_, ec);
    }

    fs::path tmp_dir_;
};

// ── Group WS — Workspace State ────────────────────────────────────────────────

// WS1: Load on a non-existent state file returns FileNotFound
TEST_F(LlmWikiCoreTest, WS1_Load_NoFile_ReturnsFileNotFound) {
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState state;
    auto status = mgr.load(state);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, WorkspaceStatus::Code::FileNotFound);
}

// WS2: Save populates the state file on disk
TEST_F(LlmWikiCoreTest, WS2_Save_CreatesStateFile) {
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState state;
    state.workspace_root = tmp_dir_.string();
    state.version        = "1.0.0";

    auto status = mgr.save(state);
    EXPECT_TRUE(status.ok()) << status.message;

    EXPECT_TRUE(fs::exists(tmp_dir_ / "wiki" / "state.json"));
}

// WS3: Save then load roundtrip preserves workspace_root
TEST_F(LlmWikiCoreTest, WS3_SaveThenLoad_Roundtrip_PreservesWorkspaceRoot) {
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState saved;
    saved.workspace_root = tmp_dir_.string();
    saved.version        = "1.0.0";

    ASSERT_TRUE(mgr.save(saved).ok());

    WorkspaceState loaded;
    auto status = mgr.load(loaded);
    EXPECT_TRUE(status.ok()) << status.message;
    EXPECT_EQ(loaded.workspace_root, saved.workspace_root);
}

// WS4: Save then load preserves the links map
TEST_F(LlmWikiCoreTest, WS4_SaveThenLoad_Roundtrip_PreservesLinks) {
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState saved;
    saved.workspace_root     = tmp_dir_.string();
    saved.links["pageA"]     = {"pageB", "pageC"};
    saved.links["pageB"]     = {"pageA"};

    ASSERT_TRUE(mgr.save(saved).ok());

    WorkspaceState loaded;
    ASSERT_TRUE(mgr.load(loaded).ok());

    ASSERT_EQ(loaded.links.count("pageA"), 1u);
    EXPECT_EQ(loaded.links.at("pageA").size(), 2u);
}

// WS5: Save then load preserves tasks map
TEST_F(LlmWikiCoreTest, WS5_SaveThenLoad_Roundtrip_PreservesTasks) {
    WorkspaceStateManager mgr(tmp_dir_);
    WorkspaceState saved;
    saved.workspace_root      = tmp_dir_.string();
    saved.tasks["task-1"]     = {{"status", "pending"}, {"owner", "agent"}};

    ASSERT_TRUE(mgr.save(saved).ok());

    WorkspaceState loaded;
    ASSERT_TRUE(mgr.load(loaded).ok());

    ASSERT_EQ(loaded.tasks.count("task-1"), 1u);
    EXPECT_EQ(loaded.tasks.at("task-1").at("status"), "pending");
}

// WS6: Multiple consecutive saves all succeed
TEST_F(LlmWikiCoreTest, WS6_MultipleSaves_AllSucceed) {
    WorkspaceStateManager mgr(tmp_dir_);
    for (int i = 0; i < 3; ++i) {
        WorkspaceState state;
        state.workspace_root = tmp_dir_.string();
        state.version        = "1.0." + std::to_string(i);
        EXPECT_TRUE(mgr.save(state).ok()) << "Failed on iteration " << i;
    }
}

// WS7: After multiple saves, load returns the most recent state
TEST_F(LlmWikiCoreTest, WS7_MultipleSaves_LoadReturnsLatest) {
    WorkspaceStateManager mgr(tmp_dir_);
    for (int i = 0; i < 3; ++i) {
        WorkspaceState state;
        state.workspace_root = tmp_dir_.string();
        state.version        = "v" + std::to_string(i);
        ASSERT_TRUE(mgr.save(state).ok());
    }

    WorkspaceState loaded;
    ASSERT_TRUE(mgr.load(loaded).ok());
    EXPECT_EQ(loaded.version, "v2");
}
