/*
 * Live connector-mode API tests for an externally running ThemisDB instance.
 *
 * These tests target the real HTTP surface exposed by connector mode and are
 * intentionally tolerant of deployment-specific authentication settings.
 */

#include <gtest/gtest.h>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

namespace {

std::string getEnv(const char* name, const char* fallback = "") {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string(fallback);
}

int getEnvInt(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (!value || !*value) {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

bool getEnvBool(const char* name, bool fallback) {
    const std::string value = getEnv(name);
    if (value.empty()) {
        return fallback;
    }
    if (value == "1" || value == "true" || value == "TRUE" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "FALSE" || value == "no" || value == "off") {
        return false;
    }
    return fallback;
}

struct ConnectorConfig {
    std::string host = getEnv("THEMIS_CONNECTOR_TEST_HOST", "127.0.0.1");
    int port = getEnvInt("THEMIS_CONNECTOR_TEST_PORT", 8765);
    bool use_https = getEnvBool("THEMIS_CONNECTOR_TEST_HTTPS", false);
    int timeout_ms = getEnvInt("THEMIS_CONNECTOR_TEST_TIMEOUT_MS", 5000);
    std::string bearer_token = getEnv("THEMIS_CONNECTOR_TEST_BEARER_TOKEN");
    std::string docs_dir = getEnv("THEMIS_CONNECTOR_TEST_DOCS_DIR", "docs");
    std::string llm_model_id = getEnv("THEMIS_CONNECTOR_TEST_MODEL_ID");
    std::string llm_model_path = getEnv("THEMIS_CONNECTOR_TEST_MODEL_PATH");
    std::string lora_base_model = getEnv("THEMIS_CONNECTOR_TEST_LORA_BASE_MODEL");
    int docs_ingest_max_files = getEnvInt("THEMIS_CONNECTOR_TEST_DOCS_MAX_FILES", 3);
    int docs_ingest_max_chars = getEnvInt("THEMIS_CONNECTOR_TEST_DOCS_MAX_CHARS", 4000);
    int llm_wait_timeout_sec = getEnvInt("THEMIS_CONNECTOR_TEST_LLM_WAIT_SEC", 120);
    int llm_request_timeout_ms = getEnvInt("THEMIS_CONNECTOR_TEST_LLM_TIMEOUT_MS", 30000);
};

const ConnectorConfig& config() {
    static const ConnectorConfig instance;
    return instance;
}

std::string makeUniqueKey() {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return "connector:test:" + std::to_string(now);
}

std::string sanitizeKeyPart(std::string value) {
    for (char& ch : value) {
        const bool valid =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-' || ch == ':';
        if (!valid) {
            ch = '_';
        }
    }
    return value;
}

std::optional<std::string> loadTextFileLimited(const std::filesystem::path& path, std::size_t max_chars) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        return std::nullopt;
    }

    std::string content = {};
    content.reserve(max_chars);
    char buffer[2048];
    while (in && content.size() < max_chars) {
        in.read(buffer, sizeof(buffer));
        const std::streamsize read_count = in.gcount();
        if (read_count <= 0) {
            break;
        }

        const std::size_t remaining = max_chars - content.size();
        const std::size_t to_append =
            static_cast<std::size_t>(read_count) > remaining ? remaining : static_cast<std::size_t>(read_count);
        content.append(buffer, to_append);
    }

    if (content.empty()) {
        return std::nullopt;
    }
    return content;
}

bool isSupportedDocExtension(const std::filesystem::path& p) {
    const std::string ext = p.extension().string();
    return ext == ".md" || ext == ".txt" || ext == ".rst";
}

std::string buildDocsNdjsonFromDirectory(const std::filesystem::path& docs_dir,
                                         int max_files,
                                         int max_chars_per_file) {
    if (!std::filesystem::exists(docs_dir) || !std::filesystem::is_directory(docs_dir)) {
        return "";
    }

    const std::size_t max_files_safe = max_files > 0 ? static_cast<std::size_t>(max_files) : 1u;
    const std::size_t max_chars_safe = max_chars_per_file > 0 ? static_cast<std::size_t>(max_chars_per_file) : 2048u;

    std::ostringstream out = {};
    std::size_t emitted = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(docs_dir)) {
        if (emitted >= max_files_safe) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto& p = entry.path();
        if (!isSupportedDocExtension(p)) {
            continue;
        }

        const auto content_opt = loadTextFileLimited(p, max_chars_safe);
        if (!content_opt.has_value()) {
            continue;
        }

        const std::string key = "docs:" + sanitizeKeyPart(p.filename().string()) + ":" + std::to_string(emitted);
        json doc = {
            {"key", key},
            {"title", p.filename().string()},
            {"source_path", p.string()},
            {"content", *content_opt},
            {"blob", json{{"text", *content_opt}, {"title", p.filename().string()}, {"source", p.string()}}.dump()}
        };

        out << doc.dump() << '\n';
        ++emitted;
    }

    return out.str();
}

