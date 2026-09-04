#include <gtest/gtest.h>

#include "exporters/huggingface_hub_client.h"
#include "governance/model_governance.h"
#include "governance/policy_engine.h"
#include "security/key_provider.h"
#include "utils/audit_logger.h"
#include "exporters/exporter_metrics.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <cstdlib>

namespace fs = std::filesystem;
using namespace themis::exporters;
using themis::governance::ModelGovernancePolicy;
using themis::governance::PolicyEngine;
using json = nlohmann::json;

// ── Helpers ──────────────────────────────────────────────────────────────────

static std::string makeDatasetDir() {
    const std::string path =
        (fs::temp_directory_path() / "test_hf_hub_dataset_XXXXXX").string();
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

static std::string decodeBase64ForTest(const std::string& input) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> table(256, -1);
    for (size_t i = 0; i < chars.size(); ++i) {
        table[static_cast<unsigned char>(chars[i])] = static_cast<int>(i);
    }

    std::string out = {};
    int val = 0;
    int bits = -8;
    for (unsigned char c : input) {
        if (c == '=') {
            break;
        }
        int d = table[c];
        if (d < 0) {
            continue;
        }
        val = (val << 6) + d;
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

static std::string readDecodedAuditPayloadText(const std::string& log_path) {
    std::ifstream f(log_path);
    if (!f.is_open()) {
        return {};
    }

    std::ostringstream combined = {};
    std::string line = {};
    while (std::getline(f, line)) {
        if (line.empty()) {
            continue;
        }
        try {
            const auto record = json::parse(line);
            if (!record.contains("payload") || !record["payload"].is_object()) {
                combined << line << '\n';
                continue;
            }

            const auto& payload = record["payload"];
            if (payload.value("type", std::string{}) != "plaintext") {
                combined << line << '\n';
                continue;
            }

            if (payload.contains("data") && payload["data"].is_object()) {
                combined << payload["data"].dump() << '\n';
                continue;
            }
            if (payload.contains("data_b64") && payload["data_b64"].is_string()) {
                combined << decodeBase64ForTest(payload["data_b64"].get<std::string>()) << '\n';
                continue;
            }
            combined << line << '\n';
        } catch (...) {
            combined << line << '\n';
        }
    }

    return combined.str();
}

/// RAII helper that saves/restores the HF_TOKEN environment variable so tests
/// that set it do not leak state into subsequent tests, even on failure.
struct HfTokenGuard {
    std::string original_ = {};
    bool had_original_ = false;

    static std::string getEnvValue(const char* name, bool& found) {
        const char* raw = ::getenv(name);
        if (!raw) {
            found = false;
            return {};
        }
        found = true;
        return std::string(raw);
    }

    static void setEnvValue(const char* name, const char* value) {
#ifdef _WIN32
        _putenv_s(name, value ? value : "");
#else
        ::setenv(name, value ? value : "", 1);
#endif
    }

    static void unsetEnvValue(const char* name) {
#ifdef _WIN32
        _putenv_s(name, "");
#else
        ::unsetenv(name);
#endif
    }

    explicit HfTokenGuard(const char* value) {
        original_ = getEnvValue("HF_TOKEN", had_original_);
        if (value) {
          setEnvValue("HF_TOKEN", value);
        }
        else       unsetEnvValue("HF_TOKEN");
    }
    ~HfTokenGuard() {
        if (had_original_) {
          setEnvValue("HF_TOKEN", original_.c_str());
        }
        else               unsetEnvValue("HF_TOKEN");
    }
};

// ── TestTokenKeyProvider ─────────────────────────────────────────────────────

/// Minimal in-test KeyProvider that stores variable-length byte sequences
/// (HF tokens are UTF-8 strings, not fixed-size AES-256 keys).
/// NOT for production use.
class TestTokenKeyProvider : public themis::KeyProvider {
public:
    /// Register a token for the given kek_id.
    void storeToken(const std::string& kek_id, const std::string& token) {
        tokens_[kek_id] = std::vector<uint8_t>(token.begin(), token.end());
    }

    std::vector<uint8_t> getKey(const std::string& key_id) override {
        auto it = tokens_.find(key_id);
        if (it == tokens_.end()) {
            throw themis::KeyNotFoundException(key_id, 0);
        }
        return it->second;
    }

    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t /*version*/) override {
        return getKey(key_id);
    }

    uint32_t rotateKey(const std::string&) override { return 0; }
    std::vector<themis::KeyMetadata> listKeys() override { return {}; }

    themis::KeyMetadata getKeyMetadata(const std::string& key_id, uint32_t /*version*/ = 0) override {
        themis::KeyMetadata m;
        m.key_id = key_id;
        return m;
    }

    void deleteKey(const std::string&, uint32_t) override {}

    bool hasKey(const std::string& key_id, uint32_t /*version*/ = 0) override {
        return tokens_.count(key_id) > 0;
    }

    uint32_t createKeyFromBytes(const std::string& key_id,
                                const std::vector<uint8_t>& key_bytes,
                                const themis::KeyMetadata& /*metadata*/ = {}) override {
        tokens_[key_id] = key_bytes;
        return 1;
    }

private:
    std::map<std::string, std::vector<uint8_t>> tokens_;
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
    ASSERT_TRUE(fs::exists(log_path)) << "Audit log file not found: " << log_path;
    std::string content = readDecodedAuditPayloadText(log_path);
    EXPECT_FALSE(content.empty()) << "Audit log file is empty";
    EXPECT_NE(content.find("hub_upload"), std::string::npos)
        << "Expected 'hub_upload' event in audit log";
    EXPECT_NE(content.find("denied"), std::string::npos)
        << "Expected 'denied' outcome in audit log";
    EXPECT_NE(content.find("secret-org/restricted-data"), std::string::npos)
        << "Expected repo_id in audit log";
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
    ASSERT_TRUE(fs::exists(log_path)) << "Audit log file not found: " << log_path;
    std::string content = readDecodedAuditPayloadText(log_path);
    EXPECT_FALSE(content.empty()) << "Audit log file is empty";
    EXPECT_NE(content.find("hub_upload"), std::string::npos)
        << "Expected 'hub_upload' event in audit log";
    EXPECT_NE(content.find("error"), std::string::npos)
        << "Expected 'error' outcome in audit log";
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

// ── uploadShards: token / config validation ───────────────────────────────────

TEST(HuggingFaceHubClientTest, UploadShardsNoTokenReturnsError) {
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
    HubUploadConfig cfg;
    cfg.repo_id  = "org/repo";
    cfg.hf_token = "";
    HuggingFaceHubClient client(cfg);

    const std::string content = R"({"text":"hello"})" "\n";
    MemoryShardSpec shard;
    shard.relative_path = "data/train-00000-of-00001.jsonl";
    shard.content.assign(content.begin(), content.end());

    const auto result = client.uploadShards({shard});
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("HF_TOKEN"), std::string::npos);
}

TEST(HuggingFaceHubClientTest, UploadShardsEmptyRepoIdReturnsError) {
    HubUploadConfig cfg;
    cfg.repo_id  = "";  // intentionally empty
    cfg.hf_token = "test-token";
    HuggingFaceHubClient client(cfg);

    MemoryShardSpec shard;
    shard.relative_path = "data/shard.jsonl";
    shard.content       = {'a', 'b', 'c'};

    const auto result = client.uploadShards({shard});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(HuggingFaceHubClientTest, UploadShardsEmptyListReturnsError) {
    HubUploadConfig cfg;
    cfg.repo_id  = "org/repo";
    cfg.hf_token = "test-token";
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadShards({});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(HuggingFaceHubClientTest, UploadShardsProgressCallbackNotCalledWhenNoToken) {
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset
    HubUploadConfig cfg;
    cfg.repo_id = "org/repo";

    MemoryShardSpec shard;
    shard.relative_path = "data/shard.jsonl";
    shard.content       = {'x'};

    bool cb_called = false;
    HuggingFaceHubClient client(cfg);
    const auto result = client.uploadShards(
        {shard}, [&cb_called](double) { cb_called = true; });

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(cb_called);
}

// ── uploadShards: PolicyEngine integration ────────────────────────────────────

TEST(HuggingFaceHubClientTest, UploadShardsPolicyDeniedReturnsFailure) {
    auto mgp = std::make_shared<ModelGovernancePolicy>();
    mgp->addRestrictedCollection("restricted-org/secret-data");

    PolicyEngine engine;
    engine.setModelGovernancePolicy(mgp);

    HubUploadConfig cfg;
    cfg.repo_id         = "restricted-org/secret-data";
    cfg.hf_token        = "dummy-token";
    cfg.requesting_user = "eve";
    cfg.policy_engine   = &engine;
    HuggingFaceHubClient client(cfg);

    MemoryShardSpec shard;
    shard.relative_path = "data/shard.jsonl";
    shard.content       = {'a'};

    const auto result = client.uploadShards({shard});
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("PolicyEngine"), std::string::npos)
        << "Expected 'PolicyEngine' in error; got: " << result.error_message;
    EXPECT_EQ(result.http_status, 0);
}

TEST(HuggingFaceHubClientTest, UploadShardsTokenFromEnvProceeedsToNetwork) {
    // Policy permits; upload will fail at network level (unreachable host).
    HfTokenGuard token_guard("test-memory-token");

    auto mgp = std::make_shared<ModelGovernancePolicy>();
    PolicyEngine engine;
    engine.setModelGovernancePolicy(mgp);

    HubUploadConfig cfg;
    cfg.repo_id         = "org/allowed-dataset";
    cfg.hub_base_url    = "http://localhost:1"; // unreachable
    cfg.max_retries     = 0;
    cfg.timeout_seconds = 2;
    cfg.requesting_user = "dave";
    cfg.policy_engine   = &engine;
    HuggingFaceHubClient client(cfg);

    MemoryShardSpec shard;
    shard.relative_path = "data/train.jsonl";
    const std::string content = R"({"text":"hello world"})" "\n";
    shard.content.assign(content.begin(), content.end());

    const auto result = client.uploadShards({shard});
    EXPECT_FALSE(result.success);
    // Must NOT be a PolicyEngine or HF_TOKEN error.
    EXPECT_EQ(result.error_message.find("PolicyEngine"), std::string::npos)
        << "Got: " << result.error_message;
    EXPECT_EQ(result.error_message.find("HF_TOKEN"), std::string::npos)
        << "Got: " << result.error_message;
}

// ── uploadShards: Audit log integration ──────────────────────────────────────

TEST(HuggingFaceHubClientTest, UploadShardsAuditLogWrittenOnPolicyDenial) {
    auto [audit_logger, log_path] = makeTestAuditLogger();

    auto mgp = std::make_shared<ModelGovernancePolicy>();
    mgp->addRestrictedCollection("secret-org/restricted");

    PolicyEngine engine;
    engine.setModelGovernancePolicy(mgp);

    HubUploadConfig cfg;
    cfg.repo_id         = "secret-org/restricted";
    cfg.hf_token        = "dummy";
    cfg.requesting_user = "mallory";
    cfg.policy_engine   = &engine;
    cfg.audit_log       = audit_logger;
    HuggingFaceHubClient client(cfg);

    MemoryShardSpec shard;
    shard.relative_path = "data/shard.jsonl";
    shard.content       = {'a'};

    const auto result = client.uploadShards({shard});
    EXPECT_FALSE(result.success);

    audit_logger->flush();
    ASSERT_TRUE(fs::exists(log_path)) << "Audit log not found: " << log_path;
    std::string content_str = readDecodedAuditPayloadText(log_path);
    EXPECT_NE(content_str.find("hub_upload"), std::string::npos);
    EXPECT_NE(content_str.find("denied"),     std::string::npos);
    EXPECT_NE(content_str.find("secret-org/restricted"), std::string::npos);
}

TEST(HuggingFaceHubClientTest, UploadShardsAuditLogWrittenOnNoToken) {
    HfTokenGuard token_guard(nullptr);
    auto [audit_logger, log_path] = makeTestAuditLogger();

    HubUploadConfig cfg;
    cfg.repo_id         = "org/dataset";
    cfg.hf_token        = "";
    cfg.requesting_user = "carol";
    cfg.audit_log       = audit_logger;
    HuggingFaceHubClient client(cfg);

    MemoryShardSpec shard;
    shard.relative_path = "data/shard.jsonl";
    shard.content       = {'x'};

    const auto result = client.uploadShards({shard});
    EXPECT_FALSE(result.success);

    audit_logger->flush();
    ASSERT_TRUE(fs::exists(log_path)) << "Audit log not found: " << log_path;
    std::string content_str = readDecodedAuditPayloadText(log_path);
    EXPECT_NE(content_str.find("hub_upload"), std::string::npos);
    EXPECT_NE(content_str.find("error"), std::string::npos);
    EXPECT_NE(content_str.find("HF_TOKEN"), std::string::npos);
}

// ── hf_token_kek_id / KEK token resolution ───────────────────────────────────

TEST(HuggingFaceHubClientTest, DefaultConfigHasNoKekFields) {
    HubUploadConfig cfg;
    EXPECT_TRUE(cfg.hf_token_kek_id.empty());
    EXPECT_EQ(cfg.key_provider, nullptr);
}

TEST(HuggingFaceHubClientTest, KekTokenResolution_HappyPath) {
    // A TestTokenKeyProvider that returns a valid token for the given kek_id.
    // The upload will fail at the network layer (unreachable host), not at the
    // token-resolution stage.
    HfTokenGuard token_guard(nullptr);  // ensure HF_TOKEN is unset

    auto provider = std::make_shared<TestTokenKeyProvider>();
    provider->storeToken("hf-token-key", "hf_kek_resolved_token_xyz");

    const std::string dataset_dir = makeDatasetDir();
    HubUploadConfig cfg;
    cfg.repo_id          = "org/test-kek-repo";
    cfg.hf_token_kek_id  = "hf-token-key";
    cfg.key_provider     = provider;
    cfg.hub_base_url     = "http://localhost:1";  // unreachable → no real upload
    cfg.max_retries      = 0;
    cfg.timeout_seconds  = 2;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset(dataset_dir);
    // Should fail with a network/curl error, NOT with a token or KEK error.
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message.find("kek_id"), std::string::npos)
        << "Should not fail with kek_id error when token resolves; got: "
        << result.error_message;
    EXPECT_EQ(result.error_message.find("HF_TOKEN"), std::string::npos)
        << "Should not fail with HF_TOKEN error when token resolves via KEK; got: "
        << result.error_message;

    fs::remove_all(dataset_dir);
}

TEST(HuggingFaceHubClientTest, KekTokenResolution_PriorityOverEnv) {
    // hf_token_kek_id must take precedence over the HF_TOKEN env variable.
    HfTokenGuard token_guard("env-token-should-not-be-used");

    auto provider = std::make_shared<TestTokenKeyProvider>();
    provider->storeToken("my-hf-kek", "hf_from_kek_not_env");

    const std::string dataset_dir = makeDatasetDir();
    HubUploadConfig cfg;
    cfg.repo_id         = "org/prio-test-repo";
    cfg.hf_token_kek_id = "my-hf-kek";
    cfg.key_provider    = provider;
    cfg.hub_base_url    = "http://localhost:1";
    cfg.max_retries     = 0;
    cfg.timeout_seconds = 2;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset(dataset_dir);
    // Token was resolved (via KEK), so failure is network-related, not token.
    EXPECT_EQ(result.error_message.find("HF_TOKEN"), std::string::npos)
        << "KEK token should take priority over HF_TOKEN env; got: "
        << result.error_message;
    EXPECT_EQ(result.error_message.find("kek_id"), std::string::npos)
        << "KEK resolution should not have failed; got: " << result.error_message;

    fs::remove_all(dataset_dir);
}

TEST(HuggingFaceHubClientTest, KekTokenResolution_HfTokenTakesPriorityOverKek) {
    // hf_token (explicit) must take priority over hf_token_kek_id.
    HfTokenGuard token_guard(nullptr);

    auto provider = std::make_shared<TestTokenKeyProvider>();
    provider->storeToken("unused-kek", "hf_kek_token_unused");

    const std::string dataset_dir = makeDatasetDir();
    HubUploadConfig cfg;
    cfg.repo_id         = "org/priority-test";
    cfg.hf_token        = "hf_explicit_wins";
    cfg.hf_token_kek_id = "unused-kek";
    cfg.key_provider    = provider;
    cfg.hub_base_url    = "http://localhost:1";
    cfg.max_retries     = 0;
    cfg.timeout_seconds = 2;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset(dataset_dir);
    // Token resolved from hf_token; failure should be network-related.
    EXPECT_EQ(result.error_message.find("HF_TOKEN"), std::string::npos)
        << "hf_token should take priority; got: " << result.error_message;
    EXPECT_EQ(result.error_message.find("kek_id"), std::string::npos)
        << "kek_id path should not be reached; got: " << result.error_message;

    fs::remove_all(dataset_dir);
}

TEST(HuggingFaceHubClientTest, KekTokenResolution_NullProviderReturnsError) {
    // hf_token_kek_id set but key_provider is null → clear misconfiguration error.
    HfTokenGuard token_guard(nullptr);
    HubUploadConfig cfg;
    cfg.repo_id         = "org/repo";
    cfg.hf_token_kek_id = "some-kek-id";
    cfg.key_provider    = nullptr;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("hf_token_kek_id"), std::string::npos)
        << "Error should mention hf_token_kek_id; got: " << result.error_message;
    EXPECT_NE(result.error_message.find("key_provider"), std::string::npos)
        << "Error should mention key_provider; got: " << result.error_message;
}

