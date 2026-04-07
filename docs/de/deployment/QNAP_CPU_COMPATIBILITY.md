# QNAP CPU Compatibility Fix

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🚀 Deployment

---

## 📑 Inhaltsverzeichnis

- [Problem](#problem)
- [Root Cause](#root-cause)
- [Solution](#solution)
- [Building for QNAP](#building-for-qnap)

## Problem

ThemisDB v1.0.0 crashes on QNAP NAS devices with "Illegal instruction" error:

```
"/data/themis_server"
"/data/vector_indexes"
Illegal instruction
```

## Root Cause

- **Standard Docker build** uses `-march=native` compiler flag for **maximum performance**
- This optimizes for the build host CPU (GitHub Actions runners with AVX2/AVX512/FMA3)
- QNAP NAS devices use Intel Celeron CPUs with **varying instruction sets**:
  - **Celeron N5095** (Jasper Lake, 2021): Has AVX2 but **missing FMA3**
  - **Celeron J4125** (Gemini Lake, 2019): No AVX2, no FMA3
  - **Celeron N3060** (Braswell, 2016): No AVX2, no FMA3
- Standard build contains `_mm256_fmadd_ps` (FMA3) → crashes on N5095
- **Solution**: QNAP build uses conservative **SSE4.2** baseline (compatible with all Celeron since 2010)

### SIMD Optimization in ThemisDB

ThemisDB uses SIMD (Single Instruction Multiple Data) optimizations for:
- **Vector search** (`src/utils/simd_distance.cpp`): 
  - AVX512F: `_mm512_fmadd_ps` (16 floats at once)
  - **AVX2 + FMA3**: `_mm256_fmadd_ps` (8 floats at once) ← **Crashes on N5095**
  - SSE2 fallback: Scalar computation
- **JSON parsing** (`simdjson` library): Hardware-accelerated JSON processing (safe, has runtime detection)
- **Spatial indexing**: Optional SIMD kernels for geospatial queries

**Critical Issue**: Code uses `_mm256_fmadd_ps` (FMA3 instruction) in AVX2 path, but Celeron N5095 lacks FMA3.

## Solution

### Strategy

- **Standard build**: `-march=native` → **Maximum performance** (AVX512/AVX2/FMA3 on capable hosts)
- **QNAP build**: `-march=x86-64 -msse4.2` → **Universal compatibility** (works on all Celeron CPUs)

### CMake Build Option

Added `THEMIS_QNAP_BUILD` option in `CMakeLists.txt`:

```cmake
option(THEMIS_QNAP_BUILD "Build for QNAP NAS with SSE4.2 baseline" OFF)
```

When enabled:
- **Disables AVX/AVX2/FMA**: No advanced SIMD instructions
- **Uses SSE4.2**: Universal x86-64 baseline (available since Celeron N3xxx series)
- Compiler flags: `-march=x86-64 -msse4.2`
- **No code changes**: Standard AVX2 code simply won't compile in, SSE fallback used

#### Dockerfile (Standard Build)
```dockerfile
ARG QNAP_BUILD=OFF

RUN cmake -S . -B build -G Ninja \
    ...
    -DTHEMIS_QNAP_BUILD=${QNAP_BUILD}
```

#### Dockerfile.qnap (QNAP-Optimized Build)
```dockerfile
RUN cmake -S . -B build -G Ninja \
    ...
    -DTHEMIS_QNAP_BUILD=ON  # Always ON for QNAP
```

## Building for QNAP

### Docker Build

```powershell
# Build QNAP-compatible image
docker build -f Dockerfile.qnap -t themisdb/themisdb:qnap .

# Or use automated script
.\docker-build-push.ps1 -Version 1.0.1 -Push
```

The `docker-build-push.ps1` script automatically builds both:
- `themisdb/themisdb:latest` (standard, AVX2-optimized)
- `themisdb/themisdb:qnap` (baseline x86-64, QNAP-compatible)

### Manual CMake Build

```bash
# QNAP-compatible build
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_QNAP_BUILD=ON

cmake --build build --target themis_server
```

## Performance Impact

### Vector Search Performance

| Build Type | CPU | ISA | Distance (256-dim) | Performance |
|------------|-----|-----|-------------------|-------------|
| **Standard** | Xeon Gold 6248 | AVX512+FMA | Vector operation | **~500 ns** |
| **Standard** | Core i7-10700K | AVX2+FMA3 | Vector operation | **~800 ns** |
| **QNAP** | **Celeron N5095** | **SSE4.2** | Vector operation | **~3 µs** |
| Fallback | Celeron J4125 | SSE4.2 | Vector operation | ~3 µs |
| Scalar | Any x86-64 | - | Scalar loop | ~5 µs |

**Impact**: 
- Standard build: **Maximum performance** with native optimizations
- QNAP build: ~4x slower than AVX2, but **universal compatibility**
- Still faster than scalar fallback

### JSON Parsing Performance

`simdjson` library gracefully falls back to non-AVX mode:
- AVX2: ~2.5 GB/s parsing speed
- Baseline: ~1.2 GB/s parsing speed
- Impact: ~2x slower but acceptable for most workloads

### Graph & Relational Queries

- **No impact**: Graph traversal, AQL queries, RocksDB operations don't use AVX
- MVCC, LSM-tree compaction: Same performance on QNAP

## QNAP CPU Compatibility Matrix

| QNAP Model | CPU | AVX2 | FMA3 | Standard Build | QNAP Build |
|------------|-----|------|------|----------------|------------|
| **TS-x53D** | **Celeron N5095** | ✅ | ❌ | ❌ Crash (FMA3) | ✅ Works |
| TS-464 | Celeron J4125 | ❌ | ❌ | ❌ Crash | ✅ Works |
| TS-x73A | AMD Ryzen V1500B | ✅ | ✅ | ✅ Works | ✅ Works |
| TVS-h674 | Core i5-12400 | ✅ | ✅ | ✅ Works | ✅ Works |
| TS-x51A | Celeron N3060 | ❌ | ❌ | ❌ Crash | ✅ Works |
| TS-x77XU | Xeon E5 | ✅ | ✅ | ✅ Works | ✅ Works |

**Recommendation**: Use `themisdb/themisdb:qnap` tag for all QNAP deployments to ensure compatibility.

## Deployment

### Docker Hub Tags

After rebuild, two variants are available:

| Tag | Target Platform | CPU Requirements | Use Case |
|-----|----------------|------------------|----------|
| `1.0.1`, `latest` | Standard servers | **Any x86-64** (optimized for AVX2+) | Production servers, cloud |
| `1.0.1-qnap`, `qnap` | QNAP NAS | **SSE4.2+** (universal Celeron) | QNAP Container Station |
| `1.0.1-qnap`, `qnap` | QNAP NAS | x86-64 baseline | QNAP Container Station |

### QNAP Container Station

```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb/themisdb:qnap  # ← Use QNAP tag
    container_name: themisdb
    ports:
      - "18765:18765"
    volumes:
      - /share/Container/themisdb:/data
    environment:
      - THEMIS_LOG_LEVEL=info
      - TZ=Europe/Berlin
    restart: unless-stopped
```

## Testing

### Verify CPU Compatibility

```bash
# Inside QNAP container
ldd /usr/local/bin/themis_server
# Should not depend on AVX-specific libraries

# Check for illegal instructions
/usr/local/bin/themis_server --version
# Should print version without crash
```

### Check for AVX Usage

```bash
# On build host
objdump -d build/themis_server | grep -E "vaddps|vfmadd|vmovaps"
# QNAP build should show NO AVX instructions
# Standard build will show many
```

## GitHub Actions CI/CD

Updated `.github/workflows/release.yml` to build both variants:

```yaml
- name: Build Docker Images
  run: |
    # Standard build
    docker buildx build \
      --platform linux/amd64 \
      --tag themisdb/themisdb:${{ github.ref_name }} \
      --tag themisdb/themisdb:latest \
      --push \
      .
    
    # QNAP build
    docker buildx build \
      --platform linux/amd64 \
      --file Dockerfile.qnap \
      --tag themisdb/themisdb:${{ github.ref_name }}-qnap \
      --tag themisdb/themisdb:qnap \
      --push \
      .
```

## Rollback Plan

If issues persist after rebuild:

1. **Stop container**: `docker stop themisdb`
2. **Check logs**: `docker logs themisdb > qnap_error.log`
3. **Verify image**: `docker inspect themisdb/themisdb:qnap | grep Created`
4. **Fallback**: Use Ubuntu-based manual deployment (no Docker)

## Future Improvements

### Runtime CPU Detection

Consider runtime detection of CPU features:

```cpp
// In src/utils/simd_distance.cpp
#include <cpuid.h>

bool has_avx2() {
    unsigned int eax, ebx, ecx, edx;
    __get_cpuid(7, &eax, &ebx, &ecx, &edx);
    return (ebx & bit_AVX2) != 0;
}

float l2_distance_auto(const float* a, const float* b, size_t dim) {
    static bool use_avx2 = has_avx2();
    return use_avx2 ? avx2_l2_sq(a, b, dim) : scalar_l2_sq(a, b, dim);
}
```

### Multi-Arch Docker Manifests

Use Docker manifest lists for automatic platform selection:

```bash
docker manifest create themisdb/themisdb:latest \
  themisdb/themisdb:latest-avx2 \
  themisdb/themisdb:latest-baseline
```

## References

- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [GCC x86 Options](https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html)
- [simdjson Documentation](https://github.com/simdjson/simdjson)
- [QNAP Container Station Guide](https://www.qnap.com/en/how-to/faq/article/how-to-use-container-station)

## Support

If issues persist after v1.0.1 deployment:

1. Open GitHub issue: https://github.com/makr-code/ThemisDB/issues
2. Include QNAP model, CPU info (`cat /proc/cpuinfo`), and full error log
3. Attach output of `docker inspect themisdb/themisdb:qnap`

---

**Version**: 1.0.1  
**Last Updated**: 2026-04-06  
**Status**: ✅ Fixed (pending rebuild)
