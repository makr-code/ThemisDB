# Predictive Failure Detection

## Overview

ML-based system to predict shard failures before they occur, enabling proactive replacement and preventing data loss.

## Features

- **SMART Metrics Monitoring**: Temperature, reallocated sectors, error rates
- **Performance Tracking**: Latency trends, throughput degradation
- **I/O Pattern Analysis**: Error frequency, retry counts
- **Health Check Integration**: Recovery success rate, consistency errors
- **Proactive Alerts**: Notify operators when high-risk detected
- **Auto-Recovery Integration**: Works with auto-recovery system for automated response

## Configuration

```cpp
#include "sharding/predictive_detector.h"

PredictiveConfig config;
config.enabled = true;
config.model_path = "/path/to/models/failure_prediction.onnx";
config.check_interval = std::chrono::hours(1);
config.failure_threshold = 0.7f;  // 70% probability threshold
config.lookback_days = 30;        // 30-day historical window

// Set alert callback
config.alert_callback = [](const std::string& message) {
    LOG(WARNING) << message;
    // Send notification to ops team
};

// Create detector
PredictiveFailureDetector detector(config, strategy, topology);
detector.start();
```

## Usage

### Recording Metrics

```cpp
PredictiveShardMetrics metrics;
metrics.shard_id = "shard_001";
metrics.timestamp = std::chrono::system_clock::now();
metrics.avg_latency_ms = 10.5;
metrics.p95_latency_ms = 15.0;
metrics.p99_latency_ms = 20.0;
metrics.throughput_ops_per_sec = 1000;
metrics.read_errors = 0;
metrics.write_errors = 0;
metrics.retry_count = 2;
metrics.failed_health_checks = 0;
metrics.recovery_success_rate = 1.0f;

detector.recordMetrics(metrics);
```

### Getting Predictions

```cpp
// Get all predictions
auto predictions = detector.getPredictions();

// Get high-risk shards only
auto high_risk = detector.getHighRiskShards();

for (auto& pred : high_risk) {
    LOG(WARNING) << "Shard " << pred.shard_id 
                 << " has " << (pred.failure_probability * 100) 
                 << "% failure risk in next " 
                 << pred.predicted_days_to_failure << " days";
    
    // Trigger proactive replacement
    triggerShardReplacement(pred.shard_id);
}

// Predict specific shard
auto prediction = detector.predictShard("shard_001");
if (prediction.isHighRisk()) {
    // Take action
}
```

### Statistics

```cpp
auto stats = detector.getStats();

LOG(INFO) << "Predictions made: " << stats.predictions_made;
LOG(INFO) << "High risk detected: " << stats.high_risk_detected;
LOG(INFO) << "Alerts sent: " << stats.alerts_sent;
LOG(INFO) << "True positive rate: " << stats.getTruePositiveRate() * 100 << "%";
LOG(INFO) << "False positive rate: " << stats.getFalsePositiveRate() * 100 << "%";
LOG(INFO) << "Avg inference time: " << stats.avg_inference_time.count() << "ms";
```

## ML Model

### Training

The system uses a machine learning model to predict failures. To train a new model:

```bash
# Collect historical failure data
./collect_metrics.sh > failure_data.csv

# Train model
cd scripts
python train_failure_model.py \
    --data failure_data.csv \
    --output ../models/failure_prediction.onnx \
    --model-type xgboost

# Deploy model
# Place failure_prediction.onnx in models/ directory
# Configure path in PredictiveConfig
```

### Data Collection

Historical data should include:

**Required metrics per shard:**
- Timestamp
- Latency metrics (avg, p95, p99)
- Throughput
- Error counts (read/write)
- Retry counts
- Health check status
- Recovery attempts

**Target variable:**
- `failed`: 1 if shard failed within prediction horizon, 0 otherwise

See `models/README.md` for detailed schema.

### Model Performance

Target metrics:
- **True Positive Rate**: >70% (detect actual failures)
- **False Positive Rate**: <10% (minimize false alarms)
- **Inference Time**: <100ms per shard
- **Prediction Horizon**: 7-30 days advance warning

### Retraining

Retrain monthly with new failure data to maintain accuracy:

```bash
# Collect new data
./collect_recent_failures.sh > new_data.csv

# Combine with existing data
cat historical_data.csv new_data.csv > updated_data.csv

# Retrain
python train_failure_model.py --data updated_data.csv --output model_v2.onnx

# A/B test before deployment
```

