# Future Works Implementation Guide

## Overview

This document describes the production-ready features implemented as part of Phase 3 "Future Works":
1. **ONNX Runtime Integration** - Real NLI models for faithfulness verification
2. **HTTP Metrics Client** - Continuous learning endpoint integration

---

## 1. ONNX Runtime Integration

### Purpose
Enable production NLI (Natural Language Inference) models for accurate claim verification in quality control.

### Components

#### ONNXModelLoader
Manages ONNX model loading, caching, and validation.

**Key Features:**
- Local model loading with path validation
- Automatic model downloading from URLs
- SHA256 checksum verification for integrity
- Model caching for performance
- Predefined NLI model registry
- Thread-safe cache operations

**Usage:**
```cpp
#include "rag/onnx_model_loader.h"

// Basic usage
ONNXModelLoader loader;
auto model = loader.loadModel("/path/to/model.onnx");

// With auto-download
ONNXModelLoaderConfig config;
config.cache_dir = "./models";
config.auto_download = true;
config.verify_checksum = true;

ONNXModelLoader loader(config);
auto model = loader.loadOrDownloadModel(
    "deberta-v3-large-mnli",
    "https://huggingface.co/microsoft/deberta-v3-large-mnli/resolve/main/model.onnx",
    "expected_checksum_here"
);
```

#### NLIModelFactory
Pre-configured models for common use cases.

**Supported Models:**
| Model | Size | Description |
|-------|------|-------------|
| DeBERTa-v3-large-mnli | ~1.42 GB | Microsoft DeBERTa v3 Large trained on MNLI |
| RoBERTa-large-mnli | ~1.42 GB | RoBERTa Large trained on MNLI |
| BART-large-mnli | ~1.63 GB | BART Large trained on MNLI |

**Usage:**
```cpp
#include "rag/onnx_model_loader.h"

// Get predefined model info
auto model_info = NLIModelFactory::getDebertaV3LargeMNLI();
std::cout << "Model: " << model_info.model_name << "\n";
std::cout << "Size: " << model_info.model_size_bytes / 1024 / 1024 << " MB\n";
std::cout << "URL: " << model_info.model_url << "\n";

// List all supported models
auto all_models = NLIModelFactory::getAllSupportedModels();
for (const auto& model : all_models) {
    std::cout << model.model_name << "\n";
}
```

### Integration with NLI Faithfulness Verifier

```cpp
#include "rag/onnx_model_loader.h"
#include "rag/nli_faithfulness_verifier.h"

// Load model
ONNXModelLoader loader;
auto model = loader.loadOrDownloadModel("deberta-v3-large-mnli", url);

if (model) {
    // Configure NLI verifier with real model
    NLIFaithfulnessVerifier::Config config;
    config.model_path = model->model_path;
    config.tokenizer_path = model->tokenizer_path;
    config.max_sequence_length = 512;
    config.use_gpu = true;  // Enable GPU if available
    
    NLIFaithfulnessVerifier verifier(config);
    
    // Now using real ONNX model for inference!
    auto result = verifier.verifyClaim("claim", "document");
    std::cout << "Entailment probability: " << result.entailment_prob << "\n";
}
```

### Configuration Options

```cpp
struct ONNXModelLoaderConfig {
    std::string cache_dir = "./models";  // Model cache directory
    bool verify_checksum = true;         // Verify SHA256 checksums
    bool auto_download = false;          // Auto-download missing models
    int download_timeout_sec = 300;      // Download timeout (5 minutes)
    bool create_cache_dir = true;        // Create cache dir if missing
};
```

### Model Management

```cpp
ONNXModelLoader loader;

// List cached models
auto cached = loader.listCachedModels();
for (const auto& model_name : cached) {
    std::cout << "Cached: " << model_name << "\n";
}

// Check if model is cached
if (loader.isModelCached("deberta-v3-large-mnli")) {
    std::cout << "Model is cached\n";
}

// Get cached model path
std::string path = loader.getCachedModelPath("deberta-v3-large-mnli");

// Clear cache
loader.clearCache("specific_model");  // Clear one model
loader.clearCache();  // Clear all models
```

---

## 2. HTTP Metrics Client

### Purpose
Upload quality control metrics to continuous learning endpoints for system optimization.

### Components

#### HTTPMetricsClient
Handles HTTP/HTTPS communication with metrics endpoints.

**Key Features:**
- Single and batch metric uploads
- Automatic retry with exponential backoff
- SSL/TLS support with certificate verification
- Bearer token authentication
- Gzip compression
- Connection pooling
- Request/response statistics tracking

