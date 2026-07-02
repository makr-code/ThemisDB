/**
 * @file AUTOMATIC_RETRAINING_IMPLEMENTATION.md
 * @brief Implementation Guide: Automatic LoRA/LLM Retraining with Feedback Integration
 * @date 2026-07-02
 * @author AI Agent (Copilot)
 * @issue #5447
 * @status COMPLETE
 */

# Automatic LoRA/LLM Retraining with Feedback Integration

## Executive Summary

This implementation provides an end-to-end automatic retraining system for LoRA adapters and LLMs that:

1. **Collects feedback** from both user ratings and telemetry metrics
2. **Detects quality degradation** through intelligent pattern recognition
3. **Triggers retraining** automatically based on configurable thresholds
4. **Manages versions** with safe rollback capability
5. **Maintains audit trails** for compliance and debugging

**Key Achievement:** Closes issue #5447 by implementing a production-ready feedback pipeline that enables adapters to automatically improve based on real-world performance.

---

## Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     LoRA Adapter System                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │         1. FEEDBACK COLLECTION LAYER                    │  │
│  │  ┌─────────────────────────────────────────────────┐    │  │
│  │  │  User Feedback          Telemetry Metrics      │    │  │
│  │  │  ├─ Rating (1-5)        ├─ Accuracy (0-1)     │    │  │
│  │  │  ├─ Correction          ├─ Latency (ms)       │    │  │
│  │  │  └─ Text Comment        ├─ Error Rate         │    │  │
│  │  │                         └─ Throughput         │    │  │
│  │  └─────────────────────────────────────────────────┘    │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                          │                         │
│           └──────────────┬───────────┘                         │
│                          ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │    2. TELEMETRY FEEDBACK ADAPTER                         │  │
│  │    ├─ Normalize metrics to [0-1] range                  │  │
│  │    ├─ Detect quality issues:                            │  │
│  │    │  ├─ Low accuracy (< min_threshold)                │  │
│  │    │  ├─ High latency (> max_latency)                  │  │
│  │    │  ├─ Consecutive errors (>= max_errors)           │  │
│  │    │  └─ High error rate (> threshold)                 │  │
│  │    ├─ Generate synthetic feedback for issues            │  │
│  │    └─ Aggregate metrics in time window (5 min)         │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │    3. ADAPTIVE RETRAINING CONTROLLER                     │  │
│  │    ├─ Accumulate feedback entries                       │  │
│  │    ├─ Evaluate multi-trigger decision:                  │  │
│  │    │  ├─ Feedback Threshold (e.g., 50 items)          │  │
│  │    │  ├─ Time Interval (e.g., 24 hours)               │  │
│  │    │  └─ Quality Degradation (e.g., 5% drop)          │  │
│  │    ├─ Apply rate limiting (min 1 hour between)         │  │
│  │    ├─ Convert feedback to training data                │  │
│  │    └─ Orchestrate training execution                   │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │    4. ADAPTER VERSION MANAGER                            │  │
│  │    ├─ Create version snapshots                          │  │
│  │    ├─ Compute SHA256 checksums                          │  │
│  │    ├─ Track version metadata:                           │  │
│  │    │  ├─ Training source & samples                     │  │
│  │    │  ├─ Final loss & validation accuracy             │  │
│  │    │  ├─ Deployment status & stability                │  │
│  │    │  └─ Rollback eligibility                         │  │
│  │    └─ Automatic version cleanup (retention policy)    │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │    5. ROLLBACK & MONITORING LAYER                        │  │
│  │    ├─ Monitor deployed adapter quality                  │  │
│  │    ├─ Detect accuracy drops:                            │  │
│  │    │  ├─ Track baseline metrics per version            │  │
│  │    │  ├─ Compare current vs. baseline                  │  │
│  │    │  └─ Auto-rollback if threshold exceeded           │  │
│  │    └─ Audit all train/deploy/rollback events           │  │
│  └──────────────────────────────────────────────────────────┘  │
│                          │                                    │
└──────────────────────────┼────────────────────────────────────┘
                           ▼
                    LoRATrainingService
                (Existing training infrastructure)
```

### Data Flow

```
User/Telemetry Input
    ↓
Feedback Validation & Processing
    ├─ Check required fields
    ├─ Apply training weights
    └─ Flag for training
    ↓
Accumulation & Batching
    ├─ Store in feedback_buffer
    ├─ Monitor count/size
    └─ Aggregate telemetry metrics
    ↓
Trigger Evaluation
    ├─ Threshold: feedback_count >= threshold
    ├─ Interval: time_since_last >= interval
    └─ Quality: metrics_degraded(current, baseline)
    ↓
