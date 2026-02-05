---
name: Performance-Regression-Detection
about: Automatische Detection von Performance-Regressionen durch historische Vergleiche
title: '[MONITORING] Performance-Regression-Detection'
labels: ['enhancement', 'future', 'content-pipeline', 'monitoring', 'performance', 'priority:low']
assignees: ''
---

## Beschreibung

Implementierung eines automatisierten Systems zur Detection von Performance-Regressionen durch statistische Vergleiche mit historischen Baseline-Metriken, mit Alerting und Trend-Reports.

## Kontext

Performance-Regressionen sind oft subtil und schwer zu erkennen. Ein automatisiertes System kann Verschlechterungen frühzeitig detektieren, bevor sie zu Production-Problemen werden.

## Ziele

- Baseline-Metriken-Speicherung für Performance-KPIs
- Statistische Vergleichsmethoden (t-test, z-test)
- Automatisches Regression-Alerting
- Performance-Trend-Reports
- Integration in CI/CD-Pipeline (optional)

## Lösungsansatz

### Schritt 1: Baseline-Metriken-Speicherung (1 Tag)
- **Tasks**:
  - [ ] Performance-Metriken-Definition:
    - Upload-Latenz (p50, p95, p99)
    - Throughput (uploads/sec, MB/sec)
    - Compression-Latenz
    - Storage-Latenz
    - End-to-End-Latenz
  - [ ] Baseline-Snapshot-Mechanismus
  - [ ] Time-Series-Storage für Historicals
  - [ ] Retention-Policy (z.B. 90 Tage)
  - [ ] Baseline-Update-Strategie (rolling window)

### Schritt 2: Statistischer Vergleich (1-1.5 Tage)
- **Tasks**:
  - [ ] Statistische Test-Implementierung:
    - Student's t-test für Mittelwert-Vergleiche
    - Mann-Whitney U-test für nicht-normale Verteilungen
    - Z-Score für Ausreißer-Detection
  - [ ] Signifikanz-Level-Konfiguration (z.B. p < 0.05)
  - [ ] Change-Percentage-Berechnung
  - [ ] Confidence-Interval-Berechnung

### Schritt 3: Regression-Alerting (0.5-1 Tag)
- **Tasks**:
  - [ ] Alert: Latenz-Regression (> 20% increase, p < 0.05)
  - [ ] Alert: Throughput-Regression (> 15% decrease, p < 0.05)
  - [ ] Alert: Error-Rate-Increase (> 50% increase)
  - [ ] Alert-Deduplication (avoid alert-spam)
  - [ ] Alert-Context (affected metrics, change %)
  - [ ] Integration mit Slack/Email

### Schritt 4: Performance-Trend-Reports (0.5-1 Tag)
- **Tasks**:
  - [ ] Daily/Weekly Performance-Reports
  - [ ] Trend-Visualisierung (improving/degrading/stable)
  - [ ] Root-Cause-Hints (correlated changes)
  - [ ] Historical-Comparison-Charts
  - [ ] Export als PDF/HTML/JSON

## Performance-Regression-Detection

