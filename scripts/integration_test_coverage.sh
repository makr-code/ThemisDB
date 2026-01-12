#!/bin/bash
# Integration Test Coverage Script
# Generates coverage reports for integration tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
COVERAGE_DIR="${BUILD_DIR}/coverage_integration"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo "Integration Test Coverage Report"
echo "=========================================="
echo ""

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov not found${NC}"
    echo "Install with: sudo apt-get install lcov"
    exit 1
fi

# Check if genhtml is installed
if ! command -v genhtml &> /dev/null; then
    echo -e "${RED}Error: genhtml not found${NC}"
    echo "Install with: sudo apt-get install lcov"
    exit 1
fi

# Create build directory with coverage flags
echo -e "${GREEN}Step 1: Configuring build with coverage...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DTHEMIS_BUILD_TESTS=ON

# Build tests
echo -e "${GREEN}Step 2: Building integration tests...${NC}"
cmake --build . --target all -- -j$(nproc)

# Run integration tests
echo -e "${GREEN}Step 3: Running integration tests...${NC}"
ctest -R integration --output-on-failure || {
    echo -e "${YELLOW}Warning: Some tests failed, continuing with coverage report${NC}"
}

# Create coverage directory
mkdir -p "$COVERAGE_DIR"

# Capture coverage data
echo -e "${GREEN}Step 4: Capturing coverage data...${NC}"
lcov --capture \
    --directory "$BUILD_DIR" \
    --output-file "${COVERAGE_DIR}/coverage.info" \
    --rc lcov_branch_coverage=1

# Remove external and test code from coverage
echo -e "${GREEN}Step 5: Filtering coverage data...${NC}"
lcov --remove "${COVERAGE_DIR}/coverage.info" \
    '/usr/*' \
    '*/vcpkg_installed/*' \
    '*/tests/*' \
    '*/test_*' \
    '*_test.cpp' \
    --output-file "${COVERAGE_DIR}/coverage_filtered.info" \
    --rc lcov_branch_coverage=1

# Generate HTML report
echo -e "${GREEN}Step 6: Generating HTML report...${NC}"
genhtml "${COVERAGE_DIR}/coverage_filtered.info" \
    --output-directory "${COVERAGE_DIR}/html" \
    --title "ThemisDB Integration Test Coverage" \
    --legend \
    --branch-coverage \
    --rc lcov_branch_coverage=1

# Generate summary
echo ""
echo "=========================================="
echo "Coverage Summary"
echo "=========================================="
lcov --summary "${COVERAGE_DIR}/coverage_filtered.info" --rc lcov_branch_coverage=1

# Component-specific coverage (if possible)
echo ""
echo "=========================================="
echo "Component Coverage Breakdown"
echo "=========================================="

# Storage
echo -e "${YELLOW}Storage Layer:${NC}"
lcov --extract "${COVERAGE_DIR}/coverage_filtered.info" \
    '*/src/storage/*' \
    --output-file "${COVERAGE_DIR}/coverage_storage.info" 2>/dev/null || true
if [ -f "${COVERAGE_DIR}/coverage_storage.info" ]; then
    lcov --summary "${COVERAGE_DIR}/coverage_storage.info" 2>/dev/null | tail -1
else
    echo "  No coverage data"
fi

# Query Engine
echo -e "${YELLOW}Query Engine:${NC}"
lcov --extract "${COVERAGE_DIR}/coverage_filtered.info" \
    '*/src/query/*' \
    --output-file "${COVERAGE_DIR}/coverage_query.info" 2>/dev/null || true
if [ -f "${COVERAGE_DIR}/coverage_query.info" ]; then
    lcov --summary "${COVERAGE_DIR}/coverage_query.info" 2>/dev/null | tail -1
else
    echo "  No coverage data"
fi

# RPC
echo -e "${YELLOW}RPC Service:${NC}"
lcov --extract "${COVERAGE_DIR}/coverage_filtered.info" \
    '*/src/rpc/*' \
    --output-file "${COVERAGE_DIR}/coverage_rpc.info" 2>/dev/null || true
if [ -f "${COVERAGE_DIR}/coverage_rpc.info" ]; then
    lcov --summary "${COVERAGE_DIR}/coverage_rpc.info" 2>/dev/null | tail -1
else
    echo "  No coverage data"
fi

# Security
echo -e "${YELLOW}Security:${NC}"
lcov --extract "${COVERAGE_DIR}/coverage_filtered.info" \
    '*/src/security/*' \
    --output-file "${COVERAGE_DIR}/coverage_security.info" 2>/dev/null || true
if [ -f "${COVERAGE_DIR}/coverage_security.info" ]; then
    lcov --summary "${COVERAGE_DIR}/coverage_security.info" 2>/dev/null | tail -1
else
    echo "  No coverage data"
fi

echo ""
echo "=========================================="
echo -e "${GREEN}Coverage report generated!${NC}"
echo "HTML report: ${COVERAGE_DIR}/html/index.html"
echo ""
echo "To view the report:"
echo "  firefox ${COVERAGE_DIR}/html/index.html"
echo "  or"
echo "  python3 -m http.server 8000 -d ${COVERAGE_DIR}/html"
echo "=========================================="
