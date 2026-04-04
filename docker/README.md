# ThemisDB Docker Guide

This directory contains Docker assets for building, running, and releasing ThemisDB container images.

## Scope

- Community image build and runtime assets
- Docker Compose files for local development and test setups
- Release tagging and publish conventions
- Edition-specific Docker assets under `community/`, `enterprise/`, and `hyperscaler/`

## Docker Hub Repository

Official public image repository:

- `themisdb/themisdb`

Current release tags:

- `themisdb/themisdb:1.8.1-rc1`
- `themisdb/themisdb:latest`

Both tags currently resolve to the same image digest.

## Quick Start

Pull and run the current image:

```bash
docker pull themisdb/themisdb:latest
docker run --rm -p 8080:8080 themisdb/themisdb:latest
```

Use the explicit release candidate tag:

```bash
docker pull themisdb/themisdb:1.8.1-rc1
docker run --rm -p 8080:8080 themisdb/themisdb:1.8.1-rc1
```

## Local Build (Docker Desktop)

Build locally from repository root:

```bash
docker buildx build --progress=plain -t themis:community-llm-gpu .
```

Retag for Docker Hub release:

```bash
docker tag themis:community-llm-gpu themisdb/themisdb:1.8.1-rc1
docker tag themis:community-llm-gpu themisdb/themisdb:latest
```

Push tags:

```bash
docker push themisdb/themisdb:1.8.1-rc1
docker push themisdb/themisdb:latest
```

Verify remote tags and digest:

```bash
docker buildx imagetools inspect themisdb/themisdb:1.8.1-rc1
docker buildx imagetools inspect themisdb/themisdb:latest
```

## Tagging Policy

- Release candidates: `X.Y.Z-rcN` (example: `1.8.1-rc1`)
- Stable releases: `X.Y.Z`
- `latest` should normally point to the most recent stable release
- If `latest` is intentionally moved to an RC for validation, document it in release notes and CI summaries

## Compose Files

This directory includes multiple compose files for specific scenarios:

- `docker-compose.yml`: baseline setup
- `docker-compose.dev.yml`: development-focused setup
- `docker-compose.test.yml`: testing setup
- `docker-compose.gpu-examples.yml`: GPU-focused examples

Use for example:

```bash
docker compose -f docker-compose.dev.yml up -d --build
```

## Troubleshooting

Authentication error on push:

```text
Username and password required
```

Fix:

```bash
docker login
docker push themisdb/themisdb:1.8.1-rc1
```

Check local tags:

```bash
docker image ls | grep themisdb/themisdb
```

Check manifest availability:

```bash
docker manifest inspect themisdb/themisdb:latest
```

## Related Files

- `DOCKER_BUILD_STRATEGY_QUICKREF.md`
- `DOCKERHUB_README.md`
- `Dockerfile.unified`
- `Dockerfile.themisdb`
- `.github/workflows/04-release_dockerhub-publish-on-release.yml`
