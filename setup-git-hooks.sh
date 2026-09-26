#!/bin/bash
# ThemisDB Git Hooks Setup
# Configures git to use .githooks directory for hook enforcement
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOOKS_DIR="$REPO_ROOT/.githooks"

echo "🔧 Setting up Git hooks in $REPO_ROOT..."

# Configure git to use .githooks directory
git -C "$REPO_ROOT" config core.hooksPath .githooks

# Verify
CONFIGURED_PATH=$(git -C "$REPO_ROOT" config core.hooksPath)
if [ "$CONFIGURED_PATH" = ".githooks" ]; then
    echo "✅ Git hooks configured successfully"
    echo "   core.hooksPath = $CONFIGURED_PATH"
    echo ""
    echo "Installed hooks:"
    find "$HOOKS_DIR" -maxdepth 1 -type f -executable | sort | sed 's|.*/|  - |'
else
    echo "❌ Failed to configure git hooks"
    exit 1
fi
