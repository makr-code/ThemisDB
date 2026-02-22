# ThemisDB Development Environment Setup Guide

This guide covers setting up your development environment for ThemisDB with AI-Guardrails and modern tooling.

## 🎯 Quick Setup (Recommended)

### Option 1: Dev Container (Fastest)

**Prerequisites:** Docker Desktop + VS Code with Remote Containers extension

```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Open in VS Code
code .

# 3. Reopen in Container
# Command Palette (Ctrl+Shift+P) → "Dev Containers: Reopen in Container"

# ✅ Environment is automatically configured!
```

### Option 2: Local Setup

**Prerequisites:** Git, Python 3.8+, CMake 3.20+, C++ compiler

```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Install pre-commit hooks
# Windows
.\scripts\setup-pre-commit.ps1

# Linux/macOS
./scripts/setup-pre-commit.sh

# 3. Bootstrap third-party dependencies (vcpkg, ffmpeg, llama.cpp)
# Windows
.\scripts\setup-third-party.ps1

# Linux/macOS
pwsh ./scripts/setup-third-party.ps1

# 4. Configure VS Code (optional)
cp -r .vscode.example .vscode

# ✅ Ready to build!
```

## 📋 Detailed Setup Instructions

### 1. Prerequisites

#### All Platforms

- **Git** - Version control
- **Python 3.8+** - For pre-commit hooks
- **CMake 3.20+** - Build system
- **vcpkg** - Package manager (included in repo)

#### Windows

- **Visual Studio 2022** - C++ compiler
  - Workload: "Desktop development with C++"
  - Components: CMake tools, vcpkg
- **Windows SDK** - Latest version

#### Linux

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    curl \
    zip \
    unzip

# Fedora/RHEL
sudo dnf install -y \
    gcc-c++ \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip
```

#### macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake ninja python git
```

### 2. Clone Repository

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

### 3. Install Pre-commit Hooks

Pre-commit hooks automatically validate code before commits.

#### Windows

```powershell
.\scripts\setup-pre-commit.ps1
```

#### Linux/macOS

```bash
./scripts/setup-pre-commit.sh
```

**What this does:**
- Installs pre-commit tool
- Configures git hooks
- Sets up validation checks:
  - Code formatting (clang-format)
  - CMake linting
  - Markdown linting
  - Secret detection
  - Copilot instruction validation

### 4. Configure third-party dependencies

ThemisDB uses vcpkg plus external components (`ffmpeg`, `llama.cpp`).

#### Windows

```powershell
.\scripts\setup-third-party.ps1
```

#### Linux/macOS

```bash
pwsh ./scripts/setup-third-party.ps1
```

**This ensures:**
- `vcpkg/scripts/buildsystems/vcpkg.cmake` is available
- `ffmpeg` and `llama.cpp` submodules are initialized
- `vcpkg.json` baseline commit is validated/fetched when possible

### 5. Configure VS Code (Optional but Recommended)

```bash
# Copy example configuration
cp -r .vscode.example .vscode

# Edit settings for your environment
code .vscode/settings.json
```

**Install recommended extensions:**
- Open VS Code Command Palette (`Ctrl+Shift+P`)
- Run: "Extensions: Show Recommended Extensions"
- Click "Install All"

**Key extensions:**
- C/C++ (ms-vscode.cpptools)
- CMake Tools (ms-vscode.cmake-tools)
- GitHub Copilot (github.copilot)
- Remote Development (ms-vscode-remote.remote-*)

### 6. First Build

#### Windows (Visual Studio)

```powershell
# Configure with preset
cmake --preset windows-vs2022-release

# Build
cmake --build build-msvc --config Release --parallel 8

# Run tests
cd build-msvc
ctest -C Release --output-on-failure
```

#### Linux (GCC/Ninja)

```bash
# Configure with preset
cmake --preset linux-gcc-release

# Build
cmake --build build-wsl --parallel $(nproc)

# Run tests
cd build-wsl
ctest --output-on-failure
```

#### Docker

```bash
# Build container
docker build -f docker/Dockerfile.themis-server -t themis-server .

# Run tests in container
docker run --rm themis-server ctest
```

## 🛠️ Development Workflow

### Daily Workflow

1. **Pull latest changes**
   ```bash
   git checkout develop
   git pull origin develop
   ```

2. **Create feature branch**
   ```bash
   git checkout -b feature/my-feature
   ```

3. **Make changes** - Edit code, add tests

4. **Pre-commit validation** - Runs automatically on commit
   ```bash
   git add .
   git commit -m "feat: Add my feature"
   # Pre-commit hooks run here
   ```

