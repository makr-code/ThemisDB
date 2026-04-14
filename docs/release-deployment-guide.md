# ThemisDB v1.4.0 Release Deployment Guide

**Deployment Date**: January 12, 2026

## Table of Contents
1. [Pre-Release Checklist](#pre-release-checklist)
2. [Build Process](#build-process)
3. [Packaging](#packaging)
4. [GitHub Release](#github-release)
5. [Deployment Verification](#deployment-verification)

---

## Pre-Release Checklist

- [x] Version bumped to v1.4.0 in `VERSION`
- [x] `src/version.h` updated (THEMIS_VERSION_PATCH = 0)
- [x] CHANGELOG.md updated with v1.4.0 entries
- [x] Git Flow branches created:
  - [x] `main` branch with v1.4.0 tag
  - [x] `develop` branch with v1.4.1-dev version
  - [x] `release-v1.4.0` branch
- [x] All branches pushed to GitHub origin
- [x] No merge conflicts remaining
- [x] All GitHub Actions passing (CI/CD)

---

## Build Process

### Windows Release Build

**Status**: ⏳ In Progress (started 2026-01-12 15:50 UTC)

**Command**:
```powershell
cd C:\VCC\themis
cmake --preset windows-vs2022-release
cmake --build build-msvc --config Release --parallel 8
```

**Expected Output**:
- `build-msvc\Release\themis_server.exe` (~50-80 MB)
- `build-msvc\Release\themis_tests.exe` (~20-40 MB)
- `build-msvc\Release\*.lib` (static libraries)

**Estimated Time**: 20-30 minutes
- vcpkg stage: 56 min (already done)
- CMake configure: ~25 sec (already done)
- Build compilation: ~20-30 min (in progress)

**Monitor Status**:
```bat
cd C:\VCC\themis
scripts\check-build-status.bat
```

### Linux Release Build

**Status**: ⏳ Pending (awaits Windows completion)

**Command**:
```bash
cd /path/to/themis
bash .github/workflows/04-release_build-binary-linux.yml
```

**Expected Output**:
- `build-linux-release/install/bin/themis_server` (~50-80 MB)
- `build-linux-release/install/bin/themis_tests` (~20-40 MB)

**Estimated Time**: 15-20 minutes

---

## Packaging

### Windows Package

**Command**:
```bat
cd C:\VCC\themis
scripts\package-windows.bat
```

**Output**:
- `release\v1.4.0-windows\themisdb-1.4.0-windows-x64.zip`
- `release\v1.4.0-windows\SHA256SUMS`
- `release\v1.4.0-windows\SHA256SUMS.gpg` (optional)

**Manual Steps** (if script fails):
```powershell
# Create ZIP
Compress-Archive -Path "C:\VCC\themis\build-msvc\Release\themis_server.exe" `
                 -DestinationPath "release\v1.4.0-windows\themisdb-1.4.0-windows-x64.zip"

# Generate checksums
Get-FileHash "release\v1.4.0-windows\themisdb-1.4.0-windows-x64.zip" -Algorithm SHA256 | `
  Select-Object @{Name="Hash"; Expression={$_.Hash}}, `
                @{Name="File"; Expression={"themisdb-1.4.0-windows-x64.zip"}} | `
  Format-Table -HideTableHeaders | Out-File "release\v1.4.0-windows\SHA256SUMS"
```

### Linux Package

**Command**:
```bash
cd /path/to/themis/build-linux-release

# Create tarball
tar czf ../release/v1.4.0-linux/themisdb-1.4.0-linux-x64.tar.gz install/

# Generate checksums
cd ../release/v1.4.0-linux/
sha256sum *.tar.gz > SHA256SUMS

# Optional: DEB package
cpack -G DEB -C Release
```

**Output**:
- `release/v1.4.0-linux/themisdb-1.4.0-linux-x64.tar.gz`
- `release/v1.4.0-linux/themisdb-1.4.0-linux-x64.deb` (optional)
- `release/v1.4.0-linux/SHA256SUMS`

---

## GitHub Release

### Using Script (Automated)

**Requirements**:
```bash
pip install PyGithub
```

**Command**:
```bash
python3 .github/workflows/04-release_build-binary-linux.yml \
  --version v1.4.0 \
  --token <GitHub PAT> \
  --repo makr-code/ThemisDB
```

**Dry Run** (preview without uploading):
```bash
python3 .github/workflows/04-release_build-binary-linux.yml \
  --version v1.4.0 \
  --token <GitHub PAT> \
  --dry-run
```

### Manual Creation

1. **Go to Release Page**:
   - URL: https://github.com/makr-code/ThemisDB/releases/new

2. **Fill Release Details**:
   - **Tag**: `v1.4.0`
   - **Title**: `ThemisDB v1.4.0`
   - **Description**: (copy from [RELEASE_NOTES.md](../release/v1.4.0/RELEASE_NOTES.md))

3. **Attach Files**:
   - Windows: `themisdb-1.4.0-windows-x64.zip`
   - Linux: `themisdb-1.4.0-linux-x64.tar.gz`, `themisdb-1.4.0-linux-x64.deb`
   - Checksums: `SHA256SUMS`
   - Signature: `SHA256SUMS.gpg` (if signed)

4. **Publish**:
   - Click "Publish release"

---

## Deployment Verification

### Test Windows Binary

```powershell
# Extract archive
Expand-Archive "themisdb-1.4.0-windows-x64.zip" -DestinationPath "C:\Test\ThemisDB"

# Test executable
C:\Test\ThemisDB\themis_server.exe --help
C:\Test\ThemisDB\themis_server.exe --version

# Optional: Run tests
C:\Test\ThemisDB\themis_tests.exe
```

**Expected Output**:
```
ThemisDB Server v1.4.0
Usage: themis_server [OPTIONS]

Options:
  --help                Show this message
  --version             Show version
  --config FILE         Configuration file (default: config.json)
  ...
```

### Test Linux Binary

```bash
# Extract archive
tar xzf themisdb-1.4.0-linux-x64.tar.gz
cd themisdb-1.4.0-linux-x64

# Test executable
./bin/themis_server --help
./bin/themis_server --version

# Optional: Run tests
./bin/themis_tests
```

**Expected Output**:
```
ThemisDB Server v1.4.0
Usage: themis_server [OPTIONS]

Options:
  --help                Show this message
  --version             Show version
  --config FILE         Configuration file (default: config.json)
  ...
```

### Verify Checksums

**Windows**:
```powershell
$hash = (Get-FileHash "themisdb-1.4.0-windows-x64.zip" -Algorithm SHA256).Hash
$expected = (Get-Content SHA256SUMS | Select-Object -First 1).Split()[0]
$hash -eq $expected ? "✅ Checksum valid" : "❌ Checksum mismatch"
```

**Linux**:
```bash
sha256sum -c SHA256SUMS
# Expected: themisdb-1.4.0-linux-x64.tar.gz: OK
```

---

## Post-Release Actions

### GitHub

1. **Create Release Milestone**:
   - Title: `v1.4.0`
   - Status: Closed
   - Issues: Mark all as closed

2. **Update Documentation**:
   - Update [docs/downloads.md](../docs/downloads.md) with download links
   - Update [README.md](../README.md) "Latest Release" section

3. **Announce Release**:
   - GitHub Discussions: Create announcement
   - Email: Notify stakeholders
   - Social media: Tweet/post release announcement

### Repository Maintenance

1. **Protect Branches**:
   ```bash
   # main branch requires reviews and passing checks
   # develop branch: requires reviews
   ```

2. **Configure Branch Protection**:
   - Settings → Branches → main
     - Require branches to be up to date before merging
     - Require status checks (CI/CD)
     - Dismiss stale PR approvals
     - Restrict who can push

3. **Archive Release Branch**:
   ```bash
   # Keep release-v1.4.0 for hotfix purposes
   # Can delete after next minor release (v1.5.0)
   ```

---

## Troubleshooting

### Build Failures

**CMake Error**:
```
CMake Error at cmake/Dependencies.cmake:275
```
- **Fix**: Remove merge conflict markers (`<<<<<<<`, `=======`, `>>>>>>>`)
- **Status**: ✅ Already fixed in this release

**MSVC Compiler Error**:
```
fatal error C1083: Cannot open include file
```
- **Fix**: Ensure vcpkg is fully installed: `vcpkg install --triplet x64-windows`
- **Workaround**: Run: `del build-msvc && cmake --preset windows-vs2022-release`

**LLM Integration Error**:
```
error: llama.cpp not found
```
- **Fix**: Ensure `llama.cpp/` directory exists
- **Workaround**: `git clone https://github.com/ggerganov/llama.cpp.git llama.cpp`

### Package Issues

**ZIP File Empty**:
- Ensure `build-msvc\Release\themis_server.exe` exists before packaging
- Check build logs for compilation errors

**Checksum Mismatch**:
- Regenerate checksums after repackaging
- Ensure files aren't being modified between builds

### GitHub Release Issues

**Upload Fails**:
- Check GitHub PAT permissions (requires `repo`, `public_repo`)
- Verify network connectivity
- Try manual upload via web interface

**Release Not Visible**:
- Tag must match semver format (`v1.4.0`)
- Check if repository is private (releases visible only to collaborators)

---

## Support

For issues or questions:
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Email**: support@themisdb.com

---

**Last Updated**: April 2026
**Next Release**: v1.5.0 (scheduled for Q2 2026)
