#!/bin/bash
# Test execution script for distributed_tensor module
# Purpose: Build and run all focused tests to verify gap closure

set -e

REPO_ROOT="${1:-.}"
BUILD_PRESET="${2:-windows-release}"
PARALLEL_JOBS="${3:-4}"
TIMEOUT_SECS="${4:-300}"

echo "=== Distributed Tensor Module - Build & Test Execution ===" 
echo "Repository: $REPO_ROOT"
echo "Preset: $BUILD_PRESET"
echo "Parallel Jobs: $PARALLEL_JOBS"
echo "Test Timeout: $TIMEOUT_SECS seconds"
echo "Timestamp: $(date -Iseconds)"
echo ""

# Change to repo directory
cd "$REPO_ROOT"

# Step 1: Configure CMake
echo "[STEP 1] Configuring CMake with preset: $BUILD_PRESET"
cmake --preset "$BUILD_PRESET" 2>&1 | tail -20

# Step 2: Build distributed_tensor module
echo ""
echo "[STEP 2] Building distributed_tensor module"
cmake --build --preset "$BUILD_PRESET" --target theme sis_distributed_tensor --parallel "$PARALLEL_JOBS" 2>&1 | tail -30

# Step 3: Run focused test targets
echo ""
echo "[STEP 3] Running distributed_tensor focused tests"

TESTS=(
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

PASSED=0
FAILED=0
FAILED_TESTS=()

for test in "${TESTS[@]}"; do
    echo "  Running: $test"
    if timeout "$TIMEOUT_SECS" ctest --preset "$BUILD_PRESET" -R "^${test}$" --output-on-failure -j "$PARALLEL_JOBS" 2>&1 | grep -E "Passed|Failed|Skipped"; then
        ((PASSED++))
    else
        ((FAILED++))
        FAILED_TESTS+=("$test")
    fi
done

echo ""
echo "[STEP 4] Test Summary"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "Failed tests:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  - $test"
    done
    exit 1
else
    echo ""
    echo "✓ All tests passed!"
    exit 0
fi
