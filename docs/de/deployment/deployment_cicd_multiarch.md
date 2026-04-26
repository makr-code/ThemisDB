# CI/CD for ARM and Multi-Architecture Builds

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment

---

## 📑 Table of Contents

- [Overview](#this-document-describes-the-continuous-integration-and-deployment-setup-for-themisdbs-multi-architecture-builds-including-arm-support)
- [GitHub Actions Workflows](#github-actions-workflows)
- [Build Artifacts](#build-artifacts-v130)
- [Docker Images](#docker-images-v130)


This document describes the continuous integration and deployment setup for ThemisDB's multi-architecture builds, including ARM support.

## GitHub Actions Workflows

ThemisDB uses GitHub Actions for automated building and testing across multiple architectures.

### 1. ARM Build Test Workflow (`arm-build.yml`)

**Purpose:** Validates ARM compilation on every code change.

**Triggers:**
- Push to `main` or `develop` branches (when ARM-related files change)
- Pull requests to `main` or `develop`
- Manual workflow dispatch

**Jobs:**

#### a) ARM64 Docker Build Test
- Uses QEMU emulation to build Docker image for `linux/arm64`
- Validates the Dockerfile multi-arch support
- Tests basic image functionality

#### b) ARMv7 Docker Build Test
- Uses QEMU emulation to build Docker image for `linux/arm/v7`
- Validates Raspberry Pi 2/3 compatibility
- Tests basic image functionality

#### c) ARM64 Cross-Compilation Test
- Installs ARM64 cross-compiler toolchain
- Configures CMake with `aarch64-linux-gnu-gcc`
- Verifies architecture detection
- Checks for NEON optimization flags

#### d) Test Script Validation
- Runs `scripts/test-arm-support.sh`
- Validates architecture detection logic
- Ensures build presets are correct

**Results:**
- Summary table in GitHub Actions UI
- Fails if any ARM build test fails

### 2. Multi-Architecture Build Workflow (`build-multiarch.yml`)

**Purpose:** Comprehensive build matrix for all supported platforms.

**Triggers:**
- Push to `main`, `develop`, or `copilot/**` branches
- Pull requests to `main` or `develop`
- Manual workflow dispatch with optional ARM build toggle

**Build Matrix (v1.3.0):**

| OS | Architecture | Config | Preset | LLM | RPC | GPU/CUDA |
|----|--------------|--------|--------|-----|-----|----------|
| Ubuntu 22.04 | x64 | Release Minimal | linux-ninja-clang-release | OFF | OFF | OFF |
| Ubuntu 22.04 | x64 | Release LLM | linux-llm-release | ON | OFF | OFF |
| Ubuntu 22.04 | x64 | Release LLM+GPU | linux-llm-gpu-release | ON | OFF | ON |
| Ubuntu 22.04 | x64 | Release Full | linux-full-release | ON | ON | ON |
| Ubuntu 22.04 | x64 | Debug | linux-ninja-clang-debug | OFF | OFF | OFF |
| Ubuntu 22.04 | arm64 | Release | linux-arm64-gcc-release | OFF | OFF | OFF |
| Ubuntu 22.04 | arm64 | Release LLM | linux-arm64-llm-release | ON | OFF | OFF |

**Modular Build Flags (v1.3.0):**
- `THEMIS_ENABLE_LLM=ON/OFF` - Enable llama.cpp LLM integration (+96 files)
- `THEMIS_BUILD_RPC_FRAMEWORK=ON/OFF` - Enable gRPC RPC framework (+26 files)
- `THEMIS_ENABLE_CUDA=ON/OFF` - Enable NVIDIA CUDA GPU acceleration
- `THEMIS_ENABLE_GPU=ON/OFF` - Enable GPU vector search (FAISS)

**Features:**
- vcpkg caching for faster builds
- Artifact upload for built binaries
- Docker multi-arch builds with QEMU
- Automatic push to GitHub Container Registry (on main branch)
- Modular feature matrix for flexible deployments

**Docker Platforms:**
- `linux/amd64` (x86_64)
- `linux/arm64` (ARM64/AArch64)
- `linux/arm/v7` (ARMv7)

## Build Artifacts (v1.3.0)

Successful builds produce the following artifacts:

**Core Builds:**
- `themisdb-ubuntu-22.04-x64-Release` - Linux x86_64 binaries (minimal)
- `themisdb-ubuntu-22.04-x64-Debug` - Linux x86_64 debug binaries
- `themisdb-ubuntu-22.04-arm64-Release` - Linux ARM64 binaries (minimal)

**Feature Builds (v1.3.0):**
- `themisdb-ubuntu-22.04-x64-llm-Release` - With LLM integration
- `themisdb-ubuntu-22.04-x64-llm-gpu-Release` - With LLM + CUDA
- `themisdb-ubuntu-22.04-x64-full-Release` - With LLM + RPC + GPU
- `themisdb-ubuntu-22.04-arm64-llm-Release` - ARM64 with LLM

Artifacts are retained for 7 days.

## Docker Images (v1.3.0)

Multi-architecture Docker images are automatically built and optionally pushed to:

> **Stand 2026-04 (aktueller Release-Pfad):**
> Community-Container werden ueber den aktiven Workflow
> `.github/workflows/04-release_publish-community.yml` nach Docker Hub unter
> `themisdb/themisdb` veroeffentlicht (`latest` + Versions-Tag).
> Die unten aufgefuehrten branch-basierten GHCR-Tags sind historisch zu verstehen.

**GitHub Container Registry (historisch/Legacy):**
- `ghcr.io/makr-code/themisdb:main` - Latest main branch build (minimal)
- `ghcr.io/makr-code/themisdb:main-llm` - Latest with LLM
- `ghcr.io/makr-code/themisdb:main-llm-gpu` - Latest with LLM + GPU
- `ghcr.io/makr-code/themisdb:main-full` - Latest with all features
- `ghcr.io/makr-code/themisdb:develop` - Latest develop branch build
- `ghcr.io/makr-code/themisdb:<branch>-<sha>` - Branch builds with commit SHA

**Docker Hub (Public, aktiv):**
- `themisdb/themisdb:latest` - Aktuelles Community-Release
- `themisdb/themisdb:<version>` - Versionierter Release-Tag (z. B. `1.8.1-rc1`)

**Supported Platforms per Image:**
- `linux/amd64`
- `linux/arm64`
- `linux/arm/v7`

Docker automatically selects the correct image for your platform when pulling.

## Local Development

### Testing ARM Builds Locally

**Using Docker Buildx:**

```bash
# Enable BuildKit
export DOCKER_BUILDKIT=1

# Build for ARM64
docker buildx build \
    --platform linux/arm64 \
    -t themisdb:arm64-local \
    -f Dockerfile .

# Build for ARMv7
docker buildx build \
    --platform linux/arm/v7 \
    -t themisdb:armv7-local \
    -f Dockerfile .
```

**Using Cross-Compilation:**

```bash
# Install ARM64 cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Bootstrap vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg && ./bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

# Configure for ARM64
cmake -S . -B build-arm64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64

# Build
cmake --build build-arm64 -j$(nproc)
```

### Running Test Script Locally

```bash
# Ensure vcpkg is set up
export VCPKG_ROOT=~/vcpkg

# Run validation
./scripts/test-arm-support.sh
```

## Caching Strategy

### vcpkg Binary Caching

Uses GitHub Actions cache to store compiled vcpkg packages:

```yaml
env:
  VCPKG_BINARY_SOURCES: 'clear;x-gha,readwrite'
```

This significantly speeds up subsequent builds (from 30+ minutes to 5-10 minutes).

### Docker Layer Caching

Uses GitHub Actions cache for Docker layers:

```yaml
cache-from: type=gha
cache-to: type=gha,mode=max
```

Reduces Docker build time by reusing unchanged layers.

## Performance Considerations

### Build Times (Approximate)

| Platform | Configuration | First Build | Cached Build |
|----------|--------------|-------------|--------------|
| x86_64 | Release | 25-30 min | 5-8 min |
| x86_64 | Debug | 20-25 min | 4-6 min |
| ARM64 (cross) | Release | 35-45 min | 8-12 min |
| ARM64 (QEMU) | Release | 60-90 min | 15-25 min |
| ARMv7 (QEMU) | Release | 70-100 min | 20-30 min |

QEMU emulation is slower but allows testing ARM builds without actual ARM hardware.

### Optimization Tips

1. **Use cross-compilation for faster ARM builds** (35-45 min vs 60-90 min)
2. **Enable caching** to reduce subsequent build times by 70-80%
3. **Run ARM builds only when necessary** (use path filters)
4. **Use manual workflow dispatch** for experimental ARM builds

## Continuous Deployment

### Automatic Deployment (Main Branch)

When code is pushed to `main`:

1. Multi-arch Docker images are built
2. Images are tagged with `main`, `<commit-sha>`, and version tags
3. Images are pushed to GitHub Container Registry
4. All platforms (amd64, arm64, arm/v7) are included

### Pull Requests

On pull requests:

1. ARM builds are tested via Docker and cross-compilation
2. Images are built but not pushed
3. Test results are shown in PR status checks

## Troubleshooting

### QEMU Build Timeouts

If QEMU builds timeout:
- Reduce build parallelism: `-j2` instead of `-j$(nproc)`
- Disable tests: `-DTHEMIS_BUILD_TESTS=OFF`
- Use workflow dispatch with manual ARM build toggle

### Cross-Compilation Failures

Common issues:
- Missing cross-compiler: Install `gcc-aarch64-linux-gnu`
- vcpkg triplet mismatch: Ensure `arm64-linux` triplet is used
- System libraries: Some dependencies may need ARM64 system libraries

### vcpkg Cache Issues

If builds fail with vcpkg errors:
- Clear cache manually in GitHub Actions settings
- Rebuild vcpkg baseline: Update `vcpkg.json` baseline commit

## Monitoring

### Build Status Badges

The README displays build status for all architectures:

```markdown
[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml)
```

> **Note:** Multi-arch build status (ARM, x86_64) is tracked via the `01-core/themis-core-ci.yml` workflow. Dedicated `arm-build.yml` and `build-multiarch.yml` workflows are not currently active.

### GitHub Actions Dashboard

View all workflow runs: https://github.com/makr-code/ThemisDB/actions

Filter by:
- Workflow: `arm-build.yml` or `build-multiarch.yml`
- Branch: `main`, `develop`, etc.
- Event: `push`, `pull_request`, `workflow_dispatch`

## Future Enhancements

Planned CI/CD improvements:

1. **Hardware Testing**: Run tests on actual ARM hardware (Raspberry Pi, AWS Graviton)
2. **Performance Benchmarks**: Automated ARM vs x86_64 performance comparisons
3. **Release Automation**: Automatic binary packaging and GitHub releases
4. **Matrix Expansion**: Add macOS ARM64 (Apple Silicon) builds
5. **Test Coverage**: ARM-specific test coverage reports

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Docker Buildx Multi-Platform](https://docs.docker.com/build/building/multi-platform/)
- [QEMU User Emulation](https://www.qemu.org/docs/master/user/main.html)
- [vcpkg Binary Caching](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching)

## Support

For CI/CD issues:
1. Check workflow logs in GitHub Actions
2. Review this documentation
3. See `docs/ARM_RASPBERRY_PI_BUILD.md` for build requirements
4. Open an issue with workflow run URL and error logs