TEST(HuggingFaceHubClientTest, KekTokenResolution_KeyNotFoundReturnsError) {
    // key_provider does not have the requested kek_id → clear error.
    HfTokenGuard token_guard(nullptr);

    auto provider = std::make_shared<TestTokenKeyProvider>();
    // Deliberately NOT storing a key for "missing-kek-id".

    HubUploadConfig cfg;
    cfg.repo_id         = "org/repo";
    cfg.hf_token_kek_id = "missing-kek-id";
    cfg.key_provider    = provider;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    // The error should be about KEK resolution failing, not "no HF_TOKEN".
    EXPECT_NE(result.error_message.find("hf_token_kek_id resolution failed"), std::string::npos)
        << "Error should indicate KEK resolution failure; got: " << result.error_message;
    EXPECT_EQ(result.error_message.find("HF_TOKEN"), std::string::npos)
        << "Should not mention 'HF_TOKEN' for KEK failure; got: " << result.error_message;
}

TEST(HuggingFaceHubClientTest, KekTokenResolution_AuditLogWrittenOnKekError) {
    // When KEK resolution fails, the audit log should record the error.
    HfTokenGuard token_guard(nullptr);
    auto [audit_logger, log_path] = makeTestAuditLogger();

    auto provider = std::make_shared<TestTokenKeyProvider>();

    HubUploadConfig cfg;
    cfg.repo_id         = "org/repo";
    cfg.hf_token_kek_id = "nonexistent-kek";
    cfg.key_provider    = provider;
    cfg.requesting_user = "dave";
    cfg.audit_log       = audit_logger;
    HuggingFaceHubClient client(cfg);

    const auto result = client.uploadDataset("/tmp/any_dir");
    EXPECT_FALSE(result.success);

    audit_logger->flush();
    ASSERT_TRUE(fs::exists(log_path)) << "Audit log not found: " << log_path;
    std::string content_str = readDecodedAuditPayloadText(log_path);
    EXPECT_NE(content_str.find("hub_upload"), std::string::npos);
    EXPECT_NE(content_str.find("error"),      std::string::npos);
}

