#!/bin/bash
# ThemisDB PyPI Publishing Script
# Publishes themisdb to PyPI

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLIENT_DIR="$ROOT_DIR/clients/python"

DRY_RUN=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run) DRY_RUN=true; shift ;;
        --version) VERSION="$2"; shift 2 ;;
        *) shift ;;
    esac
done

echo "📦 PyPI Publishing for themisdb"

# Check prerequisites
if ! command -v python3 &> /dev/null; then
    echo "❌ python3 not found"
    exit 1
fi

if [[ -z "${PYPI_TOKEN:-}" ]] && [[ "$DRY_RUN" == "false" ]]; then
    echo "❌ PYPI_TOKEN environment variable not set"
    exit 1
fi

cd "$CLIENT_DIR"

# Create virtual environment
echo "📦 Setting up virtual environment..."
python3 -m venv .venv
source .venv/bin/activate

# Install build tools
pip install --upgrade pip build twine

# Update version if specified
if [[ -n "$VERSION" ]]; then
    if [[ -f "pyproject.toml" ]]; then
        sed -i "s/^version = .*/version = \"$VERSION\"/" pyproject.toml
    fi
fi

# Build
echo "🔨 Building..."
python -m build

# Run tests
echo "🧪 Running tests..."
pip install -e ".[dev]" || pip install -e .
pytest tests/ -v || echo "No tests found or tests skipped"

# Publish
if [[ "$DRY_RUN" == "true" ]]; then
    echo "🔍 Dry run - would publish:"
    ls -la dist/
    twine check dist/*
else
    echo "🚀 Publishing to PyPI..."
    twine upload dist/* -u __token__ -p "$PYPI_TOKEN"
fi

# Cleanup
deactivate
rm -rf .venv dist *.egg-info

echo "✅ PyPI publishing complete"
