#pragma once

#include "observability/telemetry_keys.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

#include <string_view>

namespace themis::observability {

/**
 * @brief Emits one normalized structured log line for layer handoff decisions.
 */

/**
 * @file layer_decision_log.h
 * @brief Structured logging for layer routing and model-selection decisions.
 *
 * Declares LayerDecisionLog, a lightweight sink that records which
 * processing layer was chosen for each request and the contributing factors.
 */
inline void emitLayerDecisionLog(std::string_view layer_name,
                                 std::string_view correlation_id,
                                 std::string_view routing_reason_code,
                                 std::string_view confidence_policy_version,
                                 std::string_view confidence_threshold_key,
                                 std::string_view fallback_mode,
                                 std::string_view fallback_reason_code,
                                 std::string_view escalation_source_layer,
                                 bool resolved = true) {
    nlohmann::json payload = nlohmann::json::object();
    payload[std::string(telemetry::fields::kEvent)] = std::string(telemetry::events::kLayerHandoffDecision);
    payload[std::string(telemetry::fields::kLayerName)] = std::string(layer_name);
    payload[std::string(telemetry::fields::kCorrelationId)] = std::string(correlation_id);
    payload[std::string(telemetry::fields::kRoutingReasonCode)] = std::string(routing_reason_code);
    payload[std::string(telemetry::fields::kConfidencePolicyVersion)] = std::string(confidence_policy_version);
    payload[std::string(telemetry::fields::kConfidenceThresholdKey)] = std::string(confidence_threshold_key);
    payload[std::string(telemetry::fields::kFallbackMode)] = std::string(fallback_mode);
    payload[std::string(telemetry::fields::kFallbackReasonCode)] = std::string(fallback_reason_code);
    payload[std::string(telemetry::fields::kEscalationSourceLayer)] = std::string(escalation_source_layer);
    payload[std::string(telemetry::fields::kResolved)] = resolved;

    THEMIS_INFO("{}", payload.dump());
}

} // namespace themis::observability
