/**
 * @file distributed_training_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=24; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=21, Debt=0, C=4, H=7, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/distributed_training_coordinator.h"
#include "llm/byzantine_detector.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <future>
#include <set>

namespace themis {
namespace llm {

// ============================================================================
// DistributedTrainingConfig JSON Serialization
// ============================================================================

json DistributedTrainingConfig::toJSON() const {
    json j;
    j["sync_strategy"] = static_cast<int>(sync_strategy);
    j["compression"] = static_cast<int>(compression);
    j["coordinator_shard"] = coordinator_shard;
    j["participant_shards"] = participant_shards;
    j["gradient_accumulation_steps"] = gradient_accumulation_steps;
    j["sync_frequency"] = sync_frequency;
    j["gradient_clip_norm"] = gradient_clip_norm;
    j["use_mixed_precision"] = use_mixed_precision;
    j["sparse_gradients"] = sparse_gradients;
    j["sparse_threshold"] = sparse_threshold;
    j["max_retry_attempts"] = max_retry_attempts;
    j["timeout_seconds"] = timeout_seconds;
    j["enable_checkpointing"] = enable_checkpointing;
    j["checkpoint_frequency"] = checkpoint_frequency;
    j["checkpoint_path"] = checkpoint_path;
    j["enable_byzantine_detection"] = enable_byzantine_detection;
    j["detection_method"] = static_cast<int>(detection_method);
    j["detection_threshold"] = detection_threshold;
    j["max_byzantine_shards"] = max_byzantine_shards;
    j["byzantine_action"] = static_cast<int>(byzantine_action);
    return j;
}

DistributedTrainingConfig DistributedTrainingConfig::fromJSON(const json& j) {
    DistributedTrainingConfig config;
    if (j.contains("sync_strategy")) 
        config.sync_strategy = static_cast<SyncStrategy>(j["sync_strategy"].get<int>());
    if (j.contains("compression")) 
        config.compression = static_cast<GradientCompressionType>(j["compression"].get<int>());
    if (j.contains("coordinator_shard")) 
        config.coordinator_shard = j["coordinator_shard"].get<std::string>();
    if (j.contains("participant_shards")) 
        config.participant_shards = j["participant_shards"].get<std::vector<std::string>>();
    if (j.contains("gradient_accumulation_steps")) 
        config.gradient_accumulation_steps = j["gradient_accumulation_steps"].get<int>();
    if (j.contains("sync_frequency")) 
        config.sync_frequency = j["sync_frequency"].get<int>();
    if (j.contains("gradient_clip_norm")) 
        config.gradient_clip_norm = j["gradient_clip_norm"].get<float>();
    if (j.contains("use_mixed_precision")) 
        config.use_mixed_precision = j["use_mixed_precision"].get<bool>();
    if (j.contains("sparse_gradients")) 
        config.sparse_gradients = j["sparse_gradients"].get<bool>();
    if (j.contains("sparse_threshold")) 
        config.sparse_threshold = j["sparse_threshold"].get<float>();
    if (j.contains("max_retry_attempts")) 
        config.max_retry_attempts = j["max_retry_attempts"].get<int>();
    if (j.contains("timeout_seconds")) 
        config.timeout_seconds = j["timeout_seconds"].get<int>();
    if (j.contains("enable_checkpointing")) 
        config.enable_checkpointing = j["enable_checkpointing"].get<bool>();
    if (j.contains("checkpoint_frequency")) 
        config.checkpoint_frequency = j["checkpoint_frequency"].get<int>();
    if (j.contains("checkpoint_path")) 
        config.checkpoint_path = j["checkpoint_path"].get<std::string>();
    if (j.contains("enable_byzantine_detection"))
        config.enable_byzantine_detection = j["enable_byzantine_detection"].get<bool>();
    if (j.contains("detection_method"))
        config.detection_method = static_cast<ByzantineDetectionMethod>(j["detection_method"].get<int>());
    if (j.contains("detection_threshold"))
        config.detection_threshold = j["detection_threshold"].get<float>();
    if (j.contains("max_byzantine_shards"))
        config.max_byzantine_shards = j["max_byzantine_shards"].get<int>();
    if (j.contains("byzantine_action"))
        config.byzantine_action = static_cast<ByzantineAction>(j["byzantine_action"].get<int>());
    return config;
}

// ============================================================================
// GradientTensor Implementation
// ============================================================================

size_t GradientTensor::compressed_size() const {
    if (compressed_data.has_value()) {
        return compressed_data->size();
    }
    return uncompressed_size();
}

void GradientTensor::compress(GradientCompressionType type) {
    compression_type = type;
    
    switch (type) {
        case GradientCompressionType::NONE:
            compressed_data.reset();
            break;
            
        case GradientCompressionType::QUANTIZATION_8BIT: {
            // Simple 8-bit quantization: map float32 to uint8
            if (data.empty()) break;
            
            // Find min/max for scaling
            float min_val = *std::min_element(data.begin(), data.end());
            float max_val = *std::max_element(data.begin(), data.end());
            float scale = (max_val - min_val) / 255.0f;
            
            // Store scale and min as metadata (first 8 bytes)
            std::vector<uint8_t> compressed;
            compressed.reserve(data.size() + 8);
            
            // Store scale and min
            uint32_t scale_bits, min_bits;
            memcpy(&scale_bits, &scale, sizeof(float));
            memcpy(&min_bits, &min_val, sizeof(float));
            compressed.push_back((scale_bits >> 24) & 0xFF);
            compressed.push_back((scale_bits >> 16) & 0xFF);
            compressed.push_back((scale_bits >> 8) & 0xFF);
            compressed.push_back(scale_bits & 0xFF);
            compressed.push_back((min_bits >> 24) & 0xFF);
            compressed.push_back((min_bits >> 16) & 0xFF);
            compressed.push_back((min_bits >> 8) & 0xFF);
            compressed.push_back(min_bits & 0xFF);
            
            // Quantize values
            for (float val : data) {
                uint8_t quantized = scale > 0 ? 
                    static_cast<uint8_t>(std::round((val - min_val) / scale)) : 0;
                compressed.push_back(quantized);
            }
            
            compressed_data = compressed;
            break;
        }
        
        case GradientCompressionType::QUANTIZATION_4BIT: {
            // 4-bit quantization: pack two values per byte
            if (data.empty()) break;
            
            float min_val = *std::min_element(data.begin(), data.end());
            float max_val = *std::max_element(data.begin(), data.end());
            float scale = (max_val - min_val) / 15.0f;
            
            std::vector<uint8_t> compressed;
            compressed.reserve((data.size() + 1) / 2 + 8);
            
            // Store metadata
            uint32_t scale_bits, min_bits;
            memcpy(&scale_bits, &scale, sizeof(float));
            memcpy(&min_bits, &min_val, sizeof(float));
            compressed.push_back((scale_bits >> 24) & 0xFF);
            compressed.push_back((scale_bits >> 16) & 0xFF);
            compressed.push_back((scale_bits >> 8) & 0xFF);
            compressed.push_back(scale_bits & 0xFF);
            compressed.push_back((min_bits >> 24) & 0xFF);
            compressed.push_back((min_bits >> 16) & 0xFF);
            compressed.push_back((min_bits >> 8) & 0xFF);
            compressed.push_back(min_bits & 0xFF);
            
            // Quantize and pack
            for (size_t i = 0; i < data.size(); i += 2) {
                uint8_t val1 = scale > 0 ? 
                    static_cast<uint8_t>(std::round((data[i] - min_val) / scale)) & 0x0F : 0;
                uint8_t val2 = (i + 1 < data.size() && scale > 0) ? 
                    static_cast<uint8_t>(std::round((data[i+1] - min_val) / scale)) & 0x0F : 0;
                compressed.push_back((val1 << 4) | val2);
            }
            
            compressed_data = compressed;
            break;
        }
        
        case GradientCompressionType::SPARSE_TOPK: {
            // Keep only top 10% of gradients by magnitude
            if (data.empty()) break;
            
            size_t k = std::max(size_t(1), data.size() / 10);
            
            // Create indices sorted by absolute value
            std::vector<std::pair<size_t, float>> indexed_vals;
            indexed_vals.reserve(data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                indexed_vals.push_back({i, std::abs(data[i])});
            }
            std::partial_sort(indexed_vals.begin(), indexed_vals.begin() + k, 
                            indexed_vals.end(),
                            [](const auto& a, const auto& b) { return a.second > b.second; });
            
            // Store as sparse format: [count(4), (index(4), value(4))...]
            std::vector<uint8_t> compressed;
            compressed.reserve(4 + k * 8);
            
            uint32_t count = static_cast<uint32_t>(k);
            compressed.push_back((count >> 24) & 0xFF);
            compressed.push_back((count >> 16) & 0xFF);
            compressed.push_back((count >> 8) & 0xFF);
            compressed.push_back(count & 0xFF);
            
            for (size_t i = 0; i < k; ++i) {
                uint32_t idx = static_cast<uint32_t>(indexed_vals[i].first);
                float val = data[idx];
                uint32_t val_bits;
                memcpy(&val_bits, &val, sizeof(float));
                
                compressed.push_back((idx >> 24) & 0xFF);
                compressed.push_back((idx >> 16) & 0xFF);
                compressed.push_back((idx >> 8) & 0xFF);
                compressed.push_back(idx & 0xFF);
                compressed.push_back((val_bits >> 24) & 0xFF);
                compressed.push_back((val_bits >> 16) & 0xFF);
                compressed.push_back((val_bits >> 8) & 0xFF);
                compressed.push_back(val_bits & 0xFF);
            }
            
            compressed_data = compressed;
            break;
        }
        
        case GradientCompressionType::ERROR_FEEDBACK:
            // For now, same as no compression (error feedback requires stateful tracking)
            compressed_data.reset();
            break;
    }
}

void GradientTensor::decompress() {
    if (!compressed_data.has_value() || compression_type == GradientCompressionType::NONE) {
        return;
    }
    
    const auto& compressed = compressed_data.value();
    
    switch (compression_type) {
        case GradientCompressionType::QUANTIZATION_8BIT: {
            if (compressed.size() < 8) break;
            
            // Extract scale and min
            uint32_t scale_bits = (compressed[0] << 24) | (compressed[1] << 16) | 
                                 (compressed[2] << 8) | compressed[3];
            uint32_t min_bits = (compressed[4] << 24) | (compressed[5] << 16) | 
                               (compressed[6] << 8) | compressed[7];
            float scale, min_val;
            memcpy(&scale, &scale_bits, sizeof(float));
            memcpy(&min_val, &min_bits, sizeof(float));
            
            // Dequantize
            data.clear();
            data.reserve(compressed.size() - 8);
            for (size_t i = 8; i < compressed.size(); ++i) {
                data.push_back(compressed[i] * scale + min_val);
            }
            break;
        }
        
        case GradientCompressionType::QUANTIZATION_4BIT: {
            if (compressed.size() < 8) break;
            
            // Extract metadata
            uint32_t scale_bits = (compressed[0] << 24) | (compressed[1] << 16) | 
                                 (compressed[2] << 8) | compressed[3];
            uint32_t min_bits = (compressed[4] << 24) | (compressed[5] << 16) | 
                               (compressed[6] << 8) | compressed[7];
            float scale, min_val;
            memcpy(&scale, &scale_bits, sizeof(float));
            memcpy(&min_val, &min_bits, sizeof(float));
            
            // Dequantize
            data.clear();
            data.reserve((compressed.size() - 8) * 2);
            for (size_t i = 8; i < compressed.size(); ++i) {
                uint8_t packed = compressed[i];
                uint8_t val1 = (packed >> 4) & 0x0F;
                uint8_t val2 = packed & 0x0F;
                data.push_back(val1 * scale + min_val);
                data.push_back(val2 * scale + min_val);
            }
            break;
        }
        
        case GradientCompressionType::SPARSE_TOPK: {
            if (compressed.size() < 4) break;
            
            // Extract count
            uint32_t count = (compressed[0] << 24) | (compressed[1] << 16) | 
                            (compressed[2] << 8) | compressed[3];
            
            // Initialize to zeros
            data.assign(shape[0] * (shape.size() > 1 ? shape[1] : 1), 0.0f);
            
            // Fill in sparse values
            size_t pos = 4;
            for (uint32_t i = 0; i < count && pos + 8 <= compressed.size(); ++i) {
                uint32_t idx = (compressed[pos] << 24) | (compressed[pos+1] << 16) | 
                              (compressed[pos+2] << 8) | compressed[pos+3];
                uint32_t val_bits = (compressed[pos+4] << 24) | (compressed[pos+5] << 16) | 
                                   (compressed[pos+6] << 8) | compressed[pos+7];
                float val;
                memcpy(&val, &val_bits, sizeof(float));
                
                if (idx < data.size()) {
                    data[idx] = val;
                }
                pos += 8;
            }
            break;
        }
        
        default:
            break;
    }
    
    compressed_data.reset();
    compression_type = GradientCompressionType::NONE;
}

json GradientTensor::toJSON() const {
    json j;
    j["layer_name"] = layer_name;
    j["shape"] = shape;
    j["source_shard"] = source_shard;
    j["timestamp_ms"] = timestamp_ms;
    j["step_number"] = step_number;
    j["compression_type"] = static_cast<int>(compression_type);
    
    if (compressed_data.has_value()) {
        j["compressed_data"] = compressed_data.value();
    } else {
        j["data"] = data;
    }
    
    return j;
}

GradientTensor GradientTensor::fromJSON(const json& j) {
    GradientTensor tensor;
    if (j.contains("layer_name")) tensor.layer_name = j["layer_name"].get<std::string>();
    if (j.contains("shape")) tensor.shape = j["shape"].get<std::vector<int>>();
    if (j.contains("source_shard")) tensor.source_shard = j["source_shard"].get<std::string>();
    if (j.contains("timestamp_ms")) tensor.timestamp_ms = j["timestamp_ms"].get<int64_t>();
    if (j.contains("step_number")) tensor.step_number = j["step_number"].get<int>();
    if (j.contains("compression_type")) 
        tensor.compression_type = static_cast<GradientCompressionType>(j["compression_type"].get<int>());
    
    if (j.contains("compressed_data")) {
        tensor.compressed_data = j["compressed_data"].get<std::vector<uint8_t>>();
    } else if (j.contains("data")) {
        tensor.data = j["data"].get<std::vector<float>>();
    }
    
    return tensor;
}

// ============================================================================
// GradientExchangeMessage Implementation
// ============================================================================

json GradientExchangeMessage::toJSON() const {
    json j;
    j["message_id"] = message_id;
    j["source_shard"] = source_shard;
    j["destination_shard"] = destination_shard;
    j["iteration_number"] = iteration_number;
    j["total_participants"] = total_participants;
    j["participants_seen"] = participants_seen;
    j["sent_timestamp_ms"] = sent_timestamp_ms;
    j["received_timestamp_ms"] = received_timestamp_ms;
    
    j["gradients"] = json::array();
    for (const auto& grad : gradients) {
        j["gradients"].push_back(grad.toJSON());
    }
    
    // Serialize loss metrics
    if (local_loss.has_value()) {
        j["local_loss"] = local_loss.value();
    }
    if (local_accuracy.has_value()) {
        j["local_accuracy"] = local_accuracy.value();
    }
    j["samples_in_batch"] = samples_in_batch;
    
    return j;
}

GradientExchangeMessage GradientExchangeMessage::fromJSON(const json& j) {
    GradientExchangeMessage msg;
    if (j.contains("message_id")) msg.message_id = j["message_id"].get<std::string>();
    if (j.contains("source_shard")) msg.source_shard = j["source_shard"].get<std::string>();
    if (j.contains("destination_shard")) msg.destination_shard = j["destination_shard"].get<std::string>();
    if (j.contains("iteration_number")) msg.iteration_number = j["iteration_number"].get<int>();
    if (j.contains("total_participants")) msg.total_participants = j["total_participants"].get<int>();
    if (j.contains("participants_seen")) 
        msg.participants_seen = j["participants_seen"].get<std::vector<std::string>>();
    if (j.contains("sent_timestamp_ms")) msg.sent_timestamp_ms = j["sent_timestamp_ms"].get<int64_t>();
    if (j.contains("received_timestamp_ms")) msg.received_timestamp_ms = j["received_timestamp_ms"].get<int64_t>();
    
    if (j.contains("gradients")) {
        for (const auto& grad_json : j["gradients"]) {
            msg.gradients.push_back(GradientTensor::fromJSON(grad_json));
        }
    }
    
    // Deserialize loss metrics
    if (j.contains("local_loss")) {
        msg.local_loss = j["local_loss"].get<float>();
    }
    if (j.contains("local_accuracy")) {
        msg.local_accuracy = j["local_accuracy"].get<float>();
    }
    if (j.contains("samples_in_batch")) {
        msg.samples_in_batch = j["samples_in_batch"].get<int>();
    }
    
    return msg;
}

// ============================================================================
// ShardTrainingState Implementation
// ============================================================================

json ShardTrainingState::toJSON() const {
    json j;
    j["shard_id"] = shard_id;
    j["current_epoch"] = current_epoch;
    j["current_step"] = current_step;
    j["total_steps"] = total_steps;
    j["current_loss"] = current_loss;
    j["avg_grad_norm"] = avg_grad_norm;
    j["samples_processed"] = samples_processed;
    j["is_active"] = is_active;
    j["is_synchronized"] = is_synchronized;
    j["last_heartbeat_ms"] = last_heartbeat_ms;
    j["consecutive_failures"] = consecutive_failures;
    j["gpu_utilization"] = gpu_utilization;
    j["memory_usage_gb"] = memory_usage_gb;
    return j;
}

ShardTrainingState ShardTrainingState::fromJSON(const json& j) {
    ShardTrainingState state;
    if (j.contains("shard_id")) state.shard_id = j["shard_id"].get<std::string>();
    if (j.contains("current_epoch")) state.current_epoch = j["current_epoch"].get<int>();
    if (j.contains("current_step")) state.current_step = j["current_step"].get<int>();
    if (j.contains("total_steps")) state.total_steps = j["total_steps"].get<int>();
    if (j.contains("current_loss")) state.current_loss = j["current_loss"].get<float>();
    if (j.contains("avg_grad_norm")) state.avg_grad_norm = j["avg_grad_norm"].get<float>();
    if (j.contains("samples_processed")) state.samples_processed = j["samples_processed"].get<int>();
    if (j.contains("is_active")) state.is_active = j["is_active"].get<bool>();
    if (j.contains("is_synchronized")) state.is_synchronized = j["is_synchronized"].get<bool>();
    if (j.contains("last_heartbeat_ms")) state.last_heartbeat_ms = j["last_heartbeat_ms"].get<int64_t>();
    if (j.contains("consecutive_failures")) state.consecutive_failures = j["consecutive_failures"].get<int>();
    if (j.contains("gpu_utilization")) state.gpu_utilization = j["gpu_utilization"].get<float>();
    if (j.contains("memory_usage_gb")) state.memory_usage_gb = j["memory_usage_gb"].get<float>();
    return state;
}

// ============================================================================
// DistributedTrainingStats Implementation
// ============================================================================

json DistributedTrainingStats::toJSON() const {
    json j;
    j["total_steps_completed"] = total_steps_completed;
    j["total_gradient_syncs"] = total_gradient_syncs;
    j["total_bytes_sent"] = total_bytes_sent;
    j["total_bytes_received"] = total_bytes_received;
    j["avg_sync_time_ms"] = avg_sync_time_ms;
    j["max_sync_time_ms"] = max_sync_time_ms;
    j["compression_ratio"] = compression_ratio;
    j["bandwidth_saved_gb"] = bandwidth_saved_gb;
    j["total_retries"] = total_retries;
    j["shard_failures"] = shard_failures;
    j["successful_recoveries"] = successful_recoveries;
    j["effective_speedup"] = effective_speedup;
    j["communication_overhead_pct"] = communication_overhead_pct;
    j["byzantine_detections"] = byzantine_detections;
    j["byzantine_shards_excluded"] = byzantine_shards_excluded;
    j["per_shard_detection_count"] = per_shard_detection_count;
    j["avg_anomaly_score"] = avg_anomaly_score;
    j["gradient_norm_history"] = gradient_norm_history;
    return j;
}

// ============================================================================
// AllReduceAggregator Implementation
// ============================================================================

std::vector<GradientTensor> AllReduceAggregator::aggregate(
    const std::vector<std::vector<GradientTensor>>& shard_gradients
) {
    if (shard_gradients.empty()) {
        return {};
    }
    
    // Use first shard's gradients as template
    const auto& template_grads = shard_gradients[0];
    std::vector<GradientTensor> aggregated;
    aggregated.reserve(template_grads.size());
    
    // For each layer
    for (size_t layer_idx = 0; layer_idx < template_grads.size(); ++layer_idx) {
        GradientTensor agg_tensor;
        agg_tensor.layer_name = template_grads[layer_idx].layer_name;
        agg_tensor.shape = template_grads[layer_idx].shape;
        agg_tensor.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Initialize with zeros
        size_t tensor_size = template_grads[layer_idx].data.size();
        agg_tensor.data.resize(tensor_size, 0.0f);
        
        // Sum gradients from all shards
        int valid_shards = 0;
        for (const auto& shard_grad_list : shard_gradients) {
            if (layer_idx >= shard_grad_list.size()) continue;
            
            const auto& grad = shard_grad_list[layer_idx];
            if (grad.data.size() != tensor_size) {
                spdlog::warn("Gradient size mismatch for layer {}: expected {}, got {}", 
                           grad.layer_name, tensor_size, grad.data.size());
                continue;
            }
            
            for (size_t i = 0; i < tensor_size; ++i) {
                agg_tensor.data[i] += grad.data[i];
            }
            valid_shards++;
        }
        
        // Average
        if (valid_shards > 0) {
            float scale = 1.0f / valid_shards;
            for (auto& val : agg_tensor.data) {
                val *= scale;
            }
        }
        
        aggregated.push_back(std::move(agg_tensor));
    }
    
    return aggregated;
}

// ============================================================================
// ParameterServerAggregator Implementation
// ============================================================================

std::vector<GradientTensor> ParameterServerAggregator::aggregate(
    const std::vector<std::vector<GradientTensor>>& shard_gradients
) {
    if (shard_gradients.empty()) {
        return {};
    }
    
    const auto& template_grads = shard_gradients[0];
    std::vector<GradientTensor> aggregated;
    aggregated.reserve(template_grads.size());
    
    // Calculate total weight
    float total_weight = 0.0f;
    for (const auto& [shard_id, weight] : shard_weights_) {
        total_weight += weight;
    }
    
    if (total_weight <= 0.0f) {
        // Fall back to simple averaging
        AllReduceAggregator fallback;
        return fallback.aggregate(shard_gradients);
    }
    
    // For each layer
    for (size_t layer_idx = 0; layer_idx < template_grads.size(); ++layer_idx) {
        GradientTensor agg_tensor;
        agg_tensor.layer_name = template_grads[layer_idx].layer_name;
        agg_tensor.shape = template_grads[layer_idx].shape;
        agg_tensor.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        size_t tensor_size = template_grads[layer_idx].data.size();
        agg_tensor.data.resize(tensor_size, 0.0f);
        
        // Weighted sum
        for (size_t shard_idx = 0; shard_idx < shard_gradients.size(); ++shard_idx) {
            const auto& shard_grad_list = shard_gradients[shard_idx];
            if (layer_idx >= shard_grad_list.size()) continue;
            
            const auto& grad = shard_grad_list[layer_idx];
            if (grad.data.size() != tensor_size) continue;
            
            // Get weight for this shard
            float weight = 1.0f / shard_gradients.size();  // Default weight
            auto it = shard_weights_.find(grad.source_shard);
            if (it != shard_weights_.end()) {
                weight = it->second / total_weight;
            }
            
            for (size_t i = 0; i < tensor_size; ++i) {
                agg_tensor.data[i] += grad.data[i] * weight;
            }
        }
        
        aggregated.push_back(std::move(agg_tensor));
    }
    
    return aggregated;
}

// ============================================================================
// RingAllReduceAggregator Implementation
// ============================================================================

void RingAllReduceAggregator::setRingTopology(const std::vector<std::string>& ring_order) {
    ring_order_ = ring_order;
    spdlog::info("Ring topology set with {} nodes", ring_order.size());
}

std::vector<GradientTensor> RingAllReduceAggregator::aggregate(
    const std::vector<std::vector<GradientTensor>>& shard_gradients
) {
    // For simplicity, fall back to AllReduce for now
    // Full ring-allreduce would require multiple communication rounds
    // which would be implemented in a real distributed system
    spdlog::info("Ring-AllReduce using simplified all-reduce for {} shards", 
               shard_gradients.size());
    
    AllReduceAggregator fallback;
    return fallback.aggregate(shard_gradients);
}

// ============================================================================
// DistributedTrainingCoordinator Implementation
// ============================================================================

DistributedTrainingCoordinator::DistributedTrainingCoordinator(
    std::shared_ptr<ShardRouter> shard_router,
    std::shared_ptr<ShardTopology> shard_topology,
    const DistributedTrainingConfig& config
) : shard_router_(shard_router),
    shard_topology_(shard_topology),
    config_(config),
    is_initialized_(false),
    is_running_(false),
    current_step_(0) {
    
    spdlog::info("DistributedTrainingCoordinator created");
    spdlog::info("  Coordinator shard: {}", config_.coordinator_shard);
    spdlog::info("  Participant shards: {}", config_.participant_shards.size());
    spdlog::info("  Sync strategy: {}", static_cast<int>(config_.sync_strategy));
}

DistributedTrainingCoordinator::~DistributedTrainingCoordinator() {
    if (is_running_) {
        stop();
    }
}

bool DistributedTrainingCoordinator::initialize(
    const std::string& adapter_id, 
    const TrainingConfig& /*training_config*/
) {
    if (is_initialized_) {
        spdlog::warn("Coordinator already initialized");
        return true;
    }
    
    adapter_id_ = adapter_id;
    // Note: training_config is passed in for initialization but not stored as member
    // Individual training parameters are managed by local shard trainers
    
    spdlog::info("Initializing distributed training for adapter: {}", adapter_id);
    
    // Initialize aggregator based on strategy
    initializeAggregator();
    
    // Initialize Byzantine detector if enabled
    if (config_.enable_byzantine_detection) {
        initializeByzantineDetector();
        spdlog::info("Byzantine fault detection enabled (method={}, threshold={}, max_f={})",
                    static_cast<int>(config_.detection_method),
                    config_.detection_threshold,
                    config_.max_byzantine_shards);
    }
    
    // Validate shard participation
    if (!validateShardParticipation()) {
        spdlog::error("Shard participation validation failed");
        return false;
    }
    
    // Initialize shard states
    active_shards_ = config_.participant_shards;
    for (const auto& shard_id : active_shards_) {
        ShardTrainingState state;
        state.shard_id = shard_id;
        state.is_active = true;
        state.is_synchronized = true;
        state.last_heartbeat_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        shard_states_[shard_id] = state;
    }
    
    // Reset statistics
    stats_ = DistributedTrainingStats();
    start_time_ = std::chrono::steady_clock::now();
    current_step_ = 0;
    
    is_initialized_ = true;
    is_running_ = true;
    
    spdlog::info("Distributed training initialized successfully");
    return true;
}

