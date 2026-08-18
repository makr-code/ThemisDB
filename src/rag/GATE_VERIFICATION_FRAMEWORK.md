# RAG Module Performance Gate Verification Framework

**Purpose:** Define and validate all performance gates for RAG module Phase 5-6 delivery  
**Date:** 2026-08-18  
**Scope:** Wave B GA release readiness validation  
**Target:** Production-ready RAG performance profile

---

## Gate Definitions

### Retrieval Performance Gates

#### GATE-RAG-RETR-01: Dense Vector Search Latency
**Component:** Hybrid Retriever (dense embedding path)  
**Measurement:** p95 latency for 1K-document index search  
**Target:** ≤50ms  
**Rationale:** Dense searches dominate retrieval latency in semantic search workloads

**Benchmark File:** `benchmarks/rag/bench_rag_hybrid_retriever.cpp`  
**Verification Method:**
```bash
./benchmarks/rag/bench_rag_hybrid_retriever \
  --benchmark_filter="DenseSearch/1000" \
  --benchmark_out=results.json
```

**Pass Criteria:**
- p95 latency ≤50ms
- p99 latency ≤75ms
- No timeout failures
- Memory stable (no growth >5%)

**Failure Response:** Block GA promotion, remediate index structure or embedding model

---

#### GATE-RAG-RETR-02: Sparse BM25 Search Latency
**Component:** Hybrid Retriever (sparse keyword path)  
**Measurement:** p95 latency for 10K-document BM25 search  
**Target:** ≤10ms  
**Rationale:** Sparse search must be sub-10ms to keep fusion overhead <20ms

**Benchmark File:** `benchmarks/rag/bench_rag_evaluation.cpp`  
**Verification Method:**
```bash
./benchmarks/rag/bench_rag_evaluation \
  --benchmark_filter="BM25Search/10000" \
  --benchmark_out=results.json
```

**Pass Criteria:**
- p95 latency ≤10ms
- p99 latency ≤15ms
- Index memory <500MB for 10K docs
- Zero segmentation faults

**Failure Response:** Block GA promotion, check index corruption, rebuild index

---

#### GATE-RAG-RETR-03: Multi-Index Fusion Latency
**Component:** Hybrid Retriever (RRF fusion)  
**Measurement:** p95 latency for end-to-end fusion (dense + sparse + rerank)  
**Target:** ≤100ms p95  
**Rationale:** Full retrieval stack must complete within 100ms for responsive UX

**Benchmark File:** `benchmarks/rag/bench_rag_hybrid_retriever.cpp`  
**Verification Method:**
```bash
./benchmarks/rag/bench_rag_hybrid_retriever \
  --benchmark_filter="FullFusion/1000" \
  --benchmark_out=results.json
```

**Pass Criteria:**
- p95 latency ≤100ms
- p99 latency ≤150ms
- Consistent across 10 runs
- Zero results mismatches

**Failure Response:** Investigate reranking overhead, profile bottlenecks, optimize fusion algorithm

---

### Evaluation Performance Gates

#### GATE-RAG-EVAL-01: Relevance Evaluation Throughput
**Component:** Quality Control Pipeline (relevance evaluator)  
**Measurement:** Batch evaluation throughput (queries/sec) for 10-document batch  
**Target:** ≥100 evaluations/sec  
**Rationale:** Evaluation must not become throughput bottleneck in high-traffic scenarios

**Benchmark File:** `benchmarks/rag/bench_rag_evaluation.cpp`  
**Verification Method:**
```bash
./benchmarks/rag/bench_rag_evaluation \
  --benchmark_filter="RelevanceBatch/10" \
  --benchmark_out=results.json
```

**Pass Criteria:**
- Throughput ≥100 eval/sec
- p99 latency ≤100ms per batch
- Model memory stable
- No evaluation staleness

**Failure Response:** Profile model inference, consider batch optimization, check GPU availability

---

#### GATE-RAG-EVAL-02: Ethics/Fairness Overhead
**Component:** Quality Control Pipeline (ethics detector)  
**Measurement:** p95 latency for ethics check on 100-token context  
**Target:** ≤50ms  
**Rationale:** Safety checks must not exceed 50ms per query to stay within E2E budget

**Benchmark File:** `benchmarks/rag/bench_rag_ethics.cpp`  
**Verification Method:**
```bash
./benchmarks/rag/bench_rag_ethics \
  --benchmark_filter="EthicsCheck/100" \
  --benchmark_out=results.json
```

**Pass Criteria:**
- p95 latency ≤50ms
- p99 latency ≤75ms
- Zero false negatives on known bias patterns
- Memory <100MB

**Failure Response:** Optimize model loading, implement caching, consider async evaluation

---

#### GATE-RAG-EVAL-03: LLM Judge Delegation Fallback
**Component:** Quality Control Pipeline (LLM judge integration)  
**Measurement:** p95 latency for fallback path (when LLM unavailable)  
**Target:** ≤10ms  
**Rationale:** Fallback must be near-instant to avoid cascading timeouts

