/**
 * @file tensor_summary_types.cpp
 * @brief Tensor summary types factory implementation.
 */

#include "tensor/tensor_summary_types.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace tensor {

// ============================================================================
// Helper: Get current ISO-8601 timestamp
// ============================================================================

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// ============================================================================
// SummaryFactory implementation
// ============================================================================

AdapterSummary SummaryFactory::createAdapterSummary(
    const std::string&        adapter_key,
    const std::string&        base_model_id,
    const CompressionResult&  compression_result) {

    AdapterSummary summary;
    summary.id = adapter_key;
    summary.adapter_key = adapter_key;
    summary.base_model_id = base_model_id;
    summary.compression_strategy = "TT_DECOMPOSITION";  // Default
    summary.compression_info = compression_result;
    summary.created_at = getCurrentTimestamp();
    
    // Extract compression ratio and parameters
    if (compression_result.success) {
        summary.compression_info.compression_ratio = compression_result.compression_ratio;
        summary.avg_tt_rank = compression_result.achieved_rank;
    }
    
    summary.inference_ready = true;
    summary.routing_reason = "adapter_summary_created";
    
    return summary;
}

PackageSummary SummaryFactory::createPackageSummary(
    const std::string&              package_id,
    const std::vector<std::string>& adapter_keys) {

    PackageSummary summary;
    summary.id = package_id;
    summary.package_id = package_id;
    summary.adapter_keys = adapter_keys;
    summary.adapter_count = adapter_keys.size();
    summary.created_at = getCurrentTimestamp();
    summary.production_ready = true;
    summary.routing_reason = "package_summary_created";
    
    // Default version and description
    summary.version = "v1.0";
    summary.description = "Package containing " + std::to_string(adapter_keys.size()) + " adapters";
    
    return summary;
}

ShardSummary SummaryFactory::createShardSummary(
    const std::string&        shard_id,
    std::size_t               candidates_before,
    const CompressionResult&  compression_result) {

    ShardSummary summary;
    summary.id = shard_id;
    summary.shard_id = shard_id;
    summary.candidates_before_compression = candidates_before;
    summary.candidates_after_compression = 
        candidates_before > 0 ? 
        static_cast<std::size_t>(candidates_before / compression_result.compression_ratio) :
        0;
    summary.compression_info = compression_result;
    summary.created_at = getCurrentTimestamp();
    summary.shard_healthy = true;
    summary.routing_reason = "shard_summary_created";
    
    return summary;
}

} // namespace tensor
} // namespace themis