```cpp
class PerformanceRegressionDetector {
public:
    struct Baseline {
        std::string metric_name;
        std::chrono::system_clock::time_point timestamp;
        std::vector<double> samples;  // Historical samples
        
        // Statistical properties
        double mean;
        double stddev;
        double p50;
        double p95;
        double p99;
    };
    
    struct RegressionResult {
        std::string metric_name;
        bool regression_detected;
        double current_value;
        double baseline_value;
        double change_percentage;
        double p_value;  // Statistical significance
        double z_score;
        std::string severity;  // "minor", "moderate", "severe"
    };
    
    // Update baseline with new samples
    void UpdateBaseline(const std::string& metric_name,
                       const std::vector<double>& new_samples) {
        auto& baseline = baselines_[metric_name];
        
        // Rolling window (e.g., last 30 days)
        baseline.samples.insert(baseline.samples.end(),
                               new_samples.begin(), new_samples.end());
        
        // Keep only recent samples
        if (baseline.samples.size() > max_samples_) {
            baseline.samples.erase(
                baseline.samples.begin(),
                baseline.samples.begin() + 
                    (baseline.samples.size() - max_samples_));
        }
        
        // Recalculate statistics
        baseline.mean = CalculateMean(baseline.samples);
        baseline.stddev = CalculateStdDev(baseline.samples);
        baseline.p50 = CalculatePercentile(baseline.samples, 50);
        baseline.p95 = CalculatePercentile(baseline.samples, 95);
        baseline.p99 = CalculatePercentile(baseline.samples, 99);
        baseline.timestamp = std::chrono::system_clock::now();
    }
    
    // Check for regression
    RegressionResult CheckRegression(
        const std::string& metric_name,
        const std::vector<double>& current_samples) {
        
        RegressionResult result;
        result.metric_name = metric_name;
        
        auto baseline = baselines_[metric_name];
        
        // Current statistics
        double current_mean = CalculateMean(current_samples);
        result.current_value = current_mean;
        result.baseline_value = baseline.mean;
        
        // Calculate change percentage
        result.change_percentage = 
            100.0 * (current_mean - baseline.mean) / baseline.mean;
        
        // Statistical test (t-test)
        result.p_value = TTest(current_samples, baseline.samples);
        
        // Z-score for severity
        result.z_score = (current_mean - baseline.mean) / baseline.stddev;
        
        // Regression detection
        bool statistically_significant = result.p_value < significance_level_;
        bool meaningful_change = std::abs(result.change_percentage) > 
                                min_change_threshold_;
        
        result.regression_detected = statistically_significant && 
                                     meaningful_change;
        
        // Severity classification
        if (std::abs(result.z_score) > 3.0) {
            result.severity = "severe";
        } else if (std::abs(result.z_score) > 2.0) {
            result.severity = "moderate";
        } else {
            result.severity = "minor";
        }
        
        return result;
    }
    
private:
    std::map<std::string, Baseline> baselines_;
    double significance_level_ = 0.05;  // p < 0.05
    double min_change_threshold_ = 10.0;  // 10% change
    size_t max_samples_ = 10000;
    
    double TTest(const std::vector<double>& sample1,
                const std::vector<double>& sample2) {
        // Welch's t-test implementation
        double mean1 = CalculateMean(sample1);
        double mean2 = CalculateMean(sample2);
        double var1 = CalculateVariance(sample1);
        double var2 = CalculateVariance(sample2);
        size_t n1 = sample1.size();
        size_t n2 = sample2.size();
        
        double t = (mean1 - mean2) / 
                   std::sqrt(var1/n1 + var2/n2);
        
        // Degrees of freedom (Welch-Satterthwaite)
        double df = std::pow(var1/n1 + var2/n2, 2) /
                   (std::pow(var1/n1, 2)/(n1-1) + 
                    std::pow(var2/n2, 2)/(n2-1));
        
        // Convert t-statistic to p-value
        return TDistributionCDF(std::abs(t), df);
    }
};

// Integration in Monitoring System
class PerformanceMonitor {
public:
    void CollectMetrics() {
        // Collect current metrics
        std::vector<double> latency_samples = 
            CollectLatencySamples(std::chrono::hours(1));
        std::vector<double> throughput_samples = 
            CollectThroughputSamples(std::chrono::hours(1));
        
        // Check for regressions
        auto latency_result = 
            detector_.CheckRegression("upload_latency_p95", latency_samples);
        auto throughput_result = 
            detector_.CheckRegression("upload_throughput", throughput_samples);
        
        // Alert if regression detected
        if (latency_result.regression_detected) {
            SendRegressionAlert(latency_result);
        }
        if (throughput_result.regression_detected) {
            SendRegressionAlert(throughput_result);
        }
        
        // Update baselines
        detector_.UpdateBaseline("upload_latency_p95", latency_samples);
        detector_.UpdateBaseline("upload_throughput", throughput_samples);
    }

private:
    PerformanceRegressionDetector detector_;
};
```

## Regression-Alert-Beispiel

