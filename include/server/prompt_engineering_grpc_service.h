/**
 * @file prompt_engineering_grpc_service.h
 * @brief gRPC service implementation for prompt engineering operations
 * 
 * Provides high-performance binary protocol access to all prompt engineering
 * capabilities including optimization, A/B testing, feedback, and version control.
 */

#pragma once

#include "proto/prompt_engineering_service.grpc.pb.h"
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
 * @brief gRPC service for prompt engineering operations
 * 
 * Implements the PromptEngineeringService proto definition, providing
 * gRPC access to all prompt engineering features in parallel with HTTP API.
 */
class PromptEngineeringGrpcService final 
    : public prompt_engineering::PromptEngineeringService::Service {
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

    ~PromptEngineeringGrpcService() override = default;

    // Optimization operations
    grpc::Status Optimize(
        grpc::ServerContext* context,
        const prompt_engineering::OptimizeRequest* request,
        prompt_engineering::OptimizeResponse* response) override;

    grpc::Status GetOptimizationHistory(
        grpc::ServerContext* context,
        const prompt_engineering::HistoryRequest* request,
        prompt_engineering::HistoryResponse* response) override;

    // A/B Testing operations
    grpc::Status ListABTests(
        grpc::ServerContext* context,
        const prompt_engineering::ABTestListRequest* request,
        prompt_engineering::ABTestListResponse* response) override;

    grpc::Status GetABTest(
        grpc::ServerContext* context,
        const prompt_engineering::ABTestDetailRequest* request,
        prompt_engineering::ABTestDetailResponse* response) override;

    // Feedback operations
    grpc::Status SubmitFeedback(
        grpc::ServerContext* context,
        const prompt_engineering::FeedbackRequest* request,
        prompt_engineering::FeedbackResponse* response) override;

    // Statistics and monitoring
    grpc::Status GetStats(
        grpc::ServerContext* context,
        const prompt_engineering::StatsRequest* request,
        prompt_engineering::StatsResponse* response) override;

    // Version control operations
    grpc::Status GetVersions(
        grpc::ServerContext* context,
        const prompt_engineering::VersionsRequest* request,
        prompt_engineering::VersionsResponse* response) override;

    grpc::Status Rollback(
        grpc::ServerContext* context,
        const prompt_engineering::RollbackRequest* request,
        prompt_engineering::RollbackResponse* response) override;

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<::themis::prompt_engineering::PromptManager> manager_;
    std::shared_ptr<::themis::prompt_engineering::PromptOptimizer> optimizer_;
    std::shared_ptr<::themis::prompt_engineering::PromptPerformanceTracker> tracker_;
    std::shared_ptr<::themis::prompt_engineering::SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<::themis::prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<::themis::prompt_engineering::PromptVersionControl> version_control_;
    std::shared_ptr<::themis::prompt_engineering::PromptEngineeringIntegration> integration_;

    // Helper methods for authentication
    bool validateBearerToken(grpc::ServerContext* context);
    std::string extractBearerToken(grpc::ServerContext* context);
};

} // namespace server
} // namespace themis