Retraining Decision
    ├─ Rate limit check
    ├─ Confidence scoring
    └─ Decision rationale
    ↓
Training Execution
    ├─ Convert feedback → TrainingData
    ├─ Invoke LoRATrainingService::trainOnTheFly()
    └─ Wait for completion
    ↓
Version Snapshot
    ├─ Create version record
    ├─ Compute SHA256 hash
    └─ Store adapter snapshot
    ↓
Deployment & Monitoring
    ├─ Update active version
    ├─ Start quality monitoring
    └─ Set baseline metrics
    ↓
Quality Watch
    ├─ Collect inference metrics
    ├─ Detect degradation
    └─ Trigger rollback if needed
```

---

## Component Details

### 1. Telemetry Feedback Adapter

**Header:** `include/llm/lora_framework/telemetry_feedback_adapter.h`
**Implementation:** `src/llm/lora_framework/telemetry_feedback_adapter.cpp`

#### Purpose
Bridges system telemetry with the feedback pipeline. Converts performance metrics into actionable feedback entries.

#### Key Classes

```cpp
struct TelemetryMetrics {
    // Quality indicators
    float accuracy = 0.0f;
    float precision = 0.0f;
    float recall = 0.0f;
    float f1_score = 0.0f;
    float perplexity = 0.0f;
    
    // Performance indicators
    float latency_ms = 0.0f;
    int input_tokens, output_tokens;
    
    // Error tracking
    bool has_error = false;
    int consecutive_errors = 0;
};

class TelemetryFeedbackAdapter {
    // Record a metric and get optional feedback if quality issue detected
    std::optional<Feedback> recordMetric(const TelemetryMetrics& metric);
    
    // Compute aggregate metrics for a version
    AdapterVersionMetrics computeVersionMetrics(
        const std::string& adapter_id,
        const std::optional<std::string>& version_to_check = std::nullopt
    ) const;
    
    // Check if quality degraded vs. baseline
    bool isQualityDegraded(
        const std::string& adapter_id,
        const AdapterVersionMetrics& baseline_metrics
    ) const;
};
```

#### Configuration

```cpp
struct Config {
    // Thresholds for feedback generation
    float min_accuracy_threshold = 0.85f;
    float max_latency_ms = 500.0f;
    int max_consecutive_errors = 3;
    float error_rate_threshold = 0.1f;  // 10%
    
    // Aggregation window (metrics pruned after this)
    std::chrono::seconds aggregation_window{300};  // 5 min
    
    // History retention
    size_t max_metrics_history = 1000;
};
```

#### Quality Issue Detection Algorithm

1. **Accuracy Check:** If accuracy < min_accuracy_threshold (0.85)
2. **Latency Check:** If latency_ms > max_latency_ms (500)
3. **Error Check:** If has_error = true
4. **Consecutive Error Check:** If consecutive_errors >= max (3)
5. **Error Rate Check:** If error_count/total_queries > threshold (10%)

#### Generated Feedback Properties
- `id`: UUID (Boost)
- `adapter_id`: From metric
- `user_id`: "telemetry_system"
- `rating`: 1-5 based on accuracy
- `training_category`: "negative"/"neutral" based on severity
- `training_weight`: 0.7 (lower than user feedback to avoid over-fitting to single bad metric)
- `metadata`: Includes telemetry details (latency, perplexity, etc.)

### 2. Adaptive Retraining Controller

**Header:** `include/llm/lora_framework/adaptive_retraining_controller.h`
**Implementation:** `src/llm/lora_framework/adaptive_retraining_controller.cpp`

#### Purpose
Orchestrates the automatic retraining process with multiple trigger conditions.

#### Key Classes

```cpp
struct RetrainingDecision {
    bool should_retrain = false;
    enum class Reason {
        FEEDBACK_THRESHOLD_MET,
        TIME_INTERVAL_ELAPSED,
        QUALITY_DEGRADATION,
        MANUAL_TRIGGER,
        ERROR_RECOVERY
    };
    Reason reason;
    float confidence = 0.0f;  // 0-1
};

struct RetrainingResult {
    bool success = false;
    std::string old_version, new_version;
    float final_loss = 0.0f;
    float validation_accuracy = 0.0f;
    float accuracy_improvement = 0.0f;
    bool requires_rollback = false;
};

struct RetrainingTriggerConfig {
    // Feedback trigger
    bool feedback_trigger_enabled = true;
    size_t feedback_threshold = 50;
    
    // Time interval trigger
    bool interval_trigger_enabled = true;
    std::chrono::hours retraining_interval{24};
    
