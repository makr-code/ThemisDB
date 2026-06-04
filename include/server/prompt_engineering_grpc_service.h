/**
 * @file prompt_engineering_grpc_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

// NOTE: Proto file "prompt_engineering_service.grpc.pb.h" not generated yet
// This service is a stub until the proto definition is generated
// #include "proto/prompt_engineering_service.grpc.pb.h"

#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/self_improvement_orchestrator.h"
#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_version_control.h"
#include "prompt_engineering/prompt_engineering_integration.h"
#include <grpcpp/grpcpp.h>
#include <functional>
#include <memory>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace server {

/**
 * @brief gRPC service for prompt engineering operations (STUB - Proto not generated)
 * 
 * Placeholder implementation. Full gRPC service available once proto is generated.
 */
class PromptEngineeringGrpcService final {
public:
    using ServiceAccessorFn = std::function<void*()>;

    /**
     * @brief Construct gRPC service with all prompt engineering components
     */
    PromptEngineeringGrpcService(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<::themis::prompt_engineering::PromptManager> manager,
        std::shared_ptr<::themis::prompt_engineering::PromptOptimizer> optimizer,
        std::shared_ptr<::themis::prompt_engineering::PromptPerformanceTracker> tracker,
        std::shared_ptr<::themis::prompt_engineering::SelfImprovementOrchestrator> orchestrator,
        std::shared_ptr<::themis::prompt_engineering::FeedbackCollector> feedback_collector,
        std::shared_ptr<::themis::prompt_engineering::PromptVersionControl> version_control,
        std::shared_ptr<::themis::prompt_engineering::PromptEngineeringIntegration> integration
    );

    ~PromptEngineeringGrpcService() = default;

    /**
     * @brief Return opaque pointer to an externally provided grpc::Service instance.
     *
     * Returns nullptr when no service accessor callback is configured.
     */
    void* service() const;

    /**
     * @brief Configure process-wide callback that provides grpc::Service pointer.
     *
     * Used by non-proto builds to wire a generated service from another module.
     */
    static void setServiceAccessorFn(ServiceAccessorFn fn);

    // NOTE: All gRPC methods removed - proto file not generated yet
    // Stub service - full implementation available once proto is generated

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<::themis::prompt_engineering::PromptManager> manager_;
    std::shared_ptr<::themis::prompt_engineering::PromptOptimizer> optimizer_;
    std::shared_ptr<::themis::prompt_engineering::PromptPerformanceTracker> tracker_;
    std::shared_ptr<::themis::prompt_engineering::SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<::themis::prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<::themis::prompt_engineering::PromptVersionControl> version_control_;
    std::shared_ptr<::themis::prompt_engineering::PromptEngineeringIntegration> integration_;
    void* service_ptr_ = nullptr;
};

} // namespace server
} // namespace themis