// ── MemoryShardSpec construction ──────────────────────────────────────────────

TEST(HuggingFaceHubClientTest, MemoryShardSpecHoldsDataCorrectly) {
    const std::string jsonl = R"({"id":1,"text":"foo"})" "\n"
                              R"({"id":2,"text":"bar"})" "\n";
    MemoryShardSpec shard;
    shard.relative_path = "data/train-00000-of-00001.jsonl";
    shard.content.assign(jsonl.begin(), jsonl.end());

    EXPECT_EQ(shard.relative_path, "data/train-00000-of-00001.jsonl");
    EXPECT_EQ(shard.content.size(), jsonl.size());
    EXPECT_EQ(std::string(shard.content.begin(), shard.content.end()), jsonl);
}

// ── HTTP 429 / ExporterMetrics rate-limit integration ─────────────────────────

// ExporterMetrics: recordRateLimitHit increments counter.
TEST(HuggingFaceHubClientTest, ExporterMetrics_RecordRateLimitHit_IncrementsCounter) {
    ExporterMetrics m;
    EXPECT_EQ(m.getRateLimitHits(), 0u);
    m.recordRateLimitHit();
    EXPECT_EQ(m.getRateLimitHits(), 1u);
    m.recordRateLimitHit();
    m.recordRateLimitHit();
    EXPECT_EQ(m.getRateLimitHits(), 3u);
}

