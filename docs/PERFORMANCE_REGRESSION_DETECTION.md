# Performance Regression Detection & Baseline Management

## Overview

ThemisDB implements an automated performance regression detection system that:

- 📊 Stores baseline benchmark reports per branch and release
- 🚨 Automatically detects and blocks PRs with 10%+ performance regressions
- 📈 Provides dashboards for tracking performance trends across releases
- 🔔 Sends alerts for performance violations
- 📝 Maintains comprehensive documentation of thresholds and processes

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Pull Request Workflow                     │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  1. PR opened/updated                                         │
│  2. GitHub Actions triggers benchmark run                     │
│  3. Compares results against baseline                         │
│  4. Generates regression report                               │
│  5. Posts comment on PR with results                          │
│  6. Blocks merge if regressions exceed threshold              │
│                                                               │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                  Baseline Update Workflow                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  1. Push to main/develop or release tag created               │
│  2. Runs comprehensive benchmark suite                        │
│  3. Creates/updates baseline in benchmarks/baselines/         │
│  4. Commits baseline back to repository                       │
│  5. Exports metrics to Prometheus/Grafana                     │
│                                                               │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│              Monitoring & Alerting System                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  • Grafana dashboard shows performance trends                 │
│  • Prometheus metrics track regression counts                 │
│  • Alerts trigger on threshold violations                     │
│  • Historical data enables release comparisons                │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## Components

### 1. Baseline Storage (`benchmarks/baselines/`)

Baselines are stored as JSON files with the following structure:

```
benchmarks/baselines/
├── main/
│   └── latest.json          # Latest baseline from main branch
├── develop/
│   └── latest.json          # Latest baseline from develop branch
└── releases/
    ├── v1.4.0.json         # Release baseline
    └── v1.4.1.json         # Release baseline
```

Each baseline file contains:

```json
{
  "version": "1.4.1",
  "branch": "main",
  "commit": "abc123def456",
  "timestamp": "2024-12-30T10:00:00Z",
  "benchmarks": {
    "BenchmarkName": {
      "real_time": 1000.0,
      "cpu_time": 950.0,
      "iterations": 10000,
      "items_per_second": 50000.0,
      "bytes_per_second": 100000.0
    }
  }
}
```

### 2. Baseline Manager (`baseline_manager.py`)

Python script for managing baselines:

```bash
# Save a new baseline
python benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version 1.4.1 \
  --commit abc123 \
  --release

# Load a baseline
python benchmarks/baseline_manager.py load --branch main
python benchmarks/baseline_manager.py load --version 1.4.0

# List all baselines
python benchmarks/baseline_manager.py list
```

### 3. Regression Detector (`performance_regression_detector.py`)

Compares benchmark results against baselines and detects regressions:

```bash
python benchmarks/performance_regression_detector.py \
  --baseline benchmarks/baselines/main/latest.json \
  --current build/benchmark_results \
  --output regression_report.txt \
  --fail-on major \
  --threshold-minor 5.0 \
  --threshold-major 10.0 \
  --threshold-critical 20.0
```

**Severity Levels:**

- **Minor**: 5-10% performance change
- **Major**: 10-20% performance change (blocks PRs by default)
- **Critical**: >20% performance change

### 4. GitHub Actions Workflows

#### Performance Regression Check (`.github/workflows/performance-regression-check.yml`)

Triggers on PRs to main/develop:

1. Builds ThemisDB in Release mode
2. Runs core benchmark suite (faster subset for PR checks)
3. Compares against appropriate baseline (main or develop)
4. Posts detailed report as PR comment
5. Blocks PR merge if regressions exceed threshold

**Configuration:**
- Default threshold: `major` (10%)
- Can be overridden via workflow dispatch
- Runs on code changes in `src/`, `include/`, benchmarks

#### Update Performance Baselines (`.github/workflows/update-performance-baselines.yml`)

Triggers on:
- Push to main branch
- Push to develop branch
- Release tags (v*)
- Manual workflow dispatch

