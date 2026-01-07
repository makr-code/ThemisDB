#!/bin/bash
# Test entrypoint script for LLM RAID Pipeline tests/benchmarks

set -e

TEST_TYPE="${1:-pipeline}"
TEST_RESULTS="/test_results"
TEST_DATA="/test_data"
TEST_MODELS="/test_models"
TEST_LORAS="/test_loras"

# Create directories
mkdir -p "$TEST_RESULTS" "$TEST_DATA" "$TEST_MODELS" "$TEST_LORAS"

echo "=========================================="
echo "ThemisDB LLM RAID Pipeline Test Suite"
echo "=========================================="
echo "Test Type: $TEST_TYPE"
echo "Results Dir: $TEST_RESULTS"
echo "Test Data Dir: $TEST_DATA"
echo "=========================================="

cd "$TEST_RESULTS"

case "$TEST_TYPE" in
    "pipeline")
        echo "Running: Full RAID LoRA Pipeline Tests..."
        test_llm_raid_pipeline --gtest_output=xml:pipeline_results.xml
        exit_code=$?
        echo "Pipeline tests completed with exit code: $exit_code"
        ;;
    
    "inline")
        echo "Running: Inline LoRA Tests..."
        test_llm_lora_inline --gtest_output=xml:inline_results.xml
        exit_code=$?
        echo "Inline tests completed with exit code: $exit_code"
        ;;
    
    "bench_lora")
        echo "Running: LoRA Inline Benchmarks..."
        bench_lora_inline \
            --benchmark_out=lora_inline_results.json \
            --benchmark_out_format=json \
            --benchmark_time_unit=ms
        exit_code=$?
        echo "LoRA benchmarks completed with exit code: $exit_code"
        ;;
    
    "bench_pipeline")
        echo "Running: RAID Pipeline Benchmarks..."
        bench_llm_raid_pipeline \
            --benchmark_out=raid_pipeline_results.json \
            --benchmark_out_format=json \
            --benchmark_time_unit=ms
        exit_code=$?
        echo "Pipeline benchmarks completed with exit code: $exit_code"
        ;;
    
    "all_tests")
        echo "Running: All Unit Tests..."
        test_llm_lora_inline --gtest_output=xml:inline_results.xml
        test_llm_raid_pipeline --gtest_output=xml:pipeline_results.xml
        exit_code=$?
        echo "All tests completed with exit code: $exit_code"
        ;;
    
    "all_bench")
        echo "Running: All Benchmarks..."
        bench_lora_inline \
            --benchmark_out=lora_inline_results.json \
            --benchmark_out_format=json
        bench_llm_raid_pipeline \
            --benchmark_out=raid_pipeline_results.json \
            --benchmark_out_format=json
        exit_code=$?
        echo "All benchmarks completed with exit code: $exit_code"
        ;;
    
    "all")
        echo "Running: All Tests + Benchmarks..."
        test_llm_lora_inline --gtest_output=xml:inline_results.xml
        test_llm_raid_pipeline --gtest_output=xml:pipeline_results.xml
        bench_lora_inline \
            --benchmark_out=lora_inline_results.json \
            --benchmark_out_format=json
        bench_llm_raid_pipeline \
            --benchmark_out=raid_pipeline_results.json \
            --benchmark_out_format=json
        exit_code=$?
        echo "All tests and benchmarks completed with exit code: $exit_code"
        ;;
    
    *)
        echo "Unknown test type: $TEST_TYPE"
        echo "Available options:"
        echo "  pipeline        - Full RAID LoRA Pipeline Tests"
        echo "  inline          - Inline LoRA Tests"
        echo "  bench_lora      - LoRA Inline Benchmarks"
        echo "  bench_pipeline  - RAID Pipeline Benchmarks"
        echo "  all_tests       - All Unit Tests"
        echo "  all_bench       - All Benchmarks"
        echo "  all             - All Tests + Benchmarks"
        exit 1
        ;;
esac

# Generate summary
echo ""
echo "=========================================="
echo "Test Results Summary"
echo "=========================================="
ls -lh "$TEST_RESULTS"
echo "=========================================="

exit $exit_code
