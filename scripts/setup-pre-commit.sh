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

# Install gitleaks (required by the secret-scanning pre-commit hook)
echo "🔍 Checking for gitleaks..."
if ! command -v gitleaks &> /dev/null; then
    echo "⚠️  gitleaks not found.  Installing via package manager or manual download..."
    if command -v brew &> /dev/null; then
        brew install gitleaks
    elif command -v apt-get &> /dev/null; then
        # Install latest release from GitHub
        GITLEAKS_VERSION="8.18.2"
        GITLEAKS_ARCH="linux_x64"
        curl -sSfL \
            "https://github.com/zricethezav/gitleaks/releases/download/v${GITLEAKS_VERSION}/gitleaks_${GITLEAKS_VERSION}_${GITLEAKS_ARCH}.tar.gz" \
            -o /tmp/gitleaks.tar.gz
        tar -xzf /tmp/gitleaks.tar.gz -C /tmp gitleaks
        sudo mv /tmp/gitleaks /usr/local/bin/gitleaks
        rm -f /tmp/gitleaks.tar.gz
        echo "✅ gitleaks installed to /usr/local/bin/gitleaks"
    else
        echo "⚠️  Cannot auto-install gitleaks on this system."
        echo "Download from: https://github.com/zricethezav/gitleaks/releases"
        echo "The 'gitleaks' hook will be skipped until gitleaks is in PATH."
    fi
else
    echo "✅ gitleaks found: $(gitleaks version 2>/dev/null || gitleaks --version)"
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
echo "Included secret-scanning hooks:"
echo "  • detect-secrets  – keyword + entropy baseline check"
echo "  • gitleaks        – git-history secret scan"
echo "  • secret_scan.py  – Shannon entropy (≥ 4.5 bits/char) + pattern scan"
echo ""
echo "To manually run hooks: pre-commit run --all-files"
echo "To update hooks: pre-commit autoupdate"
echo "To scan all files directly: python3 scripts/secret_scan.py --all"
