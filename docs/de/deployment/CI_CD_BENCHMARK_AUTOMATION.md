# THEMIS CI/CD BENCHMARK AUTOMATION

**Konfiguriert für:** GitHub Actions  
**Ziel:** Automatische Performance-Regression Detection  
**Update-Häufigkeit:** Pro Commit, Nightly, Weekly  

---

## 🎯 ÜBERBLICK - Automation Strategy

```
┌──────────────────────────────────────────────────────────────┐
│                    GITHUB ACTIONS WORKFLOW                   │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  1. Pull Request → 2. Commit → 3. Nightly → 4. Weekly       │
│     Bench      Bench       Bench         Bench               │
│     (Quick)    (Full)      (Full)        (Full+Compare)     │
│     2 min      30 min      2h            4h                  │
│                                                               │
│  ↓              ↓            ↓            ↓                   │
│  Pass/Fail      Report       Report       Comparative       │
│  Comment        & Alert      & Alert      Analysis           │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

---

## 📋 WORKFLOW 1: PR Quick-Benchmark (2 min)

**Trigger:** Jeder Push in Pull Request  
**Datei:** `.github/workflows/pr-benchmark.yml`

```yaml
name: Quick Benchmark - PR

on:
  pull_request:
    paths:
      - 'src/**'
      - 'benchmarks/**'
      - '.github/workflows/pr-benchmark.yml'

jobs:
  quick-bench:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Build (Release)
        run: |
          mkdir -p build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release ..
          cmake --build . --config Release --parallel 8
      
      - name: Run Quick Benchmarks
        run: |
          cd build
          ./bin/query_benchmark --benchmark_filter="SimpleEvaluation|VectorInsert" \
            --benchmark_out=quick_results.json --benchmark_out_format=json
      
      - name: Load Baseline
        run: |
          # Download letzte v1.3.4 Baseline
          wget https://github.com/themis-io/themis/releases/download/v1.3.4/benchmark-baseline.json
      
      - name: Compare Results
        run: |
          python3 scripts/compare_benchmarks.py \
            --baseline benchmark-baseline.json \
            --current build/quick_results.json \
            --threshold 5  # 5% degradation allowed
      
      - name: Comment on PR
        if: always()
        uses: actions/github-script@v7
        with:
          script: |
            const fs = require('fs');
            const comment = fs.readFileSync('benchmark-comment.md', 'utf8');
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: comment
            });
```

---

## 📋 WORKFLOW 2: Full Benchmark (30 min)

**Trigger:** Merge in `develop` Branch  
**Datei:** `.github/workflows/full-benchmark.yml`

```yaml
name: Full Benchmark - Post-Merge

on:
  push:
    branches: [develop]
    paths:
      - 'src/**'
      - 'benchmarks/**'

jobs:
  full-bench:
    runs-on: [self-hosted, high-performance]  # Dedicated runner
    timeout-minutes: 45
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Build (Release)
        run: |
          cmake -S . -B build-ci -DCMAKE_BUILD_TYPE=Release
          cmake --build build-ci --config Release --parallel 16
      
      - name: Run Full Benchmark Suite
        run: |
          cd build-ci
          ./bin/query_benchmark --benchmark_out=results_query.json \
            --benchmark_out_format=json --benchmark_time_unit=us
          ./bin/vector_benchmark --benchmark_out=results_vector.json \
            --benchmark_out_format=json
          ./bin/index_benchmark --benchmark_out=results_index.json \
            --benchmark_out_format=json
          # ... weitere benchmarks
      
      - name: Aggregate Results
        run: |
          python3 scripts/aggregate_benchmarks.py \
            build-ci/results_*.json \
            --output build-ci/aggregated_results.json
      
      - name: Compare Against Baseline
        run: |
          python3 scripts/regression_detector.py \
            --baseline benchmarks/baseline_v1.3.4.json \
            --current build-ci/aggregated_results.json \
            --sensitivity high
      
      - name: Upload Results to S3
        run: |
          aws s3 cp build-ci/aggregated_results.json \
            s3://themis-benchmarks/${{ github.sha }}_results.json
      
      - name: Slack Notification
        if: failure()
        uses: slackapi/slack-github-action@v1
        with:
          payload: |
            {
              "text": "❌ Performance Regression Detected",
              "blocks": [
                {
                  "type": "section",
                  "text": {
                    "type": "mrkdwn",
                    "text": "*Commit:* ${{ github.sha }}\n*Details:* <https://github.com/themis-io/themis/runs/${{ github.run_id }}|View Logs>"
                  }
                }
              ]
            }