Actions:
1. Runs comprehensive benchmark suite
2. Creates baseline using `baseline_manager.py`
3. Commits baseline back to repository
4. Uploads artifacts for historical tracking

### 5. Monitoring Dashboard

Grafana dashboard configuration: `benchmarks/monitoring/performance_regression_dashboard.json`

**Key Visualizations:**

- Performance trends over time (items/sec, latency)
- Regression counts by severity
- Top 10 regressed benchmarks
- Release-to-release comparisons
- PR block statistics

**Metrics Exported:**

```
themisdb_benchmark_items_per_second{benchmark_name, version, branch}
themisdb_benchmark_bytes_per_second{benchmark_name, version, branch}
themisdb_benchmark_cpu_time_ms{benchmark_name, version, branch}
themisdb_regression_count{severity}
themisdb_improvement_count
themisdb_pr_blocked_total
```

### 6. Metrics Exporter (`metrics_exporter.py`)

Exports metrics to Prometheus format:

```bash
# Export to file
python benchmarks/metrics_exporter.py \
  --baseline benchmarks/baselines/main/latest.json \
  --regression-report regression_report.json \
  --output benchmark_metrics.prom

# Push to Prometheus Pushgateway
python benchmarks/metrics_exporter.py \
  --baseline benchmarks/baselines/main/latest.json \
  --pushgateway http://localhost:9091
```

## Thresholds Configuration

### Default Thresholds

```python
THRESHOLDS = {
    'minor': 5.0,      # 5% change
    'major': 10.0,     # 10% change (blocks PRs)
    'critical': 20.0   # 20% change
}
```

### Customizing Thresholds

#### For PR Checks:

Edit `.github/workflows/performance-regression-check.yml`:

```yaml
- name: Detect Regressions
  run: |
    python3 benchmarks/performance_regression_detector.py \
      --threshold-minor 5.0 \
      --threshold-major 10.0 \    # Change this
      --threshold-critical 20.0 \
      --fail-on major               # Or change this to 'critical'
```

#### For Grafana Alerts:

Edit `benchmarks/monitoring/performance_regression_dashboard.json`:

```json
"alert": {
  "conditions": [
    {
      "evaluator": {
        "params": [-10],  // Change threshold here
        "type": "lt"
      }
    }
  ]
}
```

## Usage Guide

### For Developers

#### Viewing Regression Results

1. Open your PR on GitHub
2. Wait for "Performance Regression Check" workflow to complete
3. Review the automated comment with regression report
4. Check the detailed artifacts for full data

#### Addressing Regressions

If your PR is blocked due to regressions:

1. Review the regression report to identify affected benchmarks
2. Investigate the code changes that may have caused the regression
3. Options:
   - Fix the performance issue
   - Justify the regression if it's an acceptable trade-off
   - Request threshold adjustment if the regression is unavoidable

#### Manual Baseline Comparison

```bash
# Build your code
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --config Release

# Run benchmarks
cd build
./bench_crud --benchmark_format=json --benchmark_out=my_results.json

# Compare against baseline
python ../benchmarks/performance_regression_detector.py \
  --baseline ../benchmarks/baselines/main/latest.json \
  --current my_results.json \
  --output my_regression_report.txt
```

### For Maintainers

#### Creating Initial Baseline

```bash
# Run benchmarks
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --config Release
cd build
for bench in bench_*; do
  ./$bench --benchmark_format=json --benchmark_out=${bench}.json
done

# Create baseline
cd ..
python benchmarks/baseline_manager.py save \
  --results build \
  --branch main \
  --version $(cat VERSION) \
  --commit $(git rev-parse HEAD)

# Commit baseline
git add benchmarks/baselines/
git commit -m "chore: Add initial performance baseline"
git push
```

#### Updating Baselines Manually

```bash
# Trigger workflow dispatch
gh workflow run update-performance-baselines.yml -f branch=main
```

#### Reviewing Historical Trends

