/*
 * ThemisDB | File: prompt_engineering_grpc_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 90
 * Open Issues: TODOs=1, Stubs=3, Gaps=7, Unimpl=1, Mock=1, Sim=1, Debt=0
 * Gap Correlation: internal=7 | external_v3=19 | delta=12 | status=divergent
 * External Severity (v3): C=2, H=12, M=5
 * PR: #3632 fix(build): register 40+ missing sources across 7 modules in cmake ... (2026-03-12T07:39:41Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * @file prompt_engineering_grpc_service.cpp
 * @brief Stub gRPC service implementation for prompt engineering.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Provide a link-compatible constructor body for
 *   PromptEngineeringGrpcService so that the class can be instantiated and
 *   stored by the server without requiring the protoc-generated
 *   prompt_engineering.grpc.pb.h stubs to be compiled in.  The gRPC
 *   service() method (defined in the header) returns nullptr until the proto
 *   stubs are generated; the HttpServer/gRPC layer skips nullptr services.
 * Activation: Always active — the full gRPC handler implementation lives in
 *   a separate file that is compiled only when THEMIS_HAS_PROMPT_GRPC is set.
 * Production Delta: All gRPC calls to the prompt-engineering endpoint receive
 *   UNIMPLEMENTED status.  REST endpoints backed by PromptManager and friends
 *   are unaffected (they bypass this service class).
 * Removal Plan: Run protoc on proto/prompt_engineering.proto; compile the
 *   generated stubs and the full handler; set THEMIS_HAS_PROMPT_GRPC=1.
 *   Tracking: src/prompt_engineering/FUTURE_ENHANCEMENTS.md §"gRPC Service"
 * Roadmap ref: src/server/ROADMAP.md §gRPC service lifecycle
 */

#include "server/prompt_engineering_grpc_service.h"
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
        std::lock_guard<std::mutex> lock(g_prompt_grpc_service_accessor_mutex);
        fn = g_prompt_grpc_service_accessor_fn;
    }
    if (fn) {
        try {
            service_ptr_ = fn();
        } catch (const std::exception& e) {
            THEMIS_ERROR("Prompt gRPC service accessor callback failed: {}", e.what());
            service_ptr_ = nullptr;
        } catch (const std::exception&) {
            THEMIS_ERROR("Prompt gRPC service accessor callback failed: unknown error");
            service_ptr_ = nullptr;
        }
    }
}

void* PromptEngineeringGrpcService::service() const {
    return service_ptr_;
}

void PromptEngineeringGrpcService::setServiceAccessorFn(ServiceAccessorFn fn) {
    std::lock_guard<std::mutex> lock(g_prompt_grpc_service_accessor_mutex);
    g_prompt_grpc_service_accessor_fn = std::move(fn);
}

} // namespace server
} // namespace themis
