#!/bin/bash

# Configuration Section
declare -A ISSUE_MILESTONES
declare -A ISSUE_DEPENDENCIES
declare -A ISSUE_LABELS

# Map issue numbers to milestones
ISSUE_MILESTONES=(
    [1]="v1.5.0"    # Issue #1 is related to milestone v1.5.0
    [2]="v1.5.0"    # Issue #2 is related to milestone v1.5.0
    [3]="v1.6.0"    # Issue #3 is related to milestone v1.6.0
    # ... add remaining issues and their milestones
    [128]="v2.2.0"  # Issue #128 is related to milestone v2.2.0
)

# Map issue numbers to dependencies
ISSUE_DEPENDENCIES=(
    [1]="2"  # Issue #1 blocks issue #2
    [3]="4"  # Issue #3 blocks issue #4
    # ... add remaining dependencies
)

# Map issue numbers to labels
ISSUE_LABELS=(
    [1]="priority:High"
    [2]="priority:Medium"
    [3]="priority:Low"
    # ... add remaining issues and their labels
)

# Helper Function for Error Handling
handle_error() {
    echo "Error: $1"
    exit 1
}

# Function to set milestone for an issue
set_milestone() {
    local issue_number=$1
    local milestone=${ISSUE_MILESTONES[$issue_number]}
    echo "Setting milestone '$milestone' for issue #$issue_number..."
    gh issue edit $issue_number --milestone "$milestone" || handle_error "Failed to set milestone for issue #$issue_number"
}

# Function to create blocking relationship between issues
set_dependency() {
    local blocker=$1
    local blocked=$2
    echo "Setting issue #$blocker to block issue #$blocked..."
    gh issue edit $blocked --add-blocker $blocker || handle_error "Failed to set blocker relationship from issue #$blocker to issue #$blocked"
}

# Function to apply labels to an issue
apply_labels() {
    local issue_number=$1
    local labels=${ISSUE_LABELS[$issue_number]}
    echo "Applying labels '$labels' to issue #$issue_number..."
    gh issue edit $issue_number --add-label "$labels" || handle_error "Failed to apply labels to issue #$issue_number"
}

# Main Script Execution
for issue in "">${!ISSUE_MILESTONES[@]}"; do
    set_milestone $issue
    apply_labels $issue
    
    # Check for dependencies
    if [[ -v ISSUE_DEPENDENCIES[$issue] ]]; then
        for dependent in ${ISSUE_DEPENDENCIES[$issue]//,/ }; do
            set_dependency $issue $dependent
        done
    fi
    
    echo "Processed issue #$issue successfully."
done

echo "All milestones, dependencies, and labels have been applied."