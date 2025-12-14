# ThemisDB Changelog (Docker Hub)

All notable changes to ThemisDB Docker images.

---

## [1.0.2] - 2025-12-14

🔧 **Patch Release - Build Stability & QNAP Support**

### Docker Images

**New Tags:**
- `themisdb/themisdb:v1.0.2` - Latest stable (amd64, arm64)
- `themisdb/themisdb:qnap` - QNAP NAS optimized (amd64 only, Ubuntu 20.04, SSE4.2)
- `themisdb/themisdb:v1.0.2-qnap` - QNAP v1.0.2 release

**Updated Tags:**
- `themisdb/themisdb:latest` - Now points to v1.0.2
- `themisdb/themisdb:v1.0` - Updated to v1.0.2
- `themisdb/themisdb:v1` - Updated to v1.0.2

### Fixed

**Windows Build Stability:**
- Resolved RocksDB linking issue on Windows MSVC builds
- Intelligent RocksDB target selection (static vs shared)
- Stable Windows Release builds now available

**Linux Build:**
- WSL build validated
- All artifacts and checksums updated

**QNAP Compatibility:**
- New QNAP-optimized image with Ubuntu 20.04 base (GLIBC 2.31)
- SSE4.2 CPU baseline for broader NAS device compatibility
- Default port 18765 to avoid QNAP service conflicts
- Requires QTS 5.0+ or QuTS hero h5.0+

### Platform Support

**Image Sizes:**
- Standard (amd64/arm64): ~150MB compressed
- QNAP (amd64): ~145MB compressed

**Platforms:**
- `linux/amd64` - Intel/AMD x64 processors
- `linux/arm64` - ARM v8 (Raspberry Pi, Apple Silicon, AWS Graviton)
- QNAP x86_64 NAS devices (optimized variant)

### Documentation

- Updated Docker Hub README with QNAP instructions
- Added platform-specific deployment guides
- Expanded troubleshooting section

---

## [1.0.1] - 2025-12-11

🐳 **Docker Hub Initial Release**

### Added

**Multi-Architecture Images:**
- `themisdb/themisdb:v1.0.1` - First production release
- `themisdb/themisdb:latest` - Latest stable
- Platforms: `linux/amd64` + `linux/arm64`

**Features:**
- Multi-model database (Key-Value, Document, Vector, Graph, Geospatial, Full-Text)
- ACID transactions
- GPU acceleration (CUDA, Vulkan, ROCm, OpenCL)
- Horizontal scaling & sharding
- Multi-master replication
- Client SDKs for 7 languages

**Build Configuration:**
- Multi-stage Docker build
- vcpkg package manager
- Ubuntu 22.04 base image
- ~150MB compressed image size
- Build time: ~3.8 hours (multi-arch)

**Deployment:**
- Health checks built-in
- Volume support for data persistence
- Environment variable configuration
- Docker Compose examples
- Kubernetes manifests

**Documentation:**
- Complete Docker deployment guide
- Quick start guide
- Configuration reference
- Platform-specific instructions (Linux, macOS, Windows, ARM)
- Troubleshooting guide

### Docker Hub Publishing

- **Registry:** `docker.io/themisdb/themisdb`
- **Build Method:** Docker Buildx with multi-platform support
- **CI/CD:** Automated builds via GitHub Actions

### Image Specs

**Base Image:** Ubuntu 22.04 LTS  
**Architecture:** Multi-arch (amd64, arm64)  
**Compression:** ~150MB compressed, ~400MB uncompressed  
**User:** Non-root by default  
**Healthcheck:** `/health` endpoint on port 18765

### Ports

- `8080` - REST API & Web UI
- `18765` - Binary protocol

### Volumes

- `/data` - Database storage
- `/etc/themis` - Configuration files
- `/var/log/themis` - Application logs

---

## [1.0.0] - 2025-11-30

🎉 **Production Release**

### Initial Features

**Core Database:**
- LSM Tree storage engine (RocksDB-based)
- Multi-model support (6 data models)
- MVCC concurrency control
- ACID compliance

**Performance:**
- SIMD optimizations (AVX2/NEON)
- GPU acceleration support
- Query optimizer
- Index rebuilding with progress tracking

**Enterprise:**
- Authentication & authorization
- Audit logging
- Encryption at rest
- OpenTelemetry instrumentation

**Analytics:**
- OLAP engine (CUBE, ROLLUP, Window Functions)
- Complex Event Processing (CEP)
- Process mining

**Distribution:**
- Sharding with auto-rebalancing
- Replication (leader-follower, multi-master)
- RAID-like redundancy (MIRROR, STRIPE, PARITY, GEO)
- Gossip protocol for cluster membership

**Client SDKs:**
- Python, JavaScript, Rust, Go, Java, C#, Swift
- Feature parity across all languages
- Auto-generated from OpenAPI spec

---

## Versioning

ThemisDB follows [Semantic Versioning](https://semver.org/):

- **MAJOR** (v1.x.x) - Incompatible API changes
- **MINOR** (v1.0.x) - Backwards-compatible new features
- **PATCH** (v1.0.2) - Backwards-compatible bug fixes

**Docker Tags:**
- `latest` - Always the latest stable release
- `v1` - Latest v1.x.x release
- `v1.0` - Latest v1.0.x release
- `v1.0.2` - Specific version
- `qnap` - Latest QNAP-optimized release
- `vX.Y.Z-qnap` - Specific QNAP version

---

## Upgrade Notes

### v1.0.1 → v1.0.2

**Breaking Changes:** None (patch release)

**Migration:** No database migration required

**Docker:**
```bash
# Pull new image
docker pull themisdb/themisdb:v1.0.2

# Stop and remove old container
docker stop themis && docker rm themis

# Start with new image (data persists in volume)
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:v1.0.2
```

**Rollback:**
```bash
# Revert to v1.0.1 if needed
docker pull themisdb/themisdb:v1.0.1
docker stop themis && docker rm themis
docker run -d --name themis -v themis_data:/data themisdb/themisdb:v1.0.1
```

### v1.0.0 → v1.0.1

**Breaking Changes:** None

**New Features:**
- Docker Hub publishing
- Multi-architecture images
- Improved health checks

---

## Upcoming

### v1.1.0 (Q1 2026) - Optimization Release

**Planned Features:**
- RocksDB TTL & incremental backups
- TBB parallel algorithms
- Arrow Parquet export
- CUDA as core dependency
- vLLM co-location support
- mimalloc integration (20-40% memory boost)

**Impact:** 3-10x performance improvements

### v1.2.0 (Q2 2026) - Enterprise Features

**Planned:**
- vLLM AI support (LoRA, FAISS advanced)
- PostGIS geo-spatial compatibility
- Timescale IoT features
- Enhanced query optimizer

---

## Support

**Issues:** https://github.com/makr-code/ThemisDB/issues  
**Discussions:** https://github.com/makr-code/ThemisDB/discussions  
**Documentation:** https://github.com/makr-code/ThemisDB

---

**Last Updated:** December 14, 2025
