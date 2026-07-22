# Build Reproducibility CI/CD Integration Guide

**Version:** 1.0.0  
**Status:** Production  
**Date:** 2026-07-22

## Overview

This guide explains how to integrate ThemisDB's Build Reproducibility feature into your CI/CD pipelines. Build reproducibility ensures that a binary can be traced back to its exact source commit and toolchain, enabling security auditing and compliance verification.

## Quick Start

### Step 1: Enable Reproducibility in Your Build

Build Reproducibility is **enabled by default** in all ThemisDB builds. During CMake configuration, metadata is automatically captured:

```bash
cmake --preset community-release
# Output includes:
# -- Build reproducibility: commit=abc123... branch=develop dirty=0
```

### Step 2: Export Manifest After Build

After compiling, export the build manifest:

```bash
# In your CI script, after successful build:
cmake --install ./build --prefix /install

# Export manifest programmatically or via CLI tool
# (Implement according to your build system)
```

Or call the C++ API directly:

```cpp
#include "themis/build_info.h"

int main() {
    const bool success = themis::build_info::exportBuildManifest(
        "/path/to/build-manifest.json"
    );
    return success ? 0 : 1;
}
```

### Step 3: Archive Manifest with Release

Store the manifest alongside your release artifacts:

```bash
# In release packaging phase
mkdir -p releases/${VERSION}/
cp build/Release/themisdb releases/${VERSION}/
cp build-manifest.json releases/${VERSION}/manifest.json

# Create checksums
sha256sum releases/${VERSION}/themisdb > releases/${VERSION}/CHECKSUMS
sha256sum releases/${VERSION}/manifest.json >> releases/${VERSION}/CHECKSUMS
```

### Step 4: Verify on Consumer Side

Consumers can verify the binary matches the declared commit:

```cpp
#include "themis/build_info.h"

bool verifyReleaseAuthenticity(const std::string& manifest_path) {
    return themis::build_info::verifyBuildManifest(manifest_path);
}
```

## GitHub Actions Integration

### Example: Build with Reproducibility Gate

```yaml
name: Build with Reproducibility Gate

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v3
        with:
          fetch-depth: 0  # Ensure full git history
      
      - name: Configure CMake
        run: cmake --preset linux-release
      
      - name: Build
        run: cmake --build --preset linux-release --parallel 8
      
      - name: Export Build Manifest
        run: |
          mkdir -p build-artifacts
          ./build/Release/themis-cli export-manifest \
            --output build-artifacts/build-manifest.json
      
      - name: Verify Manifest Structure
        run: |
          # Validate JSON structure
          python3 -m json.tool build-artifacts/build-manifest.json > /dev/null
          
          # Extract git_commit from manifest
          GIT_COMMIT=$(python3 -c "
            import json
            with open('build-artifacts/build-manifest.json') as f:
              data = json.load(f)
              print(data.get('git_commit', ''))
          ")
          
          echo "Manifest git_commit: ${GIT_COMMIT}"
          echo "Current git SHA: $(git rev-parse HEAD)"
          
          # Verify they match
          CURRENT_SHA=$(git rev-parse HEAD)
          if [ "${GIT_COMMIT}" != "${CURRENT_SHA}" ]; then
            echo "ERROR: Build commit mismatch!"
            exit 1
          fi
      
      - name: Upload Manifest
        uses: actions/upload-artifact@v3
        with:
          name: build-manifest
          path: build-artifacts/build-manifest.json

  verify:
    runs-on: ubuntu-latest
    needs: build
    
    steps:
      - uses: actions/download-artifact@v3
        with:
          name: build-manifest
      
      - name: Check Manifest Requirements
        run: |
          python3 << 'EOF'
          import json
          
          with open('build-manifest.json') as f:
            manifest = json.load(f)
          
          # Verify no dirty builds for releases
          if manifest.get('git_dirty'):
            print("WARNING: Build from dirty working tree")
            if 'release' in os.environ.get('GITHUB_REF_NAME', ''):
              raise Exception("Dirty builds not allowed for releases")
          
          # Verify toolchain version
          toolchain = manifest.get('toolchain', '')
          print(f"✓ Toolchain: {toolchain}")
          
          # Verify dependencies
          deps = manifest.get('dependencies', {})
          print(f"✓ Dependencies: {', '.join(deps.keys())}")
          
          # Verify binary hash
          binary_hash = manifest.get('binary_hash', '')
          if binary_hash.startswith('('):
            print(f"⚠ Binary hash: {binary_hash} (not available)")
          else:
            print(f"✓ Binary hash: {binary_hash[:16]}...")
          
          EOF
```

