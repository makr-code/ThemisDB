# ThemisDB Docker Guide

This directory contains the repository's Docker assets for builds, local execution, and release-oriented container workflows.

## Canonical build entrypoint

The build entrypoint for Docker Desktop and `docker buildx` is the root [../Dockerfile](../Dockerfile). This file is the authoritative assembly path for local image builds.

The supporting files in this directory are deployment/configuration helpers and compose assets, not the main build file.

## Scope

- root Docker image build for local and CI builds
- cache-aware vcpkg and BuildKit setup
- runtime and compose support for development/test workflows
- edition-specific support files under [community](community), [enterprise](enterprise), and [hyperscaler](hyperscaler)

## Base Image Versioning Strategy

### Primary Dockerfile (Dockerfile.unified)

- **Base image:** `ubuntu:24.04`
- **Rationale:** pinned LTS base improves reproducibility between CI and Docker Desktop
- **Benefit for cross-compilation:** avoids implicit base-image drift between registries/runs
- **Security:** receives Ubuntu LTS security patches via package updates

### Ethics AI Dockerfile (Dockerfile.ethics-ai)

- **Base image:** `python:3.11-slim`
- **Rationale:** Python slim images receive regular patch updates within the major.minor version
- **Benefit for cross-compilation:** Allows automatic Python 3.11.x security patches across platforms
- **Note:** `-slim` is preferred over `-slim-bookworm` to allow flexibility in underlying Debian version

## Local build

From the repository root:

```bash
docker buildx build --progress=plain --load \
  --platform linux/amd64 \
  -f docker/Dockerfile.unified \
  -t themisdb:test \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=OFF \
  --build-arg FORCE_CPU_ONLY=ON \
  .
```

This is the recommended smoke test for validating the root build path with cache-aware BuildKit layers.

### CI parity and optional multi-arch

`release-docker-image.yml` now defaults to `linux/amd64` for faster, more reliable release-consumer builds.  
To reproduce that path locally:

```bash
docker buildx build --progress=plain --load \
  --platform linux/amd64 \
  -f docker/Dockerfile.unified \
  -t themisdb:ci-amd64 \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg FORCE_CPU_ONLY=ON \
  .
```

When you explicitly need a multi-arch image (for example pre-release validation), run:

```bash
docker buildx build --progress=plain --push \
  --platform linux/amd64,linux/arm64 \
  -f docker/Dockerfile.unified \
  -t ghcr.io/<owner>/themisdb:multiarch-test \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg FORCE_CPU_ONLY=ON \
  .
```

## Buildx Builder starten

`docker buildx start` ist kein gültiger Buildx-Befehl. Wenn ein benannter Builder gestoppt ist, starte ihn mit Bootstrap:

```bash
docker buildx ls
docker buildx inspect themisdb-multiarch --bootstrap
```

Falls der Builder noch nicht existiert, ihn zuerst anlegen und dann aktivieren:

```bash
docker buildx create --name themisdb-multiarch --use
docker buildx inspect themisdb-multiarch --bootstrap
```

Damit wird der Docker-Container-Builder gestartet und für `docker buildx build` bereitgestellt.

## Cache-aware build behavior

The current build uses BuildKit cache mounts for:

- apt package cache
- vcpkg downloads
- vcpkg buildtrees
- vcpkg packages

This avoids repeated re-downloads and keeps the Docker build reproducible across rebuilds.

## Important note about stale cache directories

A known failure mode is when a cached vcpkg directory already exists and the build tries to clone into it again:

```text
fatal: destination path '/opt/vcpkg' already exists and is not an empty directory.
```

The root Dockerfile handles this by checking for the repository metadata before cloning and by creating the cache directories before bootstrap.

## Compose files

The docker directory includes compose files for local scenarios, for example:

- [docker-compose.yml](docker-compose.yml)
- [docker-compose.dev.yml](docker-compose.dev.yml)
- [docker-compose.test.yml](docker-compose.test.yml)
- [docker-compose.gpu-examples.yml](docker-compose.gpu-examples.yml)

Example:

```bash
docker compose -f docker-compose.dev.yml up -d --build
```

## Quick reference

See [DOCKER_BUILD_STRATEGY_QUICKREF.md](DOCKER_BUILD_STRATEGY_QUICKREF.md) for the current build strategy summary.

## Related files

- [../Dockerfile](../Dockerfile)
- [DOCKER_BUILD_STRATEGY_QUICKREF.md](DOCKER_BUILD_STRATEGY_QUICKREF.md)
- [DOCKERHUB_README.md](DOCKERHUB_README.md)
- [docker-compose.yml](docker-compose.yml)
- [docker-compose.dev.yml](docker-compose.dev.yml)
- [docker-compose.test.yml](docker-compose.test.yml)
- [docker-compose.gpu-examples.yml](docker-compose.gpu-examples.yml)
