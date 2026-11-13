#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "server/http_server.h"
#include "server/policy_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace {

using themis::PolicyEngine;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class FixedPolicyEvaluator final : public PolicyEngine::IPolicyEvaluator {
public:
    explicit FixedPolicyEvaluator(std::optional<PolicyEngine::Decision> decision)
        : decision_(std::move(decision)) {}

    std::optional<PolicyEngine::Decision> evaluate(
        const std::string&,
        const std::string&,
        const std::string&,
        const std::optional<std::string>&,
        const std::optional<std::string>&) const override {
        return decision_;
    }

private:
    std::optional<PolicyEngine::Decision> decision_;
};

struct ScopedEnv {
    std::string name;
    std::string previous_value;
    bool had_previous_value = false;

    ScopedEnv(const char* key, const std::string& value) : name(key) {
        if (const char* previous = std::getenv(key)) {
            previous_value = previous;
            had_previous_value = true;
        }
#ifdef _WIN32
        _putenv_s(key, value.c_str());
#else
        ::setenv(key, value.c_str(), 1);
#endif
    }

    ~ScopedEnv() {
#ifdef _WIN32
        _putenv_s(name.c_str(), had_previous_value ? previous_value.c_str() : "");
#else
        if (had_previous_value) {
            ::setenv(name.c_str(), previous_value.c_str(), 1);
        } else {
            ::unsetenv(name.c_str());
        }
#endif
    }
};

class RagPolicyHttpFixture {
public:
    static constexpr uint16_t kPort = 18107;
    static constexpr const char* kToken = "test-rag-policy-token";

    bool start(const std::optional<std::string>& policy_json) {
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        base_path_ = std::filesystem::temp_directory_path() /
                     ("themis_rag_policy_http_" + suffix);
        db_path_ = base_path_ / "db";
        policy_path_ = base_path_ / "policies.json";
        std::filesystem::create_directories(base_path_);

        if (policy_json.has_value()) {
            std::ofstream policy_file(policy_path_);
            policy_file << *policy_json;
            if (!policy_file) {
                return false;
            }
        }

        token_env_ = std::make_unique<ScopedEnv>("THEMIS_TOKEN_RAG", kToken);
        policy_env_ = std::make_unique<ScopedEnv>(
            "THEMIS_POLICIES_PATH", policy_path_.string());

        themis::RocksDBWrapper::Config storage_config;
        storage_config.db_path = db_path_.string();
        storage_config.memtable_size_mb = 32;
        storage_config.block_cache_size_mb = 64;
        storage_ = std::make_shared<themis::RocksDBWrapper>(storage_config);
        if (!storage_->open()) {
            return false;
        }

        secondary_index_ =
            std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
        transaction_manager_ = std::make_shared<themis::TransactionManager>(
            *storage_, *secondary_index_, *graph_index_, *vector_index_);

        themis::server::HttpServer::Config server_config;
        server_config.host = "127.0.0.1";
        server_config.port = kPort;
        server_config.num_threads = 2;
        server_ = std::make_unique<themis::server::HttpServer>(
            server_config, storage_, secondary_index_, graph_index_,
            vector_index_, transaction_manager_);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        return true;
    }

    void stop() {
        if (server_) {
            server_->stop();
            server_.reset();
        }
        if (storage_) {
            storage_->close();
            storage_.reset();
        }
        policy_env_.reset();
        token_env_.reset();
        if (!base_path_.empty()) {
            std::filesystem::remove_all(base_path_);
        }
    }

    ~RagPolicyHttpFixture() { stop(); }

private:
    std::filesystem::path base_path_;
    std::filesystem::path db_path_;
    std::filesystem::path policy_path_;
    std::unique_ptr<ScopedEnv> token_env_;
    std::unique_ptr<ScopedEnv> policy_env_;
    std::unique_ptr<themis::server::HttpServer> server_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> transaction_manager_;
};

http::response<http::string_body> postRagRequest(const std::string& collection) {
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);
    stream.connect(resolver.resolve("127.0.0.1",
                                    std::to_string(RagPolicyHttpFixture::kPort)));

    http::request<http::string_body> request{
        http::verb::post, "/api/v1/llm/rag", 11};
    request.set(http::field::host, "127.0.0.1");
    request.set(http::field::authorization,
                std::string("Bearer ") + RagPolicyHttpFixture::kToken);
    request.set(http::field::content_type, "application/json");
    request.body() = nlohmann::json{
        {"query", "test query"}, {"collection", collection}}.dump();
    request.prepare_payload();

    http::write(stream, request);
    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);
    beast::error_code error;
    stream.socket().shutdown(tcp::socket::shutdown_both, error);
    return response;
}

PolicyEngine::Policy collectionPolicy(bool allow) {
    PolicyEngine::Policy policy;
    policy.id = "rag-collection";
    policy.subjects.insert("reader");
    policy.actions.insert("data.read");
    policy.resources.emplace_back("docs");
    policy.effect_allow = allow;
    return policy;
}

TEST(RagCollectionPolicyTest, ExistingAuthorizationKeepsEmptyPolicyDefaultAllow) {
    PolicyEngine engine;

    const auto decision = engine.authorize("reader", "data.read", "docs");

    EXPECT_TRUE(decision.allowed);
    EXPECT_EQ(decision.reason, "no_policies_default_allow");
}

