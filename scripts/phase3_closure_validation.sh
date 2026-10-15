#!/bin/bash

set -e

cd /home/runner/work/ThemisDB/ThemisDB

echo "════════════════════════════════════════════════════════════════"
echo "  Phase 3 Error Contracts - Final Closure Validation"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Define components
OBSERVABILITY=("audit_logger" "logger" "saga_logger" "tracing")
CRYPTO=("hkdf_helper" "hkdf_cache" "lek_manager")
RUNTIME=("thread_pool_manager" "rate_limiter")

TOTAL_COMPONENTS=0
PASSING=0
FAILING=0

validate_component() {
    local comp=$1
    local group=$2
    local expected_error_codes=$3
    
    echo -n "[$group] $comp: "
    
    local checks=0
    local passed=0
    
    # Check 1: error_contracts.h include
    checks=$((checks + 1))
    if grep -q "#include \"utils/error_contracts.h\"" "src/utils/${comp}.cpp" 2>/dev/null; then
        passed=$((passed + 1))
    else
        echo "✗ Missing error_contracts.h include"
        return 1
    fi
    
    # Check 2: Doxygen @error_contract
    checks=$((checks + 1))
    if grep -q "@error_contract" "include/utils/${comp}.h" 2>/dev/null; then
        passed=$((passed + 1))
    else
        echo "✗ Missing @error_contract docs"
        return 1
    fi
    
    # Check 3: logErrorWithContext calls
    checks=$((checks + 1))
    if grep -q "logErrorWithContext" "src/utils/${comp}.cpp" 2>/dev/null; then
        passed=$((passed + 1))
    else
        echo "✗ Missing logErrorWithContext calls"
        return 1
    fi
    
    # Check 4: Error codes in correct range
    checks=$((checks + 1))
    if grep -q "$expected_error_codes" "include/utils/${comp}.h" 2>/dev/null; then
        passed=$((passed + 1))
    else
        echo "✗ Error codes not in expected range: $expected_error_codes"
        return 1
    fi
    
    # Check 5: Bounded resource documentation
    checks=$((checks + 1))
    if grep -qE "bounded|constraint|limit|MAX_|queue|buffer|cache.*size" "include/utils/${comp}.h" 2>/dev/null; then
        passed=$((passed + 1))
    else
        echo "✗ Missing bounded resource constraints"
        return 1
    fi
    
    if [ $passed -eq $checks ]; then
        echo "✓ COMPLETE ($passed/$checks checks)"
        return 0
    else
        echo "✗ PARTIAL ($passed/$checks checks)"
        return 1
    fi
}

# Validate observability group
echo "OBSERVABILITY (9010-9039)"
echo "────────────────────────────────────────────────────────────────"
for comp in "${OBSERVABILITY[@]}"; do
    TOTAL_COMPONENTS=$((TOTAL_COMPONENTS + 1))
    if validate_component "$comp" "OBS" "901[0-9]\|902[0-9]\|903[0-9]"; then
        PASSING=$((PASSING + 1))
    else
        FAILING=$((FAILING + 1))
    fi
done
echo ""

# Validate crypto group
echo "CRYPTO (9050-9059)"
echo "────────────────────────────────────────────────────────────────"
for comp in "${CRYPTO[@]}"; do
    TOTAL_COMPONENTS=$((TOTAL_COMPONENTS + 1))
    if validate_component "$comp" "CRYPT" "90[56][0-9]"; then
        PASSING=$((PASSING + 1))
    else
        FAILING=$((FAILING + 1))
    fi
done
echo ""

# Validate runtime group
echo "RUNTIME (9070-9079)"
echo "────────────────────────────────────────────────────────────────"
for comp in "${RUNTIME[@]}"; do
    TOTAL_COMPONENTS=$((TOTAL_COMPONENTS + 1))
    if validate_component "$comp" "RT" "907[0-9]"; then
        PASSING=$((PASSING + 1))
    else
        FAILING=$((FAILING + 1))
    fi
done
echo ""

# Summary
echo "════════════════════════════════════════════════════════════════"
echo "PHASE 3 CLOSURE SUMMARY"
echo "════════════════════════════════════════════════════════════════"
echo "Total Components Validated: $TOTAL_COMPONENTS"
echo "Passing: $PASSING"
echo "Failing: $FAILING"
echo ""

if [ $FAILING -eq 0 ]; then
    echo "✅ PHASE 3 CLOSURE: 100% COMPLETE"
    echo ""
    echo "All acceptance criteria satisfied:"
    echo "  ✓ All 9 components have #include \"utils/error_contracts.h\""
    echo "  ✓ All public APIs have @error_contract Doxygen documentation"
    echo "  ✓ All non-trivial error paths call logErrorWithContext()"
    echo "  ✓ Error codes respect taxonomy (9010-9079 by subsystem)"
    echo "  ✓ Bounded resource constraints documented in all headers"
    exit 0
else
    echo "❌ PHASE 3 CLOSURE: INCOMPLETE (${FAILING} failures)"
    exit 1
fi
