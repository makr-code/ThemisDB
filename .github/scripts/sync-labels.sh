#!/bin/bash
# GitHub Labels Sync Script for ThemisDB
# This is a wrapper script that calls the Python implementation
#
# Usage:
#   ./sync-labels.sh                    # Dry-run mode (shows what would be done)
#   ./sync-labels.sh --apply            # Actually apply changes to GitHub
#   ./sync-labels.sh --delete-existing  # Delete all existing labels first (dangerous!)
#
# Prerequisites:
#   - Python 3.x with PyYAML installed
#   - GitHub CLI (gh) installed and authenticated
#   - Appropriate permissions on the repository

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/sync-labels.py"

# Check if Python script exists
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo "Error: Python script not found at $PYTHON_SCRIPT"
    exit 1
fi

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed."
    echo "Please install Python 3 from: https://www.python.org/"
    exit 1
fi

# Forward all arguments to the Python script
exec python3 "$PYTHON_SCRIPT" "$@"

