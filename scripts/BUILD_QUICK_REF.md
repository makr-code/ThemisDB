# ThemisDB Build Quick Reference

Quick guide for building different ThemisDB editions.

---

## Editions

ThemisDB supports multiple editions with different feature sets:

| Edition | Script | Build Time | Binary Size | Features |
|---------|--------|------------|-------------|----------|
| **MINIMAL** | `build-minimal.sh` | ~5-10 min | ~30-50 MB | Core database only |
| **COMMUNITY** | `build.sh` | ~20-30 min | ~80-150 MB | Full features |
| **ENTERPRISE** | Manual CMake | ~30-40 min | ~150-250 MB | + Sharding, HA |
| **HYPERSCALER** | Manual CMake | ~40-60 min | ~200-300 MB | + Multi-DC |

---

## Quick Start

### MINIMAL Edition (Fastest)
```bash
# Core database only, no LLM, no GPU, smallest binary
./scripts/build-minimal.sh

# Run
./build-minimal/themis_server --config config/config-minimal.yaml
```

### COMMUNITY Edition (Default)
```bash
# Full features, optional LLM/GPU
./scripts/build.sh

# Run
./build/themis_server --config config/config.yaml
```

### ENTERPRISE Edition
```bash
# Contact sales@themisdb.com for license
cmake -S . -B build-enterprise \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-enterprise --parallel
```

---

## Docker Builds

### MINIMAL Edition
```bash
docker build -f Dockerfile.minimal -t themisdb:minimal .
docker run -d -p 8080:8080 -v themis_data:/data themisdb:minimal
```

### COMMUNITY Edition
```bash
docker build -f Dockerfile.themis-server -t themisdb:community .
docker run -d -p 8080:8080 -v themis_data:/data themisdb:community
```

---

## Build Options

### Custom CMake Configuration
```bash
# MINIMAL with tests enabled
cmake -S . -B build \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_BUILD_TESTS=ON

# COMMUNITY with GPU disabled
cmake -S . -B build \
  -DTHEMIS_EDITION=COMMUNITY \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_ENABLE_LLM=OFF

# ENTERPRISE with all features
cmake -S . -B build \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_SHARDING=ON
```

---

## Troubleshooting

### Build fails with "vcpkg not found"
```bash
# Install vcpkg
git clone https://github.com/microsoft/vcpkg.git $HOME/vcpkg
cd $HOME/vcpkg && ./bootstrap-vcpkg.sh

# Set environment variable
export VCPKG_ROOT=$HOME/vcpkg
```

### Out of memory during build
```bash
# Reduce parallel jobs
NUM_JOBS=2 ./scripts/build-minimal.sh
```

### Wrong edition running
```bash
# Check edition
./build/themis_server --version

# Expected output:
# ThemisDB v1.3.5 (MINIMAL Edition)
```

---

## Feature Comparison

See [docs/EDITION_COMPARISON.md](../docs/EDITION_COMPARISON.md) for detailed comparison.

Quick reference:

- **MINIMAL:** Core DB, no LLM, no GPU, no sharding (embedded/IoT)
- **COMMUNITY:** Full features, optional LLM/GPU (development/SMB)
- **ENTERPRISE:** + Sharding, HA, RBAC, 24/7 support (large deployments)
- **HYPERSCALER:** + Multi-DC, unlimited scaling (cloud providers)

---

## More Information

- [MINIMAL Edition Guide](../docs/MINIMAL_EDITION.md)
- [Full Build Guide](../docs/guides/guides_build_strategy.md)
- [Edition Comparison](../docs/EDITION_COMPARISON.md)
- [Configuration Reference](../config/config.yaml)
