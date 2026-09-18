/**
 * @file diagnostic_events.cpp
 * @brief Diagnostic event system for structured logging and observability
 */

#include "error_codes.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

namespace themis {
namespace plugins {
namespace user_storage {

std::string DiagnosticEvent::toJsonString() const {
    json j;
    j["timestamp_ms"] = timestamp_ms;
    j["type"] = static_cast<int>(type);
    j["error_code"] = static_cast<uint16_t>(error_code);
    j["component"] = component;
    j["message"] = message;
    if (!level.empty()) {
        j["level"] = level;
    }
    if (system_errno_val != 0) {
        j["errno"] = system_errno_val;
    }
    if (!remediation.empty()) {
        j["remediation"] = remediation;
    }
    return j.dump();
}

namespace {
    DiagnosticEventHandler g_event_handler = [](const DiagnosticEvent& event) {
        auto logger = spdlog::get("user_storage_encrypted");
        if (!logger) {
            logger = spdlog::default_logger();
        }
        
        std::string level_str = {};
        switch (event.type) {
            case DiagnosticEvent::Type::ERROR_DETECTED:
                logger->error("[{}] {} - {}", event.component, 
                    errorCodeToString(event.error_code), event.message);
                if (!event.remediation.empty()) {
                    logger->warn("  Remediation: {}", event.remediation);
                }
                break;
            case DiagnosticEvent::Type::MOUNT_FAILED:
            [[fallthrough]];
            case DiagnosticEvent::Type::UNMOUNT_FAILED:
            [[fallthrough]];
            case DiagnosticEvent::Type::ROTATION_FAILED:
                logger->error("[{}] {} - {}", event.component,
                    errorCodeToString(event.error_code), event.message);
                break;
            case DiagnosticEvent::Type::STALE_MOUNT_RECONCILED:
                logger->warn("[{}] Stale mount reconciled: {}", event.component, event.message);
                break;
            case DiagnosticEvent::Type::MOUNT_STARTED:
            [[fallthrough]];
            case DiagnosticEvent::Type::UNMOUNT_STARTED:
            [[fallthrough]];
            case DiagnosticEvent::Type::ROTATION_STARTED:
                logger->info("[{}] {}", event.component, event.message);
                break;
            case DiagnosticEvent::Type::MOUNT_COMPLETED:
            [[fallthrough]];
            case DiagnosticEvent::Type::UNMOUNT_COMPLETED:
            [[fallthrough]];
            case DiagnosticEvent::Type::ROTATION_COMPLETED:
                logger->info("[{}] {}", event.component, event.message);
                break;
        }
    };
}

/**
 * @brief Register Diagnostic Event Handler.
 * @param[in] handler Input parameter.
 * @details Calls: std::move().
 */
void registerDiagnosticEventHandler(DiagnosticEventHandler handler) {
    if (handler) {
        g_event_handler = std::move(handler);
    }
}

/**
 * @brief Emit Diagnostic Event.
 * @param[in] event Input parameter.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), g_event_handler().
 */
void emitDiagnosticEvent(DiagnosticEvent event) {
    // Ensure timestamp is set
    if (event.timestamp_ms == 0) {
        auto now = std::chrono::system_clock::now();
        auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        event.timestamp_ms = ms_since_epoch;
    }
    
    if (g_event_handler) {
        g_event_handler(event);
    }
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
