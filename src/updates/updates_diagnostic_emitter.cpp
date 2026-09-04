/**
 * @file updates_diagnostic_emitter.cpp
 * @brief Implementation of the diagnostic emitter for Updates module
 * @version 1.0.0
 * @since 1.8.1 (Q3 2026)
 */

#include "updates/updates_diagnostic_emitter.h"
#include "utils/logger.h"
#include <sstream>
#include <iomanip>

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

namespace themis {
namespace updates {

// ============================================================================
// DiagnosticEmitter
// ============================================================================

DiagnosticEmitter::DiagnosticEmitter() {
    // Empty constructor; listeners added via addListener()
}

void DiagnosticEmitter::addListener([[maybe_unused]] std::shared_ptr<DiagnosticListener> listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock([[maybe_unused]] listeners_mutex_);
    listeners_.push_back([[maybe_unused]] std::move(listener));
}

void DiagnosticEmitter::clearListeners() {
    std::lock_guard<std::mutex> lock([[maybe_unused]] listeners_mutex_);
    listeners_.clear();
}

size_t DiagnosticEmitter::listenerCount() const {
    std::lock_guard<std::mutex> lock([[maybe_unused]] listeners_mutex_);
    return listeners_.size();
}

void DiagnosticEmitter::invokeListeners(const ErrorContext& context, bool is_error) const {
    std::vector<std::shared_ptr<DiagnosticListener>> listeners_copy;
    {
        std::lock_guard<std::mutex> lock([[maybe_unused]] listeners_mutex_);
        listeners_copy = listeners_;
    }
    
    for ([[maybe_unused]] const auto& listener : listeners_copy) {
        try {
            listener->onDiagnosticEvent(context, is_error);
        } catch (const std::exception& e) {
            LOG_WARN("DiagnosticEmitter: listener threw exception: {}", e.what());
        }
    }
}

void DiagnosticEmitter::emitError(const ErrorContext& context) {
    // Invoke listeners first (they may buffer or forward the event)
    invokeListeners(context, true);
    
    // Emit to logger based on severity
    std::string msg = formatErrorMessage(context);
    
    switch (context.severity) {
        case DiagnosticSeverity::CRITICAL:
            LOG_ERROR("DiagnosticEmitter: {}", msg);
            break;
        case DiagnosticSeverity::ERROR:
            LOG_ERROR("DiagnosticEmitter: {}", msg);
            break;
        case DiagnosticSeverity::WARN:
            LOG_WARN("DiagnosticEmitter: {}", msg);
            break;
        case DiagnosticSeverity::INFO:
            LOG_INFO("DiagnosticEmitter: {}", msg);
            break;
    }
}

void DiagnosticEmitter::emitInfo(const std::string& operation,
                                  const std::string& phase,
                                  const std::string& message,
                                  const std::string& node_id,
                                  const std::string& version) {
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = DiagnosticErrorCode::UNKNOWN_ERROR;  // Not an error
    ctx.severity = DiagnosticSeverity::INFO;
    ctx.root_cause = RootCauseClass::UNKNOWN;
    ctx.message = message;
    ctx.operation = operation;
    ctx.phase = phase;
    ctx.node_id = node_id;
    ctx.version = version;
    
    invokeListeners(ctx, false);
    LOG_INFO("DiagnosticEmitter: {} | {} | {} [node={}, version={}]",
            operation, phase, message, 
            node_id.empty() ? "local" : node_id,
            version.empty() ? "unknown" : version);
}

void DiagnosticEmitter::emitStateTransition(const std::string& from_state,
                                            const std::string& to_state,
                                            const std::string& version) {
    std::string msg = "state transition: " + from_state + " -> " + to_state;
    emitInfo("state_transition", to_state, msg, "", version);
}

void DiagnosticEmitter::emitCheckpointCreated(uint64_t checkpoint_id,
                                              const std::string& description,
                                              const std::string& version) {
    std::string msg = "checkpoint created: id=" + std::to_string(checkpoint_id);
    if (!description.empty()) {
        msg += ", description=" + description;
    }
    
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = DiagnosticErrorCode::UNKNOWN_ERROR;
    ctx.severity = DiagnosticSeverity::INFO;
    ctx.root_cause = RootCauseClass::UNKNOWN;
    ctx.message = msg;
    ctx.operation = "create_checkpoint";
    ctx.phase = "checkpointing";
    ctx.version = version;
    ctx.extra_context["checkpoint_id"] = checkpoint_id;
    
    invokeListeners(ctx, false);
    LOG_INFO("DiagnosticEmitter: checkpoint created [id={}, version={}]", 
            checkpoint_id, version.empty() ? "unknown" : version);
}

void DiagnosticEmitter::emitCheckpointRollback(uint64_t checkpoint_id,
                                               bool success,
                                               const std::string& reason) {
    std::string msg = "checkpoint rollback: id=" + std::to_string(checkpoint_id) +
                     ", success=" + (success ? "true" : "false");
    if (!reason.empty()) {
        msg += ", reason=" + reason;
    }
    
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = success ? DiagnosticErrorCode::UNKNOWN_ERROR 
                            : DiagnosticErrorCode::ROLLBACK_FAILED;
    ctx.severity = success ? DiagnosticSeverity::INFO : DiagnosticSeverity::WARN;
    ctx.root_cause = success ? RootCauseClass::UNKNOWN : RootCauseClass::CASCADE;
    ctx.message = msg;
    ctx.operation = "rollback_checkpoint";
    ctx.phase = "rolling_back";
    ctx.extra_context["checkpoint_id"] = checkpoint_id;
    
    invokeListeners(ctx, !success);
    if (success) {
        LOG_INFO("DiagnosticEmitter: checkpoint rollback succeeded [id={}]", checkpoint_id);
    } else {
        LOG_WARN("DiagnosticEmitter: checkpoint rollback failed [id={}, reason={}]", 
                checkpoint_id, reason.empty() ? "unknown" : reason);
    }
}

void DiagnosticEmitter::emitPatchApply(const std::string& file_path,
                                       bool success,
                                       const std::string& error_msg) {
    std::string msg = "patch apply: file=" + file_path + ", success=" + 
                     (success ? "true" : "false");
    if (!error_msg.empty()) {
        msg += ", error=" + error_msg;
    }
    
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = success ? DiagnosticErrorCode::UNKNOWN_ERROR 
                            : DiagnosticErrorCode::PATCH_APPLY_FAILED;
    ctx.severity = success ? DiagnosticSeverity::INFO : DiagnosticSeverity::ERROR;
    ctx.root_cause = success ? RootCauseClass::UNKNOWN : RootCauseClass::ARTIFACT;
    ctx.message = msg;
    ctx.operation = "apply_patch";
    ctx.phase = "applying";
    ctx.extra_context["file_path"] = file_path;
    
    invokeListeners(ctx, !success);
    if (success) {
        LOG_INFO("DiagnosticEmitter: patch applied [file={}]", file_path);
    } else {
        LOG_ERROR("DiagnosticEmitter: patch apply failed [file={}, error={}]", 
                 file_path, error_msg.empty() ? "unknown" : error_msg);
    }
}

void DiagnosticEmitter::emitCoordinatedEvent(const std::string& operation,
                                             const std::string& node_id,
                                             bool success,
                                             const std::string& detail) {
    std::string msg = operation + ": node=" + node_id + ", success=" + 
                     (success ? "true" : "false");
    if (!detail.empty()) {
        msg += ", detail=" + detail;
    }
    
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = success ? DiagnosticErrorCode::UNKNOWN_ERROR 
                            : DiagnosticErrorCode::COORDINATION_PEER_FAILED;
    ctx.severity = success ? DiagnosticSeverity::INFO : DiagnosticSeverity::ERROR;
    ctx.root_cause = success ? RootCauseClass::UNKNOWN : RootCauseClass::NETWORK;
    ctx.message = msg;
    ctx.operation = operation;
    ctx.phase = "coordinating";
    ctx.node_id = node_id;
    
    invokeListeners(ctx, !success);
    if (success) {
        LOG_INFO("DiagnosticEmitter: coordinated event [op={}, node={}]", 
                operation, node_id);
    } else {
        LOG_ERROR("DiagnosticEmitter: coordinated event failed [op={}, node={}, detail={}]", 
                 operation, node_id, detail.empty() ? "unknown" : detail);
    }
}

std::string DiagnosticEmitter::formatErrorMessage(const ErrorContext& context) {
    std::ostringstream oss;
    
    oss << "[" << errorCodeName(context.error_code) << ":" 
        << static_cast<uint16_t>(context.error_code) << "] ";
    
    if (!context.operation.empty()) {
        oss << "Operation '" << context.operation << "' ";
    }
    
    if (!context.phase.empty()) {
        oss << "in phase '" << context.phase << "' ";
    }
    
    if (!context.node_id.empty()) {
        oss << "on node '" << context.node_id << "': ";
    } else {
        oss << "(local): ";
    }
    
    oss << context.message;
    
    oss << " [severity=" << severityName(context.severity)
        << ", root_cause=" << static_cast<uint16_t>(context.root_cause) << "]";
    
    return oss.str();
}

} // namespace updates
} // namespace themis
