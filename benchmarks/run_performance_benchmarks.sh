#!/bin/bash
# run_performance_benchmarks.sh
# Automated runner for ThemisDB real performance benchmarks
# Generates JSON output for CI regression tracking

set -e

# Configuration
OUTPUT_DIR="${OUTPUT_DIR:-benchmark_results}"
BUILD_DIR="${BUILD_DIR:-build}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_FILE="$OUTPUT_DIR/results_$TIMESTAMP.json"

# Benchmark executables
BENCHMARKS=(
    "bench_storage_performance"
    "bench_olap_performance"
    "bench_embedding_cache_performance"
    "bench_llm_inference_performance"
)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print header
echo "================================================"
echo "ThemisDB Real Performance Benchmarks"
echo "================================================"
echo "Timestamp: $TIMESTAMP"
echo "Output directory: $OUTPUT_DIR"
echo "Build directory: $BUILD_DIR"
echo "================================================"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Function to run a benchmark
run_benchmark() {
    local bench_name=$1
    local bench_path="$BUILD_DIR/benchmarks/$bench_name"
    local output_file="$OUTPUT_DIR/${bench_name}_${TIMESTAMP}.json"
    
    if [ ! -f "$bench_path" ]; then
        echo -e "${RED}[SKIP]${NC} $bench_name (not found)"
        return 1
    fi
    
    echo -e "${YELLOW}[RUN]${NC} $bench_name"
    
    # Run benchmark with JSON output
    if "$bench_path" \
        --benchmark_out="$output_file" \
        --benchmark_out_format=json \
        --benchmark_repetitions=3 \
        --benchmark_report_aggregates_only=true; then
        if ! python3 -c 'import json, math, sys
with open(sys.argv[1], encoding="utf-8") as result_file:
    result = json.load(result_file)
measurements = result.get("benchmarks") if isinstance(result, dict) else None
if not isinstance(measurements, list) or not measurements:
    raise SystemExit("benchmark JSON contains no measurements")
for row in measurements:
    value = row.get("real_time") if isinstance(row, dict) else None
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise SystemExit("benchmark JSON contains a measurement without finite numeric real_time")' "$output_file"; then
            echo -e "${RED}[FAIL]${NC} $bench_name (missing or invalid measurement JSON)"
            return 1
        fi

        echo -e "${GREEN}[PASS]${NC} $bench_name"
        echo "  Output: $output_file"
        return 0
    else
        echo -e "${RED}[FAIL]${NC} $bench_name"
        return 1
    fi
}

# Run all benchmarks
echo "Running benchmarks..."
echo ""

success_count=0
fail_count=0
skip_count=0
VALIDATED_BENCHMARKS=()

for bench in "${BENCHMARKS[@]}"; do
    if run_benchmark "$bench"; then
        success_count=$((success_count + 1))
        VALIDATED_BENCHMARKS+=("$bench")
    else
        if [ -f "$BUILD_DIR/benchmarks/$bench" ]; then
            fail_count=$((fail_count + 1))
        else
            skip_count=$((skip_count + 1))
        fi
    fi
    echo ""
done

# Combine results into valid JSON
echo "================================================"
echo "Combining results..."
echo "================================================"

combined_file="$OUTPUT_DIR/all_benchmarks_${TIMESTAMP}.json"
{
    echo "{"
    echo "  \"context\": {"
    echo "    \"timestamp\": \"$TIMESTAMP\","
    echo "    \"build_dir\": \"$BUILD_DIR\","
    echo "    \"hostname\": \"$(hostname)\","
    echo "    \"os\": \"$(uname -s)\","
    echo "    \"architecture\": \"$(uname -m)\""
    echo "  },"
    echo "  \"benchmark_files\": ["
    
    first=true
    for bench in "${VALIDATED_BENCHMARKS[@]}"; do
        result_file="$OUTPUT_DIR/${bench}_${TIMESTAMP}.json"
        if [ -f "$result_file" ]; then
            if [ "$first" = false ]; then
                echo ","
            fi
            first=false
            echo "    {"
            echo "      \"name\": \"$bench\","
            echo "      \"file\": \"$result_file\""
            echo -n "    }"
        fi
    done
    
    echo ""
    echo "  ]"
    echo "}"
} > "$combined_file"

echo "Combined results saved to: $combined_file"
echo ""

# Summary
echo "================================================"
echo "Summary"
echo "================================================"
echo -e "Successful: ${GREEN}$success_count${NC}"
echo -e "Failed:     ${RED}$fail_count${NC}"
echo -e "Skipped:    ${YELLOW}$skip_count${NC}"
echo "Total:      $((success_count + fail_count + skip_count))"
echo "================================================"

# Generate human-readable report
echo ""
echo "Generating human-readable report..."
report_file="$OUTPUT_DIR/report_${TIMESTAMP}.txt"

{
    echo "ThemisDB Performance Benchmark Report"
    echo "====================================="
    echo ""
    echo "Timestamp: $TIMESTAMP"
    echo "Hostname: $(hostname)"
    echo "OS: $(uname -s)"
    echo "Architecture: $(uname -m)"
    echo ""
    echo "Benchmarks Run: $success_count"
    echo "Failed: $fail_count"
    echo "Skipped: $skip_count"
    echo ""
    echo "====================================="
    echo ""
    
    for bench in "${VALIDATED_BENCHMARKS[@]}"; do
        result_file="$OUTPUT_DIR/${bench}_${TIMESTAMP}.json"
        if [ -f "$result_file" ]; then
            echo "Benchmark: $bench"
            echo "-------------------------------------"
            # Extract key metrics from JSON (requires jq if available)
            if command -v jq &> /dev/null; then
                jq -r '.benchmarks[] | "  \(.name): \(.real_time) \(.time_unit)"' "$result_file" 2>/dev/null || echo "  (JSON parsing failed)"
            else
                echo "  (Install jq for detailed metrics)"
            fi
            echo ""
        fi
    done
} > "$report_file"

echo "Report saved to: $report_file"

# Exit with appropriate code
if [ "$fail_count" -gt 0 ]; then
    exit 1
fi

# A collection of skips is not a successful benchmark run.
if [ "$success_count" -eq 0 ]; then
    echo "No benchmark produced a validated measurement."
    exit 2
fi

exit 0
