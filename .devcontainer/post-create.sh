#!/bin/bash
set -e

echo "🚀 Setting up ThemisDB development environment..."

if [ -f "vcpkg.json" ]; then
    echo "📦 Installing vcpkg dependencies..."
    vcpkg install --triplet x64-linux || echo "⚠️  Some packages failed to install"
fi

if [ -f ".pre-commit-config.yaml" ]; then
    echo "🪝 Installing pre-commit hooks..."
    pre-commit install || echo "⚠️  Pre-commit installation failed"
fi

if [ -z "$(git config --global user.name)" ]; then
    echo "📝 Configuring git..."
    git config --global user.name "Dev Container User"
    git config --global user.email "dev@themisdb.local"
fi

echo "🔨 Creating build directory..."
mkdir -p build-dev

echo "✅ Development environment ready!"
echo ""
echo "Next steps:"
echo "  1. Configure CMake: cmake --preset linux-gcc-release"
echo "  2. Build: cmake --build build-dev"
echo "  3. Run tests: ctest --test-dir build-dev"
