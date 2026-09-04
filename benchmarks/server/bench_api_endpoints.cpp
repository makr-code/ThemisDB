/**
 * @file bench_api_endpoints.cpp
 * @brief Performance benchmarks for API endpoints (Issue #1511).
 *
 * Measures latency and throughput for the following operations:
 *   1.  GraphQL parse – uncached, simple 10-field query
 *   2.  GraphQL parse – cache hit (same query, second call)
 *   3.  GraphQL parse – complex nested query (depth 4, 20+ fields)
 *   4.  GraphQL parse – invalid query (rejection path)
 *   5.  GraphQL execute – parse + execute with mock document resolver
 *   6.  Correlation ID generation – UUID v4 overhead (< 10 µs target)
 *   7.  Correlation ID check – X-Correlation-ID header present (no-op path)
 *   8.  JSON response serialisation – single-document response body
 *   9.  JSON response serialisation – paginated list (100 documents)
 *   10. JSON request deserialisation – document PUT body
 *   11. HTTP message build – well-formed GET request (Beast)
 *   12. HTTP message build – well-formed POST request with JSON body (Beast)
 *   13. REST endpoint roundtrip – GET /health through in-process server
 *   14. REST endpoint roundtrip – GET /v1/documents via persistent keep-alive connection
 *
 * Latency targets (from src/api/FUTURE_ENHANCEMENTS.md):
 *   - GraphQL parse+execute (10-field query): < 2 ms p99
 *   - Correlation ID generation overhead:     < 10 µs/req
 *
 * Build:
 *   cmake -DTHEMIS_BUILD_BENCHMARKS=ON ... && cmake --build . --target bench_api_endpoints
 * Run:
 *   ./benchmarks/bench_api_endpoints --benchmark_format=json
 */

#include <benchmark/benchmark.h>

#include "api/graphql.h"
#include "api/graphql_cache.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

// ============================================================================
// In-process HTTP server fixture dependencies
// ============================================================================

#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

// ============================================================================
// Shared GraphQL query strings
// ============================================================================

namespace {

// 10-field simple document query (target for < 2 ms p99)
static const std::string kSimpleQuery =
    "query GetDocument {"
    "  document(id: \"doc_001\") {"
    "    id"
    "    title"
    "    author"
    "    createdAt"
    "    updatedAt"
    "    status"
    "    version"
    "    collection"
    "    tags"
    "    content"
    "  }"
    "}";

// Complex nested query (depth 4, 20+ fields)
static const std::string kComplexQuery =
    "query GetGraph {"
    "  document(id: \"doc_001\") {"
    "    id"
    "    title"
    "    author"
    "    edges {"
    "      id"
    "      type"
    "      weight"
    "      target {"
    "        id"
    "        title"
    "        collection"
    "        tags"
    "        nested {"
    "          id"
    "          value"
    "          score"
    "        }"
    "      }"
    "    }"
    "    vectors {"
    "      dimension"
    "      values"
    "      model"
    "    }"
    "  }"
    "}";

// Invalid query to benchmark the rejection path
static const std::string kInvalidQuery =
    "query { document( { unclosed";

// ============================================================================
// UUID v4 generator (RFC 4122 compliant, random variant)
// ============================================================================

static std::string generateUuidV4() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    static thread_local std::uniform_int_distribution<uint64_t> dist;

    const uint64_t hi = dist(rng);
    const uint64_t lo = dist(rng);

