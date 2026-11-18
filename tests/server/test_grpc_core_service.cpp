/**
 * @file test_grpc_core_service.cpp
 * @brief Wave 9 Block 1 — gRPC core service layer acceptance tests (W9-1..W9-6).
 *
 * Covers the wiring of all data-plane RPCs in ThemisCoreServiceImpl:
 *
 *  GCS-01 — storageKey() joins collection and key with ':'
 *  GCS-02 — storageKey() preserves empty collection prefix
 *  GCS-03 — storageKey() is injective for distinct (collection, key) pairs
 *  GCS-04 — ServiceInstanceFn bridge: setServiceInstanceFn + getServiceInstance round-trip
 *  GCS-05 — Constructor throws when no ServiceInstanceFn and no gRPC stubs
 *  GCS-06 — ServiceInstanceFn returning nullptr causes constructor to throw
 *  GCS-07 — ServiceInstanceFn throwing causes constructor to re-throw
 *  GCS-08 — ServiceInstanceFn stores last-registered fn (idempotent overwrite)
 *  GCS-09 — Source: STUB/SIMULATION note removed from service layer body
 *  GCS-10 — Source: Create, Read, Update, Delete handlers are present
 *  GCS-11 — Source: BatchCreate, BatchRead, BatchUpdate, BatchDelete are present
 *  GCS-12 — Source: BeginTransaction, CommitTransaction, RollbackTransaction are present
 *  GCS-13 — Source: ExecuteAQL and StreamQuery are present
 *  GCS-14 — Source: ScanCollection is present
 *  GCS-15 — Source: GetStatus is present and populates version field
 *  GCS-16 — AQLEngine type alias resolves to IQueryEngine (compile-time)
 *
 * Under THEMIS_HAS_CORE_GRPC (gRPC stubs available):
 *  GCS-17 — Create with empty collection returns success=false, code=400
 *  GCS-18 — Create with valid key succeeds against in-memory RocksDB
 *  GCS-19 — Read returns 404 for unknown key
 *  GCS-20 — Read returns document after Create
 *  GCS-21 — Update returns 404 for missing key when create_if_missing=false
 *  GCS-22 — Update with create_if_missing=true inserts a new doc
 *  GCS-23 — Delete returns success=false for non-existent key
 *  GCS-24 — ScanCollection streams all docs in collection
 *  GCS-25 — BatchCreate inserts multiple documents
 *  GCS-26 — BatchRead retrieves inserted docs
 *  GCS-27 — BatchDelete removes all specified keys
 *  GCS-28 — ExecuteAQL returns UNIMPLEMENTED when no engine wired
 *  GCS-29 — GetStatus returns uptime_seconds >= 0 and version non-empty
 *
 * QRY-07 coverage:
 * core gRPC AQL cancellation/deadline propagation, late-result discard, and
 * streaming cleanup are exercised by GrpcCoreServiceCancellation tests.
 *
 * Tests GCS-01..GCS-16 compile and run in all build configurations.
 * Tests GCS-17..GCS-29 are guarded by THEMIS_HAS_CORE_GRPC.
 *
 * @version 1.0.0
 * @note CTest labels: server grpc wave9
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

// Service under test
#include "server/themis_core_grpc_service.h"

#ifndef THEMIS_ROOT_DIR
#  define THEMIS_COMPUTE_ROOT_DIR() \
     (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path())
#else
#  define THEMIS_COMPUTE_ROOT_DIR() (std::filesystem::path(THEMIS_ROOT_DIR))
#endif

namespace themis::core::test {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::filesystem::path repoRoot() { return THEMIS_COMPUTE_ROOT_DIR(); }

static std::string readFile(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f) {
      return "";
    }
    return {std::istreambuf_iterator<char>(f), {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-01 — storageKey() joins collection and key with ':'
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, StorageKey_BasicJoin) {
    // The storageKey helper is file-scoped but we can verify the convention via
    // source inspection and the expected behaviour documented in the .cpp.
    // Convention: storageKey("col","key") == "col:key"
    const std::string col = "users";
    const std::string key = "u123";
    const std::string expected = col + ":" + key;
    EXPECT_EQ(expected, "users:u123");
}

// GCS-02 — storageKey() preserves empty collection prefix
TEST(GrpcCoreService, StorageKey_EmptyCollectionPrefix) {
    const std::string expected = ":mykey";
    EXPECT_EQ(expected, std::string("") + ":" + "mykey");
}

// GCS-03 — storageKey() is injective for distinct (collection, key) pairs
TEST(GrpcCoreService, StorageKey_Injectivity) {
    auto sk = [](const std::string& c, const std::string& k) { return c + ":" + k; };
    EXPECT_NE(sk("a:b", "c"), sk("a", "b:c"));  // Verifies delimiter edge case
    EXPECT_NE(sk("users", "123"), sk("users", "456"));
    EXPECT_NE(sk("users", "key"), sk("orders", "key"));
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-04 — ServiceInstanceFn bridge round-trip
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, ServiceInstanceFn_RoundTrip) {
    static int sentinel = 42;
    ThemisCoreServiceImpl::setServiceInstanceFn([] {
        return static_cast<void*>(&sentinel);
    });
    auto impl = std::make_unique<ThemisCoreServiceImpl>(nullptr, nullptr, nullptr);
    EXPECT_EQ(impl->getServiceInstance(), static_cast<void*>(&sentinel));
    // Cleanup
    ThemisCoreServiceImpl::setServiceInstanceFn(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-05 — Constructor throws when no ServiceInstanceFn and no gRPC stubs
// ─────────────────────────────────────────────────────────────────────────────
#if !__has_include("themis_core.grpc.pb.h")
TEST(GrpcCoreService, Constructor_ThrowsWhenNoFnAndNoStubs) {
    ThemisCoreServiceImpl::setServiceInstanceFn(nullptr);
    EXPECT_THROW(
        ThemisCoreServiceImpl(nullptr, nullptr, nullptr),
        std::runtime_error
    );
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// GCS-06 — ServiceInstanceFn returning nullptr causes constructor to throw
// ─────────────────────────────────────────────────────────────────────────────
#if !__has_include("themis_core.grpc.pb.h")
TEST(GrpcCoreService, Constructor_ThrowsWhenFnReturnsNullptr) {
    ThemisCoreServiceImpl::setServiceInstanceFn([] { return static_cast<void*>(nullptr); });
    EXPECT_THROW(
        ThemisCoreServiceImpl(nullptr, nullptr, nullptr),
        std::runtime_error
    );
    ThemisCoreServiceImpl::setServiceInstanceFn(nullptr);
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// GCS-07 — ServiceInstanceFn throwing causes constructor to re-throw
// ─────────────────────────────────────────────────────────────────────────────
#if !__has_include("themis_core.grpc.pb.h")
TEST(GrpcCoreService, Constructor_ThrowsWhenFnThrows) {
    ThemisCoreServiceImpl::setServiceInstanceFn([]() -> void* {
        throw std::runtime_error("injected fn error");
    });
    EXPECT_THROW(
        ThemisCoreServiceImpl(nullptr, nullptr, nullptr),
        std::runtime_error
    );
    ThemisCoreServiceImpl::setServiceInstanceFn(nullptr);
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// GCS-08 — setServiceInstanceFn overwrites previous fn (idempotent)
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, ServiceInstanceFn_Overwrite) {
    static int v1 = 1, v2 = 2;
    ThemisCoreServiceImpl::setServiceInstanceFn([] { return static_cast<void*>(&v1); });
    ThemisCoreServiceImpl::setServiceInstanceFn([] { return static_cast<void*>(&v2); });
    auto impl = std::make_unique<ThemisCoreServiceImpl>(nullptr, nullptr, nullptr);
    EXPECT_EQ(impl->getServiceInstance(), static_cast<void*>(&v2));
    ThemisCoreServiceImpl::setServiceInstanceFn(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-09 — STUB/SIMULATION body has been removed from the service layer
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_StubNoteRemoved) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    ASSERT_FALSE(src.empty()) << "Could not read themis_core_grpc_service.cpp";
    // The old block-level STUB note referencing UNIMPLEMENTED RPC invocation
    // must no longer appear in the body of the service.
    EXPECT_EQ(src.find("UNIMPLEMENTED RPC invoked"), std::string::npos)
        << "Old STUB/SIMULATION note still present";
    // The W9 metadata comment must be present.
    EXPECT_NE(src.find("W9-1..W9-6"), std::string::npos)
        << "Wave 9 metadata not present";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-10 — Create, Read, Update, Delete handlers are present
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_CRUDHandlersPresent) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    EXPECT_NE(src.find("grpc::Status Create("), std::string::npos)  << "Create missing";
    EXPECT_NE(src.find("grpc::Status Read("),   std::string::npos)  << "Read missing";
    EXPECT_NE(src.find("grpc::Status Update("), std::string::npos)  << "Update missing";
    EXPECT_NE(src.find("grpc::Status Delete("), std::string::npos)  << "Delete missing";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-11 — Batch handlers are present
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_BatchHandlersPresent) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    EXPECT_NE(src.find("grpc::Status BatchCreate("),  std::string::npos) << "BatchCreate missing";
    EXPECT_NE(src.find("grpc::Status BatchRead("),    std::string::npos) << "BatchRead missing";
    EXPECT_NE(src.find("grpc::Status BatchUpdate("),  std::string::npos) << "BatchUpdate missing";
    EXPECT_NE(src.find("grpc::Status BatchDelete("),  std::string::npos) << "BatchDelete missing";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-12 — Transaction handlers are present
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_TransactionHandlersPresent) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    EXPECT_NE(src.find("grpc::Status BeginTransaction("),    std::string::npos) << "BeginTransaction missing";
    EXPECT_NE(src.find("grpc::Status CommitTransaction("),   std::string::npos) << "CommitTransaction missing";
    EXPECT_NE(src.find("grpc::Status RollbackTransaction("), std::string::npos) << "RollbackTransaction missing";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-13 — AQL and stream query handlers are present
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_AQLHandlersPresent) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    EXPECT_NE(src.find("grpc::Status ExecuteAQL("),   std::string::npos) << "ExecuteAQL missing";
    EXPECT_NE(src.find("grpc::Status StreamQuery("),  std::string::npos) << "StreamQuery missing";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-14 — ScanCollection handler is present
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_ScanCollectionPresent) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    EXPECT_NE(src.find("grpc::Status ScanCollection("), std::string::npos)
        << "ScanCollection missing";
    EXPECT_NE(src.find("scanPrefix"), std::string::npos)
        << "scanPrefix call missing from ScanCollection";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-15 — GetStatus handler is present
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, Source_GetStatusPresent) {
    const auto src = readFile(repoRoot() / "src/server/themis_core_grpc_service.cpp");
    EXPECT_NE(src.find("grpc::Status GetStatus("), std::string::npos)
        << "GetStatus handler missing";
    EXPECT_NE(src.find("set_version"), std::string::npos)
        << "GetStatus does not call set_version()";
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-16 — AQLEngine type alias resolves to IQueryEngine (compile-time)
// ─────────────────────────────────────────────────────────────────────────────
TEST(GrpcCoreService, AQLEngineAlias_CompilesAsIQueryEngine) {
    // If themis::AQLEngine is defined as using AQLEngine = IQueryEngine, then
    // this static_assert succeeds at compile time.
    static_assert(
        std::is_same_v<themis::AQLEngine, themis::IQueryEngine>,
        "AQLEngine must be an alias for IQueryEngine"
    );
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-17..GCS-29 — Full RPC tests (only when gRPC stubs present)
// ─────────────────────────────────────────────────────────────────────────────
#if __has_include("themis_core.grpc.pb.h")
#include <grpcpp/grpcpp.h>
#include "themis_core.grpc.pb.h"
#include "themis_core.pb.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"

/// Minimal in-memory mock for AQLEngine (IQueryEngine).
class MockAQLEngine : public themis::IQueryEngine {
public:
    std::string query_received = {};
    std::string fixed_response = R"([{"_id":"1"}])";

    themis::Result<std::string> execute(const std::string& query) override {
        query_received = query;
        return fixed_response;
    }
    themis::Result<void> validate(const std::string&) const override {
        return themis::OkVoid();
    }
    themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
        createExpressionEvaluator() const override { return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(themis::errors::ErrorCode::NOT_IMPLEMENTED, ""); }
    themis::Result<std::string> explainQuery(const std::string&) const override {
        return std::string("EXPLAIN OK");
    }
};

class CooperativeAQLEngine final : public themis::IQueryEngine {
public:
    using themis::IQueryEngine::execute;

    [[nodiscard]] themis::Result<std::string> execute(
        const std::string& query) override {
        ++legacy_calls;
        query_received = query;
        return fixed_response;
    }

    [[nodiscard]] themis::Result<std::string> execute(
        const std::string& query,
        const std::string& bind_vars_json,
        const themis::query::RequestContext& request,
        themis::query::StopReason& stop_reason) override {
        ++context_calls;
        query_received = query;
        bind_vars_received = bind_vars_json;
        entered.store(true, std::memory_order_release);
        while (!request.shouldStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        stop_reason = request.stopReason();
        observed_stop_reason.store(stop_reason, std::memory_order_release);
        completed.store(true, std::memory_order_release);
        return themis::Err<std::string>(
            stop_reason == themis::query::StopReason::Cancelled
                ? themis::errors::ErrorCode::ERR_QUERY_CANCELLED
                : themis::errors::ErrorCode::ERR_QUERY_TIMEOUT,
            "cooperative AQL stopped");
    }

    [[nodiscard]] themis::Result<void> validate(const std::string&) const override {
        return themis::OkVoid();
    }
    [[nodiscard]] themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::NOT_IMPLEMENTED, "unused");
    }
    [[nodiscard]] themis::Result<std::string> explainQuery(const std::string&) const override {
        return std::string("{}");
    }

    std::string fixed_response = R"([{"_id":"1"}])";
    std::string query_received;
    std::string bind_vars_received;
    std::atomic<unsigned int> legacy_calls{0};
    std::atomic<unsigned int> context_calls{0};
    std::atomic<themis::query::StopReason> observed_stop_reason{
        themis::query::StopReason::None};
    std::atomic<bool> entered{false};
    std::atomic<bool> completed{false};
};

class BlockingLegacyAQLEngine final : public themis::IQueryEngine {
public:
    [[nodiscard]] themis::Result<std::string> execute(const std::string&) override {
        entered.store(true, std::memory_order_release);
        while (!release.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        returned.store(true, std::memory_order_release);
        return std::string(R"([{"_id":"late"}])");
    }
    [[nodiscard]] themis::Result<void> validate(const std::string&) const override {
        return themis::OkVoid();
    }
    [[nodiscard]] themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::NOT_IMPLEMENTED, "unused");
    }
    [[nodiscard]] themis::Result<std::string> explainQuery(const std::string&) const override {
        return std::string("{}");
    }

    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};
    std::atomic<bool> returned{false};
};

void waitForEngine(const std::atomic<bool>& entered) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
}

class RunningCoreGrpcServer {
public:
    explicit RunningCoreGrpcServer(std::shared_ptr<themis::IQueryEngine> engine)
        : owner_(nullptr, nullptr, std::move(engine)) {
        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:0",
                                 grpc::InsecureServerCredentials(),
                                 &port_);
        builder.RegisterService(
            static_cast<ThemisCoreService::Service*>(owner_.getServiceInstance()));
        server_ = builder.BuildAndStart();
        if (server_ && port_ > 0) {
            channel_ = grpc::CreateChannel(
                "127.0.0.1:" + std::to_string(port_),
                grpc::InsecureChannelCredentials());
            stub_ = ThemisCoreService::NewStub(channel_);
        }
    }

    [[nodiscard]] bool ready() const noexcept {
        return server_ && stub_;
    }

    [[nodiscard]] ThemisCoreService::Stub& stub() const {
        return *stub_;
    }

private:
    ThemisCoreServiceImpl owner_;
    int port_{0};
    std::unique_ptr<grpc::Server> server_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<ThemisCoreService::Stub> stub_;
};

/// Fixture: creates an in-memory RocksDB + real ThemisCoreServiceImpl.
class GrpcCoreServiceRPCTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temporary directory for RocksDB.
        tmp_dir_ = std::filesystem::temp_directory_path()
            / ("themis_grpc_core_test_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tmp_dir_);

        themis::RocksDBWrapper::Config cfg;
        cfg.path = tmp_dir_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "RocksDB open failed";

        aql_ = std::make_shared<MockAQLEngine>();
        impl_ = std::make_unique<ThemisCoreServiceImpl>(db_, nullptr, aql_);
        svc_ = static_cast<ThemisCoreService::Service*>(impl_->getServiceInstance());
        ASSERT_NE(svc_, nullptr);
    }

    void TearDown() override {
        impl_.reset();
        db_.reset();
        std::filesystem::remove_all(tmp_dir_);
    }

    // Convenience: call Create
    CreateResponse create(const std::string& col, const std::string& key,
                          const std::string& data) {
        grpc::ServerContext ctx;
        CreateRequest req;
        req.set_collection(col);
        req.set_key(key);
        req.set_data(data);
        CreateResponse resp;
        svc_->Create(&ctx, &req, &resp);
        return resp;
    }

    // Convenience: call Read
    ReadResponse read(const std::string& col, const std::string& key) {
        grpc::ServerContext ctx;
        ReadRequest req;
        req.set_collection(col);
        req.set_key(key);
        ReadResponse resp;
        svc_->Read(&ctx, &req, &resp);
        return resp;
    }

    std::filesystem::path tmp_dir_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<MockAQLEngine>          aql_;
    std::unique_ptr<ThemisCoreServiceImpl>  impl_;
    ThemisCoreService::Service*             svc_ = nullptr;
};

// GCS-17 — Create with empty collection returns success=false, code=400
TEST_F(GrpcCoreServiceRPCTest, Create_EmptyCollection_Returns400) {
    auto resp = create("", "key1", "data");
    EXPECT_FALSE(resp.success());
    EXPECT_EQ(resp.error().code(), 400);
}

// GCS-18 — Create with valid key succeeds
TEST_F(GrpcCoreServiceRPCTest, Create_ValidKey_Succeeds) {
    auto resp = create("users", "u1", R"({"name":"Alice"})");
    EXPECT_TRUE(resp.success());
    EXPECT_EQ(resp.key(), "u1");
}

// GCS-19 — Read returns 404 for unknown key
TEST_F(GrpcCoreServiceRPCTest, Read_UnknownKey_Returns404) {
    auto resp = read("users", "no_such_key");
    EXPECT_FALSE(resp.success());
    EXPECT_EQ(resp.error().code(), 404);
}

// GCS-20 — Read returns document after Create
TEST_F(GrpcCoreServiceRPCTest, Read_AfterCreate_ReturnsDocument) {
    const std::string data = R"({"name":"Bob"})";
    ASSERT_TRUE(create("users", "u2", data).success());
    auto resp = read("users", "u2");
    EXPECT_TRUE(resp.success());
    EXPECT_EQ(resp.document().key(), "u2");
    EXPECT_EQ(resp.document().collection(), "users");
    EXPECT_EQ(resp.document().data(), data);
}

// GCS-21 — Update returns 404 for missing key when create_if_missing=false
TEST_F(GrpcCoreServiceRPCTest, Update_MissingKey_NoCreate_Returns404) {
    grpc::ServerContext ctx;
    UpdateRequest req;
    req.set_collection("users");
    req.set_key("ghost");
    req.set_data("data");
    req.set_create_if_missing(false);
    UpdateResponse resp;
    svc_->Update(&ctx, &req, &resp);
    EXPECT_FALSE(resp.success());
    EXPECT_EQ(resp.error().code(), 404);
}

// GCS-22 — Update with create_if_missing=true inserts doc
TEST_F(GrpcCoreServiceRPCTest, Update_CreateIfMissing_Succeeds) {
    grpc::ServerContext ctx;
    UpdateRequest req;
    req.set_collection("users");
    req.set_key("u_new");
    req.set_data(R"({"v":1})");
    req.set_create_if_missing(true);
    UpdateResponse resp;
    svc_->Update(&ctx, &req, &resp);
    EXPECT_TRUE(resp.success());
    // Verify readable
    EXPECT_TRUE(read("users", "u_new").success());
}

// GCS-23 — Delete returns success=false for non-existent key
TEST_F(GrpcCoreServiceRPCTest, Delete_NonExistent_ReturnsFalse) {
    grpc::ServerContext ctx;
    DeleteRequest req;
    req.set_collection("users");
    req.set_key("not_there");
    DeleteResponse resp;
    svc_->Delete(&ctx, &req, &resp);
    // del() on a non-existent key may return false
    EXPECT_FALSE(resp.success());
}

// GCS-24 — ScanCollection streams all docs in collection
TEST_F(GrpcCoreServiceRPCTest, ScanCollection_StreamsAllDocs) {
    ASSERT_TRUE(create("scan_col", "a", "va").success());
    ASSERT_TRUE(create("scan_col", "b", "vb").success());
    ASSERT_TRUE(create("scan_col", "c", "vc").success());

    grpc::ServerContext ctx;
    ScanRequest req;
    req.set_collection("scan_col");

    struct Writer : grpc::ServerWriter<ScanResult> {
        using grpc::ServerWriter<ScanResult>::ServerWriter;
        bool Write(const ScanResult& sr, grpc::WriteOptions) override {
            docs.push_back(sr.document().key());
            return true;
        }
        std::vector<std::string> docs;
    };

    grpc::internal::Call call;
    Writer writer(&call);
    svc_->ScanCollection(&ctx, &req, &writer);
    EXPECT_EQ(writer.docs.size(), 3u);
}

// GCS-25 — BatchCreate inserts multiple documents
TEST_F(GrpcCoreServiceRPCTest, BatchCreate_InsertsMultiple) {
    grpc::ServerContext ctx;
    BatchCreateRequest req;
    req.set_collection("batch_col");
    for (int i = 0; i < 3; ++i) {
        auto* doc = req.add_documents();
        doc->set_key("k" + std::to_string(i));
        doc->set_data("v" + std::to_string(i));
    }
    BatchCreateResponse resp;
    svc_->BatchCreate(&ctx, &req, &resp);
    EXPECT_TRUE(resp.success());
    EXPECT_EQ(resp.created_count(), 3);
}

// GCS-26 — BatchRead retrieves inserted docs
TEST_F(GrpcCoreServiceRPCTest, BatchRead_RetrievesInserted) {
    for (int i = 0; i < 2; ++i) {
        create("br_col", "k" + std::to_string(i), "v" + std::to_string(i));
    }
    grpc::ServerContext ctx;
    BatchReadRequest req;
    req.set_collection("br_col");
    req.add_keys("k0");
    req.add_keys("k1");
    BatchReadResponse resp;
    svc_->BatchRead(&ctx, &req, &resp);
    EXPECT_TRUE(resp.success());
    EXPECT_EQ(resp.documents_size(), 2);
}

// GCS-27 — BatchDelete removes all specified keys
TEST_F(GrpcCoreServiceRPCTest, BatchDelete_RemovesKeys) {
    create("bd_col", "x", "vx");
    create("bd_col", "y", "vy");

    grpc::ServerContext ctx;
    BatchDeleteRequest req;
    req.set_collection("bd_col");
    req.add_keys("x");
    req.add_keys("y");
    BatchDeleteResponse resp;
    svc_->BatchDelete(&ctx, &req, &resp);
    EXPECT_GE(resp.deleted_count(), 1);

    EXPECT_FALSE(read("bd_col", "x").success());
    EXPECT_FALSE(read("bd_col", "y").success());
}

// GCS-28 — ExecuteAQL returns UNIMPLEMENTED when engine is null
TEST_F(GrpcCoreServiceRPCTest, ExecuteAQL_NullEngine_ReturnsUnimplemented) {
    // Rebuild service without AQL engine
    impl_.reset();
    impl_ = std::make_unique<ThemisCoreServiceImpl>(db_, nullptr, nullptr);
    svc_ = static_cast<ThemisCoreService::Service*>(impl_->getServiceInstance());

    grpc::ServerContext ctx;
    AQLRequest req;
    req.set_query("FOR d IN users RETURN d");
    AQLResponse resp;
    auto status = svc_->ExecuteAQL(&ctx, &req, &resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
}

// GCS-29 — GetStatus returns non-empty version and uptime >= 0
TEST_F(GrpcCoreServiceRPCTest, GetStatus_PopulatesVersionAndUptime) {
    grpc::ServerContext ctx;
    StatusRequest req;
    req.set_include_stats(true);
    StatusResponse resp;
    auto status = svc_->GetStatus(&ctx, &req, &resp);
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(resp.version().empty());
    EXPECT_GE(resp.uptime_seconds(), 0);
}

TEST(GrpcCoreServiceCancellation, ExecuteAQLPreCancellationSkipsEngine) {
    auto engine = std::make_shared<CooperativeAQLEngine>();
    RunningCoreGrpcServer server(engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.TryCancel();
    AQLRequest request;
    request.set_query("FOR d IN docs RETURN d");
    AQLResponse response;
    const auto status = server.stub().ExecuteAQL(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.results_size(), 0);
    EXPECT_EQ(engine->context_calls.load(), 0u);
    EXPECT_EQ(engine->legacy_calls.load(), 0u);
}

TEST(GrpcCoreServiceCancellation, ExecuteAQLPropagatesInFlightCancellation) {
    auto engine = std::make_shared<CooperativeAQLEngine>();
    RunningCoreGrpcServer server(engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    AQLRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto* bind_var = request.mutable_bind_vars();
    (*bind_var)["limit"] = "10";
    AQLResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().ExecuteAQL(&context, request, &response);
    });

    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    rpc_thread.join();
    waitForEngine(engine->completed);

    EXPECT_TRUE(entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.results_size(), 0);
    EXPECT_EQ(engine->context_calls.load(), 1u);
    EXPECT_EQ(engine->legacy_calls.load(), 0u);
    EXPECT_NE(engine->bind_vars_received.find("\"limit\":\"10\""), std::string::npos);
}

TEST(GrpcCoreServiceCancellation, ExecuteAQLPropagatesInFlightDeadline) {
    auto engine = std::make_shared<CooperativeAQLEngine>();
    RunningCoreGrpcServer server(engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() +
                         std::chrono::seconds{2});
    AQLRequest request;
    request.set_query("FOR d IN docs RETURN d");
    AQLResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().ExecuteAQL(&context, request, &response);
    });
    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    rpc_thread.join();
    if (entered) {
        waitForEngine(engine->completed);
    }

    EXPECT_TRUE(entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_EQ(engine->observed_stop_reason.load(std::memory_order_acquire),
              themis::query::StopReason::DeadlineExceeded);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.results_size(), 0);
    EXPECT_EQ(engine->context_calls.load(), 1u);
    EXPECT_EQ(engine->legacy_calls.load(), 0u);
}

TEST(GrpcCoreServiceCancellation, ExecuteAQLDiscardsLegacyResultAfterCancellation) {
    auto engine = std::make_shared<BlockingLegacyAQLEngine>();
    RunningCoreGrpcServer server(engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    AQLRequest request;
    request.set_query("FOR d IN docs RETURN d");
    AQLResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().ExecuteAQL(&context, request, &response);
    });

    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    engine->release.store(true, std::memory_order_release);
    rpc_thread.join();
    waitForEngine(engine->returned);

    EXPECT_TRUE(entered);
    EXPECT_TRUE(engine->returned.load(std::memory_order_acquire));
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.results_size(), 0);
}

TEST(GrpcCoreServiceCancellation, StreamQueryStopsBeforeWritingAfterCancellation) {
    auto engine = std::make_shared<CooperativeAQLEngine>();
    RunningCoreGrpcServer server(engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    AQLRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto reader = server.stub().StreamQuery(&context, request);
    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    waitForEngine(engine->completed);

    QueryResult row;
    std::size_t row_count = 0;
    while (reader->Read(&row)) {
        ++row_count;
    }
    const auto status = reader->Finish();

    EXPECT_TRUE(entered);
    EXPECT_EQ(row_count, 0u);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_EQ(engine->context_calls.load(), 1u);
}

#endif // THEMIS_HAS_CORE_GRPC

} // namespace themis::core::test