    // Quality degradation trigger
    bool quality_trigger_enabled = true;
    float accuracy_drop_threshold = 0.05f;  // 5%
    float latency_increase_threshold = 1.2f;  // 20%
    
    // Rollback config
    bool auto_rollback_enabled = true;
    float rollback_accuracy_threshold = 0.80f;
    
    // Rate limiting
    std::chrono::seconds min_time_between_retrains{3600};  // 1 hour
};

class AdaptiveRetrainingController {
public:
    // Add user feedback
    void addFeedback(const Feedback& feedback);
    
    // Add telemetry metric
    void addMetric(const TelemetryMetrics& metric);
    
    // Evaluate if retraining needed
    RetrainingDecision evaluateRetrainingNeed();
    
    // Execute retraining
    RetrainingResult executeRetraining(
        const RetrainingDecision& decision,
        const std::optional<TrainingData>& training_data = std::nullopt
    );
    
    // Check and rollback if quality dropped
    bool checkAndRollbackIfNeeded();
    
    // Manual trigger
    RetrainingResult triggerRetrainingNow(
        const std::optional<TrainingData>& training_data = std::nullopt
    );
};
```

#### Trigger Evaluation Logic

```cpp
RetrainingDecision evaluate() {
    // Priority order:
    
    // 1. Check rate limiting first
    if (time_since_last_retrain < min_time_between_retrains) {
        return NO_RETRAIN;  // Too soon
    }
    
    // 2. Check feedback threshold (highest confidence)
    if (feedback_buffer.size() >= threshold) {
        return RETRAIN(FEEDBACK_THRESHOLD_MET, confidence=0.95);
    }
    
    // 3. Check time interval (medium confidence)
    if (time_since_last_retrain > interval) {
        if (feedback_buffer.size() > 10) {  // Must have some data
            return RETRAIN(TIME_INTERVAL_ELAPSED, confidence=0.85);
        }
    }
    
    // 4. Check quality degradation (high confidence)
    if (is_quality_degraded(current, baseline)) {
        return RETRAIN(QUALITY_DEGRADATION, confidence=0.90);
    }
    
    return NO_RETRAIN;
}
```

#### Rollback Decision Logic

```cpp
bool checkAndRollback() {
    if (!auto_rollback_enabled) return false;
    
    auto current_metrics = compute_metrics(current_version);
    
    // Check if accuracy dropped below threshold
    if (current_metrics.avg_accuracy < rollback_accuracy_threshold) {
        // Find and restore previous version
        previous_version = version_history[size - 2];
        restore_version(previous_version);
        log_rollback_event(...);
        return true;
    }
    
    return false;
}
```

### 3. Adapter Version Manager

**Header:** `include/llm/lora_framework/adapter_version_manager.h`
**Implementation:** `src/llm/lora_framework/adapter_version_manager.cpp`

#### Purpose
Manages adapter versioning, snapshots, and rollback capability.

#### Key Classes

```cpp
struct AdapterVersionInfo {
    std::string version_id;                 // "v1.0", "v1.1", etc.
    std::string adapter_id;
    std::chrono::system_clock::time_point created_at;
    
    // Training metadata
    std::string training_source;            // "user_feedback", "telemetry", etc.
    int training_samples = 0;
    float final_loss = 0.0f;
    float validation_accuracy = 0.0f;
    
    // Performance metrics
    float avg_accuracy = 0.0f;
    float avg_latency_ms = 0.0f;
    int total_inferences = 0;
    int errors = 0;
    
    // Status
    bool is_active = false;
    bool is_stable = true;
    std::string deployment_status;          // "staging", "production", "disabled"
    
    // Integrity
    std::string weights_hash;               // SHA256
    bool can_rollback_to = true;
};

struct AdapterVersionSnapshot {
    std::string version_id;
    std::string storage_path;
    std::string checksum;                   // SHA256 of adapter_config
    json adapter_config;
    // ... metadata
};

struct VersionComparison {
    std::string version_a, version_b;
    float accuracy_delta = 0.0f;            // positive = improvement
    float latency_delta_ms = 0.0f;          // negative = improvement
    bool is_b_better = false;
};

class AdapterVersionManager {
public:
    // Version lifecycle
    bool createVersion(const std::string& version_id, 
                      const std::string& training_source,
                      const json& metrics);
    
    std::optional<AdapterVersionSnapshot> createSnapshot(
        const std::string& version_id,
        const json& adapter_data);
    
    bool setActiveVersion(const std::string& version_id);
    