### Example: Release Verification Gate

```yaml
name: Release Verification

on:
  release:
    types: [published]

jobs:
  verify-release:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Download Release Artifacts
        run: |
          mkdir -p release-artifacts
          gh release download ${{ github.event.release.tag_name }} \
            --pattern '*.json' \
            --dir release-artifacts
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
      
      - name: Verify All Manifests
        run: |
          set -e
          for manifest in release-artifacts/*.json; do
            echo "Verifying $(basename $manifest)..."
            
            # Parse manifest
            GIT_COMMIT=$(jq -r '.git_commit' "$manifest")
            GIT_DIRTY=$(jq -r '.git_dirty' "$manifest")
            
            # Check dirty flag
            if [ "$GIT_DIRTY" == "true" ]; then
              echo "ERROR: Release from dirty build!"
              exit 1
            fi
            
            # Verify commit exists in repository
            if ! git cat-file -e "$GIT_COMMIT" 2>/dev/null; then
              echo "ERROR: Commit $GIT_COMMIT not found in repository!"
              exit 1
            fi
            
            echo "✓ Manifest for commit $GIT_COMMIT verified"
          done
      
      - name: Create Verification Report
        run: |
          cat > VERIFICATION_REPORT.md << 'EOF'
          # Release Verification Report
          
          **Release:** ${{ github.event.release.tag_name }}
          **Date:** $(date -u +"%Y-%m-%dT%H:%M:%SZ")
          
          ## Manifests
          $(ls -1 release-artifacts/*.json | sed 's|^|- |')
          
          ## Status
          All build manifests verified ✓
          
          EOF
          cat VERIFICATION_REPORT.md
```

## GitLab CI Integration

```yaml
build_reproducible:
  stage: build
  image: ubuntu:22.04
  script:
    - cmake --preset linux-release
    - cmake --build --preset linux-release
    - ./build/Release/themis-cli export-manifest --output build-manifest.json
  artifacts:
    paths:
      - build-manifest.json
    expire_in: 30 days
  after_script:
    # Verify manifest structure
    - |
      python3 -c "
        import json
        with open('build-manifest.json') as f:
          data = json.load(f)
          assert 'git_commit' in data
          assert 'toolchain' in data
          assert 'binary_hash' in data
          print('✓ Manifest structure valid')
      "

verify_release_reproducibility:
  stage: verify
  image: python:3.10
  script:
    - |
      python3 << 'PYTHON'
      import json
      import sys
      
      with open('build-manifest.json') as f:
        manifest = json.load(f)
      
      # Gate: reject dirty builds for releases
      if manifest.get('git_dirty'):
        if 'release' in os.environ.get('CI_COMMIT_REF_NAME', ''):
          print("GATE FAILED: Dirty build not allowed for release")
          sys.exit(1)
      
      # Report
      print(f"Git Commit: {manifest['git_commit']}")
      print(f"Branch: {manifest['git_branch']}")
      print(f"Toolchain: {manifest['toolchain']}")
      print(f"Binary Hash: {manifest['binary_hash'][:16]}...")
      print("✓ Release reproducibility verified")
      PYTHON
  only:
    - tags
```

## Jenkins Pipeline Integration

