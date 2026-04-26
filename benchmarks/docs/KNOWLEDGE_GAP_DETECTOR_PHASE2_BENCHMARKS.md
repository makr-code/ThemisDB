> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# Knowledge Gap Detector Phase 2 Benchmarks

Comprehensive Google Benchmark suite for evaluating the performance of Phase 2 LLM-based confidence metrics in the Knowledge Gap Detector.

## Overview

This benchmark suite measures the performance of:
- **Token Probability Tracking**: Perplexity calculation, outlier detection, confidence aggregation
- **Self-Consistency Checking**: Multiple sampling, semantic similarity, contradiction detection  
- **FLARE Active Retrieval**: Sentence splitting, query reformulation, iterative retrieval

## Building the Benchmarks

```bash
# Configure with benchmarks enabled
cmake -B build -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_ENABLE_LLM=ON

# Build the Phase 2 benchmarks
cmake --build build --target bench_knowledge_gap_detector_phase2

# Or build all benchmarks
cmake --build build --target all
```

## Running the Benchmarks

### Run All Phase 2 Benchmarks

```bash
cd build
./benchmarks/bench_knowledge_gap_detector_phase2
```

### Run Specific Categories

```bash
# Token probability tracking only
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_filter="Perplexity"

# Self-consistency checks only
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_filter="SelfConsistency|Semantic|Contradiction"

# FLARE active retrieval only
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_filter="FLARE"

# Configuration benchmarks
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_filter="DetectorCreation|Configuration"
```

### Advanced Options

```bash
# Run with multiple repetitions for statistical accuracy
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true

# Run with minimal time per benchmark (faster)
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_min_time=0.1

# Export results to JSON
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_format=json --benchmark_out=results.json

# Export results to CSV
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_format=csv --benchmark_out=results.csv
```

## Benchmark Categories

### 2.1 Token Probability Tracking (10 benchmarks)

Tests the overhead of perplexity calculation and confidence aggregation:

- `BM_PerplexityCalculation_Small` - 50 tokens
- `BM_PerplexityCalculation_Medium` - 200 tokens
- `BM_PerplexityCalculation_Large` - 512 tokens
- `BM_SlidingWindowPerplexity_WindowSize10` - 100 tokens, window=10
- `BM_SlidingWindowPerplexity_WindowSize20` - 100 tokens, window=20
- `BM_OutlierDetection_NoOutliers` - Clean token probabilities
- `BM_OutlierDetection_WithOutliers` - With outlier tokens
- `BM_ConfidenceAggregation` - Geometric mean calculation

**Performance Target**: < 10ms overhead per LLM response

### 2.2 Self-Consistency Checking (6 benchmarks)

Tests consistency verification performance:

- `BM_SelfConsistency_3Samples` - With 3 samples
- `BM_SelfConsistency_5Samples` - With 5 samples
- `BM_SemanticSimilarity_ShortTexts` - Short text comparison
- `BM_SemanticSimilarity_LongTexts` - Long text comparison
- `BM_ContradictionDetection` - Negation pattern detection

**Performance Target**: < 2s for 5 samples (with GPU batch inference)

### 2.3 FLARE Active Retrieval (7 benchmarks)

Tests iterative retrieval performance:

- `BM_FLARE_Disabled` - Baseline without FLARE
- `BM_FLARE_SingleRound` - 1 retrieval round
- `BM_FLARE_ThreeRounds` - 3 retrieval rounds (default)
- `BM_SentenceSplitting_ShortText` - Short text processing
- `BM_SentenceSplitting_LongText` - Long text processing
- `BM_QueryReformulation` - Query reformulation overhead

**Performance Target**: < 500ms per retrieval round

### Configuration & Setup (5 benchmarks)

Tests detector creation and configuration overhead:

- `BM_DetectorCreation_Default` - Default configuration
- `BM_DetectorCreation_FastFactory` - Fast mode factory
- `BM_DetectorCreation_BalancedFactory` - Balanced mode factory
- `BM_DetectorCreation_ThoroughFactory` - Thorough mode factory
- `BM_ConfigurationUpdate` - Runtime configuration change

### Comprehensive Detection (3 benchmarks)

Tests end-to-end detection performance:

- `BM_ComprehensiveDetection_Fast` - Fast mode
- `BM_ComprehensiveDetection_Balanced` - Balanced mode (with token probabilities)
- `BM_ComprehensiveDetection_Thorough` - Thorough mode (all features enabled)

## Performance Targets

Based on Phase 2 requirements:

| Component | Target | Benchmark |
|-----------|--------|-----------|
| Token Probability Tracking | < 10ms | `BM_PerplexityCalculation_*` |
| Perplexity Calculation | < 5ms | `BM_PerplexityCalculation_*` |
| Self-Consistency (5 samples) | < 2s | `BM_SelfConsistency_5Samples` |
| FLARE Re-Retrieval | < 500ms/round | `BM_FLARE_ThreeRounds` |
| Total Overhead (complex queries) | < 3s | `BM_ComprehensiveDetection_Thorough` |

## Example Output

```
Run on (8 X 3400 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 1.23, 1.45, 1.67

---------------------------------------------------------------------------
Benchmark                                    Time             CPU   Iterations
---------------------------------------------------------------------------
BM_PerplexityCalculation_Small            2.34 ms         2.34 ms          299
BM_PerplexityCalculation_Medium           8.67 ms         8.66 ms           81
BM_PerplexityCalculation_Large           20.1 ms         20.1 ms           35
BM_SlidingWindowPerplexity_WindowSize10  3.12 ms         3.12 ms          224
BM_SelfConsistency_5Samples              125 ms          125 ms             6
BM_FLARE_ThreeRounds                     456 ms          456 ms             2
```

## Integration with CI/CD

Add to your CI pipeline:

```yaml
- name: Run Phase 2 Benchmarks
  run: |
    cd build
    ./benchmarks/bench_knowledge_gap_detector_phase2 \
      --benchmark_format=json \
      --benchmark_out=phase2_benchmark_results.json \
      --benchmark_min_time=0.5
    
- name: Check Performance Targets
  run: |
    python scripts/check_benchmark_targets.py phase2_benchmark_results.json
```

## Troubleshooting

### Benchmark Not Building

Ensure Google Benchmark is installed:
```bash
# Via vcpkg
vcpkg install benchmark

# Or via apt (Ubuntu/Debian)
sudo apt-get install libbenchmark-dev
```

### High Variance in Results

Run with more repetitions:
```bash
./benchmarks/bench_knowledge_gap_detector_phase2 \
  --benchmark_repetitions=20 \
  --benchmark_report_aggregates_only=true
```

### Benchmarks Taking Too Long

Use shorter minimum time:
```bash
./benchmarks/bench_knowledge_gap_detector_phase2 --benchmark_min_time=0.1
```

## Related Documentation

- [Phase 2 Implementation](../KNOWLEDGE_GAP_DETECTOR_PHASE2_COMPLETE.md)
- [Usage Guide](../docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_USAGE.md)
- [Integration Example](../examples/rag_knowledge_gap_integration.cpp)

## Contributing

When adding new Phase 2 features, please:
1. Add corresponding benchmarks to this suite
2. Document performance targets
3. Run benchmarks before and after optimizations
4. Include results in PR descriptions