```

---

## 📋 WORKFLOW 3: Nightly Stress Test (2h)

**Trigger:** Täglich 22:00 UTC  
**Datei:** `.github/workflows/nightly-stress-test.yml`

```yaml
name: Nightly Stress Test

on:
  schedule:
    - cron: '0 22 * * *'  # 22:00 UTC

jobs:
  stress-test:
    runs-on: ubuntu-latest
    timeout-minutes: 150
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Build
        run: |
          cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build --parallel 8
      
      - name: Run Long-Running Stress Tests
        run: |
          cd build
          # 100M Vector Insert Test
          ./bin/stress_test --operation=vector_insert \
            --item_count=100000000 \
            --duration=1800 \
            --thread_count=8 \
            --output=stress_vector.json
          
          # 1M Index Updates with Queries
          ./bin/stress_test --operation=index_update_query_mix \
            --item_count=1000000 \
            --duration=1800 \
            --read_write_ratio=70:30 \
            --output=stress_index.json
          
          # Memory Leak Detection
          valgrind --leak-check=full \
            --show-leak-kinds=all \
            ./bin/query_benchmark --benchmark_min_time=600 \
            2>&1 | tee valgrind_output.txt
      
      - name: Analyze Memory Patterns
        run: |
          python3 scripts/analyze_memory.py \
            --valgrind valgrind_output.txt \
            --report memory_analysis.md
      
      - name: Create Report
        if: always()
        run: |
          python3 scripts/create_stress_report.py \
            build/stress_vector.json \
            build/stress_index.json \
            --output stress_report.md
      
      - name: Archive Results
        uses: actions/upload-artifact@v3
        with:
          name: stress-test-results
          path: |
            build/stress_*.json
            memory_analysis.md
            stress_report.md
          retention-days: 30
      
      - name: Alert on Issues
        if: failure()
        run: |
          # E-Mail an Perf Team
          curl -X POST https://api.sendgrid.com/v3/mail/send \
            -H "Authorization: Bearer ${{ secrets.SENDGRID_KEY }}" \
            -H "Content-Type: application/json" \
            -d @alert_email.json
```

---

## 📋 WORKFLOW 4: Weekly Comparative Analysis (4h)

**Trigger:** Jeden Sonntag 00:00 UTC  
**Datei:** `.github/workflows/weekly-comparative.yml`

```yaml
name: Weekly Comparative Analysis

on:
  schedule:
    - cron: '0 0 * * 0'  # Sunday 00:00 UTC

jobs:
  comparative-analysis:
    runs-on: [self-hosted, analysis-engine]
    timeout-minutes: 300
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Checkout Multiple Branches
        run: |
          # Compare current develop against v1.3.4
          git fetch origin v1.3.4:refs/remotes/origin/v1.3.4
          git fetch origin develop:refs/remotes/origin/develop
      
      - name: Build All Versions
        run: |
          for version in "v1.3.4" "develop"; do
            git checkout $version
            mkdir build-$version
            cd build-$version
            cmake -DCMAKE_BUILD_TYPE=Release ..
            cmake --build . --parallel 16
            cd ..
          done
      
      - name: Run Comprehensive Benchmarks
        run: |
          mkdir results
          for version in "v1.3.4" "develop"; do
            echo "Running benchmarks for $version..."
            cd build-$version
            
            # Multiple iterations for statistical significance
            for i in {1..3}; do
              ./bin/full_benchmark \
                --benchmark_out=../results/${version}_iteration_${i}.json \
                --benchmark_out_format=json \
                --benchmark_repetitions=10
            done
            cd ..
          done
      
      - name: Statistical Analysis
        run: |
          python3 scripts/statistical_analysis.py \
            --results-dir results \
            --confidence-level 0.95 \
            --output weekly_analysis.md
      
      - name: Generate Performance Report
        run: |
          python3 scripts/generate_weekly_report.py \
            --analysis weekly_analysis.md \
            --metrics-history benchmarks/historical_metrics.csv \
            --output WEEKLY_PERFORMANCE_REPORT.md
      
      - name: Create Visualizations
        run: |
          python3 scripts/create_charts.py \
            --results results \
            --output graphs
          # Upload PNG files to GitHub Pages
      
      - name: Commit Report
        run: |
          git config user.name "Benchmark Bot"
          git config user.email "bot@themis.io"
          git add WEEKLY_PERFORMANCE_REPORT.md
          git add benchmarks/historical_metrics.csv
          git commit -m "Weekly benchmark report: $(date -u +%Y-%m-%d)"
          git push origin develop
      
      - name: Slack Summary
        uses: slackapi/slack-github-action@v1
        with:
          webhook-url: ${{ secrets.SLACK_WEBHOOK_BENCHMARKS }}
          payload: |
            {
              "text": "📊 Weekly Performance Analysis Complete",
              "blocks": [
                {
                  "type": "section",
                  "text": {
                    "type": "mrkdwn",
                    "text": "*Week:* $(date -u +%Y-W%V)\n*Status:* ✅ Complete\n*Report:* <https://github.com/themis-io/themis/blob/develop/WEEKLY_PERFORMANCE_REPORT.md|View Details>"
                  }
                }
              ]
            }