**Usage:**
```cpp
#include "rag/http_metrics_client.h"

// Create client
HTTPMetricsClientConfig config;
config.endpoint_url = "https://api.example.com";
config.auth_token = "your_bearer_token";
config.timeout_ms = 5000;
config.max_retries = 3;

HTTPMetricsClient client(config);

// Send single metric
QualityMetricPayload metric;
metric.query = "User query";
metric.faithfulness_score = 0.95;
metric.relevance_score = 0.98;
metric.completeness_score = 0.90;
metric.coherence_score = 0.92;
metric.overall_score = 0.94;
metric.decision = "ACCEPT";
metric.latency_ms = 150;
metric.mode = "BALANCED";
metric.timestamp = getCurrentTimestamp();

auto response = client.sendMetric(metric);
if (response.success) {
    std::cout << "Metric uploaded successfully\n";
}
```

#### HTTPMetricsClientFactory
Pre-configured clients for different environments.

**Factory Methods:**
```cpp
// Local development (no SSL, no auth)
auto local_client = HTTPMetricsClientFactory::createLocalClient(
    "http://localhost:8080"
);

// Production (SSL, auth, compression)
auto prod_client = HTTPMetricsClientFactory::createProductionClient(
    "https://api.continuous-learning.com",
    "auth_token_here"
);

// Custom configuration
HTTPMetricsClientConfig custom_config;
// ... configure as needed
auto custom_client = HTTPMetricsClientFactory::createCustomClient(custom_config);
```

### Batch Uploads

```cpp
// Create batch of metrics
std::vector<QualityMetricPayload> metrics_batch;

for (const auto& qc_result : qc_results) {
    QualityMetricPayload metric;
    metric.query = qc_result.query;
    metric.faithfulness_score = qc_result.dimension_scores["faithfulness"];
    // ... populate other fields
    metrics_batch.push_back(metric);
}

// Upload batch (automatically splits if > max_batch_size)
auto response = client.sendMetricsBatch(metrics_batch);
```

### Configuration Options

```cpp
struct HTTPMetricsClientConfig {
    std::string endpoint_url;         // Base URL for metrics endpoint
    int timeout_ms = 5000;            // Request timeout (5 seconds)
    int max_retries = 3;              // Maximum retry attempts
    int retry_backoff_ms = 1000;      // Initial retry backoff (exponential)
    bool verify_ssl = true;           // Verify SSL certificates
    std::string auth_token;           // Optional Bearer token
    std::string user_agent = "ThemisDB-QC/1.0";  // User agent
    bool enable_compression = true;   // Enable gzip compression
    int max_batch_size = 100;         // Maximum metrics per batch
    int connection_pool_size = 4;     // HTTP connection pool size
};
```

### Statistics Tracking

```cpp
// Get statistics
auto stats = client.getStatistics();

std::cout << "Requests sent: " << stats.requests_sent << "\n";
std::cout << "Succeeded: " << stats.requests_succeeded << "\n";
std::cout << "Failed: " << stats.requests_failed << "\n";
std::cout << "Metrics sent: " << stats.metrics_sent << "\n";
std::cout << "Retries: " << stats.retries_attempted << "\n";
std::cout << "Avg latency: " << stats.avg_latency.count() << "ms\n";

// Reset statistics
client.resetStatistics();
```

### Callbacks for Monitoring

```cpp
client.setRequestCallback([](const std::string& method, 
                            const std::string& url,
                            int status_code,
                            int64_t latency_ms) {
    std::cout << method << " " << url 
              << " -> " << status_code 
              << " (" << latency_ms << "ms)\n";
});
```

---

## 3. End-to-End Integration

### Complete Quality Control with Production Features

```cpp
#include "rag/quality_control_factory.h"
#include "rag/onnx_model_loader.h"
#include "rag/http_metrics_client.h"

// Step 1: Load ONNX model
ONNXModelLoader model_loader;
auto model = model_loader.loadOrDownloadModel("deberta-v3-large-mnli", url);

// Step 2: Create quality control pipeline with real NLI model
QualityControlFactory::SetupConfig qc_config;
qc_config.nli_model_path = model->model_path;
qc_config.enable_continuous_learning = true;

auto pipeline = QualityControlFactory::createProduction(qc_config);

// Step 3: Setup HTTP metrics client
auto http_client = HTTPMetricsClientFactory::createProductionClient(
    "https://api.continuous-learning.com",
    "auth_token"
);

// Step 4: Run quality control
auto qc_result = pipeline->runQualityControl(query, documents, answer);

// Step 5: Upload metrics
if (qc_result.decision != QCDecision::RETRY) {
    QualityMetricPayload metric;
    metric.query = query;
    metric.faithfulness_score = qc_result.dimension_scores["faithfulness"];
    metric.relevance_score = qc_result.dimension_scores["relevance"];
    metric.completeness_score = qc_result.dimension_scores["completeness"];
    metric.coherence_score = qc_result.dimension_scores["coherence"];
    metric.overall_score = qc_result.overall_score;
    metric.decision = "ACCEPT";  // or REJECT, WARN
    metric.latency_ms = static_cast<int>(qc_result.latency_ms);
    metric.mode = "BALANCED";
    metric.timestamp = getCurrentTimestamp();
    
    auto response = http_client->sendMetric(metric);
    if (!response.success) {
        // Handle upload failure
        std::cerr << "Failed to upload metric: " << response.error_message << "\n";
    }
}
```

