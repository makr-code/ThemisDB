# Retroactive Release Building Guide

This guide explains how to extract source code at specific version tags, commits, or branches and build/package binaries retroactively for all past releases of ThemisDB.

## Overview

The retroactive release building system allows you to:
- Extract source code at any version tag
- **Build from specific commits (intermediate releases)**
- **Build from merge commits or branch names**
- Build binaries for that specific version
- Package the binaries in release formats (TGZ, DEB, RPM, ZIP)
- Generate SHA256 checksums
- Create release notes automatically

This is useful for:
- Regenerating binaries for past releases
- **Building intermediate releases from merge commits**
- Building releases for new platforms retroactively
- Creating consistent release artifacts across all versions
- Supporting older versions with new builds

## Prerequisites

### Linux/macOS
- Git
- CMake 3.15+
- C++ compiler (GCC 9+ or Clang 10+)
- Build tools (make, ninja)
- Package creation tools (rpmbuild, dpkg-deb)

### Windows
- Git
- CMake 3.15+
- Visual Studio 2019 or later
- PowerShell 5.0+

## Usage

### Linux/macOS

#### List Available Tags
```bash
./.github/workflows/04-release_create-release-archive.yml --list-tags
```

#### Build Specific Tag
```bash
# Build for Linux
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4 --platform linux

# Build for all platforms (if supported)
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4 --platform all
```

#### Build from Specific Commit (NEW)
```bash
# Build from a specific commit SHA (intermediate release)
./.github/workflows/04-release_create-release-archive.yml --commit a1b2c3d4 --platform linux

# Build from a merge commit
./.github/workflows/04-release_create-release-archive.yml --commit abc123 --platform linux

# Build from a branch name (e.g., release branch)
./.github/workflows/04-release_create-release-archive.yml --commit release/v1.3.4 --platform linux
```

#### Build All Tags
```bash
# Build all version tags for Linux
./.github/workflows/04-release_create-release-archive.yml --all-tags --platform linux

# Build all tags for all platforms
./.github/workflows/04-release_create-release-archive.yml --all-tags --platform all
```

#### Advanced Options
```bash
# Clean build directories before building
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4 --clean

# Custom output directory
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4 --output-dir ./my-releases

# Skip build, only package existing binaries
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4 --skip-build
```

### Windows

#### List Available Tags
```powershell
.github/workflows/04-release_create-release-archive.yml -ListTags
```

#### Build Specific Tag
```powershell
# Build for Windows
.github/workflows/04-release_create-release-archive.yml -Tag v1.3.4

# Build with clean
.github/workflows/04-release_create-release-archive.yml -Tag v1.3.4 -Clean
```

#### Build from Specific Commit (NEW)
```powershell
# Build from a specific commit SHA
.github/workflows/04-release_create-release-archive.yml -Commit a1b2c3d4

# Build from a merge commit or branch
.github/workflows/04-release_create-release-archive.yml -Commit release/v1.3.4 -Clean
```

#### Build All Tags
```powershell
# Build all version tags
.github/workflows/04-release_create-release-archive.yml -AllTags

# Custom output directory
.github/workflows/04-release_create-release-archive.yml -AllTags -OutputDir ".\my-releases"
```

## Output Structure

After running the retroactive build, the output directory will have the following structure:

```
release-retroactive/
├── v1.3.0/
│   ├── themisdb-1.3.0-Linux.tar.gz
│   ├── themisdb-1.3.0-Linux.deb
│   ├── themisdb-1.3.0-Linux.rpm
│   ├── SHA256SUMS.txt
│   └── RELEASE_NOTES_v1.3.0.md
├── v1.3.4/
│   ├── themisdb-1.3.4-Linux.tar.gz
│   ├── themisdb-1.3.4-Linux.deb
│   ├── themisdb-1.3.4-Linux.rpm
│   ├── SHA256SUMS.txt
│   └── RELEASE_NOTES_v1.3.4.md
└── ...
```

## Generated Files

For each version tag, the following files are generated:

### Linux Packages
- **TGZ Archive**: `themisdb-{version}-Linux.tar.gz` - Portable archive
- **DEB Package**: `themisdb-{version}-Linux.deb` - Debian/Ubuntu package
- **RPM Package**: `themisdb-{version}-Linux.rpm` - Red Hat/CentOS package

