/**
 * @file telemetry_keys.h
 * @brief Canonical telemetry attribute key constants.
 *
 * Defines string constants for OpenTelemetry span and metric attribute
 * keys used throughout the ThemisDB observability layer.
 */

#pragma once

#include <string_view>

namespace themis::observability::telemetry {

namespace fields {
inline constexpr std::string_view kEvent = "event";
inline constexpr std::string_view kLayerName = "layer_name";
inline constexpr std::string_view kCorrelationId = "correlation_id";
inline constexpr std::string_view kRoutingReasonCode = "routing_reason_code";
inline constexpr std::string_view kConfidencePolicyVersion = "confidence_policy_version";
inline constexpr std::string_view kConfidenceThresholdKey = "confidence_threshold_key";
inline constexpr std::string_view kFallbackMode = "fallback_mode";
inline constexpr std::string_view kFallbackReasonCode = "fallback_reason_code";
inline constexpr std::string_view kEscalationSourceLayer = "escalation_source_layer";
inline constexpr std::string_view kResolved = "resolved";
} // namespace fields

namespace events {
inline constexpr std::string_view kLayerHandoffDecision = "layer_handoff_decision";
} // namespace events

namespace layers {
inline constexpr std::string_view kAnn = "ann";
inline constexpr std::string_view kTensor = "tensor";
inline constexpr std::string_view kGraph = "graph";
inline constexpr std::string_view kFinalLayer = "final_layer";
} // namespace layers

namespace defaults {
inline constexpr std::string_view kAnnNoCorrelation = "ann-frontdoor:no-correlation";
inline constexpr std::string_view kTensorRagNoSessionCorrelation = "tensor-rag:no-session";
} // namespace defaults

namespace metadata_keys {
inline constexpr std::string_view kReasoningChain = "reasoning_chain";
} // namespace metadata_keys

} // namespace themis::observability::telemetry
