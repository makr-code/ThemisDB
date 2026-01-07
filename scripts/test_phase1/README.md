# Phase 1 Testing & Validation Scripts

**Purpose**: Test and validate Phase 1 LLM optimizations (Flash Attention, KV-Cache Reuse, Embeddings)

## Overview

This directory contains test scripts and benchmarks to validate that Phase 1 features work correctly and deliver the expected performance improvements.

## Prerequisites

- Issue #1 (Fix Compilation Infrastructure) must be resolved
- ThemisDB built with llama.cpp integration
- GGUF model files available (e.g., Mistral-7B-Instruct-Q4_K_M)
- GPU with CUDA support (for Flash Attention)

## Test Scripts

### 1. Flash Attention Tests
**Script**: `test_flash_attention.sh`

Tests Flash Attention functionality and performance:
- Configuration validation
- Inference correctness
- Performance benchmark (target: 15-25% speedup)
- Memory usage reduction (target: 30% reduction)
- Fallback mechanism

**Usage**:
```bash
./scripts/test_phase1/test_flash_attention.sh --model /models/mistral-7b-q4.gguf
```

### 2. KV-Cache Reuse Tests
**Script**: `test_kv_cache_reuse.sh`

Tests KV-Cache Reuse/Prefix Caching:
- Cache hit/miss logic
- LRU eviction
- Statistics API
- RAG workload simulation
- Performance benchmark (target: 10-20x first-token speedup)

**Usage**:
```bash
./scripts/test_phase1/test_kv_cache_reuse.sh --model /models/mistral-7b-q4.gguf
```

### 3. Embeddings Extraction Tests
**Script**: `test_embeddings_extraction.sh`

Tests embeddings extraction functionality:
- Embeddings dimension validation
- L2 normalization
- Semantic similarity
- Batch processing
- Performance measurement

**Usage**:
```bash
./scripts/test_phase1/test_embeddings_extraction.sh --model /models/mistral-7b-q4.gguf
```

### 4. Integration Tests
**Script**: `test_integration.sh`

Tests all Phase 1 features together:
- Combined feature activation
- RAG pipeline (embeddings + generation with Flash + Cache)
- Performance validation
- No conflicts between features

**Usage**:
```bash
./scripts/test_phase1/test_integration.sh --model /models/mistral-7b-q4.gguf
```

### 5. Main Test Runner
**Script**: `run_all_tests.sh`

Orchestrates all Phase 1 tests:
- Runs all test suites in sequence
- Collects results
- Generates report
- Validates acceptance criteria

**Usage**:
```bash
./scripts/test_phase1/run_all_tests.sh --model /models/mistral-7b-q4.gguf --output-dir ./results/phase1_benchmarks
```

## Expected Results

### Flash Attention
- **Speedup**: 15-25% faster inference
- **Memory**: 30% less VRAM usage
- **Accuracy**: No loss

### KV-Cache Reuse
- **First-token**: 10-20x faster on cache hits
- **Hit rate**: 60-70% (RAG workload)
- **Total speedup**: 40-60% reduction in inference time

### Embeddings
- **Dimension**: 4096 (Mistral-7B)
- **Normalization**: L2 normalized (magnitude ≈ 1.0)
- **Semantic**: Similar texts > 0.7 similarity
- **Throughput**: >10 sentences/sec

## Output

Test results are saved to:
- JSON: `results/phase1_benchmarks/phase1_results.json`
- HTML Report: `results/phase1_benchmarks/phase1_report.html`
- Logs: `results/phase1_benchmarks/test_logs/`

## Configuration Examples

See `config/llm_config.example.yaml` for test configurations with Phase 1 features enabled.

## Troubleshooting

### Flash Attention Not Available
- Check llama.cpp version (b2000+ required)
- Rebuild with CUDA support
- Verify GPU is detected

### Low Cache Hit Rate
- Increase cache size
- Lower similarity threshold
- Check workload for unique prompts

### Poor Embedding Quality
- Use instruction-tuned model
- Add retrieval prefix to text
- Check model compatibility

## References

- [Flash Attention Implementation](../../docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)
- [KV-Cache Reuse Implementation](../../docs/en/llm/KV_CACHE_REUSE_IMPLEMENTATION.md)
- [Embeddings Extraction Implementation](../../docs/en/llm/EMBEDDINGS_EXTRACTION_IMPLEMENTATION.md)
- [Phase 1 Implementation Summary](../../P1_IMPLEMENTATION_SUMMARY.md)
