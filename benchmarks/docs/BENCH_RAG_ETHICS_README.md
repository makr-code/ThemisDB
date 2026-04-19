> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# RAG Ethics Benchmarks

Performance benchmarks for the RAG Ethics Integration features.

## Overview

This benchmark suite measures the performance of ethical compliance evaluation and ethical perspective gap detection in the RAG system.

## Benchmarks Included

### Full Ethical Compliance Evaluation
- `BM_EthicalCompliance_Full_Good` - Full evaluation with good ethical answer
- `BM_EthicalCompliance_Full_Bad` - Full evaluation with ethically problematic answer
- `BM_EthicalCompliance_Disabled` - Baseline with ethical evaluation disabled

### Individual Dimension Benchmarks
- `BM_AutonomyRespect_Good` - Autonomy respect with respectful language
- `BM_AutonomyRespect_Patronizing` - Autonomy respect with patronizing language
- `BM_MoralDiversity_MultiFramework` - Moral diversity with multiple frameworks
- `BM_MoralDiversity_SingleFramework` - Moral diversity with single framework
- `BM_CitationQuality_WithCitations` - Citation quality with proper citations
- `BM_CitationQuality_NoCitations` - Citation quality without citations

### Ethical Perspective Gap Detection
- `BM_EthicalGapDetection_EthicalQuery` - Gap detection for ethical queries
- `BM_EthicalGapDetection_NonEthicalQuery` - Gap detection for non-ethical queries
- `BM_EthicalGapDetection_SufficientPerspectives` - Gap detection with sufficient perspectives

### Pattern Detection Micro-Benchmarks
- `BM_PatronizingDetection_NoPatterns` - Patronizing detection without patterns
- `BM_PatronizingDetection_MultiplePatterns` - Patronizing detection with multiple patterns
- `BM_BiasDetection_Balanced` - Bias detection with balanced text
- `BM_BiasDetection_Biased` - Bias detection with biased text
- `BM_FrameworkRecognition_SingleFramework` - Framework recognition with one framework
- `BM_FrameworkRecognition_MultipleFrameworks` - Framework recognition with multiple frameworks

### VETO Mechanism
- `BM_VetoMechanism_Pass` - VETO mechanism with passing score
- `BM_VetoMechanism_Fail` - VETO mechanism with failing score

### Scalability Benchmarks
- `BM_EthicalCompliance_VaryingAnswerLength` - Tests with varying answer lengths (1, 10, 50, 100 repetitions)
- `BM_EthicalGapDetection_VaryingDocCount` - Tests with varying document counts (1, 5, 10, 20 docs)

## Performance Targets

Based on the RAG Ethics Integration requirements:

- **Total Ethical Evaluation**: < 800ms
- **Autonomy Assessment**: < 200ms
- **Moral Diversity Check**: < 300ms
- **Citation Quality**: < 200ms
- **Gap Detection**: < 100ms

## Building

```bash
# Configure with benchmarks enabled
cmake -B build -DTHEMIS_BUILD_BENCHMARKS=ON

# Build the RAG ethics benchmark
cmake --build build --target bench_rag_ethics
```

## Running

> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`


### Run All Benchmarks
```bash
./build/benchmarks/bench_rag_ethics
```

### Run Specific Benchmarks
```bash
# Run only full compliance benchmarks
./build/benchmarks/bench_rag_ethics --benchmark_filter="Full"

# Run only pattern detection benchmarks
./build/benchmarks/bench_rag_ethics --benchmark_filter="Pattern"

# Run only gap detection benchmarks
./build/benchmarks/bench_rag_ethics --benchmark_filter="Gap"
```

### Output Options
```bash
# JSON output for analysis
./build/benchmarks/bench_rag_ethics --benchmark_out=rag_ethics_results.json --benchmark_out_format=json

# CSV output
./build/benchmarks/bench_rag_ethics --benchmark_out=rag_ethics_results.csv --benchmark_out_format=csv

# Console output with color
./build/benchmarks/bench_rag_ethics --benchmark_color=true
```

### Performance Analysis
```bash
# Run with multiple repetitions for statistical analysis
./build/benchmarks/bench_rag_ethics --benchmark_repetitions=10 --benchmark_report_aggregates_only=true

# Quick benchmarks (minimum time per test)
./build/benchmarks/bench_rag_ethics --benchmark_min_time=0.1

# Run specific scalability tests
./build/benchmarks/bench_rag_ethics --benchmark_filter="Varying"
```

## Interpreting Results

### Expected Performance

Based on pattern-based detection (no LLM calls):

1. **Full Ethical Compliance**: Should be < 10ms per evaluation
2. **Autonomy Assessment**: Should be < 5ms
3. **Moral Diversity**: Should be < 5ms
4. **Citation Quality**: Should be < 3ms
5. **Gap Detection**: Should be < 2ms

These are significantly faster than the targets because pattern-based detection avoids LLM inference latency.

### Identifying Performance Issues

- **Significantly longer times**: Check for unexpected LLM calls or inefficient pattern matching
- **High variance**: May indicate caching issues or system load
- **Linear scaling with input size**: Expected for pattern detection; investigate if non-linear

## Benchmark Categories

### 1. Correctness Benchmarks
Test that detection works correctly with different inputs:
- Good vs. bad ethical answers
- Ethical vs. non-ethical queries
- Sufficient vs. insufficient perspectives

### 2. Performance Benchmarks
Measure raw performance:
- Pattern detection speed
- Framework recognition speed
- VETO mechanism overhead

### 3. Scalability Benchmarks
Test performance with varying input sizes:
- Answer length (short to very long)
- Document count (1 to 20+)

## Integration with CI/CD

These benchmarks can be integrated into CI/CD pipelines to detect performance regressions:

```bash
# Run benchmarks and save results
./build/benchmarks/bench_rag_ethics --benchmark_out=results.json --benchmark_out_format=json

# Compare with baseline (if you have a previous results.json)
# Use tools like benchmark-compare or custom scripts to analyze differences
```

## Contributing

When adding new benchmarks:

1. Follow the existing naming convention: `BM_<Feature>_<Variant>`
2. Use `benchmark::DoNotOptimize()` to prevent compiler optimizations from skipping work
3. Add documentation to this README
4. Include expected performance targets
5. Consider adding both correctness and performance variants

## References

- RAG Ethics Integration Issue: [RAG-ETHICS]
- Implementation Documentation: `docs/RAG_ETHICS_IMPLEMENTATION.md`
- Test Suite: `tests/test_rag_ethics.cpp`
