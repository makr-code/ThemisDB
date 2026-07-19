#!/bin/bash
# Field Diagnostics Implementation Verification Script
# This script validates the Phase 3 implementation

set -e

REPO_ROOT="${1:-.}"
ERRORS=0
WARNINGS=0

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

function warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    WARNINGS=$((WARNINGS + 1))
}

function error() {
    echo -e "${RED}[ERROR]${NC} $1"
    ERRORS=$((ERRORS + 1))
}

echo "=========================================="
echo "Field Diagnostics Implementation Verification"
echo "=========================================="
echo

# Check 1: Schema header exists
echo "[1/10] Checking schema header..."
if [ -f "$REPO_ROOT/include/utils/field_diagnostics_schema.h" ]; then
    info "Schema header exists"
    LINES=$(wc -l < "$REPO_ROOT/include/utils/field_diagnostics_schema.h")
    info "  Lines: $LINES"
else
    error "Schema header not found at include/utils/field_diagnostics_schema.h"
fi
echo

# Check 2: Collector header exists
echo "[2/10] Checking collector header..."
if [ -f "$REPO_ROOT/include/observability/field_diagnostics_collector.h" ]; then
    info "Collector header exists"
    LINES=$(wc -l < "$REPO_ROOT/include/observability/field_diagnostics_collector.h")
    info "  Lines: $LINES"
else
    error "Collector header not found at include/observability/field_diagnostics_collector.h"
fi
echo

# Check 3: Collector implementation exists
echo "[3/10] Checking collector implementation..."
if [ -f "$REPO_ROOT/src/observability/field_diagnostics_collector.cpp" ]; then
    info "Collector implementation exists"
    LINES=$(wc -l < "$REPO_ROOT/src/observability/field_diagnostics_collector.cpp")
    info "  Lines: $LINES"
else
    error "Collector implementation not found at src/observability/field_diagnostics_collector.cpp"
fi
echo

# Check 4: Tests exist
echo "[4/10] Checking test suite..."
if [ -f "$REPO_ROOT/tests/observability/test_field_diagnostics.cpp" ]; then
    info "Test suite exists"
    LINES=$(wc -l < "$REPO_ROOT/tests/observability/test_field_diagnostics.cpp")
    info "  Lines: $LINES"
    TESTS=$(grep -c "TEST" "$REPO_ROOT/tests/observability/test_field_diagnostics.cpp")
    info "  Test cases: $TESTS"
else
    error "Test suite not found at tests/observability/test_field_diagnostics.cpp"
fi
echo

# Check 5: Operations runbook exists
echo "[5/10] Checking operations runbook..."
if [ -f "$REPO_ROOT/docs/de/operations/FIELD_DIAGNOSTICS_RUNBOOK.md" ]; then
    info "Operations runbook exists"
    LINES=$(wc -l < "$REPO_ROOT/docs/de/operations/FIELD_DIAGNOSTICS_RUNBOOK.md")
    info "  Lines: $LINES"
else
    error "Operations runbook not found at docs/de/operations/FIELD_DIAGNOSTICS_RUNBOOK.md"
fi
echo

# Check 6: Integration guide exists
echo "[6/10] Checking integration guide..."
if [ -f "$REPO_ROOT/docs/de/observability/FIELD_DIAGNOSTICS_INTEGRATION_GUIDE.md" ]; then
    info "Integration guide exists"
    LINES=$(wc -l < "$REPO_ROOT/docs/de/observability/FIELD_DIAGNOSTICS_INTEGRATION_GUIDE.md")
    info "  Lines: $LINES"
else
    error "Integration guide not found at docs/de/observability/FIELD_DIAGNOSTICS_INTEGRATION_GUIDE.md"
fi
echo

