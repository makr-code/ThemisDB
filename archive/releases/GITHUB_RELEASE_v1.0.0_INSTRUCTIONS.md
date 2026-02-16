# Creating GitHub Release v1.0.0

This document provides step-by-step instructions for creating the GitHub Release for ThemisDB v1.0.0.

## Prerequisites

✅ Git tag `v1.0.0` has been created at commit `60e901590e4b2e5990877b3c0f49cdcd2bb1f992`
✅ Archive script `scripts/archive-version.sh` is available
✅ Release notes exist at `docs/de/archive/RELEASE_NOTES_v1.0.0.md`
✅ Source archive has been generated: `themisdb-1.0.0-source.zip` (61MB)
✅ SHA256 checksum: `8bfd035a1418df98d2dcfbabecd5ad01ae6fd4a8c01c2eeffc486c15aa436280`

## Option 1: Automated via GitHub Actions (Recommended)

1. Go to the GitHub repository: https://github.com/makr-code/ThemisDB
2. Navigate to **Actions** → **Create Release Archive**
3. Click **Run workflow** button
4. Enter version: `1.0.0`
5. Click **Run workflow**

The workflow will automatically:
- Find the v1.0.0 tag
- Create the source archive
- Generate checksums
- Create the GitHub Release with all assets

## Option 2: Manual Release Creation

### Step 1: Push the Git Tag

```bash
cd /home/runner/work/ThemisDB/ThemisDB
git push origin v1.0.0
```

### Step 2: Create the Source Archive

The archive has already been created in the current directory:
- File: `themisdb-1.0.0-source.zip` (61MB)
- Checksum: `themisdb-1.0.0-source.zip.sha256`

To recreate it:
```bash
./scripts/archive-version.sh 1.0.0 v1.0.0
```

### Step 3: Create GitHub Release via Web UI

1. Go to: https://github.com/makr-code/ThemisDB/releases/new
2. **Choose a tag**: Select `v1.0.0` from dropdown
3. **Release title**: `ThemisDB v1.0.0`
4. **Description**: Copy from template below
5. **Attach files**: Upload `themisdb-1.0.0-source.zip`
6. Click **Publish release**

### Release Description Template

```markdown
# ThemisDB v1.0.0

**Release Date:** December 2, 2025

ThemisDB v1.0.0 is the first official stable release of ThemisDB - a high-performance, multi-model database combining document, graph, vector, and spatial capabilities with enterprise-grade security and compliance features.

## Source Code Archive

Download the source code archive and verify its integrity:

**File:** [themisdb-1.0.0-source.zip](https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-source.zip)

**SHA256 Checksum:**
```
8bfd035a1418df98d2dcfbabecd5ad01ae6fd4a8c01c2eeffc486c15aa436280  themisdb-1.0.0-source.zip
```

**Verification:**
```bash
# Download and verify
wget https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-source.zip
echo "8bfd035a1418df98d2dcfbabecd5ad01ae6fd4a8c01c2eeffc486c15aa436280  themisdb-1.0.0-source.zip" | sha256sum -c
```

## What's Included

The source archive contains:
- Complete ThemisDB source code
- Build system (CMake, vcpkg configuration)
- Documentation and examples
- Tests and benchmarks

**Excluded** (to keep size manageable):
- `external/` - Built via vcpkg during compilation
- `vcpkg/` - Downloaded during build
- `llama.cpp/` - External submodule

## Building from Source

```bash
# Extract archive
unzip themisdb-1.0.0-source.zip
cd themisdb-1.0.0

# Build with CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Or use build script
./scripts/build.sh
```

## Release Highlights

### Core Features
- Multi-Model Database Engine (documents, graphs, vectors, spatial data)
- MVCC Transaction Support with ACID compliance
- Vector Search with HNSW-based approximate nearest neighbor
- Spatial Indexing with R-tree and PostGIS-compatible functions
- Graph Traversal with bidirectional path finding
- ContentFS for binary large object storage
- AQL (Advanced Query Language)

### Security & Compliance
- Field-Level Encryption (AES-256-GCM)
- PII Detection and automated compliance
- Comprehensive Audit Logging
- Rate Limiting and Policy Engine

### Enterprise Features
- Backup & Restore with point-in-time recovery
- Prometheus-compatible metrics
- Kubernetes-ready health checks
- TLS Support for client connections

## Installation

### Docker (Recommended)

```bash
docker pull themisdb/themisdb:1.0.0
docker run -d --name themisdb -p 8765:8765 -v themis-data:/data themisdb/themisdb:1.0.0
```

### Binary Packages

Pre-built binaries are available on Docker Hub. See the [release notes](https://github.com/makr-code/ThemisDB/blob/main/docs/de/archive/RELEASE_NOTES_v1.0.0.md) for download links and installation instructions.

## Documentation

- **Complete Release Notes**: [RELEASE_NOTES_v1.0.0.md](https://github.com/makr-code/ThemisDB/blob/main/docs/de/archive/RELEASE_NOTES_v1.0.0.md)
- **API Reference**: [docs/API.md](https://github.com/makr-code/ThemisDB/blob/main/docs/API.md)
- **Getting Started**: [README.md](https://github.com/makr-code/ThemisDB/blob/main/README.md)
- **Docker Deployment**: [docs/guides/guides_deployment.md](https://github.com/makr-code/ThemisDB/blob/main/docs/guides/guides_deployment.md)

## Known Issues

- ARM64/Raspberry Pi support is in development
- Vector index auto-save is disabled by default
- Some EPSG coordinate system codes may require manual initialization

## Support

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions

---

**Commit:** `60e901590e4b2e5990877b3c0f49cdcd2bb1f992`
**Full Changelog**: https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md
```

## Verification After Release

After creating the release, verify:

1. **Tag is visible**: https://github.com/makr-code/ThemisDB/tags
2. **Release is published**: https://github.com/makr-code/ThemisDB/releases
3. **Archive is downloadable**: Click on the ZIP file in the release
4. **Checksum matches**:
   ```bash
   wget https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-source.zip
   sha256sum themisdb-1.0.0-source.zip
   # Should output: 8bfd035a1418df98d2dcfbabecd5ad01ae6fd4a8c01c2eeffc486c15aa436280
   ```

## Troubleshooting

### Tag Not Found
If the tag is not showing up:
```bash
git push origin v1.0.0
```

### Archive Upload Failed
- Ensure file size is under GitHub's 2GB limit (our file is 61MB, so no issue)
- Try uploading via GitHub CLI:
  ```bash
  gh release upload v1.0.0 themisdb-1.0.0-source.zip
  ```

### Workflow Failed
Check the Actions tab for error logs. Common issues:
- Missing permissions (workflow needs `contents: write`)
- Invalid version format
- Tag doesn't exist yet

## Next Steps

After v1.0.0 release is created:
1. Announce the release (GitHub Discussions, social media, etc.)
2. Update README.md with release badge/link if needed
3. Monitor for community feedback
4. Prepare for v1.0.1 or v1.1.0 based on feedback
