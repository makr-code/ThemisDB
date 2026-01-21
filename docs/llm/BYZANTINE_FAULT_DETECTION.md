# Byzantine Fault Detection for Distributed Training

## Overview

Byzantine Fault Detection is a critical security feature for distributed LoRA training that identifies and handles malicious or corrupted gradients from participating shards. This ensures model integrity and prevents training corruption from hardware failures, software bugs, or adversarial attacks.

## Table of Contents

1. [Introduction](#introduction)
2. [Byzantine Failures in Distributed Training](#byzantine-failures)
3. [Detection Methods](#detection-methods)
4. [Configuration](#configuration)
5. [Usage Examples](#usage-examples)
6. [Tuning Guide](#tuning-guide)
7. [Security Best Practices](#security-best-practices)
8. [Performance Considerations](#performance-considerations)
9. [Troubleshooting](#troubleshooting)

## Introduction

In distributed training, **Byzantine failures** occur when shards send incorrect gradients due to:

- **Malicious actors**: Deliberate gradient corruption (adversarial attacks)
- **Hardware failures**: GPU errors, memory corruption, bit flips
- **Software bugs**: Incorrect gradient computations
- **Data poisoning**: Backdoors or label flipping attacks

Without detection, these failures can:
- Cause model divergence or non-convergence
- Inject backdoors into trained models  
- Reduce model accuracy
- Waste training resources

Byzantine Fault Detection automatically identifies anomalous gradients and takes corrective action.

## Byzantine Failures

### Common Failure Modes

| Failure Type | Cause | Impact |
|--------------|-------|--------|
| Scale Attack | Malicious shard multiplies gradients by large factor | Model divergence |
| Sign Flip | Shard negates gradients | Reverses training progress |
| Zero Gradients | Shard sends all zeros | No learning from that shard |
| Gaussian Noise | Hardware corruption adds random noise | Unstable training |
| Stale Gradients | Shard sends old gradients | Delayed convergence |
| Backdoor Injection | Specific pattern injected | Model backdoor |

### Real-World Scenarios

**Hardware Failure Example:**
```
GPU error on Shard-2 → corrupted gradient computation →
Byzantine detector identifies outlier → Shard-2 excluded →
Training continues with remaining shards
```

**Adversarial Attack Example:**
```
Malicious actor controls Shard-5 → sends scaled gradients (×100) →
Median detector flags high anomaly score →
Gradients clipped to safe range or shard excluded
```

## Detection Methods

ThemisDB implements four Byzantine detection algorithms:

### 1. Median-based Detection (MEDIAN)

**Algorithm:** Uses Median Absolute Deviation (MAD) to detect outliers.

**How it works:**
1. Compute L2 norm of gradients from each shard
2. Calculate median norm across all shards
3. Compute MAD (Median Absolute Deviation)
4. Flag shards where `|norm - median| > threshold × MAD`

**Advantages:**
- Fast (O(n log n))
- Low false positive rate
- Works well for most scenarios

**Best for:**
- General-purpose Byzantine detection
- Production deployments with minimal overhead

**Parameters:**
- `detection_threshold`: MAD multiplier (default: 3.0)
  - Lower = more sensitive (more detections)
  - Higher = more tolerant (fewer false positives)

### 2. Krum Algorithm (KRUM)

**Algorithm:** Selects gradients closest to other gradients, excludes outliers.

**How it works:**
1. Compute pairwise distances between all gradients
2. For each gradient, sum distances to k nearest neighbors
3. Select m gradients with smallest scores
4. Reject remaining gradients as Byzantine

**Advantages:**
- Stronger theoretical guarantees
- Good for adversarial environments

**Limitations:**
- Requires at least `2f + 3` shards (f = max Byzantine shards)
- Slower (O(n²))

**Best for:**
- Security-critical deployments
- Known adversarial threats

**Parameters:**
- `max_byzantine_shards` (f): Maximum Byzantine shards tolerated

### 3. Bulyan Algorithm (BULYAN)

**Algorithm:** Enhanced Krum with trimmed mean for stronger guarantees.

**How it works:**
1. Apply Krum to select m gradients
2. For each coordinate, compute trimmed mean (exclude top/bottom f values)
3. Return aggregated gradient

**Advantages:**
- Strongest Byzantine tolerance
- Tolerates up to `f < n/4` Byzantine shards

**Limitations:**
- Requires at least `4f + 3` shards
- Highest computational cost (O(n²))

**Best for:**
- High-security applications
- Mission-critical deployments

**Parameters:**
- `max_byzantine_shards` (f): Maximum Byzantine shards tolerated

### 4. Ensemble Detection (ENSEMBLE)

**Algorithm:** Combines multiple detection methods for best accuracy.

**How it works:**
1. Run both Median and Krum detection
2. Combine results (a shard is suspected if detected by either method)
3. Use maximum anomaly score from both methods

**Advantages:**
- Best detection rate
- Very low false negative rate

**Limitations:**
- Higher computational overhead (~10%)

**Best for:**
- Maximum security requirements
- Zero-trust environments

## Configuration

### Basic Configuration

Add Byzantine detection to your distributed training config:

```cpp
DistributedTrainingConfig config;
config.sync_strategy = SyncStrategy::ALL_REDUCE;
config.participant_shards = {"shard-1", "shard-2", "shard-3", "shard-4"};

// Enable Byzantine detection
config.enable_byzantine_detection = true;
config.detection_method = ByzantineDetectionMethod::MEDIAN;
config.detection_threshold = 3.0f;
config.max_byzantine_shards = 1;
config.byzantine_action = ByzantineAction::EXCLUDE;
```

### Configuration Options

#### Detection Method
```cpp
enum class ByzantineDetectionMethod {
    NONE,        // Disabled
    MEDIAN,      // Median + MAD (recommended)
    KRUM,        // Krum algorithm
    BULYAN,      // Bulyan algorithm
    ENSEMBLE     // Combine methods
};
```

#### Action on Detection
```cpp
enum class ByzantineAction {
    WARN,        // Log warning, continue training
    EXCLUDE,     // Exclude suspected shards (recommended)
    CLIP,        // Clip gradients to safe range
    SHUTDOWN     // Stop training immediately
};
```

### YAML Configuration

```yaml
lora_training:
  distributed:
    enabled: true
    sync_strategy: all_reduce
    
    # Byzantine fault detection
    byzantine_detection:
      enabled: true
      method: median  # median, krum, bulyan, ensemble
      threshold: 3.0  # MAD multiplier (for median)
      max_byzantine_shards: 1  # f parameter (for krum/bulyan)
      action: exclude  # warn, exclude, clip, shutdown
```

## Usage Examples

### Example 1: Basic Byzantine Detection

```cpp
#include "llm/distributed_training_coordinator.h"
#include "llm/byzantine_detector.h"

// Create coordinator with Byzantine detection
DistributedTrainingConfig config;
config.enable_byzantine_detection = true;
config.detection_method = ByzantineDetectionMethod::MEDIAN;
config.detection_threshold = 3.0f;
config.byzantine_action = ByzantineAction::EXCLUDE;

auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
    shard_router, shard_topology, config
);

// Initialize training
coordinator->initialize("my-adapter", training_config);

// Execute training steps
while (!training_complete) {
    auto result = coordinator->executeStep();
    
    if (result.success) {
        // Byzantine detection happens automatically during aggregation
        // Check if any Byzantine shards were detected
        auto stats = coordinator->getStatistics();
        if (stats.byzantine_detections > 0) {
            std::cout << "Byzantine shards detected: " 
                     << stats.byzantine_shards_excluded << std::endl;
        }
    }
}
```

### Example 2: Custom Detection Threshold

```cpp
// More sensitive detection (lower threshold)
config.detection_threshold = 2.5f;  // Flag more aggressively

// More lenient detection (higher threshold)
config.detection_threshold = 4.0f;  // Reduce false positives
```

### Example 3: Using Krum for Adversarial Environments

```cpp
// Configure for adversarial environment
config.enable_byzantine_detection = true;
config.detection_method = ByzantineDetectionMethod::KRUM;
config.max_byzantine_shards = 2;  // Tolerate up to 2 Byzantine shards
config.byzantine_action = ByzantineAction::EXCLUDE;

// Note: Requires at least 2*2+3 = 7 shards
```

### Example 4: Monitoring Byzantine Detection

```cpp
// Set up progress callback to monitor detections
coordinator->setProgressCallback([](int step, const auto& result) {
    if (result.success) {
        // Log Byzantine detection events
        // (Detection results are logged automatically by coordinator)
    }
});

// Get statistics
auto stats = coordinator->getStatistics();
std::cout << "Byzantine detections: " << stats.byzantine_detections << std::endl;
std::cout << "Shards excluded: " << stats.byzantine_shards_excluded << std::endl;
std::cout << "Avg anomaly score: " << stats.avg_anomaly_score << std::endl;

// Per-shard detection counts
for (const auto& [shard_id, count] : stats.per_shard_detection_count) {
    std::cout << "  " << shard_id << ": " << count << " detections" << std::endl;
}
```

## Tuning Guide

### Choosing Detection Method

| Scenario | Recommended Method | Reasoning |
|----------|-------------------|-----------|
| Production (untrusted environment) | MEDIAN | Best balance of speed and accuracy |
| Known adversarial threats | KRUM or BULYAN | Stronger theoretical guarantees |
| Security-critical applications | ENSEMBLE | Maximum detection accuracy |
| Low overhead required | MEDIAN | Minimal performance impact (~1-2%) |
| High number of shards (>10) | MEDIAN | Scales better than Krum/Bulyan |

### Threshold Tuning (Median Method)

**Default:** `threshold = 3.0` (3 MAD)

**Adjust based on:**

- **False Positives (legitimate shards flagged):**
  - Increase threshold to 3.5 or 4.0
  - Review gradient distribution (may indicate data imbalance)

- **Missed Attacks (Byzantine shards not detected):**
  - Decrease threshold to 2.5 or 2.0
  - Consider switching to ENSEMBLE method

- **Gradient Variability:**
  - High variability → increase threshold
  - Low variability → can use lower threshold

### Shard Count Requirements

| Method | Minimum Shards | Formula |
|--------|----------------|---------|
| MEDIAN | 2 | n ≥ 2 |
| KRUM | 2f + 3 | f = max_byzantine_shards |
| BULYAN | 4f + 3 | f = max_byzantine_shards |
| ENSEMBLE | 2f + 3 | (uses Krum internally) |

**Example:**
- For f=1 (tolerate 1 Byzantine shard):
  - Krum needs ≥ 5 shards
  - Bulyan needs ≥ 7 shards

## Security Best Practices

### 1. Enable in Production

Always enable Byzantine detection in production multi-tenant environments:

```cpp
config.enable_byzantine_detection = true;  // Always true in production
```

### 2. Use Appropriate Action

Choose action based on trust level:

```cpp
// Trusted environment (internal shards)
config.byzantine_action = ByzantineAction::WARN;

// Semi-trusted environment
config.byzantine_action = ByzantineAction::CLIP;

// Untrusted environment (recommended)
config.byzantine_action = ByzantineAction::EXCLUDE;

// Zero-trust environment
config.byzantine_action = ByzantineAction::SHUTDOWN;
```

### 3. Monitor Detection Metrics

Set up alerts for Byzantine detections:

```cpp
if (stats.byzantine_detections > 0) {
    // Alert monitoring system
    alert("Byzantine shards detected", {
        {"count", stats.byzantine_shards_excluded},
        {"avg_anomaly_score", stats.avg_anomaly_score}
    });
}
```

### 4. Implement Reputation System

Track shard behavior over time:

```cpp
// Check per-shard detection counts
for (const auto& [shard_id, count] : stats.per_shard_detection_count) {
    if (count > 3) {
        // Shard has been flagged multiple times
        // Consider blacklisting or investigating
        blacklist_shard(shard_id);
    }
}
```

### 5. Regular Threshold Review

Periodically review and adjust thresholds based on:
- False positive rate
- Missed attack rate  
- Gradient distribution statistics

## Performance Considerations

### Overhead by Method

| Method | Overhead | Throughput Impact |
|--------|----------|-------------------|
| MEDIAN | ~1-2% | Negligible |
| KRUM | ~3-5% | Low |
| BULYAN | ~5-8% | Moderate |
| ENSEMBLE | ~10% | Moderate |

### Optimization Tips

1. **Use MEDIAN for large scale:**
   - Scales to 20+ shards with minimal overhead
   - O(n log n) complexity

2. **Batch size optimization:**
   - Byzantine detection overhead is per-step
   - Larger batches amortize cost

3. **Gradient compression:**
   - Byzantine detection works with compressed gradients
   - Apply compression before detection to reduce overhead

## Troubleshooting

### High False Positive Rate

**Symptoms:** Legitimate shards frequently flagged

**Solutions:**
1. Increase detection threshold (3.0 → 3.5 or 4.0)
2. Check for data imbalance across shards
3. Verify gradient computation correctness
4. Review shard hardware health

### Missed Byzantine Shards

**Symptoms:** Known corrupted gradients not detected

**Solutions:**
1. Decrease detection threshold (3.0 → 2.5)
2. Switch to ENSEMBLE method
3. Verify detection is enabled
4. Check shard count meets requirements

### Performance Degradation

**Symptoms:** Training slower with Byzantine detection

**Solutions:**
1. Use MEDIAN instead of KRUM/BULYAN
2. Increase sync_frequency (reduce detection frequency)
3. Profile to confirm overhead is from detection

### Configuration Errors

**Error:** "Insufficient shards for Krum/Bulyan"

**Solution:** 
- Krum: Need at least `2f + 3` shards
- Bulyan: Need at least `4f + 3` shards
- Either add more shards or reduce `max_byzantine_shards`

## Advanced Topics

### Custom Detection Logic

For specialized scenarios, implement custom `ByzantineDetector`:

```cpp
class CustomDetector : public ByzantineDetector {
public:
    DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override {
        // Custom detection logic
        // ...
        return result;
    }
    
    std::string getName() const override { return "CUSTOM"; }
};
```

### Forensic Analysis

Log detection events for post-mortem analysis:

```cpp
void logByzantineEvent(const DetectionResult& result) {
    json event;
    event["timestamp"] = getCurrentTimestamp();
    event["method"] = result.detection_method;
    event["suspected_shards"] = result.suspected_shards;
    event["anomaly_scores"] = result.anomaly_scores;
    
    // Store in audit log
    audit_log->write(event);
}
```

## References

- [Krum: Communication-Efficient Machine Learning with Byzantine Workers](https://papers.nips.cc/paper/2017/hash/f4b9ec30ad9f68f89b29639786cb62ef-Abstract.html)
- [The Byzantine Generals Problem](https://lamport.azurewebsites.net/pubs/byz.pdf)
- [Bulyan: Byzantine-tolerant Machine Learning](https://arxiv.org/abs/1802.07927)

## Support

For questions or issues:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.readthedocs.io
- Security: security@themisdb.io
