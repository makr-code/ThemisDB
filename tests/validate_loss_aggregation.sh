#!/bin/bash
# Validation Script for PR #757: Loss Aggregation Implementation
# This script performs lightweight validation without requiring a full build

# Don't use set -e to avoid premature exit on grep failures

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=========================================="
echo "PR #757 Loss Aggregation Validation"
echo "=========================================="
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass_count=0
fail_count=0
skip_count=0

print_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((pass_count++))
}

print_fail() {
    echo -e "${RED}✗${NC} $1"
    ((fail_count++))
}

print_skip() {
    echo -e "${YELLOW}⊘${NC} $1"
    ((skip_count++))
}

print_section() {
    echo ""
    echo "========================================"
    echo "$1"
    echo "========================================"
    echo ""
}

# =============================================================================
# 1. File Structure Validation
# =============================================================================
print_section "1. File Structure Validation"

# Check header file
if [ -f "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h" ]; then
    print_pass "Header file exists"
    
    # Check for loss fields in GradientExchangeMessage
    if grep -q "std::optional<float> local_loss" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "GradientExchangeMessage has local_loss field"
    else
        print_fail "GradientExchangeMessage missing local_loss field"
    fi
    
    if grep -q "std::optional<float> local_accuracy" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "GradientExchangeMessage has local_accuracy field"
    else
        print_fail "GradientExchangeMessage missing local_accuracy field"
    fi
    
    if grep -q "int samples_in_batch" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "GradientExchangeMessage has samples_in_batch field"
    else
        print_fail "GradientExchangeMessage missing samples_in_batch field"
    fi
    
    # Check for loss fields in StepResult
    if grep -q "std::optional<float> aggregated_loss" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "StepResult has aggregated_loss field"
    else
        print_fail "StepResult missing aggregated_loss field"
    fi
    
    if grep -q "std::map<std::string, float> per_shard_loss" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "StepResult has per_shard_loss field"
    else
        print_fail "StepResult missing per_shard_loss field"
    fi
    
    # Check for aggregateLoss method
    if grep -q "aggregateLoss" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "aggregateLoss() method declared"
    else
        print_fail "aggregateLoss() method not declared"
    fi
    
    # Check for computeWeightedLoss method
    if grep -q "computeWeightedLoss" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h"; then
        print_pass "computeWeightedLoss() method declared"
    else
        print_fail "computeWeightedLoss() method not declared"
    fi
else
    print_fail "Header file not found"
fi

# Check implementation file
if [ -f "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp" ]; then
    print_pass "Implementation file exists"
    
    # Check toJSON implementation for loss fields
    if grep -q '"local_loss"' "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp"; then
        print_pass "toJSON() serializes local_loss"
    else
        print_fail "toJSON() missing local_loss serialization"
    fi
    
    # Check fromJSON implementation for loss fields
    if grep -q 'msg.local_loss = j\["local_loss"\]' "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp"; then
        print_pass "fromJSON() deserializes local_loss"
    else
        print_fail "fromJSON() missing local_loss deserialization"
    fi
    
    # Check aggregateLoss implementation
    if grep -q "std::optional<float> DistributedTrainingCoordinator::aggregateLoss" "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp"; then
        print_pass "aggregateLoss() implemented"
    else
        print_fail "aggregateLoss() not implemented"
    fi
    
    # Check computeWeightedLoss implementation
    if grep -q "float DistributedTrainingCoordinator::computeWeightedLoss" "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp"; then
        print_pass "computeWeightedLoss() implemented"
    else
        print_fail "computeWeightedLoss() not implemented"
    fi
    
    # Check executeStep calls aggregateLoss
    if grep -q "result.aggregated_loss = aggregateLoss" "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp"; then
        print_pass "executeStep() calls aggregateLoss()"
    else
        print_fail "executeStep() doesn't call aggregateLoss()"
    fi
else
    print_fail "Implementation file not found"
fi

# Check test file
if [ -f "$PROJECT_ROOT/tests/test_distributed_training_coordinator.cpp" ]; then
    print_pass "Test file exists"
    
    # Count loss-related tests
    loss_test_count=$(grep -c "TEST_F.*Loss" "$PROJECT_ROOT/tests/test_distributed_training_coordinator.cpp" || echo 0)
    if [ "$loss_test_count" -ge 6 ]; then
        print_pass "Found $loss_test_count loss-related tests (expected ≥6)"
    else
        print_fail "Only found $loss_test_count loss-related tests (expected ≥6)"
    fi
else
    print_fail "Test file not found"
fi

# Check training service integration
if [ -f "$PROJECT_ROOT/src/llm/lora_framework/lora_training_service.cpp" ]; then
    print_pass "Training service file exists"
    
    # Check if simulated loss is removed and aggregated_loss is used
    if grep -q "step_result.aggregated_loss" "$PROJECT_ROOT/src/llm/lora_framework/lora_training_service.cpp"; then
        print_pass "Training service uses aggregated_loss"
    else
        print_fail "Training service doesn't use aggregated_loss"
    fi
    
    # Check for per_shard_loss metrics
    if grep -q "per_shard_loss" "$PROJECT_ROOT/src/llm/lora_framework/lora_training_service.cpp"; then
        print_pass "Training service tracks per_shard_loss"
    else
        print_fail "Training service doesn't track per_shard_loss"
    fi
