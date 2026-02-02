# Performance Regression Detection - Quick Reference

## Quick Commands

### For Developers

```bash
# Check if your changes cause regressions
python benchmarks/performance_regression_detector.py \
  --baseline benchmarks/baselines/main/latest.json \
  --current my_results.json \
  --output my_report.txt

# View current baselines
python benchmarks/baseline_manager.py list

# Load specific baseline
python benchmarks/baseline_manager.py load --branch main
```

### For CI/CD

```bash
# Trigger baseline update (maintainers only)
gh workflow run update-performance-baselines.yml -f branch=main

# View PR performance check results
gh run list --workflow="Performance Regression Check"
gh run view <run-id>
```

### For Monitoring

```bash
# Export metrics to Prometheus
python benchmarks/metrics_exporter.py \
  --baseline benchmarks/baselines/main/latest.json \
  --regression-report regression_report.json \
  --output metrics.prom

# Push to Prometheus Pushgateway
python benchmarks/metrics_exporter.py \
  --baseline benchmarks/baselines/main/latest.json \
  --pushgateway http://localhost:9091
```

## Thresholds

| Level    | Threshold | Action                    |
|----------|-----------|---------------------------|
| Minor    | 5-10%     | Warning only              |
| Major    | 10-20%    | **Blocks PR** (default)   |
| Critical | >20%      | **Blocks PR** + alert     |

## Files & Locations

```
benchmarks/
├── baselines/                          # Baseline storage
│   ├── main/latest.json               # Main branch baseline
│   ├── develop/latest.json            # Develop branch baseline
│   └── releases/v*.json               # Release baselines
├── baseline_manager.py                 # Baseline CRUD operations
├── performance_regression_detector.py  # Regression detection
└── metrics_exporter.py                # Prometheus export

.github/workflows/
├── performance-regression-check.yml    # PR checks (runs on PRs)
└── update-performance-baselines.yml    # Baseline updates (runs on push)

benchmarks/monitoring/
└── performance_regression_dashboard.json  # Grafana dashboard
```

## Workflow Triggers

### Performance Regression Check
- **Trigger**: PRs to main/develop
- **Runs**: Core benchmark subset (~15 min)
- **Action**: Posts comment + blocks if regressions found

### Update Performance Baselines
- **Trigger**: Push to main/develop, release tags
- **Runs**: Full benchmark suite (~45 min)
- **Action**: Creates/updates baseline, commits to repo

## Common Scenarios

### Scenario 1: PR Blocked by Regression

1. Review regression report in PR comment
2. Identify affected benchmarks
3. Options:
   - Fix the performance issue
   - Justify regression in PR description
   - Request threshold adjustment (if unavoidable)

### Scenario 2: Need to Update Baseline

```bash
# Manual workflow trigger
gh workflow run update-performance-baselines.yml -f branch=main

# Or run locally and commit
python benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version $(cat VERSION) \
  --commit $(git rev-parse HEAD)

git add benchmarks/baselines/
git commit -m "chore: Update performance baseline"
git push
```

### Scenario 3: False Positive Regression

1. Check if benchmark is unstable (high variance)
2. Rerun workflow to verify
3. If persistent false positive:
   - Increase threshold for that benchmark
   - Add more iterations/repetitions
   - Exclude from regression checks

### Scenario 4: Monitoring Performance Trends

1. Access Grafana dashboard
2. Select time range and benchmarks
3. Compare across releases
4. Investigate anomalies

## Exit Codes

| Code | Meaning                              |
|------|--------------------------------------|
| 0    | No blocking regressions              |
| 1    | Blocking regressions detected        |

## Environment Variables

```bash
# Custom threshold for CI
export REGRESSION_THRESHOLD_MAJOR=15.0  # Default: 10.0

# Pushgateway URL
export PROMETHEUS_PUSHGATEWAY="http://prometheus:9091"
```

## Support

- 📖 Full Documentation: `docs/PERFORMANCE_REGRESSION_DETECTION.md`
- 🐛 Issues: Label with `performance`
- 💬 Questions: Open discussion with `performance` label

## Metrics Reference

### Prometheus Metrics

```
# Benchmark performance
themisdb_benchmark_items_per_second{benchmark_name, version, branch}
themisdb_benchmark_bytes_per_second{benchmark_name, version, branch}
themisdb_benchmark_cpu_time_ms{benchmark_name, version, branch}

# Regression tracking
themisdb_regression_count{severity}
themisdb_improvement_count
themisdb_pr_blocked_total
themisdb_benchmark_regression_pct{benchmark_name, severity}
```

### Dashboard Panels

1. Performance Trend - Items per Second
2. Regression Count by Severity
3. Top 10 Regressed Benchmarks
4. Benchmark Execution Time Trend
5. Release Comparison
6. Improvement Count
7. PR Blocks Due to Regressions
8. Memory Usage Trend

## Tips

✅ **DO:**
- Run benchmarks in Release mode
- Use multiple repetitions for stability
- Document justified regressions
- Monitor trends regularly

❌ **DON'T:**
- Commit baseline changes in feature branches
- Ignore minor regressions (they add up)
- Skip regression checks for "small" changes
- Adjust thresholds without discussion

---

Last updated: 2024-12-30
Version: 1.0