### Windows Packages
- **ZIP Archive**: `ThemisDB-{version}-Win64.zip` - Windows binaries

### Metadata Files
- **SHA256SUMS.txt**: SHA256 checksums for all packages
- **RELEASE_NOTES_{tag}.md**: Automatically generated release notes

## Workflow Integration

### Manual Retroactive Builds

If you need to manually build releases for existing tags:

```bash
# Example: Build releases for v1.3.0 and v1.3.4
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.0
./.github/workflows/04-release_create-release-archive.yml --tag v1.3.4

# Or build all at once
./.github/workflows/04-release_create-release-archive.yml --all-tags
```

### Automated CI/CD Integration

You can integrate the retroactive builder into GitHub Actions:

```yaml
name: Retroactive Release Build

on:
  workflow_dispatch:
    inputs:
      tag:
        description: 'Version tag to build (e.g., v1.3.4)'
        required: true
        type: string

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0  # Fetch all history for tags
      
      - name: Run Retroactive Builder
        run: |
          ./.github/workflows/04-release_create-release-archive.yml --tag ${{ inputs.tag }}
      
      - name: Upload Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: release-${{ inputs.tag }}
          path: release-retroactive/${{ inputs.tag }}/*
```

## Verifying Builds

After building, verify the packages:

### Linux
```bash
# Verify checksums
cd release-retroactive/v1.3.4
sha256sum -c SHA256SUMS.txt

# Test DEB package
sudo dpkg -i themisdb-1.3.4-Linux.deb
themis_server --version

# Test RPM package
sudo rpm -i themisdb-1.3.4-Linux.rpm
themis_server --version
```

### Windows
```powershell
# Verify checksums
cd release-retroactive\v1.3.4
Get-FileHash -Algorithm SHA256 ThemisDB-1.3.4-Win64.zip

# Test binary
Expand-Archive ThemisDB-1.3.4-Win64.zip -DestinationPath test
.\test\bin\themis_server.exe --version
```

## Troubleshooting

### Tag Not Found
**Problem**: Error "Failed to checkout tag: v1.x.x"

**Solution**: Ensure the tag exists in the repository:
```bash
git fetch --tags
git tag -l "v*"
```

### Build Failures
**Problem**: Build fails for a specific tag

**Solution**: 
1. Check the build dependencies for that version
2. Review the CMakeLists.txt at that tag
3. Ensure all submodules are initialized
4. Try with `--clean` flag

### Package Generation Fails
**Problem**: cpack fails to generate packages

**Solution**:
1. Install packaging tools: `sudo apt-get install rpm dpkg-dev`
2. Check CMake configuration supports CPack
3. Review error messages in build log

### Disk Space Issues
**Problem**: Running out of disk space when building multiple tags

**Solution**:
1. Build tags individually instead of all at once
2. Use `--clean` flag between builds
3. Clear output directory periodically
4. Increase disk space or use external storage

## Best Practices

1. **Test Before Mass Building**: Test with one tag first before building all tags
2. **Use Version Control**: Commit the scripts to your repository
3. **Document Custom Builds**: Add notes if specific tags require special build flags
4. **Automate Verification**: Create verification scripts to test packages
5. **Archive Carefully**: Store generated packages in a safe location
6. **Version Documentation**: Keep release notes with packages

## Security Considerations

1. **Checksum Verification**: Always verify SHA256 checksums before distributing
2. **Code Signing**: Consider signing packages for Windows and macOS
3. **GPG Signatures**: Add GPG signatures for Linux packages
4. **Build Environment**: Use clean, isolated build environments
5. **Audit Trail**: Keep logs of build dates and build hosts

## Related Documentation

- [Release Workflow](/.github/workflows/release.yml) - Automated release workflow
- [Contributing Guide](/CONTRIBUTING.md) - Contributing guidelines
- [Build Documentation](/docs/build/README.md) - General build instructions
- [Docker Deployment](/docker/README.md) - Docker-based builds

## Support

For issues or questions:
- GitHub Issues: [Report a problem](https://github.com/makr-code/ThemisDB/issues)
- Discussions: [Ask the community](https://github.com/makr-code/ThemisDB/discussions)

---

**Last Updated**: 2026-04-06  
**Script Version**: 1.0.0
