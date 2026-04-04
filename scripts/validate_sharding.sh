#!/bin/bash
# ThemisDB v1.4 Distributed Sharding - Validation Script
# This script validates the distributed sharding implementation

echo "=================================================="
echo "ThemisDB v1.4 Distributed Sharding Validation"
echo "=================================================="
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
TESTS_PASSED=0
TESTS_FAILED=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
check_pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((TESTS_PASSED++))
}

check_fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((TESTS_FAILED++))
}

check_warn() {
    echo -e "${YELLOW}⚠ WARN${NC}: $1"
}

# Check 1: Verify source files exist
echo "=== Check 1: Source Files ==="
SOURCE_FILES=(
    "include/sharding/consensus_module.h"
    "include/sharding/consensus_factory.h"
    "include/sharding/raft_consensus_adapter.h"
    "include/sharding/gossip_consensus_adapter.h"
    "include/sharding/paxos_consensus.h"
    "include/sharding/cross_shard_transaction.h"
    "include/sharding/metadata_shard.h"
    "src/sharding/consensus_factory.cpp"
    "src/sharding/raft_consensus_adapter.cpp"
    "src/sharding/gossip_consensus_adapter.cpp"
    "src/sharding/paxos_consensus.cpp"
    "src/sharding/cross_shard_transaction.cpp"
)

for file in "${SOURCE_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        check_pass "Source file exists: ${file}"
    else
        check_fail "Source file missing: ${file}"
    fi
done
echo ""

# Check 2: Verify test files exist
echo "=== Check 2: Test Files ==="
TEST_FILES=(
    "tests/test_consensus_module.cpp"
)

for file in "${TEST_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        check_pass "Test file exists: ${file}"
    else
        check_fail "Test file missing: ${file}"
    fi
done
echo ""

# Check 3: Verify documentation exists
echo "=== Check 3: Documentation ==="
DOC_FILES=(
    "docs/de/sharding/CONSENSUS_MODULE.md"
    "docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md"
    "docs/de/migration/DATA_MIGRATION_COMPATIBILITY.md"
    "docs/de/sharding/QUICK_START_GUIDE.md"
    "DISTRIBUTED_SHARDING_IMPLEMENTATION_SUMMARY.md"
    "INTEGRATION_CHECKLIST.md"
)

for file in "${DOC_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        check_pass "Documentation exists: ${file}"
    else
        check_fail "Documentation missing: ${file}"
    fi
done
echo ""

# Check 4: Verify examples exist
echo "=== Check 4: Examples ==="
EXAMPLE_FILES=(
    "examples/distributed_sharding/distributed_sharding_example.cpp"
    "examples/distributed_sharding/README.md"
)

for file in "${EXAMPLE_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        check_pass "Example exists: ${file}"
    else
        check_fail "Example missing: ${file}"
    fi
done
echo ""

# Check 5: Verify CMake integration
echo "=== Check 5: CMake Integration ==="

if grep -q "consensus_factory.cpp" "${SCRIPT_DIR}/cmake/CMakeLists.txt"; then
    check_pass "consensus_factory.cpp in CMakeLists.txt"
else
    check_fail "consensus_factory.cpp not in CMakeLists.txt"
fi

if grep -q "raft_consensus_adapter.cpp" "${SCRIPT_DIR}/cmake/CMakeLists.txt"; then
    check_pass "raft_consensus_adapter.cpp in CMakeLists.txt"
else
    check_fail "raft_consensus_adapter.cpp not in CMakeLists.txt"
fi

if grep -q "test_consensus_module" "${SCRIPT_DIR}/tests/CMakeLists.txt"; then
    check_pass "test_consensus_module in tests/CMakeLists.txt"
else
    check_fail "test_consensus_module not in tests/CMakeLists.txt"
fi
echo ""

# Check 6: Verify header guards
echo "=== Check 6: Header Guards ==="
HEADER_FILES=(
    "include/sharding/consensus_module.h"
    "include/sharding/consensus_factory.h"
    "include/sharding/raft_consensus_adapter.h"
    "include/sharding/gossip_consensus_adapter.h"
    "include/sharding/paxos_consensus.h"
    "include/sharding/cross_shard_transaction.h"
)

for file in "${HEADER_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        if grep -q "#ifndef" "${SCRIPT_DIR}/${file}" && grep -q "#define" "${SCRIPT_DIR}/${file}" && grep -q "#endif" "${SCRIPT_DIR}/${file}"; then
            check_pass "Header guard present: ${file}"
        else
            check_fail "Header guard missing/incomplete: ${file}"
        fi
    fi
done
echo ""

# Check 7: Verify copyright headers
echo "=== Check 7: Copyright Headers ==="
ALL_SOURCE_FILES=("${SOURCE_FILES[@]}" "${TEST_FILES[@]}")

for file in "${ALL_SOURCE_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        if head -n 5 "${SCRIPT_DIR}/${file}" | grep -q "Copyright"; then
            check_pass "Copyright header present: ${file}"
        else
            check_warn "Copyright header missing: ${file}"
        fi
    fi
done
echo ""

# Check 8: Verify TODO markers in stubs
echo "=== Check 8: TODO Markers ==="
STUB_FILES=(
    "src/sharding/raft_consensus_adapter.cpp"
    "src/sharding/paxos_consensus.cpp"
    "src/sharding/cross_shard_transaction.cpp"
)

TODO_COUNT=0
for file in "${STUB_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        COUNT=$(grep -c "TODO:" "${SCRIPT_DIR}/${file}" || true)
        if [ "$COUNT" -gt 0 ]; then
            check_pass "TODO markers found in ${file}: ${COUNT}"
            TODO_COUNT=$((TODO_COUNT + COUNT))
        else
            check_warn "No TODO markers in ${file}"
        fi
    fi
done
echo "Total TODO markers: ${TODO_COUNT}"
echo ""

# Check 9: Code statistics
echo "=== Check 9: Code Statistics ==="
TOTAL_LOC=0
for file in "${SOURCE_FILES[@]}" "${TEST_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        LOC=$(wc -l < "${SCRIPT_DIR}/${file}")
        TOTAL_LOC=$((TOTAL_LOC + LOC))
    fi
done
echo "Total lines of code (headers + implementation + tests): ${TOTAL_LOC}"
check_pass "Code statistics calculated"
echo ""

# Check 10: Documentation statistics
echo "=== Check 10: Documentation Statistics ==="
TOTAL_DOC_LOC=0
for file in "${DOC_FILES[@]}"; do
    if [ -f "${SCRIPT_DIR}/${file}" ]; then
        LOC=$(wc -l < "${SCRIPT_DIR}/${file}")
        TOTAL_DOC_LOC=$((TOTAL_DOC_LOC + LOC))
    fi
done
echo "Total lines of documentation: ${TOTAL_DOC_LOC}"
check_pass "Documentation statistics calculated"
echo ""

# Summary
echo "=================================================="
echo "Validation Summary"
echo "=================================================="
echo -e "Tests Passed: ${GREEN}${TESTS_PASSED}${NC}"
echo -e "Tests Failed: ${RED}${TESTS_FAILED}${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All validation checks passed!${NC}"
    echo ""
    echo "Next Steps:"
    echo "1. Build the project: cmake -B build && cmake --build build"
    echo "2. Run tests: ./build/tests/test_consensus_module"
    echo "3. Review INTEGRATION_CHECKLIST.md for deployment steps"
    exit 0
else
    echo -e "${RED}✗ Some validation checks failed${NC}"
    echo "Please review the failed checks above"
    exit 1
fi
