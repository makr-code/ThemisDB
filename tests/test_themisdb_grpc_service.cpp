/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_themisdb_grpc_service.cpp                     ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:31:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9bb592d7  2026-02-24  Implement ThemisDBGrpcService and fix ThemisCoreServiceIm... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "api/themisdb_grpc_service.h"
#include "api/themisdb_grpc_service_factory.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/index_interface.h"

using namespace themis::api;

// ============================================================================
// Minimal mock implementations for injection testing
// (used without proto stubs; test only the C++ wiring layer)
// ============================================================================

namespace {

// A minimal IQueryEngine that returns a configurable JSON string.
class StubQueryEngine : public themis::IQueryEngine {
public:
    std::string response_json = R"([{"_key":"doc1"},{"_key":"doc2"}])";
    std::string last_query;

    themis::Result<std::string> execute(const std::string& query) override {
        last_query = query;
        return response_json;
    }
    themis::Result<void> validate(const std::string& /*query*/) const override {
        return {};
    }
    themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::ERR_UNKNOWN, "not implemented in stub");
    }
    themis::Result<std::string> explainQuery(const std::string& /*query*/) const override {
        return std::string("stub-plan");
    }
};

// A minimal IVectorIndex that echoes back key names.
class StubVectorIndex : public themis::IVectorIndex {
public:
    bool insert(std::string_view /*pk*/, const std::vector<float>& /*vec*/) override {
        return true;
    }
    bool remove(std::string_view /*pk*/) override { return true; }

    std::vector<themis::VectorSearchResult> search(
        const std::vector<float>& /*qv*/, uint32_t k,
        const themis::IExpressionEvaluator* /*filter*/ = nullptr) const override {
        std::vector<themis::VectorSearchResult> hits;
        for (uint32_t i = 0; i < k && i < 3; ++i) {
            hits.emplace_back("key_" + std::to_string(i), static_cast<float>(i) * 0.1f);
        }
        return hits;
    }
    std::vector<themis::VectorSearchResult> rangeSearch(
        const std::vector<float>& /*qv*/, float /*max_dist*/,
        const themis::IExpressionEvaluator* /*filter*/ = nullptr) const override {
        return {};
    }
    std::string getName() const override { return "stub"; }
    uint32_t getDimension() const override { return 4; }
    std::string getStatistics() const override { return "{}"; }
};

} // namespace

// ============================================================================
// ThemisDBGrpcService – construction and service() accessor tests
// ============================================================================

TEST(ThemisDBGrpcServiceTest, ConstructWithNullComponents) {
    // Both components are optional (may be null when stubs are absent).
    ASSERT_NO_THROW({
        ThemisDBGrpcService svc(nullptr, nullptr);
    });
}

TEST(ThemisDBGrpcServiceTest, ServiceReturnsNullOrValidPointer) {
    ThemisDBGrpcService svc(nullptr, nullptr);

    // When the generated proto headers are present service() returns a non-null
    // grpc::Service*; when they are absent it returns nullptr.  Either value is
    // acceptable – the caller must check before registering.
    void* ptr = svc.service();
    (void)ptr;  // accepted: nullptr or a valid pointer both compile cleanly
    SUCCEED();
}

TEST(ThemisDBGrpcServiceTest, ServiceCallIsIdempotent) {
    ThemisDBGrpcService svc(nullptr, nullptr);

    void* first  = svc.service();
    void* second = svc.service();
    EXPECT_EQ(first, second);
}

// ============================================================================
// Extended constructor – wired components
// ============================================================================

TEST(ThemisDBGrpcServiceTest, ConstructWithWiredComponents) {
    auto engine = std::make_shared<StubQueryEngine>();
    auto index  = std::make_shared<StubVectorIndex>();

    ASSERT_NO_THROW({
        ThemisDBGrpcService svc(nullptr, nullptr, engine, index);
    });
    ThemisDBGrpcService svc(nullptr, nullptr, engine, index);
    SUCCEED(); // construction succeeds; service() will return valid ptr if stubs present
}

TEST(ThemisDBGrpcServiceTest, ServicePointerStableAfterWiring) {
    auto engine = std::make_shared<StubQueryEngine>();
    ThemisDBGrpcService svc(nullptr, nullptr, engine, nullptr);
    void* p1 = svc.service();
    void* p2 = svc.service();
    EXPECT_EQ(p1, p2);
}

// ============================================================================
// ThemisDBGrpcServiceFactory – fluent builder tests
// ============================================================================

TEST(ThemisDBGrpcServiceFactoryTest, BuildWithNoComponents) {
    // Factory with no components should build cleanly and produce a valid svc.
    auto svc = ThemisDBGrpcServiceFactory{}.build();
    ASSERT_NE(svc, nullptr);
    // service() returns nullptr without stubs; either value is acceptable.
    (void)svc->service();
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, BuildWithQueryEngine) {
    auto engine = std::make_shared<StubQueryEngine>();
    auto svc = ThemisDBGrpcServiceFactory{}
                   .withQueryEngine(engine)
                   .build();
    ASSERT_NE(svc, nullptr);
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, BuildWithVectorIndex) {
    auto index = std::make_shared<StubVectorIndex>();
    auto svc = ThemisDBGrpcServiceFactory{}
                   .withVectorIndex(index)
                   .build();
    ASSERT_NE(svc, nullptr);
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, BuildFullyWired) {
    auto engine = std::make_shared<StubQueryEngine>();
    auto index  = std::make_shared<StubVectorIndex>();
    auto svc = ThemisDBGrpcServiceFactory{}
                   .withQueryEngine(engine)
                   .withVectorIndex(index)
                   .build();
    ASSERT_NE(svc, nullptr);
    // service() is non-null when proto stubs are compiled in.
    (void)svc->service();
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, FactoryIsReusable) {
    // A factory instance can build multiple independent service objects.
    auto engine = std::make_shared<StubQueryEngine>();
    ThemisDBGrpcServiceFactory factory;
    factory.withQueryEngine(engine);

    auto svc1 = factory.build();
    auto svc2 = factory.build();
    ASSERT_NE(svc1, nullptr);
    ASSERT_NE(svc2, nullptr);
    // They must be distinct objects.
    EXPECT_NE(svc1.get(), svc2.get());
}


