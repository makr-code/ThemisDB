/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_core_grpc_service.cpp                       ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:30:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     135                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9bb592d7b  2026-02-24  Implement ThemisDBGrpcService and fix ThemisCoreServiceIm... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/themis_core_grpc_service.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "utils/logger.h"
#include <atomic>
#include <chrono>
#include <string>

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
    THEMIS_WARN("ThemisCoreServiceImpl: themis_core.grpc.pb.h not found; "
                "service will be a no-op until protoc generates the stubs");
#endif
}

ThemisCoreServiceImpl::~ThemisCoreServiceImpl() = default;

void* ThemisCoreServiceImpl::getServiceInstance() {
#if THEMIS_HAS_CORE_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    // proto stubs not generated; returning null is expected here
    void* no_service = nullptr;
    return no_service;
#endif
}

} // namespace core
} // namespace themis
