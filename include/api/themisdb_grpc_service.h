/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_grpc_service.h                            ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:05:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9bb592d7  2026-02-24  Implement ThemisDBGrpcService and fix ThemisCoreServiceIm... ║
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
 *   ThemisDBGrpcService svc(db, txn_mgr);
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
 */
class ThemisDBGrpcService {
public:
    /**
     * @param db       Storage backend (must outlive this object).
     * @param txn_mgr  Transaction manager (must outlive this object).
     */
    ThemisDBGrpcService(
        std::shared_ptr<RocksDBWrapper>     db,
        std::shared_ptr<TransactionManager> txn_mgr
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
    std::shared_ptr<RocksDBWrapper>     db_;
    std::shared_ptr<TransactionManager> txn_mgr_;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace api
} // namespace themis