    // Apply RFC 4122 variant and version bits
    const uint64_t hi_v =
        (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL; // version 4
    const uint64_t lo_v =
        (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // variant 1

    char buf[37];
    const uint32_t a  = static_cast<uint32_t>(hi_v >> 32);
    const uint16_t b  = static_cast<uint16_t>(hi_v >> 16);
    const uint16_t c  = static_cast<uint16_t>(hi_v);
    const uint16_t d  = static_cast<uint16_t>(lo_v >> 48);
    const uint64_t e  = lo_v & 0x0000FFFFFFFFFFFFULL;
    std::snprintf(buf, sizeof(buf),
        "%08x-%04x-%04x-%04x-%012llx",
        a, b, c, d, static_cast<unsigned long long>(e));
    return std::string(buf);
}

// ============================================================================
// In-process HTTP server fixture (started once per fixture instance)
// ============================================================================

static constexpr uint16_t kBenchPort = 18480;
static const std::string  kBenchHost = "127.0.0.1";
static const std::string  kBenchDb   = "data/bench_api_endpoints_db";

struct ServerEnv {
    std::shared_ptr<themis::RocksDBWrapper>          storage;
    std::shared_ptr<themis::SecondaryIndexManager>   secondary_index;
    std::shared_ptr<themis::GraphIndexManager>       graph_index;
    std::shared_ptr<themis::VectorIndexManager>      vector_index;
    std::shared_ptr<themis::TransactionManager>      tx_manager;
    std::unique_ptr<themis::server::HttpServer>      server;
    bool ready = false;

    static ServerEnv& instance() {
        static ServerEnv env;
        return env;
    }

    bool ensureInit(benchmark::State& state) {
        if (ready) {
          return true;
        }
        try {
            std::filesystem::remove_all(kBenchDb);

            themis::RocksDBWrapper::Config cfg;
            cfg.db_path             = kBenchDb;
            cfg.memtable_size_mb    = 64;
            cfg.block_cache_size_mb = 128;
            storage = std::make_shared<themis::RocksDBWrapper>(cfg);
            if (!storage->open()) {
                state.SkipWithError("bench_api_endpoints: failed to open RocksDB");
                return false;
            }

            secondary_index = std::make_shared<themis::SecondaryIndexManager>(*storage);
            graph_index     = std::make_shared<themis::GraphIndexManager>(*storage);
            vector_index    = std::make_shared<themis::VectorIndexManager>(*storage);
            tx_manager      = std::make_shared<themis::TransactionManager>(
                *storage, *secondary_index, *graph_index, *vector_index);

            themis::server::HttpServer::Config scfg;
            scfg.host        = kBenchHost;
            scfg.port        = kBenchPort;
            scfg.num_threads = 2;

            server = std::make_unique<themis::server::HttpServer>(
                scfg, storage, secondary_index, graph_index, vector_index, tx_manager);
            server->start();

            // Give the acceptor thread a moment to bind
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            ready = true;
            return true;
        } catch (const std::exception& ex) {
            state.SkipWithError(ex.what());
            return false;
        }
    }

    ~ServerEnv() {
        if (server) {
            server->stop();
        }
        if (storage) {
            storage->close();
        }
        std::filesystem::remove_all(kBenchDb);
    }
};

// Helper: send one HTTP request over a new connection, return response status
static unsigned int sendHttpRequest(const std::string& target, http::verb verb,
                                    const std::string& body = "") {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.connect(resolver.resolve(kBenchHost, std::to_string(kBenchPort)));
        stream.expires_after(std::chrono::seconds(5));

        http::request<http::string_body> req{verb, target, 11};
        req.set(http::field::host, kBenchHost);
        req.set(http::field::connection, "close");
        if (!body.empty()) {
            req.set(http::field::content_type, "application/json");
            req.body() = body;
        }
        req.prepare_payload();
        http::write(stream, req);

        beast::flat_buffer buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return static_cast<unsigned int>(res.result_int());
    } catch (...) {
        return 0;
    }
}

} // anonymous namespace

// ============================================================================
// 1. GraphQL parse – uncached, simple 10-field query
// ============================================================================