TEST(RagCollectionPolicyTest, StrictAuthorizationDeniesWhenPolicySetIsEmpty) {
    PolicyEngine engine;

    const auto decision = engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt,
        /*allow_when_empty=*/false);

    EXPECT_FALSE(decision.allowed);
    EXPECT_EQ(decision.reason, "no_policies_fail_closed");
}

TEST(RagCollectionPolicyTest, StrictAuthorizationUsesExplicitAllowAndDenyRules) {
    PolicyEngine allow_engine;
    allow_engine.addPolicy(collectionPolicy(true));
    const auto allow = allow_engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    EXPECT_TRUE(allow.allowed);
    EXPECT_EQ(allow.policy_id, "rag-collection");

    PolicyEngine deny_engine;
    deny_engine.addPolicy(collectionPolicy(false));
    const auto deny = deny_engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    EXPECT_FALSE(deny.allowed);
    EXPECT_EQ(deny.policy_id, "rag-collection");
}

TEST(RagCollectionPolicyTest, ExactCollectionGrantDoesNotMatchPrefixSibling) {
    PolicyEngine engine;
    engine.addPolicy(collectionPolicy(true));

    const auto exact_collection = engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    const auto prefix_sibling = engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs-private", std::nullopt, std::nullopt,
        false, true);

    EXPECT_TRUE(exact_collection.allowed);
    EXPECT_FALSE(prefix_sibling.allowed);
    EXPECT_EQ(prefix_sibling.reason, "no_matching_policy");
}

TEST(RagCollectionPolicyTest, ExactResourceModeRequiresAListedCollectionOrWildcard) {
    PolicyEngine empty_resource_engine;
    auto broad_policy = collectionPolicy(true);
    broad_policy.resources.clear();
    empty_resource_engine.addPolicy(broad_policy);

    const auto empty_resource = empty_resource_engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    EXPECT_FALSE(empty_resource.allowed);

    PolicyEngine wildcard_engine;
    auto wildcard_policy = collectionPolicy(true);
    wildcard_policy.resources = {"*"};
    wildcard_engine.addPolicy(wildcard_policy);
    const auto wildcard = wildcard_engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs-private", std::nullopt, std::nullopt,
        false, true);
    EXPECT_TRUE(wildcard.allowed);
}

TEST(RagCollectionPolicyTest, StrictAuthorizationHonorsOPAAndFailsClosedOnFallback) {
    PolicyEngine opa_allow_engine;
    FixedPolicyEvaluator opa_allow(PolicyEngine::Decision{
        true, "opa-rag-allow", "matched_opa_policy"});
    opa_allow_engine.setOpaEvaluator(&opa_allow);
    const auto opa_allow_decision = opa_allow_engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    EXPECT_TRUE(opa_allow_decision.allowed);
    EXPECT_EQ(opa_allow_decision.policy_id, "opa-rag-allow");

    PolicyEngine fallback_engine;
    FixedPolicyEvaluator unavailable(std::nullopt);
    fallback_engine.setOpaEvaluator(&unavailable);
    const auto fallback_decision = fallback_engine.authorizeWithEmptyPolicyDefault(
        "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    EXPECT_FALSE(fallback_decision.allowed);
    EXPECT_EQ(fallback_decision.reason, "no_policies_fail_closed");

    PolicyEngine local_fallback_engine;
    local_fallback_engine.addPolicy(collectionPolicy(true));
    local_fallback_engine.setOpaEvaluator(&unavailable);
    const auto local_fallback_decision =
        local_fallback_engine.authorizeWithEmptyPolicyDefault(
            "reader", "data.read", "docs", std::nullopt, std::nullopt, false, true);
    EXPECT_TRUE(local_fallback_decision.allowed);
    EXPECT_EQ(local_fallback_decision.policy_id, "rag-collection");
}

TEST(RagCollectionPolicyHttpTest, EmptyPolicySetDeniesAuthenticatedRagRequest) {
    RagPolicyHttpFixture fixture;
    ASSERT_TRUE(fixture.start(std::nullopt));

    const auto response = postRagRequest("docs");

    EXPECT_EQ(response.result(), http::status::forbidden);
    const auto body = nlohmann::json::parse(response.body());
    EXPECT_EQ(body.value("error", ""), "policy_denied");
    EXPECT_EQ(body.value("message", ""), "no_policies_fail_closed");
}

TEST(RagCollectionPolicyHttpTest, NativePolicyMatchesTheExactCollection) {
    const std::string policies = R"json({
        "policies": [
            {
                "id": "rag-route",
                "subjects": ["rag_reader"],
                "actions": ["llm"],
                "resources": ["/api/v1/llm/"],
                "effect": "allow"
            },
            {
                "id": "rag-docs",
                "subjects": ["rag_reader"],
                "actions": ["data.read"],
                "resources": ["docs"],
                "effect": "allow"
            }
        ]
    })json";
    RagPolicyHttpFixture fixture;
    ASSERT_TRUE(fixture.start(policies));

    const auto prefix_sibling = postRagRequest("docs-private");
    EXPECT_EQ(prefix_sibling.result(), http::status::forbidden);
    const auto denied_body = nlohmann::json::parse(prefix_sibling.body());
    EXPECT_EQ(denied_body.value("message", ""), "no_matching_policy");
}

} // namespace
