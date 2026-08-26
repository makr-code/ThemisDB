/**
 * @file themis_core_grpc_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=2, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/themis_core_grpc_service.h"
#include <stdexcept>
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

/** @brief Implementation detail. */
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

        // STUB/SIMULATION NOTE:
        // Purpose: All non-Ping RPCs (Create, Read, Update, Delete, Batch*,
        //          Transaction*, ExecuteAQL, StreamQuery, ScanCollection,
        //          GetStatus) are not yet implemented in the service layer.
        //          The generated ThemisCoreService::Service base class returns
        //          gRPC UNIMPLEMENTED for each method automatically.
        // Activation: Compiled whenever THEMIS_HAS_CORE_GRPC is defined
        //             (i.e., the protobuf/gRPC stubs for themis_core.proto are
        //             on the include path and the gRPC SDK is linked).
        // Production Delta: Any gRPC client calling these methods receives
        //                   status UNIMPLEMENTED (code 12).  HTTP/REST APIs
        //                   backed by this service will not function for
        //                   data-plane operations.
        // Removal Plan: Wire real service logic (storage + transaction +
        //               AQL dispatch) per method, targeting v1.7.0 / Q4 2026.
        THEMIS_WARN("ThemisCoreServiceImpl: UNIMPLEMENTED RPC invoked — "
                    "service layer not yet wired (see STUB/SIMULATION NOTE in "
                    "themis_core_grpc_service.cpp:88)");

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
    ServiceInstanceFn fn;
    {
        std::lock_guard<std::mutex> lock(g_core_grpc_instance_mutex);
        fn = g_core_grpc_instance_fn;
    }
    if (!fn) {
        const std::string error =
            "ThemisCoreServiceImpl requires generated core gRPC stubs or an "
            "injected non-null ServiceInstanceFn in non-proto builds";
        THEMIS_CRITICAL("{}", error);
        throw std::runtime_error(error);
    }
    try {
        service_ptr_ = fn();
    } catch (const std::exception& e) {
        THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: {}", e.what());
        throw std::runtime_error("ThemisCoreServiceImpl service callback threw an exception");
    } catch (...) {
        THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: unknown error");
        throw std::runtime_error("ThemisCoreServiceImpl service callback threw an unknown exception");
    }
    if (!service_ptr_) {
        const std::string error =
            "ThemisCoreServiceImpl ServiceInstanceFn returned nullptr in non-proto build";
        THEMIS_CRITICAL("{}", error);
        throw std::runtime_error(error);
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
