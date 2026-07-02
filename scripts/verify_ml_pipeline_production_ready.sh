#!/bin/bash
##
# @file verify_ml_pipeline_production_ready.sh
# @brief Production Readiness Verification Script
#
# This script verifies that the ML pipeline is fully configured and ready
# for production deployment.
#
# Usage: ./verify_ml_pipeline_production_ready.sh [--verbose] [--fix]
#
# Options:
#   --verbose   Print detailed output for each check
#   --fix       Attempt to auto-fix issues where possible
#

set -euo pipefail

##
# Configuration
##
VERBOSE=false
AUTOFIX=false
THEMISDB_HOST="${THEMISDB_HOST:-localhost}"
THEMISDB_PORT="${THEMISDB_PORT:-8080}"
PROMETHEUS_PORT="${PROMETHEUS_PORT:-9091}"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
CHECKS_PASSED=0
CHECKS_FAILED=0
CHECKS_WARNED=0

##
# Helper Functions
##

log_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((CHECKS_PASSED++))
}

log_fail() {
    echo -e "${RED}✗${NC} $1"
    ((CHECKS_FAILED++))
}

log_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    ((CHECKS_WARNED++))
}

log_info() {
    if [ "$VERBOSE" = true ]; then
        echo "  → $1"
    fi
}

check_http_endpoint() {
    local endpoint=$1
    local description=$2
    
    if curl -s -f "http://${THEMISDB_HOST}:${THEMISDB_PORT}${endpoint}" > /dev/null 2>&1; then
        log_pass "$description"
        return 0
    else
        log_fail "$description"
        return 1
    fi
}

check_metrics() {
    local metric=$1
    local description=$2
    
    if curl -s "http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics" | grep -q "$metric"; then
        log_pass "$description"
        return 0
    else
        log_fail "$description"
        return 1
    fi
}

##
# Parsing arguments
##
for arg in "$@"; do
    case $arg in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --fix)
            AUTOFIX=true
            shift
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Usage: $0 [--verbose] [--fix]"
            exit 1
            ;;
    esac
done

##
# Main Verification
##

echo "=========================================="
echo "ML Pipeline Production Readiness Check"
echo "=========================================="
echo ""

# 1. Basic Connectivity
echo "1. Basic Connectivity Checks"
echo "   ================================="

if check_http_endpoint "/health" "HTTP server health endpoint"; then
    HEALTH=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/health | grep -o '"status":"[^"]*"' || echo '"status":"unknown"')
    log_info "Server status: $HEALTH"
fi

if check_http_endpoint "/metrics" "Prometheus metrics endpoint"; then
    METRICS_COUNT=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep -c "^themisdb_ml" || echo "0")
    log_info "ML metrics available: $METRICS_COUNT"
fi

echo ""

# 2. ML Orchestrator Status
echo "2. ML Orchestrator Verification"
echo "   ================================="

if check_http_endpoint "/api/ml/status" "ML orchestrator status endpoint"; then
    STATUS=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/status)
    
    if echo "$STATUS" | grep -q '"orchestrator_running":true'; then
        log_pass "Orchestrator is running"
        log_info "Full status: $STATUS"
    else
        log_fail "Orchestrator is not running"
        if [ "$AUTOFIX" = true ]; then
            log_info "Attempting to start orchestrator..."
            curl -X POST http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/start || true
        fi
    fi
fi

echo ""

# 3. Signal Provider Verification
echo "3. Signal Provider Wiring Verification"
echo "   ================================="

# Check Loop 1: HNSW miss-rate
if check_metrics "themisdb_ml_loop_1_signal_source" "Loop 1 HNSW miss-rate signal"; then
    SOURCE=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_loop_1_signal_source" | head -1)
    log_info "Loop 1 signal source: $SOURCE"
fi

# Check Loop 2: Workload drift
if check_metrics "themisdb_ml_loop_2_signal_source" "Loop 2 workload drift signal"; then
    SOURCE=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_loop_2_signal_source" | head -1)
    log_info "Loop 2 signal source: $SOURCE"
fi

# Check Loop 4: Feedback count
if check_metrics "themisdb_ml_loop_4_signal_source" "Loop 4 feedback entry count signal"; then
    SOURCE=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_loop_4_signal_source" | head -1)
    log_info "Loop 4 signal source: $SOURCE"
fi

echo ""

# 4. Loop Execution Verification
echo "4. Learning Loop Execution Verification"
echo "   ================================="

for loop in LOOP_1_HNSW_QUERY LOOP_2_WORKLOAD LOOP_3_INDEX LOOP_4_RLAIF; do
    if check_metrics "themisdb_ml_loop_executions_total{loop_id=\"$loop\"" "Loop execution metric for $loop"; then
        EXEC_COUNT=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_loop_executions_total{loop_id=\"$loop\"" | grep -o "} [0-9]*" | tr -d '} ' || echo "0")
        log_info "Executions for $loop: $EXEC_COUNT"
    fi
done

echo ""

# 5. A/B Testing Framework Verification
echo "5. A/B Testing Framework Verification"
echo "   ================================="

if check_http_endpoint "/api/ab/status" "A/B testing status endpoint"; then
    AB_STATUS=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ab/status)
    log_info "A/B testing status: $AB_STATUS"
fi

if check_metrics "themisdb_ml_ab_test_active" "A/B test active metric"; then
    ACTIVE=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_ab_test_active" | wc -l || echo "0")
    log_info "Active A/B tests: $ACTIVE"
fi

echo ""

# 6. Feedback Collection Verification
echo "6. Feedback Collection Verification"
echo "   ================================="