    // Rollback
    bool rollback(const std::string& version_id);
    
    // Analysis
    std::optional<VersionComparison> compareVersions(
        const std::string& version_a,
        const std::string& version_b) const;
    
    // Maintenance
    int cleanup();  // Remove old versions based on retention policy
};
```

#### Snapshot Storage & Integrity

```
Storage Format:
  Path: <snapshot_storage_path>/<adapter_id>_<version>.json[.gz]
  
  Content: {
    "version_id": "v2.0",
    "adapter_id": "themis_help_lora",
    "adapter_config": { /* full adapter state */ },
    "training_config": { /* training parameters */ },
    "checksum": "abc123...",  // SHA256 of adapter_config
    "is_compressed": true,
    "snapshot_time": 1234567890
  }

Integrity Check:
  1. Load snapshot JSON
  2. Recompute SHA256 of adapter_config field
  3. Compare with stored checksum
  4. FAIL if mismatch
```

#### Version Retention Policy

```cpp
struct Config {
    size_t max_versions_kept = 10;
    std::chrono::days version_retention_period{30};  // 30 days
};

// Cleanup algorithm:
// 1. Delete versions older than retention_period
// 2. If still > max_versions_kept, delete oldest remaining
// 3. Never delete active version
```

---

## Integration Points

### With Existing Components

#### 1. LoRATrainingService
```cpp
// Called by AdaptiveRetrainingController::executeRetraining()
auto result = training_service->trainOnTheFly(adapter_id, training_data);
```

#### 2. LoRAAuditLogger
```cpp
// Log all events
audit_logger->logTraining(
    LoRAAuditEventType::TRAINING_STARTED,
    adapter_id,
    feedback_count,
    0.0f,
    0.0f,
    {/* metadata */}
);

// Log rollback
audit_logger->logEvent(
    LoRAAuditEventType::ROLLBACK_TRIGGERED,
    adapter_id,
    {/* rollback details */}
);
```

#### 3. ThemisHelpLoRA
```cpp
// Can inject the retraining controller
auto controller = std::make_unique<AdaptiveRetrainingController>(
    adapter_id,
    deps,
    config
);

// Periodically evaluate and execute
auto decision = controller->evaluateRetrainingNeed();
if (decision.should_retrain) {
    auto result = controller->executeRetraining(decision);
}
```

---

## Usage Examples

### Basic Setup

```cpp
// 1. Create dependencies
auto telemetry_adapter = std::make_shared<TelemetryFeedbackAdapter>();
auto audit_logger = std::make_shared<LoRAAuditLogger>();
auto training_service = std::make_shared<LoRATrainingService>();

// 2. Configure triggers
RetrainingTriggerConfig config;
config.feedback_threshold = 50;
config.retraining_interval = std::chrono::hours(24);
config.auto_rollback_enabled = true;

// 3. Create controller
AdaptiveRetrainingController::Dependencies deps{
    telemetry_adapter,
    audit_logger,
    training_service
};

AdaptiveRetrainingController controller("my_adapter", deps, config);
```

### Feedback Collection

```cpp
// From user
Feedback user_fb;
user_fb.adapter_id = "my_adapter";
user_fb.user_id = "user123";
user_fb.rating = 5;
user_fb.prompt = "What is AI?";
user_fb.response = "AI is...";
user_fb.flagged_for_training = true;
controller.addFeedback(user_fb);

// From telemetry
TelemetryMetrics metric;
metric.adapter_id = "my_adapter";
metric.accuracy = 0.88f;
metric.latency_ms = 150.0f;
controller.addMetric(metric);  // May auto-generate feedback
```

### Retraining Workflow

```cpp
// Evaluate need
auto decision = controller.evaluateRetrainingNeed();

if (decision.should_retrain) {
    // Execute
    auto result = controller.executeRetraining(decision);
    
    if (result.success) {
        // New version deployed: result.new_version
        // Check quality improvement
        if (result.accuracy_improvement > 0.02f) {
            // Good improvement
        }
    }
}

// Later: Check for rollback
if (controller.checkAndRollbackIfNeeded()) {
    // Quality degradation detected, rolled back
}
```

### Version Management

```cpp
AdapterVersionManager version_mgr("my_adapter");

// Create versions
version_mgr.createVersion("v1.0", "initial", {});
version_mgr.createVersion("v1.1", "user_feedback", {});

// Create snapshots
json adapter_data = /* adapter weights */;
version_mgr.createSnapshot("v1.1", adapter_data);

// Compare versions
auto comparison = version_mgr.compareVersions("v1.0", "v1.1");
if (comparison && comparison->is_b_better) {
    version_mgr.setActiveVersion("v1.1");
}

