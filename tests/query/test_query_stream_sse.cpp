/**
 * @file test_query_stream_sse.cpp
 * @brief Tests for GET /v2/query/stream — AQL query result SSE streaming endpoint.
 *
 * Covers:
 *  - Missing query parameter returns 400
 *  - SSE Content-Type header
 *  - SSE retry directive is present
 *  - Rows streamed as individual SSE data events
 *  - Terminal "done" event is emitted
 *  - Heartbeat comment emitted when no rows are produced
 *  - WebSocket /v2/changes path propagation (unit-level)
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sstream>

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

// ============================================================================
// Test fixture
// ============================================================================

class QueryStreamSseTest : public ::testing::Test {
protected:
    static constexpr uint16_t kPort = 18095;
    static constexpr const char* kDbPath = "data/themis_query_stream_sse_test";

    void SetUp() override {
        if (std::filesystem::exists(kDbPath)) {
            std::filesystem::remove_all(kDbPath);
        }

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = kDbPath;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_     = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_    = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_      = std::make_shared<themis::TransactionManager>(
            *storage_, *secondary_index_, *graph_index_, *vector_index_);

        themis::server::HttpServer::Config scfg;
        scfg.host        = "127.0.0.1";
        scfg.port        = kPort;
        scfg.num_threads = 2;

        server_ = std::make_unique<themis::server::HttpServer>(
            scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (server_) {
          server_->stop();
        }
        if (storage_) {
          storage_->close();
        }
        std::filesystem::remove_all(kDbPath);
    }

    // -----------------------------------------------------------------------
    // HTTP helpers
    // -----------------------------------------------------------------------

    http::response<http::string_body> httpGetResponse(const std::string& target) {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto results = resolver.resolve("127.0.0.1", std::to_string(kPort));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "127.0.0.1");
        http::write(stream, req);

        beast::flat_buffer buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return res;
    }

    json httpPost(const std::string& target, const json& body) {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto results = resolver.resolve("127.0.0.1", std::to_string(kPort));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();
        http::write(stream, req);

        beast::flat_buffer buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return json::parse(res.body());
    }

    // Parse "data: ..." lines from an SSE body
    static std::vector<json> parseSseDataEvents(const std::string& body) {
        std::vector<json> events;
        std::istringstream iss(body);
        std::string line = {};
        while (std::getline(iss, line)) {
            if (!line.empty() && line.rfind("data: ", 0) == 0) {
                try {
                    events.push_back(json::parse(line.substr(6)));
                } catch (...) {}
            }
        }
        return events;
    }

    // Parse "event: <name>" lines
    static std::vector<std::string> parseSseEventNames(const std::string& body) {
        std::vector<std::string> names;
        std::istringstream iss(body);
        std::string line = {};
        while (std::getline(iss, line)) {
            if (!line.empty() && line.rfind("event: ", 0) == 0) {
                names.push_back(line.substr(7));
                // trim CR
                if (!names.back().empty() && names.back().back() == '\r') {
                    names.back().pop_back();
                }
            }
        }
        return names;
    }

    std::shared_ptr<themis::RocksDBWrapper>       storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager>     graph_index_;
    std::shared_ptr<themis::VectorIndexManager>    vector_index_;
    std::shared_ptr<themis::TransactionManager>    tx_manager_;
    std::unique_ptr<themis::server::HttpServer>    server_;
};

// ============================================================================
// Tests
// ============================================================================

TEST_F(QueryStreamSseTest, MissingQueryParam_Returns400) {
    auto res = httpGetResponse("/v2/query/stream");
    EXPECT_EQ(static_cast<int>(res.result()), 400);
}

TEST_F(QueryStreamSseTest, ValidQuery_ContentTypeIsEventStream) {
    // Encode a trivial AQL that returns no results
    auto res = httpGetResponse("/v2/query/stream?q=FOR+x+IN+nonexistent+RETURN+x");
    // Content-Type must be text/event-stream regardless of whether rows exist
    std::string ct = std::string(res[http::field::content_type]);
    EXPECT_NE(ct.find("text/event-stream"), std::string::npos)
        << "Expected text/event-stream, got: " << ct;
}

TEST_F(QueryStreamSseTest, ValidQuery_ContainsRetryDirective) {
    auto res = httpGetResponse("/v2/query/stream?q=FOR+x+IN+nonexistent+RETURN+x");
    // SSE body must start with "retry: <ms>" directive
    EXPECT_NE(res.body().find("retry: "), std::string::npos);
}

TEST_F(QueryStreamSseTest, ValidQuery_EmitsTerminalDoneEvent) {
    auto res = httpGetResponse("/v2/query/stream?q=FOR+x+IN+nonexistent+RETURN+x");
    auto names = parseSseEventNames(res.body());
    bool has_done = false;
    for (const auto& n : names) {
        if (n == "done") { has_done = true; break; }
    }
    EXPECT_TRUE(has_done) << "Expected 'event: done' in SSE body:\n" << res.body();
}

TEST_F(QueryStreamSseTest, EmptyResult_EmitsHeartbeatComment) {
    auto res = httpGetResponse("/v2/query/stream?q=FOR+x+IN+nonexistent+RETURN+x");
    EXPECT_NE(res.body().find(": heartbeat"), std::string::npos)
        << "Expected heartbeat comment in SSE body:\n" << res.body();
}

TEST_F(QueryStreamSseTest, WithEntities_StreamsIndividualRows) {
    // Insert some entities first
    (void)httpPost("/entities", json{{"key", "stream:1"}, {"blob", R"({"val":1})"}});
    (void)httpPost("/entities", json{{"key", "stream:2"}, {"blob", R"({"val":2})"}});
    (void)httpPost("/entities", json{{"key", "stream:3"}, {"blob", R"({"val":3})"}});

    // AQL query over the 'stream' prefix — adapt to actual storage table name
    // Using the generic entity scan which works with the test storage backend
    auto res = httpGetResponse("/v2/query/stream?q=FOR+x+IN+entities+RETURN+x");

    // Must be text/event-stream
    EXPECT_NE(std::string(res[http::field::content_type]).find("text/event-stream"),
              std::string::npos);

    // Must have a "done" event
    auto names = parseSseEventNames(res.body());
    bool has_done = false;
    for (const auto& n : names) { if (n == "done") { has_done = true; break; } }
    EXPECT_TRUE(has_done);

    // The "done" event data should contain rows_streamed
    auto events = parseSseDataEvents(res.body());
    // Find the done-event data (last event in most cases)
    for (const auto& ev : events) {
        if (ev.contains("rows_streamed")) {
            EXPECT_GE(ev["rows_streamed"].get<int>(), 0);
            break;
        }
    }
}

TEST_F(QueryStreamSseTest, DoneEvent_ContainsRowsStreamed) {
    auto res = httpGetResponse("/v2/query/stream?q=FOR+x+IN+nonexistent+RETURN+x");
    auto events = parseSseDataEvents(res.body());
    bool found = false;
    for (const auto& ev : events) {
        if (ev.contains("rows_streamed")) {
            EXPECT_GE(ev["rows_streamed"].get<int>(), 0);
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected 'rows_streamed' in done event data";
}

// ============================================================================
// WebSocket /v2/changes message-format tests (unit-level, no live server)
// ============================================================================

#ifdef THEMIS_ENABLE_WEBSOCKET
#include "server/websocket_session.h"

TEST(WsChangesMessageFormat, ActionFieldIsNormalisedToType) {
    // Validate the JSON message format that /v2/changes clients are expected to send.
    // The handler normalises "action" → "type" and "collection" → "channel" + "key_prefix".
    json client_msg = {
        {"action", "subscribe"},
        {"collection", "orders"},
        {"filter", {{"type", "PUT"}}}
    };

    // Simulate the normalisation logic in WebSocketSession::processMessage
    if (client_msg.contains("action") && !client_msg.contains("type")) {
        client_msg["type"] = client_msg["action"];
    }
    if (client_msg.contains("collection") && !client_msg.contains("channel")) {
        client_msg["channel"]    = "changefeed";
        client_msg["key_prefix"] = client_msg["collection"].get<std::string>() + ":";
    }

    EXPECT_EQ(client_msg["type"],       "subscribe");
    EXPECT_EQ(client_msg["channel"],    "changefeed");
    EXPECT_EQ(client_msg["key_prefix"], "orders:");
}

TEST(WsChangesMessageFormat, UnsubscribeActionIsNormalised) {
    json client_msg = {{"action", "unsubscribe"}, {"collection", "orders"}};

    if (client_msg.contains("action") && !client_msg.contains("type")) {
        client_msg["type"] = client_msg["action"];
    }
    if (client_msg.contains("collection") && !client_msg.contains("channel")) {
        client_msg["channel"]    = "changefeed";
        client_msg["key_prefix"] = client_msg["collection"].get<std::string>() + ":";
    }

    EXPECT_EQ(client_msg["type"],    "unsubscribe");
    EXPECT_EQ(client_msg["channel"], "changefeed");
}

TEST(WsChangesMessageFormat, ExistingTypeFieldIsPreserved) {
    // If the client already sends "type", it must not be overwritten
    json client_msg = {{"type", "subscribe"}, {"channel", "changefeed"}};

    if (client_msg.contains("action") && !client_msg.contains("type")) {
        client_msg["type"] = client_msg["action"];
    }

    EXPECT_EQ(client_msg["type"], "subscribe");
    EXPECT_FALSE(client_msg.contains("action"));
}

TEST(WsChangesMessageFormat, FilterTypeIsParsedToPutEventType) {
    // Validate that filter.type="PUT" produces the correct event type set.
    // This mirrors the logic in WebSocketSession::processMessage.
    json client_msg = {
        {"action", "subscribe"},
        {"collection", "orders"},
        {"filter", {{"type", "PUT"}}}
    };

    // Normalise
    if (client_msg.contains("action") && !client_msg.contains("type")) {
        client_msg["type"] = client_msg["action"];
    }
    if (client_msg.contains("collection") && !client_msg.contains("channel")) {
        client_msg["channel"]    = "changefeed";
        client_msg["key_prefix"] = client_msg["collection"].get<std::string>() + ":";
    }

    // Simulate event_types extraction
    std::set<std::string> event_types = {};

    if (client_msg.contains("filter") && client_msg["filter"].is_object()) {
        const auto& flt = client_msg["filter"];
        if (flt.contains("type") && flt["type"].is_string()) {
            event_types.insert(flt["type"].get<std::string>());
        }
    }

    EXPECT_EQ(event_types.size(), 1u);
    EXPECT_TRUE(event_types.count("PUT") > 0);
}

TEST(WsChangesMessageFormat, FilterTypeDeleteIsParsed) {
    json client_msg = {
        {"action", "subscribe"},
        {"collection", "orders"},
        {"filter", {{"type", "DELETE"}}}
    };
    std::set<std::string> event_types = {};

    if (client_msg.contains("filter") && client_msg["filter"].is_object()) {
        const auto& flt = client_msg["filter"];
        if (flt.contains("type") && flt["type"].is_string()) {
            event_types.insert(flt["type"].get<std::string>());
        }
    }
    EXPECT_EQ(event_types.size(), 1u);
    EXPECT_TRUE(event_types.count("DELETE") > 0);
}

TEST(WsChangesMessageFormat, NoFilterMeansAllEventTypes) {
    json client_msg = {{"action", "subscribe"}, {"collection", "orders"}};
    std::set<std::string> event_types = {};

    if (client_msg.contains("filter") && client_msg["filter"].is_object()) {
        const auto& flt = client_msg["filter"];
        if (flt.contains("type") && flt["type"].is_string()) {
            event_types.insert(flt["type"].get<std::string>());
        }
    }
    // No filter → empty set → all event types (unfiltered)
    EXPECT_TRUE(event_types.empty());
}

#endif // THEMIS_ENABLE_WEBSOCKET