```

---

## 🔧 HELPER SCRIPTS

### Script 1: `scripts/compare_benchmarks.py`

```python
#!/usr/bin/env python3
import json
import sys
import argparse
from statistics import mean, stdev

class BenchmarkComparator:
    def __init__(self, baseline, current, threshold=5):
        self.baseline = self.load_json(baseline)
        self.current = self.load_json(current)
        self.threshold = threshold  # 5% default
        self.regressions = []
        self.improvements = []
    
    def load_json(self, filepath):
        with open(filepath) as f:
            return json.load(f)
    
    def compare(self):
        """Vergleiche alle Benchmarks"""
        for bench_name, current_val in self.current.items():
            if bench_name not in self.baseline:
                print(f"⭐ NEW: {bench_name} = {current_val}")
                continue
            
            baseline_val = self.baseline[bench_name]
            delta_pct = ((current_val - baseline_val) / baseline_val) * 100
            
            if delta_pct < -self.threshold:
                self.regressions.append({
                    'name': bench_name,
                    'baseline': baseline_val,
                    'current': current_val,
                    'delta_pct': delta_pct
                })
                print(f"❌ REGRESSION: {bench_name} {delta_pct:+.1f}%")
            elif delta_pct > self.threshold:
                self.improvements.append({
                    'name': bench_name,
                    'baseline': baseline_val,
                    'current': current_val,
                    'delta_pct': delta_pct
                })
                print(f"✅ IMPROVEMENT: {bench_name} {delta_pct:+.1f}%")
            else:
                print(f"➡️  STABLE: {bench_name} {delta_pct:+.1f}%")
    
    def write_comment(self, output_file):
        """GitHub PR Comment schreiben"""
        comment = f"## 📊 Benchmark Results\n\n"
        
        if self.regressions:
            comment += "### ❌ Regressions\n"
            for reg in self.regressions:
                comment += f"- **{reg['name']}**: {reg['delta_pct']:+.1f}% " \
                          f"({reg['baseline']:.0f} → {reg['current']:.0f})\n"
        
        if self.improvements:
            comment += "### ✅ Improvements\n"
            for imp in self.improvements:
                comment += f"- **{imp['name']}**: {imp['delta_pct']:+.1f}% " \
                          f"({imp['baseline']:.0f} → {imp['current']:.0f})\n"
        
        comment += f"\n**Summary:** {len(self.improvements)} improvements, " \
                  f"{len(self.regressions)} regressions"
        
        with open(output_file, 'w') as f:
            f.write(comment)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', required=True)
    parser.add_argument('--current', required=True)
    parser.add_argument('--threshold', type=int, default=5)
    args = parser.parse_args()
    
    comparator = BenchmarkComparator(args.baseline, args.current, args.threshold)
    comparator.compare()
    comparator.write_comment('benchmark-comment.md')
    
    if comparator.regressions:
        sys.exit(1)  # Fail job if regressions found
```

### Script 2: `scripts/regression_detector.py`

```python
#!/usr/bin/env python3
import json
import sys
from statistics import mean

class RegressionDetector:
    """Erkennt Performance-Regressionen mit statistischer Signifikanz"""
    
    def __init__(self, baseline, current, sensitivity='medium'):
        self.baseline = self.load_data(baseline)
        self.current = self.load_data(current)
        self.sensitivity = sensitivity
        self.thresholds = {
            'high': 2.5,      # 2.5% = streng
            'medium': 5.0,    # 5% = moderat
            'low': 10.0       # 10% = locker
        }
    
    def load_data(self, filepath):
        with open(filepath) as f:
            return json.load(f)
    
    def detect(self):
        """Detektiere signifikante Regressionen"""
        threshold = self.thresholds[self.sensitivity]
        critical_regressions = []
        
        for metric_name, current_val in self.current.items():
            if metric_name not in self.baseline:
                continue
            
            baseline_val = self.baseline[metric_name]
            delta_pct = ((current_val - baseline_val) / baseline_val) * 100
            
            # Regressionen sind negative delta
            if delta_pct < -threshold:
                critical_regressions.append({
                    'metric': metric_name,
                    'threshold': threshold,
                    'actual': delta_pct
                })
        
        if critical_regressions:
            print("🚨 CRITICAL REGRESSIONS DETECTED:")
            for reg in critical_regressions:
                print(f"  {reg['metric']}: {reg['actual']:.1f}% "
                      f"(threshold: -{reg['threshold']:.1f}%)")
            return False
        
        print("✅ No significant regressions detected")
        return True

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', required=True)
    parser.add_argument('--current', required=True)
    parser.add_argument('--sensitivity', default='medium')
    args = parser.parse_args()
    
    detector = RegressionDetector(args.baseline, args.current, args.sensitivity)
    success = detector.detect()
    sys.exit(0 if success else 1)