// Rollback if needed
version_mgr.rollback("v1.0");
```

---

## Testing

**Test File:** `tests/lora/test_adaptive_retraining.cpp`

### Test Coverage

| Category | Test | Verifies |
|----------|------|----------|
| **Telemetry** | Quality issue detection | Low accuracy, high latency, errors generate feedback |
| **Telemetry** | Metrics aggregation | Time-windowed metric collection |
| **Telemetry** | Degradation detection | Quality drop vs. baseline identified |
| **Retraining** | Feedback threshold trigger | Retrains when feedback >= threshold |
| **Retraining** | Execution success | Training completes, version incremented |
| **Retraining** | Version history | Versions tracked correctly |
| **Versions** | Creation & storage | Versions created with metadata |
| **Versions** | Snapshot creation | Snapshots with checksums created |
| **Versions** | Rollback | Versions rolled back correctly |
| **Versions** | Comparison | Versions compared for improvement |
| **Integration** | End-to-end flow | Full pipeline from feedback to deployment |
| **Integration** | Auto-rollback | Rollback triggered on quality drop |
| **Integration** | History tracking | Retraining history maintained |
| **Integration** | Metrics export | Metadata exported correctly |

---

## Performance Considerations

### Time Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| recordMetric | O(1) | Lock + push_back |
| evaluateRetrainingNeed | O(1) | All checks O(1) |
| createVersion | O(v) | v = existing versions |
| rollback | O(v) | Load snapshot, update state |
| getVersionMetrics | O(m) | m = metrics in window |
| cleanup | O(v log v) | Sorting old versions |

### Space Complexity

| Storage | Size | Notes |
|---------|------|-------|
| metrics_buffer | O(m) | m ~ 1000 recent metrics |
| feedback_buffer | O(f) | f ~ 100 pending feedback items |
| version_history | O(v) | v ~ 10 recent versions |
| snapshots | O(v × s) | v ~ 10, s ~ 500MB each = 5GB potential |

### Snapshot Storage Management

- Default max 10 versions × 500MB = 5GB max
- Configure via `AdapterVersionManager::Config::max_versions_kept`
- Automatic cleanup by age + size
- Compression optional (gzip)

---

## Failure Modes & Recovery

| Failure | Detection | Recovery |
|---------|-----------|----------|
| Training fails | Exception in trainOnTheFly() | Log error, keep current version, retry later |
| Snapshot corrupt | Checksum mismatch on load | Abort rollback, keep current version |
| Version deploy fails | Result.success = false | Log, notify admin, no version change |
| Quality degrades post-deploy | Metric monitoring | Auto-rollback to previous version |
| Rate limit exceeded | time_since < min_interval | Skip retrain, retry after interval |
| Feedback buffer overflow | buffer.size() > MAX | FIFO eviction or return error |

---

## Security Considerations

1. **Input Validation**
   - Verify feedback ratings (1-5)
   - Validate metric ranges (0-1 for scores, >= 0 for latency)
   - Sanitize feedback text (prevent injection)

2. **Access Control**
   - Feedback from authenticated users only
   - Telemetry from trusted monitoring systems only
   - Version management restricted to authorized components

3. **Integrity**
   - SHA256 checksums on snapshots
   - Version metadata immutable after creation
   - Audit log for all train/deploy/rollback events

4. **Thread Safety**
   - All operations protected by mutex
   - RAII-based lock management
   - No deadlock with consistent lock ordering

---

## Future Enhancements

1. **Distributed Retraining**
   - Multi-GPU training coordination
   - Federated learning across model instances

2. **Advanced Triggers**
   - Statistical significance testing for improvements
   - A/B testing framework for gradual deployment
   - Anomaly detection for unusual metric patterns

3. **Adaptive Thresholds**
   - ML-based threshold optimization
   - Per-adapter configuration based on domain

4. **Model Selection**
   - Ensemble of versions for inference
   - Multi-armed bandit for version routing

5. **Observability**
   - Prometheus metrics export
   - Grafana dashboards
   - OpenTelemetry integration

---

## Conclusion

This implementation provides a robust, production-ready framework for automatic LoRA/LLM retraining with feedback integration. It satisfies all acceptance criteria:

✅ Secure feedback pipeline from user & telemetry  
✅ Automatic retraining triggers (threshold/interval/quality)  
✅ Safe rollback mechanism with quality guardrails  
✅ Comprehensive audit logging  
✅ End-to-end regression tests  

The modular design allows for easy extension and integration with existing ThemisDB components.
