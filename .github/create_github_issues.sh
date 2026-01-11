#!/bin/bash
# Auto-create GitHub Issues from .github/issues/*.md files
# Usage: ./create_github_issues.sh

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ISSUES_DIR="${SCRIPT_DIR}/issues"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed.${NC}"
    echo "Install it from: https://cli.github.com/"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    echo -e "${RED}Error: Not authenticated with GitHub CLI.${NC}"
    echo "Run: gh auth login"
    exit 1
fi

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║          ThemisDB GitHub Issues Auto-Creator                  ║${NC}"
echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo ""

# Check if issues directory exists
if [ ! -d "$ISSUES_DIR" ]; then
    echo -e "${RED}Error: Issues directory not found: $ISSUES_DIR${NC}"
    exit 1
fi

# Find all issue markdown files
ISSUE_FILES=($(find "$ISSUES_DIR" -name "*.md" -type f ! -name "README.md" ! -name "COMPLETE_TODO_INVENTORY.md" ! -name "DOCUMENTATION_TODOS_ANALYSIS.md" | sort))

if [ ${#ISSUE_FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}No issue files found in $ISSUES_DIR${NC}"
    exit 0
fi

echo -e "${GREEN}Found ${#ISSUE_FILES[@]} issue files:${NC}"
for file in "${ISSUE_FILES[@]}"; do
    echo -e "  - $(basename "$file")"
done
echo ""

# Ask for confirmation
read -p "Create GitHub issues for all files? (y/N): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Aborted.${NC}"
    exit 0
fi

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    Creating Issues...                          ${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo ""

CREATED_COUNT=0
SKIPPED_COUNT=0
ERROR_COUNT=0

# Array to store created issue URLs
declare -a CREATED_ISSUES

for issue_file in "${ISSUE_FILES[@]}"; do
    BASENAME=$(basename "$issue_file")
    
    echo -e "${BLUE}Processing: ${BASENAME}${NC}"
    
    # Extract title from markdown (first line starting with 'title:' in frontmatter)
    TITLE=$(grep -m 1 "^title:" "$issue_file" | sed 's/^title: *//; s/"//g; s/^"//; s/"$//')
    
    if [ -z "$TITLE" ]; then
        # Fallback: use filename as title
        TITLE=$(basename "$issue_file" .md | sed 's/-/ /g; s/^[0-9]*//; s/^ *//')
        echo -e "${YELLOW}  Warning: No title found in frontmatter, using filename: $TITLE${NC}"
    fi
    
    # Extract labels from frontmatter
    LABELS=$(grep -m 1 "^labels:" "$issue_file" | sed 's/^labels: *//')
    
    # Extract milestone from frontmatter
    MILESTONE=$(grep -m 1 "^milestone:" "$issue_file" | sed 's/^milestone: *//')
    
    # Remove frontmatter from body (everything between --- and ---)
    BODY=$(awk '/^---$/{if(++count==2){flag=1;next}}flag' "$issue_file")
    
    # Check if issue with same title already exists
    EXISTING_ISSUE=$(gh issue list --search "in:title \"$TITLE\"" --json number,title --jq ".[] | select(.title == \"$TITLE\") | .number" 2>/dev/null || echo "")
    
    if [ -n "$EXISTING_ISSUE" ]; then
        echo -e "${YELLOW}  ⊘ Skipped: Issue already exists (#${EXISTING_ISSUE})${NC}"
        SKIPPED_COUNT=$((SKIPPED_COUNT + 1))
        echo ""
        continue
    fi
    
    # Build gh issue create command
    CMD="gh issue create --title \"$TITLE\" --body-file /dev/stdin"
    
    # Add labels if present
    if [ -n "$LABELS" ]; then
        # Convert comma-separated labels to space-separated and remove spaces around commas
        LABELS_CLEANED=$(echo "$LABELS" | sed 's/, */ /g; s/,/ /g')
        CMD="$CMD --label \"$LABELS_CLEANED\""
    fi
    
    # Add milestone if present
    if [ -n "$MILESTONE" ]; then
        CMD="$CMD --milestone \"$MILESTONE\""
    fi
    
    # Create the issue
    echo "$BODY" | eval "$CMD" > /tmp/gh_issue_output.txt 2>&1
    
    if [ $? -eq 0 ]; then
        ISSUE_URL=$(cat /tmp/gh_issue_output.txt)
        ISSUE_NUMBER=$(echo "$ISSUE_URL" | grep -oE '[0-9]+$')
        echo -e "${GREEN}  ✓ Created: Issue #${ISSUE_NUMBER}${NC}"
        echo -e "    ${ISSUE_URL}"
        CREATED_ISSUES+=("$BASENAME → #${ISSUE_NUMBER} - $TITLE")
        CREATED_COUNT=$((CREATED_COUNT + 1))
    else
        echo -e "${RED}  ✗ Error creating issue${NC}"
        cat /tmp/gh_issue_output.txt
        ERROR_COUNT=$((ERROR_COUNT + 1))
    fi
    
    echo ""
    
    # Rate limiting: wait 2 seconds between API calls
    sleep 2
done

# Clean up
rm -f /tmp/gh_issue_output.txt

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                         Summary                                ${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${GREEN}Created:${NC}  $CREATED_COUNT issues"
echo -e "${YELLOW}Skipped:${NC}  $SKIPPED_COUNT issues (already exist)"
echo -e "${RED}Errors:${NC}   $ERROR_COUNT issues"
echo ""

if [ ${#CREATED_ISSUES[@]} -gt 0 ]; then
    echo -e "${GREEN}Created Issues:${NC}"
    for issue_info in "${CREATED_ISSUES[@]}"; do
        echo -e "  ✓ $issue_info"
    done
    echo ""
fi

if [ $CREATED_COUNT -gt 0 ]; then
    echo -e "${GREEN}✓ Successfully created $CREATED_COUNT GitHub issues!${NC}"
    echo ""
    echo -e "${BLUE}View all issues:${NC}"
    echo "  gh issue list"
    echo ""
    echo -e "${BLUE}Or visit:${NC}"
    REPO=$(gh repo view --json nameWithOwner -q .nameWithOwner)
    echo "  https://github.com/${REPO}/issues"
fi

exit 0