DistributedTrainingCoordinator::StepResult DistributedTrainingCoordinator::executeStep() {
    StepResult result;
    result.success = false;
    result.step_number = current_step_;
    
    if (!is_initialized_ || !is_running_) {
        spdlog::error("Coordinator not initialized or stopped");
        return result;
    }
    
    auto step_start = std::chrono::high_resolution_clock::now();
    
    // 1. Collect gradients from all shards
    auto sync_start = std::chrono::high_resolution_clock::now();
    auto shard_gradients = collectGradients(current_step_);
    
    if (shard_gradients.empty()) {
        spdlog::error("Failed to collect gradients from any shard");
        return result;
    }
    
    // 2. Aggregate gradients
    result.aggregated_gradients = aggregateGradients(shard_gradients);
    
    // 2b. Aggregate loss values from shards
    result.aggregated_loss = aggregateLoss(shard_gradients);
    
    // Collect per-shard loss for monitoring
    for (const auto& [shard_id, state] : shard_states_) {
        if (state.current_loss > 0.0f) {
            result.per_shard_loss[shard_id] = state.current_loss;
        }
    }
    
    // Log aggregated loss if available
    if (result.aggregated_loss.has_value()) {
        spdlog::debug("Step {} aggregated loss: {:.6f}", current_step_, result.aggregated_loss.value());
    }
    
    // 3. Broadcast aggregated gradients back to shards
    if (!broadcastGradients(result.aggregated_gradients, current_step_)) {
        spdlog::warn("Failed to broadcast gradients to some shards");
    }
    
    auto sync_end = std::chrono::high_resolution_clock::now();
    result.sync_time_ms = std::chrono::duration<float, std::milli>(sync_end - sync_start).count();
    
    // 4. Update statistics
    stats_.total_gradient_syncs++;
    stats_.total_steps_completed++;
    
    if (stats_.total_gradient_syncs == 1) {
        stats_.avg_sync_time_ms = result.sync_time_ms;
        stats_.max_sync_time_ms = result.sync_time_ms;
    } else {
        stats_.avg_sync_time_ms = (stats_.avg_sync_time_ms * (stats_.total_gradient_syncs - 1) + 
                                   result.sync_time_ms) / stats_.total_gradient_syncs;
        stats_.max_sync_time_ms = std::max(stats_.max_sync_time_ms, result.sync_time_ms);
    }
    
    // 5. Check shard health
    result.shard_states = checkShardHealth();
    
    // 6. Save checkpoint if needed
    if (config_.enable_checkpointing && 
        current_step_ > 0 && 
        current_step_ % config_.checkpoint_frequency == 0) {
        saveCheckpoint(current_step_);
    }
    
    auto step_end = std::chrono::high_resolution_clock::now();
    result.total_time_ms = std::chrono::duration<float, std::milli>(step_end - step_start).count();
    
    // 7. Call progress callback
    if (progress_callback_) {
        progress_callback_(current_step_, result);
    }
    
    current_step_++;
    result.success = true;
    
    return result;
}

