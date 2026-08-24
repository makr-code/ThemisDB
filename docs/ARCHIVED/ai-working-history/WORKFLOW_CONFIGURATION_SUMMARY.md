# CI/CD Workflow Configuration - Production Ready

**Date:** 2026-08-08  
**Status:** ✅ Complete and Production Ready  
**Workflows Modified:**
- `.github/workflows/packaging-release.yml`
- `.github/workflows/docker-image.yml`

---

## Summary

Configured ThemisDB CI/CD workflows to automatically build, package, and deploy releases with comprehensive testing across all production presets.

---

## 1. packaging-release.yml - Automated Release Build Pipeline

### Trigger Events

| Event | Condition | Action |
|-------|-----------|--------|
| **Tag Push** | `v[0-9]+.[0-9]+.[0-9]+*` pattern | Auto-build & package |
| **GitHub Release** | `published` or `created` | Auto-build & package |
| **Manual Dispatch** | Workflow button click | Optional matrix selection |

### Build Matrix Job

Tests all three production presets across appropriate platforms:

```yaml
Matrix Configuration:
├── community-release (Linux)
│   └── Uses system packages (librocksdb-dev, etc.)
├── linux-release (Linux)
│   └── Uses vcpkg toolchain (production Linux build)
└── windows-release (Windows)
    └── Uses vcpkg toolchain (production Windows build)
```

**Features:**
- Parallel builds across all presets
- Automatic dependency installation per preset
- vcpkg bootstrapping for linux-release and windows-release
- Test execution with 120-second timeout per test
- Build log artifact upload on failure
- `fail-fast: false` ensures all matrix combinations are tested

### Package Job

Triggered only on actual releases (tags/releases):

**Outputs:**
- ✅ ZIP archive
- ✅ DEB package
- ✅ RPM package  
- ✅ TGZ tarball
- ✅ SHA256 checksums for each artifact

**Retention:** 30 days

### Workflow Dispatch Options

Manual trigger provides preset selection:
- `all` - Run all three presets (default)
- `community-only` - Test community-release only
- `linux-release-only` - Test linux-release only
- `windows-release-only` - Test windows-release only

---

## 2. docker-image.yml - Docker Build & Push Pipeline

### Trigger Events

| Event | Condition | Action |
|-------|-----------|--------|
| **Push** | Branch: `develop` | Build (no push) + cache update |
| **Tag Push** | `v[0-9]+.[0-9]+.[0-9]+*` pattern | Build + push with version tags |
| **GitHub Release** | `published` or `created` | Build + push with version tags |
| **Pull Request** | Target: `develop` | Build (no push) + cache update |
| **Manual Dispatch** | Workflow button click | Optional push to registry |

### Version Tagging Strategy

**For Releases (Tags/Release events):**
```
Version v1.2.3 generates tags:
├── themisdb/themisdb:1.2.3         (Full version)
├── themisdb/themisdb:1.2           (Minor version)
└── themisdb/themisdb:latest        (Only for stable releases, not pre-release)
```

**For Develop Branch:**
```
Latest commit generates tag:
└── themisdb/themisdb:dev-{SHORT_SHA}
```

### Multi-Platform Support

**Platforms:**
- `linux/amd64` - Intel/AMD 64-bit
- `linux/arm64` - ARM 64-bit (Apple Silicon, ARM servers)

### Authentication

Automatic login to Docker registries:
- **Docker Hub:** Uses `DOCKERHUB_USERNAME` and `DOCKERHUB_TOKEN` secrets
- **GitHub Container Registry (ghcr.io):** Uses GitHub token (automatic)

### Push Conditions

Images are pushed when:
1. ✅ GitHub Release is published/created
2. ✅ Push to `develop` branch
3. ✅ Manual workflow_dispatch with `push_to_registry: true`

Pull requests do **not** push (only build and cache).

### Build Cache

Uses GitHub Actions cache layer:
- `type=gha` - Read from GHA cache
- `mode=max` - Aggressive caching for faster subsequent builds

---

## 3. Production Best Practices Implemented

### Concurrency Control
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: false
```
- Prevents duplicate runs for same ref
- Maintains ordering for release builds

### Permissions Model
```yaml
permissions:
  contents: read          # Checkout code only
  packages: write         # Push to registries
  actions: read           # Read workflow state
