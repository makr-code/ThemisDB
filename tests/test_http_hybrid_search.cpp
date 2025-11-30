#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include <filesystem>

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "storage/base_entity.h"
#include "content/content_manager.h"
#include "content/content_processor.h"

// Hybrid HTTP tests: tie-break ordering, IN/RANGE filters, direct seeding & content import.

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpHybridSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        const std::string db_path = "data/themis_http_hybrid_search_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
        themis::RocksDBWrapper::Config cfg; cfg.db_path = db_path; cfg.memtable_size_mb = 64; cfg.block_cache_size_mb = 128;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
        tx_manager_ = std::make_shared<themis::TransactionManager>(*storage_, *secondary_index_, *graph_index_, *vector_index_);

        // Initialize vector indices for both direct seeding and content-based tests
        auto st1 = vector_index_->init("hybrid_docs", 3, themis::VectorIndexManager::Metric::COSINE, 16, 200, 64);
        ASSERT_TRUE(st1.ok) << st1.message;
        auto st2 = vector_index_->init("chunks", 8, themis::VectorIndexManager::Metric::COSINE, 16, 200, 64);
        ASSERT_TRUE(st2.ok) << st2.message;

        // HTTP server
        themis::server::HttpServer::Config scfg; scfg.host = "127.0.0.1"; scfg.port = 18087; scfg.num_threads = 2;
        server_ = std::make_unique<themis::server::HttpServer>(scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }

    void TearDown() override {
        if (server_) server_->stop();
        storage_->close();
        std::filesystem::remove_all("data/themis_http_hybrid_search_test");
    }

    void seedDirectVectorData() {
        // Direct vector index seeding for tie-break and filter tests
        // Two embeddings designed to yield identical cosine distance to query to test tie-break (docA, docB)
        themis::BaseEntity a("docA");
        a.setField("embedding", std::vector<float>{1.0f, 0.0f, 0.0f});
        a.setField("dataset", std::string("alpha"));
        a.setField("score", std::string("10"));
        ASSERT_TRUE(vector_index_->addEntity(a, "embedding").ok);

        themis::BaseEntity b("docB");
        b.setField("embedding", std::vector<float>{1.0f, 0.0f, 0.0f});
        b.setField("dataset", std::string("beta"));
        b.setField("score", std::string("15"));
        ASSERT_TRUE(vector_index_->addEntity(b, "embedding").ok);

        themis::BaseEntity c("docC");
        c.setField("embedding", std::vector<float>{0.0f, 1.0f, 0.0f});
        c.setField("dataset", std::string("alpha"));
        c.setField("score", std::string("25"));
        ASSERT_TRUE(vector_index_->addEntity(c, "embedding").ok);

        // Provide whitelist mapping for direct vector filters
        json filterSchema = {
            {"collections", {
                {"hybrid_docs", {
                    {"filter_whitelist", json::array({"dataset","score"})}
                }}
            }}
        };
        auto fstr = filterSchema.dump();
        storage_->put("config:content_filter_schema", std::vector<uint8_t>(fstr.begin(), fstr.end()));
    }

    json postJson(const std::string& target, const json& body) {
        net::io_context ioc; tcp::resolver resolver(ioc); beast::tcp_stream stream(ioc);
        auto const results = resolver.resolve("127.0.0.1", "18087");
        stream.connect(results);
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump(); req.prepare_payload();
        http::write(stream, req);
        beast::flat_buffer buf; http::response<http::string_body> res; http::read(stream, buf, res);
        beast::error_code ec; stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        try { return json::parse(res.body()); } catch (...) { return json{{"__raw__", res.body()}}; }
    }

    std::unique_ptr<themis::server::HttpServer> server_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
};

// ============================================================================
// Test Suite 1: Direct Vector Index Seeding (tie-break, IN/RANGE filters)
// ============================================================================

TEST_F(HttpHybridSearchTest, DirectVector_TieBreakPkOrderingIdenticalScores) {
    seedDirectVectorData();
    
    json req = {
        {"collection", "hybrid_docs"},
        {"vector", json::array({1.0,0.0,0.0})},
        {"k", 5},
        {"tie_break", "pk"},
        {"filters", json::array()}
    };
    auto res = postJson("/search/hybrid", req);
    ASSERT_TRUE(res.contains("results")) << res.dump();
    auto arr = res["results"].get<json::array_t>();
    // Expect docA then docB (alphabetical pk) ahead of docC due to worse distance
    ASSERT_GE(arr.size(), 2u);
    EXPECT_EQ(arr[0]["pk"], "docA");
    EXPECT_EQ(arr[1]["pk"], "docB");
}

