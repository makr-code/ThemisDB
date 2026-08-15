# RUNBOOK: GPU Fallback Detection & Performance Monitoring

**Audience:** Database Operators, SREs, GPU/Acceleration Team Lead  
**Purpose:** Detect GPU failures, manage CPU fallback transitions, and maintain performance SLOs  
**Severity:** High (affects acceleration-dependent workloads)  
**Estimated Duration:** 10 min - 1 hour (diagnosis + recovery)  

---

## Overview

This runbook guides operators through monitoring GPU health, detecting CUDA failures, managing CPU fallback transitions, and verifying performance during and after fallback. It covers kernel timeout patterns, memory pressure, and guaranteed clean CPU degradation without service interruption.

**Key Principles:**
- GPU failures must be detected immediately (< 5 sec latency)
- CPU fallback must trigger automatically without operator intervention
- CPU baseline must meet minimum performance SLOs (no performance cliff)
- GPU recovery and re-warm-up must be monitored for stability
- All GPU/CPU transitions must be traceable in logs and metrics

---

## Prerequisites Checklist

- [ ] GPU monitoring dashboard configured (health, utilization, error rate)
- [ ] CPU baseline performance metrics established (latency p99, throughput)
- [ ] GPU failure detection thresholds defined and tested
- [ ] CPU fallback capability tested under production-like load
- [ ] Escalation procedures documented for persistent GPU failures
- [ ] GPU warm-up procedure tested and automated
- [ ] Network paths (GPU → CPU) validated for fallback routing

---

## GPU Health & Monitoring Baseline

### 1. Establish GPU Baseline (Initial Setup)

**Objective:** Document healthy GPU state and performance baselines.

```bash
# Collect GPU health metrics
query-gpu-health --gpu-id all

# Output:
# GPU 0:
#   Temperature: 65°C (normal: < 80°C)
#   Memory Used: 12.5 GB / 16 GB
#   Memory Utilization: 78%
#   Power Draw: 250 W (normal: < 300 W)
#   Fan Speed: 45% (normal: < 70%)
#   CUDA Kernel Errors: 0 (lifetime)
#   Tensor Core Health: OK
#   Memory ECC Errors: 0 (corrected)

# GPU 1:
#   [Similar metrics]
```

### 2. Baseline Performance Metrics

**Objective:** Document performance of GPU-accelerated workloads.

```bash
# Run baseline benchmark
run-benchmark --suite gpu-performance-baseline \
  --gpu-enabled true \
  --gpu-device all \
  --duration 5m \
  --workload matrix-multiply,tensor-operation

# Record baseline:
# - GPU Matrix Multiply: latency p99 = 15 ms, throughput = 5000 ops/sec
# - GPU Tensor Op: latency p99 = 25 ms, throughput = 8000 ops/sec
# - GPU Utilization: 95%
# - GPU Memory: 14.2 GB / 16 GB
```

---

## Step-by-Step GPU Failure Diagnosis & Recovery

### Step 1: GPU Health Check (Every 30 sec, Automated)

**Objective:** Continuously monitor GPU health and detect issues early.

**Automated Monitoring (background):**
```bash
# Health check runs continuously
watch-gpu-health --check-interval 30s --alert-threshold critical

# Automatic alerting on:
# - Temperature > 85°C
# - Memory utilization > 95%
# - CUDA kernel errors > 0
# - Power draw > 350 W
# - Fan speed > 80%
```

**Manual health check (if needed):**
```bash
query-gpu-health --gpu-id <gpu-id> --include-diagnostics true

# Output:
# GPU 0 Health Report:
#   Status: HEALTHY | DEGRADED | CRITICAL
#   Temperature: 85°C (⚠ WARNING: trending up)
#   Memory: 15.8 GB / 16 GB (95% utilization)
#   Kernel Errors: 0
#   Last Error: None
#   Fan Speed: 65% (increasing)
#   Power Draw: 295 W (normal range)
#   Recommendation: NONE
```