```
- Minimal required permissions (principle of least privilege)
- Explicit read-only for code operations

### Error Handling
- ✅ `continue-on-error: true` on optional test jobs
- ✅ `|| true` conditionals for best-effort operations (RPM on non-RPM systems)
- ✅ Build log artifact upload on failure for debugging
- ✅ Graceful fallback when optional tools unavailable

### Artifact Retention
```yaml
retention-days: 30
```
- Packages retained for 30 days
- Build logs retained for 7 days (debugging)
- Automatic cleanup to manage storage

### GitHub Step Summaries

Each job provides human-readable summaries:

**Release Packages:**
```
### 📦 Release Packages Built

**Version:** v1.2.3

**Artifacts:**
- themisdb-1.2.3-Linux.zip (45MB)
- themisdb_1.2.3_amd64.deb (32MB)
- themisdb-1.2.3-Linux.tar.gz (28MB)
```

**Docker Images:**
```
### 🐳 Docker Image Published

**Version:** 1.2.3

**Tags:** themisdb/themisdb:1.2.3, themisdb/themisdb:1.2, themisdb/themisdb:latest

**Pull commands:**
docker pull themisdb/themisdb:1.2.3
docker pull themisdb/themisdb:latest
```

---

## 4. Workflow Execution Flow

### Release Flow (Tag or GitHub Release)

```
Tag pushed (v1.2.3)
    ↓
[Trigger packaging-release.yml]
    ↓
    ├→ [build-matrix] ——— test community-release
    │  ├→ test linux-release
    │  └→ test windows-release
    │
    ├→ [package] (after build-matrix) ——— build & package artifacts
    │  └→ Upload: ZIP, DEB, RPM, TGZ + checksums
    │
    └→ [Trigger docker-image.yml]
       └→ Build & push Docker images (multi-platform)
          └→ Tags: 1.2.3, 1.2, latest
```

### Develop Push Flow

```
Push to develop
    ↓
[Trigger cmake-multi-platform.yml] ——— Core build matrix (existing)
    ├→ test Linux (GCC, Clang)
    ├→ test Windows (MSVC)
    └→ Sanitizer jobs (ASan, UBSan)
    
[Trigger docker-image.yml]
    └→ Build Docker image
       └→ Tag: dev-{SHORT_SHA}
       └→ Cache update (no push)
```

---

## 5. Testing the Configuration

### Test Tag Push
```bash
git tag v0.1.0-test
git push origin v0.1.0-test
```
**Expected:** packaging-release.yml + docker-image.yml triggered

### Test Workflow Dispatch
1. Go to GitHub Actions → `release-build`
2. Click "Run workflow"
3. Select `build_matrix: community-only` (faster test)
4. Click "Run"

**Expected:** Only community-release build runs

### Verify Docker Push
```bash
docker pull themisdb/themisdb:dev-{commit-sha}
```
**Expected:** Image available after develop push

---

## 6. Required Secrets

| Secret | Used In | Purpose |
|--------|---------|---------|
| `DOCKERHUB_USERNAME` | docker-image.yml | Docker Hub authentication |
| `DOCKERHUB_TOKEN` | docker-image.yml | Docker Hub push token |

**Setup:**
1. Go to repo → Settings → Secrets and variables → Actions
2. Add `DOCKERHUB_USERNAME` and `DOCKERHUB_TOKEN`

---

## 7. Future Enhancements

- [ ] Add Windows-specific packaging (MSI installer)
- [ ] Add ARM build matrix for linux-release on ARM runners
- [ ] Add SBOM (Software Bill of Materials) generation
- [ ] Add signature verification for releases
- [ ] Add changelog auto-generation from commit log
- [ ] Add release notes generation from merged PRs

---

## 8. Troubleshooting

### Issue: vcpkg checkout fails
**Solution:** Check vcpkg recursive clone permissions and network

### Issue: Windows build timeout
**Solution:** Increase `--parallel` from 4 to lower value or add timeout input

### Issue: Docker push fails with auth
**Solution:** Verify DOCKERHUB_USERNAME and DOCKERHUB_TOKEN secrets are set

### Issue: DEB/RPM generation fails
**Solution:** Expected on non-Debian systems; workflow uses `|| true` to continue

---

## Acceptance Checklist

- [x] Auto-trigger on tags (v*.*.*)
- [x] Auto-trigger on GitHub Releases
- [x] Manual workflow_dispatch with options
- [x] Full build matrix (community, linux-release, windows-release)
- [x] Docker image build and push
- [x] Multi-platform Docker builds
- [x] Version tagging for releases
- [x] Artifact generation and checksums
- [x] Concurrency control
- [x] Proper error handling
- [x] GitHub step summaries
- [x] Artifact retention policies
- [x] YAML syntax validation ✓
- [x] Production-ready permissions model

---

**Status:** 🟢 Production Ready  
**Deployment:** Ready to merge to `develop`
