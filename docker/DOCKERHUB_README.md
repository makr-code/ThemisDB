# ThemisDB Docker Image

Official Docker image for ThemisDB.

- Docker Hub: https://hub.docker.com/r/themisdb/themisdb
- Repository: https://github.com/makr-code/ThemisDB

## Supported Tags

- `latest`
- `1.8.1-rc1`

Current state: `latest` and `1.8.1-rc1` point to the same image digest.

## Quick Start

```bash
docker pull themisdb/themisdb:latest
docker run --rm -p 8080:8080 themisdb/themisdb:latest
```

Use explicit release candidate:

```bash
docker pull themisdb/themisdb:1.8.1-rc1
docker run --rm -p 8080:8080 themisdb/themisdb:1.8.1-rc1
```

## Configuration

The container starts `themis_server` and expects runtime config via mounted files and/or environment settings, depending on your deployment profile.

Typical pattern:

```bash
docker run --rm \
  -p 8080:8080 \
  -v $(pwd)/config:/etc/themis \
  -v themis_data:/data \
  themisdb/themisdb:latest
```

## Changelog (Docker Image)

### 1.8.1-rc1 (2026-04-04)

- Geo hardening and RFC 7946 completion improvements integrated.
- Search and storage stabilization updates included.
- Release candidate published as `themisdb/themisdb:1.8.1-rc1`.
- `latest` currently aligned to this RC image for validation.

### latest

- Moving tag for the currently designated default image.
- At the moment, it resolves to `1.8.1-rc1`.

For full release details, see:

- `docs/de/releases/RELEASE_NOTES_v1.8.1-rc1.md`
- `CHANGELOG.md`

## Support

- Issues: https://github.com/makr-code/ThemisDB/issues
- Discussions: https://github.com/makr-code/ThemisDB/discussions