```groovy
pipeline {
    agent any
    
    stages {
        stage('Build') {
            steps {
                sh '''
                    cmake --preset community-release
                    cmake --build --preset community-release
                '''
            }
        }
        
        stage('Export Manifest') {
            steps {
                sh '''
                    mkdir -p artifacts
                    ./build/Release/themis-cli export-manifest \
                        --output artifacts/build-manifest.json
                '''
            }
        }
        
        stage('Verify Reproducibility') {
            steps {
                sh '''
                    # Validate JSON
                    python3 -m json.tool artifacts/build-manifest.json
                    
                    # Extract and verify git_commit
                    MANIFEST_COMMIT=$(python3 -c "
                        import json
                        with open('artifacts/build-manifest.json') as f:
                            print(json.load(f)['git_commit'])
                    ")
                    
                    CURRENT_COMMIT=$(git rev-parse HEAD)
                    
                    if [ "$MANIFEST_COMMIT" != "$CURRENT_COMMIT" ]; then
                        echo "ERROR: Commit mismatch!"
                        exit 1
                    fi
                    
                    echo "✓ Build reproducibility verified"
                '''
            }
        }
        
        stage('Archive') {
            steps {
                archiveArtifacts artifacts: 'artifacts/build-manifest.json'
            }
        }
    }
}
```

## Manual Verification

### Generate and Verify Locally

```bash
# Clone and build from a known commit
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git checkout <COMMIT_SHA>

# Build with reproducibility
cmake --preset community-release
cmake --build --preset community-release

# Export manifest
./build/Release/themis-cli export-manifest local-build.json

# Examine manifest
cat local-build.json | jq .

# Verify manifest matches binary
./build/Release/themis-cli verify-manifest local-build.json
# Output: Build manifest verified ✓
```

### Compare Two Builds

```bash
# Build 1 on machine A
git checkout abc123def456
cmake --preset community-release
cmake --build --preset community-release
./build/Release/themis-cli export-manifest manifest-a.json

# Build 2 on machine B
git checkout abc123def456  # Same commit!
cmake --preset community-release
cmake --build --preset community-release
./build/Release/themis-cli export-manifest manifest-b.json

# Compare manifests
diff <(jq -S . manifest-a.json) <(jq -S . manifest-b.json)

# If identical, builds are reproducible!
# If different binary hashes, investigate toolchain differences
```

## Reproducibility Checklist

- [ ] Build from clean repository state
- [ ] CMake configuration captures git metadata (verify in configure output)
- [ ] Build completes without warnings
- [ ] Build manifest exported successfully
- [ ] Manifest validates as JSON
- [ ] `git_dirty` field is `false` (for releases)
- [ ] `git_commit` matches current HEAD
- [ ] `toolchain` matches expected compiler version
- [ ] `binary_hash` is either SHA-256 hex or error marker
- [ ] Manifest dependencies match lock file/vcpkg versions

## Troubleshooting

### Manifest Shows git_dirty=true

**Cause**: Workspace had uncommitted changes during build

**Solution**: 
- Clean working tree: `git clean -fdx`
- Rebuild from clean state
- Reject dirty builds in CI gates for releases

### Binary Hash Unavailable

**Cause**: OpenSSL library not available at compile time

**Solution**:
- Install OpenSSL: `sudo apt-get install libssl-dev`
- Re-run CMake to pick up OpenSSL
- Rebuild

### Git Metadata Shows "unknown"

**Cause**: Git not found or not in repository during build

**Solution**:
- Install git: `sudo apt-get install git`
- Ensure build runs in git repository
- Re-run CMake to capture git state

## References

- [Build Reproducibility Architecture](../build/BUILD_REPRODUCIBILITY.md)
- [ThemisDB Build System Guide](../build-guide/)
- [Reproducible Builds Organization](https://reproducible-builds.org/)
- [CMake Build Metadata Capture](../cmake/)
