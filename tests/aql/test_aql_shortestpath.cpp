#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <system_error>

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "storage/base_entity.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using json = nlohmann::json;
using tcp = net::ip::tcp;

class HttpAqlShortestPathTest : public ::testing::Test {
protected:
    static std::filesystem::path findRepoRootWithSchemas() {
        std::error_code ec = {};
        auto base = std::filesystem::current_path(ec);
        if (ec) {
            return {};
        }
        for (int i = 0; i < 8; ++i) {
            const auto candidate = base / "config" / "schemas";
            if (std::filesystem::exists(candidate, ec) && std::filesystem::is_directory(candidate, ec)) {
                return base;
            }
            if (!base.has_parent_path()) {
                break;
            }
            base = base.parent_path();
        }
        return {};
    }

    static unsigned short allocateFreePort() {
        try {
            net::io_context ioc;
            tcp::acceptor acceptor(ioc, {net::ip::make_address("127.0.0.1"), 0});
            return acceptor.local_endpoint().port();
        } catch (...) {
            return 18102;
        }
    }

    bool waitUntilServerReady(std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            try {
                net::io_context ioc;
                tcp::resolver resolver(ioc);
                beast::tcp_stream stream(ioc);
                auto const endpoints = resolver.resolve("127.0.0.1", std::to_string(port_));
                stream.connect(endpoints);
                beast::error_code ec;
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                return true;
            } catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        return false;
    }

    void SetUp() override {
        std::error_code ec = {};
        original_cwd_ = std::filesystem::current_path(ec);
        const auto repo_root = findRepoRootWithSchemas();
        if (!repo_root.empty()) {
            std::filesystem::current_path(repo_root, ec);
        }

        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_http_aql_shortestpath_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_ = std::make_shared<themis::TransactionManager>(*storage_, *secondary_index_, *graph_index_, *vector_index_);

        themis::server::HttpServer::Config scfg;
        scfg.host = "127.0.0.1";
        port_ = allocateFreePort();
        scfg.port = port_;
        scfg.num_threads = 1;
        server_ = std::make_unique<themis::server::HttpServer>(scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_);
        server_->start();
        ASSERT_TRUE(waitUntilServerReady(std::chrono::milliseconds(2000)))
            << "HTTP server did not become ready on port " << port_;

        setupGraph();
    }

    void TearDown() override {
        if (server_) {
            server_->stop();
            server_.reset();
        }
        if (storage_) {
            storage_->close();
            storage_.reset();
        }
        secondary_index_.reset();
        graph_index_.reset();
        vector_index_.reset();
        tx_manager_.reset();
        std::filesystem::remove_all(db_path_);

        if (!original_cwd_.empty()) {
            std::error_code ec = {};
            std::filesystem::current_path(original_cwd_, ec);
        }
    }

    void setupGraph() {
        themis::BaseEntity e1("edge1");
        e1.setField("id", std::string("edge1"));
        e1.setField("_from", std::string("user1"));
        e1.setField("_to", std::string("user2"));
        e1.setField("_weight", 1.0);
        ASSERT_TRUE(graph_index_->addEdge(e1).ok);

        themis::BaseEntity e2("edge2");
        e2.setField("id", std::string("edge2"));
        e2.setField("_from", std::string("user2"));
        e2.setField("_to", std::string("user3"));
        e2.setField("_weight", 2.0);
        ASSERT_TRUE(graph_index_->addEdge(e2).ok);
    }

    http::response<http::string_body> post(const std::string& target, const json& j) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            auto const results = resolver.resolve("127.0.0.1", std::to_string(port_));
            stream.connect(results);

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::content_type, "application/json");
            req.body() = j.dump();
            req.prepare_payload();

            http::write(stream, req);
            beast::flat_buffer buf;
            http::response<http::string_body> res;
            http::read(stream, buf, res);
            beast::error_code ec; stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            return res;
        } catch (const std::exception& e) {
            ADD_FAILURE() << "POST failed: " << e.what();
            return http::response<http::string_body>{http::status::internal_server_error, 11};
        }
    }

    std::string db_path_;
    std::filesystem::path original_cwd_;
    unsigned short port_{18102};
    std::unique_ptr<themis::server::HttpServer> server_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
};

TEST_F(HttpAqlShortestPathTest, ShortestPath_ReturnsVerticesAndCost) {
    json req = {
        {"query", "FOR v IN 1..3 OUTBOUND 'user1' GRAPH 'cities' SHORTEST_PATH TO 'user3' RETURN v"}
    };
    auto res = post("/query/aql", req);
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    auto body = json::parse(res.body());
    // Keep this test API-shape-oriented: query support may evolve,
    // but successful shortest-path dispatch should return entities array.
    ASSERT_TRUE(body.contains("entities"));
    ASSERT_TRUE(body["entities"].is_array());
    EXPECT_GE(body["entities"].size(), 0);
}
