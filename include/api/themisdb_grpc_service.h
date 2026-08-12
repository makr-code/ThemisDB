/**
 * @file themisdb_grpc_service.h
 * @brief gRPC service implementation for ThemisDB database operations.
 *
 * @details Implements the generated gRPC service stub, translating protobuf
 * messages to/from internal data structures and delegating to core service logic.
 *
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include <functional>
#include <memory>

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/registry.h>
#endif

// Forward declarations of the internal components required by the service.
namespace themis {
class RocksDBWrapper;
class TransactionManager;
} // namespace themis

// Interface forward declarations (always available without proto stubs)
namespace themis {
class IQueryEngine;
class IVectorIndex;
using IQueryEnginePtr = std::shared_ptr<IQueryEngine>;
} // namespace themis

namespace themis {
namespace api {

/**
 * @brief Wrapper for the ThemisDBService gRPC service implementation.
 *
 * Provides a concrete service that implements the ThemisDBService RPC surface
 * defined in proto/themisdb.proto.  The wrapper follows the same conditional-
 * compilation pattern as WalGrpcService: the actual grpc::Service-derived Impl
 * is compiled in only when the generated `themisdb.grpc.pb.h` header is on the
 * include path; otherwise the wrapper becomes a safe no-op.
 *
 * Lifecycle – register with GrpcApiServer before calling start():
 * @code
 *   ThemisDBGrpcService svc(db, txn_mgr, aql_engine, vector_index);
 *   grpc_api_server.registerService(svc.service());  // nullptr when stubs absent
 *   grpc_api_server.start();
 * @endcode
 *
 * Covered RPCs (mirrors REST surface, see proto/themisdb.proto):
 *   - Document CRUD : CreateDocument, GetDocument, UpdateDocument, DeleteDocument
 *   - Batch ops     : BatchWrite, BatchRead
 *   - AQL           : ExecuteAQL, StreamAQL (server-side streaming)
 *   - Vector search : VectorSearch, FilteredVectorSearch, HybridSearch, FullTextSearch
 *   - Health        : HealthCheck
 *
 * When an AQL engine and/or vector index are provided via the extended
 * constructor the corresponding RPC stubs delegate to them rather than
 * returning UNIMPLEMENTED.  See ThemisDBGrpcServiceFactory for a fluent
 * builder that wires all components together.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Keep gRPC wrapper type available even when generated protobuf stubs are absent.
 * Activation: Active when generated themisdb gRPC headers are not available on include path.
 * Production Delta: service() returns nullptr and RPC registration is skipped instead of serving requests.
 * Removal Plan: Remove fallback behavior once protobuf code generation is mandatory in all builds.
 */
class ThemisDBGrpcService {
public:
    /// Callback type that provides an opaque grpc::Service* to the wrapper
    /// when the generated proto stubs are absent from the build.
    using ServiceFn = std::function<void*()>;

    /**
     * @brief Construct with storage only (AQL and vector search return UNIMPLEMENTED).
     * @param db       Storage backend (must outlive this object).
     * @param txn_mgr  Transaction manager (must outlive this object).
     */
    ThemisDBGrpcService(
        std::shared_ptr<RocksDBWrapper>     db,
        std::shared_ptr<TransactionManager> txn_mgr
    );

    /**
     * @brief Construct with all components wired in.
     *
     * AQL engine enables ExecuteAQL, StreamAQL, HybridSearch, and FullTextSearch.
     * Vector index enables VectorSearch and FilteredVectorSearch.
     *
     * @param db           Storage backend (must outlive this object).
     * @param txn_mgr      Transaction manager (must outlive this object).
     * @param aql_engine   Query engine for AQL RPCs (may be nullptr).
     * @param vector_index Vector similarity index (may be nullptr).
     */
    ThemisDBGrpcService(
        std::shared_ptr<RocksDBWrapper>          db,
        std::shared_ptr<TransactionManager>      txn_mgr,
        std::shared_ptr<themis::IQueryEngine>    aql_engine,
        std::shared_ptr<themis::IVectorIndex>    vector_index
    );

    ~ThemisDBGrpcService();

    /**
     * @brief Return the underlying grpc::Service pointer for registration.
     *
     * Returns the concrete service implementation when `themisdb.grpc.pb.h`
     * is available (i.e. protoc has been run).  Returns nullptr otherwise so
     * that callers can safely skip registration without crashing.
     *
     * If a service callback was registered via setServiceFn(), its return
     * value is used for non-proto builds.
     */
    void* service();

    /**
     * @brief Configure a process-wide callback that provides a grpc::Service*.
     *
     * Used in non-proto builds to wire a service instance obtained from another
     * module (e.g. a dynamically loaded plugin or a test double).  The callback
     * is invoked once during construction in an exception-safe manner; exceptions
     * cause the service pointer to remain null (fail-closed).
     *
     * Pass an empty function to remove a previously registered callback.
     */
    static void setServiceFn(ServiceFn fn);

#ifdef THEMIS_HAS_PROMETHEUS
    /**
     * @brief Register gRPC request counters in a Prometheus registry.
     *
     * Registers `grpc_requests_total{method,status}` for transport-level gRPC
     * status codes emitted by this service instance.
     *
     * @param registry Shared Prometheus registry used by the server process.
     */
    void setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry);
#endif

private:
    std::shared_ptr<RocksDBWrapper>             db_;
    std::shared_ptr<TransactionManager>         txn_mgr_;
    std::shared_ptr<themis::IQueryEngine>       aql_engine_;
    std::shared_ptr<themis::IVectorIndex>       vector_index_;

    void* service_ptr_ = nullptr;

    class Impl;
    std::unique_ptr<Impl> impl_;

    /// Internal helper used by both constructors.
    void buildImpl();
};

} // namespace api
} // namespace themis