### Step 2: Failure Detection (< 5 sec from failure)

**Objective:** Detect and classify GPU failures; determine if CPU fallback needed.

1. **Monitor for CUDA kernel failures:**
   ```bash
   # Check for kernel execution errors
   query-gpu-metrics --gpu-id <gpu-id> \
     --metric kernel_launch_failures,kernel_timeout_events,memory_errors

   # Output:
   # Kernel Launch Failures (last 1 min): 5 (⚠ CRITICAL)
   # Kernel Timeout Events (last 1 min): 2 (⚠ CRITICAL)
   # Memory Errors (last 1 hour): 0
   ```

2. **Classify failure type:**

   | Failure Type | Metric | Threshold | Action |
   |---|---|---|---|
   | **CUDA Error** | kernel_launch_failures | > 3 in 60s | Escalate to GPU team |
   | **Kernel Timeout** | kernel_timeout_events | > 2 in 60s | Reduce batch size or trigger CPU fallback |
   | **Memory Pressure** | memory_utilization | > 98% | Increase memory or reduce workload |
   | **Temperature High** | gpu_temperature | > 85°C | Reduce workload; check cooling |
   | **Hardware Fault** | ecc_errors | > 5 uncorrectable | Hard fail; replace GPU |

3. **Decision Point:**
   - ✅ **Transient error (1-2 occurrences):** Monitor for 30 sec; escalate if persists
   - ⚠ **Recurring error (3-5 in 1 min):** Trigger CPU fallback (Step 3)
   - ❌ **Critical error (>5 in 1 min or ECC failure):** Immediate hard fail → CPU fallback + GPU offline

### Step 3: Trigger CPU Fallback (Automatic or Manual)

**Objective:** Safely transition workloads from GPU to CPU.

1. **Automatic Fallback (triggered by failure conditions in Step 2):**
   ```bash
   # System automatically initiates fallback if:
   # - Kernel timeout events > 2 in 60s, OR
   # - Kernel launch failures > 3 in 60s, OR
   # - GPU temperature > 90°C, OR
   # - Memory utilization > 99%

   # Monitor fallback progress
   watch-gpu-fallback-progress --gpu-id <gpu-id>

   # Output:
   # GPU Fallback Status: IN_PROGRESS
   # Phase 1 (Drain GPU): 60% complete
   # Phase 2 (Enable CPU): waiting
   # Phase 3 (Route traffic): waiting
   ```

2. **Manual Fallback (if automatic fails):**
   ```bash
   # Issue manual fallback command
   initiate-gpu-fallback \
     --gpu-id <gpu-id> \
     --strategy graceful \  # or "immediate" for critical failures
     --target-cpu-cores all \
     --monitor-interval 1s

   # Progress:
   # Phase 1: Draining GPU (in-flight operations complete)
   # Phase 2: Enabling CPU fallback path
   # Phase 3: Rerouting new operations to CPU
   # Phase 4: Disabling GPU for this workload
   ```

### Step 4: Monitor CPU Fallback Performance (5-30 min)

**Objective:** Ensure CPU performance meets minimum SLOs.

1. **Query CPU performance during fallback:**
   ```bash
   query-cpu-performance --fallback-active true \
     --metric latency_p99,throughput,cpu_utilization

   # Output:
   # CPU Performance (during fallback):
   #   Latency p99: 85 ms (baseline GPU: 15 ms; acceptable min: 150 ms)
   #   Throughput: 2000 ops/sec (baseline GPU: 5000 ops/sec; acceptable min: 1500 ops/sec)
   #   CPU Utilization: 92% (baseline: 5-10%; now absorbing GPU load)
   #   Status: ACCEPTABLE (meets fallback SLO)
   ```

