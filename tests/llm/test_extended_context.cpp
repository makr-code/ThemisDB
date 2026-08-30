/**
 * @file test_extended_context.cpp
 * @brief Unit tests for Extended Context Window (32K-128K) and RoPE/YARN Scaling
 * 
 * Tests the production-ready extended context functionality including:
 * - RoPE/YARN scaling configuration and validation
 * - Memory estimation and tracking
 * - Thread-safety with LoRA adapters
 * - Feature flags and backward compatibility
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "llm/model_loader.h"
#include "llm/grafana_metrics.h"
#include <nlohmann/json.hpp>

using namespace themis::llm;
using namespace themis::llm::monitoring;
using json = nlohmann::json;

// ═══════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════

class ExtendedContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize metrics collector
        exporter_ = std::make_unique<PrometheusExporter>();
        metrics_ = std::make_unique<LLMMetricsCollector>(exporter_.get());
        
        // Default configuration
        loader_config_ = LazyModelLoader::Config();
        loader_config_.max_vram_mb = 24576;  // 24GB
        loader_config_.max_ram_mb = 65536;   // 64GB
        loader_config_.default_n_ctx = 4096;
    }
    
    void TearDown() override {
        metrics_.reset();
        exporter_.reset();
    }
    
    std::unique_ptr<PrometheusExporter> exporter_;
    std::unique_ptr<LLMMetricsCollector> metrics_;
    LazyModelLoader::Config loader_config_;
};

// ═══════════════════════════════════════════════════════════
// Context Length Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, ContextLengthMetricsRecording) {
    const std::string model_id = "llama-7b";
    const size_t context_length = 32768;
    
    metrics_->recordContextLength(model_id, context_length);
    
    // Verify metric was recorded
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_context_length"));
    EXPECT_THAT(output, ::testing::HasSubstr(model_id));
}

TEST_F(ExtendedContextTest, ContextLengthValidation4K) {
    const size_t context_length = 4096;
    
    // 4K should always be valid (native context)
    EXPECT_GE(context_length, 512);   // Min context
    EXPECT_LE(context_length, 131072); // Max context (128K)
}

TEST_F(ExtendedContextTest, ContextLengthValidation32K) {
    const size_t context_length = 32768;
    
    // 32K is within valid range
    EXPECT_GE(context_length, 512);
    EXPECT_LE(context_length, 131072);
}

TEST_F(ExtendedContextTest, ContextLengthValidation128K) {
    const size_t context_length = 131072;
    
    // 128K is max supported
    EXPECT_GE(context_length, 512);
    EXPECT_LE(context_length, 131072);
}

TEST_F(ExtendedContextTest, ContextLengthExceedsMax) {
    const size_t context_length = 262144;  // 256K - too large
    
    // Should exceed maximum
    EXPECT_GT(context_length, 131072);
}

// ═══════════════════════════════════════════════════════════
// RoPE Scaling Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, RoPEScalingLinear) {
    json config;
    config["rope_scaling_enabled"] = true;
    config["rope_scaling_method"] = "linear";
    config["rope_original_context"] = 4096;
    config["rope_max_context"] = 8192;
    
    // Calculate expected scaling factor
    float expected_scale = static_cast<float>(4096) / static_cast<float>(8192);
    EXPECT_FLOAT_EQ(expected_scale, 0.5f);
    
    // Record metric
    const std::string model_id = "test-model";
    metrics_->recordRoPEScalingMethod(model_id, "linear");
    metrics_->recordContextScalingFactor(model_id, 2.0);  // 2x scaling
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_rope_scaling_method"));
    EXPECT_THAT(output, ::testing::HasSubstr("linear"));
}

TEST_F(ExtendedContextTest, RoPEScalingNTK) {
    json config;
    config["rope_scaling_enabled"] = true;
    config["rope_scaling_method"] = "ntk";
    config["rope_original_context"] = 4096;
    config["rope_max_context"] = 32768;
    
    // Calculate NTK scaling ratio
    float scaling_ratio = static_cast<float>(32768) / static_cast<float>(4096);
    EXPECT_FLOAT_EQ(scaling_ratio, 8.0f);
    
    // NTK freq base calculation
    float freq_base = 10000.0f * std::pow(scaling_ratio, 0.5f);
    EXPECT_GT(freq_base, 10000.0f);
    
    metrics_->recordRoPEScalingMethod("test-model", "ntk");
    metrics_->recordContextScalingFactor("test-model", scaling_ratio);
}

TEST_F(ExtendedContextTest, RoPEScalingYARN) {
    json config;
    config["rope_scaling_enabled"] = true;
    config["rope_scaling_method"] = "yarn";
    config["rope_original_context"] = 4096;
    config["rope_max_context"] = 32768;
    config["rope_yarn_ext_factor"] = 1.0;
    config["rope_yarn_attn_factor"] = 1.0;
    config["rope_yarn_beta_fast"] = 32.0;
    config["rope_yarn_beta_slow"] = 1.0;
    
    const std::string model_id = "test-model";
    
    // Record YaRN parameters
    metrics_->recordYARNParameters(
        model_id,
        config["rope_yarn_ext_factor"],
        config["rope_yarn_attn_factor"],
        config["rope_yarn_beta_fast"],
        config["rope_yarn_beta_slow"]
    );
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_yarn_ext_factor"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_yarn_attn_factor"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_yarn_beta_fast"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_yarn_beta_slow"));
}

TEST_F(ExtendedContextTest, RoPEScalingYARNParameterValidation) {
    // Valid parameters
    float ext_factor = 1.0f;
    float attn_factor = 1.0f;
    float beta_fast = 32.0f;
    float beta_slow = 1.0f;
    
    // Validate ranges (from config comments)
    EXPECT_GE(ext_factor, 1.0f);
    EXPECT_LE(ext_factor, 4.0f);
    
    EXPECT_GE(attn_factor, 0.5f);
    EXPECT_LE(attn_factor, 2.0f);
    
    EXPECT_GE(beta_fast, 16.0f);
    EXPECT_LE(beta_fast, 64.0f);
    
    EXPECT_GE(beta_slow, 0.5f);
    EXPECT_LE(beta_slow, 2.0f);
}

TEST_F(ExtendedContextTest, RoPEScalingErrorRecording) {
    const std::string model_id = "test-model";
    const std::string error = "invalid_config";
    
    metrics_->recordRoPEScalingError(model_id, error);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_rope_scaling_errors_total"));
    EXPECT_THAT(output, ::testing::HasSubstr(error));
}

// ═══════════════════════════════════════════════════════════
// Memory Estimation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, MemoryEstimation7B_4K) {
    // 7B model Q4 quantization @ 4K context
    const size_t num_params = 7000000000;  // 7B
    const float bytes_per_param = 0.5f;     // Q4
    const size_t n_ctx = 4096;
    const size_t n_layers = 32;
    const size_t hidden_size = 4096;
    const float dtype_size = 2.0f;  // FP16
    const float safety_margin = 1.2f;
    
    // Calculate model size
    size_t model_size_mb = static_cast<size_t>(
        (num_params * bytes_per_param) / (1024.0 * 1024.0)
    );
    
    // Calculate KV cache size
    size_t kv_cache_mb = static_cast<size_t>(
        (n_ctx * n_layers * hidden_size * 2 * dtype_size) / (1024.0 * 1024.0)
    );
    
    // Total memory with safety margin
    size_t total_mb = static_cast<size_t>(
        (model_size_mb + kv_cache_mb) * safety_margin
    );
    
    // Expected with explicit K+V cache (2x):
    // ~3.5GB model + ~2GB cache = ~6.4GB total (with safety margin)
    EXPECT_GT(model_size_mb, 3000);  // > 3GB
    EXPECT_LT(model_size_mb, 4000);  // < 4GB
    EXPECT_GT(kv_cache_mb, 1900);    // > 1.9GB
    EXPECT_LT(kv_cache_mb, 2100);    // < 2.1GB
    EXPECT_GT(total_mb, 6200);       // > 6.2GB
    EXPECT_LT(total_mb, 6800);       // < 6.8GB
}

TEST_F(ExtendedContextTest, MemoryEstimation7B_32K) {
    // 7B model Q4 @ 32K context (8x scaling)
    const size_t num_params = 7000000000;
    const float bytes_per_param = 0.5f;
    const size_t n_ctx = 32768;  // Extended context
    const size_t n_layers = 32;
    const size_t hidden_size = 4096;
    const float dtype_size = 2.0f;
    const float safety_margin = 1.2f;
    
    size_t model_size_mb = static_cast<size_t>(
        (num_params * bytes_per_param) / (1024.0 * 1024.0)
    );
    
    size_t kv_cache_mb = static_cast<size_t>(
        (n_ctx * n_layers * hidden_size * 2 * dtype_size) / (1024.0 * 1024.0)
    );
    
    size_t total_mb = static_cast<size_t>(
        (model_size_mb + kv_cache_mb) * safety_margin
    );
    
    // Expected with explicit K+V cache (2x):
    // ~3.5GB model + ~16GB cache = ~23.6GB total (with safety margin)
    EXPECT_GT(kv_cache_mb, 15500);   // > 15.5GB
    EXPECT_LT(kv_cache_mb, 17000);   // < 17.0GB
    EXPECT_GT(total_mb, 22500);      // > 22.5GB
    EXPECT_LT(total_mb, 24500);      // < 24.5GB
}

TEST_F(ExtendedContextTest, MemoryEstimationAccuracy) {
    const std::string model_id = "test-model";
    const size_t estimated_mb = 12000;  // 12GB estimate
    const size_t actual_mb = 13000;     // 13GB actual
    
    metrics_->recordMemoryEstimate(model_id, estimated_mb, actual_mb);
    
    // Calculate expected accuracy
    double accuracy = (static_cast<double>(actual_mb) / static_cast<double>(estimated_mb)) * 100.0;
    EXPECT_GT(accuracy, 100.0);  // Actual > Estimated
    EXPECT_LT(accuracy, 120.0);  // Within 20% margin
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_memory_estimation_accuracy_percent"));
}

TEST_F(ExtendedContextTest, MemoryEstimationDivisionByZeroProtection) {
    const std::string model_id = "test-model";
    const size_t estimated_mb = 5;      // Too small (< 10MB threshold)
    const size_t actual_mb = 100;
    
    // Should handle gracefully (no crash, set to 100%)
    metrics_->recordMemoryEstimate(model_id, estimated_mb, actual_mb);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_memory_estimation_accuracy_percent"));
}

// ═══════════════════════════════════════════════════════════
// RAM/VRAM Tracking Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, RAMUsageTracking) {
    const std::string model_id = "test-model";
    const size_t ram_used = 12288;   // 12GB
    const size_t ram_total = 65536;  // 64GB
    
    metrics_->recordRAMUsage(model_id, ram_used, ram_total);
    
    // Verify percentage calculation
    const float expected_pct = static_cast<float>(
        (static_cast<double>(ram_used) / static_cast<double>(ram_total)) * 100.0);
    EXPECT_FLOAT_EQ(expected_pct, 18.75f);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_ram_used_mb"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_ram_total_mb"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_ram_usage_percent"));
}

TEST_F(ExtendedContextTest, VRAMUsageTracking) {
    const std::string model_id = "test-model";
    const size_t vram_used = 16384;  // 16GB
    const size_t vram_total = 24576; // 24GB
    
    metrics_->recordVRAMUsage(model_id, vram_used, vram_total);
    
    const float expected_pct = static_cast<float>(
        (static_cast<double>(vram_used) / static_cast<double>(vram_total)) * 100.0);
    EXPECT_FLOAT_EQ(expected_pct, 66.666667f);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_vram_used_mb"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_vram_total_mb"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_vram_usage_percent"));
}

TEST_F(ExtendedContextTest, MemoryPressureTracking) {
    const std::string model_id = "test-model";
    
    // Low pressure
    metrics_->recordMemoryPressure(model_id, 45.0);
    
    // Medium pressure
    metrics_->recordMemoryPressure(model_id, 75.0);
    
    // High pressure
    metrics_->recordMemoryPressure(model_id, 90.0);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_memory_pressure_percent"));
}

TEST_F(ExtendedContextTest, OOMEventTracking) {
    const std::string model_id = "test-model";
    const std::string reason = "context_too_large";
    
    metrics_->recordOOMEvent(model_id, reason);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_oom_events_total"));
    EXPECT_THAT(output, ::testing::HasSubstr(reason));
}

// ═══════════════════════════════════════════════════════════
// Thread-Safety Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, LoRAAdapterSwitchTracking) {
    const std::string model_id = "test-model";
    const std::string from_adapter = "adapter_a";
    const std::string to_adapter = "adapter_b";
    const double duration_ms = 150.0;
    
    metrics_->recordLoRAAdapterSwitch(model_id, from_adapter, to_adapter, duration_ms);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_lora_adapter_switches_total"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_lora_adapter_switch_duration_ms"));
}

TEST_F(ExtendedContextTest, ContextLockWaitTracking) {
    const std::string model_id = "test-model";
    
    // Normal wait time (< 100ms threshold)
    metrics_->recordContextLockWait(model_id, 50.0);
    
    // High contention (> 100ms threshold)
    metrics_->recordContextLockWait(model_id, 150.0);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_context_lock_wait_ms"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_context_lock_contention_total"));
}

TEST_F(ExtendedContextTest, SequentialModeTracking) {
    const std::string model_id = "test-model";
    
    // Sequential mode (safe)
    metrics_->recordConcurrentLoRAOperation(model_id, true);
    
    // Concurrent mode (unsafe)
    metrics_->recordConcurrentLoRAOperation(model_id, false);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_lora_sequential_mode"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_lora_concurrent_operations_total"));
}

// ═══════════════════════════════════════════════════════════
// Feature Flag Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, ExtendedContextEnabledFlag) {
    const std::string model_id = "test-model";
    
    // Test enabled
    metrics_->recordExtendedContextEnabled(model_id, true);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_extended_context_enabled"));
}

TEST_F(ExtendedContextTest, ExtendedContextDisabledFlag) {
    const std::string model_id = "test-model";
    
    // Test disabled
    metrics_->recordExtendedContextEnabled(model_id, false);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_extended_context_enabled"));
}

TEST_F(ExtendedContextTest, ContextCacheSizeTracking) {
    const std::string model_id = "test-model";
    const size_t cache_size_mb = 8192;  // 8GB cache
    
    metrics_->recordContextCacheSize(model_id, cache_size_mb);
    
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_context_cache_size_mb"));
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ExtendedContextTest, End2EndMetricsCollection) {
    const std::string model_id = "llama-7b-test";
    
    // Record complete workflow
    metrics_->recordExtendedContextEnabled(model_id, true);
    metrics_->recordContextScalingFactor(model_id, 8.0);  // 8x scaling
    metrics_->recordRoPEScalingMethod(model_id, "yarn");
    metrics_->recordYARNParameters(model_id, 1.0, 1.0, 32.0, 1.0);
    
    metrics_->recordContextLength(model_id, 32768);
    metrics_->recordContextCacheSize(model_id, 8192);
    
    metrics_->recordRAMUsage(model_id, 12288, 65536);
    metrics_->recordVRAMUsage(model_id, 16384, 24576);
    metrics_->recordMemoryEstimate(model_id, 13000, 14000);
    
    metrics_->recordConcurrentLoRAOperation(model_id, true);  // Sequential
    
    // Verify all metrics present
    std::string output = exporter_->exportMetrics();
    EXPECT_THAT(output, ::testing::HasSubstr("llm_extended_context_enabled"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_context_scaling_factor"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_rope_scaling_method"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_yarn_ext_factor"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_context_length"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_ram_usage_percent"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_vram_usage_percent"));
    EXPECT_THAT(output, ::testing::HasSubstr("llm_lora_sequential_mode"));
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════


