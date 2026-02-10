#include "llm/llm_model_audit_logger.h"
#include <spdlog/spdlog.h>
#include <utility>

namespace themis {
namespace llm {

// Pimpl stub to keep ABI stable while the implementation is fleshed out later.
class LLMModelAuditLogger::Impl {
public:
    explicit Impl(utils::AuditLoggerConfig cfg) : config(std::move(cfg)) {}

    utils::AuditLoggerConfig config;
};

LLMModelAuditLogger::LLMModelAuditLogger(const utils::AuditLoggerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {
    spdlog::debug("LLMModelAuditLogger stub initialized: {}", impl_->config.log_path);
}

LLMModelAuditLogger::~LLMModelAuditLogger() = default;

void LLMModelAuditLogger::logInference(const LLMModelInferenceAudit& audit) {
    spdlog::debug("LLM inference audit (stub) model={} request={}", audit.model_id, audit.request_id);
}

void LLMModelAuditLogger::logEvent(
    LLMModelAuditEventType event_type,
    const std::string& model_id,
    const json& details) {
    spdlog::debug("LLM model event (stub) model={} event={} details={}", model_id, static_cast<int>(event_type), details.dump());
}

void LLMModelAuditLogger::logModelLifecycle(
    LLMModelAuditEventType /*event_type*/,
    const std::string& model_id,
    const std::string& version,
    const json& /*metadata*/) {
    spdlog::debug("LLM model lifecycle (stub) model={} version={}", model_id, version);
}

void LLMModelAuditLogger::logFineTuning(
    LLMModelAuditEventType /*event_type*/,
    const std::string& model_id,
    const std::string& base_model_id,
    int num_samples,
    float final_loss,
    const json& /*hyperparameters*/) {
    spdlog::debug(
        "LLM fine-tuning (stub) model={} base={} samples={} loss={}",
        model_id,
        base_model_id,
        num_samples,
        final_loss);
}

void LLMModelAuditLogger::logDeployment(
    LLMModelAuditEventType /*event_type*/,
    const std::string& model_id,
    const std::string& deployment_target,
    const json& /*config*/) {
    spdlog::debug("LLM deployment (stub) model={} target={}", model_id, deployment_target);
}

std::vector<json> LLMModelAuditLogger::queryLogs(
    const std::string& model_id,
    std::optional<std::chrono::system_clock::time_point> /*start_time*/,
    std::optional<std::chrono::system_clock::time_point> /*end_time*/) {
    spdlog::debug("LLM queryLogs (stub) model={}", model_id);
    return {};
}

std::vector<LLMModelInferenceAudit> LLMModelAuditLogger::getInferenceHistory(
    const std::string& model_id,
    int limit) {
    spdlog::debug("LLM getInferenceHistory (stub) model={} limit={}", model_id, limit);
    return {};
}

} // namespace llm
} // namespace themis