bool DistributedTrainingCoordinator::finalize() {
    if (!is_initialized_) {
        return true;
    }
    
    spdlog::info("Finalizing distributed training");
    spdlog::info("  Total steps completed: {}", current_step_);
    spdlog::info("  Total gradient syncs: {}", stats_.total_gradient_syncs);
    spdlog::info("  Average sync time: {:.2f} ms", stats_.avg_sync_time_ms);
    
    // Save final checkpoint
    if (config_.enable_checkpointing) {
        saveCheckpoint(current_step_);
    }
    
    is_running_ = false;
    is_initialized_ = false;
    
    return true;
}

void DistributedTrainingCoordinator::stop() {
    spdlog::info("Stopping distributed training coordinator");
    is_running_ = false;
}

std::map<std::string, std::vector<GradientTensor>> 
DistributedTrainingCoordinator::collectGradients(int step_number) {
    std::map<std::string, std::vector<GradientTensor>> collected;
    
    spdlog::debug("Collecting gradients from {} shards for step {}", 
                 active_shards_.size(), step_number);
    
    if (!shard_router_) {
        // Fallback to simulated mode when ShardRouter is not available
        spdlog::warn("No ShardRouter available, using simulated gradients");
        
        for (const auto& shard_id : active_shards_) {
            // Create dummy gradient for testing/standalone mode
            std::vector<GradientTensor> shard_grads;
            GradientTensor dummy;
            dummy.layer_name = "test_layer";
            dummy.source_shard = shard_id;
            dummy.step_number = step_number;
            dummy.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            dummy.shape = {64, 64};
            dummy.data.resize(64 * 64, 0.1f);  // Dummy data
            
            shard_grads.push_back(dummy);
            collected[shard_id] = shard_grads;
            
            // Simulate loss values for testing/standalone mode
            // Each shard has a simulated loss that decreases over steps
            float shard_loss = 1.0f / (1.0f + step_number * 0.1f);
            // Add some variance between shards (±10%)
            float variance = (std::hash<std::string>{}(shard_id) % 20 - 10) / 100.0f;
            shard_loss *= (1.0f + variance);
            
            // Update shard state with simulated loss
            if (shard_states_.count(shard_id) > 0) {
                shard_states_[shard_id].current_loss = shard_loss;
                shard_states_[shard_id].samples_processed = 32;  // Simulated batch size
                spdlog::debug("Shard {} simulated loss: {:.6f}", shard_id, shard_loss);
            }
        }
        
        return collected;
    }
    
    // Real RPC implementation using ShardRouter
    spdlog::info("Collecting gradients via ShardRouter RPC");
    
    for (const auto& shard_id : active_shards_) {
        try {
            // Create gradient collection request
            json request = {
                {"adapter_id", adapter_id_},
                {"step_number", step_number},
                {"timeout_ms", config_.timeout_seconds * 1000}
            };
            
            // Send RPC request to shard.
            // B3: request.dump() JSON-encodes all fields (including adapter_id_ and step_number),
            // which prevents injection of control characters into the RPC query string.
            // This is internal shard communication, not an LLM inference prompt.
            std::string rpc_query = "collect_gradients:" + request.dump();
            json response = shard_router_->executeQuery(rpc_query);
            
            // Parse response into gradient tensors
            std::vector<GradientTensor> shard_grads;
            if (response.contains("gradients") && response["gradients"].is_array()) {
                for (const auto& grad_json : response["gradients"]) {
                    try {
                        auto gradient = GradientTensor::fromJSON(grad_json);
                        shard_grads.push_back(gradient);
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to parse gradient from {}: {}", shard_id, e.what());
                    }
                }
            }
            
            if (!shard_grads.empty()) {
                collected[shard_id] = shard_grads;
                spdlog::debug("Collected {} gradients from {}", shard_grads.size(), shard_id);
            } else {
                spdlog::warn("No gradients received from {}", shard_id);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to collect gradients from {}: {}", shard_id, e.what());
            handleShardFailure(shard_id);
        }
    }
    
    return collected;
}

std::vector<GradientTensor> DistributedTrainingCoordinator::aggregateGradients(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    if (!aggregator_) {
        spdlog::error("Aggregator not initialized");
        return {};
    }
    
    // Make a mutable copy for Byzantine detection
    auto mutable_shard_gradients = shard_gradients;
    
    // Byzantine fault detection
    if (config_.enable_byzantine_detection && byzantine_detector_) {
        try {
            auto detection_result = byzantine_detector_->detectByzantineShards(shard_gradients);
            
            if (detection_result.requires_action) {
                spdlog::warn("Byzantine shards detected: {}", 
                            fmt::format("{}", fmt::join(detection_result.suspected_shards, ", ")));
                
                // Update statistics
                stats_.byzantine_detections++;
                for (const auto& shard_id : detection_result.suspected_shards) {
                    stats_.per_shard_detection_count[shard_id]++;
                }
                
                // Compute average anomaly score
                float total_anomaly = 0.0f;
                for (const auto& [_, score] : detection_result.anomaly_scores) {
                    total_anomaly += score;
                }
                stats_.avg_anomaly_score = detection_result.anomaly_scores.empty() ? 
                    0.0f : (total_anomaly / detection_result.anomaly_scores.size());
                
                // Take action based on configuration
                switch (config_.byzantine_action) {
                    case ByzantineAction::EXCLUDE:
                        // Remove suspected shards from aggregation
                        for (const auto& shard_id : detection_result.suspected_shards) {
                            mutable_shard_gradients.erase(shard_id);
                            stats_.byzantine_shards_excluded++;
                            handleShardFailure(shard_id);
                            spdlog::warn("Excluding Byzantine shard {} from aggregation", shard_id);
                        }
                        break;
                        
                    case ByzantineAction::CLIP:
                        // Clip gradients to safe range
                        clipAnomalousGradients(mutable_shard_gradients, detection_result);
                        spdlog::info("Clipped {} anomalous gradient shards", 
                                    detection_result.suspected_shards.size());
                        break;
                        
                    case ByzantineAction::WARN:
                        // Continue but log
                        spdlog::warn("Byzantine shards detected but continuing (action=WARN)");
                        break;
                        
                    case ByzantineAction::SHUTDOWN:
                        throw std::runtime_error(
                            fmt::format("Byzantine shards detected ({}), shutting down training",
                                      fmt::join(detection_result.suspected_shards, ", ")));
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("Byzantine detection failed: {}", e.what());
            // Continue with normal aggregation if detection fails
        }
    }
    
    // Convert map to vector for aggregator
    std::vector<std::vector<GradientTensor>> grad_list;
    grad_list.reserve(mutable_shard_gradients.size());
    
    for (const auto& [shard_id, gradients] : mutable_shard_gradients) {
        grad_list.push_back(gradients);
    }
    
    spdlog::debug("Aggregating gradients from {} shards using {}", 
                 grad_list.size(), aggregator_->getStrategy());
    
    return aggregator_->aggregate(grad_list);
}

std::optional<float> DistributedTrainingCoordinator::aggregateLoss(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    // Collect loss values and sample counts from gradients
    // NOTE: In the current implementation, loss is stored in the first gradient's metadata
    // In production, this would be stored in GradientExchangeMessage.local_loss
    std::vector<std::pair<float, int>> shard_losses_and_counts;
    
    for (const auto& [shard_id, gradients] : shard_gradients) {
        if (gradients.empty()) {
            spdlog::debug("Shard {} has no gradients, skipping loss aggregation", shard_id);
            continue;
        }
        
        // Check if shard state has loss information
        if (shard_states_.count(shard_id) > 0) {
            const auto& state = shard_states_[shard_id];
            if (state.current_loss > 0.0f && state.samples_processed > 0) {
                shard_losses_and_counts.push_back({state.current_loss, state.samples_processed});
                spdlog::debug("Shard {} contributed loss={:.6f} with {} samples", 
                            shard_id, state.current_loss, state.samples_processed);
            }
        }
    }
    
    if (shard_losses_and_counts.empty()) {
        spdlog::debug("No loss values available from any shard");
        return std::nullopt;
    }
    
    // Compute weighted average
    float aggregated = computeWeightedLoss(shard_losses_and_counts);
    spdlog::debug("Aggregated loss from {} shards: {:.6f}", 
                 shard_losses_and_counts.size(), aggregated);
    
    return aggregated;
}

float DistributedTrainingCoordinator::computeWeightedLoss(
    const std::vector<std::pair<float, int>>& shard_losses_and_counts
) {
    if (shard_losses_and_counts.empty()) {
        return 0.0f;
    }
    
    // Compute weighted average: sum(loss_i * samples_i) / sum(samples_i)
    float weighted_sum = 0.0f;
    int total_samples = 0;
    
    for (const auto& [loss, samples] : shard_losses_and_counts) {
        weighted_sum += loss * samples;
        total_samples += samples;
    }
    
    if (total_samples == 0) {
        // Fall back to simple average if no sample counts available
        spdlog::warn("No sample counts available, using simple average");
        float sum = 0.0f;
        for (const auto& [loss, _] : shard_losses_and_counts) {
            sum += loss;
        }
        return sum / shard_losses_and_counts.size();
    }
    
    return weighted_sum / total_samples;
}

bool DistributedTrainingCoordinator::broadcastGradients(
    const std::vector<GradientTensor>& gradients,
    int step_number
) {
    spdlog::debug("Broadcasting {} gradient tensors to {} shards",
                 gradients.size(), active_shards_.size());
    
    if (!shard_router_) {
        // Fallback to simulated mode
        spdlog::warn("No ShardRouter available, skipping broadcast (standalone mode)");
        
        // Update statistics for simulated mode
        for (const auto& grad : gradients) {
            stats_.total_bytes_sent += grad.uncompressed_size();
        }
        
        return true;
    }
    
    // Real RPC implementation using ShardRouter
    spdlog::info("Broadcasting gradients via ShardRouter RPC");
    
    // Optionally compress gradients if configured
    auto gradients_to_send = config_.compression != GradientCompressionType::NONE ?
        compressGradients(gradients) : gradients;
    
    // Broadcast to all shards in parallel using futures
    std::vector<std::future<bool>> futures;
    
    for (const auto& shard_id : active_shards_) {
        futures.push_back(std::async(std::launch::async, [&, shard_id]() {
            try {
                // Create broadcast request with gradients
                json request = {
                    {"step_number", step_number},
                    {"gradients", json::array()}
                };
                
                // Serialize gradients to JSON
                for (const auto& grad : gradients_to_send) {
                    request["gradients"].push_back(grad.toJSON());
                }
                
                // Send RPC request to apply gradients
                std::string rpc_query = "apply_gradients:" + request.dump();
                json response = shard_router_->executeQuery(rpc_query);
                
                // Check if successful
                if (response.contains("success") && response["success"].get<bool>()) {
                    spdlog::debug("Successfully broadcast gradients to {}", shard_id);
                    return true;
                } else {
                    spdlog::warn("Gradient broadcast to {} returned failure", shard_id);
                    return false;
                }
                
            } catch (const std::exception& e) {
                spdlog::error("Failed to broadcast gradients to {}: {}", shard_id, e.what());
                return false;
            }
        }));
    }
    
    // Wait for all broadcasts to complete
    bool all_success = true;
    for (auto& future : futures) {
        try {
            bool success = future.get();
            all_success &= success;
        } catch (const std::exception& e) {
            spdlog::error("Broadcast future threw exception: {}", e.what());
            all_success = false;
        }
    }
    
    // Update statistics
    for (const auto& grad : gradients) {
        stats_.total_bytes_sent += grad.compressed_size();
    }
    
    if (all_success) {
        spdlog::debug("All gradients broadcast successfully");
    } else {
        spdlog::warn("Some gradient broadcasts failed");
    }
    
    return all_success;
}

std::map<std::string, ShardTrainingState> 
DistributedTrainingCoordinator::checkShardHealth() {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (!shard_router_) {
        // Fallback to simulated health checks
        spdlog::debug("No ShardRouter available, using simulated health checks");
        
        for (auto& [shard_id, state] : shard_states_) {
            state.last_heartbeat_ms = now_ms;
            state.is_active = true;
        }
        
        return shard_states_;
    }
    
    // Real health checks via RPC
    spdlog::debug("Checking shard health via ShardRouter RPC");
    
    for (auto& [shard_id, state] : shard_states_) {
        try {
            // Create health check request
            json request = {
                {"type", "health_check"},
                {"timestamp", now_ms}
            };
            
            // Send RPC request
            std::string rpc_query = "health_check:" + request.dump();
            json response = shard_router_->executeQuery(rpc_query);
            
            // Parse health response
            if (response.contains("is_active")) {
                state.is_active = response["is_active"].get<bool>();
            }
            if (response.contains("gpu_utilization")) {
                state.gpu_utilization = response["gpu_utilization"].get<float>();
            }
            if (response.contains("memory_usage_gb")) {
                state.memory_usage_gb = response["memory_usage_gb"].get<float>();
            }
            if (response.contains("last_heartbeat_ms")) {
                state.last_heartbeat_ms = response["last_heartbeat_ms"].get<int64_t>();
            } else {
                state.last_heartbeat_ms = now_ms;
            }
            
            // Reset failure count on successful health check
            if (state.is_active) {
                state.consecutive_failures = 0;
            }
            
        } catch (const std::exception& e) {
            spdlog::warn("Health check failed for {}: {}", shard_id, e.what());
            state.consecutive_failures++;
            
            // Mark as inactive if too many consecutive failures
            if (state.consecutive_failures > 3) {
                state.is_active = false;
            }
        }
        
        // Check if shard is too far behind (timeout check)
        int64_t time_since_heartbeat = now_ms - state.last_heartbeat_ms;
        if (time_since_heartbeat > config_.timeout_seconds * 1000) {
            state.is_active = false;
            state.consecutive_failures++;
            spdlog::warn("Shard {} appears to be down (no heartbeat for {}ms)",
                       shard_id, time_since_heartbeat);
        }
    }
    
    return shard_states_;
}

bool DistributedTrainingCoordinator::handleShardFailure(const std::string& failed_shard) {
    spdlog::warn("Handling failure of shard: {}", failed_shard);
    
    // Remove from active shards
    active_shards_.erase(
        std::remove(active_shards_.begin(), active_shards_.end(), failed_shard),
        active_shards_.end()
    );
    
    // Update state
    if (shard_states_.count(failed_shard)) {
        shard_states_[failed_shard].is_active = false;
    }
    
    stats_.shard_failures++;
    
    // Check if we still have enough shards to continue
    if (active_shards_.empty()) {
        spdlog::error("No active shards remaining, cannot continue training");
        stop();
        return false;
    }
    
    spdlog::info("Continuing with {} active shards", active_shards_.size());
    return true;
}

bool DistributedTrainingCoordinator::saveCheckpoint(int step_number) {
    if (config_.checkpoint_path.empty()) {
        spdlog::warn("Checkpoint path not configured");
        return false;
    }
    
    try {
        json checkpoint;
        checkpoint["adapter_id"] = adapter_id_;
        checkpoint["step_number"] = step_number;
        checkpoint["config"] = config_.toJSON();
        checkpoint["stats"] = stats_.toJSON();
        
        checkpoint["shard_states"] = json::object();
        for (const auto& [shard_id, state] : shard_states_) {
            checkpoint["shard_states"][shard_id] = state.toJSON();
        }
        
        std::string checkpoint_file = config_.checkpoint_path + 
            "/checkpoint_step_" + std::to_string(step_number) + ".json";
        // Wave-B L3: write to a temp file first, then rename atomically so a
        // partial write (e.g. OOM during dump()) never leaves a corrupt checkpoint.
        std::string tmp_file = checkpoint_file + ".tmp";
        {
            std::ofstream file(tmp_file);
            if (!file.is_open()) {
                spdlog::error("Failed to open checkpoint tmp file: {}", tmp_file);
                return false;
            }
            file << checkpoint.dump(2);
            // flush/close before rename; ofstream RAII closes on scope exit.
        }
        std::filesystem::rename(tmp_file, checkpoint_file);
        
        spdlog::info("Checkpoint saved: {}", checkpoint_file);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to save checkpoint: {}", e.what());
        return false;
    }
}

bool DistributedTrainingCoordinator::resumeFromCheckpoint(const std::string& checkpoint_path) {
    try {
        std::ifstream file(checkpoint_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open checkpoint file: {}", checkpoint_path);
            return false;
        }
        
        json checkpoint;
        file >> checkpoint;
        file.close();
        
        // Restore state
        if (checkpoint.contains("adapter_id")) adapter_id_ = checkpoint["adapter_id"];
        if (checkpoint.contains("step_number")) current_step_ = checkpoint["step_number"];
        if (checkpoint.contains("stats")) {
            // Restore statistics (partial)
            auto stats_json = checkpoint["stats"];
            if (stats_json.contains("total_steps_completed"))
                stats_.total_steps_completed = stats_json["total_steps_completed"];
            if (stats_json.contains("total_gradient_syncs"))
                stats_.total_gradient_syncs = stats_json["total_gradient_syncs"];
        }
        
        if (checkpoint.contains("shard_states")) {
            shard_states_.clear();
            for (auto& [shard_id, state_json] : checkpoint["shard_states"].items()) {
                shard_states_[shard_id] = ShardTrainingState::fromJSON(state_json);
            }
        }
        
        spdlog::info("Resumed from checkpoint: {}", checkpoint_path);
        spdlog::info("  Adapter: {}", adapter_id_);
        spdlog::info("  Step: {}", current_step_);
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to resume from checkpoint: {}", e.what());
        return false;
    }
}

DistributedTrainingStats DistributedTrainingCoordinator::getStatistics() const {
    return stats_;
}

std::map<std::string, ShardTrainingState> 
DistributedTrainingCoordinator::getShardStates() const {
    return shard_states_;
}

float DistributedTrainingCoordinator::estimateRemainingTime() const {
    if (stats_.total_steps_completed == 0 || stats_.avg_sync_time_ms <= 0) {
        return 0.0f;
    }
    
    // Estimate based on average step time
    // This is simplified - real implementation would use more sophisticated estimation
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_minutes = std::chrono::duration<float, std::ratio<60>>(elapsed).count();
    
    float avg_time_per_step = elapsed_minutes / stats_.total_steps_completed;

    // Use total_steps from config when available to compute a real ETA.
    if (config_.total_steps > 0) {
        int remaining = config_.total_steps - static_cast<int>(stats_.total_steps_completed);
        if (remaining <= 0) {
            return 0.0f;  // Training already at or past the configured step count
        }
        return avg_time_per_step * static_cast<float>(remaining);
    }

    // total_steps not configured — cannot compute remaining time
    return 0.0f;
}

void DistributedTrainingCoordinator::setProgressCallback(ProgressCallback callback) {
    progress_callback_ = callback;
}

void DistributedTrainingCoordinator::updateConfig(const DistributedTrainingConfig& config) {
    config_ = config;
    
    // Reinitialize aggregator if strategy changed
    initializeAggregator();
    
    spdlog::info("Configuration updated");
}

void DistributedTrainingCoordinator::initializeAggregator() {
    switch (config_.sync_strategy) {
        case SyncStrategy::ALL_REDUCE:
            aggregator_ = std::make_unique<AllReduceAggregator>();
            spdlog::info("Using AllReduce aggregation strategy");
            break;
            
        case SyncStrategy::PARAMETER_SERVER: {
            // Create equal weights for all shards
            std::map<std::string, float> weights;
            float weight = 1.0f / config_.participant_shards.size();
            for (const auto& shard : config_.participant_shards) {
                weights[shard] = weight;
            }
            aggregator_ = std::make_unique<ParameterServerAggregator>(weights);
            spdlog::info("Using ParameterServer aggregation strategy");
            break;
        }
        
        case SyncStrategy::RING_ALL_REDUCE: {
            auto ring_agg = std::make_unique<RingAllReduceAggregator>();
            ring_agg->setRingTopology(config_.participant_shards);
            aggregator_ = std::move(ring_agg);
            spdlog::info("Using RingAllReduce aggregation strategy");
            break;
        }
        
        case SyncStrategy::HIERARCHICAL:
        case SyncStrategy::ASYNC_SGD:
            // Fall back to AllReduce for now
            aggregator_ = std::make_unique<AllReduceAggregator>();
            spdlog::warn("Strategy not fully implemented, using AllReduce");
            break;
    }
}

void DistributedTrainingCoordinator::initializeByzantineDetector() {
    byzantine_detector_ = ByzantineDetectorFactory::create(
        config_.detection_method,
        config_.detection_threshold,
        config_.max_byzantine_shards
    );
    
    if (byzantine_detector_) {
        spdlog::info("Initialized Byzantine detector: {}", byzantine_detector_->getName());
    } else {
        spdlog::warn("Failed to initialize Byzantine detector");
    }
}

void DistributedTrainingCoordinator::clipAnomalousGradients(
    std::map<std::string, std::vector<GradientTensor>>& shard_gradients,
    const DetectionResult& detection_result
) {
    // Compute statistics for clipping
    auto stats = byzantine_detector_->computeStatistics(shard_gradients);
    
    if (stats.global_mad < 1e-10f) {
        spdlog::warn("Cannot clip gradients: MAD too small");
        return;
    }
    
    // Clip each suspected shard's gradients
    for (const auto& shard_id : detection_result.suspected_shards) {
        if (shard_gradients.find(shard_id) == shard_gradients.end()) {
            continue;
        }
        
        auto& gradients = shard_gradients[shard_id];
        
        // Compute current norm
        float current_norm = 0.0f;
        for (const auto& tensor : gradients) {
            for (float val : tensor.data) {
                current_norm += val * val;
            }
        }
        current_norm = std::sqrt(current_norm);
        
        // Compute safe range: median ± k*MAD
        float max_norm = stats.global_median_norm + config_.detection_threshold * stats.global_mad;
        
        if (current_norm > max_norm && current_norm > 1e-10f) {
            // Scale down gradients to max_norm
            float scale = max_norm / current_norm;
            
            for (auto& tensor : gradients) {
                for (auto& val : tensor.data) {
                    val *= scale;
                }
            }
            
            spdlog::info("Clipped gradients from shard {} (norm {:.6f} -> {:.6f})",
                        shard_id, current_norm, max_norm);
        }
    }
}

bool DistributedTrainingCoordinator::validateShardParticipation() {
    if (config_.participant_shards.empty()) {
        spdlog::error("No participant shards configured");
        return false;
    }
    
    if (!shard_topology_) {
        // Fallback to simple validation without topology
        spdlog::warn("No ShardTopology available, skipping shard discovery");
        spdlog::info("Assuming all configured shards are available");
        return true;
    }
    
    // Real validation using ShardTopology
    spdlog::info("Validating shard participation using ShardTopology");
    
    // Query topology for available shards
    auto available_shards = shard_topology_->getHealthyShards();
    
    // Create set of available shard IDs for fast lookup
    std::set<std::string> available_shard_ids;
    for (const auto& shard_info : available_shards) {
        available_shard_ids.insert(shard_info.shard_id);
    }
    
    // Verify all participant shards are available
    for (const auto& shard_id : config_.participant_shards) {
        if (available_shard_ids.find(shard_id) == available_shard_ids.end()) {
            spdlog::error("Shard {} not available in topology", shard_id);
            return false;
        }
        
        // Send ping to verify reachability if ShardRouter is available
        if (shard_router_) {
            try {
                json ping_request = {
                    {"type", "ping"},
                    {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count()}
                };
                
                std::string rpc_query = "ping:" + ping_request.dump();
                json response = shard_router_->executeQuery(rpc_query);
                
                if (!response.contains("success") || !response["success"].get<bool>()) {
                    spdlog::error("Shard {} not reachable (ping failed)", shard_id);
                    return false;
                }
                
                spdlog::debug("Shard {} is reachable", shard_id);
                
            } catch (const std::exception& e) {
                spdlog::error("Failed to ping shard {}: {}", shard_id, e.what());
                return false;
            }
        }
    }
    
    spdlog::info("All {} participant shards validated successfully", 
                config_.participant_shards.size());
    return true;
}

void DistributedTrainingCoordinator::updateStatistics(const StepResult& /*result*/) {
    // Statistics are updated in executeStep
}

std::vector<GradientTensor> DistributedTrainingCoordinator::compressGradients(
    const std::vector<GradientTensor>& gradients
) {
    std::vector<GradientTensor> compressed;
    compressed.reserve(gradients.size());
    
    for (auto grad : gradients) {
        if (config_.compression != GradientCompressionType::NONE) {
            grad.compress(config_.compression);
        }
        compressed.push_back(std::move(grad));
    }
    
    return compressed;
}

std::vector<GradientTensor> DistributedTrainingCoordinator::decompressGradients(
    const std::vector<GradientTensor>& gradients
) {
    std::vector<GradientTensor> decompressed;
    decompressed.reserve(gradients.size());
    
    for (auto grad : gradients) {
        grad.decompress();
        decompressed.push_back(std::move(grad));
    }
    
    return decompressed;
}

// ============================================================================
// Factory Implementation
// ============================================================================

std::unique_ptr<DistributedTrainingCoordinator> 
DistributedTrainingCoordinatorFactory::create(
    std::shared_ptr<ShardRouter> shard_router,
    std::shared_ptr<ShardTopology> shard_topology,
    const DistributedTrainingConfig& config
) {
    return std::make_unique<DistributedTrainingCoordinator>(
        shard_router, shard_topology, config
    );
}

std::unique_ptr<DistributedTrainingCoordinator> 
DistributedTrainingCoordinatorFactory::createWithAutoDiscovery(
    std::shared_ptr<ShardRouter> shard_router,
    SyncStrategy strategy
) {
    // Auto-discover shards from topology
    DistributedTrainingConfig config;
    config.sync_strategy = strategy;
    
    // In real implementation, query shard topology for available shards
    // For now, use empty configuration
    spdlog::info("Creating coordinator with auto-discovery");
    
    // Create minimal topology if not provided
    std::shared_ptr<ShardTopology> topology;
    
    return std::make_unique<DistributedTrainingCoordinator>(
        shard_router, topology, config
    );
}

} // namespace llm
} // namespace themis

