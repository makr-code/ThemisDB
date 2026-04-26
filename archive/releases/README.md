# ThemisDB Release Archives

This directory contains documentation and tooling for creating and managing ThemisDB source code release archives.

## Overview

ThemisDB uses a standardized process for creating source code archives for each version release. Source archives are distributed as ZIP files with SHA256 checksums, excluding large dependency directories that can be built from the source.

## Release Archive Process

### What's Included

Source archives contain:
- All ThemisDB source code
- Build system files (CMake, vcpkg configuration)
- Documentation
- Scripts and tools
- Tests and examples
- Configuration files

### What's Excluded

To keep archive sizes manageable, the following directories are excluded:
- `external/` - Third-party dependencies (should be built via vcpkg)
- `vcpkg/` - vcpkg package manager files (downloaded during build)
- `llama.cpp/` - Large external dependency (submodule)
- `.git/` - Git metadata

## Creating a Release Archive

### Using the Archive Script

The `scripts/archive-version.sh` script automates the creation of source archives:

```bash
# Syntax
./scripts/archive-version.sh <version> <commit-sha> [edition] [sourcecode|binary] [arm|x86|x64]

# Example: Create v1.0.0 archive
./scripts/archive-version.sh 1.0.0 60e901590e4b2e5990877b3c0f49cdcd2bb1f992 community sourcecode x64

# Example: Create v1.3.4 archive
./scripts/archive-version.sh 1.3.4 abc123def456 community sourcecode x64
```

**Parameters:**
- `<version>`: Version number in format `X.Y.Z` or `X.Y.Z-suffix` (e.g., `1.0.0`, `1.3.4-beta`)
- `<commit-sha>`: Git commit SHA to archive (full or short hash)
- `[edition]`: Lowercase edition token (default: `community`)
- `[sourcecode|binary]`: Artefact kind token (default: `sourcecode`)
- `[arm|x86|x64]`: Architecture token (default: `x64`)

**Output:**
- `themisdb-{version}-community-sourcecode-x64.zip` - Source code archive
- `themisdb-{version}-community-sourcecode-x64.zip.sha256` - SHA256 checksum file

### Manual Archive Creation

If you need to create an archive manually:

```bash
VERSION="1.0.0"
COMMIT_SHA="60e9015"

git archive --format=zip \
   --output="themisdb-${VERSION}-community-sourcecode-x64.zip" \
  --prefix="themisdb-${VERSION}/" \
  "${COMMIT_SHA}" \
  -- . \
  ':!external/' \
  ':!vcpkg/' \
  ':!llama.cpp/' \
  ':!.git/'

# Generate checksum
sha256sum "themisdb-${VERSION}-community-sourcecode-x64.zip" > "themisdb-${VERSION}-community-sourcecode-x64.zip.sha256"
```

## GitHub Release Workflow

### Automated Release (GitHub Actions)

Use the GitHub Actions workflow for automated release creation:

1. Go to **Actions** → **Create Release Archive**
2. Click **Run workflow**
3. Enter the version number (e.g., `1.0.0`)
4. The workflow will:
   - Find the appropriate commit (from git tag or VERSION file)
   - Create the source archive
   - Generate checksums
   - Create a GitHub Release with the archive as an asset

### Manual Release

If creating a release manually:

1. **Create Git Tag** (if not exists):
   ```bash
   git tag -a v1.0.0 60e9015 -m "Release v1.0.0"
   git push origin v1.0.0
   ```

2. **Create Source Archive**:
   ```bash
   ./scripts/archive-version.sh 1.0.0 60e9015 community sourcecode x64
   ```

3. **Create GitHub Release**:
   - Go to https://github.com/makr-code/ThemisDB/releases/new
   - Select the tag (e.g., `v1.0.0`)
   - Add release title: "ThemisDB v1.0.0"
   - Add description from `docs/de/archive/RELEASE_NOTES_v1.0.0.md`
   - Upload `themisdb-1.0.0-community-sourcecode-x64.zip`
   - Include SHA256 checksum in the release notes

## Release Checklist

Before creating a release:

- [ ] Update `VERSION` file with new version number
- [ ] Create release notes in `docs/de/archive/RELEASE_NOTES_vX.Y.Z.md`
- [ ] Update `CHANGELOG.md`
- [ ] Commit all changes
- [ ] Create and push Git tag: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`
- [ ] Create source archive using `scripts/archive-version.sh`
- [ ] Create GitHub Release with archive and checksums
- [ ] Verify download links work
- [ ] Announce release

## Archive Verification

To verify an archive's integrity:

```bash
# Check SHA256 checksum
sha256sum -c themisdb-1.0.0-community-sourcecode-x64.zip.sha256

# Extract and inspect contents
unzip -l themisdb-1.0.0-community-sourcecode-x64.zip | head -50

# Verify excluded directories are not present
unzip -l themisdb-1.0.0-community-sourcecode-x64.zip | grep -E "(external/|vcpkg/|llama\.cpp/)"
# Should return no results
```

## Building from Source Archive

To build ThemisDB from a source archive:

```bash
# Extract archive
unzip themisdb-1.0.0-community-sourcecode-x64.zip
cd themisdb-1.0.0

# Build with CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Or use the build script
./scripts/build.sh
```

Note: Dependencies (vcpkg packages) will be downloaded during the build process.

## Historical Releases

### v1.0.0 (December 2, 2025)
- **Commit**: `60e901590e4b2e5990877b3c0f49cdcd2bb1f992`
- **Release Notes**: [RELEASE_NOTES_v1.0.0.md](../../docs/de/archive/RELEASE_NOTES_v1.0.0.md)
- **SHA256**: See GitHub Release

## Related Documentation

- [RELEASE_NOTES_v1.0.0.md](../../docs/de/archive/RELEASE_NOTES_v1.0.0.md) - v1.0.0 release notes
- [CHANGELOG.md](../../CHANGELOG.md) - Complete changelog
- [CONTRIBUTING.md](../../CONTRIBUTING.md) - Contribution guidelines
- [scripts/archive-version.sh](../../scripts/archive-version.sh) - Archive creation script

## Support

For questions about releases:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions
