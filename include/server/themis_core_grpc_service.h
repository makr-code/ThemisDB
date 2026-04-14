/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_core_grpc_service.h                         ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:43:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     85                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9bb592d7b  2026-02-24  Implement ThemisDBGrpcService and fix ThemisCoreServiceIm... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>

// Forward declarations of the internal components used by the service.
namespace themis {
    class RocksDBWrapper;
    class TransactionManager;
    class AQLEngine;
}

namespace themis {
namespace core {

/**
 * @brief ThemisDB Core gRPC Service wrapper (ThemisCoreService, themis_core.proto)
 *
 * Wraps the ThemisCoreService implementation defined in proto/themis_core.proto.
 * Follows the same conditional-compilation pattern as WalGrpcService: the actual
 * grpc::Service-derived implementation is compiled in only when the generated
 * `themis_core.grpc.pb.h` header is on the include path; otherwise the wrapper
 * becomes a safe no-op that returns nullptr from getServiceInstance().
 *
 * Part of ThemisDB v1.3.0 – Feature #8: gRPC Protocol
 */
class ThemisCoreServiceImpl {
public:
    /**
     * @brief Construct service with database components.
     * @param db         RocksDB wrapper for storage operations.
     * @param txn_mgr    Transaction manager for ACID operations.
     * @param aql_engine AQL query engine.
     */
    ThemisCoreServiceImpl(
        std::shared_ptr<RocksDBWrapper>    db,
        std::shared_ptr<TransactionManager> txn_mgr,
        std::shared_ptr<AQLEngine>          aql_engine
    );

    ~ThemisCoreServiceImpl();

    /**
     * @brief Return the underlying grpc::Service pointer for registration.
     *
     * Returns the concrete service implementation when `themis_core.grpc.pb.h`
     * is available (i.e. protoc has been run).  Returns nullptr otherwise so
     * that callers can safely skip registration without crashing.
     */
    void* getServiceInstance();

private:
    std::shared_ptr<RocksDBWrapper>     db_;
    std::shared_ptr<TransactionManager> txn_mgr_;
    std::shared_ptr<AQLEngine>          aql_engine_;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace core
} // namespace themis
