#!/bin/bash
# GitHub Labels Sync Script for ThemisDB
# This script synchronizes labels from .github/labels.yml to GitHub
#
# Usage:
#   ./sync-labels.sh                    # Dry-run mode (shows what would be done)
#   ./sync-labels.sh --apply            # Actually apply changes to GitHub
#   ./sync-labels.sh --delete-existing  # Delete all existing labels first (dangerous!)
#
# Prerequisites:
#   - GitHub CLI (gh) installed and authenticated
#   - Appropriate permissions on the repository

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LABELS_FILE="$REPO_ROOT/.github/labels.yml"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse command line arguments
DRY_RUN=true
DELETE_EXISTING=false

for arg in "$@"; do
    case $arg in
        --apply)
            DRY_RUN=false
            shift
            ;;
        --delete-existing)
            DELETE_EXISTING=true
            shift
            ;;
        --help)
            echo "Usage: $0 [--apply] [--delete-existing]"
            echo ""
            echo "Options:"
            echo "  --apply            Actually apply changes (default is dry-run)"
            echo "  --delete-existing  Delete all existing labels before syncing"
            echo "  --help             Show this help message"
            exit 0
            ;;
    esac
done

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed.${NC}"
    echo "Please install it from: https://cli.github.com/"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    echo -e "${RED}Error: Not authenticated with GitHub CLI.${NC}"
    echo "Please run: gh auth login"
    exit 1
fi

# Check if labels file exists
if [ ! -f "$LABELS_FILE" ]; then
    echo -e "${RED}Error: Labels file not found: $LABELS_FILE${NC}"
    exit 1
fi

# Check if yq is installed for YAML parsing
if ! command -v yq &> /dev/null; then
    echo -e "${YELLOW}Warning: yq is not installed. Using Python for YAML parsing.${NC}"
    USE_PYTHON=true
else
    USE_PYTHON=false
fi

echo -e "${BLUE}════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  ThemisDB GitHub Labels Sync${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════${NC}"
echo ""

if [ "$DRY_RUN" = true ]; then
    echo -e "${YELLOW}⚠️  DRY-RUN MODE: No changes will be made${NC}"
    echo -e "${YELLOW}   Run with --apply to actually sync labels${NC}"
    echo ""
fi

if [ "$DELETE_EXISTING" = true ]; then
    echo -e "${RED}⚠️  WARNING: Will delete all existing labels!${NC}"
    if [ "$DRY_RUN" = false ]; then
        echo -n "Are you sure? (yes/no): "
        read -r confirm
        if [ "$confirm" != "yes" ]; then
            echo "Aborted."
            exit 0
        fi
    fi
    echo ""
fi

# Function to parse YAML and create/update labels
parse_and_sync_labels() {
    if [ "$USE_PYTHON" = true ]; then
        python3 - <<EOF
import yaml
import subprocess
import sys

# Read labels file
with open("$LABELS_FILE", "r") as f:
    labels = yaml.safe_load(f)

dry_run = $DRY_RUN
delete_existing = $DELETE_EXISTING

# Get existing labels from GitHub
print("${BLUE}Fetching existing labels from GitHub...${NC}")
try:
    result = subprocess.run(
        ["gh", "label", "list", "--json", "name,description,color", "--limit", "1000"],
        capture_output=True,
        text=True,
        check=True
    )
    import json
    existing_labels = {label["name"]: label for label in json.loads(result.stdout)}
except Exception as e:
    print(f"${RED}Error fetching labels: {e}${NC}")
    sys.exit(1)

print(f"${GREEN}Found {len(existing_labels)} existing labels${NC}")
print()

# Delete existing labels if requested
if delete_existing:
    print("${RED}Deleting all existing labels...${NC}")
    for label_name in existing_labels.keys():
        if dry_run:
            print(f"  ${YELLOW}[DRY-RUN]${NC} Would delete: {label_name}")
        else:
            try:
                subprocess.run(
                    ["gh", "label", "delete", label_name, "--yes"],
                    check=True,
                    capture_output=True
                )
                print(f"  ${RED}[-]${NC} Deleted: {label_name}")
            except Exception as e:
                print(f"  ${RED}Error deleting {label_name}: {e}${NC}")
    print()

# Process each label from the YAML file
stats = {"created": 0, "updated": 0, "unchanged": 0}

print("${BLUE}Syncing labels...${NC}")
for label in labels:
    name = label["name"]
    color = label["color"]
    description = label.get("description", "")
    
    # Check if label exists
    if name in existing_labels and not delete_existing:
        existing = existing_labels[name]
        needs_update = (
            existing["color"] != color or
            existing.get("description", "") != description
        )
        
        if needs_update:
            if dry_run:
                print(f"  ${YELLOW}[DRY-RUN]${NC} Would update: {name}")
            else:
                try:
                    subprocess.run(
                        ["gh", "label", "edit", name, "--color", color, "--description", description],
                        check=True,
                        capture_output=True
                    )
                    print(f"  ${YELLOW}[~]${NC} Updated: {name}")
                    stats["updated"] += 1
                except Exception as e:
                    print(f"  ${RED}Error updating {name}: {e}${NC}")
        else:
            stats["unchanged"] += 1
    else:
        # Create new label
        if dry_run:
            print(f"  ${YELLOW}[DRY-RUN]${NC} Would create: {name}")
        else:
            try:
                subprocess.run(
                    ["gh", "label", "create", name, "--color", color, "--description", description],
                    check=True,
                    capture_output=True
                )
                print(f"  ${GREEN}[+]${NC} Created: {name}")
                stats["created"] += 1
            except Exception as e:
                print(f"  ${RED}Error creating {name}: {e}${NC}")

# Print summary
print()
print("${BLUE}════════════════════════════════════════════════════${NC}")
print("${BLUE}  Summary${NC}")
print("${BLUE}════════════════════════════════════════════════════${NC}")
if dry_run:
    print("${YELLOW}DRY-RUN MODE - No changes were made${NC}")
print(f"${GREEN}✓${NC} Created:   {stats['created']}")
print(f"${YELLOW}~${NC} Updated:   {stats['updated']}")
print(f"  Unchanged: {stats['unchanged']}")
print()

if dry_run:
    print("${YELLOW}Run with --apply to actually sync these labels${NC}")
EOF
    else
        # Use yq for parsing
        echo "Using yq to parse labels..."
        # Implementation with yq would go here
        # For now, fallback to Python
        echo -e "${RED}yq parsing not yet implemented. Please use Python mode.${NC}"
        exit 1
    fi
}

# Run the sync
parse_and_sync_labels

echo -e "${GREEN}Done!${NC}"
