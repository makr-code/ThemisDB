#!/bin/bash
set -e

# ThemisDB LLM RAID Pipeline Test Entrypoint with Orchestration
# Orchestriert: Daten pushen -> Tests laufen -> Metriken sammeln -> Ergebnisse validieren

TEST_TYPE=${1:-"all"}
RESULTS_DIR="/test_results"
ORCHESTRATOR_SCRIPT="/opt/themis/bin/raid_lora_orchestrator.py"

# Create directories
mkdir -p "$RESULTS_DIR" "/test_data" "/test_models" "/test_loras"

echo "=========================================="
echo "ThemisDB LLM RAID Pipeline - Orchestrated"
echo "Test Type: $TEST_TYPE"
echo "Timestamp: $(date)"
echo "=========================================="

# Check if orchestrator script exists
if [ -f "$ORCHESTRATOR_SCRIPT" ]; then
    echo ""
    echo "Running Orchestrated Pipeline..."
    echo "(Daten pushen -> Tests -> Metriken sammeln -> Validierung -> Report)"
    echo ""
    
    python3 "$ORCHESTRATOR_SCRIPT" "$TEST_TYPE"
    ORCHESTRATOR_EXIT=$?
    
    echo ""
    echo "Orchestrator exited with code: $ORCHESTRATOR_EXIT"
    
    # Copy orchestrator results
    if [ -f "$RESULTS_DIR/orchestrator_results.json" ]; then
        echo "Orchestrator results saved"
    fi
    
    if [ -f "$RESULTS_DIR/test_report.html" ]; then
        echo "HTML report generated: $RESULTS_DIR/test_report.html"
    fi
else
    echo "Orchestrator script not found: $ORCHESTRATOR_SCRIPT"
    echo "Falling back to direct test execution..."
fi

echo ""
echo "Result files in $RESULTS_DIR:"
ls -lh "$RESULTS_DIR" 2>/dev/null || echo "No results yet"

echo ""
echo "=========================================="
echo "Test Execution Completed"
echo "=========================================="