**Benchmark File:** `benchmarks/rag/bench_delegate_evaluator.cpp`  
**Verification Method:**
```bash
./benchmarks/rag/bench_delegate_evaluator \
  --benchmark_filter="FallbackPath" \
  --benchmark_out=results.json
```

**Pass Criteria:**
- Fallback latency ≤10ms
- No missing error indicators
- Graceful scoring degradation
- Consistent detection of unavailability

**Failure Response:** Review fallback implementation, check error propagation, verify circuit breaker

---

### End-to-End Pipeline Gates

#### GATE-RAG-E2E-01: Full RAG Pipeline Latency
**Component:** Full pipeline (query → retrieval → evaluation → assembly)  
**Measurement:** p95 latency for complete RAG query  
**Target:** ≤500ms p95 (for 50K-chunk index)  
**Rationale:** End-to-end SLO for production RAG workloads

**Integration Test:** `tests/rag/test_rag_context_engine.cpp`  
**Verification Method:**
```bash
ctest -R "test_rag_context_engine" --output-on-failure --verbose
```

**Pass Criteria:**
- p95 latency ≤500ms
- p99 latency ≤750ms
- 0 query failures
- Consistent across index sizes (1K→50K docs)

**Failure Response:** Profile pipeline stages, identify bottleneck (retrieval vs evaluation vs assembly), optimize path

---

#### GATE-RAG-E2E-02: Memory Stability Under Load
**Component:** Full pipeline under sustained load  
**Measurement:** Memory growth rate over 10K consecutive queries  
**Target:** <5% memory growth (absolute: <100MB on 2GB baseline)  
**Rationale:** Production reliability requires stable memory usage

**Stress Test:** `tests/rag/test_rag_error_handling_edge_cases_focused.cpp` (E1-E4)  
**Verification Method:**
```bash
ctest -R "test_rag.*EdgeCases.*Stress" --output-on-failure --verbose
# Monitor RSS during execution
```

**Pass Criteria:**
- Memory growth <5% over 10K queries
- No memory leaks (ASan clean)
- No fragmentation (valgrind clean)
- Stable GC/eviction patterns

**Failure Response:** Review cache eviction, check for memory leaks, profile allocation patterns

---

## Gate Acceptance Criteria

### Global Thresholds

| Criterion | Threshold | Action |
|-----------|-----------|--------|
| Performance regression vs baseline | >10% | FAIL - Block GA promotion |
| Memory growth under load | >5% | FAIL - Block GA promotion |
| Test pass rate | <100% | FAIL - Block GA promotion |
| Sanitizer violations | >0 | FAIL - Block GA promotion |
| Timeout failures | >0 | FAIL - Block GA promotion |

### Per-Gate Tolerance

- **Latency Gates:** ±10% tolerance (regression detection automatic)
- **Throughput Gates:** ±15% tolerance (variation from batch effects)
- **Memory Gates:** ±5% tolerance (platform variation)
- **Stability Gates:** ZERO tolerance (binary pass/fail)

---

## Verification Procedure

### Pre-Release Gate Validation

#### Step 1: Environment Setup
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset develop-strict \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -B build-rag-gates
cmake --build build-rag-gates --config Release --parallel 4
```

#### Step 2: Baseline Capture
```bash
cd build-rag-gates

# Capture baseline on clean run
for i in {1..3}; do
  echo "=== Run $i ==="
  ./benchmarks/rag/bench_rag_hybrid_retriever --benchmark_out=baseline-run-$i.json
  ./benchmarks/rag/bench_rag_evaluation --benchmark_out=baseline-run-$i.json
  ./benchmarks/rag/bench_rag_ethics --benchmark_out=baseline-run-$i.json
  ./benchmarks/rag/bench_delegate_evaluator --benchmark_out=baseline-run-$i.json
done

# Compute aggregate baseline
python3 <<EOF
import json
import statistics

def aggregate_baselines(files, filter_name):
    results = []
    for f in files:
        with open(f) as fp:
            data = json.load(fp)
            for bench in data.get('benchmarks', []):
                if filter_name in bench['name']:
                    results.append(bench['real_time'])
    return statistics.mean(results) if results else None

gates = ['DenseSearch/1000', 'BM25Search/10000', 'FullFusion/1000']
for gate in gates:
    mean = aggregate_baselines(['baseline-run-1.json', 'baseline-run-2.json', 'baseline-run-3.json'], gate)
    print(f"{gate}: {mean:.2f}ms baseline")
EOF
```

#### Step 3: Regression Detection
```bash
# Run current benchmarks and compare
./benchmarks/rag/bench_rag_hybrid_retriever --benchmark_out=current.json

# Check for violations
python3 <<EOF
import json