# Check 7: Schema contains required enums
echo "[7/10] Checking schema enums..."
if [ -f "$REPO_ROOT/include/utils/field_diagnostics_schema.h" ]; then
    ENUMS_FOUND=0
    grep -q "enum class DiagnosticFailureCategory" "$REPO_ROOT/include/utils/field_diagnostics_schema.h" && ENUMS_FOUND=$((ENUMS_FOUND + 1))
    grep -q "enum class DiagnosticSeverity" "$REPO_ROOT/include/utils/field_diagnostics_schema.h" && ENUMS_FOUND=$((ENUMS_FOUND + 1))
    grep -q "struct DiagnosticEvent" "$REPO_ROOT/include/utils/field_diagnostics_schema.h" && ENUMS_FOUND=$((ENUMS_FOUND + 1))
    
    if [ $ENUMS_FOUND -eq 3 ]; then
        info "All required enums and structs found (3/3)"
    else
        error "Missing enums/structs: $ENUMS_FOUND/3 found"
    fi
else
    error "Cannot check schema enums - file not found"
fi
echo

# Check 8: Collector contains required methods
echo "[8/10] Checking collector methods..."
if [ -f "$REPO_ROOT/include/observability/field_diagnostics_collector.h" ]; then
    METHODS_FOUND=0
    grep -q "getInstance()" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && METHODS_FOUND=$((METHODS_FOUND + 1))
    grep -q "emitWithPIIMasking" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && METHODS_FOUND=$((METHODS_FOUND + 1))
    grep -q "emitDiagnosticEvent" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && METHODS_FOUND=$((METHODS_FOUND + 1))
    grep -q "getEventsSince" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && METHODS_FOUND=$((METHODS_FOUND + 1))
    grep -q "getEventCountsByCategory" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && METHODS_FOUND=$((METHODS_FOUND + 1))
    
    if [ $METHODS_FOUND -eq 5 ]; then
        info "All required methods found (5/5)"
    else
        error "Missing methods: $METHODS_FOUND/5 found"
    fi
else
    error "Cannot check collector methods - file not found"
fi
echo

# Check 9: PII masking functions exist
echo "[9/10] Checking PII masking..."
if [ -f "$REPO_ROOT/include/utils/field_diagnostics_schema.h" ]; then
    PII_FOUND=0
    grep -q "isPIIField" "$REPO_ROOT/include/utils/field_diagnostics_schema.h" && PII_FOUND=$((PII_FOUND + 1))
    grep -q "sanitizePII" "$REPO_ROOT/include/utils/field_diagnostics_schema.h" && PII_FOUND=$((PII_FOUND + 1))
    grep -q "PII_MASK_FIELDS" "$REPO_ROOT/include/utils/field_diagnostics_schema.h" && PII_FOUND=$((PII_FOUND + 1))
    
    if [ $PII_FOUND -eq 3 ]; then
        info "PII masking infrastructure found (3/3)"
    else
        error "Missing PII functions: $PII_FOUND/3 found"
    fi
else
    error "Cannot check PII masking - file not found"
fi
echo

# Check 10: Thread safety patterns
echo "[10/10] Checking thread safety..."
if [ -f "$REPO_ROOT/include/observability/field_diagnostics_collector.h" ]; then
    SYNC_FOUND=0
    grep -q "shared_mutex" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && SYNC_FOUND=$((SYNC_FOUND + 1))
    grep -q "std::deque" "$REPO_ROOT/include/observability/field_diagnostics_collector.h" && SYNC_FOUND=$((SYNC_FOUND + 1))
    grep -q "std::atomic" "$REPO_ROOT/src/observability/field_diagnostics_collector.cpp" && SYNC_FOUND=$((SYNC_FOUND + 1))
    
    if [ $SYNC_FOUND -eq 3 ]; then
        info "Thread safety mechanisms found (3/3)"
    else
        warn "Thread safety might be incomplete: $SYNC_FOUND/3 found"
    fi
else
    error "Cannot check thread safety - file not found"
fi
echo

# Summary
echo "=========================================="
echo "Verification Results"
echo "=========================================="
echo "  Errors:   $ERRORS"
echo "  Warnings: $WARNINGS"
echo

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some checks failed${NC}"
    exit 1
fi