if check_http_endpoint "/api/ml/feedback/count" "Feedback count endpoint"; then
    COUNT=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/feedback/count)
    log_info "Feedback entry count: $COUNT"
fi

if check_metrics "themisdb_ml_feedback_entries_total" "Feedback metrics"; then
    FEEDBACK=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_feedback_entries_total" | head -1)
    log_info "Feedback metric: $FEEDBACK"
fi

echo ""

# 7. ML Observability Metrics
echo "7. ML Observability Metrics Verification"
echo "   ================================="

REQUIRED_METRICS=(
    "themisdb_ml_loop_transitions_total"
    "themisdb_ml_loop_duration_ms"
    "themisdb_ml_loop_executions_total"
    "themisdb_ml_loop_errors_total"
    "themisdb_ml_adapter_deployments_total"
    "themisdb_ml_retraining_progress_percent"
    "themisdb_ml_model_accuracy"
    "themisdb_ml_inference_latency_ms"
)

for metric in "${REQUIRED_METRICS[@]}"; do
    if check_metrics "$metric" "Metric: $metric"; then
        true
    fi
done

echo ""

# 8. Component Registration Verification
echo "8. Component Registration Verification"
echo "   ================================="

if check_http_endpoint "/api/ml/components" "Component registry endpoint"; then
    COMPONENTS=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/components)
    
    for component_type in "lora_adapters" "retrieval_systems" "prompt_systems" "knowledge_gap_detectors"; do
        if echo "$COMPONENTS" | grep -q "$component_type"; then
            COUNT=$(echo "$COMPONENTS" | grep -o "\"$component_type\"" | wc -l)
            log_pass "Component type registered: $component_type ($COUNT instances)"
        else
            log_warn "No $component_type registered"
        fi
    done
fi

echo ""

# 9. Performance and Resource Checks
echo "9. Performance and Resource Checks"
echo "   ================================="

# Check loop execution time SLO
LOOP1_TIME=$(curl -s http://${THEMISDB_HOST}:${PROMETHEUS_PORT}/metrics | grep "themisdb_ml_loop_duration_ms" | grep "loop_id=\"LOOP_1" | tail -1 | grep -o "[0-9]*" | tail -1 || echo "0")

if [ "$LOOP1_TIME" -gt 0 ]; then
    if [ "$LOOP1_TIME" -lt 5000 ]; then  # 5 second SLO
        log_pass "Loop 1 execution time within SLO ($LOOP1_TIME ms < 5000 ms)"
    else
        log_warn "Loop 1 execution time exceeds SLO ($LOOP1_TIME ms > 5000 ms)"
    fi
fi

echo ""

# 10. Data Persistence Checks
echo "10. Data Persistence Verification"
echo "   ================================="

# Check if storage is accessible
if curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/storage/status > /dev/null 2>&1; then
    log_pass "ML storage accessible"
    
    STORAGE_STATUS=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/storage/status)
    log_info "Storage details: $STORAGE_STATUS"
else
    log_fail "ML storage not accessible"
fi

echo ""

# 11. Alert Configuration
echo "11. Alert Configuration Verification"
echo "   ================================="

# Check if alert endpoints are available
if check_http_endpoint "/api/alerts/status" "Alert status endpoint"; then
    ALERTS=$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/alerts/status)
    log_info "Alert configuration: $ALERTS"
fi

echo ""

# 12. Security and Access Control
echo "12. Security Verification"
echo "   ================================="

# Check if endpoints require proper authentication
AUTH_TEST=$(curl -s -w "%{http_code}" http://${THEMISDB_HOST}:${THEMISDB_PORT}/api/ml/secure-endpoint 2>/dev/null || echo "000")

if [[ "$AUTH_TEST" == "401" ]] || [[ "$AUTH_TEST" == "403" ]]; then
    log_pass "Secure endpoints properly protected"
elif [[ "$AUTH_TEST" == "404" ]]; then
    log_warn "Secure endpoint not found (this is OK in test)"
fi

echo ""

##
# Summary Report
##

echo "=========================================="
echo "Production Readiness Summary"
echo "=========================================="
echo ""

TOTAL_CHECKS=$((CHECKS_PASSED + CHECKS_FAILED + CHECKS_WARNED))
PASS_RATE=$((100 * CHECKS_PASSED / TOTAL_CHECKS))

echo "Results:"
echo "  ✓ Passed:  $CHECKS_PASSED"
echo "  ✗ Failed:  $CHECKS_FAILED"
echo "  ⚠ Warned:  $CHECKS_WARNED"
echo "  ─────────"
echo "  Total:     $TOTAL_CHECKS"
echo ""
echo "Pass Rate: $PASS_RATE%"
echo ""

if [ $CHECKS_FAILED -eq 0 ]; then
    echo -e "${GREEN}=========================================="
    echo "✓ System is PRODUCTION READY"
    echo "=========================================="
    exit 0
elif [ $CHECKS_FAILED -lt 3 ]; then
    echo -e "${YELLOW}=========================================="
    echo "⚠ System is mostly ready with minor issues"
    echo "=========================================="
    echo ""
    echo "Recommended actions:"
    echo "  1. Review failed checks above"
    echo "  2. Run with --fix option: ./verify_ml_pipeline_production_ready.sh --fix"
    echo "  3. Re-run verification"
    exit 1
else
    echo -e "${RED}=========================================="
    echo "✗ System is NOT ready for production"
    echo "=========================================="
    echo ""
    echo "Required actions:"
    echo "  1. Fix all failed checks"
    echo "  2. Review deployment checklist"
    echo "  3. Consult operations runbook"
    echo "  4. Contact ML operations team"
    exit 2
fi