// ExporterMetrics: reset() clears rate_limit_hits.
TEST(HuggingFaceHubClientTest, ExporterMetrics_Reset_ClearsRateLimitHits) {
    ExporterMetrics m;
    m.recordRateLimitHit();
    m.recordRateLimitHit();
    ASSERT_EQ(m.getRateLimitHits(), 2u);
    m.reset();
    EXPECT_EQ(m.getRateLimitHits(), 0u);
}

// ExporterMetrics: toJson includes the rate_limit_hit key.
TEST(HuggingFaceHubClientTest, ExporterMetrics_ToJson_ContainsRateLimitHitKey) {
    ExporterMetrics m;
    m.recordRateLimitHit();
    const auto j = m.toJson();
    ASSERT_TRUE(j.contains("exporters.huggingface.rate_limit_hit"))
        << "toJson() must contain 'exporters.huggingface.rate_limit_hit'";
    EXPECT_EQ(j["exporters.huggingface.rate_limit_hit"].get<size_t>(), 1u);
}

// HubUploadConfig: metrics field can be set and accessed.
TEST(HuggingFaceHubClientTest, HubUploadConfig_MetricsFieldRoundtrip) {
    auto metrics = std::make_shared<ExporterMetrics>();
    HubUploadConfig cfg;
    cfg.repo_id  = "org/repo";
    cfg.hf_token = "tok";
    cfg.metrics  = metrics;

    // Construct client to confirm config field is accepted.
    HuggingFaceHubClient client(cfg);
    // Record a hit directly on the metrics object.
    metrics->recordRateLimitHit();
    EXPECT_EQ(metrics->getRateLimitHits(), 1u);
}

// When no HF token is configured, upload fails without touching metrics.
TEST(HuggingFaceHubClientTest, RateLimit_MetricsNotIncrementedOnAuthFailure) {
    HfTokenGuard guard(nullptr);  // ensure HF_TOKEN is unset
    auto metrics = std::make_shared<ExporterMetrics>();

    HubUploadConfig cfg;
    cfg.repo_id  = "org/repo";
    cfg.hf_token = "";
    cfg.metrics  = metrics;
    HuggingFaceHubClient client(cfg);

    const std::string dir = makeDatasetDir();
    const auto result = client.uploadDataset(dir);
    EXPECT_FALSE(result.success);
    // No HTTP calls were made (failed at token validation), so no rate-limit hits.
    EXPECT_EQ(metrics->getRateLimitHits(), 0u);
    fs::remove_all(dir);
}
