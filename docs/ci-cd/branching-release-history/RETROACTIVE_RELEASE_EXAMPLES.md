# Retroactive Release Building - Quick Start Examples

This document provides practical examples for using the retroactive release building system.

## Prerequisites Verification

Before starting, verify your environment:

```bash
# Check Git
git --version

# Check CMake
cmake --version

# Check build tools
gcc --version    # Linux
clang --version  # macOS

# Check available tags
git tag -l "v*"
```

## Example 1: Build Single Tag for Linux

Build release packages for version v1.3.4 on Linux:

```bash
# Navigate to repository root
cd /path/to/ThemisDB

# Run retroactive builder
./.github/workflows/04-release_create-release-archive.yml \
    --tag v1.3.4 \
    --platform linux \
    --clean

# Check output
ls -lh release-retroactive/v1.3.4/
```

Expected output:
```
release-retroactive/v1.3.4/
├── themisdb-1.3.4-Linux.tar.gz
├── themisdb-1.3.4-Linux.deb
├── themisdb-1.3.4-Linux.rpm
├── SHA256SUMS.txt
└── RELEASE_NOTES_v1.3.4.md
```

## Example 2: Build from Specific Commit (Intermediate Release)

Build release from a specific commit SHA (useful for intermediate releases between tags):

```bash
# Navigate to repository root
cd /path/to/ThemisDB

# Find the commit SHA you want to build
git log --oneline | grep "Merge"
# Example output: a1b2c3d Merge branch 'release/v1.3.5' into main

# Build from that commit
./.github/workflows/04-release_create-release-archive.yml \
    --commit a1b2c3d \
    --platform linux \
    --clean

# Check output
ls -lh release-retroactive/a1b2c3d/
```

Expected output:
```
release-retroactive/a1b2c3d/
├── themisdb-1.3.5-Linux.tar.gz  (version from VERSION file)
├── themisdb-1.3.5-Linux.deb
├── themisdb-1.3.5-Linux.rpm
├── SHA256SUMS.txt
└── RELEASE_NOTES_a1b2c3d.md
```

## Example 3: Build from Release Branch

Build from a release branch before it's merged:

```bash
# Build from release branch
./.github/workflows/04-release_create-release-archive.yml \
    --commit release/v1.4.0 \
    --platform linux

# Or use branch name directly
./.github/workflows/04-release_create-release-archive.yml \
    --commit origin/release/v1.4.0 \
    --platform linux
```

Expected output:
```
release-retroactive/release-v1.4.0/
├── themisdb-1.4.0-Linux.tar.gz
├── themisdb-1.4.0-Linux.deb
├── themisdb-1.4.0-Linux.rpm
├── SHA256SUMS.txt
└── RELEASE_NOTES_release-v1.4.0.md
```

## Example 4: Build Single Tag for Windows

Build release packages for version v1.3.4 on Windows:

```powershell
# Navigate to repository root
cd C:\path\to\ThemisDB

# Run retroactive builder
.github/workflows/04-release_create-release-archive.yml `
    -Tag v1.3.4 `
    -Platform windows `
    -Clean

# Check output
Get-ChildItem release-retroactive\v1.3.4\
```

Expected output:
```
release-retroactive\v1.3.4\
├── ThemisDB-1.3.4-Win64.zip
├── SHA256SUMS.txt
└── RELEASE_NOTES_v1.3.4.md
```

## Example 5: Build from Merge Commit (Windows)

Build release packages from a merge commit on Windows:

```powershell
# Navigate to repository root
cd C:\path\to\ThemisDB

# Find merge commits
git log --oneline --merges | Select-Object -First 10

# Build from specific merge commit
.github/workflows/04-release_create-release-archive.yml `
    -Commit abc1234 `
    -Platform windows `
    -Clean

# Check output
Get-ChildItem release-retroactive\abc1234\
```

Expected output:
```
release-retroactive\abc1234\
├── ThemisDB-1.3.5-Win64.zip
├── SHA256SUMS.txt
└── RELEASE_NOTES_abc1234.md
```

## Example 6: Build All Tags

Build all available version tags:

```bash
# Linux/macOS
./.github/workflows/04-release_create-release-archive.yml --all-tags --platform linux

# Windows
.github/workflows/04-release_create-release-archive.yml -AllTags
```

Expected output structure:
```
release-retroactive/
├── v1.0.0/
│   ├── packages...
│   ├── SHA256SUMS.txt
│   └── RELEASE_NOTES_v1.0.0.md
├── v1.3.0/
│   ├── packages...
│   ├── SHA256SUMS.txt
│   └── RELEASE_NOTES_v1.3.0.md
└── v1.3.4/
    ├── packages...
    ├── SHA256SUMS.txt
    └── RELEASE_NOTES_v1.3.4.md
```

## Example 4: Custom Output Directory