TEST_F(HttpHybridSearchTest, DirectVector_INandRANGE_FilteringWorks) {
    seedDirectVectorData();
    
    // Filter: dataset IN ["alpha"] AND score RANGE {min:5, max:20}
    json filters = json::array({
        json{{"field","dataset"},{"op","IN"},{"value", json::array({"alpha"})}},
        json{{"field","score"},{"op","RANGE"},{"value", json{{"min",5},{"max",20}}}}
    });
    json req = {
        {"collection", "hybrid_docs"},
        {"vector", json::array({1.0,0.0,0.0})},
        {"k", 10},
        {"tie_break", "pk"},
        {"filters", filters}
    };
    auto res = postJson("/search/hybrid", req);
    ASSERT_TRUE(res.contains("results")) << res.dump();
    auto arr = res["results"].get<json::array_t>();
    // Should include docA (alpha, score 10) exclude docC (alpha, score 25) exclude docB (beta)
    std::vector<std::string> pks; for (auto& v : arr) pks.push_back(v["pk"].get<std::string>());
    EXPECT_NE(std::find(pks.begin(), pks.end(), "docA"), pks.end());
    EXPECT_EQ(std::find(pks.begin(), pks.end(), "docC"), pks.end());
    EXPECT_EQ(std::find(pks.begin(), pks.end(), "docB"), pks.end());
}

// ============================================================================
// Test Suite 2: Content Import Based (chunk embeddings, metadata filters)
// ============================================================================

TEST_F(HttpHybridSearchTest, ContentImport_TieBreak_ByPkOnEqualScores) {
    // Prepare two chunks with identical embeddings
    std::vector<float> emb = {0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.0f, -0.2f, 0.05f};
    json req = {
        {"content", json{{"id","doc-hybrid-eq"},{"mime_type","text/plain"}}},
        {"chunks", json::array({
            json{{"id","ha"},{"seq_num",0},{"chunk_type","text"},{"text","alpha topic"},{"embedding", emb}},
            json{{"id","hb"},{"seq_num",1},{"chunk_type","text"},{"text","beta topic"},{"embedding", emb}}
        })}
    };
    (void)postJson("/content/import", req);

    // Hybrid search with no expansion; equal scores -> deterministic order by pk
    json hreq = {
        {"query", "some query text"},
        {"k", 2},
        {"expand", json{{"hops", 0}}},
        {"tie_break", "pk"},
        {"tie_break_epsilon", 1e-12}
    };
    auto resp = postJson("/search/hybrid", hreq);
    ASSERT_TRUE(resp.contains("results")) << resp.dump();
    auto arr = resp["results"].get<json::array_t>();
    ASSERT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr[0].value("pk", ""), std::string("chunks:ha"));
    EXPECT_EQ(arr[1].value("pk", ""), std::string("chunks:hb"));
}

TEST_F(HttpHybridSearchTest, ContentImport_Filters_Array_IN_and_RANGE_Work) {
    // Content with metadata fields mapped via schema to user_metadata fields
    // Configure filter schema mapping: dataset -> user_metadata.dataset, score -> user_metadata.score
    json schema = { {"field_map", json{{"dataset","user_metadata.dataset"},{"score","user_metadata.score"}}} };
    auto s = schema.dump();
    storage_->put("config:content_filter_schema", std::vector<uint8_t>(s.begin(), s.end()));

    // Import three contents with single chunks
    auto make = [&](const std::string& id, const std::string& dataset, double score){
        json meta = {
            {"id", id}, {"mime_type","text/plain"},
            {"user_metadata", json{{"dataset", dataset},{"score", score}}}
        };
        json c = {
            {"content", meta},
            {"chunks", json::array({ json{{"id", id+"-c"},{"seq_num",0},{"chunk_type","text"},{"text","x"},{"embedding", std::vector<float>{0.1f,0.2f,0.3f,0.4f,0.1f,0.0f,-0.2f,0.05f}} })}
        };
        (void)postJson("/content/import", c);
    };
    make("d1","train", 0.4);
    make("d2","eval", 0.8);
    make("d3","test", 0.6);

    // Query with filters: dataset IN ["train","test"], score RANGE [0.5, 1.0]
    json hreq = {
        {"query", "q"}, {"k", 10}, {"expand", json{{"hops",0}}},
        {"filters", json::array({
            json{{"field","dataset"},{"op","IN"},{"values", json::array({"train","test"})}},
            json{{"field","score"},{"op","RANGE"},{"min", 0.5},{"max", 1.0}}
        })}
    };
    auto res = postJson("/search/hybrid", hreq);
    ASSERT_TRUE(res.contains("results")) << res.dump();
    auto arr = res["results"].get<json::array_t>();
    // Only d3 should pass both filters
    std::vector<std::string> pks; pks.reserve(arr.size());
    for (auto& it : arr) if (it.contains("pk")) pks.push_back(it["pk"].get<std::string>());
    // Chunks use id+"-c" and are prefixed with "chunks:" in results
    EXPECT_NE(std::find(pks.begin(), pks.end(), std::string("chunks:d3-c")), pks.end());
    EXPECT_EQ(std::find(pks.begin(), pks.end(), std::string("chunks:d1-c")), pks.end());
    EXPECT_EQ(std::find(pks.begin(), pks.end(), std::string("chunks:d2-c")), pks.end());
}
