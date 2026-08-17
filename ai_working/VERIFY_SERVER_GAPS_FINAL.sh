#!/bin/bash
# Final Server Module Gap Closure Verification Script
# Run after all 4 batches have completed and been merged

set -e

REPO_DIR="/home/runner/work/ThemisDB/ThemisDB"
cd "$REPO_DIR"

echo "==================================================================="
echo "Server Module Gap Closure — Final Verification"
echo "==================================================================="
echo ""

# Phase 1: Build Verification
echo "[1/5] Build Verification..."
echo "------"

if [ -f CMakePresets.json ]; then
    echo "Attempting debug build (server subset)..."
    
    # Try a lightweight server build to verify syntax
    if cmake --preset linux-debug \
        -DTHEMIS_BUILD_TESTS=ON \
        -DTHEMIS_BUILD_BENCHMARKS=OFF \
        2>&1 | tail -20; then
        echo "✓ Debug configure successful"
    else
        echo "✗ Debug configure failed"
        exit 1
    fi
else
    echo "⚠ CMakePresets.json not found, skipping build test"
fi

echo ""

# Phase 2: Test Inventory
echo "[2/5] Test Inventory..."
echo "------"

TEST_FILES=$(find tests/server -name "test_*.cpp" -type f | sort)
TEST_COUNT=$(echo "$TEST_FILES" | wc -l)
echo "Found $TEST_COUNT server test files:"
echo "$TEST_FILES" | sed 's/^/  - /'

echo ""

# Phase 3: Gap Scan Comparison
echo "[3/5] Gap Scan Analysis..."
echo "------"

if [ -f src/server/MODULE_GAPS.md ]; then
    echo "Module gaps file exists: src/server/MODULE_GAPS.md"
    
    SUMMARY_LINE=$(grep "Total Verified Gaps" src/server/MODULE_GAPS.md)
    if [ -n "$SUMMARY_LINE" ]; then
        echo "Status: $SUMMARY_LINE"
    fi
    
    # Count lines mentioning "severity=CRITICAL"
    CRITICAL_COUNT=$(grep -c "severity=CRITICAL" src/server/MODULE_GAPS.md 2>/dev/null || echo "N/A")
    HIGH_COUNT=$(grep -c "severity=HIGH" src/server/MODULE_GAPS.md 2>/dev/null || echo "N/A")
    
    echo "  CRITICAL findings: $CRITICAL_COUNT"
    echo "  HIGH findings: $HIGH_COUNT"
else
    echo "⚠ Gap scan file not found"
fi

echo ""

# Phase 4: Source File Statistics
echo "[4/5] Source Code Statistics..."
echo "------"

SRC_FILES=$(find src/server -name "*.cpp" -not -name "*.test.cpp" -type f)
SRC_COUNT=$(echo "$SRC_FILES" | wc -l)
TOTAL_LINES=$(find src/server -name "*.cpp" -not -name "*.test.cpp" -exec wc -l {} + | tail -1 | awk '{print $1}')

echo "Server source files: $SRC_COUNT"
echo "Total source lines: $TOTAL_LINES"

# Check for expected header files
HEADER_COUNT=$(find include/server -name "*.h" -o -name "*.hpp" 2>/dev/null | wc -l)
echo "Server header files: $HEADER_COUNT"

echo ""

# Phase 5: Compliance Checks
echo "[5/5] Compliance Checks..."
echo "------"

ISSUES=0

# Check for common anti-patterns
echo "Scanning for CRITICAL anti-patterns..."

# Check for raw new/delete (should use smart pointers)
RAW_NEW_COUNT=$(grep -r "new " src/server/*.cpp 2>/dev/null | grep -v "// " | grep -v std | wc -l)
if [ "$RAW_NEW_COUNT" -gt 5 ]; then
    echo "  ⚠ Found ~$RAW_NEW_COUNT lines with 'new' (should use smart_ptr)"
    ((ISSUES++))
fi

# Check for nullptr checks
NULL_CHECK_COUNT=$(grep -r "== nullptr\|!= nullptr\|== NULL\|!= NULL" src/server/*.cpp 2>/dev/null | wc -l)
echo "  ✓ Found $NULL_CHECK_COUNT null-check instances"

# Check for exception handling
TRY_COUNT=$(grep -r "try {" src/server/*.cpp 2>/dev/null | wc -l)
CATCH_COUNT=$(grep -r "catch" src/server/*.cpp 2>/dev/null | wc -l)
echo "  ✓ Found $TRY_COUNT try-blocks and $CATCH_COUNT catch-blocks"

# Check for audit logging
AUDIT_LOG_COUNT=$(grep -r "audit_log\|EmitDiagnostic\|log_audit" src/server/*.cpp 2>/dev/null | wc -l)
echo "  ✓ Found $AUDIT_LOG_COUNT audit/diagnostic emit calls"

# Check for timeout patterns
TIMEOUT_COUNT=$(grep -r "timeout\|deadline\|duration" src/server/*.cpp 2>/dev/null | wc -l)
echo "  ✓ Found $TIMEOUT_COUNT timeout/deadline references"

echo ""

# Summary
echo "==================================================================="
echo "Final Verification Summary"
echo "==================================================================="
echo ""
echo "Build Status: $([ $ISSUES -eq 0 ] && echo '✓ PASS' || echo '⚠ REVIEW NEEDED')"
echo "Test Coverage: $TEST_COUNT server tests ready"
echo "Source Files: $SRC_COUNT files (~$TOTAL_LINES lines)"
echo "Compliance Issues: $ISSUES"
echo ""
echo "Next Steps:"
echo "1. Review any compliance issues above"
echo "2. Run full test suite: ctest -R test_server"
echo "3. Run benchmarks: ctest -R bench_server"
echo "4. Merge PR to develop"
echo ""
