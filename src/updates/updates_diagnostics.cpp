/**
 * @file updates_diagnostics.cpp
 * @brief Implementation of diagnostics primitives for the Updates module
 * @version 1.0.0
 * @since 1.8.1 (Q3 2026)
 */

#include "updates/updates_diagnostics.h"
#include <iomanip>
#include <sstream>

namespace themis {
namespace updates {

// ============================================================================
// ErrorContext serialization
// ============================================================================

json ErrorContext::toJson() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm_val = {};
#ifdef _WIN32
    gmtime_s(&tm_val, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_val);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);

    json j;
    j["timestamp"]      = buf;
    j["error_code"]     = static_cast<uint16_t>(error_code);
    j["error_name"]     = errorCodeName(error_code);
    j["severity"]       = severityName(severity);
    j["root_cause"]     = static_cast<uint16_t>(root_cause);
    j["message"]        = message;
    j["operation"]      = operation;
    j["phase"]          = phase;
    j["node_id"]        = node_id;
    j["version"]        = version;
    j["extra_context"]  = extra_context;
    
    return j;
}

std::optional<ErrorContext> ErrorContext::fromJson(const json& j) {
    try {
        ErrorContext ctx;
        
        // Parse timestamp
        std::string ts = j.value("timestamp", "");
        if (!ts.empty()) {
            std::tm tm_val = {};
            std::istringstream ss(ts);
            ss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%SZ");
            if (!ss.fail()) {
#ifdef _WIN32
                auto time_t_val = _mkgmtime(&tm_val);
#else
                auto time_t_val = timegm(&tm_val);
#endif
                ctx.timestamp = std::chrono::system_clock::from_time_t(time_t_val);
            }
        } else {
            ctx.timestamp = std::chrono::system_clock::now();
        }
        
        // Parse error code
        uint16_t code = j.value("error_code", 7499);
        ctx.error_code = static_cast<DiagnosticErrorCode>(code);
        
        // Parse severity
        std::string sev = j.value("severity", "ERROR");
        if (sev == "INFO") {
          ctx.severity = DiagnosticSeverity::INFO;
        }
        else if (sev == "WARN") ctx.severity = DiagnosticSeverity::WARN;
        else if (sev == "CRITICAL") ctx.severity = DiagnosticSeverity::CRITICAL;
        else ctx.severity = DiagnosticSeverity::ERROR;
        
        // Parse root cause
        uint16_t cause = j.value("root_cause", 7);
        ctx.root_cause = static_cast<RootCauseClass>(cause);
        
        // Parse other fields
        ctx.message = j.value("message", "");
        ctx.operation = j.value("operation", "");
        ctx.phase = j.value("phase", "");
        ctx.node_id = j.value("node_id", "");
        ctx.version = j.value("version", "");
        
        if (j.contains("extra_context")) {
            ctx.extra_context = j["extra_context"];
        }
        
        return ctx;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace updates
} // namespace themis
