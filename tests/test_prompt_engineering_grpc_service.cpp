/**
 * @file test_prompt_engineering_grpc_service.cpp
 * @brief Unit tests for PromptEngineeringGrpcService
 */

#include <gtest/gtest.h>
#include "server/prompt_engineering_grpc_service.h"
// #include "proto/prompt_engineering_service.grpc.pb.h"  // Proto file not generated yet
#include <grpcpp/grpcpp.h>
#include <cstdint>
#include <memory>
#include <stdexcept>

// NOTE: All tests disabled - proto files not generated, all request/response types missing

#if 0
using namespace themis;
using namespace themis::server;

class PromptEngineeringGrpcServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: These are null pointers for basic testing
        // In a real scenario, we'd use mocks or test doubles
        storage_ = nullptr;
        manager_ = nullptr;
        optimizer_ = nullptr;
        tracker_ = nullptr;
        orchestrator_ = nullptr;
        feedback_collector_ = nullptr;
        version_control_ = nullptr;
        integration_ = nullptr;

        service_ = std::make_unique<PromptEngineeringGrpcService>(
            storage_,
            manager_,
            optimizer_,
            tracker_,
            orchestrator_,
            feedback_collector_,
            version_control_,
            integration_
        );
    }

    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<prompt_engineering::PromptManager> manager_;
    std::shared_ptr<prompt_engineering::PromptOptimizer> optimizer_;
    std::shared_ptr<prompt_engineering::PromptPerformanceTracker> tracker_;
    std::shared_ptr<prompt_engineering::SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<prompt_engineering::PromptVersionControl> version_control_;
    std::shared_ptr<prompt_engineering::PromptEngineeringIntegration> integration_;
    
    std::unique_ptr<PromptEngineeringGrpcService> service_;
};

TEST_F(PromptEngineeringGrpcServiceTest, Construction) {
    // Simply test that the service can be constructed
    EXPECT_NE(service_, nullptr);
}

