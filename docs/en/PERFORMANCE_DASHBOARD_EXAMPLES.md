# Performance Dashboard - Example Charts & Visualizations

This document provides example charts and visualizations from the ThemisDB Performance Dashboard along with interpretation guidelines.

## Chart Gallery

### 1. Throughput Trends - CRUD Operations

**What it shows:** Operations per second for Create, Read, Update, Delete operations over time across different branches.

**Key Metrics:**
- **Write Throughput**: Target > 40,000 ops/sec
- **Read Throughput**: Target > 100,000 ops/sec  
- **CRUD Mixed**: Target > 45,000 ops/sec

**Example Interpretation:**

```
Main Branch (blue line):     45,234 ops/sec ✅ Above target
Develop Branch (green line): 43,112 ops/sec ⚠️ Slightly below
PR #1234 (orange line):      38,901 ops/sec ❌ Regression detected
```

**What to look for:**
- ✅ Stable or upward trends
- ⚠️ Sudden drops (investigate immediately)
- ℹ️ Gradual decline over multiple days (technical debt accumulation)

---

### 2. Latency Percentiles (P99/P95/P50)

**What it shows:** Response time distribution for queries and operations.

**Thresholds:**
- **P50 (Median)**: < 2ms ideal
- **P95**: < 10ms target
- **P99**: < 50ms critical threshold

**Example Data:**

| Benchmark | P50 | P95 | P99 | Status |
|-----------|-----|-----|-----|--------|
| Point Read | 0.8ms | 2.1ms | 4.5ms | ✅ Excellent |
| Point Write | 1.2ms | 3.8ms | 8.2ms | ✅ Good |
| Range Query | 5.4ms | 18.2ms | 45.1ms | ✅ Acceptable |
| Complex Join | 12.3ms | 89.4ms | 156.7ms | ⚠️ Review |

**What to look for:**
- P99/P50 ratio (should be < 10x)
- Sudden spikes in P99 (outliers or systemic issues)
- Gradual increase over releases (performance degradation)

---

### 3. Error Rate Tracking

**What it shows:** Percentage of failed operations over time.

**Targets:**
- **Normal**: < 0.1%
- **Warning**: 0.1% - 1%
- **Critical**: > 1%

**Example Visualization:**

```
Error Rate Over Time (7 days)
1.2% |     *                    🔴 Critical spike
1.0% |    * *                   
0.8% |   *   *                  
0.5% |  *     *                 🟡 Elevated
0.2% | *       *              
0.1% |*         *****         🟢 Normal
0.0% |________________*********___________
     Mon Tue Wed Thu Fri Sat Sun
```

**What to look for:**
- Spikes correlating with deployments
- Patterns (time-of-day, day-of-week)
- Sustained elevated rates

---

### 4. Branch Comparison - Throughput

**What it shows:** Side-by-side throughput comparison between branches.

**Example Data:**

```
Throughput Comparison (ops/sec)
Main:    ████████████████████████ 45,234
Develop: ██████████████████████   42,112 (-6.9%)
Feature: ████████████████         35,890 (-20.7%) ⚠️
```

**Decision Matrix:**

| Difference | Action |
|------------|--------|
| < 5% | ✅ Acceptable, merge |
| 5-10% | ⚠️ Review, document reason |
| 10-20% | ❌ Block, investigate |
| > 20% | ❌ Block, critical regression |

---

### 5. Release Comparison - Throughput

**What it shows:** Performance evolution across releases.

**Example Chart:**

```
Release Performance Trend
50K |                          v1.4.1
    |                    v1.4.0  *
45K |              v1.3.2  *    *
    |        v1.3.1  *    *    
40K |  v1.3.0  *    *    
    | *    *    
35K |*    
    |_________________________________
    Jan      Feb   Mar   Apr   May
    
Legend: * = Release
```

**What to look for:**
- Consistent improvements (good)
- Flat trends (stagnation)
- Regressions between releases (immediate attention)

---

### 6. Hardware Comparison

**What it shows:** Performance across different hardware configurations.

**Example Data:**

| Hardware | CRUD ops/sec | Latency P99 | Cost/hr | Efficiency |
|----------|--------------|-------------|---------|------------|
| GitHub Actions (2 cores) | 25,000 | 15ms | $0.008 | 3.1M ops/$ |
| AWS c5.2xlarge (8 cores) | 65,000 | 8ms | $0.34 | 191K ops/$ |
| Bare Metal (32 cores) | 180,000 | 3ms | $2.50 | 72K ops/$ |

**What to look for:**
- Linear scaling with cores
- Cost efficiency for your use case
- Consistent P99 across hardware

---

### 7. LLM Token Generation Throughput

**What it shows:** Tokens per second for LLM inference workloads.

**Targets:**
- **7B models**: > 50 tokens/sec
- **13B models**: > 25 tokens/sec
- **70B models**: > 5 tokens/sec

**Example Chart:**

```
Token Generation Rate (tokens/sec)
60 | Mistral-7B    *****
50 |             **     **
40 |           **         **
30 | Llama-13B *            ***
20 |         **               **
10 | Llama-70B**                 ***
 0 |_________________________________
   0   5   10  15  20  25  30  min
```

**What to look for:**
- Consistent generation rate (no throttling)
- First token latency < 200ms
- No memory leaks (rate stays stable)

---

### 8. Vector Search Performance

**What it shows:** Performance of HNSW and other vector index types.

**Benchmarks:**
- **HNSW**: Approximate nearest neighbor search
- **Flat**: Brute-force exact search
- **IVF**: Inverted file index

