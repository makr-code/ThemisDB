/*
 * ThemisDB | File: test_profiling_api_handler.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>

#include "server/profiling_api_handler.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;

namespace themis::server {
namespace {

http::request<http::string_body> makeRequest(http::verb method, const std::string& target) {
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    req.prepare_payload();
    return req;
}

json parseBody(const http::response<http::string_body>& response) {
    return json::parse(response.body());
}

class ProfilingApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        query_profiler = std::make_shared<observability::QueryProfiler>();
        storage_profiler = std::make_shared<observability::StorageProfiler>();
        analyzer = std::make_shared<observability::PerformanceAnalyzer>();
        handler = std::make_unique<ProfilingApiHandler>(query_profiler, storage_profiler, analyzer);

        query_profiler->enable();
        query_profiler->start_query("q1", "FOR d IN docs RETURN d");
        query_profiler->end_query("q1");
        query_profiler->start_query("q2", "FOR d IN docs FILTER d.x == 1 RETURN d");
        query_profiler->end_query("q2");
    }

    std::shared_ptr<observability::QueryProfiler> query_profiler;
    std::shared_ptr<observability::StorageProfiler> storage_profiler;
    std::shared_ptr<observability::PerformanceAnalyzer> analyzer;
    std::unique_ptr<ProfilingApiHandler> handler;
};

TEST_F(ProfilingApiHandlerTest, QueriesIgnoresPartialLimitParameterName) {
    const auto response = handler->handle_request(
        makeRequest(http::verb::get, "/api/profiling/queries?xlimit=1"));

    ASSERT_EQ(response.result(), http::status::ok);
    const auto body = parseBody(response);
    ASSERT_TRUE(body.is_array());
    EXPECT_EQ(body.size(), 2u);
}

TEST_F(ProfilingApiHandlerTest, QueriesRejectsNegativeLimit) {
    const auto response = handler->handle_request(
        makeRequest(http::verb::get, "/api/profiling/queries?limit=-1"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["error"], "invalid limit");
}

TEST_F(ProfilingApiHandlerTest, SlowQueriesRejectsNegativeThreshold) {
    const auto response = handler->handle_request(
        makeRequest(http::verb::get, "/api/profiling/slow-queries?threshold_ms=-1"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["error"], "invalid threshold_ms");
}

TEST_F(ProfilingApiHandlerTest, SetConfigRejectsNegativeQueryThreshold) {
    json body = {
        {"query_profiler", {
            {"slow_query_threshold_ms", -1}
        }}
    };

    auto request = makeRequest(http::verb::post, "/api/profiling/config");
    request.body() = body.dump();
    request.prepare_payload();

    const auto response = handler->handle_request(request);

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_NE(parseBody(response)["error"].get<std::string>().find("slow_query_threshold_ms"), std::string::npos);
}

TEST_F(ProfilingApiHandlerTest, SetConfigRejectsNegativeStorageThreshold) {
    json body = {
        {"storage_profiler", {
            {"slow_op_threshold_ms", -1}
        }}
    };

    auto request = makeRequest(http::verb::post, "/api/profiling/config");
    request.body() = body.dump();
    request.prepare_payload();

    const auto response = handler->handle_request(request);

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_NE(parseBody(response)["error"].get<std::string>().find("slow_op_threshold_ms"), std::string::npos);
}

TEST_F(ProfilingApiHandlerTest, SetConfigRejectsOutOfRangeCacheHitThreshold) {
    json body = {
        {"analyzer", {
            {"cache_hit_rate_threshold", 1.5}
        }}
    };

    auto request = makeRequest(http::verb::post, "/api/profiling/config");
    request.body() = body.dump();
    request.prepare_payload();

    const auto response = handler->handle_request(request);

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_NE(parseBody(response)["error"].get<std::string>().find("cache_hit_rate_threshold"), std::string::npos);
}


} // namespace
} // namespace themis::server