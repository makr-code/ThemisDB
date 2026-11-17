#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "index/rotary_embeddings.h"
#include "index/vector_index.h"
#include "server/rope_api_handler.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace {

TEST(RopeSearchRequestContext, ExpiredRequestReturnsGatewayTimeoutWithoutResults) {
    const auto suffix =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const auto db_path = std::filesystem::temp_directory_path() /
                         ("themis_rope_search_cancel_" + std::to_string(suffix));

    themis::RocksDBWrapper::Config storage_config;
    storage_config.db_path = db_path.string();
    auto storage = std::make_shared<themis::RocksDBWrapper>(storage_config);
    ASSERT_TRUE(storage->open());

    auto vector_index = std::make_shared<themis::VectorIndexManager>(*storage);
    ASSERT_TRUE(vector_index->init(
        "rope_search", 128, themis::VectorIndexManager::Metric::COSINE).ok);
    themis::RotationConfig rotation;
    rotation.hidden_dim = 128;
    rotation.num_rotation_pairs = 64;
    rotation.computeThetaCache();
    ASSERT_TRUE(vector_index->setRotaryEmbeddingConfig(rotation).ok);

    themis::server::RopeApiHandler handler(storage, vector_index, nullptr);
    boost::beast::http::request<boost::beast::http::string_body> request{
        boost::beast::http::verb::post,
        "/api/v1/vector-index/rope_search/rope/search",
        11};
    nlohmann::json query = nlohmann::json::array();
    for (int i = 0; i < 128; ++i) {
        query.push_back(1.0);
    }
    request.body() = nlohmann::json{
        {"query", query}, {"position", 0}, {"k", 1}}.dump();

    const auto response = handler.handleSearchPost(
        request, themis::query::RequestContext::withTimeout(
                    std::chrono::milliseconds{0}));

    EXPECT_EQ(response.result(), boost::beast::http::status::gateway_timeout);
    const auto response_json = nlohmann::json::parse(response.body());
    EXPECT_FALSE(response_json.contains("results"));

    auto oversized_request = request;
    oversized_request.body().assign(2'000'001, ' ');
    const auto oversized_response = handler.handleSearchPost(
        oversized_request, themis::query::RequestContext{});
    EXPECT_EQ(oversized_response.result(),
              boost::beast::http::status::payload_too_large);

    vector_index.reset();
    storage->close();
    std::error_code error;
    std::filesystem::remove_all(db_path, error);
}

}  // namespace
