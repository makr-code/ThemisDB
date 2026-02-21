/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pitr_grpc_service.h                                ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
