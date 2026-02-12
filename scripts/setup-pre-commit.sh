#!/bin/bash
# Setup script for pre-commit hooks
# This script installs pre-commit and configures it for ThemisDB

set -e

echo "🚀 Setting up pre-commit hooks for ThemisDB..."

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "❌ Error: Python 3 is required but not installed."
    echo "Please install Python 3 and try again."
    exit 1
fi

# Check if pip is installed
if ! command -v pip3 &> /dev/null; then
    echo "❌ Error: pip3 is required but not installed."
    echo "Please install pip3 and try again."
    exit 1
fi

# Install pre-commit
echo "📦 Installing pre-commit..."
pip3 install --user pre-commit

# Verify installation
if ! command -v pre-commit &> /dev/null; then
    echo "⚠️  pre-commit was installed but not found in PATH."
    echo "You may need to add ~/.local/bin to your PATH."
    echo "Add this to your ~/.bashrc or ~/.zshrc:"
    echo '  export PATH="$HOME/.local/bin:$PATH"'
    exit 1
fi

# Install git hooks
echo "🪝 Installing git hooks..."
pre-commit install

# Run hooks on all files (optional, to verify setup)
echo ""
echo "✅ Pre-commit hooks installed successfully!"
echo ""
echo "Would you like to run hooks on all files now? (y/n)"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo "🔍 Running pre-commit on all files..."
    pre-commit run --all-files || echo "⚠️  Some hooks failed. Please review and fix."
else
    echo "Skipping initial run. Hooks will run automatically on commit."
fi

echo ""
echo "🎉 Setup complete!"
echo ""
echo "Pre-commit hooks are now active. They will run automatically before each commit."
echo "To manually run hooks: pre-commit run --all-files"
echo "To update hooks: pre-commit autoupdate"