```

---

## 📊 DASHBOARD & REPORTING

### GitHub Pages Dashboard

```html
<!-- docs/benchmark-dashboard.html -->
<!DOCTYPE html>
<html>
<head>
    <title>Themis Performance Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>Themis Performance Metrics</h1>
    
    <div id="query-chart"></div>
    <div id="vector-chart"></div>
    <div id="memory-chart"></div>
    
    <script>
        // Auto-updated from CI/CD pipeline
        fetch('benchmark-data.json')
            .then(r => r.json())
            .then(data => {
                // Render charts
                new Chart(document.getElementById('query-chart'), {
                    type: 'line',
                    data: {
                        labels: data.dates,
                        datasets: [{
                            label: 'Query Throughput (M/sec)',
                            data: data.query_throughput,
                            borderColor: 'rgb(75, 192, 192)',
                            tension: 0.1
                        }]
                    }
                });
            });
    </script>
</body>
</html>
```

---

## 🔐 SECRETS & ENVIRONMENT

Erforderliche GitHub Secrets:

```
SENDGRID_KEY              # E-Mail Alerts
SLACK_WEBHOOK_BENCHMARKS  # Slack Integration
AWS_ACCESS_KEY_ID         # S3 Upload
AWS_SECRET_ACCESS_KEY     # S3 Upload
BENCHMARK_BOT_TOKEN       # GitHub Commits
```

---

## ⚙️ HARDWARE-ANFORDERUNGEN

### Für Quick Benchmarks (PR)
- **Runner:** Ubuntu Latest (GitHub-hosted)
- **CPU:** 4 cores
- **RAM:** 8GB
- **Dauer:** 2 min

### Für Full Benchmarks
- **Runner:** Self-hosted, High-Performance
- **CPU:** 16+ cores
- **RAM:** 32GB
- **Storage:** 500GB+ SSD
- **Dauer:** 30 min

### Für Stress Tests
- **Runner:** Self-hosted, Ultra-High-Performance
- **CPU:** 32 cores
- **RAM:** 64GB
- **Storage:** 1TB SSD
- **Dauer:** 2-4h

---

## 📈 METRICS BEING TRACKED

```
Laufzeitmetriken:
  ✓ Vector Insert Throughput (items/sec)
  ✓ Index Insert Throughput (items/sec)
  ✓ Query Performance (items/sec)
  ✓ Memory Usage (GB)
  ✓ Cache Hit Rate (%)
  ✓ Latency p99 (μs)

Regressionsdetection:
  ✓ Throughput Regression > 5%
  ✓ Latency Regression > 10%
  ✓ Memory Increase > 15%
  ✓ Memory Leaks > 100MB

Historische Daten:
  ✓ Wöchentliche Trends
  ✓ Monatliche Vergleiche
  ✓ Hardware-Profilierung
```

---

## 🚀 IMPLEMENTIERUNGS-SCHRITTE

1. **Repositories Setup**
   ```bash
   mkdir -p .github/workflows
   mkdir -p scripts
   mkdir -p benchmarks/baseline
   ```

2. **Baseline Erstellen**
   ```bash
   ./run_benchmarks.sh > benchmarks/baseline_v1.3.4.json
   git add benchmarks/baseline_v1.3.4.json
   ```

3. **Workflows Hinzufügen**
   ```bash
   cp pr-benchmark.yml .github/workflows/
   cp full-benchmark.yml .github/workflows/
   cp nightly-stress-test.yml .github/workflows/
   cp weekly-comparative.yml .github/workflows/
   ```

4. **Scripts Installieren**
   ```bash
   pip install -r scripts/requirements.txt
   ```

5. **Secrets Konfigurieren**
   - GitHub: Settings → Secrets → New Repository Secret
   - Alle erforderlichen Secrets eingeben

6. **Workflows Aktivieren**
   - GitHub Actions Tab → Alle Workflows enabled setzen

---

**Automation erstellt:** 29.12.2025  
**Wartung:** Pro Woche überprüft  
**Support:** Engineering Team  
**Ziel:** Zero Performance Regressions
