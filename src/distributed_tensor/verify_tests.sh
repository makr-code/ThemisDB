#!/bin/bash
# Test Verification Script for Distributed Tensor Module (issue #5640)
# 
# This script verifies that all focused test targets are properly built and executable.
# Run this after building the project with one of the available CMake presets.
#
# Usage: ./src/distributed_tensor/verify_tests.sh <build_dir>
# Example: ./src/distributed_tensor/verify_tests.sh build-linux-release
#

set -e

BUILD_DIR="${1:-build-linux-release}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory '$BUILD_DIR' not found"
    echo "Usage: $0 <build_dir>"
    exit 1
fi

echo "=========================================="
echo "Distributed Tensor Module - Test Verification"
echo "=========================================="
echo "Build Directory: $BUILD_DIR"
echo ""

# Expected test targets
declare -a UNIT_TEST_TARGETS=(
    "module_epic3_distributed_tensor_distributed_planner_test_focused"
    "module_epic3_distributed_tensor_manifest_store_phase_a_focused"
    "module_epic3_distributed_tensor_lifecycle_staleness_management_focused"
    "module_epic3_distributed_tensor_tensor_delta_log_focused"
    "module_epic3_distributed_tensor_tensor_rebuild_fallback_focused"
    "module_epic3_distributed_tensor_phase3_failure_semantics_focused"
    "module_epic3_distributed_tensor_phase4_contract_coverage_focused"
    "module_epic3_distributed_tensor_tensor_storage_strategy_focused"
    "module_epic3_distributed_tensor_tensor_training_coordinator_focused"
    "module_epic3_distributed_tensor_tensor_update_worker_focused"
    "module_epic3_distributed_tensor_integrity_verification_test_focused"
)

declare -a BENCHMARK_TARGETS=(
    "module_epic3_distributed_tensor_integrity_verification_bench_focused"
)

echo "Phase 1: Verifying Unit Test Binaries"
echo "======================================"
MISSING_COUNT=0
FOUND_COUNT=0

for target in "${UNIT_TEST_TARGETS[@]}"; do
    # Try different possible locations
    if [ -f "$BUILD_DIR/tests/epic3_distributed_tensor/$target" ]; then
        echo "  ✓ $target"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/bin/$target" ]; then
        echo "  ✓ $target (in bin/)"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/bin_out/$target" ]; then
        echo "  ✓ $target (in bin_out/)"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/$target" ]; then
        echo "  ✓ $target"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/tests/epic3_distributed_tensor/${target}.exe" ]; then
        echo "  ✓ $target.exe (Windows)"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/tests/epic3_distributed_tensor/$target.out" ]; then
        echo "  ✓ $target.out"
        ((FOUND_COUNT++))
    else
        echo "  ✗ $target NOT FOUND"
        ((MISSING_COUNT++))
    fi
done

echo ""
echo "Phase 2: Verifying Benchmark Binaries"
echo "======================================"
for target in "${BENCHMARK_TARGETS[@]}"; do
    if [ -f "$BUILD_DIR/tests/epic3_distributed_tensor/$target" ]; then
        echo "  ✓ $target"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/bin/$target" ]; then
        echo "  ✓ $target (in bin/)"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/bin_out/$target" ]; then
        echo "  ✓ $target (in bin_out/)"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/tests/epic3_distributed_tensor/${target}.exe" ]; then
        echo "  ✓ $target.exe (Windows)"
        ((FOUND_COUNT++))
    elif [ -f "$BUILD_DIR/tests/epic3_distributed_tensor/$target.out" ]; then
        echo "  ✓ $target.out"
        ((FOUND_COUNT++))
    else
        echo "  ✗ $target NOT FOUND"
        ((MISSING_COUNT++))
    fi
done

echo ""
echo "=========================================="
echo "Summary"
echo "=========================================="
EXPECTED=$((${#UNIT_TEST_TARGETS[@]} + ${#BENCHMARK_TARGETS[@]}))
echo "Expected test targets: $EXPECTED"
echo "Found: $FOUND_COUNT"
echo "Missing: $MISSING_COUNT"

if [ $MISSING_COUNT -eq 0 ]; then
    echo ""
    echo "✓ All test targets found!"
    echo ""
    echo "Next Steps:"
    echo "  1. Run unit tests:"
    echo "     cd $BUILD_DIR && ctest -L epic3_distributed_tensor -V"
    echo ""
    echo "  2. Run specific test:"
    echo "     ./$BUILD_DIR/tests/epic3_distributed_tensor/module_epic3_distributed_tensor_phase4_contract_coverage_focused"
    echo ""
    echo "  3. Run benchmark:"
    echo "     ./$BUILD_DIR/tests/epic3_distributed_tensor/module_epic3_distributed_tensor_integrity_verification_bench_focused"
    exit 0
else
    echo ""
    echo "✗ Some test targets are missing"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Verify the build completed successfully"
    echo "  2. Check for build errors in CMake output"
    echo "  3. Verify all dependencies are installed"
    echo "  4. Check that tests/epic3_distributed_tensor/CMakeLists.txt exists"
    exit 1
fi
