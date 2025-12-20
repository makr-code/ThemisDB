# vcpkg Downloads Cache

This directory serves as a cache for vcpkg source downloads to enable offline builds.

## Offline-First Build Concept

- **With Cache**: Place downloaded vcpkg source packages here (~2GB) for offline builds
- **Without Cache**: Docker will download packages on demand during build (requires internet)

## Populating the Cache

Run the cache update script before building:

```powershell
# Windows
.\scripts\update-vcpkg-cache.ps1

# Linux/macOS
./scripts/update-vcpkg-cache.sh
```

## Cache Structure

When populated, this directory contains:
- Source archives (.tar.gz, .zip)
- Checksum files
- vcpkg asset metadata

## Docker Build

The Dockerfile automatically detects if cache is present:
- **Offline mode**: Uses cached files (faster, no internet needed)
- **Online mode**: Downloads from vcpkg.io (slower, requires internet)

## Best Practices

1. Populate cache before CI/CD builds
2. Update cache periodically to get latest package versions
3. Share cache across builds to save bandwidth
4. Cache is gitignored to avoid bloating repository