---

## 4. Performance Considerations

### ONNX Model Loading
- **Model caching**: Models are cached after first load (~50ms subsequent loads)
- **Checksum verification**: Adds ~100-500ms depending on model size
- **Download time**: Large models (1-2GB) take 5-10 minutes on typical connections

### HTTP Metrics Upload
- **Single metric**: ~50-200ms (depends on network latency)
- **Batch upload**: More efficient, ~100-300ms for 100 metrics
- **Retry overhead**: Exponential backoff (1s, 2s, 4s)
- **Compression**: Reduces payload size by ~70%

### Recommendations
1. **Batch metrics**: Upload every 60 seconds or every 100 metrics
2. **Local caching**: Keep models on local SSD for fast loading
3. **Connection pooling**: Reuse HTTP connections (already enabled)
4. **Async uploads**: Use separate thread for metric uploads (non-blocking)

---

## 5. Production Deployment

### Model Deployment

```bash
# Create model directory
mkdir -p /var/lib/themisdb/models

# Download DeBERTa model
wget -O /var/lib/themisdb/models/deberta-v3-large-mnli.onnx \
  https://huggingface.co/microsoft/deberta-v3-large-mnli/resolve/main/model.onnx

# Verify checksum
sha256sum /var/lib/themisdb/models/deberta-v3-large-mnli.onnx

# Set permissions
chown themisdb:themisdb /var/lib/themisdb/models/*.onnx
chmod 644 /var/lib/themisdb/models/*.onnx
```

### Configuration File

```yaml
# config/quality_control.yaml
quality_control:
  # ONNX Models
  models:
    cache_dir: /var/lib/themisdb/models
    auto_download: false  # Manual download in production
    verify_checksum: true
    nli_model: deberta-v3-large-mnli.onnx
  
  # HTTP Metrics
  metrics:
    enabled: true
    endpoint: https://api.continuous-learning.example.com
    auth_token: ${CL_AUTH_TOKEN}  # From environment
    timeout_ms: 5000
    max_retries: 3
    batch_size: 100
    upload_interval_sec: 60
  
  # Quality Control
  pipeline:
    mode: BALANCED  # FAST, BALANCED, or THOROUGH
    enable_nli: true
    enable_retry: true
    max_retry_attempts: 2
```

### Environment Variables

```bash
# Continuous learning authentication
export CL_AUTH_TOKEN="your_secure_token_here"

# Model paths
export ONNX_MODEL_DIR="/var/lib/themisdb/models"

# HTTP proxy (if needed)
export HTTPS_PROXY="http://proxy.example.com:8080"
```

---

## 6. Monitoring and Alerting

### Metrics to Monitor

```cpp
// HTTP Client Metrics
auto http_stats = http_client->getStatistics();
if (http_stats.requests_failed > 0.1 * http_stats.requests_sent) {
    // Alert: High failure rate
}

if (http_stats.avg_latency.count() > 1000) {
    // Alert: High latency
}

// Model Loading
auto cached_models = model_loader.listCachedModels();
if (cached_models.empty()) {
    // Alert: No models cached
}
```

### Health Checks

```cpp
// Check endpoint health
if (!http_client->isEndpointHealthy()) {
    // Alert: Endpoint unreachable
    // Fall back to local logging
}

// Check model availability
if (!model_loader.isModelCached("deberta-v3-large-mnli")) {
    // Alert: Required model not cached
}
```

---

## 7. Troubleshooting

### Common Issues

**Issue: Model download fails**
```
Solution:
- Check network connectivity
- Verify URL is accessible
- Check disk space in cache directory
- Increase download_timeout_sec
```

**Issue: HTTP upload fails**
```
Solution:
- Verify endpoint URL is correct
- Check authentication token
- Test endpoint with curl
- Check SSL certificate validity
- Review proxy settings
```