```json
{
  "alert_type": "performance_regression",
  "timestamp": "2026-02-05T16:30:00Z",
  "metric_name": "upload_latency_p95",
  "severity": "moderate",
  "regression_detected": true,
  "details": {
    "current_value": 1.85,
    "baseline_value": 1.45,
    "change_percentage": 27.6,
    "p_value": 0.003,
    "z_score": 2.5,
    "statistical_significance": "significant (p < 0.05)",
    "confidence": "95%"
  },
  "context": {
    "measurement_period": "last_1_hour",
    "baseline_period": "last_30_days",
    "sample_count_current": 3600,
    "sample_count_baseline": 108000
  },
  "recommendation": "Investigate recent changes to upload pipeline"
}
```

## Performance-Trend-Report

```
================================================================================
                   Performance Trend Report
                      2026-02-05
================================================================================

Summary:
- Total Metrics Tracked: 12
- Regressions Detected: 2
- Improvements Detected: 3
- Stable Metrics: 7

================================================================================
REGRESSIONS DETECTED
================================================================================

1. Upload Latency (p95)
   Current:    1.85s
   Baseline:   1.45s
   Change:     +27.6% ⚠️
   Severity:   MODERATE
   P-Value:    0.003 (significant)
   Z-Score:    2.5
   
   Trend: [30-day chart]
   1.2s ┤                                    ╭──╮
   1.4s ┤                      ╭─────────────╯  │
   1.6s ┤                      │                │
   1.8s ┤╭─────────────────────╯                ╰─
   
   Recommendation: Investigate changes in last 24h

2. Compression Throughput
   Current:    72 MB/s
   Baseline:   85 MB/s
   Change:     -15.3% ⚠️
   Severity:   MINOR
   P-Value:    0.042 (significant)
   Z-Score:    -1.8
   
   Recommendation: Check compression settings changes

================================================================================
IMPROVEMENTS
================================================================================

1. Chunking Latency
   Current:    85ms
   Baseline:   110ms
   Change:     -22.7% ✓
   
2. Storage Write Throughput
   Current:    450 MB/s
   Baseline:   380 MB/s
   Change:     +18.4% ✓

================================================================================
STABLE METRICS
================================================================================

- Upload Success Rate: 99.2% (stable)
- Error Rate: 0.8% (stable)
- Memory Usage: 2.1 GB (stable)
- ... (4 more)

================================================================================
```

## Alerting-Rules

```yaml
# alerts/performance_regression.yml
groups:
  - name: performance_regression_alerts
    interval: 1h
    rules:
      - alert: LatencyRegression
        expr: |
          performance_regression_detected{metric="upload_latency_p95"} == 1
        labels:
          severity: warning
          component: content-pipeline
        annotations:
          summary: "Performance regression in upload latency"
          description: |
            Upload latency (p95) regressed by {{ $labels.change_percentage }}%
            Current: {{ $labels.current_value }}s
            Baseline: {{ $labels.baseline_value }}s
            P-Value: {{ $labels.p_value }}

      - alert: ThroughputRegression
        expr: |
          performance_regression_detected{metric="upload_throughput"} == 1
        labels:
          severity: warning
          component: content-pipeline
        annotations:
          summary: "Performance regression in upload throughput"
          description: |
            Upload throughput regressed by {{ $labels.change_percentage }}%
            Current: {{ $labels.current_value }} uploads/s
            Baseline: {{ $labels.baseline_value }} uploads/s

      - alert: SeverePerformanceRegression
        expr: |
          performance_regression_detected{severity="severe"} == 1
        labels:
          severity: critical
          component: content-pipeline
        annotations:
          summary: "Severe performance regression detected"
          description: |
            Severe regression in {{ $labels.metric }}
            Z-Score: {{ $labels.z_score }}
            Change: {{ $labels.change_percentage }}%
```

## Konfiguration

