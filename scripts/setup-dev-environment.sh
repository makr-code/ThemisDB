#!/bin/bash
# Complete Development Environment Setup for ThemisDB
# This script sets up everything needed for ThemisDB development

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${GREEN}🚀 ThemisDB Complete Development Environment Setup${NC}"
echo ""

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to print status
print_status() {
    echo -e "${CYAN}==>${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check prerequisites
print_status "Checking prerequisites..."

MISSING_DEPS=()

if ! command_exists git; then
    MISSING_DEPS+=("git")
fi

if ! command_exists python3; then
    MISSING_DEPS+=("python3")
fi

if ! command_exists cmake; then
    MISSING_DEPS+=("cmake")
fi

if ! command_exists ninja; then
    MISSING_DEPS+=("ninja-build")
fi

if ! command_exists g++; then
    MISSING_DEPS+=("g++")
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    print_error "Missing required dependencies: ${MISSING_DEPS[*]}"
    echo ""
    echo "Install them with:"
    echo "  sudo apt-get install -y ${MISSING_DEPS[*]}"
    exit 1
fi

print_success "All prerequisites installed"

# 1. Install Pre-commit Hooks
print_status "Installing pre-commit hooks..."
if command_exists pip3; then
    pip3 install --user pre-commit
    pre-commit install
    print_success "Pre-commit hooks installed"
else
    print_warning "pip3 not found, skipping pre-commit installation"
fi

# 2. Setup vcpkg
print_status "Setting up vcpkg..."
if [ -d "vcpkg" ]; then
    if [ ! -f "vcpkg/vcpkg" ]; then
        print_status "Bootstrapping vcpkg..."
        cd vcpkg
        ./bootstrap-vcpkg.sh -disableMetrics
        cd ..
        print_success "vcpkg bootstrapped"
    else
        print_success "vcpkg already bootstrapped"
    fi
else
    print_error "vcpkg directory not found. Clone the repository with submodules:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

# 3. Install vcpkg dependencies (optional, can be time-consuming)
read -p "Install vcpkg dependencies now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Installing vcpkg dependencies (this may take 30-60 minutes)..."
    ./vcpkg/vcpkg install --triplet x64-linux
    print_success "vcpkg dependencies installed"
else
    print_warning "Skipping vcpkg dependency installation"
    echo "  Run later: ./vcpkg/vcpkg install --triplet x64-linux"
fi

# 4. Copy VSCode configuration (if not exists)
print_status "Setting up VSCode configuration..."
if [ ! -d ".vscode" ]; then
    if [ -d ".vscode.example" ]; then
        cp -r .vscode.example .vscode
        print_success "VSCode configuration copied from .vscode.example"
    else
        print_warning "No .vscode.example found"
    fi
else
    print_success ".vscode directory already exists"
fi

# 5. Create initial build directory
print_status "Creating initial build directory..."
mkdir -p build-dev
print_success "Build directory created: build-dev"

# 6. Generate compile_commands.json
print_status "Configuring CMake..."
if cmake -S . -B build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; then
    print_success "CMake configured successfully"
    
    # Create symbolic link for IntelliSense
    if [ -f "build-dev/compile_commands.json" ]; then
        ln -sf build-dev/compile_commands.json compile_commands.json
        print_success "Created compile_commands.json symlink for IntelliSense"
    fi
else
    print_warning "CMake configuration failed (you may need to install dependencies first)"
fi

# 7. Setup Git hooks
print_status "Configuring Git..."
if [ -z "$(git config --global user.name)" ]; then
    print_warning "Git user.name not set. Please configure:"
    echo "  git config --global user.name 'Your Name'"
fi
if [ -z "$(git config --global user.email)" ]; then
    print_warning "Git user.email not set. Please configure:"
    echo "  git config --global user.email 'your.email@example.com'"
fi

# Summary
echo ""
echo -e "${GREEN}✓ Development environment setup complete!${NC}"
echo ""
echo "Next steps:"
echo "  1. Open project in VSCode: code ."
echo "  2. Install recommended extensions (VSCode will prompt)"
echo "  3. Select CMake kit: Ctrl+Shift+P → 'CMake: Select a Kit'"
echo "  4. Build: Press F7 or Ctrl+Shift+P → 'CMake: Build'"
echo "  5. Run tests: Ctrl+Shift+P → 'CMake: Run Tests'"
echo ""
echo "Documentation:"
echo "  - Setup Guide: SETUP.md"
echo "  - Build Guide: .github/copilot/BUILD_GUIDE.md"
echo "  - VSCode Guide: .github/copilot/VSCODE_CONTEXT.md"
echo ""
echo "Happy coding! 🎉"
