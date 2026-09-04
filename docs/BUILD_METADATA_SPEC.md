# build-metadata.json Schema

**Version:** 1.0  
**Last Updated:** 2026-09-04

This document defines the structure and meaning of `build-metadata.json`, which is generated automatically for every ThemisDB release build.

## Purpose

The `build-metadata.json` file provides machine-readable metadata about a build, enabling:

- **Release traceability**: link releases back to source commits and CI/CD runs
- **Artifact verification**: track build timestamps, build numbers, and release types
- **Registry consistency**: validate that all registries (GitHub, Docker, WinGet) have matching metadata
- **Reproducible builds**: capture exact build conditions and outputs

## Schema

```json
{
  "build": {
    "timestamp": "string (ISO 8601)",
    "url": "string (GitHub Actions run URL)",
    "number": "number (GitHub run number)",
    "id": "string (GitHub run ID)"
  },
  "version": {
    "version": "string (SemVer: X.Y.Z or X.Y.Z-prerelease)",
    "release_type": "string (nightly|alpha|beta|rc|stable)"
  },
  "git": {
    "commit": "string (full git SHA-1)",
    "commit_short": "string (7-char git SHA-1 prefix)",
    "branch": "string (git branch name)"
  },
  "sbom_hash": "string (SHA-256 hash of SBOM if available, empty string otherwise)"
}
```

## Field Definitions

### `build`

Metadata about the GitHub Actions build run.

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | ISO 8601 | Build completion time (UTC) |
| `url` | URL | Link to GitHub Actions run details |
| `number` | integer | `${{ github.run_number }}` |
| `id` | string | `${{ github.run_id }}` |

### `version`

Release version and type.

| Field | Type | Description |
|-------|------|-------------|
| `version` | SemVer | Semantic version from `VERSION` file |
| `release_type` | enum | Release type from `RELEASE_TYPE` file: `nightly`, `alpha`, `beta`, `rc`, `stable` |

### `git`

Git repository state at build time.

| Field | Type | Description |
|-------|------|-------------|
| `commit` | SHA-1 | Full git commit hash (40 chars) |
| `commit_short` | SHA-1 | Abbreviated commit hash (7 chars, matches `git rev-parse --short HEAD`) |
| `branch` | string | Git branch name (e.g., `develop`, `community`, `release/v2.4.0`) |

### `sbom_hash`

Hash of the Software Bill of Materials (SBOM) in CycloneDX JSON format, if present.

| Field | Type | Description |
|-------|------|-------------|
| `sbom_hash` | SHA-256 hex | SHA-256 of `build-metadata.sbom.cdx.json` if available; empty string otherwise |

## Example

```json
{
  "build": {
    "timestamp": "2026-09-04T15:32:45Z",
    "url": "https://github.com/makr-code/ThemisDB/actions/runs/9876543210",
    "number": 1234,
    "id": "9876543210"
  },
  "version": {
    "version": "2.4.0-rc1",
    "release_type": "rc"
  },
  "git": {
    "commit": "abc1234567890def1234567890abc1234567890d",
    "commit_short": "abc1234",
    "branch": "release/v2.4.0"
  },
  "sbom_hash": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
}
```

## Validation

Each build metadata must satisfy:

- `version` matches the version tag pattern (SemVer 2.0.0)
- `release_type` is one of the allowed values
- `commit` is a valid git SHA-1 (40 hex characters)
- `timestamp` is valid ISO 8601
- `url` is a valid HTTPS URL pointing to GitHub Actions

## Storage & Distribution

### GitHub Releases

- Stored as artifact: `build-metadata.json` (in release assets)
- Also: `build-metadata.sbom.cdx.json` (SBOM artifact)

### Docker Images

Embedded in OCI annotations / Docker labels:

```dockerfile
LABEL org.opencontainers.image.version="2.4.0-rc1"
LABEL org.opencontainers.image.revision="abc1234567890def1234567890abc1234567890d"
LABEL org.opencontainers.image.created="2026-09-04T15:32:45Z"
LABEL dev.themisdb.release_type="rc"
LABEL dev.themisdb.github_run="1234"
```

Query labels from image:

```bash
docker inspect themisdb:2.4.0-rc1 | jq '.[0].ContainerConfig.Labels'
```

### WinGet Manifests

Stored in manifest installer notes:

```yaml
Installers:
  - Architecture: x64
    InstallerUrl: https://github.com/makr-code/ThemisDB/releases/download/v2.4.0-rc1/themisdb-2.4.0-rc1-windows-binary-x64.zip
    InstallerSha256: "..."
    ReleaseNotes: |
      Build: #1234 (2026-09-04T15:32:45Z)
      Commit: abc1234567890def1234567890abc1234567890d
      Type: rc
```

## Related Documents

- [VERSIONING.md](../../VERSIONING.md) — Version format and release types
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) — Release process
- [.github/actions/generate-build-metadata/action.yml](.github/actions/generate-build-metadata/action.yml) — Generator implementation
