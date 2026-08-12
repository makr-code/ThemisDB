/**
 * @file pitr_grpc_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

// Forward declarations
namespace themis {
    class PITRManager;
    namespace transaction {
        class SnapshotManager;
    }
}

namespace themis {
namespace pitr {

/**
 * @brief ThemisDB PITR gRPC Service Implementation
 * 
 * Provides gRPC interface for Point-in-Time Recovery operations:
 * - Snapshot management (create, list, get, delete)
 * - Restore operations (preview, execute)
 * - Progress monitoring
 * 
 * Part of ThemisDB v1.5.0 - Feature: PITR for MVCC
 * 
 * Note: This service provides the same functionality as PITRApiHandler
 * but through gRPC protocol instead of REST API.
 */
class PITRServiceImpl {
public:
    /**
     * @brief Construct service with PITR components
     * @param pitr_manager PITR manager for restore operations
     * @param snapshot_manager Snapshot manager for tag operations
     */
    PITRServiceImpl(
        std::shared_ptr<PITRManager> pitr_manager,
        std::shared_ptr<transaction::SnapshotManager> snapshot_manager
    );

    ~PITRServiceImpl() = default;

    /**
     * @brief Get service instance for gRPC server registration
     * This will return the actual grpc::Service* after proto generation
     */
    void* getServiceInstance();

private:
    std::shared_ptr<PITRManager> pitr_manager_;
    std::shared_ptr<transaction::SnapshotManager> snapshot_manager_;
    
    // Helper methods
    bool validateTagName(const std::string& tag_name);
    bool validateDescription(const std::string& description);
};

} // namespace pitr
} // namespace themis
