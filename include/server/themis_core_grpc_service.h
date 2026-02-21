/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_core_grpc_service.h                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

// Forward declarations
namespace themis {
    class RocksDBWrapper;
    class TransactionManager;
    class AQLEngine;
}

namespace themis {
namespace core {

/**
 * @brief ThemisDB Core gRPC Service Implementation
 * 
 * Provides gRPC interface for:
 * - CRUD operations (Create, Read, Update, Delete)
 * - Batch operations
 * - Transaction management (Begin, Commit, Rollback)
 * - AQL query execution
 * - Collection scanning with streaming
 * - Health and status monitoring
 * 
 * Part of ThemisDB v1.3.0 - Feature #8: gRPC Protocol
 */
class ThemisCoreServiceImpl {
public:
    /**
     * @brief Construct service with database components
     * @param db RocksDB wrapper for storage operations
     * @param txn_mgr Transaction manager for ACID operations
     * @param aql_engine AQL query engine
     */
    ThemisCoreServiceImpl(
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<TransactionManager> txn_mgr,
        std::shared_ptr<AQLEngine> aql_engine
    );

    ~ThemisCoreServiceImpl() = default;

    // CRUD Operations (to be implemented in cpp file with proto includes)
    // Note: Actual grpc::Service inheritance and method implementations
    // will be in the .cpp file after proto generation

    /**
     * @brief Get service instance for gRPC server registration
     * This will return the actual grpc::Service* after proto generation
     */
    void* getServiceInstance();

private:
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<TransactionManager> txn_mgr_;
    std::shared_ptr<AQLEngine> aql_engine_;
    
    // Helper methods
    bool validateCollection(const std::string& collection);
    bool validateKey(const std::string& key);
    std::string generateTransactionId();
};

} // namespace core
} // namespace themis
