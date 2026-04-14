/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_engineering_grpc_service.h                  ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:43:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_engineering_grpc_service.h
 * @brief gRPC service implementation for prompt engineering operations
 * 
 * Provides high-performance binary protocol access to all prompt engineering
 * capabilities including optimization, A/B testing, feedback, and version control.
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
};

} // namespace server
} // namespace themis