1. Access Grafana dashboard at configured URL
2. Select time range and release versions
3. Compare performance across releases
4. Investigate any anomalies

### For Release Managers

#### Before Release

1. Check Grafana dashboard for performance trends
2. Review any outstanding regression issues
3. Ensure all baselines are up-to-date

#### During Release

Baseline is automatically created when release tag is pushed:

```bash
git tag -a v1.4.1 -m "Release v1.4.1"
git push origin v1.4.1
```

The workflow will:
1. Run comprehensive benchmarks
2. Create `benchmarks/baselines/releases/v1.4.1.json`
3. Commit and push the baseline

#### After Release

1. Verify baseline was created correctly
2. Check Grafana dashboard for new release metrics
3. Document any significant performance changes in release notes

## Alerting

### Grafana Alerts

Alerts are configured in the dashboard for:

- **Critical Regressions**: >20% performance drop
- **Major Regressions**: >10% performance drop
- **High PR Block Rate**: More than 10 PRs blocked in 7 days

### Notification Channels

Configure in Grafana:

1. Go to Alerting → Notification channels
2. Add channels (Slack, email, PagerDuty, etc.)
3. Link channels to dashboard alerts

### GitHub Actions Notifications

For workflow failures:

```yaml
- name: Notify on failure
  if: failure()
  uses: slackapi/slack-github-action@v1
  with:
    webhook-url: ${{ secrets.SLACK_WEBHOOK_BENCHMARKS }}
```

## Troubleshooting

### PR Check Not Running

**Cause**: Workflow triggers don't match changed files

**Solution**: Check `.github/workflows/performance-regression-check.yml` paths configuration

### No Baseline Available

**Cause**: First run or baseline not committed

**Solution**: 
1. Run baseline update workflow manually
2. Or create baseline using `baseline_manager.py`
3. Commit to repository

### False Positive Regressions

**Cause**: Benchmark noise, system load, or timing variations

**Solutions**:
- Increase benchmark repetitions: `--benchmark_repetitions=5`
- Run on dedicated CI hardware
- Adjust thresholds if consistently noisy
- Filter out unstable benchmarks

### Dashboard Not Showing Data

**Cause**: Metrics not being exported or Prometheus not scraping

**Solutions**:
1. Check metrics are being exported: `cat benchmark_metrics.prom`
2. Verify Pushgateway is accessible
3. Check Prometheus scrape configuration
4. Verify Grafana data source configuration

### Baseline Conflicts

**Cause**: Multiple simultaneous baseline updates

**Solution**: Baselines are automatically serialized by Git, resolve conflicts manually if needed

## Best Practices

### 1. Stable Benchmarks

- Use sufficient iterations for stable results
- Disable turbo boost and dynamic frequency scaling on CI
- Use dedicated benchmark runners if possible
- Warm up caches before timing measurements

### 2. Meaningful Baselines

- Update baselines regularly (after merges to main)
- Create baselines for all releases
- Keep at least last 5-10 release baselines
- Document significant changes in baseline updates

### 3. Regression Investigation

- Always investigate regressions before merging
- Document justified regressions in PR description
- Link to related issues or design decisions
- Consider if regression is worth the feature/fix

### 4. Dashboard Monitoring

- Review dashboard weekly
- Set up alerts for critical regressions
- Track trends across releases
- Investigate unexpected spikes

### 5. Threshold Management

- Start conservative (10% major threshold)
- Adjust based on benchmark stability
- Document threshold changes
- Different thresholds for different benchmark types

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Grafana Dashboard Best Practices](https://grafana.com/docs/grafana/latest/dashboards/best-practices/)
- [Prometheus Metrics Types](https://prometheus.io/docs/concepts/metric_types/)
- [GitHub Actions Workflow Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)

## Support

For issues or questions:

1. Check [Troubleshooting](#troubleshooting) section
2. Review workflow logs in GitHub Actions
3. Check Grafana alerts and metrics
4. Open an issue with `performance` label

---

*Last updated: 2024-12-30*