## Architecture

### Components

1. **Metrics Collector**: Aggregates metrics from all shards
2. **Feature Extractor**: Computes statistical features (mean, std, trend)
3. **ML Inference Engine**: ONNX Runtime for fast prediction
4. **Alert Manager**: Notifies operators when high-risk detected
5. **Integration Layer**: Works with auto-recovery system

### Feature Engineering

The system extracts 50+ features from historical data:

**Latency features (0-5):**
- Mean, std dev, trend
- Current values (avg, p95, p99)

**Throughput features (6-9):**
- Mean, std dev, trend
- Current throughput

**Error rate features (10-14):**
- Mean, std dev, trend
- Current read/write errors

**Health features (15-18):**
- Failed health checks
- Recovery attempts
- Recovery success rate
- Retry count

Features are normalized to [0, 1] range for model input.

## Integration with Auto-Recovery

The predictive detector integrates with the auto-recovery system:

```cpp
// Auto-recovery config
AutoRecoveryConfig recovery_config;
recovery_config.enable_auto_repair = true;

AutoRecoveryManager recovery(recovery_config, strategy, ring, topology);

// Predictive detector
PredictiveConfig pred_config;
pred_config.enabled = true;
pred_config.alert_callback = [&recovery](const std::string& msg) {
    LOG(WARNING) << msg;
    // Trigger proactive repair
    recovery.triggerRepair();
};

PredictiveFailureDetector detector(pred_config, strategy, topology);

// Start both systems
recovery.start();
detector.start();

// High-risk shards automatically trigger repair
```

## Performance

- **Inference Time**: <50ms per shard (target: <100ms)
- **Memory**: ~1KB per shard for metrics history
- **CPU**: Minimal overhead with 1-hour check interval
- **Scalability**: Tested with 1000+ shards

## Monitoring

Monitor the detector itself:

```cpp
// Periodic health check
auto stats = detector.getStats();

if (stats.avg_inference_time > std::chrono::milliseconds(100)) {
    LOG(WARNING) << "Inference time exceeds target: " 
                 << stats.avg_inference_time.count() << "ms";
}

if (stats.getFalsePositiveRate() > 0.1f) {
    LOG(WARNING) << "False positive rate too high: " 
                 << stats.getFalsePositiveRate() * 100 << "%";
    // Consider retraining model
}
```

## Troubleshooting

### High False Positive Rate

- Retrain model with more recent data
- Adjust `failure_threshold` (increase from 0.7 to 0.8)
- Review feature importance

### Low True Positive Rate

- Collect more failure samples for training
- Increase `lookback_days` for more historical context
- Add more features (SMART metrics if available)

### High Inference Time

- Check model size (should be <10MB)
- Verify ONNX Runtime is properly optimized
- Consider reducing feature count

### No Predictions

- Verify model is loaded: check `model_path`
- Ensure metrics are being recorded
- Check that detector is started: `isRunning()`

## Best Practices

1. **Start Conservative**: Use high threshold (0.8) initially
2. **Monitor Performance**: Track true/false positive rates
3. **Regular Retraining**: Monthly with new data
4. **A/B Testing**: Test new models before production deployment
5. **Alert Fatigue**: Tune threshold to balance detection vs. false alarms
6. **Integration**: Combine with auto-recovery for automated response
7. **Data Quality**: Ensure metrics are accurate and timely

## Security Considerations

- Model files should be validated (checksums)
- Access control on model updates
- Audit log for predictions and alerts
- Protect sensitive metrics data

## Future Enhancements

- Multi-horizon predictions (7, 14, 30 days)
- Confidence intervals for predictions
- Explainability (SHAP values for feature importance)
- Real-time SMART data integration
- Anomaly detection for unusual patterns
- Transfer learning from similar deployments

## References

- [Feature Proposals Document](../../docs/features/FEATURE_PROPOSALS.md)
- [Auto-Recovery Manager](auto_recovery_manager.h)
- [Backblaze Hard Drive Stats](https://www.backblaze.com/b2/hard-drive-test-data.html)
- [Google: Failure Trends in Large Disk Drive Population](https://research.google/pubs/pub32774/)

## Testing

See `tests/test_predictive_detector.cpp` for comprehensive test suite covering:

- Configuration and lifecycle
- Metrics collection and history management
- Prediction accuracy
- Alert generation
- Statistics tracking
- Concurrent access
- Edge cases

## License

Same as ThemisDB (MIT License)
