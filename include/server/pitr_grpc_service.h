/**
 * @file pitr_grpc_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PITRServiceImpl {
public:
    PITRServiceImpl(
        std::shared_ptr<PITRManager> pitr_manager,
        std::shared_ptr<transaction::SnapshotManager> snapshot_manager
    );

    ~PITRServiceImpl() = default;

    /**
     * @brief Get Service Instance.
     * @return Pointer to the result.
     */
    void* getServiceInstance();

private:
    std::shared_ptr<PITRManager> pitr_manager_;
    std::shared_ptr<transaction::SnapshotManager> snapshot_manager_;
    
    // Helper methods
    /**
     * @brief Validate Tag Name.
     * @param[in] tag_name Name of the tag.
     * @return True when the operation succeeds.
     */
    bool validateTagName(const std::string& tag_name);
    /**
     * @brief Validate Description.
     * @param[in] description Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDescription(const std::string& description);
};

} // namespace pitr
} // namespace themis