2. **Verify SLO compliance:**
   ```bash
   verify-fallback-slo \
     --workload matrix-multiply \
     --metric latency_p99,throughput

   # Output:
   # SLO Check Results:
   #   Latency p99: 85 ms vs. SLO 150 ms ✅ PASS
   #   Throughput: 2000 ops/sec vs. SLO 1500 ops/sec ✅ PASS
   # Overall Status: SLO MAINTAINED
   ```

3. **Check for cascading issues:**
   ```bash
   # Monitor error rates during fallback
   query-metrics --metric error_rate,timeout_rate \
     --window 5m --compare-to baseline

   # Output:
   # Error Rate: 0.01% vs. baseline 0.001% (10x increase, but within tolerance)
   # Timeout Rate: 0.05% vs. baseline 0.01% (5x increase, but within tolerance)
   # Status: ACCEPTABLE (no cascading failures)
   ```

4. **Decision Point:**
   - ✅ **CPU performance meets SLO:** Fallback successful; proceed to Step 5
   - ⚠ **CPU performance degraded but acceptable:** Continue monitoring; consider scaling
   - ❌ **CPU performance below SLO:** Escalate to application team; may need workload reduction

### Step 5: GPU Recovery & Diagnostics (Parallel to CPU fallback)

**Objective:** Diagnose GPU failure root cause and determine if GPU can be recovered.

1. **Immediate diagnostics:**
   ```bash
   # Collect GPU diagnostics
   run-gpu-diagnostics --gpu-id <gpu-id> \
     --intensive true \
     --duration 2m

   # Output:
   # GPU Diagnostics Results:
   #   CUDA API Health: OK
   #   Memory Health: OK (no ECC errors)
   #   Compute Capability: OK (all tensor cores functional)
   #   Power Supply: FAULT (detected power fluctuation 30 sec ago)
   #   Thermal Management: DEGRADED (fan malfunction suspected)
   # Recommended Action: Check power supply and cooling
   ```

2. **If transient failure (e.g., temporary power issue):**
   ```bash
   # Reset GPU
   reset-gpu --gpu-id <gpu-id> --soft true

   # Monitor for recovery
   wait-gpu-ready --gpu-id <gpu-id> --timeout 60s

   # Verify health post-reset
   query-gpu-health --gpu-id <gpu-id>

   # Output:
   # GPU 0 Status: HEALTHY (recovered)
   # Last Error: 5 min ago (resolved)
   # Kernel Errors: 0
   ```

3. **If hardware fault (cannot recover):**
   ```bash
   # Mark GPU offline permanently
   mark-gpu-offline --gpu-id <gpu-id> --reason hardware_fault

   # Notify hardware team
   create-ticket --system hardware \
     --title "GPU $gpu-id Hardware Fault Detected" \
     --priority high
   ```

### Step 6: GPU Warm-Up & Re-Integration (10-30 min)

**Objective:** Safely re-enable GPU after recovery.

1. **Prepare GPU for re-integration:**
   ```bash
   # Clear GPU caches and state
   reinitialize-gpu --gpu-id <gpu-id>

   # Pre-warm GPU with light workload
   run-benchmark --suite gpu-warmup \
     --gpu-id <gpu-id> \
     --intensity low \
     --duration 2m

   # Output:
   # GPU Warmup:
   #   Temperature: ramping from 25°C → 65°C
   #   Memory: allocating and deallocating (testing memory paths)
   #   Tensor Cores: exercising (verifying functionality)
   ```

2. **Transition workloads back to GPU:**
   ```bash
   # Start with small percentage of traffic (canary)
   initiate-gpu-reintegration \
     --gpu-id <gpu-id> \
     --strategy gradual \
     --initial-traffic-percent 10

   # Gradual ramp:
   # 0-5 min: 10% traffic on GPU
   # 5-10 min: 25% traffic on GPU
   # 10-15 min: 50% traffic on GPU
   # 15-20 min: 100% traffic on GPU (if no issues)
   ```

