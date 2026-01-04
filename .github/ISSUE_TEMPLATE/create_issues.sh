#!/bin/bash
# Script to create GitHub issues from templates in batch
# Usage: ./create_issues.sh [phase]
#   phase: all, phase1, phase2, phase3, or specific issue number

set -e

REPO="makr-code/ThemisDB"
TEMPLATE_DIR=".github/ISSUE_TEMPLATE"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed${NC}"
    echo "Install from: https://cli.github.com/"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    echo -e "${RED}Error: Not authenticated with GitHub${NC}"
    echo "Run: gh auth login"
    exit 1
fi

echo -e "${GREEN}Creating GitHub issues from templates...${NC}\n"

# Function to create an issue
create_issue() {
    local title="$1"
    local labels="$2"
    local template_file="$3"
    
    echo -e "${YELLOW}Creating issue: $title${NC}"
    
    if gh issue create \
        --repo "$REPO" \
        --title "$title" \
        --label "$labels" \
        --body-file "$TEMPLATE_DIR/$template_file" 2>&1 | tee /tmp/gh_output.txt
    then
        local issue_url=$(grep "https://github.com" /tmp/gh_output.txt)
        echo -e "${GREEN}✓ Created: $issue_url${NC}\n"
        return 0
    else
        echo -e "${RED}✗ Failed to create issue${NC}\n"
        return 1
    fi
}

# Phase 1: v1.4.0 - High Priority
create_phase1() {
    echo -e "${GREEN}=== Phase 1 (v1.4.0) - High Priority ===${NC}\n"
    
    create_issue \
        "[v1.4.0] Implement RAID 6 (Dual Parity) Support" \
        "enhancement,raid,high-priority,v1.4.0" \
        "01_raid6_dual_parity.md"
    
    create_issue \
        "[v1.4.0] Implement LoRA Quantization (INT8/INT4)" \
        "enhancement,lora,high-priority,v1.4.0" \
        "02_lora_quantization.md"
    
    create_issue \
        "[v1.4.0] Implement Hot Spare Management System" \
        "enhancement,raid,operations,high-priority,v1.4.0" \
        "03_hot_spare_management.md"
    
    create_issue \
        "[v1.4.0] Implement Multi-GPU LoRA Support" \
        "enhancement,lora,gpu,high-priority,v1.4.0" \
        "04_multi_gpu_lora.md"
}

# Phase 2: v1.5.0 - Medium Priority
create_phase2() {
    echo -e "${GREEN}=== Phase 2 (v1.5.0) - Medium Priority ===${NC}\n"
    
    create_issue \
        "[v1.5.0] Implement GPU-Accelerated Erasure Coding" \
        "enhancement,raid,performance,v1.5.0" \
        "05_gpu_erasure_coding.md"
    
    # Add more Phase 2 issues here as templates are created
}

# Phase 3: v1.6.0 - Strategic
create_phase3() {
    echo -e "${GREEN}=== Phase 3 (v1.6.0) - Strategic ===${NC}\n"
    
    create_issue \
        "[v1.6.0] Implement Predictive Failure Detection" \
        "enhancement,operations,ml,v1.6.0" \
        "06_predictive_failure_detection.md"
    
    # Add more Phase 3 issues here as templates are created
}

# Parse command line arguments
case "${1:-all}" in
    all)
        create_phase1
        create_phase2
        create_phase3
        ;;
    phase1)
        create_phase1
        ;;
    phase2)
        create_phase2
        ;;
    phase3)
        create_phase3
        ;;
    1)
        create_issue \
            "[v1.4.0] Implement RAID 6 (Dual Parity) Support" \
            "enhancement,raid,high-priority,v1.4.0" \
            "01_raid6_dual_parity.md"
        ;;
    2)
        create_issue \
            "[v1.4.0] Implement LoRA Quantization (INT8/INT4)" \
            "enhancement,lora,high-priority,v1.4.0" \
            "02_lora_quantization.md"
        ;;
    3)
        create_issue \
            "[v1.4.0] Implement Hot Spare Management System" \
            "enhancement,raid,operations,high-priority,v1.4.0" \
            "03_hot_spare_management.md"
        ;;
    4)
        create_issue \
            "[v1.4.0] Implement Multi-GPU LoRA Support" \
            "enhancement,lora,gpu,high-priority,v1.4.0" \
            "04_multi_gpu_lora.md"
        ;;
    5)
        create_issue \
            "[v1.5.0] Implement GPU-Accelerated Erasure Coding" \
            "enhancement,raid,performance,v1.5.0" \
            "05_gpu_erasure_coding.md"
        ;;
    6)
        create_issue \
            "[v1.6.0] Implement Predictive Failure Detection" \
            "enhancement,operations,ml,v1.6.0" \
            "06_predictive_failure_detection.md"
        ;;
    help|--help|-h)
        echo "Usage: $0 [option]"
        echo ""
        echo "Options:"
        echo "  all       Create all issues (default)"
        echo "  phase1    Create Phase 1 (v1.4.0) issues only"
        echo "  phase2    Create Phase 2 (v1.5.0) issues only"
        echo "  phase3    Create Phase 3 (v1.6.0) issues only"
        echo "  1-6       Create specific issue by number"
        echo "  help      Show this help message"
        echo ""
        echo "Examples:"
        echo "  $0              # Create all issues"
        echo "  $0 phase1       # Create only Phase 1 issues"
        echo "  $0 1            # Create only RAID 6 issue"
        exit 0
        ;;
    *)
        echo -e "${RED}Invalid option: $1${NC}"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac

echo -e "${GREEN}Done!${NC}"
echo -e "View all issues at: https://github.com/$REPO/issues"
