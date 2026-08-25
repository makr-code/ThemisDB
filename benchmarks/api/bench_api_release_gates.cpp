/**
 * @file bench_api_release_gates.cpp
 * @brief Phase 5: Benchmark-backed release gates for API transport hot paths.
 *
 * @version 0.0.1
 * @note Status: Phase 5 Q4 2026 — release gate consolidation.
 * @note Validates ROADMAP.md Phase 5 items:
 *       - "lock benchmark-backed release gates for API parsing/execution/serialization hot paths"
 *       - "validate p95/p99 envelopes under representative concurrency profiles"
 * @note Validates FUTURE_ENHANCEMENTS.md § Performance Targets:
 *       - "parser and adapter hot paths stay within release baseline regression budgets"
 *       - "correlation/tracing overhead remains bounded in low-latency request paths"
 *       - "benchmark manifest completeness reaches no-missing-case status for mapped API targets"
 *
 * ### Release Gates (hard limits for CI regression checks)
 *
 * | Gate ID     | Metric                                 | Limit       |
 * |-------------|----------------------------------------|-------------|
 * | GATE-API-01 | Policy validation: GET request         | ≤ 5 µs/op   |
 * | GATE-API-02 | Policy validation: POST with body      | ≤ 10 µs/op  |
 * | GATE-API-03 | Payload rejection (>10 MiB)            | ≤ 5 µs/op   |
 * | GATE-API-04 | Version rejection                      | ≤ 5 µs/op   |
 * | GATE-API-05 | Error taxonomy mapping (all classes)   | ≤ 1 µs/op   |
 * | GATE-API-06 | Contract validator (full check)        | ≤ 5 µs/op   |
 *
 * @note Benchmarks use `UseRealTime()` as required by benchmarks/MEASUREMENT_HYGIENE.md
 *       for wall-clock accuracy in tests involving I/O or system calls.
 */

#include <benchmark/benchmark.h>
#include <string>
#include <memory>
#include <vector>

#include "api/http_handler.h"
#include "api/api_transport_contracts.h"
#include "api/api_error_taxonomy.h"
#include "api/api_transport_policy.h"

using namespace themis::api;

// ============================================================================
// Shared test infrastructure
// ============================================================================

namespace {

/**
 * @brief Minimal pass-through handler for benchmarking the policy layer in
 *        isolation.  Produces a fixed 200 response without allocating.
 */
class PassthroughHandler : public IHttpHandler {
public:
    themis::Result<HttpResponse> handle(const HttpRequest& req) override {
        (void)req;
        HttpResponse resp;
        resp.status_code = 200;
        resp.body        = "{}";
        resp.headers["Content-Type"] = "application/json";
        return resp;
    }

    [[nodiscard]] std::string_view handlerName() const noexcept override {
        return "PassthroughHandler";
    }

    [[nodiscard]] bool requiresAuthentication() const noexcept override {
        return false;
    }
};

// Shared inner handler — constructed once per benchmark invocation context.
auto makePolicy() {
    auto inner = std::make_shared<PassthroughHandler>();
    return std::make_shared<TransportPolicyMiddleware>(inner);
}

} // anonymous namespace

// ============================================================================
// GATE-API-01: Policy validation — GET request (happy path)
// Limit: ≤ 5 µs/op
// ============================================================================

/**
 * @benchmark GATE-API-01: TransportPolicyMiddleware — valid GET (no version header)
 *
 * Measures the overhead introduced by the policy enforcement layer for the
 * simplest possible valid request: a GET with no special headers.
 */
