/**
 * @file prompt_engineering_grpc_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "server/prompt_engineering_grpc_service.h"
#include <stdexcept>
#include "utils/logger.h"
#include <exception>
#include <mutex>
#include <utility>

namespace themis {
namespace server {

namespace {
std::mutex g_prompt_grpc_service_accessor_mutex;
PromptEngineeringGrpcService::ServiceAccessorFn g_prompt_grpc_service_accessor_fn;
} // namespace

PromptEngineeringGrpcService::PromptEngineeringGrpcService(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<::themis::prompt_engineering::PromptManager> manager,
    std::shared_ptr<::themis::prompt_engineering::PromptOptimizer> optimizer,
    std::shared_ptr<::themis::prompt_engineering::PromptPerformanceTracker> tracker,
    std::shared_ptr<::themis::prompt_engineering::SelfImprovementOrchestrator> orchestrator,
    std::shared_ptr<::themis::prompt_engineering::FeedbackCollector> feedback_collector,
    std::shared_ptr<::themis::prompt_engineering::PromptVersionControl> version_control,
    std::shared_ptr<::themis::prompt_engineering::PromptEngineeringIntegration> integration)
    : storage_(std::move(storage))
    , manager_(std::move(manager))
    , optimizer_(std::move(optimizer))
    , tracker_(std::move(tracker))
    , orchestrator_(std::move(orchestrator))
    , feedback_collector_(std::move(feedback_collector))
    , version_control_(std::move(version_control))
    , integration_(std::move(integration)) {
    ServiceAccessorFn fn;
    {
        /**
         * @brief Lock.
         * @param[in] g_prompt_grpc_service_accessor_mutex Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(g_prompt_grpc_service_accessor_mutex);
        fn = g_prompt_grpc_service_accessor_fn;
    }
    if (fn) {
        try {
            service_ptr_ = fn();
        } catch (const std::exception& e) {
            THEMIS_ERROR("Prompt gRPC service accessor callback failed: {}", e.what());
            service_ptr_ = nullptr;
        } catch (...) {
            THEMIS_ERROR("Prompt gRPC service accessor callback failed: unknown error");
            service_ptr_ = nullptr;
        }
    }
}

void* PromptEngineeringGrpcService::service() const {
    return service_ptr_;
}

/**
 * @brief Set Service Accessor Fn.
 * @param[in] fn Input parameter.
 * @details Calls: lock(), std::move().
 */
void PromptEngineeringGrpcService::setServiceAccessorFn(ServiceAccessorFn fn) {
    std::lock_guard<std::mutex> lock(g_prompt_grpc_service_accessor_mutex);
    g_prompt_grpc_service_accessor_fn = std::move(fn);
}

} // namespace server
} // namespace themis