Use a custom output directory:

```bash
# Linux/macOS
./.github/workflows/04-release_create-release-archive.yml \
    --tag v1.3.4 \
    --output-dir /tmp/my-releases

# Windows
.github/workflows/04-release_create-release-archive.yml `
    -Tag v1.3.4 `
    -OutputDir "C:\temp\my-releases"
```

## Example 5: GitHub Actions Workflow

Trigger via GitHub Actions (manual workflow dispatch):

1. Go to GitHub repository
2. Navigate to "Actions" tab
3. Select "Retroactive Release Build" workflow
4. Click "Run workflow"
5. Configure options:
   - **Tag**: `v1.3.4` (or leave empty for all tags)
   - **Platform**: Select target platform
   - **Upload to release**: Check if you want to upload to GitHub release
6. Click "Run workflow"

The workflow will:
- Build binaries for the specified tag(s)
- Generate packages
- Upload artifacts (available for 90 days)
- Optionally upload to GitHub release

## Example 6: Verify Checksums

After building, verify package integrity:

```bash
# Linux/macOS
cd release-retroactive/v1.3.4
sha256sum -c SHA256SUMS.txt

# Windows
cd release-retroactive\v1.3.4
Get-Content SHA256SUMS.txt | ForEach-Object {
    $hash, $file = $_ -split '  '
    $computed = (Get-FileHash $file -Algorithm SHA256).Hash.ToLower()
    if ($hash -eq $computed) {
        Write-Host "✓ $file" -ForegroundColor Green
    } else {
        Write-Host "✗ $file" -ForegroundColor Red
    }
}
```

## Example 7: Test Installation

Test the generated packages:

### Linux DEB Package
```bash
# Install
sudo dpkg -i release-retroactive/v1.3.4/themisdb-1.3.4-Linux.deb

# Verify
themis_server --version

# Uninstall
sudo dpkg -r themisdb
```

### Linux RPM Package
```bash
# Install
sudo rpm -i release-retroactive/v1.3.4/themisdb-1.3.4-Linux.rpm

# Verify
themis_server --version

# Uninstall
sudo rpm -e themisdb
```

### Linux TGZ Archive
```bash
# Extract
tar -xzf release-retroactive/v1.3.4/themisdb-1.3.4-Linux.tar.gz

# Run
./themis-1.3.4/bin/themis_server --version
```

### Windows ZIP Archive
```powershell
# Extract
Expand-Archive release-retroactive\v1.3.4\ThemisDB-1.3.4-Win64.zip -DestinationPath test

# Run
.\test\bin\themis_server.exe --version
```

## Example 8: Continuous Integration

Integrate into your CI/CD pipeline:

```yaml
# .github/workflows/custom-build.yml
name: Custom Retroactive Build

on:
  schedule:
    - cron: '0 2 * * 0'  # Weekly on Sunday at 2 AM

jobs:
  build-all-tags:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
      
      - name: Build All Tags
        run: |
          ./.github/workflows/04-release_create-release-archive.yml --all-tags
      
      - name: Archive Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: all-releases
          path: release-retroactive/
          retention-days: 90
```

## Troubleshooting Examples

### Example 9: Handle Build Failures

If a build fails, you can:

```bash
# Try with clean build
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4 --clean

# Check logs (build happens in build-retroactive directory)
cat build-retroactive/CMakeCache.txt
cat build-retroactive/CMakeError.log

# Manually checkout and debug
git checkout v1.3.4
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --verbose
```

### Example 10: List and Filter Tags

Find specific version ranges:

```bash
# All v1.3.x releases
git tag -l "v1.3.*"

# Build only v1.3.x series
for tag in $(git tag -l "v1.3.*"); do
    ./.github/workflows/04-release_create-release-archive.yml --tag $tag
done
```

## Performance Tips

1. **Parallel Builds**: Use GitHub Actions workflow for parallel multi-platform builds
2. **Disk Space**: Clean between builds or use external storage
3. **Caching**: Consider caching vcpkg or dependencies between builds
4. **Selective Building**: Build only necessary platforms

## Security Best Practices

1. **Verify Tags**: Always verify Git tag signatures before building
2. **Checksum Verification**: Verify SHA256 checksums after building
3. **Clean Environment**: Use clean build environments (Docker, CI/CD)
4. **Code Signing**: Sign binaries before distribution
5. **Audit Trail**: Keep logs of build dates and sources

## Next Steps

- Read full documentation: [RETROACTIVE_RELEASE_BUILDING.md](RETROACTIVE_RELEASE_BUILDING.md)
- Review release workflows: [.github/workflows/](../.github/workflows/)
- Check contributing guide: [CONTRIBUTING.md](../CONTRIBUTING.md)

---

**Last Updated**: 2026-04-06  
**Script Version**: 1.0.0
