/*
 * ThemisDB | File: themis_core_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 165
 * Open Issues: TODOs=1, Stubs=3, Gaps=8, Unimpl=2, Mock=1, Sim=1, Debt=0
 * Gap Correlation: internal=8 | external_v3=31 | delta=23 | status=divergent
 * External Severity (v3): C=2, H=21, M=8
 * PR: #3632 fix(build): register 40+ missing sources across 7 modules in cmake ... (2026-03-12T07:39:41Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/themis_core_grpc_service.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "utils/logger.h"
#include <atomic>
#include <chrono>
#include <exception>
#include <mutex>
#include <string>
#include <utility>

// Conditionally compile the real service implementation when the protobuf
// stubs generated from proto/themis_core.proto are available on the include
// path.  This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
#if __has_include("themis_core.grpc.pb.h")
#  include <grpcpp/grpcpp.h>
#  include "themis_core.grpc.pb.h"
#  include "themis_core.pb.h"
#  define THEMIS_HAS_CORE_GRPC 1
#else
#  define THEMIS_HAS_CORE_GRPC 0
#endif

namespace themis {
namespace core {

namespace {
std::mutex g_core_grpc_instance_mutex;
ThemisCoreServiceImpl::ServiceInstanceFn g_core_grpc_instance_fn;
} // namespace

class ThemisCoreServiceImpl::Impl {
public:
#if THEMIS_HAS_CORE_GRPC

    Impl(std::shared_ptr<RocksDBWrapper>     db,
         std::shared_ptr<TransactionManager> txn_mgr,
         std::shared_ptr<AQLEngine>          aql_engine)
        : service_(std::move(db), std::move(txn_mgr), std::move(aql_engine)) {}

    themis::core::ThemisCoreService::Service* get() { return &service_; }

private:
    class ServiceImpl final : public themis::core::ThemisCoreService::Service {
    public:
        ServiceImpl(std::shared_ptr<RocksDBWrapper>     db,
                    std::shared_ptr<TransactionManager> txn_mgr,
                    std::shared_ptr<AQLEngine>          aql_engine)
            : db_(std::move(db))
            , txn_mgr_(std::move(txn_mgr))
            , aql_engine_(std::move(aql_engine))
            , start_time_(std::chrono::steady_clock::now()) {}

        grpc::Status HealthCheck(
            grpc::ServerContext*          /*ctx*/,
            const HealthCheckRequest*     req,
            HealthCheckResponse*          resp
        ) override {
            resp->set_status(HealthCheckResponse::HEALTHY);
            resp->set_message("ThemisDB core gRPC service is running");
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time_).count();
            resp->set_uptime_seconds(uptime);
            if (req->include_details()) {
                (*resp->mutable_details())["storage"] = db_      ? "ok" : "unavailable";
                (*resp->mutable_details())["txn_mgr"] = txn_mgr_ ? "ok" : "unavailable";
                (*resp->mutable_details())["aql"]     = aql_engine_ ? "ok" : "unavailable";
            }
            return grpc::Status::OK;
        }

        // All other RPCs (Create, Read, Update, Delete, Batch*, Transaction*,
        // ExecuteAQL, StreamQuery, ScanCollection, GetStatus) return
        // UNIMPLEMENTED until the full service layer is wired in.
        // The method stubs are automatically provided by the generated
        // ThemisCoreService::Service base class.

    private:
        std::shared_ptr<RocksDBWrapper>     db_;
        std::shared_ptr<TransactionManager> txn_mgr_;
        std::shared_ptr<AQLEngine>          aql_engine_;
        std::chrono::steady_clock::time_point start_time_;
    };

    ServiceImpl service_;

#endif // THEMIS_HAS_CORE_GRPC
};

ThemisCoreServiceImpl::ThemisCoreServiceImpl(
    std::shared_ptr<RocksDBWrapper>    db,
    std::shared_ptr<TransactionManager> txn_mgr,
    std::shared_ptr<AQLEngine>          aql_engine)
    : db_(std::move(db))
    , txn_mgr_(std::move(txn_mgr))
    , aql_engine_(std::move(aql_engine))
{
#if THEMIS_HAS_CORE_GRPC
    impl_ = std::make_unique<Impl>(db_, txn_mgr_, aql_engine_);
    THEMIS_INFO("ThemisCoreServiceImpl: initialized with gRPC protocol support");
#else
    // STUB/SIMULATION NOTE:
    // Purpose: Allow ThemisCoreServiceImpl to be constructed and linked without
    //   the generated protobuf/gRPC stub files for themis_core.proto.  The
    //   service instance is null; getServiceInstance() returns nullptr so the
    //   gRPC server simply omits this service from its handler list.
    // Activation: THEMIS_HAS_CORE_GRPC == 0 (default when protoc has not been
    //   run against proto/themis_core.proto, or when the generated
    //   themis_core.grpc.pb.h is not on the include path).
    // Production Delta: ThemisCoreService is completely absent from the gRPC
    //   server; clients calling any method on this service receive
    //   UNIMPLEMENTED.  All database, transaction, and AQL operations
    //   exposed by ThemisCoreService are inaccessible via gRPC.
    // Removal Plan: Run `cmake -DTHEMIS_ENABLE_GRPC=ON` with protoc installed
    //   so that themis_core.grpc.pb.{h,cc} are generated.  Re-enable the
    //   THEMIS_HAS_CORE_GRPC=1 path; this #else block becomes dead code.
    // Roadmap ref: src/server/FUTURE_ENHANCEMENTS.md §"gRPC Core Service Activation"
    THEMIS_WARN("ThemisCoreServiceImpl: themis_core.grpc.pb.h not found; "
                "service will be a no-op until protoc generates the stubs");

    // Try injected accessor (for non-proto builds wiring an external service).
    ServiceInstanceFn fn;
    {
        std::lock_guard<std::mutex> lock(g_core_grpc_instance_mutex);
        fn = g_core_grpc_instance_fn;
    }
    if (fn) {
        try {
            service_ptr_ = fn();
        } catch (const std::exception& e) {
            THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: {}", e.what());
            service_ptr_ = nullptr;
        }
    }
#endif
}

ThemisCoreServiceImpl::~ThemisCoreServiceImpl() = default;

void ThemisCoreServiceImpl::setServiceInstanceFn(ServiceInstanceFn fn) {
    std::lock_guard<std::mutex> lock(g_core_grpc_instance_mutex);
    g_core_grpc_instance_fn = std::move(fn);
}

void* ThemisCoreServiceImpl::getServiceInstance() {
#if THEMIS_HAS_CORE_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    return service_ptr_;
#endif
}

} // namespace core
} // namespace themis
