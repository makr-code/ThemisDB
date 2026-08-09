#!/usr/bin/env bash
#
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 ThemisDB Contributors
#
# Phase 2B: ONNX CLIP Baseline Measurement Script
#
# Automates baseline collection for all 27 benchmarks:
# - 7 Phase 2A benchmarks
# - 3 Phase 2B latency regression benchmarks
# - 4 Phase 2B initialization profiling benchmarks
# - 10 Phase 2B throughput scaling benchmarks
# - 8 Phase 2B memory scaling benchmarks
# - 4 Phase 2B batch-splitting benchmarks (reusing from Phase 2A)
#
# Usage:
#   ./run_baseline.sh [--output-dir OUTDIR] [--filter FILTER]
#
# Examples:
#   # Run all baselines
#   ./run_baseline.sh
#
#   # Run only CPU benchmarks
#   ./run_baseline.sh --filter "CPU"
#
#   # Save results to custom directory
#   ./run_baseline.sh --output-dir /tmp/baseline_results
#

set -euo pipefail

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Configuration
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_ROOT}/benchmarks/onnx_clip/baseline_results}"
FILTER="${FILTER:-}"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build-community-debug-allow-missing-rocksdb}"
TIMESTAMP=$(date -u +"%Y%m%d_%H%M%SZ")

# Benchmark executables
CPU_BENCH="${BUILD_DIR}/benchmarks/onnx_clip/bench_onnx_clip_cpu"
BACKEND_BENCH="${BUILD_DIR}/benchmarks/onnx_clip/bench_onnx_clip_vit_backend"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

usage() {
    cat << EOF
Usage: ${BASH_SOURCE[0]} [OPTIONS]

Options:
  --output-dir DIR    Output directory for baseline results (default: ${OUTPUT_DIR})
  --filter FILTER     Filter benchmarks (e.g., "Latency", "Memory", "Scaling")
  --help              Show this help message

Phase 2B Baseline Collection Script
EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --filter)
            FILTER="$2"
            shift 2
            ;;
        --help)
            usage
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Create output directory
mkdir -p "${OUTPUT_DIR}"
log_info "Output directory: ${OUTPUT_DIR}"

# Check if benchmarks exist
if [[ ! -f "${CPU_BENCH}" ]]; then
    log_error "CPU benchmark not found: ${CPU_BENCH}"
    log_info "Run: cmake --build ${BUILD_DIR} --target bench_onnx_clip_cpu"
    exit 1
fi

if [[ ! -f "${BACKEND_BENCH}" ]]; then
    log_error "Backend benchmark not found: ${BACKEND_BENCH}"
    log_info "Run: cmake --build ${BUILD_DIR} --target bench_onnx_clip_vit_backend"
    exit 1
fi

log_info "Found benchmarks:"
log_info "  CPU: ${CPU_BENCH}"
log_info "  Backend: ${BACKEND_BENCH}"

# Create baseline metadata
METADATA_FILE="${OUTPUT_DIR}/metadata_${TIMESTAMP}.json"
cat > "${METADATA_FILE}" << EOF
{
  "timestamp": "${TIMESTAMP}",
  "hostname": "$(hostname)",
  "os": "$(uname -s)",
  "cpu_model": "$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null || echo 'Unknown')",
  "cpu_cores": $(nproc 2>/dev/null || echo 'Unknown'),
  "memory_gb": $(free -g 2>/dev/null | awk 'NR==2 {print $2}' || echo 'Unknown'),
  "build_dir": "${BUILD_DIR}",
  "phase": "2B",
  "version": "1.0.0"
}
EOF
log_info "Metadata: ${METADATA_FILE}"

# Phase 2A Benchmarks (7 total)
log_info "Running Phase 2A CPU latency benchmarks..."
if [[ -z "${FILTER}" || "${FILTER}" =~ "Latency" || "${FILTER}" =~ "Phase2A" ]]; then
    "${CPU_BENCH}" \
        --benchmark_out="${OUTPUT_DIR}/phase2a_cpu_${TIMESTAMP}.json" \
        --benchmark_out_format=json \
        --benchmark_counters_tabular=true 2>&1 | tee "${OUTPUT_DIR}/phase2a_cpu_${TIMESTAMP}.log"
    log_info "Phase 2A CPU benchmarks complete"
fi