static void BM_GraphQL_Parse_Simple_Uncached(benchmark::State& state) {
    // Reset cache before loop so every iteration measures a cold parse
    themis::graphql::QueryPlanCache::instance().clear();

    for (auto _ : state) {
        themis::graphql::QueryPlanCache::instance().clear();
        auto result = themis::graphql::Parser::parse(kSimpleQuery);
        benchmark::DoNotOptimize(result);
    }

    state.SetLabel("GraphQL parse – 10-field query (cache cleared each iter)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GraphQL_Parse_Simple_Uncached);

// ============================================================================
// 2. GraphQL parse – cache hit (warm parse, same query)
// ============================================================================

static void BM_GraphQL_Parse_Simple_Cached(benchmark::State& state) {
    // Prime the cache
    themis::graphql::Parser::parse(kSimpleQuery);

    for (auto _ : state) {
        auto result = themis::graphql::Parser::parse(kSimpleQuery);
        benchmark::DoNotOptimize(result);
    }

    state.SetLabel("GraphQL parse – 10-field query (cache warm)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GraphQL_Parse_Simple_Cached);

// ============================================================================
// 3. GraphQL parse – complex nested query
// ============================================================================

static void BM_GraphQL_Parse_Complex_Uncached(benchmark::State& state) {
    for (auto _ : state) {
        themis::graphql::QueryPlanCache::instance().clear();
        auto result = themis::graphql::Parser::parse(kComplexQuery);
        benchmark::DoNotOptimize(result);
    }

    state.SetLabel("GraphQL parse – complex nested query (depth 4, cache cleared)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GraphQL_Parse_Complex_Uncached);

// ============================================================================
// 4. GraphQL parse – invalid query (rejection path)
// ============================================================================

static void BM_GraphQL_Parse_Invalid(benchmark::State& state) {
    for (auto _ : state) {
        auto result = themis::graphql::Parser::parse(kInvalidQuery);
        benchmark::DoNotOptimize(result);
    }

    state.SetLabel("GraphQL parse – invalid query (rejection path)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GraphQL_Parse_Invalid);

// ============================================================================
// 5. GraphQL execute – parse + execute with mock document resolver
// ============================================================================

static void BM_GraphQL_Execute_MockResolver(benchmark::State& state) {
    // Parse once outside the loop
    auto parseResult = themis::graphql::Parser::parse(kSimpleQuery);
    if (!parseResult.success) {
        state.SkipWithError("BM_GraphQL_Execute_MockResolver: parse failed");
        return;
    }

    // Build a mock document resolver that returns a fixed Value object
    themis::graphql::ExecutionContext ctx;
    ctx.tenant_id  = "bench-tenant";
    ctx.mask_errors = false;

    // Register a resolver for "document" that returns a static object
    auto docValue = themis::graphql::Value::object({
        {"id",         themis::graphql::Value::string("doc_001")},
        {"title",      themis::graphql::Value::string("Benchmark Document")},
        {"author",     themis::graphql::Value::string("bench-user")},
        {"createdAt",  themis::graphql::Value::string("2026-01-01T00:00:00Z")},
        {"updatedAt",  themis::graphql::Value::string("2026-02-01T00:00:00Z")},
        {"status",     themis::graphql::Value::string("published")},
        {"version",    themis::graphql::Value::integer(1)},
        {"collection", themis::graphql::Value::string("documents")},
        {"tags",       themis::graphql::Value::string("bench,perf")},
        {"content",    themis::graphql::Value::string("Lorem ipsum content body")},
    });

    ctx.resolvers["document"] = [&docValue](
        const themis::graphql::Field&,
        const std::shared_ptr<themis::graphql::Value>&,
        const themis::graphql::ExecutionContext&)
    {
        return docValue;
    };

    themis::graphql::Executor executor;

    for (auto _ : state) {
        auto result = executor.execute(parseResult.document, ctx);
        benchmark::DoNotOptimize(result);
    }

    state.SetLabel("GraphQL parse (cached) + execute – mock document resolver");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GraphQL_Execute_MockResolver);

// ============================================================================
// 6. Correlation ID generation – UUID v4 overhead (target: < 10 µs)
// ============================================================================

static void BM_CorrelationId_Generate_UUIDv4(benchmark::State& state) {
    for (auto _ : state) {
        auto id = generateUuidV4();
        benchmark::DoNotOptimize(id);
    }

    state.SetLabel("UUID v4 correlation ID generation");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CorrelationId_Generate_UUIDv4);

// ============================================================================
// 7. Correlation ID check – header present (string lookup, no-op path)
// ============================================================================

static void BM_CorrelationId_Header_Check(benchmark::State& state) {
    // Simulate checking whether X-Correlation-ID is already set in the
    // incoming request before generating one.  This models the fast path
    // where the client supplies its own correlation ID.
    http::request<http::string_body> req{http::verb::get, "/v1/documents/doc_001", 11};
    req.set(http::field::host, kBenchHost);
    req.set("X-Correlation-ID", "3f8a2c41-9b3d-4e56-87af-1234567890ab");

    for (auto _ : state) {
        const auto& field = req["X-Correlation-ID"];
        const bool present = !field.empty();
        const std::string id = present ? std::string(field) : generateUuidV4();
        benchmark::DoNotOptimize(id);
    }

    state.SetLabel("X-Correlation-ID header present check (fast path)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CorrelationId_Header_Check);

// ============================================================================
// 8. JSON response serialisation – single-document response body
// ============================================================================

static void BM_Json_Serialize_SingleDocument(benchmark::State& state) {
    using json = nlohmann::json;

    for (auto _ : state) {
        json doc;
        doc["id"]         = "doc_001";
        doc["title"]      = "Benchmark Document";
        doc["author"]     = "bench-user";
        doc["createdAt"]  = "2026-01-01T00:00:00Z";
        doc["updatedAt"]  = "2026-02-01T00:00:00Z";
        doc["status"]     = "published";
        doc["version"]    = 1;
        doc["collection"] = "documents";
        doc["tags"]       = json::array({"bench", "perf"});
        doc["content"]    = "Lorem ipsum content body";

        json response;
        response["data"] = doc;

        auto body = response.dump();
        benchmark::DoNotOptimize(body);
    }

    state.SetLabel("JSON serialise – single document response (10 fields)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Json_Serialize_SingleDocument);

// ============================================================================
// 9. JSON response serialisation – paginated list (100 documents)
// ============================================================================

static void BM_Json_Serialize_PaginatedList(benchmark::State& state) {
    using json = nlohmann::json;

    const int N = static_cast<int>(state.range(0));

    for (auto _ : state) {
        json docs = json::array();
        for (int i = 0; i < N; ++i) {
            json doc;
            doc["id"]      = "doc_" + std::to_string(i);
            doc["title"]   = "Document " + std::to_string(i);
            doc["version"] = i;
            docs.push_back(std::move(doc));
        }

        json response;
        response["data"]  = std::move(docs);
        response["total"] = N;
        response["page"]  = 1;

        auto body = response.dump();
        benchmark::DoNotOptimize(body);
    }

    state.SetLabel("JSON serialise – paginated document list");
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_Json_Serialize_PaginatedList)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// 10. JSON request deserialisation – document PUT body
// ============================================================================

static void BM_Json_Deserialize_PutRequest(benchmark::State& state) {
    using json = nlohmann::json;

    const std::string body = R"({
        "id": "doc_001",
        "title": "Benchmark Document",
        "author": "bench-user",
        "createdAt": "2026-01-01T00:00:00Z",
        "updatedAt": "2026-02-01T00:00:00Z",
        "status": "published",
        "version": 1,
        "collection": "documents",
        "tags": ["bench", "perf"],
        "content": "Lorem ipsum content body for a realistic benchmark"
    })";

    for (auto _ : state) {
        auto doc = json::parse(body);
        benchmark::DoNotOptimize(doc);
    }

    state.SetLabel("JSON deserialise – document PUT body (10 fields)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Json_Deserialize_PutRequest);

// ============================================================================
// 11. HTTP message build – GET request (Beast)
// ============================================================================

static void BM_HttpMessage_Build_GetRequest(benchmark::State& state) {
    for (auto _ : state) {
        http::request<http::string_body> req{
            http::verb::get, "/v1/documents/doc_001", 11};
        req.set(http::field::host, kBenchHost);
        req.set(http::field::accept, "application/json");
        req.set("X-Correlation-ID", "3f8a2c41-9b3d-4e56-87af-1234567890ab");
        req.prepare_payload();
        benchmark::DoNotOptimize(req);
    }

    state.SetLabel("HTTP message build – GET /v1/documents/{id}");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HttpMessage_Build_GetRequest);

// ============================================================================
// 12. HTTP message build – POST request with JSON body (Beast)
// ============================================================================

static void BM_HttpMessage_Build_PostRequest(benchmark::State& state) {
    const std::string jsonBody =
        R"({"query":"{ document(id:\"doc_001\") { id title author } }"})";

    for (auto _ : state) {
        http::request<http::string_body> req{
            http::verb::post, "/v1/graphql", 11};
        req.set(http::field::host, kBenchHost);
        req.set(http::field::content_type, "application/json");
        req.set("X-Correlation-ID", generateUuidV4());
        req.body() = jsonBody;
        req.prepare_payload();
        benchmark::DoNotOptimize(req);
    }

    state.SetLabel("HTTP message build – POST /v1/graphql with JSON body");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HttpMessage_Build_PostRequest);

// ============================================================================
// 13. REST endpoint roundtrip – GET /health through in-process server
// ============================================================================

static void BM_HttpServer_Health_Endpoint(benchmark::State& state) {
    if (!ServerEnv::instance().ensureInit(state)) {
      return;
    }

    for (auto _ : state) {
        auto status = sendHttpRequest("/health", http::verb::get);
        benchmark::DoNotOptimize(status);
    }

    state.SetLabel("HTTP GET /health roundtrip (new connection per iter)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HttpServer_Health_Endpoint);

// ============================================================================
// 14. REST endpoint roundtrip – GET /v1/documents via persistent connection
// ============================================================================

static void BM_HttpServer_Documents_List_Persistent(benchmark::State& state) {
    if (!ServerEnv::instance().ensureInit(state)) {
      return;
    }

    // Open a single persistent (keep-alive) connection for the entire run
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.connect(resolver.resolve(kBenchHost, std::to_string(kBenchPort)));
        stream.expires_after(std::chrono::seconds(30));

        for (auto _ : state) {
            http::request<http::string_body> req{
                http::verb::get, "/v1/documents", 11};
            req.set(http::field::host, kBenchHost);
            req.set(http::field::connection, "keep-alive");
            req.prepare_payload();
            http::write(stream, req);

            beast::flat_buffer buf;
            http::response<http::string_body> res;
            http::read(stream, buf, res);
            benchmark::DoNotOptimize(res);
        }

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    } catch (const std::exception& ex) {
        state.SkipWithError(ex.what());
    }

    state.SetLabel("HTTP GET /v1/documents – persistent keep-alive connection");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HttpServer_Documents_List_Persistent);