**Example Data:**

| Index Type | Build Time | Search QPS | Recall@10 | Memory |
|------------|------------|------------|-----------|--------|
| HNSW (M=16, ef=200) | 45s | 5,400 | 0.97 | 2.1GB |
| Flat | 0s | 180 | 1.00 | 1.5GB |
| IVF (nlist=100) | 12s | 2,100 | 0.93 | 1.8GB |

**What to look for:**
- Recall vs. speed trade-off
- Memory usage under limits
- Search performance > 1000 QPS for production

---

### 9. Regression Count by Severity

**What it shows:** Number of detected regressions categorized by severity.

**Example Dashboard Panel:**

```
┌─────────────────────────────────┐
│  ❌ Critical: 0                 │
│  ⚠️  Major:    2                 │
│  ℹ️  Minor:    5                 │
│  ✅ Improvements: 8              │
└─────────────────────────────────┘
```

**Alert Rules:**
- Critical (>20%): Page on-call
- Major (10-20%): Create ticket
- Minor (5-10%): Review in weekly meeting

---

### 10. Top Performance Changes

**What it shows:** Benchmarks with largest performance changes in last hour/day.

**Example Table:**

| Benchmark | Previous | Current | Change | Trend |
|-----------|----------|---------|--------|-------|
| vector_search_100k | 4,200 qps | 5,100 qps | +21.4% | ✅ Improvement |
| write_batch_1000 | 38,000 | 32,000 | -15.8% | ❌ Regression |
| read_point_query | 125,000 | 128,000 | +2.4% | ✅ Stable |
| complex_join | 890 qps | 865 qps | -2.8% | ⚠️ Watch |

**What to look for:**
- Correlate with recent code changes
- Verify intentional vs. unintentional changes
- Document justified regressions

---

## Dashboard Layout Best Practices

### 1. Overview Row (Top)
- Critical metrics at a glance
- Regression counts
- Overall health status
- Time range selector

### 2. Throughput Row
- Main performance metrics
- Branch comparisons
- Trend lines

### 3. Latency Row
- Percentile distributions
- SLO compliance
- Outlier detection

### 4. Deep Dive Rows
- Error rates
- Hardware comparisons
- Specialized benchmarks (LLM, Vector)

### 5. Analysis Row (Bottom)
- Top changes
- Detailed tables
- Investigation tools

---

## Reading the Dashboard

### Quick Health Check (< 1 minute)

1. **Look at regression panel** (top-left)
   - 0 critical = ✅
   - Any critical = 🚨 investigate immediately

2. **Check throughput trends** (center-left)
   - Lines going up or flat = ✅
   - Lines trending down = ⚠️

3. **Verify P99 latency** (center-right)
   - Under threshold = ✅
   - Spikes = ⚠️ investigate

4. **Error rate** (bottom-left)
   - Near 0% = ✅
   - > 0.1% = ⚠️

### Deep Investigation (5-10 minutes)

1. **Identify the regression**
   - Which benchmark?
   - When did it start?
   - How severe?

2. **Find the cause**
   - Correlate with commits (use annotations)
   - Check branch/PR
   - Review code changes

3. **Assess impact**
   - Production vs. benchmark environment
   - User-facing operations?
   - Critical path?

4. **Plan remediation**
   - Revert?
   - Optimize?
   - Accept trade-off?

---

## Example Queries for Debugging

### Find when throughput dropped

```promql
# Show rate of change
deriv(themisdb_benchmark_throughput_ops{benchmark="crud"}[1h])
```

### Compare PR to baseline

```promql
# Percentage difference
(themisdb_benchmark_throughput_ops{branch="pr-1234"} / 
 themisdb_benchmark_throughput_ops{branch="main"} - 1) * 100
```

### Identify outliers

```promql
# Standard deviation from mean
abs(themisdb_benchmark_latency_ms - 
    avg_over_time(themisdb_benchmark_latency_ms[24h])) > 
  2 * stddev_over_time(themisdb_benchmark_latency_ms[24h])
```

### Performance by time of day

```promql
# Group by hour
avg(themisdb_benchmark_throughput_ops) by (hour(time()))
```

---

## Custom Chart Templates

### Creating a New Panel

1. **Click "Add Panel"** in Grafana
2. **Select visualization type**:
   - Time series: Trends over time
   - Stat: Single value KPIs
   - Table: Detailed comparisons
   - Bar chart: Categorical comparisons
   
3. **Write PromQL query**:
```promql
# Example: Average throughput by branch
avg(themisdb_benchmark_throughput_ops) by (branch)
```

4. **Configure thresholds**:
   - Green: Above target
   - Yellow: Warning range
   - Red: Critical

5. **Add to dashboard**

---

## Exporting Charts

### For Documentation

```bash
# Screenshot
# Use Grafana's built-in share → snapshot feature
# Or use API:
curl -H "Authorization: Bearer YOUR_API_KEY" \
  "http://localhost:3000/render/d-solo/themisdb-performance-dashboard/...?width=800&height=400" \
  > chart.png
```

### For Reports

```bash
# PDF Export (requires Grafana Enterprise or use puppeteer)
# Alternative: Screenshot all panels and assemble in document
```

---

## Further Reading

- [Grafana Visualization Best Practices](https://grafana.com/docs/grafana/latest/best-practices/)
- [PromQL Tutorial](https://prometheus.io/docs/prometheus/latest/querying/basics/)
- [Performance Testing Guidelines](../../BENCHMARK_BEST_PRACTICES.md)
- [CHIMERA Benchmark Suite](../../benchmarks/README.md)