log_info "Running Phase 2A backend benchmarks..."
if [[ -z "${FILTER}" || "${FILTER}" =~ "Throughput" || "${FILTER}" =~ "Memory" || "${FILTER}" =~ "Phase2A" ]]; then
    "${BACKEND_BENCH}" \
        --benchmark_out="${OUTPUT_DIR}/phase2a_backend_${TIMESTAMP}.json" \
        --benchmark_out_format=json \
        --benchmark_counters_tabular=true 2>&1 | tee "${OUTPUT_DIR}/phase2a_backend_${TIMESTAMP}.log"
    log_info "Phase 2A backend benchmarks complete"
fi

# Phase 2B Latency Regression Benchmarks (3 total)
log_info "Running Phase 2B latency regression benchmarks..."
if [[ -z "${FILTER}" || "${FILTER}" =~ "Regression" || "${FILTER}" =~ "Phase2B" ]]; then
    "${CPU_BENCH}" \
        --benchmark_filter="Latency_Regression" \
        --benchmark_out="${OUTPUT_DIR}/phase2b_latency_regression_${TIMESTAMP}.json" \
        --benchmark_out_format=json \
        --benchmark_counters_tabular=true 2>&1 | tee "${OUTPUT_DIR}/phase2b_latency_regression_${TIMESTAMP}.log"
    log_info "Phase 2B latency regression benchmarks complete"
fi

# Phase 2B Initialization Profiling Benchmarks (4 total)
log_info "Running Phase 2B initialization profiling benchmarks..."
if [[ -z "${FILTER}" || "${FILTER}" =~ "Init" || "${FILTER}" =~ "Phase2B" ]]; then
    "${CPU_BENCH}" \
        --benchmark_filter="InitTime" \
        --benchmark_out="${OUTPUT_DIR}/phase2b_init_${TIMESTAMP}.json" \
        --benchmark_out_format=json \
        --benchmark_counters_tabular=true 2>&1 | tee "${OUTPUT_DIR}/phase2b_init_${TIMESTAMP}.log"
    log_info "Phase 2B initialization profiling benchmarks complete"
fi

# Phase 2B Throughput Scaling Benchmarks (10 total)
log_info "Running Phase 2B throughput scaling benchmarks..."
if [[ -z "${FILTER}" || "${FILTER}" =~ "Scaling" || "${FILTER}" =~ "Phase2B" ]]; then
    "${BACKEND_BENCH}" \
        --benchmark_filter="ThroughputScaling" \
        --benchmark_out="${OUTPUT_DIR}/phase2b_throughput_scaling_${TIMESTAMP}.json" \
        --benchmark_out_format=json \
        --benchmark_counters_tabular=true 2>&1 | tee "${OUTPUT_DIR}/phase2b_throughput_scaling_${TIMESTAMP}.log"
    log_info "Phase 2B throughput scaling benchmarks complete"
fi

# Phase 2B Memory Scaling Benchmarks (8 total)
log_info "Running Phase 2B memory scaling benchmarks..."
if [[ -z "${FILTER}" || "${FILTER}" =~ "MemoryScaling" || "${FILTER}" =~ "Phase2B" ]]; then
    "${BACKEND_BENCH}" \
        --benchmark_filter="MemoryScaling" \
        --benchmark_out="${OUTPUT_DIR}/phase2b_memory_scaling_${TIMESTAMP}.json" \
        --benchmark_out_format=json \
        --benchmark_counters_tabular=true 2>&1 | tee "${OUTPUT_DIR}/phase2b_memory_scaling_${TIMESTAMP}.log"
    log_info "Phase 2B memory scaling benchmarks complete"
fi

# Summary
log_info "=========================================="
log_info "Baseline collection complete!"
log_info "=========================================="
log_info "Results directory: ${OUTPUT_DIR}"
log_info "Timestamp: ${TIMESTAMP}"
log_info ""
log_info "Generated files:"
ls -lh "${OUTPUT_DIR}" | tail -n +2 | awk '{print "  " $9 " (" $5 ")"}'
log_info ""
log_info "Next steps:"
log_info "  1. Review JSON results for anomalies"
log_info "  2. Compare against baselines.json thresholds"
log_info "  3. Document any architecture-specific deviations"
log_info "  4. For regression detection: use analyze_baselines.py"

exit 0
