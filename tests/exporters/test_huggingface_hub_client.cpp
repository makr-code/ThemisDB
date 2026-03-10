/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_huggingface_hub_client.cpp                    ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-10                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     145                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "exporters/huggingface_hub_client.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace themis::exporters;

// ── Helpers ──────────────────────────────────────────────────────────────────

static std::string makeDatasetDir() {
    const std::string path = fs::temp_directory_path() / "test_hf_hub_dataset_XXXXXX";
    fs::create_directories(path);
    fs::create_directories(path + "/data");

    // Minimal dataset_info.json
    {
        std::ofstream f(path + "/dataset_info.json");
        f << R"({"dataset_name":"test","split_name":"train","features":[]})";
    }
    // README.md
    {
        std::ofstream f(path + "/README.md");
        f << "# Test Dataset\n";
    }
    // One JSONL shard
    {
        std::ofstream f(path + "/data/train-00000-of-00001.jsonl");
        f << R"({"text":"hello world"})" << '\n';
    }
    return path;
}

// ── Token resolution ─────────────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, NoTokenReturnsError) {
    // Ensure HF_TOKEN is not set in the test environment
    ::unsetenv("HF_TOKEN");
    const std::string dataset_dir = makeDatasetDir();

    HubUploadConfig cfg;
    cfg.repo_id   = "test-org/test-dataset";
    cfg.hf_token  = "";  // explicitly empty
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset(dataset_dir);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("HF_TOKEN"), std::string::npos);

    fs::remove_all(dataset_dir);
}

TEST(HuggingFaceHubClientTest, EmptyRepoidReturnsError) {
    HubUploadConfig cfg;
    cfg.repo_id  = "";  // intentionally empty
    cfg.hf_token = "test-token";
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(HuggingFaceHubClientTest, NonexistentDatasetDirBehavesGracefully) {
    // With no token: should fail at token check, not filesystem access.
    ::unsetenv("HF_TOKEN");
    HubUploadConfig cfg;
    cfg.repo_id  = "org/repo";
    cfg.hf_token = "";
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/definitely_does_not_exist_xyz");
    // Should fail before filesystem iteration (no token)
    EXPECT_FALSE(result.success);
}

// ── Config defaults ───────────────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, DefaultConfigValues) {
    HubUploadConfig cfg;
    cfg.repo_id = "org/repo";
    EXPECT_EQ(cfg.hub_base_url,    "https://huggingface.co");
    EXPECT_EQ(cfg.max_retries,     3);
    EXPECT_EQ(cfg.retry_delay_ms,  1000);
    EXPECT_EQ(cfg.timeout_seconds, 120L);
    EXPECT_TRUE(cfg.create_repo);
    EXPECT_FALSE(cfg.private_repo);
}

TEST(HuggingFaceHubClientTest, DefaultCommitMessage) {
    HubUploadConfig cfg;
    EXPECT_FALSE(cfg.commit_message.empty());
    EXPECT_NE(cfg.commit_message.find("ThemisDB"), std::string::npos);
}

// ── HF_TOKEN environment variable ────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, TokenFromEnvPreferredOverEmpty) {
    // Set HF_TOKEN env and expect the client to pick it up via resolveToken()
    // (we cannot actually upload; we verify the token check passes and the
    // error is about the network/repo, not the token).
    ::setenv("HF_TOKEN", "test-env-token-xyz", 1);

    const std::string dataset_dir = makeDatasetDir();
    HubUploadConfig cfg;
    cfg.repo_id      = "org/test-repo";
    cfg.hub_base_url = "http://localhost:1"; // unreachable → no real upload
    cfg.max_retries  = 0;
    cfg.timeout_seconds = 2;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset(dataset_dir);
    // Should fail with a network/curl error, NOT a "no HF_TOKEN" error
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message.find("HF_TOKEN"), std::string::npos)
        << "Should not fail with 'HF_TOKEN' message when token is set via env; got: "
        << result.error_message;

    ::unsetenv("HF_TOKEN");
    fs::remove_all(dataset_dir);
}

// ── Progress callback ─────────────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, ProgressCallbackNotCalledWhenNoToken) {
    ::unsetenv("HF_TOKEN");
    HubUploadConfig cfg;
    cfg.repo_id = "org/repo";

    bool cb_called = false;
    HuggingFaceHubClient client(cfg);
    const auto result = client.uploadDataset("/tmp/no_token_test",
        [&cb_called](double) { cb_called = true; });

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(cb_called);
}