**Issue: High memory usage**
```
Solution:
- Clear model cache if not needed
- Reduce batch size for metrics
- Disable model caching if one-time use
- Use smaller NLI models (RoBERTa instead of DeBERTa)
```

**Issue: Slow performance**
```
Solution:
- Enable GPU for ONNX inference
- Use model caching
- Batch metric uploads
- Enable compression
- Use FAST mode for real-time applications
```

---

## 8. Future Enhancements

### Planned Features

1. **Multi-Model Ensemble**
   - Use multiple NLI models for higher accuracy
   - Weighted voting system
   - Confidence calibration

2. **Adaptive Threshold Tuning**
   - Automatically adjust quality thresholds
   - Based on feedback and performance
   - Per-domain customization

3. **Real-Time Dashboard**
   - Live quality metrics visualization
   - Alert management
   - Trend analysis

4. **Cross-Lingual Support**
   - Multilingual NLI models
   - Translation integration
   - Language-specific thresholds

---

## 9. API Reference

See header files for complete API documentation:
- `include/rag/onnx_model_loader.h` - ONNX model management
- `include/rag/http_metrics_client.h` - HTTP metrics upload

---

## 10. Examples

Complete examples available:
- `examples/future_works_integration_example.cpp` - All 6 examples
- See code for detailed usage patterns

---

**Status**: Production-ready  
**Last Updated**: 2026-04-06  
**Version**: 1.0

---

## 11. Temporal Tiering — LLM / LoRA Autonomous Tier Decisions

### Current Status

The `TemporalTierManager` implements a three-tier LSM-style version store
(hot MemTable → warm L0 blocks → cold SST on disk) with a threshold-based
`TierPolicy::evaluate()` function as the Abwägungsentscheidung (tier-decision
engine).

### Future Plan — Autonomous ML/LoRA Tier Decisions

> **Note (2026-04-17):** The `TierPolicy::decision_fn` hook is explicitly
> designed to be replaced by a ThemisDB LoRA adapter that makes autonomous,
> workload-aware tier decisions without hard-coded thresholds.

#### What the LoRA advisor will observe

| Signal                       | Source                                     |
|------------------------------|--------------------------------------------|
| Access frequency per key     | Hot-tier hit/miss counters                 |
| Query timestamp distribution | `getAsOf()` call histogram                 |
| RAM pressure ratio           | `TierDecisionContext::warm_pressure`       |
| Version age                  | `oldest_hot_sys_start` relative to `now`   |
| Key cardinality per table    | `tableStats()`                             |
| Time-of-day / workload phase | System clock + workload fingerprint        |

#### Wiring (planned)

```cpp
// Future API — not yet implemented
TierPolicy policy;
policy.decision_fn = [&lora_router](const TierDecisionContext& ctx)
    -> TierDecision {
    return lora_router.adviseTierDecision(ctx);
};
tier_manager.setPolicy(policy);
```

The `TierDecisionContext` struct is the stable input interface.
The `TierDecision` enum is the stable output interface.
Both are frozen; the LoRA model only needs to implement the mapping.

#### Training strategy

- Domain: `DATABASE_OPTIMIZER` (maps to existing `DomainType` enum).
- Labels: `KEEP`, `FLUSH_HOT_TO_WARM`, `FLUSH_WARM_TO_COLD`.
- Feature vector: context fields serialised to JSON for the LoRA input.
- Data source: decision logs written by `DecisionRecordYamlProcessor`
  whenever the built-in threshold logic fires (self-labelled training data).
- Feedback loop: `WorkloadFingerprintEngine` supplies access-pattern
  embeddings; `IntentClassifier` classifies query intent to enrich context.

#### Milestones

- `[ ]` Wire `TierDecisionContext` to `DecisionRecordYamlProcessor` log (Target: Q3 2026)
- `[ ]` Collect 10 k labelled tier decisions for initial LoRA fine-tune (Target: Q3 2026)
- `[ ]` Implement `LoRARouter::adviseTierDecision(ctx)` using fine-tuned adapter (Target: Q4 2026)
- `[ ]` A/B test: threshold policy vs. LoRA policy, measure RAM savings + query latency (Target: Q4 2026)
- `[ ]` Promote LoRA advisor to default; keep thresholds as fallback (Target: Q1 2027)

#### Hook contract (stable, do not change)

```cpp
// in TierPolicy:
std::function<TierDecision(const TierDecisionContext&)> decision_fn;
// nullptr  → built-in thresholds (current production)
// non-null → called synchronously on every insert(); must be fast
```

---

**Status**: Hook implemented, LoRA wiring pending  
**Last Updated**: 2026-04-17
