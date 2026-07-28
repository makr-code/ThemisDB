/**
 * @file reason_codes.h
 * @brief Enumeration of structured reason codes for observability events.
 *
 * Centralises the machine-readable reason codes emitted alongside
 * observability events so that downstream analytics can aggregate them.
 */

#pragma once

#include <string_view>

namespace themis::observability::reason_codes {

inline constexpr std::string_view kPolicyVersionDefault = "2026-06-17";

namespace fallback_mode {
inline constexpr std::string_view kNone = "none";
inline constexpr std::string_view kDegradedContinue = "degraded_continue";
inline constexpr std::string_view kFailClosed = "fail_closed";
} // namespace fallback_mode

namespace ann {
inline constexpr std::string_view kRouteHnsw = "ANN_ROUTE_HNSW";
inline constexpr std::string_view kRouteScann = "ANN_ROUTE_SCANN";
inline constexpr std::string_view kRouteDiskann = "ANN_ROUTE_DISKANN";
inline constexpr std::string_view kRouteDistributed = "ANN_ROUTE_DISTRIBUTED";
inline constexpr std::string_view kRouteFlatBruteForce = "ANN_ROUTE_FLAT_BRUTE_FORCE";
inline constexpr std::string_view kRouteUnknown = "ANN_ROUTE_UNKNOWN";

inline constexpr std::string_view kFallbackBackendUnavailable = "ANN_BACKEND_UNAVAILABLE";
inline constexpr std::string_view kFallbackDistributedPartialFailure = "ANN_DISTRIBUTED_PARTIAL_FAILURE";
inline constexpr std::string_view kFallbackDistributedAllShardsFailed = "ANN_DISTRIBUTED_ALL_SHARDS_FAILED";

inline constexpr std::string_view kThresholdKeyDefault = "ann.default";
} // namespace ann

namespace tensor_rag {
inline constexpr std::string_view kTriggerBoth = "TENSOR_RAG_TRIGGER_BOTH";
inline constexpr std::string_view kTriggerFlare = "TENSOR_RAG_TRIGGER_FLARE";
inline constexpr std::string_view kTriggerTarg = "TENSOR_RAG_TRIGGER_TARG";
inline constexpr std::string_view kNoRetrieval = "TENSOR_RAG_NO_RETRIEVAL";

inline constexpr std::string_view kFallbackEmbeddingFnThrow = "EMBEDDING_FN_THROW";
inline constexpr std::string_view kFallbackTensorSummaryEmpty = "TENSOR_SUMMARY_EMPTY";

inline constexpr std::string_view kThresholdKeyNone = "tensor_rag:none";
inline constexpr std::string_view kThresholdKeyFlare = "flare.default";
inline constexpr std::string_view kThresholdKeyTarg = "targ.default";
inline constexpr std::string_view kThresholdKeyBoth = "tensor_rag:flare+targ";
} // namespace tensor_rag

namespace graph_truth {
inline constexpr std::string_view kOntologyValidation = "GRAPH_TRUTH_ONTOLOGY_VALIDATION";
inline constexpr std::string_view kKgValidation = "GRAPH_TRUTH_KG_VALIDATION";
inline constexpr std::string_view kNoRetriever = "GRAPH_TRUTH_NO_RETRIEVER";

inline constexpr std::string_view kFallbackRetrieverNotConfigured = "GRAPH_RETRIEVER_NOT_CONFIGURED";
} // namespace graph_truth

namespace final_layer {
inline constexpr std::string_view kPackageNotFound = "FINAL_LAYER_PACKAGE_NOT_FOUND";
inline constexpr std::string_view kRoutedByModelRule = "FINAL_LAYER_ROUTED_BY_MODEL_RULE";
inline constexpr std::string_view kSinglePackageSelected = "FINAL_LAYER_SINGLE_PACKAGE_SELECTED";
inline constexpr std::string_view kPackageUnresolved = "FINAL_LAYER_PACKAGE_UNRESOLVED";
inline constexpr std::string_view kPackageNotActive = "FINAL_LAYER_PACKAGE_NOT_ACTIVE";
inline constexpr std::string_view kPackageNotDeployable = "FINAL_LAYER_PACKAGE_NOT_DEPLOYABLE";
inline constexpr std::string_view kCompatibilityRejected = "FINAL_LAYER_COMPATIBILITY_REJECTED";
inline constexpr std::string_view kSelectedDirect = "FINAL_LAYER_SELECTED_DIRECT";

inline constexpr std::string_view kFallbackPackageNotFound = "PACKAGE_NOT_FOUND";
inline constexpr std::string_view kFallbackPackageSelectionFailed = "PACKAGE_SELECTION_FAILED";
inline constexpr std::string_view kFallbackPackageNotActive = "PACKAGE_NOT_ACTIVE";
inline constexpr std::string_view kFallbackPackageNotDeployable = "PACKAGE_NOT_DEPLOYABLE";
inline constexpr std::string_view kFallbackCompatibilityRejected = "COMPATIBILITY_REJECTED";
} // namespace final_layer

} // namespace themis::observability::reason_codes