def check_regression(baseline_ms, current_ms, tolerance_pct=10):
    delta = ((current_ms - baseline_ms) / baseline_ms) * 100
    if abs(delta) > tolerance_pct:
        return f"FAIL: {delta:+.1f}% (threshold: ±{tolerance_pct}%)"
    return f"PASS: {delta:+.1f}%"

# Example comparison
print(check_regression(45, 46))  # Slight regression
print(check_regression(45, 60))  # Major regression
EOF
```

#### Step 4: Full Test Execution
```bash
cd build-rag-gates

# Run all RAG tests with sanitizers
ASAN_OPTIONS=halt_on_error=1 TSAN_OPTIONS=halt_on_error=1 \
  ctest -R "test_rag" --output-on-failure --parallel 4 --verbose

# Capture summary
ctest -R "test_rag" --output-on-failure -D Experimental | tee test-results.log
```

#### Step 5: Gate Pass/Fail Decision
```bash
# Aggregate results
python3 <<EOF
import re, json

def parse_test_results(logfile):
    with open(logfile) as f:
        content = f.read()
    
    # Count pass/fail
    passes = len(re.findall(r'Test\s+#\d+:\s+.*PASSED', content))
    fails = len(re.findall(r'Test\s+#\d+:\s+.*FAILED', content))
    
    print(f"Tests Passed: {passes}")
    print(f"Tests Failed: {fails}")
    print(f"Pass Rate: {passes/(passes+fails)*100:.1f}%")
    
    if fails > 0:
        print("\n🔴 GATE FAILED - Test failures detected")
        return False
    print("\n🟢 GATE PASSED - All tests successful")
    return True

parse_test_results('test-results.log')
EOF
```

---

## Gate Status Dashboard

### Current Status (2026-08-18)

| Gate ID | Name | Target | Status | Evidence | Last Run |
|---------|------|--------|--------|----------|----------|
| GATE-RAG-RETR-01 | Dense vector search | ≤50ms p95 | 🟡 PENDING | Build in progress | - |
| GATE-RAG-RETR-02 | Sparse BM25 search | ≤10ms p95 | 🟡 PENDING | Build in progress | - |
| GATE-RAG-RETR-03 | Multi-index fusion | ≤100ms p95 | 🟡 PENDING | Build in progress | - |
| GATE-RAG-EVAL-01 | Relevance throughput | ≥100 eval/sec | 🟡 PENDING | Build in progress | - |
| GATE-RAG-EVAL-02 | Ethics overhead | ≤50ms p95 | 🟡 PENDING | Build in progress | - |
| GATE-RAG-EVAL-03 | Judge fallback | ≤10ms | 🟡 PENDING | Build in progress | - |
| GATE-RAG-E2E-01 | Full pipeline | ≤500ms p95 | 🟡 PENDING | Build in progress | - |
| GATE-RAG-E2E-02 | Memory stability | <5% growth | 🟡 PENDING | Build in progress | - |

**Overall Status:** 🟡 IN PROGRESS (awaiting build completion)

---

## Remediation Playbook

### Scenario: Dense Search Latency Exceeds Target (>50ms)

**Root Causes:**
1. Index too large (>1M embeddings)
2. Index corruption/suboptimal structure
3. Embedding model too slow
4. System resource contention

**Investigation Steps:**
```bash
# 1. Check index size
ls -lah indexes/

# 2. Profile execution
perf record -g -F 99 -- ./benchmarks/rag/bench_rag_hybrid_retriever
perf report

# 3. Check system load
top -bn1 | head -20

# 4. Verify model latency
time ./evaluate_embedding_model sample.txt
```

**Remediation Options:**
- Reduce index size or shard across multiple indices
- Rebuild index with optimized parameters
- Profile and optimize embedding model
- Increase system resources (CPU, memory)
- Scale horizontally (multiple index replicas)

---

### Scenario: Evaluation Throughput Below Target (<100 eval/sec)

**Root Causes:**
1. Model inference too slow
2. Model overloaded (saturation)
3. Batch size suboptimal
4. GPU unavailable (CPU-only fallback)

**Investigation Steps:**
```bash
# 1. Check model throughput
time python3 -c "
import sys
sys.path.insert(0, '.')
from rag.evaluators import RelevanceEvaluator
ev = RelevanceEvaluator()
# Time 100 evaluations
"

# 2. Monitor GPU/CPU
nvidia-smi --query-gpu=utilization.gpu,utilization.memory --format=csv,noheader -l 1

# 3. Check batch optimization
# Try different batch sizes (8, 16, 32, 64)
```

**Remediation Options:**
- Increase batch size for better throughput
- Enable GPU acceleration if available
- Implement evaluation caching
- Use model quantization for faster inference
- Scale to multiple evaluator processes

---

## Approval Sign-Off

This performance gate framework is approved for Wave B GA release validation.

**Framework Version:** 1.0  
**Effective Date:** 2026-08-18  
**Review Cycle:** Pre-release validation  
**Update Frequency:** Per major release cycle

---

**Generated:** 2026-08-18  
**Next Update:** After build completion and gate validation
