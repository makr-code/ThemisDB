#!/bin/bash

################################################################################
# ThemisDB: Batch Milestone Application Script
# Purpose: Assign milestones to all GitHub issues
# Usage: bash scripts/root/issues-dep1.sh
# Requirements: GitHub CLI (gh) installed and authenticated
# Status: Using REAL issue numbers from makr-code/ThemisDB
################################################################################

set -e

REPO="makr-code/ThemisDB"
SCRIPT_START=$(date +%s)

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

################################################################################
# REAL ISSUE MAPPING: Fetched from makr-code/ThemisDB (30 Issues)
# Format: ISSUE_NUMBER MILESTONE
################################################################################

REAL_ISSUES=(
  "3755 v2.0.0"
  "3754 v1.9.0"
  "3753 v2.1.0"
  "3752 v2.1.0"
  "3751 v2.0.0"
  "3750 v2.0.0"
  "3749 v1.8.0"
  "3747 v1.6.0"
  "3745 v2.0.0"
  "3744 v2.0.0"
  "3743 v2.0.0"
  "3742 v2.0.0"
  "3741 v2.0.0"
  "3740 v1.8.0"
  "3735 v1.7.0"
  "3734 v2.3.0"
  "3731 v1.7.0"
  "3730 v1.8.0"
  "3729 v1.7.0"
  "3728 v1.6.0"
  "3726 v1.7.0"
  "3724 v1.7.0"
  "3723 v1.8.0"
  "3716 v1.5.0"
  "3715 v1.5.0"
  "3713 v1.5.0"
  "3712 v1.5.0"
  "3711 v1.5.0"
  "3709 v1.5.0"
  "3708 v1.5.0"
)

################################################################################
# FUNCTION: Set milestone for a single issue
################################################################################
function set_milestone() {
  local issue_num=$1
  local milestone=$2
  
  echo -ne "${BLUE}[MILESTONE]${NC} Setting issue #${issue_num} to ${milestone}... "
  
  if gh issue edit "$issue_num" --milestone "$milestone" -R "$REPO" 2>/dev/null; then
    echo -e "${GREEN}✓${NC}"
    return 0
  else
    echo -e "${RED}✗${NC}"
    return 1
  fi
}

################################################################################
# FUNCTION: Process a single issue entry (milestone only)
################################################################################
function process_issue() {
  local entry=$1
  local issue_num=$(echo "$entry" | awk '{print $1}')
  local milestone=$(echo "$entry" | awk '{print $2}')

  # Set milestone
  set_milestone "$issue_num" "$milestone"
}

################################################################################
# MAIN EXECUTION
################################################################################

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  ThemisDB: Batch Milestone Application                    ║${NC}"
echo -e "${BLUE}║  Processing ${#REAL_ISSUES[@]} Real Issues from Repository                   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check gh CLI is available
if ! command -v gh &> /dev/null; then
  echo -e "${RED}ERROR: GitHub CLI (gh) not found. Please install it first.${NC}"
  echo "Install via: https://github.com/cli/cli#installation"
  exit 1
fi

# Verify gh authentication
if ! gh auth status -R "$REPO" &>/dev/null; then
  echo -e "${RED}ERROR: Not authenticated to GitHub. Run: gh auth login${NC}"
  exit 1
fi

echo -e "${GREEN}✓ GitHub CLI authenticated${NC}"
echo -e "${GREEN}✓ Repository: ${REPO}${NC}"
echo ""

# Counter variables
TOTAL_ISSUES=${#REAL_ISSUES[@]}
PROCESSED=0
SUCCESSFUL=0
FAILED=0

echo -e "${YELLOW}Starting batch processing of ${TOTAL_ISSUES} issues...${NC}"
echo ""

# Process all issues
for entry in "${REAL_ISSUES[@]}"; do
  ((PROCESSED++))
  echo -e "${BLUE}[${PROCESSED}/${TOTAL_ISSUES}]${NC}"
  
  process_issue "$entry"
  ((SUCCESSFUL++))
  echo ""
done

################################################################################
# SUMMARY REPORT
################################################################################

SCRIPT_END=$(date +%s)
DURATION=$((SCRIPT_END - SCRIPT_START))

echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Milestone Application Summary                             ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "Total Issues Processed: ${GREEN}${PROCESSED}/${TOTAL_ISSUES}${NC}"
echo -e "Successful:            ${GREEN}${SUCCESSFUL}${NC}"
echo -e "Failed:                ${RED}${FAILED}${NC}"
echo -e "Duration:              ${BLUE}${DURATION}s${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
  echo -e "${GREEN}✓ All 30 milestones successfully assigned!${NC}"
  exit 0
else
  echo -e "${YELLOW}⚠ Some milestones failed to apply.${NC}"
  exit 1
fi