5. **Push and create PR**
   ```bash
   git push origin feature/my-feature
   # Create PR to 'develop' branch
   ```

### Code Quality

**Before committing:**
```bash
# Format code
clang-format -i src/**/*.cpp

# Run linter
clang-tidy src/**/*.cpp

# Run tests
cmake --build build --target test

# Check coverage (if enabled)
cmake --build build --target coverage
```

**Pre-commit does this automatically!**

### Building for Different Targets

#### Minimal Build (Fastest)

```bash
cmake -B build-minimal \
    -DTHEMIS_ENABLE_LLM=OFF \
    -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF \
    -DTHEMIS_BUILD_TESTS=OFF
cmake --build build-minimal
```

#### Full Build with LLM & GPU

```bash
cmake -B build-full \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_GPU=ON \
    -DTHEMIS_BUILD_RPC_FRAMEWORK=ON
cmake --build build-full
```

#### Debug Build

```bash
cmake --preset linux-gcc-debug  # or windows-vs2022-debug
cmake --build build-debug
```

## 🔍 Troubleshooting

### Pre-commit Hooks Failing

**Issue:** Hooks fail on first run

**Solution:**
```bash
# Run hooks on all files to fix formatting
pre-commit run --all-files

# Commit the fixes
git add .
git commit -m "chore: Apply pre-commit fixes"
```

### vcpkg Package Not Found

**Issue:** CMake can't find a package

**Solution:**
```bash
# Verify vcpkg installation
vcpkg list

# Reinstall specific package
vcpkg install <package> --triplet x64-windows  # or x64-linux

# Clear CMake cache and reconfigure
rm -rf build-*
cmake --preset <your-preset>
```

### IntelliSense Not Working

**Issue:** VS Code doesn't recognize code

**Solution:**
```bash
# 1. Regenerate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Restart VS Code C++ extension
# Command Palette → "C/C++: Reset IntelliSense Database"

# 3. Verify configuration
# Check .vscode/c_cpp_properties.json has correct paths
```

### Build Errors

**Issue:** CMake configuration fails

**Solution:**
```bash
# Check CMAKE_PREFIX_PATH
cmake -L build | grep PREFIX_PATH

# For vcpkg issues on Windows:
.\scripts\fix-cmake-prefix-path.ps1 -Action build

# For Linux:
export CMAKE_PREFIX_PATH="/opt/vcpkg/installed/x64-linux"
```

## 🐳 Dev Container Details

### What's Included

- Ubuntu 22.04 base
- C++ build tools (GCC, Clang, Ninja)
- CMake 3.26+
- vcpkg pre-installed
- Code quality tools (clang-format, clang-tidy, cppcheck)
- Debugging tools (gdb, lldb)
- Pre-commit hooks

### Customization

Edit `.devcontainer/devcontainer.json`:

```json
{
  "remoteEnv": {
    "THEMIS_BUILD_TYPE": "Debug",  // Change build type
    "VCPKG_ROOT": "/opt/vcpkg"
  },
  "features": {
    // Add more dev container features
  }
}
```

### Volume Mounts

Persistent data:
- `themisdb-vcpkg-cache` - vcpkg packages cache

### Port Forwarding

- **8080** - ThemisDB Server
- **9090** - Prometheus Metrics

## 📚 Additional Resources

### Documentation

- [Copilot Instructions](.github/COPILOT_INSTRUCTIONS.md) - AI development guide
- [Contributing Guide](CONTRIBUTING.md) - Contribution guidelines
- [Architecture](ARCHITECTURE.md) - System design
- [Build Guides](docs/build-guide/) - Platform-specific builds

### Module Guides

- [Branching Strategy](.github/copilot/BRANCHING_GUIDE.md)
- [Build System](.github/copilot/BUILD_GUIDE.md)
- [Code Standards](.github/copilot/CODE_STANDARDS.md)
- [Testing](.github/copilot/TESTING_GUIDE.md)
- [Cross-Compilation](.github/copilot/CROSS_COMPILATION_CONTEXT.md)
- [VS Code Setup](.github/copilot/VSCODE_CONTEXT.md)

### External Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [vcpkg Documentation](https://vcpkg.io/)
- [Pre-commit Documentation](https://pre-commit.com/)
- [VS Code Remote Development](https://code.visualstudio.com/docs/remote/remote-overview)

## 🤝 Getting Help

- **Issues:** Open an issue with label `area:documentation`
- **Discussions:** Use GitHub Discussions
- **Questions:** Ask in team chat

---

**Last Updated:** 2026-02-12  
**Version:** 1.0
