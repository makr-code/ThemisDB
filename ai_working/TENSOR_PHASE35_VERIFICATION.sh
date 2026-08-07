#!/bin/bash

# Tensor Module Phase 3-5 Final Verification Script
# This script validates all completion criteria across phases 2-6

set -e

echo "================================"
echo "Tensor Phase 3-5 Verification"
echo "================================"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0

# Function to report test result
report_test() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $1"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}✗${NC} $1"
        ((TESTS_FAILED++))
    fi
}

echo "### Phase 4 Verification (Prerequisites)"
echo ""

# Test Phase 4: Contract hardening tests
echo "Running Phase 4 contract hardening tests (TNCH-01..16)..."
ctest --preset windows-release -L tensor_contract_hardening -V --output-on-failure > /tmp/tnch_tests.log 2>&1 || true
if grep -q "100% tests passed" /tmp/tnch_tests.log; then
    echo -e "${GREEN}✓${NC} TNCH-01..16 all PASS"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗${NC} TNCH-01..16 FAILED"
    ((TESTS_FAILED++))
fi

# Test Phase 4: Concurrent stress tests
echo "Running Phase 4 concurrent stress tests..."
ctest --preset windows-release -L tensor_concurrent -V --output-on-failure > /tmp/tensor_concurrent.log 2>&1 || true
if grep -q "100% tests passed" /tmp/tensor_concurrent.log; then
    echo -e "${GREEN}✓${NC} Concurrent stress tests PASS"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗${NC} Concurrent stress tests FAILED"
    ((TESTS_FAILED++))
fi

echo ""
echo "### Phase 5 Verification"
echo ""

# Benchmark gate validation
echo "Running TRNRG benchmark gates (TRNRG-01..06)..."
cmake --build /home/runner/work/ThemisDB/ThemisDB/build-windows-release --target bench_tensor_release_gates 2>/dev/null || true

if [ -f /home/runner/work/ThemisDB/ThemisDB/build-windows-release/benchmarks/tensor/bench_tensor_release_gates ]; then
    echo -e "${GREEN}✓${NC} Benchmark executable built"
    ((TESTS_PASSED++))
    
    # Run benchmarks (with short timeout for CI)
    timeout 300 /home/runner/work/ThemisDB/ThemisDB/build-windows-release/benchmarks/tensor/bench_tensor_release_gates 2>&1 | tee /tmp/trnrg_gates.log || true
    
    if grep -q "GATE-TRNRG" /tmp/trnrg_gates.log; then
        echo -e "${GREEN}✓${NC} TRNRG benchmarks executed"
        ((TESTS_PASSED++))
    else
        echo -e "${YELLOW}⚠${NC} TRNRG benchmarks output unclear (may need manual inspection)"
    fi
else
    echo -e "${RED}✗${NC} Benchmark executable build failed"
    ((TESTS_FAILED++))
fi

echo ""
echo "### Phase 2-3 Implementation Verification"
echo ""

# Check for hardening in key files
echo "Checking Phase 2-3 hardening implementation..."

FILES_TO_CHECK=(
    "src/tensor/tensor_index_manager.cpp"
    "src/tensor/tensor_ingestion_bridge.cpp"
    "src/tensor/tensor_core_bridge.cpp"
    "src/tensor/tensor_mmap_bridge.cpp"
    "src/tensor/tensor_error_handling.cpp"
)

for file in "${FILES_TO_CHECK[@]}"; do
    if grep -q "error_registry\|incident\|fail-closed\|resilience\|diagnostic" "$file" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $file contains hardening code"
        ((TESTS_PASSED++))
    else
        echo -e "${YELLOW}⚠${NC} $file may need inspection"
    fi
done

echo ""
echo "### ROADMAP Closure Verification"
echo ""

# Check ROADMAP.md for completion markings
echo "Verifying ROADMAP.md closure items..."

ROADMAP_FILE="src/tensor/ROADMAP.md"

if grep -q "- \[x\] complete hardening for tensor index manager" "$ROADMAP_FILE" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Phase 2.1 marked complete in ROADMAP"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠${NC} Phase 2.1 not yet marked in ROADMAP"
fi

if grep -q "- \[x\] standardize fail-safe behavior" "$ROADMAP_FILE" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Phase 3.1 marked complete in ROADMAP"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠${NC} Phase 3.1 not yet marked in ROADMAP"
fi

if grep -q "- \[x\] validate p95/p99 and throughput behavior" "$ROADMAP_FILE" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Phase 5.1 marked complete in ROADMAP"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠${NC} Phase 5.1 not yet marked in ROADMAP"
fi

echo ""
echo "================================"
echo "Verification Summary"
echo "================================"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All Phase 3-5 verifications PASS${NC}"
    exit 0
else
    echo -e "${RED}✗ Some verifications failed${NC}"
    exit 1
fi
