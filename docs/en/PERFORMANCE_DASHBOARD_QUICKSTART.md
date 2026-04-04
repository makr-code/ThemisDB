# Performance Dashboard - Quick Start Guide

Get the ThemisDB Performance Dashboard up and running in 5 minutes!

## Prerequisites

- Docker & Docker Compose installed
- ThemisDB repository cloned
- (Optional) ThemisDB built with benchmarks

## Step 1: Start Dashboard Stack (2 minutes)

```bash
# Navigate to grafana directory
cd grafana

# Start Grafana + Prometheus
docker-compose up -d

# Verify services are running
docker-compose ps
```

Expected output:
```
NAME                COMMAND                  STATUS    PORTS
grafana             "/run.sh"                Up        0.0.0.0:3000->3000/tcp
prometheus          "/bin/prometheus --c…"   Up        0.0.0.0:9090->9090/tcp
```

## Step 2: Access Dashboard (30 seconds)

1. Open browser: http://localhost:3000
2. Login: `admin` / `admin`
3. (Optional) Change password or skip
4. Navigate to: **Dashboards** → **ThemisDB Performance Dashboard**

That's it! 🎉 Dashboard is ready.

## Step 3: Run Benchmarks (2 minutes)

```bash
# Option A: Quick test with example data
cd benchmarks
python3 performance_tracker.py \
  --results examples/sample_results.json \
  --storage performance_data

# Option B: Run actual benchmarks
cd build
./bench_crud --benchmark_format=json --benchmark_out=results.json
cd ..
python3 benchmarks/performance_tracker.py \
  --results build/results.json \
  --storage benchmarks/performance_data
```

## Step 4: View Results (1 minute)

Refresh the Grafana dashboard. You should see:
- ✅ Throughput metrics populated
- ✅ Latency charts showing data
- ✅ No critical regressions (first run)

---

## Quick Commands Reference

### Start/Stop Dashboard

```bash
# Start
cd grafana && docker-compose up -d

# Stop
docker-compose down

# Restart
docker-compose restart

# View logs
docker-compose logs -f grafana
```

### Track Benchmarks

```bash
# Track results
python3 benchmarks/performance_tracker.py \
  --results <path-to-results> \
  --storage benchmarks/performance_data \
  --branch $(git branch --show-current) \
  --commit $(git rev-parse --short HEAD)
```

### Manage Baselines

```bash
# Save baseline
python3 benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version $(cat VERSION) \
  --commit $(git rev-parse HEAD)

# List baselines
python3 benchmarks/baseline_manager.py list

# Load baseline
python3 benchmarks/baseline_manager.py load --branch main
```

### Detect Regressions

```bash
# Compare with baseline
python3 benchmarks/performance_regression_detector.py \
  --baseline benchmarks/baselines/main/latest.json \
  --current build/results.json \
  --output report.txt
```

---

## Troubleshooting

### Dashboard shows "No Data"

**Check 1:** Is Prometheus running?
```bash
curl http://localhost:9090/-/healthy
# Should return: Prometheus is Healthy.
```

**Check 2:** Are metrics being generated?
```bash
ls -la benchmarks/performance_data/metrics/
# Should see: benchmarks.prom
```

**Check 3:** Prometheus datasource configured?
- Go to Grafana → Configuration → Data Sources
- Click "Prometheus"
- Click "Save & Test"
- Should see: "Data source is working"

**Fix:** If still not working:
```bash
# Restart services
cd grafana
docker-compose restart

# Check Prometheus config
docker-compose exec prometheus cat /etc/prometheus/prometheus.yml
```

### Can't login to Grafana

**Default credentials:**
- Username: `admin`
- Password: `admin`

**If locked out:**
```bash
# Reset admin password
docker-compose exec grafana grafana-cli admin reset-admin-password newpassword

# Or recreate containers
docker-compose down -v
docker-compose up -d
```

### Benchmarks fail to build

**Check build configuration:**
```bash
# Ensure benchmarks are enabled
cmake -B build -DBUILD_BENCHMARKS=ON
cmake --build build --target bench_crud
```

**Missing dependencies?**
```bash
# Install Google Benchmark
sudo apt-get install libbenchmark-dev
```

---

## Next Steps

### 1. Explore the Dashboard

- Click through different panels
- Try different time ranges (top-right)
- Use template variables (Branch, Release, Hardware)
- Export charts (Share → Export)

### 2. Set Up Alerts

```bash
# Configure Slack webhook
export SLACK_WEBHOOK_URL="https://hooks.slack.com/services/YOUR/WEBHOOK/URL"

# Edit prometheus.yml
vim grafana/prometheus.yml
# Add alerting section

# Restart Prometheus
cd grafana && docker-compose restart prometheus
```

### 3. CI/CD Integration

The performance checks run automatically on PRs. To test manually:

```bash
# Trigger workflow
gh workflow run performance-regression-check.yml
```

### 4. Create Baselines

```bash
# For main branch
git checkout main
# Run benchmarks...
python3 benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version $(cat VERSION) \
  --commit $(git rev-parse HEAD)
```

---

## Demo Mode

Want to see the dashboard with sample data?

```bash
# Generate sample metrics
python3 << 'EOF'
import random
import time

with open('benchmarks/performance_data/metrics/benchmarks.prom', 'w') as f:
    f.write('# HELP themisdb_benchmark_throughput_ops Throughput\n')
    f.write('# TYPE themisdb_benchmark_throughput_ops gauge\n')
    
    for i in range(10):
        throughput = random.randint(40000, 50000)
        f.write(f'themisdb_benchmark_throughput_ops{{benchmark="crud",branch="main"}} {throughput}\n')
        time.sleep(0.1)
EOF

# Prometheus will scrape these metrics automatically
```

---

## Resources

- 📖 [Full Documentation](../de/PERFORMANCE_DASHBOARD.md)
- 📊 [Example Charts](./PERFORMANCE_DASHBOARD_EXAMPLES.md)
- 🔧 [Benchmark Best Practices](../../BENCHMARK_BEST_PRACTICES.md)
- 🚀 [CHIMERA Suite](../../benchmarks/README.md)

## Support

Having issues? 

1. Check [Troubleshooting](#troubleshooting) section above
2. Search [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
3. Create new issue with `performance` label

---

**Total Time:** ~5 minutes ⏱️  
**Difficulty:** Easy 🟢  
**You're ready to monitor performance!** 🎯