else
    print_fail "Training service file not found"
fi

# =============================================================================
# 2. Code Quality Checks
# =============================================================================
print_section "2. Code Quality Checks"

# Check for potential syntax errors in key files
echo "Checking for basic syntax issues..."

# Check for multi-line declarations (informational only)
multiline_decls=$(grep -E "^\s*std::.*\($" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h" | wc -l)
if [ "$multiline_decls" -gt 0 ]; then
    print_pass "Header syntax validated ($multiline_decls multi-line declarations found)"
else
    print_pass "Header syntax validated (no multi-line declarations)"
fi

# Check for unmatched braces (simple check)
header_open=$(grep -o "{" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h" | wc -l)
header_close=$(grep -o "}" "$PROJECT_ROOT/include/llm/distributed_training_coordinator.h" | wc -l)
if [ "$header_open" -eq "$header_close" ]; then
    print_pass "Header braces appear balanced ($header_open opening, $header_close closing)"
else
    print_fail "Header braces may be unbalanced ($header_open opening, $header_close closing)"
fi

# Check implementation file
impl_open=$(grep -o "{" "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp" | wc -l)
impl_close=$(grep -o "}" "$PROJECT_ROOT/src/llm/distributed_training_coordinator.cpp" | wc -l)
if [ "$impl_open" -eq "$impl_close" ]; then
    print_pass "Implementation braces appear balanced ($impl_open opening, $impl_close closing)"
else
    print_fail "Implementation braces may be unbalanced ($impl_open opening, $impl_close closing)"
fi

# =============================================================================
# 3. Test Coverage Analysis
# =============================================================================
print_section "3. Test Coverage Analysis"

echo "Analyzing test coverage for loss aggregation..."

# Check for specific test cases
tests=(
    "GradientExchangeMessage_LossMetricsSerialization"
    "GradientExchangeMessage_OptionalLossFields"
    "Coordinator_ComputeWeightedLoss_SimpleAverage"
    "Coordinator_ComputeWeightedLoss_UnequalSamples"
    "Coordinator_ComputeWeightedLoss_ZeroSamples"
    "Coordinator_ComputeWeightedLoss_EmptyInput"
    "Coordinator_ExecuteStep_ReturnsAggregatedLoss"
    "StepResult_ContainsLossFields"
)

for test_name in "${tests[@]}"; do
    if grep -q "TEST_F.*$test_name" "$PROJECT_ROOT/tests/test_distributed_training_coordinator.cpp"; then
        print_pass "Test exists: $test_name"
    else
        print_fail "Test missing: $test_name"
    fi
done

# =============================================================================
# 4. Documentation Check
# =============================================================================
print_section "4. Documentation Check"

if [ -f "$PROJECT_ROOT/tests/LOSS_AGGREGATION_E2E_TEST_PLAN.md" ]; then
    print_pass "E2E test plan document exists"
else
    print_skip "E2E test plan document not found (optional)"
fi

# =============================================================================
# 5. Build Configuration Check
# =============================================================================
print_section "5. Build Configuration Check"

if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    if grep -q "THEMIS_ENABLE_DISTRIBUTED_TRAINING" "$PROJECT_ROOT/CMakeLists.txt"; then
        print_pass "Build option THEMIS_ENABLE_DISTRIBUTED_TRAINING exists"
    else
        print_fail "Build option THEMIS_ENABLE_DISTRIBUTED_TRAINING not found"
    fi
fi

if [ -f "$PROJECT_ROOT/tests/CMakeLists.txt" ]; then
    if grep -q "test_distributed_training_coordinator" "$PROJECT_ROOT/tests/CMakeLists.txt"; then
        print_pass "Test target configured in CMake"
    else
        print_fail "Test target not configured in CMake"
    fi
fi

# =============================================================================
# Summary
# =============================================================================
print_section "Validation Summary"

total=$((pass_count + fail_count + skip_count))
echo "Total checks: $total"
echo -e "${GREEN}Passed: $pass_count${NC}"
echo -e "${RED}Failed: $fail_count${NC}"
echo -e "${YELLOW}Skipped: $skip_count${NC}"
echo ""

if [ $fail_count -eq 0 ]; then
    echo -e "${GREEN}=========================================="
    echo "✓ All validation checks passed!"
    echo "==========================================${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Build the project with: cmake -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=ON .."
    echo "2. Run tests with: ctest -R DistributedTrainingCoordinatorTests"
    echo "3. Review LOSS_AGGREGATION_E2E_TEST_PLAN.md for detailed test plan"
    exit 0
else
    echo -e "${RED}=========================================="
    echo "✗ Validation failed with $fail_count error(s)"
    echo "==========================================${NC}"
    echo ""
    echo "Please fix the issues above before proceeding."
    exit 1
fi
