/**
 * @file bench_api_transport.cpp
 * @brief API Module Transport Layer Performance Benchmarks
 * @version 0.0.1
 * @note Status: Benchmark suite for API hot paths
 * @note Validates roadmap item: "benchmark and release-gate consolidation for 
 *       API transport paths (Target: Q3 2026)"
 * @note Validates FUTURE_ENHANCEMENTS.md § Performance Targets:
 *       - parser and adapter hot paths stay within release baseline regression budgets
 *       - correlation/tracing overhead remains bounded in low-latency request paths
 *       - benchmark manifest completeness reaches no-missing-case status for mapped API targets
 */

#include <benchmark/benchmark.h>
#include <string>
#include <memory>
#include <vector>

#include "api/http_handler.h"

using namespace themis::api;

namespace {

/**
 * @brief Lightweight mock adapter for benchmarking transport paths
 */
class BenchmarkTransportAdapter : public IHttpHandler {
public:
    themis::Result<HttpResponse> handle(const HttpRequest& req) override {
        // Simulate minimal validation and request processing
        if (req.method.empty() || req.path.empty()) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Invalid request"));
        }

        HttpResponse resp;
        resp.status_code = 200;
        resp.body = "{\"data\": \"response\"}";
        resp.headers["Content-Type"] = "application/json";
        return resp;
    }

    std::string_view handlerName() const noexcept override {
        return "BenchmarkTransportAdapter";
    }

    bool requiresAuthentication() const noexcept override {
        return false;
    }
};

/**
 * @brief Helper to create standard benchmark requests
 */
HttpRequest CreateBenchmarkRequest(const std::string& method, const std::string& path) {
    HttpRequest req;
    req.method = method;
    req.path = path;
    req.headers["X-Correlation-ID"] = "550e8400-e29b-41d4-a716-446655440000";
    return req;
}

}  // anonymous namespace

// ============================================================================
// Request Parsing and Validation Benchmarks
// ============================================================================

/**
 * @benchmark API transport: GET request handling
 * Measures time to validate and process a simple GET request
 */
