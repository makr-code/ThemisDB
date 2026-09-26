#!/bin/bash

#############################################################################
# ThemisDB Workflow Registry Sync Hook
# Purpose: Enforce that all .github/workflows/*.yml are documented in
#          WORKFLOW_REGISTRY.md before commit
# Trigger: Automatic on `git commit` when .github/workflows/*.yml changes
# Author: CI/CD Governance
# Date: 2026-09-26
#############################################################################

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
WORKFLOWS_DIR="${REPO_ROOT}/.github/workflows"
REGISTRY_FILE="${REPO_ROOT}/.github/WORKFLOW_REGISTRY.md"

# Get list of all workflow files
all_workflows=$(find "$WORKFLOWS_DIR" -maxdepth 1 -name "*.yml" -type f -exec basename {} \; | sort)
workflow_count=$(echo "$all_workflows" | wc -l)

# Get workflows mentioned in REGISTRY
documented_workflows=$(grep -o "\`[a-z0-9_-]*\.yml\`" "$REGISTRY_FILE" | sed 's/`//g' | sort -u)
documented_count=$(echo "$documented_workflows" | wc -l)

# Check for undocumented workflows
undocumented=""
for wf in $all_workflows; do
    if ! echo "$documented_workflows" | grep -q "^${wf}$"; then
        undocumented="${undocumented}${wf}\n"
    fi
done

if [ -n "$undocumented" ]; then
    echo ""
    echo "❌ WORKFLOW REGISTRY SYNC CHECK FAILED"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Total workflows: ${workflow_count}"
    echo "  Documented: ${documented_count}"
    echo "  Missing from REGISTRY:"
    echo -e "$undocumented" | sed 's/^/    - /'
    echo ""
    echo "  Action: Update .github/WORKFLOW_REGISTRY.md with the missing workflows"
    echo "          and commit again."
    echo ""
    exit 1
fi

echo "✅ Workflow Registry Sync: OK (${workflow_count} workflows documented)"
exit 0