TEST_F(PromptEngineeringGrpcServiceTest, OptimizeWithoutOrchestrator) {
    grpc::ServerContext context;
    prompt_engineering::OptimizeRequest request;
    request.set_prompt_id("test_prompt");
    prompt_engineering::OptimizeResponse response;

    auto status = service_->Optimize(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
    EXPECT_TRUE(status.error_message().find("SelfImprovementOrchestrator") != std::string::npos);
}

TEST_F(PromptEngineeringGrpcServiceTest, OptimizeWithMissingPromptId) {
    grpc::ServerContext context;
    prompt_engineering::OptimizeRequest request;
    // No prompt_id set
    prompt_engineering::OptimizeResponse response;

    auto status = service_->Optimize(&context, &request, &response);
    
    // Will fail on unavailable first since orchestrator is null
    EXPECT_NE(status.error_code(), grpc::StatusCode::OK);
}

TEST_F(PromptEngineeringGrpcServiceTest, GetHistoryWithoutOrchestrator) {
    grpc::ServerContext context;
    prompt_engineering::HistoryRequest request;
    request.set_prompt_id("test_prompt");
    prompt_engineering::HistoryResponse response;

    auto status = service_->GetOptimizationHistory(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST_F(PromptEngineeringGrpcServiceTest, ListABTestsWithoutOrchestrator) {
    grpc::ServerContext context;
    prompt_engineering::ABTestListRequest request;
    prompt_engineering::ABTestListResponse response;

    auto status = service_->ListABTests(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST_F(PromptEngineeringGrpcServiceTest, GetABTestWithoutOrchestrator) {
    grpc::ServerContext context;
    prompt_engineering::ABTestDetailRequest request;
    request.set_test_id("test_123");
    prompt_engineering::ABTestDetailResponse response;

    auto status = service_->GetABTest(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST_F(PromptEngineeringGrpcServiceTest, SubmitFeedbackWithoutCollector) {
    grpc::ServerContext context;
    prompt_engineering::FeedbackRequest request;
    request.set_prompt_id("test_prompt");
    request.set_query("test query");
    request.set_response("test response");
    request.set_type(prompt_engineering::USER_POSITIVE);
    prompt_engineering::FeedbackResponse response;

    auto status = service_->SubmitFeedback(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
    EXPECT_TRUE(status.error_message().find("FeedbackCollector") != std::string::npos);
}

TEST_F(PromptEngineeringGrpcServiceTest, GetStatsWithNullComponents) {
    grpc::ServerContext context;
    prompt_engineering::StatsRequest request;
    prompt_engineering::StatsResponse response;

    auto status = service_->GetStats(&context, &request, &response);
    
    // Should return OK even with null components (partial stats)
    EXPECT_EQ(status.error_code(), grpc::StatusCode::OK);
}

TEST_F(PromptEngineeringGrpcServiceTest, GetVersionsWithoutVersionControl) {
    grpc::ServerContext context;
    prompt_engineering::VersionsRequest request;
    request.set_prompt_id("test_prompt");
    prompt_engineering::VersionsResponse response;

    auto status = service_->GetVersions(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
    EXPECT_TRUE(status.error_message().find("PromptVersionControl") != std::string::npos);
}

TEST_F(PromptEngineeringGrpcServiceTest, RollbackWithoutOrchestrator) {
    grpc::ServerContext context;
    prompt_engineering::RollbackRequest request;
    request.set_prompt_id("test_prompt");
    prompt_engineering::RollbackResponse response;

    auto status = service_->Rollback(&context, &request, &response);
    
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST_F(PromptEngineeringGrpcServiceTest, FeedbackTypeConversion) {
    grpc::ServerContext context;
    prompt_engineering::FeedbackRequest request;
    request.set_prompt_id("test_prompt");
    request.set_type(prompt_engineering::HALLUCINATION_DETECTED);
    prompt_engineering::FeedbackResponse response;

    // Even though collector is null, the type conversion code path is executed
    auto status = service_->SubmitFeedback(&context, &request, &response);
    
    // Will fail on unavailable collector, but type was converted successfully
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST_F(PromptEngineeringGrpcServiceTest, MissingRequiredFields) {
    grpc::ServerContext context;
    
    // Test optimize without prompt_id
    {
        prompt_engineering::OptimizeRequest request;
        prompt_engineering::OptimizeResponse response;
        auto status = service_->Optimize(&context, &request, &response);
        EXPECT_NE(status.error_code(), grpc::StatusCode::OK);
    }
    
    // Test feedback without prompt_id
    {
        prompt_engineering::FeedbackRequest request;
        prompt_engineering::FeedbackResponse response;
        auto status = service_->SubmitFeedback(&context, &request, &response);
        EXPECT_NE(status.error_code(), grpc::StatusCode::OK);
    }
    
    // Test versions without prompt_id
    {
        prompt_engineering::VersionsRequest request;
        prompt_engineering::VersionsResponse response;
        auto status = service_->GetVersions(&context, &request, &response);
        EXPECT_NE(status.error_code(), grpc::StatusCode::OK);
    }
}

#endif

// Note: Comprehensive integration tests with actual components
// would require a full test harness with mocked dependencies and
// running gRPC server/client infrastructure

// Placeholder test to satisfy build system
TEST(PromptEngineeringGrpcServiceDisabled, ProtoFilesNotGenerated) {
    GTEST_SKIP() << "Proto files not generated - all gRPC service tests disabled";
}

using themis::server::PromptEngineeringGrpcService;

TEST(PromptEngineeringGrpcServiceBridgeTest, ServiceIsNullWithoutAccessor) {
    PromptEngineeringGrpcService::setServiceAccessorFn({});
    PromptEngineeringGrpcService svc(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(svc.service(), nullptr);
}

TEST(PromptEngineeringGrpcServiceBridgeTest, ServiceUsesAccessorPointer) {
    void* expected = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x1234));
    PromptEngineeringGrpcService::setServiceAccessorFn([expected]() { return expected; });

    PromptEngineeringGrpcService svc(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(svc.service(), expected);

    PromptEngineeringGrpcService::setServiceAccessorFn({});
}

TEST(PromptEngineeringGrpcServiceBridgeTest, AccessorExceptionFailsClosed) {
    PromptEngineeringGrpcService::setServiceAccessorFn([]() -> void* {
        throw std::runtime_error("boom");
    });

    PromptEngineeringGrpcService svc(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(svc.service(), nullptr);

    PromptEngineeringGrpcService::setServiceAccessorFn({});
}