```yaml
# config/regression_detection.yml
regression_detection:
  enabled: true
  
  metrics:
    - name: upload_latency_p50
      type: latency
      unit: seconds
      direction: lower_is_better
    
    - name: upload_latency_p95
      type: latency
      unit: seconds
      direction: lower_is_better
    
    - name: upload_throughput
      type: throughput
      unit: uploads_per_second
      direction: higher_is_better
    
    - name: compression_ratio
      type: efficiency
      unit: ratio
      direction: higher_is_better
  
  baseline:
    window_days: 30  # 30-day rolling baseline
    min_samples: 1000
    update_interval: 1h
  
  detection:
    significance_level: 0.05  # p < 0.05
    min_change_threshold: 10.0  # 10%
    check_interval: 1h
  
  alerting:
    enabled: true
    channels:
      slack:
        enabled: true
        webhook: "${SLACK_WEBHOOK}"
        channel: "#performance-alerts"
      
      email:
        enabled: true
        recipients:
          - performance-team@example.com
  
  reporting:
    enabled: true
    schedule: "0 9 * * MON"  # Monday 9 AM
    format: html
```

## CI/CD-Integration (Optional)

```yaml
# .github/workflows/performance-regression.yml
name: Performance Regression Check

on:
  pull_request:
    branches: [main]

jobs:
  performance-regression:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Run Performance Benchmarks
        run: |
          ./scripts/run_benchmarks.sh
      
      - name: Check for Regressions
        run: |
          ./scripts/check_regression.sh \
            --baseline-file benchmarks/baseline.json \
            --current-file benchmarks/current.json \
            --threshold 10
      
      - name: Comment on PR
        if: failure()
        uses: actions/github-script@v6
        with:
          script: |
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: '⚠️ Performance regression detected. See workflow for details.'
            })
```

## Integration Points

- [ ] PerformanceMonitor (Metriken-Collection)
- [ ] Prometheus (Metriken-Source)
- [ ] Time-Series-Database (Baseline-Storage)
- [ ] Alerting-System (Notifications)
- [ ] Reporting-System (Trend-Reports)
- [ ] CI/CD-Pipeline (optional)

## Testing-Anforderungen

### Unit-Tests
```cpp
TEST(RegressionDetector, TTest) {
    // Test t-test calculation
}

TEST(RegressionDetector, RegressionDetection) {
    // Test regression detection logic
}

TEST(RegressionDetector, BaselineUpdate) {
    // Test baseline update mechanism
}

TEST(RegressionDetector, SeverityClassification) {
    // Test severity classification
}
```

### Integration-Tests
- [ ] Baseline-Speicherung funktioniert
- [ ] Regression wird korrekt detektiert
- [ ] Alerts werden versendet
- [ ] Reports werden generiert
- [ ] Statistischer Test ist korrekt

## Success Criteria

- [ ] Baseline-Metriken-System implementiert
- [ ] Statistische Tests funktionieren
- [ ] Regression-Detection ist zuverlässig
- [ ] Alerting funktioniert
- [ ] Trend-Reports werden generiert
- [ ] False-Positive-Rate < 5%
- [ ] Detection-Delay < 1 Stunde
- [ ] Unit- und Integration-Tests bestehen
- [ ] Dokumentation vollständig

## Priorität

**Niedrig** - Nützlich für Qualitätssicherung, aber nicht kritisch

## Geschätzter Aufwand

**3-4 Tage**

## Dependencies

- **Benötigt**: Prometheus-Metriken (als Datenquelle)
- **Benötigt**: Time-Series-Database (für Baseline-Storage)
- **Optional**: Grafana (für Visualisierung)
- **Optional**: CI/CD-Integration
- **Related**: Performance-Dashboard

## Referenzen

- [ ] Statistical Testing: https://en.wikipedia.org/wiki/Student%27s_t-test
- [ ] Performance Testing Best Practices
- [ ] Continuous Performance Testing: https://martinfowler.com/articles/performance-testing.html
- [ ] ThemisDB Performance-Testing: `docs/testing/performance_testing.md`

---

**Checklist:**
- [ ] Ich habe die Anforderungen verstanden
- [ ] Ich habe statistische Methoden ausgewählt
- [ ] Ich habe Baseline-Strategie definiert
- [ ] Ich habe Alerting-Kriterien festgelegt
- [ ] Ich habe Report-Format spezifiziert
- [ ] Ich habe Testing-Anforderungen definiert
