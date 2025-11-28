# ThemisDB Build Toolchain

## Quick Start

### Windows
```powershell
# Debug build with MSVC
.\build-unified.ps1 -Platform windows -Config debug

# Release build with ClangCL
.\build-unified.ps1 -Platform windows -Config release -Compiler clangcl
```

### Linux/WSL
```powershell
# Build on WSL from Windows
.\build-unified.ps1 -Platform linux -Config release

# Or directly on Linux
./build.sh
```

### Docker
```powershell
# Standard Docker image (Ubuntu 24.04)
.\build-unified.ps1 -Platform docker -Tag themisdb:latest

# Quick build with pre-compiled binary
.\build-docker-simple.ps1
 
# QNAP (Ubuntu 20.04, GLIBC 2.31) statischer Build mit separatem Manifest
 .\build-qnap.ps1
```

### ARM64 / Raspberry Pi
```powershell
# Cross-compile for ARM64
.\build-unified.ps1 -Platform arm64 -Config release

# Raspberry Pi specific
.\build-unified.ps1 -Platform rpi -Config release
```

## Release Workflow

### 1. Prepare Release
```powershell
# Dry run to verify changes
.\scripts\release.ps1 -Version 0.2.0 -DryRun

# Update version, commit, and tag
.\scripts\release.ps1 -Version 0.2.0
```

### 2. Build Artifacts
```powershell
# Build all platforms
.\scripts\release.ps1 -Version 0.2.0 -Platforms windows,linux,docker

# Build specific platforms only
.\scripts\release.ps1 -Version 0.2.0 -Platforms docker
```

### 3. Deploy
```powershell
# Push to GitHub and Docker Hub
.\scripts\release.ps1 -Version 0.2.0 -PushGit -PushDocker
```

## File Structure

```
themis/
├── build-unified.ps1          # Main build script (all platforms)
├── build-docker-simple.ps1    # Quick Docker build (pre-built binary)
├── scripts/
│   └── release.ps1           # Release automation
├── Dockerfile                 # Full Docker build (Ubuntu 24.04)
├── Dockerfile.simple          # Minimal Docker (pre-built binary)
├── docker-compose.yml         # Standard deployment
├── docker-compose-arm.yml     # ARM64 deployment
├── CMakePresets.json          # Platform-specific CMake configs
└── BUILD_STRATEGY.md          # Detailed build documentation
```

## Deprecated Files (To Remove)

The following files are obsolete and should be removed:

- `Dockerfile.old` - Replaced by Dockerfile.simple
- `Dockerfile.runtime` - Redundant
- `Dockerfile.qnap` - Non-functional (vcpkg issues)
- `Dockerfile.qnap.simple` - Incomplete
- `build-docker-qnap.ps1` - Non-functional
- `build-docker-qnap-simple.ps1` - Incomplete
- `build-tests-msvc.ps1` - Use CMakePresets instead
- `build-msvc/` directory - Old build artifacts
- `build-wsl/` directory - Use CMakePresets instead

## Platform-Specific Notes

### Windows
- Requires Visual Studio 2022 or LLVM/Clang
- vcpkg automatically handled by CMake

### Linux/WSL
- Requires GCC 11+ or Clang 14+
- Ubuntu 24.04 LTS recommended (GLIBC 2.38+)

### Docker
- Standard: Ubuntu 24.04 (GLIBC 2.38+)
- QNAP: Ubuntu 20.04 (GLIBC 2.31) via `build-qnap.ps1` (statisch, separates Manifest `vcpkg.qnap.json`)

### ARM64
- Tested on Raspberry Pi 4/5
- Debian Bullseye or Ubuntu 22.04 recommended

## Common Tasks

### Clean Build
```powershell
.\build-unified.ps1 -Clean
```

### Build with Tests
```powershell
.\build-unified.ps1 -Platform windows -Tests
```

### Build with Benchmarks
```powershell
.\build-unified.ps1 -Platform linux -Benchmarks
```

### Static Build (for QNAP)
```powershell
.\build-unified.ps1 -Platform linux -Static
# Alternativ mit QNAP Manifest (Tests/Benchmarks/Tracing aktiv)
.\build-qnap.ps1
```

### Docker Build without Cache
```powershell
.\build-unified.ps1 -Platform docker -NoCache
```

## Troubleshooting

### vcpkg Download Failures
Use the simplified Docker build:
```powershell
.\build-docker-simple.ps1
```

### QNAP GLIBC Errors
Nutze QNAP Build-Script (statisch + eigenes Manifest):
```powershell
.\build-qnap.ps1
```

### WSL Path Issues
Ensure project is in `/mnt/c/VCC/themis` or adjust paths in scripts.

## Version Management

Version is centrally managed in `CMakeLists.txt`:
```cmake
project(ThemisDB VERSION 0.1.0)
```

Release script automatically updates:
- CMakeLists.txt
- CHANGELOG.md
- Git tags
- Docker tags

## Continuous Integration

GitHub Actions workflows:
- `.github/workflows/ci.yml` - Build and test
- `.github/workflows/build-multiarch.yml` - Multi-platform builds
- `.github/workflows/arm-build.yml` - ARM64 builds

Triggers:
- Push to `main` - Full CI
- Tags `v*.*.*` - Release builds

## Next Steps

See [BUILD_STRATEGY.md](BUILD_STRATEGY.md) for:
- Detailed platform matrix
- QNAP deployment solutions
	- Separates Manifest: `vcpkg.qnap.json` (inkl. tests/benchmarks/tracing, statischer Triplet)
- Packaging strategy (.deb, .rpm, etc.)
- Migration plan