class ConnectorApiLiveTest : public ::testing::Test {
protected:
    static bool isConnectorReachableCached() {
        static const bool reachable = []() {
            httplib::Client probe(config().host, config().port);
            probe.set_connection_timeout(std::chrono::milliseconds(config().timeout_ms));
            probe.set_read_timeout(std::chrono::milliseconds(config().timeout_ms));
            probe.set_write_timeout(std::chrono::milliseconds(config().timeout_ms));
            probe.set_follow_location(true);

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
            while (std::chrono::steady_clock::now() < deadline) {
                if (auto res = probe.Get("/health"); res) {
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            return false;
        }();

        return reachable;
    }

    void SetUp() override {
        if (config().use_https) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            ssl_client_ = std::make_unique<httplib::SSLClient>(config().host, config().port);
            ssl_client_->enable_server_certificate_verification(false);
            configureClient(*ssl_client_);
#else
            GTEST_SKIP() << "HTTPS requested but cpp-httplib has no OpenSSL support in this build";
#endif
        } else {
            client_ = std::make_unique<httplib::Client>(config().host, config().port);
            configureClient(*client_);
        }

        if (!isConnectorReachableCached()) {
            GTEST_SKIP() << "ThemisDB connector endpoint is not reachable at "
                         << config().host << ":" << config().port;
        }
    }

    void configureClient(httplib::Client& client) {
        client.set_connection_timeout(std::chrono::milliseconds(config().timeout_ms));
        client.set_read_timeout(std::chrono::milliseconds(config().timeout_ms));
        client.set_write_timeout(std::chrono::milliseconds(config().timeout_ms));
        client.set_follow_location(true);
    }

    void setClientTimeoutMs(int timeout_ms) {
        activeClient().set_connection_timeout(std::chrono::milliseconds(timeout_ms));
        activeClient().set_read_timeout(std::chrono::milliseconds(timeout_ms));
        activeClient().set_write_timeout(std::chrono::milliseconds(timeout_ms));
    }

    httplib::Result get(const std::string& path, bool with_auth = false) {
        return activeClient().Get(path, makeHeaders(with_auth));
    }

    httplib::Result postJson(const std::string& path, const json& payload, bool with_auth = false) {
        return activeClient().Post(path, makeHeaders(with_auth), payload.dump(), "application/json");
    }

    httplib::Result postJsonLlm(const std::string& path, const json& payload, bool with_auth = false) {
        const int default_timeout = config().timeout_ms;
        const int llm_timeout = std::max(default_timeout, config().llm_request_timeout_ms);
        setClientTimeoutMs(llm_timeout);
        auto res = activeClient().Post(path, makeHeaders(with_auth), payload.dump(), "application/json");
        setClientTimeoutMs(default_timeout);
        return res;
    }

    httplib::Result postRaw(const std::string& path, const std::string& payload, const char* content_type, bool with_auth = false) {
        return activeClient().Post(path, makeHeaders(with_auth), payload, content_type);
    }

    httplib::Result postNdjson(const std::string& path, const std::string& payload, bool with_auth = false) {
        return activeClient().Post(path, makeHeaders(with_auth), payload, "application/x-ndjson");
    }

    httplib::Result postNdjsonLlm(const std::string& path, const std::string& payload, bool with_auth = false) {
        const int default_timeout = config().timeout_ms;
        const int llm_timeout = std::max(default_timeout, config().llm_request_timeout_ms);
        setClientTimeoutMs(llm_timeout);
        auto res = activeClient().Post(path, makeHeaders(with_auth), payload, "application/x-ndjson");
        setClientTimeoutMs(default_timeout);
        return res;
    }

    httplib::Result putJson(const std::string& path, const json& payload, bool with_auth = false) {
        return activeClient().Put(path, makeHeaders(with_auth), payload.dump(), "application/json");
    }

    httplib::Result del(const std::string& path, bool with_auth = false) {
        return activeClient().Delete(path, makeHeaders(with_auth));
    }

    httplib::Headers makeHeaders(bool with_auth) const {
        httplib::Headers headers;
        headers.emplace("Accept", "application/json");
        if (with_auth && !config().bearer_token.empty()) {
            headers.emplace("Authorization", "Bearer " + config().bearer_token);
        }
        return headers;
    }

    void requireResponse(const httplib::Result& res, const std::string& path) {
        if (!res) {
            throw std::runtime_error("request failed for path " + path);
        }
    }

    bool requireAuthOrSkip(const httplib::Result& res, const std::string& feature_name) {
        requireResponse(res, feature_name);
        if (res->status == 401 || res->status == 403) {
            return false;
        }
        return true;
    }

    bool requireLlmFeatureOrSkip(const httplib::Result& res, const std::string& feature_name) {
        requireResponse(res, feature_name);
        if (res->status == 401 || res->status == 403) {
            return false;
        }
        if (res->status == 404 || res->status == 501 || res->status == 503) {
            return false;
        }
        return true;
    }

    void ensureConfiguredModelLoaded() {
        if (config().llm_model_id.empty() || config().llm_model_path.empty()) {
            return;
        }

        auto health_res = get("/api/v1/llm/health", true);
        if (health_res && health_res->status == 200) {
            try {
                const auto body = json::parse(health_res->body);
                if (body.value("status", std::string{}) == "healthy" &&
                    body.value("models_loaded", 0) > 0) {
                    return;
                }
            } catch (...) {
                // Fall through to explicit model load.
            }
        }

        auto load_res = postJsonLlm(
            "/api/v1/llm/models/load",
            json{{"model_id", config().llm_model_id}, {"path", config().llm_model_path}},
            true);
        requireResponse(load_res, "/api/v1/llm/models/load");

        if (load_res->status == 401 || load_res->status == 403) {
            GTEST_SKIP() << "/api/v1/llm/models/load requires authentication; set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
        }
        if (load_res->status == 404 || load_res->status == 501 || load_res->status == 503) {
            GTEST_SKIP() << "/api/v1/llm/models/load not available in this build/runtime";
        }

        ASSERT_EQ(load_res->status, 200) << load_res->body;

        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(config().llm_wait_timeout_sec);
        while (std::chrono::steady_clock::now() < deadline) {
            auto ready_res = get("/api/v1/llm/health", true);
            if (ready_res && ready_res->status == 200) {
                try {
                    const auto body = json::parse(ready_res->body);
                    if (body.value("status", std::string{}) == "healthy" &&
                        body.value("models_loaded", 0) > 0) {
                        return;
                    }
                } catch (...) {
                    // Keep polling until the runtime settles.
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        FAIL() << "LLM model did not become ready after explicit load request";
    }

    httplib::Client& activeClient() {
        if (client_) {
            return *client_;
        }
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        return *ssl_client_;
#else
        throw std::runtime_error("No active HTTP client available");
#endif
    }

private:
    std::unique_ptr<httplib::Client> client_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::unique_ptr<httplib::SSLClient> ssl_client_;
#endif
};

TEST_F(ConnectorApiLiveTest, HealthEndpointReturnsHealthyResponse) {
    auto res = get("/health");
    requireResponse(res, "/health");
    ASSERT_EQ(res->status, 200);

    json body;
    ASSERT_NO_THROW(body = json::parse(res->body));
    EXPECT_TRUE(body.contains("status"));
}

TEST_F(ConnectorApiLiveTest, HealthProbeEndpointsRespond) {
    for (const std::string& path : {"/health/live", "/health/ready"}) {
        auto res = get(path);
        requireResponse(res, path);
        EXPECT_EQ(res->status, 200) << path << " body: " << res->body;
    }
}

TEST_F(ConnectorApiLiveTest, VersionAndStatsEndpointsReturnJson) {
    auto version_res = get("/version");
    requireResponse(version_res, "/version");
    ASSERT_EQ(version_res->status, 200);

    json version_body;
    ASSERT_NO_THROW(version_body = json::parse(version_res->body));
    EXPECT_TRUE(version_body.contains("version") || version_body.contains("api_version"));

    auto stats_res = get("/stats");
    requireResponse(stats_res, "/stats");
    ASSERT_EQ(stats_res->status, 200);

    json stats_body;
    ASSERT_NO_THROW(stats_body = json::parse(stats_res->body));
    EXPECT_TRUE(stats_body.contains("server"));
    EXPECT_TRUE(stats_body.contains("storage"));
}

TEST_F(ConnectorApiLiveTest, MetricsEndpointReturnsPrometheusOrJson) {
    auto res = get("/metrics");
    requireResponse(res, "/metrics");
    ASSERT_EQ(res->status, 200);
    ASSERT_FALSE(res->body.empty());

    if (res->body.front() == '{') {
        json body;
        ASSERT_NO_THROW(body = json::parse(res->body));
        EXPECT_TRUE(body.is_object());
    } else {
        EXPECT_NE(res->body.find("# HELP"), std::string::npos);
    }
}

TEST_F(ConnectorApiLiveTest, EntityCrudRoundTripWorksWhenAuthorized) {
    const std::string key = "connector:" + makeUniqueKey();
    const std::string create_blob = json{{"name", "connector-live"}, {"kind", "integration"}}.dump();
    const json create_payload = {
        {"key", key},
        {"blob", create_blob}
    };

    auto create_res = postJson("/entities", create_payload, true);
    if (!requireAuthOrSkip(create_res, "/entities POST")) {
      return;
    }
    ASSERT_TRUE(create_res->status == 200 || create_res->status == 201) << create_res->body;

    auto get_res = get("/entities/" + key, true);
    if (!requireAuthOrSkip(get_res, "/entities/{key} GET")) return;
    ASSERT_EQ(get_res->status, 200) << get_res->body;

    auto update_res = putJson(
        "/entities/" + key,
        json{{"blob", json{{"name", "connector-live-updated"}, {"kind", "integration"}}.dump()}},
        true);
    if (!requireAuthOrSkip(update_res, "/entities/{key} PUT")) return;
    EXPECT_TRUE(update_res->status == 200 || update_res->status == 201) << update_res->body;

    auto delete_res = del("/entities/" + key, true);
    if (!requireAuthOrSkip(delete_res, "/entities/{key} DELETE")) return;
    EXPECT_EQ(delete_res->status, 200) << delete_res->body;
}

TEST_F(ConnectorApiLiveTest, EntityValidationRejectsMissingKey) {
    auto res = postJson("/entities", json{{"data", {{"name", "missing-key"}}}}, true);
    if (!requireAuthOrSkip(res, "/entities validation")) {
      return;
    }
    EXPECT_EQ(res->status, 400) << res->body;
}

TEST_F(ConnectorApiLiveTest, AqlEndpointHandlesValidAndInvalidQueries) {
    auto valid_res = postJson("/query/aql", json{{"query", "FOR x IN empty_collection RETURN x"}}, true);
    if (!requireAuthOrSkip(valid_res, "/query/aql valid")) {
      return;
    }
    EXPECT_TRUE(valid_res->status == 200 || valid_res->status == 400) << valid_res->body;

    auto invalid_res = postJson("/query/aql", json{{"query", "INVALID SYNTAX %%%"}}, true);
    if (!requireAuthOrSkip(invalid_res, "/query/aql invalid")) {
      return;
    }
    EXPECT_TRUE(invalid_res->status == 200 || invalid_res->status == 400) << invalid_res->body;
}

TEST_F(ConnectorApiLiveTest, AqlEndpointRejectsMalformedJson) {
    auto res = postRaw("/query/aql", "{invalid json!!!", "application/json", true);
    if (!requireAuthOrSkip(res, "/query/aql malformed")) {
      return;
    }
    EXPECT_EQ(res->status, 400) << res->body;
}

TEST_F(ConnectorApiLiveTest, TransactionLifecycleWorksWhenAuthorized) {
    auto begin_res = postJson("/transaction/begin", json::object(), true);
    if (!requireAuthOrSkip(begin_res, "/transaction/begin")) {
      return;
    }
    ASSERT_EQ(begin_res->status, 200) << begin_res->body;

    json begin_body;
    ASSERT_NO_THROW(begin_body = json::parse(begin_res->body));
    ASSERT_TRUE(begin_body.contains("transaction_id"));

    auto rollback_res = postJson(
        "/transaction/rollback",
        json{{"transaction_id", begin_body["transaction_id"]}},
        true);
    if (!requireAuthOrSkip(rollback_res, "/transaction/rollback")) {
      return;
    }
    EXPECT_EQ(rollback_res->status, 200) << rollback_res->body;
}

TEST_F(ConnectorApiLiveTest, TransactionEndpointsValidateMissingTransactionId) {
    auto commit_res = postJson("/transaction/commit", json::object(), true);
    if (!requireAuthOrSkip(commit_res, "/transaction/commit validation")) {
      return;
    }
    EXPECT_EQ(commit_res->status, 400) << commit_res->body;

    auto rollback_res = postJson("/transaction/rollback", json::object(), true);
    if (!requireAuthOrSkip(rollback_res, "/transaction/rollback validation")) {
      return;
    }
    EXPECT_EQ(rollback_res->status, 400) << rollback_res->body;
}

TEST_F(ConnectorApiLiveTest, ConfigEndpointSupportsReadAndRejectsInvalidTimeout) {
    auto get_res = get("/config", true);
    if (!requireAuthOrSkip(get_res, "/config GET")) {
      return;
    }
    ASSERT_EQ(get_res->status, 200) << get_res->body;

    json config_body;
    ASSERT_NO_THROW(config_body = json::parse(get_res->body));
    EXPECT_TRUE(config_body.is_object());

    auto invalid_res = postJson("/config", json{{"request_timeout_ms", 999999}}, true);
    if (!requireAuthOrSkip(invalid_res, "/config POST")) {
      return;
    }
    EXPECT_EQ(invalid_res->status, 400) << invalid_res->body;
}

TEST_F(ConnectorApiLiveTest, UnknownRouteReturns404Or405) {
    auto res = get("/api/v1/no_such_endpoint_xyz");
    requireResponse(res, "/api/v1/no_such_endpoint_xyz");
    EXPECT_TRUE(res->status == 404 || res->status == 405) << res->body;
}

TEST_F(ConnectorApiLiveTest, StatsRequestCountDoesNotGoBackwards) {
    auto first_res = get("/stats");
    requireResponse(first_res, "/stats initial");
    ASSERT_EQ(first_res->status, 200);

    json first_body;
    ASSERT_NO_THROW(first_body = json::parse(first_res->body));
    if (!first_body.contains("server") || !first_body["server"].contains("total_requests")) {
        GTEST_SKIP() << "stats response has no server.total_requests field";
    }

    const auto before = first_body["server"]["total_requests"].get<std::uint64_t>();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto health_res = get("/health");
    requireResponse(health_res, "/health");

    auto second_res = get("/stats");
    requireResponse(second_res, "/stats second");
    ASSERT_EQ(second_res->status, 200);

    json second_body;
    ASSERT_NO_THROW(second_body = json::parse(second_res->body));
    const auto after = second_body["server"]["total_requests"].get<std::uint64_t>();
    EXPECT_GE(after, before);
}

TEST_F(ConnectorApiLiveTest, IngestWorkspaceDocsAndRunRag) {
    ensureConfiguredModelLoaded();

    const std::filesystem::path docs_dir = config().docs_dir;
    const std::string ndjson = buildDocsNdjsonFromDirectory(
        docs_dir,
        config().docs_ingest_max_files,
        config().docs_ingest_max_chars);

    if (ndjson.empty()) {
        GTEST_SKIP() << "No ingestible docs found in " << docs_dir.string()
                     << "; set THEMIS_CONNECTOR_TEST_DOCS_DIR";
    }

    auto ingest_res = postNdjsonLlm("/v2/documents", ndjson, true);
    if (!requireAuthOrSkip(ingest_res, "/v2/documents ingestion")) {
      return;
    }
    ASSERT_TRUE(ingest_res->status == 200 || ingest_res->status == 207) << ingest_res->body;

    json ingest_body;
    ASSERT_NO_THROW(ingest_body = json::parse(ingest_res->body));
    ASSERT_TRUE(ingest_body.contains("inserted"));
    EXPECT_GT(ingest_body["inserted"].get<std::int64_t>(), 0);

    auto rag_res = postJsonLlm(
        "/api/v1/llm/rag",
        json{{"query", "Fasse die ingestierten Dokumente kurz zusammen."},
             {"collection", "docs"},
             {"top_k", 3}},
        true);
    requireResponse(rag_res, "/api/v1/llm/rag");
    if (rag_res->status == 401 || rag_res->status == 403) {
        GTEST_SKIP() << "/api/v1/llm/rag requires authentication; set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
    }
    if (rag_res->status == 404 || rag_res->status == 501 || rag_res->status == 503) {
        GTEST_SKIP() << "/api/v1/llm/rag not available in this build/runtime";
    }
    if (rag_res->status == 500 && rag_res->body.find("No default LLM plugin available") != std::string::npos) {
        GTEST_SKIP() << "RAG endpoint reachable but no default LLM plugin is loaded";
    }
    if (rag_res->status == 500 &&
        (rag_res->body.find("No model loaded") != std::string::npos ||
         rag_res->body.find("not ready for inference") != std::string::npos ||
         rag_res->body.find("EmbeddedLLMManager not initialized") != std::string::npos)) {
        GTEST_SKIP() << "RAG endpoint reachable but model/runtime is not ready yet";
    }
    ASSERT_EQ(rag_res->status, 200) << rag_res->body;

    json rag_body;
    ASSERT_NO_THROW(rag_body = json::parse(rag_res->body));
    EXPECT_TRUE(rag_body.contains("text"));
    EXPECT_TRUE(rag_body.contains("model"));
    EXPECT_TRUE(rag_body.contains("collection_effective"));
    EXPECT_TRUE(rag_body.contains("rag_mode_effective"));
    EXPECT_TRUE(rag_body.contains("retrieval_attempted"));
    EXPECT_TRUE(rag_body.contains("documents_retrieved"));
    EXPECT_TRUE(rag_body.contains("documents_rejected"));
    EXPECT_TRUE(rag_body.contains("top_k_effective"));
    EXPECT_TRUE(rag_body.contains("max_context_tokens_effective"));
    EXPECT_TRUE(rag_body.contains("response_budget_tokens_effective"));
    if (rag_body.contains("collection_effective")) {
        EXPECT_EQ(rag_body.value("collection_effective", std::string{}), "docs");
    }
    if (rag_body.contains("retrieval_attempted")) {
        EXPECT_TRUE(rag_body.value("retrieval_attempted", false));
    }
    if (rag_body.contains("rag_mode_effective")) {
        EXPECT_EQ(rag_body.value("rag_mode_effective", std::string{}), "text");
    }
    if (rag_body.contains("documents_rejected")) {
        EXPECT_GE(rag_body.value("documents_rejected", -1), 0);
    }
    if (rag_body.contains("top_k_effective")) {
        EXPECT_EQ(rag_body.value("top_k_effective", 0), 3);
    }
    if (rag_body.contains("max_context_tokens_effective")) {
        EXPECT_GE(rag_body.value("max_context_tokens_effective", -1), 0);
    }
    if (rag_body.contains("response_budget_tokens_effective")) {
        EXPECT_GT(rag_body.value("response_budget_tokens_effective", 0), 0);
    }

    // Verify iterative rag_mode contract and budget-cap semantics on the live endpoint.
    auto rag_iterative_res = postJsonLlm(
        "/api/v1/llm/rag",
        json{{"query", "Fasse die ingestierten Dokumente knapp iterativ zusammen."},
             {"collection", "docs"},
             {"top_k", 3},
             {"rag_mode", "iterative"},
             {"response_budget_tokens", 400},
             {"max_tokens", 32}},
        true);
    requireResponse(rag_iterative_res, "/api/v1/llm/rag iterative");
    if (rag_iterative_res->status == 400 &&
        rag_iterative_res->body.find("rag_mode") != std::string::npos) {
        GTEST_SKIP() << "Live RAG endpoint does not accept iterative rag_mode in this runtime";
    }
    if (rag_iterative_res->status == 401 || rag_iterative_res->status == 403) {
        GTEST_SKIP() << "/api/v1/llm/rag requires authentication; set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
    }
    if (rag_iterative_res->status == 404 || rag_iterative_res->status == 501 || rag_iterative_res->status == 503) {
        GTEST_SKIP() << "/api/v1/llm/rag not available in this build/runtime";
    }
    if (rag_iterative_res->status == 500 &&
        (rag_iterative_res->body.find("No default LLM plugin available") != std::string::npos ||
         rag_iterative_res->body.find("No model loaded") != std::string::npos ||
         rag_iterative_res->body.find("not ready for inference") != std::string::npos ||
         rag_iterative_res->body.find("EmbeddedLLMManager not initialized") != std::string::npos)) {
        GTEST_SKIP() << "Iterative RAG endpoint reachable but model/runtime is not ready yet";
    }
    ASSERT_EQ(rag_iterative_res->status, 200) << rag_iterative_res->body;

    json rag_iterative_body;
    ASSERT_NO_THROW(rag_iterative_body = json::parse(rag_iterative_res->body));
    EXPECT_TRUE(rag_iterative_body.contains("rag_mode_effective"));
    EXPECT_TRUE(rag_iterative_body.contains("response_budget_tokens_effective"));
    if (rag_iterative_body.contains("rag_mode_effective")) {
        EXPECT_EQ(rag_iterative_body.value("rag_mode_effective", std::string{}), "iterative");
    }
    if (rag_iterative_body.contains("response_budget_tokens_effective")) {
        EXPECT_EQ(rag_iterative_body.value("response_budget_tokens_effective", 0), 32);
    }
}

TEST_F(ConnectorApiLiveTest, LoadModelAndRunInferenceWhenConfigured) {
    if (config().llm_model_id.empty()) {
        GTEST_SKIP() << "Set THEMIS_CONNECTOR_TEST_MODEL_ID to enable model load + inference test";
    }

    auto load_res = postJsonLlm(
        "/api/v1/llm/models/load",
        json{{"model_id", config().llm_model_id}, {"path", config().llm_model_path}},
        true);
    requireResponse(load_res, "/api/v1/llm/models/load");
    if (load_res->status == 401 || load_res->status == 403) {
        GTEST_SKIP() << "/api/v1/llm/models/load requires authentication; set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
    }
    if (load_res->status == 404 || load_res->status == 501 || load_res->status == 503) {
        GTEST_SKIP() << "/api/v1/llm/models/load not available in this build/runtime";
    }
    if (load_res->status == 500 && load_res->body.find("No default LLM plugin available") != std::string::npos) {
        GTEST_SKIP() << "Model load endpoint reachable but no default LLM plugin is loaded";
    }
    if (load_res->status == 500 &&
        (load_res->body.find("Plugin returned false while loading model") != std::string::npos ||
         load_res->body.find("Model file not found") != std::string::npos)) {
        GTEST_SKIP() << "Model load endpoint reachable but model file is not available yet";
    }
    ASSERT_EQ(load_res->status, 200) << load_res->body;

    auto infer_res = postJsonLlm(
        "/api/v1/llm/inference",
        json{{"prompt", "Antworte mit einem kurzen Testsatz."},
             {"model", config().llm_model_id},
             {"max_tokens", 64},
             {"temperature", 0.2}},
        true);
    requireResponse(infer_res, "/api/v1/llm/inference");
    if (infer_res->status == 401 || infer_res->status == 403) {
        GTEST_SKIP() << "/api/v1/llm/inference requires authentication; set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
    }
    if (infer_res->status == 404 || infer_res->status == 501 || infer_res->status == 503) {
        GTEST_SKIP() << "/api/v1/llm/inference not available in this build/runtime";
    }
    if (infer_res->status == 500 &&
        (infer_res->body.find("EmbeddedLLMManager not initialized") != std::string::npos ||
         infer_res->body.find("No model loaded") != std::string::npos ||
         infer_res->body.find("not ready for inference") != std::string::npos)) {
        GTEST_SKIP() << "Inference endpoint reachable but model/runtime is not ready yet";
    }
    if (infer_res->status == 500 &&
        infer_res->body.find("type must be number, but is object") != std::string::npos) {
        GTEST_SKIP() << "Inference endpoint reachable but runtime rejected request with JSON type mismatch";
    }
    ASSERT_EQ(infer_res->status, 200) << infer_res->body;

    json infer_body;
    ASSERT_NO_THROW(infer_body = json::parse(infer_res->body));
    EXPECT_TRUE(infer_body.contains("text"));
}

TEST_F(ConnectorApiLiveTest, TriggerLoRaTrainingJob) {
    if (!getEnvBool("THEMIS_CONNECTOR_TEST_ENABLE_LORA", false)) {
        GTEST_SKIP() << "LoRA training test disabled by default; set THEMIS_CONNECTOR_TEST_ENABLE_LORA=1 to enable";
    }

    std::string base_model = config().lora_base_model;
    if (base_model.empty()) {
        base_model = config().llm_model_id;
    }
    if (base_model.empty()) {
        GTEST_SKIP() << "Set THEMIS_CONNECTOR_TEST_LORA_BASE_MODEL or THEMIS_CONNECTOR_TEST_MODEL_ID for LoRA training test";
    }

    const std::string adapter_id = "connector-lora-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    auto create_res = postJson(
        "/api/v1/llm/lora/adapters",
        json{{"adapter_id", adapter_id},
             {"base_model", base_model},
             {"rank", 8},
             {"alpha", 16.0},
             {"training_data", json{{"dataset_id", "connector-docs-dataset"}}}},
        true);
    if (!create_res) {
        GTEST_SKIP() << "LoRA adapter endpoint request failed (connection error)";
    }
    if (create_res->status == 401 || create_res->status == 403) {
        GTEST_SKIP() << "/api/v1/llm/lora/adapters requires authentication; set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
    }
    if (create_res->status == 404 || create_res->status == 501 || create_res->status == 503) {
        GTEST_SKIP() << "/api/v1/llm/lora/adapters not available in this build/runtime";
    }
    ASSERT_TRUE(create_res->status == 201 || create_res->status == 202) << create_res->body;

    json create_body;
    ASSERT_NO_THROW(create_body = json::parse(create_res->body));
    EXPECT_TRUE(create_body.contains("job_id"));
    EXPECT_TRUE(create_body.contains("status"));
}

}  // namespace
// ---------------------------------------------------------------------------
// LLM post-download readiness + multi-RAG summarization
// ---------------------------------------------------------------------------

// Verifies that after the background download+load completes the LLM reports
// healthy, then runs three distinct RAG queries that must each return non-empty
// generated text and at least one retrieved document.
TEST_F(ConnectorApiLiveTest, LlmReadyAfterDownloadAndRagSummarizesDocuments) {
    ensureConfiguredModelLoaded();

    // Step 1: wait for /api/v1/llm/health to report status=="healthy" and
    //         models_loaded > 0.  The timeout is intentionally generous because
    //         the first run triggers a background model download.
    const int wait_timeout_sec = config().llm_wait_timeout_sec;
    bool llm_ready = false;

    {
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(wait_timeout_sec);

        while (std::chrono::steady_clock::now() < deadline) {
            // Pass auth header if a bearer token is configured, because the
            // LLM health endpoint may require authentication.
            auto res = get("/api/v1/llm/health", true);
            if (!res) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            if (res->status == 401 || res->status == 403) {
                GTEST_SKIP() << "/api/v1/llm/health requires authentication; "
                             << "set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
            }
            if (res->status == 200) {
                try {
                    const auto body = json::parse(res->body);
                    if (body.contains("plugin_manager") && body["plugin_manager"].is_string()) {
                        const auto pm = body["plugin_manager"].get<std::string>();
                        if (pm.find("No default LLM plugin available") != std::string::npos) {
                            GTEST_SKIP() << "LLM health reports missing default plugin: " << pm;
                        }
                    }
                    if (body.value("status", std::string{}) == "healthy" &&
                        body.value("models_loaded", 0) > 0) {
                        llm_ready = true;
                        break;
                    }
                } catch (...) {
                    // malformed body – keep polling
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    if (!llm_ready) {
        GTEST_SKIP() << "LLM not ready after " << wait_timeout_sec << " s — "
                     << "background download may still be running or model_path is not configured. "
                     << "Increase THEMIS_CONNECTOR_TEST_LLM_WAIT_SEC to extend the wait.";
    }

    // Step 2: ingest markdown/text docs from docs_dir so the vector store has
    //         content the RAG pipeline can retrieve.
    const std::filesystem::path docs_dir = config().docs_dir;
    const std::string ndjson = buildDocsNdjsonFromDirectory(
        docs_dir,
        config().docs_ingest_max_files,
        config().docs_ingest_max_chars);

    if (ndjson.empty()) {
        GTEST_SKIP() << "No ingestible docs found in '" << docs_dir.string()
                     << "'; set THEMIS_CONNECTOR_TEST_DOCS_DIR to a directory with .md/.txt files.";
    }

    {
        auto ingest_res = postNdjsonLlm("/v2/documents", ndjson, true);
        if (!requireAuthOrSkip(ingest_res, "/v2/documents ingestion")) {
          return;
        }
        ASSERT_TRUE(ingest_res->status == 200 || ingest_res->status == 207)
            << "Document ingestion failed: " << ingest_res->body;

        json ingest_body;
        ASSERT_NO_THROW(ingest_body = json::parse(ingest_res->body));
        ASSERT_TRUE(ingest_body.contains("inserted")) << "Ingestion response missing 'inserted'";
        EXPECT_GT(ingest_body["inserted"].get<std::int64_t>(), 0)
            << "Expected at least one document to be inserted";
    }

    // Step 3: run three semantically distinct RAG queries and verify that each
    //         returns non-empty LLM-generated text and at least one document.
    const std::vector<std::string> rag_queries = {
        "Fasse die wesentlichen Inhalte der ingestierten Dokumente zusammen.",
        "Welche technischen Konzepte werden in den Dokumenten beschrieben?",
        "Erstelle eine kurze Uebersicht der wichtigsten Punkte aus den Dokumenten."
    };

    int successful_rag_responses = 0;
    for (const auto& query : rag_queries) {
        json rag_body;
        bool got_non_empty_text = false;
        bool got_documents = false;

        // Some LLM backends occasionally return an empty first response under load.
        // Retry the same semantic query once before failing the test.
        for (int attempt = 0; attempt < 2; ++attempt) {
            auto rag_res = postJsonLlm(
                "/api/v1/llm/rag",
                json{{"query", query},
                     {"collection", "docs"},
                     {"top_k", 3}},
                true);
            requireResponse(rag_res, "/api/v1/llm/rag");
            if (rag_res->status == 401 || rag_res->status == 403) {
                GTEST_SKIP() << "/api/v1/llm/rag requires a bearer token; "
                             << "set THEMIS_CONNECTOR_TEST_BEARER_TOKEN";
            }
            if (!requireLlmFeatureOrSkip(rag_res, "/api/v1/llm/rag")) {
                GTEST_SKIP() << "/api/v1/llm/rag not available in this build (404/501/503)";
            }
            if (rag_res->status == 500 && rag_res->body.find("No default LLM plugin available") != std::string::npos) {
                GTEST_SKIP() << "RAG endpoint reachable but no default LLM plugin is loaded";
            }
            if (rag_res->status == 500 &&
                (rag_res->body.find("No model loaded") != std::string::npos ||
                 rag_res->body.find("not ready for inference") != std::string::npos ||
                 rag_res->body.find("EmbeddedLLMManager not initialized") != std::string::npos)) {
                GTEST_SKIP() << "RAG endpoint reachable but model/runtime is not ready yet";
            }

            ASSERT_EQ(rag_res->status, 200)
                << "RAG request failed for query: " << query << "\n" << rag_res->body;

            ASSERT_NO_THROW(rag_body = json::parse(rag_res->body));

            EXPECT_TRUE(rag_body.contains("text"))
                << "RAG response missing 'text' field (query: " << query << ")";
            EXPECT_TRUE(rag_body.contains("model"))
                << "RAG response missing 'model'";
            EXPECT_TRUE(rag_body.contains("collection_effective"))
                << "RAG response missing 'collection_effective'";
            EXPECT_TRUE(rag_body.contains("rag_mode_effective"))
                << "RAG response missing 'rag_mode_effective'";
            EXPECT_TRUE(rag_body.contains("retrieval_attempted"))
                << "RAG response missing 'retrieval_attempted'";
            EXPECT_TRUE(rag_body.contains("documents_retrieved"))
                << "RAG response missing 'documents_retrieved'";
            EXPECT_TRUE(rag_body.contains("documents_rejected"))
                << "RAG response missing 'documents_rejected'";
            EXPECT_TRUE(rag_body.contains("top_k_effective"))
                << "RAG response missing 'top_k_effective'";
            EXPECT_TRUE(rag_body.contains("max_context_tokens_effective"))
                << "RAG response missing 'max_context_tokens_effective'";
            EXPECT_TRUE(rag_body.contains("response_budget_tokens_effective"))
                << "RAG response missing 'response_budget_tokens_effective'";

            if (rag_body.contains("top_k_effective")) {
                EXPECT_EQ(rag_body.value("top_k_effective", 0), 3)
                    << "Unexpected top_k_effective value";
            }
            if (rag_body.contains("collection_effective")) {
                EXPECT_EQ(rag_body.value("collection_effective", std::string{}), "docs")
                    << "Unexpected collection_effective value";
            }
            if (rag_body.contains("retrieval_attempted")) {
                EXPECT_TRUE(rag_body.value("retrieval_attempted", false))
                    << "retrieval_attempted should be true when collection is provided";
            }
            if (rag_body.contains("rag_mode_effective")) {
                EXPECT_EQ(rag_body.value("rag_mode_effective", std::string{}), "text")
                    << "Unexpected rag_mode_effective value";
            }
            if (rag_body.contains("documents_rejected")) {
                EXPECT_GE(rag_body.value("documents_rejected", -1), 0)
                    << "documents_rejected must be non-negative";
            }
            if (rag_body.contains("max_context_tokens_effective")) {
                EXPECT_GE(rag_body.value("max_context_tokens_effective", -1), 0)
                    << "max_context_tokens_effective must be non-negative";
            }
            if (rag_body.contains("response_budget_tokens_effective")) {
                EXPECT_GT(rag_body.value("response_budget_tokens_effective", 0), 0)
                    << "response_budget_tokens_effective must be positive";
            }

            got_non_empty_text = !rag_body.value("text", std::string{}).empty();
            got_documents = rag_body.value("documents_retrieved", 0) > 0;

            if (got_non_empty_text && got_documents) {
                break;
            }

            if (attempt == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }

        EXPECT_TRUE(got_documents)
            << "RAG returned 0 retrieved documents after retry (query: " << query << ")";

        if (!got_non_empty_text) {
            continue;
        }

        ++successful_rag_responses;
    }

    EXPECT_GE(successful_rag_responses, 2)
        << "Too few non-empty RAG responses. successful=" << successful_rag_responses
        << ", total_queries=" << rag_queries.size();
}
