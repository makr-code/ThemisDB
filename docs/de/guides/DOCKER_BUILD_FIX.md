# Docker Build Fix

## Problem
Docker build was failing with the error:
```
ERROR: error from sender: open vcpkg\buildtrees\thrift\src\v0.20.0-8f0d9a10a3.clean\tutorial\swift\swift-dep: The file cannot be accessed by the system.
```

## Root Cause
The local `vcpkg` directory (~83GB) was being included in the Docker build context despite being listed in `.dockerignore`. This is a known issue with Docker Desktop on Windows where certain patterns don't work correctly with deeply nested directories or files with long paths.

## Solution
Created a build script (`docker-build-clean.ps1`) that:
1. Creates a temporary clean build context directory
2. Copies only the necessary source files (excluding vcpkg)
3. Runs the Docker build from the clean context
4. Cleans up the temporary directory

## Usage

### Build with the clean script (Recommended)
```powershell
.\docker-build-clean.ps1 -Target runtime -Tag themisdb:latest -Edition COMMUNITY
```

### Options
- `-Target`: Docker build target (runtime, debug, base, deps, build, llama)
- `-Tag`: Image tag (default: themisdb:latest)
- `-Edition`: ThemisDB edition (MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER)

### Examples
```powershell
# Build production image
.\docker-build-clean.ps1 -Target runtime -Tag themisdb:v1.4.0 -Edition COMMUNITY

# Build debug image
.\docker-build-clean.ps1 -Target debug -Tag themisdb:debug -Edition ENTERPRISE

# Build with minimal features
.\docker-build-clean.ps1 -Target runtime -Tag themisdb:minimal -Edition MINIMAL
```

## What Files Are Copied
The script copies only these directories and files:
- `Dockerfile` and `.dockerignore`
- `CMakeLists.txt`, `VERSION`, `vcpkg-configuration.json`
- `cmake/`, `include/`, `src/`, `proto/`, `internal/`, `docker/`
- `llama.cpp/` (source only, excluding build artifacts and models)

The local `vcpkg/` directory is **not** copied - the Dockerfile clones a fresh vcpkg installation inside the container.

## Alternative: Manual Cleanup
If you don't want to use the script, you can manually remove the problematic directory before building:

```powershell
# This will take a long time due to the size
Remove-Item vcpkg\buildtrees -Recurse -Force

# Then build normally
docker build . --target=runtime --tag=themisdb:latest
```

## Notes
- The Dockerfile clones vcpkg fresh, so the local copy isn't needed for the build
- Build context size reduced from 83GB to ~84MB
- Total build time: ~15-30 minutes (depending on network and CPU)
