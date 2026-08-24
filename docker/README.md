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

## Local build

From the repository root:

```bash
docker buildx build --progress=plain --load \
  -f Dockerfile \
  -t themisdb:test \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=OFF \
  --build-arg ENABLE_GPU=OFF \
  --build-arg BUILD_TESTS=OFF \
  --build-arg BUILD_BENCHMARKS=OFF \
  .
```

This is the recommended smoke test for validating the root build path with cache-aware BuildKit layers.

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
