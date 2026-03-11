/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_huggingface_hub_client.cpp                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-11                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "exporters/huggingface_hub_client.h"
#include "governance/model_governance.h"
#include "governance/policy_engine.h"
#include "utils/audit_logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace themis::exporters;
using themis::governance::ModelGovernancePolicy;
using themis::governance::PolicyEngine;

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

/// Create an AuditLogger that writes to a temporary file.
static std::pair<std::shared_ptr<themis::utils::AuditLogger>, std::string>
makeTestAuditLogger() {
    const std::string log_path =
        (fs::temp_directory_path() / "test_hf_hub_audit_XXXXXX.jsonl").string();
    themis::utils::AuditLoggerConfig cfg;
    cfg.log_path    = log_path;
    cfg.enabled     = true;
    cfg.enable_hash_chain = false;
    auto logger = std::make_shared<themis::utils::AuditLogger>(nullptr, nullptr, cfg);
    return {logger, log_path};
}

/// RAII helper that saves/restores the HF_TOKEN environment variable so tests
/// that set it do not leak state into subsequent tests, even on failure.
struct HfTokenGuard {
    const char* original_;
    explicit HfTokenGuard(const char* value) {
        original_ = ::getenv("HF_TOKEN");
        if (value) ::setenv("HF_TOKEN", value, 1);
        else        ::unsetenv("HF_TOKEN");
    }
    ~HfTokenGuard() {
        if (original_) ::setenv("HF_TOKEN", original_, 1);
        else            ::unsetenv("HF_TOKEN");
    }
};

// ── Token resolution ─────────────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, NoTokenReturnsError) {
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
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
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
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
    EXPECT_EQ(cfg.policy_engine, nullptr);
    EXPECT_EQ(cfg.audit_log,     nullptr);
    EXPECT_TRUE(cfg.requesting_user.empty());
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
    HfTokenGuard token_guard("test-env-token-xyz");

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

    fs::remove_all(dataset_dir);
}

// ── Progress callback ─────────────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, ProgressCallbackNotCalledWhenNoToken) {
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
    HubUploadConfig cfg;
    cfg.repo_id = "org/repo";

    bool cb_called = false;
    HuggingFaceHubClient client(cfg);
    const auto result = client.uploadDataset("/tmp/no_token_test",
        [&cb_called](double) { cb_called = true; });

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(cb_called);
}

// ── PolicyEngine integration ──────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, PolicyEngineDeniedUploadReturnsFailure) {
    // Arrange: a PolicyEngine whose ModelGovernancePolicy restricts the repo.
    auto mgp = std::make_shared<ModelGovernancePolicy>();
    // repo_id is used as the collection_id in the export request.
    mgp->addRestrictedCollection("restricted-org/secret-dataset");

    PolicyEngine engine;
    engine.setModelGovernancePolicy(mgp);

    HubUploadConfig cfg;
    cfg.repo_id          = "restricted-org/secret-dataset";
    cfg.hf_token         = "dummy-token";
    cfg.requesting_user  = "alice";
    cfg.policy_engine    = &engine;

    HuggingFaceHubClient client(cfg);
    const auto result = client.uploadDataset("/tmp/any_dir");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    // Error message must mention PolicyEngine denial.
    EXPECT_NE(result.error_message.find("PolicyEngine"), std::string::npos)
        << "Expected 'PolicyEngine' in error message, got: " << result.error_message;
    // No HTTP activity should have occurred.
    EXPECT_EQ(result.http_status, 0);
}

TEST(HuggingFaceHubClientTest, PolicyEnginePermittedUploadProceedsToNetwork) {
    // Arrange: a PolicyEngine that permits the upload (no restricted collections).
    auto mgp = std::make_shared<ModelGovernancePolicy>();
    PolicyEngine engine;
    engine.setModelGovernancePolicy(mgp);

    HfTokenGuard token_guard("test-token-for-policy");
    const std::string dataset_dir = makeDatasetDir();

    HubUploadConfig cfg;
    cfg.repo_id         = "public-org/allowed-dataset";
    cfg.hub_base_url    = "http://localhost:1"; // unreachable → no real upload
    cfg.max_retries     = 0;
    cfg.timeout_seconds = 2;
    cfg.requesting_user = "bob";
    cfg.policy_engine   = &engine;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset(dataset_dir);
    // The upload will fail at the network level (unreachable host), but the
    // error must NOT be about PolicyEngine denial.
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message.find("PolicyEngine"), std::string::npos)
        << "Should not fail with 'PolicyEngine' message when policy permits; got: "
        << result.error_message;

    fs::remove_all(dataset_dir);
}

TEST(HuggingFaceHubClientTest, PolicyEngineNullptrIsBackwardCompatible) {
    // When policy_engine is null, behaviour is unchanged (backward compat).
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
    HubUploadConfig cfg;
    cfg.repo_id      = "org/repo";
    cfg.hf_token     = "";
    cfg.policy_engine = nullptr;  // explicit null
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    // Should fail at token check, not PolicyEngine.
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("HF_TOKEN"), std::string::npos);
}

// ── Audit log integration ─────────────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, AuditLogWrittenOnPolicyDenial) {
    auto [audit_logger, log_path] = makeTestAuditLogger();

    auto mgp = std::make_shared<ModelGovernancePolicy>();
    mgp->addRestrictedCollection("secret-org/restricted-data");

    PolicyEngine engine;
    engine.setModelGovernancePolicy(mgp);

    HubUploadConfig cfg;
    cfg.repo_id         = "secret-org/restricted-data";
    cfg.hf_token        = "dummy-token";
    cfg.requesting_user = "mallory";
    cfg.policy_engine   = &engine;
    cfg.audit_log       = audit_logger;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);

    // Flush and check the log file for audit entries.
    audit_logger->flush();
    std::ifstream f(log_path);
    ASSERT_TRUE(f.good()) << "Audit log file not found: " << log_path;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty()) << "Audit log file is empty";
    EXPECT_NE(content.find("hub_upload"), std::string::npos)
        << "Expected 'hub_upload' event in audit log";
    EXPECT_NE(content.find("denied"), std::string::npos)
        << "Expected 'denied' outcome in audit log";
    EXPECT_NE(content.find("secret-org/restricted-data"), std::string::npos)
        << "Expected repo_id in audit log";

    fs::remove(log_path);
}

TEST(HuggingFaceHubClientTest, AuditLogWrittenOnNoTokenError) {
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
    auto [audit_logger, log_path] = makeTestAuditLogger();

    HubUploadConfig cfg;
    cfg.repo_id         = "org/my-dataset";
    cfg.hf_token        = "";
    cfg.requesting_user = "carol";
    cfg.audit_log       = audit_logger;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);

    audit_logger->flush();
    std::ifstream f(log_path);
    ASSERT_TRUE(f.good()) << "Audit log file not found: " << log_path;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty()) << "Audit log file is empty";
    EXPECT_NE(content.find("hub_upload"), std::string::npos)
        << "Expected 'hub_upload' event in audit log";
    EXPECT_NE(content.find("error"), std::string::npos)
        << "Expected 'error' outcome in audit log";

    fs::remove(log_path);
}

TEST(HuggingFaceHubClientTest, AuditLogNullptrIsBackwardCompatible) {
    // When audit_log is null, no crash — only the upload logic runs.
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
    HubUploadConfig cfg;
    cfg.repo_id   = "org/repo";
    cfg.audit_log = nullptr;  // explicit null
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);  // fails at token check, not audit log
}
