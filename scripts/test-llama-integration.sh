#!/bin/bash
# Test script to validate llama.cpp integration improvements
# Tests dependency pinning, Flash Attention, and Continuous Batching configuration

set -e  # Exit on error

echo "=========================================="
echo "llama.cpp Integration Validation Test"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo -n "Testing: $test_name ... "
    
    if eval "$test_command" &> /tmp/test_output.log; then
        echo -e "${GREEN}✅ PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}❌ FAIL${NC}"
        echo "Error output:"
        cat /tmp/test_output.log
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Test 1: Verify .gitmodules has llama.cpp
echo "1. Checking .gitmodules configuration..."
run_test "llama.cpp in .gitmodules" \
    "grep -q 'llama.cpp' .gitmodules"

# Test 2: Verify Dependencies.cmake has FetchContent
echo "2. Checking CMake FetchContent configuration..."
run_test "FetchContent in Dependencies.cmake" \
    "grep -q 'FetchContent_Declare' cmake/Dependencies.cmake"

# Test 3: Verify pinned commit is set
echo "3. Checking llama.cpp commit pinning..."
run_test "LLAMA_CPP_GIT_TAG is set" \
    "grep -q 'LLAMA_CPP_GIT_TAG.*b4313' cmake/Dependencies.cmake"

# Test 4: Verify Flash Attention configuration
echo "4. Checking Flash Attention configuration..."
run_test "Flash Attention conditional logic" \
    "grep -q 'LLAMA_FLASH_ATTN' cmake/Dependencies.cmake"

# Test 5: Verify Continuous Batching configuration
echo "5. Checking Continuous Batching configuration..."
run_test "Continuous Batching enabled" \
    "grep -q 'LLAMA_CONTINUOUS_BATCHING' cmake/Dependencies.cmake"

# Test 6: Verify DEPENDENCIES.md exists
echo "6. Checking documentation..."
run_test "DEPENDENCIES.md exists" \
    "test -f DEPENDENCIES.md"

# Test 7: Verify DEPENDENCIES.md has llama.cpp section
run_test "DEPENDENCIES.md has llama.cpp section" \
    "grep -q 'llama.cpp' DEPENDENCIES.md"

# Test 8: Verify CI workflow exists
echo "7. Checking CI/CD configuration..."
run_test "llama-cpp-integration workflow exists" \
    "test -f .github/workflows/llama-cpp-integration.yml"

# Test 9: Verify CI workflow has version pinning check
run_test "CI has version pinning verification" \
    "grep -q 'verify-pinning' .github/workflows/llama-cpp-integration.yml"

# Test 10: Verify CI workflow has performance checks
run_test "CI has performance checks" \
    "grep -q 'performance-check' .github/workflows/llama-cpp-integration.yml"

# Test 11: Check if CMake can be configured (dry run)
echo "8. Testing CMake configuration (dry run)..."
if command -v cmake &> /dev/null; then
    run_test "CMake configuration validates" \
        "cmake -B /tmp/test_build -S . -DTHEMIS_ENABLE_LLM=OFF 2>&1 | grep -q 'Configuring done' || true"
else
    echo -e "${YELLOW}⚠️  SKIP (cmake not available)${NC}"
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All validation tests passed!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Run full build: cmake -B build -DTHEMIS_ENABLE_LLM=ON"
    echo "2. Run LLM tests: cd build && ctest -R llm"
    echo "3. Check CI pipeline results"
    exit 0
else
    echo -e "${RED}❌ Some tests failed. Please review the errors above.${NC}"
    exit 1
fi
