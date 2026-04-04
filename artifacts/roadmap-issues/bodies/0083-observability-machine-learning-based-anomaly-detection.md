### Context

This issue implements the roadmap item 'ML-Based Anomaly Detection' for the observability domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Machine Learning-Based Anomaly Detection

### Goal

Deliver the scoped changes for ML-Based Anomaly Detection in src/observability/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Machine Learning-Based Anomaly Detection
**Priority:** High  
**Target Version:** v1.7.0

Automated detection of performance anomalies using ML models.

**Features:**
- Time-series forecasting (ARIMA, Prophet)
- Outlier detection (Isolation Forest, DBSCAN)
- Seasonal pattern recognition
- Change point detection

**Implementation:**
```cpp
class MLAnomalyDetector {
public:
    explicit MLAnomalyDetector(const MLConfig& config);
    
    // Train model on historical data
    void train(const std::vector<TimeSeries>& training_data);
    
    // Detect anomalies in real-time
    std::vector<Anomaly> detectAnomalies(const TimeSeries& current_data);
    
    // Predict future values
    TimeSeries forecast(std::chrono::hours horizon);
    
    // Explain anomaly (feature importance)
    AnomalyExplanation explainAnomaly(const Anomaly& anomaly);
};

struct Anomaly {
    std::chrono::system_clock::time_point timestamp;
    std::string metric_name;
    double actual_value;
    double expected_value;
    double confidence_score;  // 0-1
    std::string severity;     // low, medium, high, critical
    std::vector<std::string> contributing_factors;
};

// Example usage
MLAnomalyDetector detector(config);
detector.train(historical_query_latencies);

auto anomalies = detector.detectAnomalies(current_query_latencies);
for (const auto& anomaly : anomalies) {
    if (anomaly.confidence_score > 0.8) {
        alertmanager.sendAlert({
            .alert_name = "MLAnomalyDetected",
            .severity = AlertSeverity::WARNING,
            .message = "Unusual pattern detected: " + anomaly.metric_name,
            .annotations = {
                {"expected", std::to_string(anomaly.expected_value)},
                {"actual", std::to_string(anomaly.actual_value)},
                {"confidence", std::to_string(anomaly.confidence_score)}
            }
        });
    }
}
```

---

### Acceptance Criteria

- [ ] Time-series forecasting (ARIMA, Prophet)
- [ ] Outlier detection (Isolation Forest, DBSCAN)
- [ ] Seasonal pattern recognition
- [ ] Change point detection

### Relationships

- Roadmap row: #83 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#machine-learning-based-anomaly-detection
- Source key: roadmap:83:observability:v1.7.0:machine-learning-based-anomaly-detection

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:83:observability:v1.7.0:machine-learning-based-anomaly-detection -->
<!-- roadmap-ref: row=83;module=observability;target=v1.7.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#machine-learning-based-anomaly-detection -->