3. **Monitor re-integration:**
   ```bash
   watch-gpu-reintegration --gpu-id <gpu-id> --interval 30s

   # Output:
   # Traffic on GPU: 10% ✓ (latency: 15 ms, errors: 0)
   # Traffic on GPU: 25% ✓ (latency: 15 ms, errors: 0)
   # Traffic on GPU: 50% ✓ (latency: 16 ms, errors: 0)
   # Status: STABLE, safe to continue to 100%
   ```

4. **Validation post-recovery:**
   ```bash
   # Benchmark GPU performance post-recovery
   run-benchmark --suite gpu-performance-baseline \
     --gpu-id <gpu-id> \
     --duration 5m

   # Compare to pre-failure baseline
   compare-metrics \
     --before baseline_pre_failure.json \
     --after current_metrics.json \
     --threshold latency_deviation_pct:5%,throughput_deviation_pct:5%

   # Output:
   # GPU Latency p99: 15 ms (pre: 15 ms, deviation: 0%) ✅
   # GPU Throughput: 5000 ops/sec (pre: 5000 ops/sec, deviation: 0%) ✅
   # Status: RECOVERED
   ```

---

## Troubleshooting Table

| Symptom | Likely Cause | Investigation | Resolution |
|---------|--------------|---|----------|
| GPU temperature spikes to 90°C | Fan malfunction or thermal paste degradation | Check fan speed; compare to baseline power draw | Clean cooling system; replace thermal paste if needed |
| Kernel timeout events increasing gradually | GPU memory fragmentation or compute bottleneck | Monitor memory utilization trend; check workload pattern | Reduce batch size; restart GPU if memory fragmented |
| Power draw drops to 50% | GPU clock throttling due to temperature | Check GPU temperature; verify cooling active | Reduce workload; check cooling efficiency |
| CPU fallback triggered but latency > SLO | Insufficient CPU cores allocated to fallback | Check CPU core allocation; monitor CPU utilization | Scale CPU allocation or reduce workload |
| GPU recovery fails after reset | Hardware fault (power supply, VRAM, or compute) | Run extensive GPU diagnostics | Replace GPU; file hardware ticket |

---

## Incident Report Template

```markdown
# GPU Failure Incident Report

## Incident Details
- **GPU ID:** [gpu-0, gpu-1, etc.]
- **Failure Type:** [CUDA Error/Timeout/Memory Fault/Thermal/Power]
- **Detection Time:** YYYY-MM-DD HH:MM:SS UTC
- **CPU Fallback Triggered:** Yes / No
- **Recovery Time:** [minutes]

## Failure Description
[What CUDA errors or symptoms occurred]

## Impact
- **Workloads Affected:** [list or count]
- **GPU Downtime:** [minutes]
- **CPU Fallback Duration:** [minutes]
- **SLO Breach:** [yes/no]

## Root Cause
[From Step 5 diagnostics]

## Remediation
- **Immediate Action:** [CPU fallback, GPU reset, or offline]
- **Resolution:** [full GPU recovery or replacement]

## Prevention
[What will prevent similar incidents]

## Sign-Off
- **Operator:** [name]
- **GPU Team Lead:** [name]
- **Date:** YYYY-MM-DD
```

---

## Quick Reference

```bash
# Check GPU health
query-gpu-health --gpu-id all

# Trigger CPU fallback (manual)
initiate-gpu-fallback --gpu-id <gpu-id> --strategy graceful

# Monitor fallback performance
watch-gpu-fallback-progress --gpu-id <gpu-id>

# Run GPU diagnostics
run-gpu-diagnostics --gpu-id <gpu-id> --intensive true

# Reset GPU
reset-gpu --gpu-id <gpu-id> --soft true

# Warm up and re-integrate GPU
initiate-gpu-reintegration --gpu-id <gpu-id> --strategy gradual

# Verify recovery
run-benchmark --suite gpu-performance-baseline --gpu-id <gpu-id>
```

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-08-15  
**Owner:** GPU/Acceleration Team  
**Next Review:** 2026-12-15
