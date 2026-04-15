/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_grpc_service.h                            ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:44:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     134                                            ║
    • Open Issues:     TODOs: 0, Stubs: 5                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>

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
     */
    void* service();

private:
    std::shared_ptr<RocksDBWrapper>             db_;
    std::shared_ptr<TransactionManager>         txn_mgr_;
    std::shared_ptr<themis::IQueryEngine>       aql_engine_;
    std::shared_ptr<themis::IVectorIndex>       vector_index_;

    class Impl;
    std::unique_ptr<Impl> impl_;

    /// Internal helper used by both constructors.
    void buildImpl();
};

} // namespace api
} // namespace themis
