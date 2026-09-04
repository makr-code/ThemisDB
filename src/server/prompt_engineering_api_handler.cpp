/**
 * @file prompt_engineering_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "server/prompt_engineering_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/self_improvement_orchestrator.h"
#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_version_control.h"
#include "prompt_engineering/prompt_engineering_integration.h"
#include "server/auth_middleware.h"
#include "utils/input_validator.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxPromptEngineeringIdentifierLength = 256;

bool isValidPromptEngineeringIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    themis::utils::InputValidator validator;
    return validator.validateStringLength(value, kMaxPromptEngineeringIdentifierLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

} // namespace

PromptEngineeringApiHandler::PromptEngineeringApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<prompt_engineering::PromptManager> manager,
    std::shared_ptr<prompt_engineering::PromptOptimizer> optimizer,
    std::shared_ptr<prompt_engineering::PromptPerformanceTracker> tracker,
    std::shared_ptr<prompt_engineering::SelfImprovementOrchestrator> orchestrator,
    std::shared_ptr<prompt_engineering::FeedbackCollector> feedback_collector,
    std::shared_ptr<prompt_engineering::PromptVersionControl> version_control,
    std::shared_ptr<prompt_engineering::PromptEngineeringIntegration> integration,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , manager_(std::move(manager))
    , optimizer_(std::move(optimizer))
    , tracker_(std::move(tracker))
    , orchestrator_(std::move(orchestrator))
    , feedback_collector_(std::move(feedback_collector))
    , version_control_(std::move(version_control))
    , integration_(std::move(integration))
    , auth_(std::move(auth))
{
}

// POST /api/v1/prompt_engineering/optimize
http::response<http::string_body> PromptEngineeringApiHandler::handleOptimize(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleOptimize");
    try {
        if (!orchestrator_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "SelfImprovementOrchestrator not available",
                req);
        }
        auto& orchestrator = *orchestrator_;

        auto body = nlohmann::json::parse(req.body());
        
        std::string prompt_id = body.value("prompt_id", "");
        std::string strategy = body.value("strategy", "auto");
        
        if (prompt_id.empty()) {
            return makeErrorResponse(
                http::status::bad_request,
                "Missing prompt_id",
                req);
        }

        // Check if optimization should be triggered
        if (!orchestrator.shouldOptimize(prompt_id)) {
            nlohmann::json response;
            response["status"] = "skipped";
            response["message"] = "Prompt does not meet optimization criteria";
            response["prompt_id"] = prompt_id;
            return makeResponse(http::status::ok, response.dump(), req);
        }

        // Trigger optimization
        std::vector<prompt_engineering::TestCase> test_cases = {};

        if (body.contains("test_cases")) {
            for (const auto& tc : body["test_cases"]) {
                prompt_engineering::TestCase test;
                test.input = tc.value("input", "");
                test.expected_output = tc.value("expected_output", "");
                if (tc.contains("context")) {
                    test.context = tc["context"];
                }
                test_cases.push_back(test);
            }
        }

        auto result = orchestrator.optimizePrompt(prompt_id, test_cases);

        nlohmann::json response;
        response["status"] = "success";
        response["prompt_id"] = prompt_id;
        response["improvement"] = result.improvement;
        response["old_score"] = result.baseline_score;
        response["new_score"] = result.optimized_score;
        response["iterations"] = result.iterations;
        
        if (result.status == prompt_engineering::OptimizationStatus::AB_TESTING) {
            response["ab_testing"] = true;
            response["ab_test_id"] = result.metadata.value("ab_test_id", "");
        }

        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(
            http::status::bad_request,
            std::string("Invalid JSON: ") + e.what(),
            req);
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// GET /api/v1/prompt_engineering/ab_tests
http::response<http::string_body> PromptEngineeringApiHandler::handleListABTests(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleListABTests");
    try {
        if (!orchestrator_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "SelfImprovementOrchestrator not available",
                req);
        }
        auto& orchestrator = *orchestrator_;

        auto tests = orchestrator.getActiveABTests();
        nlohmann::json response = nlohmann::json::array();
        
        for (const auto& test : tests) {
            response.push_back(test.toJson());
        }

        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// GET /api/v1/prompt_engineering/ab_tests/:id
http::response<http::string_body> PromptEngineeringApiHandler::handleGetABTest(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetABTest");
    try {
        std::string path = std::string(req.target());
        auto test_id = extractPathParam(path, "/api/v1/prompt_engineering/ab_tests/");

        if (test_id.empty()) {
            return makeErrorResponse(
                http::status::bad_request,
                "Missing test_id",
                req);
        }

        if (!isValidPromptEngineeringIdentifier(test_id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid test_id",
                req);
        }

        if (!orchestrator_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "SelfImprovementOrchestrator not available",
                req);
        }
        auto& orchestrator = *orchestrator_;

        auto test = orchestrator.getABTestResults(test_id);
        if (!test) {
            return makeErrorResponse(
                http::status::not_found,
                "A/B test not found",
                req);
        }

        return makeResponse(http::status::ok, test->toJson().dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// POST /api/v1/prompt_engineering/feedback
http::response<http::string_body> PromptEngineeringApiHandler::handleSubmitFeedback(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleSubmitFeedback");
    try {
        if (!feedback_collector_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "FeedbackCollector not available",
                req);
        }
        auto& feedback_collector = *feedback_collector_;

        auto body = nlohmann::json::parse(req.body());
        
        std::string prompt_id = body.value("prompt_id", "");
        std::string query = body.value("query", "");
        std::string response_text = body.value("response", "");
        std::string feedback_text = body.value("feedback_text", "");
        std::string type_str = body.value("type", "USER_POSITIVE");
        double severity = body.value("severity", 0.5);

        if (prompt_id.empty()) {
            return makeErrorResponse(
                http::status::bad_request,
                "Missing prompt_id",
                req);
        }

        // Convert string to FeedbackType
        auto type_opt = prompt_engineering::stringToFeedbackType(type_str);
        if (!type_opt) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid feedback type",
                req);
        }

        auto feedback_id = feedback_collector.recordFeedback(
            prompt_id,
            query,
            response_text,
            *type_opt,
            feedback_text,
            severity
        );

        nlohmann::json response;
        response["status"] = "success";
        response["feedback_id"] = feedback_id;
        response["prompt_id"] = prompt_id;

        return makeResponse(http::status::created, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(
            http::status::bad_request,
            std::string("Invalid JSON: ") + e.what(),
            req);
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// GET /api/v1/prompt_engineering/stats
http::response<http::string_body> PromptEngineeringApiHandler::handleGetStats(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetStats");
    try {
        nlohmann::json stats = nlohmann::json::object();

        // Integration stats
        if (integration_) {
            stats["integration"] = integration_->getStats();
        }

        // Performance tracker stats
        if (tracker_) {
            stats["performance"] = tracker_->getSummaryStatistics();
        }

        // Feedback collector stats
        if (feedback_collector_) {
            stats["feedback"] = feedback_collector_->getSummary();
        }

        // Orchestrator stats (history count, active tests)
        if (orchestrator_) {
            auto active_tests = orchestrator_->getActiveABTests();
            stats["active_ab_tests"] = active_tests.size();
        }

        // Version control stats
        if (version_control_) {
            // Could add stats for total versions, branches, etc.
            stats["version_control"]["available"] = true;
        }

        return makeResponse(http::status::ok, stats.dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// GET /api/v1/prompt_engineering/history/:id
http::response<http::string_body> PromptEngineeringApiHandler::handleGetHistory(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetHistory");
    try {
        std::string path = std::string(req.target());
        auto prompt_id = extractPathParam(path, "/api/v1/prompt_engineering/history/");

        if (prompt_id.empty()) {
            return makeErrorResponse(
                http::status::bad_request,
                "Missing prompt_id",
                req);
        }

        if (!isValidPromptEngineeringIdentifier(prompt_id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid prompt_id",
                req);
        }

        if (!orchestrator_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "SelfImprovementOrchestrator not available",
                req);
        }
        auto& orchestrator = *orchestrator_;

        auto history = orchestrator.getOptimizationHistory(prompt_id);
        nlohmann::json response = nlohmann::json::array();
        
        for (const auto& entry : history) {
            response.push_back(entry.toJson());
        }

        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// GET /api/v1/prompt_engineering/versions/:id
http::response<http::string_body> PromptEngineeringApiHandler::handleGetVersions(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetVersions");
    try {
        std::string path = std::string(req.target());
        auto prompt_id = extractPathParam(path, "/api/v1/prompt_engineering/versions/");

        if (prompt_id.empty()) {
            return makeErrorResponse(
                http::status::bad_request,
                "Missing prompt_id",
                req);
        }

        if (!isValidPromptEngineeringIdentifier(prompt_id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid prompt_id",
                req);
        }

        if (!version_control_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "PromptVersionControl not available",
                req);
        }
        auto& version_control = *version_control_;

        // Get version history
        auto versions = version_control.getHistory(prompt_id, "main", 100);
        nlohmann::json response = nlohmann::json::array();
        
        for (const auto& version : versions) {
            response.push_back(version.toJson());
        }

        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// POST /api/v1/prompt_engineering/rollback
http::response<http::string_body> PromptEngineeringApiHandler::handleRollback(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan([[maybe_unused]] "handleRollback");
    try {
        if (!orchestrator_) {
            return makeErrorResponse(
                http::status::service_unavailable,
                "SelfImprovementOrchestrator not available",
                req);
        }
        auto& orchestrator = *orchestrator_;

        auto body = nlohmann::json::parse(req.body());
        
        std::string prompt_id = body.value("prompt_id", "");
        
        if (prompt_id.empty()) {
            return makeErrorResponse(
                http::status::bad_request,
                "Missing prompt_id",
                req);
        }

        auto result = orchestrator.rollbackPrompt(prompt_id);

        nlohmann::json response;
        response["status"] = result ? "success" : "failed";
        response["prompt_id"] = prompt_id;

        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(
            http::status::bad_request,
            std::string("Invalid JSON: ") + e.what(),
            req);
    } catch (const std::exception& e) {
        return makeErrorResponse(
            http::status::internal_server_error,
            e.what(),
            req);
    }
}

// Helper methods
std::string PromptEngineeringApiHandler::extractPathParam(
    const std::string& target,
    const std::string& prefix
) {
    if (static_cast<int>(target.size()) > static_cast<int>(prefix.size()) && target.substr(0,static_cast<int>(prefix.size())) == prefix) {
        auto param = target.substr(prefix.size());
        // Remove query string if present
        auto query_pos = param.find('?');
        if (query_pos != std::string::npos) {
            param = param.substr(0, query_pos);
        }
        return param;
    }
    return "";
}

http::response<http::string_body> PromptEngineeringApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) {
    nlohmann::json error;
    error["error"] = message;
    error["status"] = static_cast<int>(status);

    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.keep_alive(req.keep_alive());
    res.body() = error.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> PromptEngineeringApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis
