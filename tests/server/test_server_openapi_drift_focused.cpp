/**
 * @file test_server_openapi_drift_focused.cpp
 * @brief Server Module — OpenAPI Drift Detection focused regression tests.
 *
 * Phase 3 Schema-Governance acceptance tests for the
 * RouteRegistry::captureSpecSnapshot() / detectDrift() API.
 *
 * Test IDs (Phase 3, OpenAPI Drift Detection):
 * - **SOD-01** — No-drift baseline: identical registry before and after
 * - **SOD-02** — Schema change on existing handler detected as "changed"
 * - **SOD-03** — Path removed from registry detected as "removed"
 * - **SOD-04** — Response schema change (operationId mutation) detected
 * - **SOD-05** — New path added without annotation → detected as "added"
 * - **SOD-06** — Drift resolves after annotation is added post-registration
 * - **SOD-07** — Thread-safe concurrent registration + snapshot (no races)
 * - **SOD-08** — Spec roundtrip: registry → snapshot → compare → equality
 *
 * All tests are fully in-process.
 * Deterministic seed: kOpenApiDriftSeed = 2025.
 *
 * @version 1.0.0
 * @note CTest labels: release_critical;server;phase3;schema
 */

#include <gtest/gtest.h>

#include "server/openapi_route_registry.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kOpenApiDriftSeed = 2025U;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a minimal RouteEntry for use in tests.
static RouteEntry makeEntry(const std::string& path,
                            const std::string& method,
                            const std::string& operation_id,
                            const std::string& summary = "",
                            bool               deprecated = false)
{
    RouteOperation op;
    op.operationId = operation_id;
    op.summary     = summary;
    op.deprecated  = deprecated;
    return RouteEntry{path, method, std::move(op)};
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: clean registry between tests
// ─────────────────────────────────────────────────────────────────────────────
class OpenApiDriftTest : public ::testing::Test {
protected:
    void SetUp() override {
        RouteRegistry::instance().clear();
    }
    void TearDown() override {
        RouteRegistry::instance().clear();
    }

    RouteRegistry& registry() { return RouteRegistry::instance(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// SOD-01: No-drift baseline
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD01_NoDriftBaseline) {
    registry().registerRoute(makeEntry("/api/v1/users", "GET", "listUsers", "List all users"));

    std::string snap1 = registry().captureSpecSnapshot();
    std::string snap2 = registry().captureSpecSnapshot();

    EXPECT_EQ(snap1, snap2)
        << "Two snapshots of an unchanged registry must be identical";

    auto report = registry().detectDrift(snap1);
    EXPECT_FALSE(report.hasDrift())
        << "Comparing a snapshot to itself must report no drift";
    EXPECT_TRUE(report.added.empty());
    EXPECT_TRUE(report.removed.empty());
    EXPECT_TRUE(report.changed.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-02: Handler registered with different request schema detected as "changed"
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD02_ChangedRouteDetected) {
    registry().registerRoute(makeEntry("/api/v1/users", "GET", "listUsers", "List users"));

    std::string baseline = registry().captureSpecSnapshot();

    // Re-register with a different summary (simulates schema change)
    registry().registerRoute(makeEntry("/api/v1/users", "GET", "listUsers",
                                       "List users (v2 schema — updated)"));

    auto report = registry().detectDrift(baseline);
    EXPECT_TRUE(report.hasDrift())
        << "Schema change on an existing route must be detected";
    EXPECT_FALSE(report.changed.empty())
        << "Changed routes list must be non-empty";
    EXPECT_TRUE(report.added.empty())
        << "No new routes were registered; added list must be empty";
    EXPECT_TRUE(report.removed.empty())
        << "No routes were removed; removed list must be empty";
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-03: Path removed from registry detected as "removed"
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD03_RemovedRouteDetected) {
    registry().registerRoute(makeEntry("/api/v1/users",    "GET",    "listUsers"));
    registry().registerRoute(makeEntry("/api/v1/sessions", "DELETE", "deleteSession"));

    std::string baseline = registry().captureSpecSnapshot();

    // Simulate a route being removed by clearing and re-registering only one
    registry().clear();
    registry().registerRoute(makeEntry("/api/v1/users", "GET", "listUsers"));

    auto report = registry().detectDrift(baseline);
    EXPECT_TRUE(report.hasDrift())
        << "Removed route must be detected as drift";
    EXPECT_EQ(report.removed.size(), 1u)
        << "Exactly one route was removed";
    EXPECT_TRUE(report.added.empty())
        << "No new routes were added";
    EXPECT_TRUE(report.changed.empty())
        << "The remaining route was not changed";
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-04: Response schema change (operationId mutation) detected
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD04_OperationIdChangedDetected) {
    registry().registerRoute(makeEntry("/api/v1/items", "POST", "createItem"));

    std::string baseline = registry().captureSpecSnapshot();

    // Change operationId (response schema identifier mutation)
    registry().registerRoute(makeEntry("/api/v1/items", "POST", "createItemV2"));

    auto report = registry().detectDrift(baseline);
    EXPECT_TRUE(report.hasDrift())
        << "operationId change must be detected as drift";
    EXPECT_FALSE(report.changed.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-05: New path added without annotation detected as "added"
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD05_NewUnannotatedPathDetected) {
    registry().registerRoute(makeEntry("/api/v1/users", "GET", "listUsers"));

    std::string baseline = registry().captureSpecSnapshot();

    // Add a new route after snapshot — simulates a handler deployed without
    // a corresponding OpenAPI annotation in the baseline spec
    registry().registerRoute(makeEntry("/api/v1/internal/debug", "GET", "debugDump"));

    auto report = registry().detectDrift(baseline);
    EXPECT_TRUE(report.hasDrift())
        << "New undocumented route must be detected as drift";
    EXPECT_EQ(report.added.size(), 1u)
        << "Exactly one route was added";
    EXPECT_TRUE(report.removed.empty());
    EXPECT_TRUE(report.changed.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-06: Drift resolves after annotation is added post-registration
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD06_DriftResolvesAfterAnnotation) {
    registry().registerRoute(makeEntry("/api/v1/users", "GET", "listUsers"));
    registry().registerRoute(makeEntry("/api/v1/items", "GET", "listItems"));

    std::string baseline = registry().captureSpecSnapshot();

    // Add a new route (drift)
    registry().registerRoute(makeEntry("/api/v1/new-feature", "PUT", "newFeature"));
    auto report1 = registry().detectDrift(baseline);
    EXPECT_TRUE(report1.hasDrift())
        << "Drift must be present before annotation is completed";

    // Take a new baseline snapshot (annotation added — drift resolved)
    std::string new_baseline = registry().captureSpecSnapshot();
    auto report2 = registry().detectDrift(new_baseline);
    EXPECT_FALSE(report2.hasDrift())
        << "Drift must resolve once a new baseline snapshot is captured";
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-07: Thread-safe concurrent registration + snapshot (no races)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD07_ThreadSafeConcurrentRegistrationAndSnapshot) {
    constexpr int kThreads   = 8;
    constexpr int kPerThread = 10;

    std::atomic<int> errors{0};

    auto worker = [&](int thread_id) {
        for (int i = 0; i < kPerThread; ++i) {
            std::string path = "/api/v1/t" + std::to_string(thread_id)
                               + "/r" + std::to_string(i);
            try {
                registry().registerRoute(
                    makeEntry(path, "GET",
                              "op-" + std::to_string(thread_id) + "-" + std::to_string(i)));
                // Interleave snapshot captures
                if (i % 3 == 0) {
                    [[maybe_unused]] std::string snap = registry().captureSpecSnapshot();
                }
            } catch (...) {
                errors.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(errors.load(), 0)
        << "No exceptions or races must occur during concurrent registration and snapshot";

    // Final snapshot must be consistent (no partial state)
    std::string final_snap = registry().captureSpecSnapshot();
    auto report = registry().detectDrift(final_snap);
    EXPECT_FALSE(report.hasDrift())
        << "Snapshot taken immediately after concurrent writes must show no drift vs itself";
}

// ─────────────────────────────────────────────────────────────────────────────
// SOD-08: Spec roundtrip: registry → snapshot → compare → equality
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(OpenApiDriftTest, SOD08_SpecRoundtripSnapshotEquality) {
    // Register a set of routes
    const std::vector<std::tuple<std::string, std::string, std::string>> routes = {
        {"/api/v1/users",    "GET",    "listUsers"},
        {"/api/v1/users",    "POST",   "createUser"},
        {"/api/v1/users/{id}", "GET",  "getUser"},
        {"/api/v1/users/{id}", "PUT",  "updateUser"},
        {"/api/v1/users/{id}", "DELETE","deleteUser"},
    };
    for (const auto& [path, method, opid] : routes) {
        registry().registerRoute(makeEntry(path, method, opid));
    }

    std::string snap = registry().captureSpecSnapshot();

    // Snapshot must be non-empty
    EXPECT_FALSE(snap.empty())
        << "Snapshot must not be empty for a non-empty registry";

    // Comparing the snapshot to itself must show zero drift
    auto report = registry().detectDrift(snap);
    EXPECT_FALSE(report.hasDrift())
        << "Snapshot roundtrip must show no drift";

    // Snapshot must be deterministic: two calls must return identical strings
    std::string snap2 = registry().captureSpecSnapshot();
    EXPECT_EQ(snap, snap2)
        << "captureSpecSnapshot must be deterministic for an unchanged registry";
}