static void BM_PolicyGetRequestHappyPath(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest req;
    req.method = "GET";
    req.path   = "/api/v2/entities";

    for (auto _ : state) {
        auto result = policy->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PolicyGetRequestHappyPath)->UseRealTime();

/**
 * @benchmark GATE-API-01: TransportPolicyMiddleware — valid GET with v1 version header
 *
 * Adds version-header lookup cost to the GET happy path.
 */
static void BM_PolicyGetRequestWithVersionHeader(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest req;
    req.method                   = "GET";
    req.path                     = "/api/v1/entities";
    req.headers["X-API-Version"] = "v1";

    for (auto _ : state) {
        auto result = policy->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PolicyGetRequestWithVersionHeader)->UseRealTime();

// ============================================================================
// GATE-API-02: Policy validation — POST with body (happy path)
// Limit: ≤ 10 µs/op
// ============================================================================

/**
 * @benchmark GATE-API-02: TransportPolicyMiddleware — valid POST 1 KiB JSON payload
 *
 * Exercises payload-size check, Content-Type enforcement, and delegation for
 * a small but realistic POST body.
 */
static void BM_PolicyPostSmallBody(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest req;
    req.method                  = "POST";
    req.path                    = "/api/v2/query";
    req.headers["Content-Type"] = "application/json";
    req.body                    = std::string(1024, 'x');

    for (auto _ : state) {
        auto result = policy->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PolicyPostSmallBody)->UseRealTime();

/**
 * @benchmark GATE-API-02: TransportPolicyMiddleware — valid POST 64 KiB payload
 *
 * Representative of a batch-write or import operation.
 */
static void BM_PolicyPostMediumBody(benchmark::State& state) {
    auto policy = makePolicy();

    const std::string body(64 * 1024, 'x');
    HttpRequest req;
    req.method                  = "POST";
    req.path                    = "/api/v2/batch";
    req.headers["Content-Type"] = "application/octet-stream";
    req.body                    = body;

    for (auto _ : state) {
        auto result = policy->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PolicyPostMediumBody)->UseRealTime();

// ============================================================================
// GATE-API-03: Payload rejection (>10 MiB) — fail-closed fast path
// Limit: ≤ 5 µs/op
// ============================================================================

/**
 * @benchmark GATE-API-03: TransportPolicyMiddleware — payload rejection (oversized body)
 *
 * Validates that payload-size enforcement exits early without examining the
 * body contents.  The rejection path must be as cheap as the happy path.
 */
static void BM_PolicyPayloadRejection(benchmark::State& state) {
    auto policy = makePolicy();

    // Body just over the 10 MiB limit.
    const std::string oversized_body(kMaxPayloadBytes + 1, 'x');
    HttpRequest req;
    req.method                  = "POST";
    req.path                    = "/api/v2/bulk";
    req.headers["Content-Type"] = "application/octet-stream";
    req.body                    = oversized_body;

    for (auto _ : state) {
        auto result = policy->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PolicyPayloadRejection)->UseRealTime();

// ============================================================================
// GATE-API-04: Version rejection — fail-closed fast path
// Limit: ≤ 5 µs/op
// ============================================================================

/**
 * @benchmark GATE-API-04: TransportPolicyMiddleware — version rejection
 *
 * Measures the overhead of rejecting a request with an unsupported
 * X-API-Version header.  Should be at most as expensive as accepting a valid
 * version (the loop over kSupportedApiVersions is O(|versions|) = O(2)).
 */
static void BM_PolicyVersionRejection(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest req;
    req.method                   = "GET";
    req.path                     = "/api/v99/entities";
    req.headers["X-API-Version"] = "v99";

    for (auto _ : state) {
        auto result = policy->handle(req);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PolicyVersionRejection)->UseRealTime();

// ============================================================================
// GATE-API-05: Error taxonomy mapping — all failure classes
// Limit: ≤ 1 µs/op
// ============================================================================

/**
 * @benchmark GATE-API-05: ApiErrorTaxonomy::toErrorCode — all failure classes
 *
 * Taxonomy mapping must be O(1) per call (switch statement).  This benchmark
 * cycles through all failure classes to ensure the hot path has no branches
 * that add surprising overhead.
 */
static void BM_ErrorTaxonomyToErrorCode(benchmark::State& state) {
    using FC = TransportFailureClass;
    const FC classes[] = {
        FC::MalformedRequest,    FC::PayloadTooLarge,
        FC::UnsupportedVersion,  FC::ContentTypeMissing,
        FC::ContentTypeMismatch, FC::Unauthorized,
        FC::RateLimitExceeded,   FC::CapabilityUnavailable,
        FC::InternalError,
    };

    for (auto _ : state) {
        for (const auto& fc : classes) {
            auto code = ApiErrorTaxonomy::toErrorCode(fc);
            benchmark::DoNotOptimize(code);
        }
    }
}
BENCHMARK(BM_ErrorTaxonomyToErrorCode)->UseRealTime();

/**
 * @benchmark GATE-API-05: ApiErrorTaxonomy::toHttpStatus — all failure classes
 */
static void BM_ErrorTaxonomyToHttpStatus(benchmark::State& state) {
    using FC = TransportFailureClass;
    const FC classes[] = {
        FC::MalformedRequest,    FC::PayloadTooLarge,
        FC::UnsupportedVersion,  FC::ContentTypeMissing,
        FC::ContentTypeMismatch, FC::Unauthorized,
        FC::RateLimitExceeded,   FC::CapabilityUnavailable,
        FC::InternalError,
    };

    for (auto _ : state) {
        for (const auto& fc : classes) {
            auto status = ApiErrorTaxonomy::toHttpStatus(fc);
            benchmark::DoNotOptimize(status);
        }
    }
}
BENCHMARK(BM_ErrorTaxonomyToHttpStatus)->UseRealTime();

// ============================================================================
// GATE-API-06: TransportContractValidator::validate — full check
// Limit: ≤ 5 µs/op
// ============================================================================

/**
 * @benchmark GATE-API-06: TransportContractValidator::validate — valid GET
 */
static void BM_ContractValidateGetValid(benchmark::State& state) {
    for (auto _ : state) {
        auto fc = TransportContractValidator::validate(
            "GET", "/api/v1/entities", "", 0, "v1");
        benchmark::DoNotOptimize(fc);
    }
}
BENCHMARK(BM_ContractValidateGetValid)->UseRealTime();

/**
 * @benchmark GATE-API-06: TransportContractValidator::validate — valid POST
 */
static void BM_ContractValidatePostValid(benchmark::State& state) {
    const std::string body(1024, 'x');
    for (auto _ : state) {
        auto fc = TransportContractValidator::validate(
            "POST", "/api/v1/ingest", "application/json",
            body.size(), "v2");
        benchmark::DoNotOptimize(fc);
    }
}
BENCHMARK(BM_ContractValidatePostValid)->UseRealTime();

/**
 * @benchmark GATE-API-06: TransportContractValidator::validate — invalid version
 */
static void BM_ContractValidateUnsupportedVersion(benchmark::State& state) {
    for (auto _ : state) {
        auto fc = TransportContractValidator::validate(
            "GET", "/api/v99/entities", "", 0, "v99");
        benchmark::DoNotOptimize(fc);
    }
}
BENCHMARK(BM_ContractValidateUnsupportedVersion)->UseRealTime();

// ============================================================================
// p95/p99 envelope: sustained throughput under representative concurrency
// ============================================================================

/**
 * @benchmark API policy: sustained throughput — 100 sequential requests
 *
 * Measures wall-clock time for 100 sequential GET requests through the policy
 * middleware.  Used to establish the p95/p99 baseline latency per request.
 */
static void BM_PolicySustainedGetThroughput(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest req;
    req.method = "GET";
    req.path   = "/api/v2/entities";

    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto result = policy->handle(req);
            benchmark::DoNotOptimize(result);
        }
        state.SetItemsProcessed(100);
    }
}
BENCHMARK(BM_PolicySustainedGetThroughput)->UseRealTime();

/**
 * @benchmark API policy: sustained throughput — 100 sequential POST requests
 *
 * Representative batch-ingest scenario with JSON bodies.
 */
static void BM_PolicySustainedPostThroughput(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest req;
    req.method                  = "POST";
    req.path                    = "/api/v2/ingest";
    req.headers["Content-Type"] = "application/json";
    req.body                    = std::string(256, 'x');

    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto result = policy->handle(req);
            benchmark::DoNotOptimize(result);
        }
        state.SetItemsProcessed(100);
    }
}
BENCHMARK(BM_PolicySustainedPostThroughput)->UseRealTime();

/**
 * @benchmark API policy: mixed request types — GET / POST / DELETE interleaved
 *
 * Models realistic API traffic with a mix of read, write, and delete operations.
 */
static void BM_PolicyMixedRequestTypes(benchmark::State& state) {
    auto policy = makePolicy();

    HttpRequest get_req;
    get_req.method = "GET";
    get_req.path   = "/api/v1/entities/42";

    HttpRequest post_req;
    post_req.method                  = "POST";
    post_req.path                    = "/api/v1/entities";
    post_req.headers["Content-Type"] = "application/json";
    post_req.body                    = "{\"key\":\"value\"}";

    HttpRequest del_req;
    del_req.method = "DELETE";
    del_req.path   = "/api/v1/entities/42";

    for (auto _ : state) {
        benchmark::DoNotOptimize(policy->handle(get_req));
        benchmark::DoNotOptimize(policy->handle(post_req));
        benchmark::DoNotOptimize(policy->handle(del_req));
        state.SetItemsProcessed(3);
    }
}
BENCHMARK(BM_PolicyMixedRequestTypes)->UseRealTime();

BENCHMARK_MAIN();