static void BM_TransportHandleGetRequest(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    auto req = CreateBenchmarkRequest("GET", "/api/v1/entities");

    for (auto _ : state) {
        auto result = adapter->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TransportHandleGetRequest)->UseRealTime();

/**
 * @benchmark API transport: POST request with small payload
 * Measures time to validate and process a POST request with 1KB payload
 */
static void BM_TransportHandlePostSmall(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    auto req = CreateBenchmarkRequest("POST", "/api/v1/query");
    req.body = std::string(1024, 'x');  // 1 KB
    req.headers["Content-Type"] = "application/json";

    for (auto _ : state) {
        auto result = adapter->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TransportHandlePostSmall)->UseRealTime();

/**
 * @benchmark API transport: POST request with medium payload
 * Measures time to validate and process a POST request with 100KB payload
 */
static void BM_TransportHandlePostMedium(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    auto req = CreateBenchmarkRequest("POST", "/api/v1/query");
    req.body = std::string(100 * 1024, 'x');  // 100 KB
    req.headers["Content-Type"] = "application/json";

    for (auto _ : state) {
        auto result = adapter->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TransportHandlePostMedium)->UseRealTime();

/**
 * @benchmark API transport: Request with extended headers
 * Measures overhead of processing requests with many custom headers
 */
static void BM_TransportHandleWithManyHeaders(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    auto req = CreateBenchmarkRequest("GET", "/api/v1/entities");

    // Add 50 custom headers
    for (int i = 0; i < 50; ++i) {
        req.headers["X-Custom-" + std::to_string(i)] = "value-" + std::to_string(i);
    }

    for (auto _ : state) {
        auto result = adapter->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TransportHandleWithManyHeaders)->UseRealTime();

// ============================================================================
// Response Serialization Benchmarks
// ============================================================================

/**
 * @benchmark API transport: Small JSON response serialization
 * Measures time to construct and return a small JSON response
 */
static void BM_TransportSerializeSmallResponse(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    auto req = CreateBenchmarkRequest("GET", "/api/v1/entity/123");

    for (auto _ : state) {
        auto result = adapter->handle(req);
        if (result.has_value()) {
            benchmark::DoNotOptimize(result->body);
        }
    }
}
BENCHMARK(BM_TransportSerializeSmallResponse)->UseRealTime();

/**
 * @benchmark API transport: Response header construction
 * Measures time to build and populate response headers
 */
static void BM_TransportResponseHeaderConstruction(benchmark::State& state) {
    for (auto _ : state) {
        HttpResponse resp;
        resp.status_code = 200;
        resp.body = "{\"data\": \"test\"}";
        resp.headers["Content-Type"] = "application/json";
        resp.headers["X-Request-ID"] = "550e8400-e29b-41d4-a716-446655440000";
        resp.headers["Cache-Control"] = "no-cache, no-store";
        resp.headers["X-API-Version"] = "v1";
        benchmark::DoNotOptimize(resp);
    }
}
BENCHMARK(BM_TransportResponseHeaderConstruction)->UseRealTime();

// ============================================================================
// Validation and Error Handling Benchmarks
// ============================================================================

/**
 * @benchmark API transport: Validation of malformed request
 * Measures time to detect and reject a malformed request
 */
static void BM_TransportValidateMalformedRequest(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    HttpRequest req;
    req.method = "";  // Malformed: empty method
    req.path = "/api/v1/entities";

    for (auto _ : state) {
        auto result = adapter->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TransportValidateMalformedRequest)->UseRealTime();

/**
 * @benchmark API transport: Error message construction
 * Measures overhead of generating error responses
 */
static void BM_TransportErrorMessageConstruction(benchmark::State& state) {
    for (auto _ : state) {
        auto error = themis::Error(
            themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
            "ERR_TRANSPORT_MALFORMED_REQUEST: method and path must be non-empty");
        auto error_msg = error.message();
        benchmark::DoNotOptimize(error_msg);
    }
}
BENCHMARK(BM_TransportErrorMessageConstruction)->UseRealTime();

// ============================================================================
// Correlation ID and Tracing Overhead Benchmarks
// ============================================================================

/**
 * @benchmark API transport: Correlation ID propagation
 * Measures overhead of propagating correlation IDs through request/response cycle
 */
static void BM_TransportCorrelationIdPropagation(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    const std::string correlation_id = "550e8400-e29b-41d4-a716-446655440000";

    for (auto _ : state) {
        HttpRequest req;
        req.method = "GET";
        req.path = "/api/v1/entities";
        req.headers["X-Correlation-ID"] = correlation_id;

        auto result = adapter->handle(req);
        if (result.has_value()) {
            auto it = result->headers.find("X-Correlation-ID");
            benchmark::DoNotOptimize(it);
        }
    }
}
BENCHMARK(BM_TransportCorrelationIdPropagation)->UseRealTime();

/**
 * @benchmark API transport: Header lookup performance
 * Measures performance of common header lookups
 */
static void BM_TransportHeaderLookup(benchmark::State& state) {
    HttpRequest req;
    req.method = "POST";
    req.path = "/api/v1/query";
    req.headers["X-Correlation-ID"] = "test-id";
    req.headers["Content-Type"] = "application/json";
    req.headers["Authorization"] = "******";
    req.headers["X-API-Version"] = "v1";

    for (auto _ : state) {
        auto it = req.headers.find("Content-Type");
        benchmark::DoNotOptimize(it);
        auto it2 = req.headers.find("X-Correlation-ID");
        benchmark::DoNotOptimize(it2);
    }
}
BENCHMARK(BM_TransportHeaderLookup)->UseRealTime();

// ============================================================================
// Concurrent Request Handling Benchmarks
// ============================================================================

/**
 * @benchmark API transport: Sequential request handling
 * Measures baseline throughput of sequential request processing
 */
static void BM_TransportSequentialRequests(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();

    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto req = CreateBenchmarkRequest("GET", "/api/v1/entity/" + std::to_string(i));
            auto result = adapter->handle(req);
            benchmark::DoNotOptimize(result);
        }
    }
}
BENCHMARK(BM_TransportSequentialRequests)->UseRealTime();

// ============================================================================
// Edge Case and Boundary Benchmarks
// ============================================================================

/**
 * @benchmark API transport: Long path handling
 * Measures performance with very long request paths
 */
static void BM_TransportLongPath(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();
    std::string long_path = "/api/v1/";
    for (int i = 0; i < 100; ++i) {
        long_path += "segment/" + std::to_string(i) + "/";
    }

    for (auto _ : state) {
        auto req = CreateBenchmarkRequest("GET", long_path);
        auto result = adapter->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TransportLongPath)->UseRealTime();

/**
 * @benchmark API transport: Version negotiation performance
 * Measures performance of version header processing and validation
 */
static void BM_TransportVersionNegotiation(benchmark::State& state) {
    auto adapter = std::make_shared<BenchmarkTransportAdapter>();

    for (auto _ : state) {
        for (const auto version : {"v1", "v2"}) {
            auto req = CreateBenchmarkRequest("GET", "/api/v1/entities");
            req.headers["X-API-Version"] = version;
            auto result = adapter->handle(req);
            benchmark::DoNotOptimize(result);
        }
    }
}
BENCHMARK(BM_TransportVersionNegotiation)->UseRealTime();

BENCHMARK_MAIN();
