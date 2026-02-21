/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_engineering_grpc_service.h                  ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:37:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   86.0/100                                       ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 831094d0a  2026-02-11  Add ThemisDB Wiki Integration plugin and documentation im... ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
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
