---
Author: "ThemisDB Team"
Created: "2025-12-14"
Last Updated: "2026-11-24"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Docker Guide

This directory contains the repository's Docker assets for builds, local execution, and release-oriented container workflows.

## Canonical build entrypoint

The release workflows build [Dockerfile.unified](Dockerfile.unified). The root
[../Dockerfile](../Dockerfile) is an alternative local build entrypoint.

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

## Runtime contract

Applies to images built from [Dockerfile.unified](Dockerfile.unified) (published image) and [../Dockerfile](../Dockerfile).

| Port  | Purpose | Notes |
|-------|---------|-------|
| 8080  | HTTP REST/AQL API, `/health`, `/metrics` | `THEMIS_PORT` / `server.port` |
| 9090  | Health/error service (`/health`, `/health/components`, `/errors`) | `THEMIS_HEALTH_PORT`, bound to `0.0.0.0` via `THEMIS_HEALTH_BIND_ADDRESS` |
| 50051 | gRPC (WAL shipping + ThemisDB API) | Starts only with `THEMIS_WAL_GRPC_ENABLE_MTLS=true` (+ cert/key/CA paths) or `THEMIS_WAL_GRPC_ALLOW_INSECURE=true` (dev only); `THEMIS_WAL_GRPC_HOST` / `THEMIS_WAL_GRPC_PORT` |
| 9091 | Bundled Prometheus UI/API | Opt-in monitoring only |
| 3000 | Bundled Grafana UI | Opt-in monitoring only |

Sub-servers and protocols:

| Component | Listener | Notes |
|-----------|----------|-------|
| MCP (`/v1/ai/*`, SSE/WebSocket transports) | HTTP port (8080) | No separate MCP port; stdio transport needs no port |
| WebSocket, SSE, GraphQL (`/graphql`) | HTTP port (8080) | – |
| gRPC-Web proxy (`POST /grpc-web/*`) | HTTP port (8080) | Forwards to `127.0.0.1:<THEMIS_WAL_GRPC_PORT>`; override with `THEMIS_GRPC_WEB_BACKEND`. Uses a plaintext channel, so it cannot reach an mTLS-only gRPC server |
| Tensor-fragment shard RPC | `sharding.tensor_fragment_rpc.listen_address` | Opt-in, mTLS required, no default; use a port other than 50051 (e.g. `0.0.0.0:50052`) and publish it yourself |
| HTTP/3 (QUIC) | – | Not enabled by `themis_server` |
| OTLP tracing (4318) | outbound | Client to a collector, not a listener |

Ports `18765`, `8765`, `8529` and `5432` are not served by the container; older compose files and docs that map them are obsolete.

| Path | Purpose |
|------|---------|
| `/etc/themis/config.yaml` | Default config ([config/config.yaml](config/config.yaml)); mount your own file read-only to replace it |
| `/var/lib/themis/data` | RocksDB data (`VOLUME`); `THEMIS_DATA_DIR` / `storage.rocksdb_path` |
| `/logs` | Audit logs (`audit.jsonl`, `retention_audit.jsonl`) of the published image |

Settings precedence: **CLI flag > environment variable > config file > built-in default**.

| Setting | CLI | Environment | Config key |
|---------|-----|-------------|-----------|
| Config file | `--config` | `THEMIS_CONFIG` | – |
| Bind host | `--host` | `THEMIS_HOST` | `server.host` |
| HTTP port | `--port` | `THEMIS_PORT` | `server.port` |
| Data directory | `--data-dir` | `THEMIS_DATA_DIR` | `storage.rocksdb_path` |
| Worker threads | `--threads` | `THEMIS_WORKER_THREADS` | `server.worker_threads` |

An explicitly requested config file (`--config`/`THEMIS_CONFIG`) that cannot be read, or an invalid port/thread value, aborts startup. The image `CMD` is empty, so no CLI flag silently overrides your environment. The container healthcheck probes `http://127.0.0.1:${THEMIS_PORT:-8080}/health`; set `THEMIS_PORT` (not only `server.port`) when changing the port.

```bash
docker run -d --name themisdb \
  -p 8080:8080 -p 9090:9090 \
  -v themis_data:/var/lib/themis/data \
  -v "$(pwd)/config.yaml:/etc/themis/config.yaml:ro" \
  makrcode/themisdb:latest
```

Migration from images before this contract: data previously lived in `/app/data/themis_server` (not a volume). To keep an existing mount, set `-e THEMIS_DATA_DIR=/app/data/themis_server`. Mounts at `/data` or `/etc/themis/config/` were never read; move them to `/var/lib/themis/data` and `/etc/themis/config.yaml`.

### Bundled monitoring

The runtime images include Prometheus 3.14.0 and Grafana 13.3.0 copied at build
time from digest-pinned official `prom/prometheus` and `grafana/grafana` images.
They increase image size but need no Docker daemon or additional pulls at
runtime. The `debug` target does not include them. Monitoring is disabled by
default; the existing separate-container Compose deployments remain unchanged.
Do not publish the same host ports for bundled and separate monitoring services.
Their upstream license files are included at `/usr/share/licenses/prometheus/`
and `/usr/share/grafana/LICENSE`; Grafana's source is available from
https://github.com/grafana/grafana.
Grafana usage reporting, update checks, and plugin preinstallation are disabled
by default; configure Grafana's `GF_` environment variables to opt in.

To enable both services, provide a non-empty Grafana admin password. Publish
their ports on loopback or protect them with your own authenticated proxy;
Prometheus itself has no authentication. Grafana's default user is `admin`.

```bash
export GRAFANA_ADMIN_PASSWORD='<choose-a-strong-password>'
docker run -d --name themisdb \
  -p 8080:8080 -p 9090:9090 \
  -p 127.0.0.1:9091:9091 -p 127.0.0.1:3000:3000 \
  -e THEMIS_MONITORING_ENABLED=true \
  -e GF_SECURITY_ADMIN_PASSWORD="$GRAFANA_ADMIN_PASSWORD" \
  -v themis_data:/var/lib/themis/data \
  -v themis_prometheus:/var/lib/prometheus \
  -v themis_grafana:/var/lib/grafana \
  themisdb/themisdb:latest
```

Alternatively use `GF_SECURITY_ADMIN_PASSWORD__FILE` with a readable mounted
secret. Grafana applies the initial admin password only when its database is
created; changing the environment does not reset an existing password. Named
volumes preserve metrics and Grafana state across container replacement; existing
mounts must be writable by the image's `themis` user.
Prometheus scrapes `127.0.0.1:${THEMIS_PORT:-8080}`; set
`THEMIS_MONITORING_SCRAPE_PORT` when an HTTP port is supplied through CLI or
config instead of `THEMIS_PORT`. The HTTP listener must accept connections on
loopback (`THEMIS_HOST=0.0.0.0` or `127.0.0.1`). Grafana provisions the local Prometheus
datasource and the bundled LoRA dashboard. If any of the three processes exits,
the entrypoint stops the others and exits; monitor the container and configure
a restart policy as needed. The container healthcheck still checks ThemisDB
only. This configuration is intended for single-container deployments;
separate services remain preferable for independent scaling and security.

Release and nightly Docker workflows validate the bundled monitoring stack on
both `linux/amd64` and `linux/arm64` before promoting the final image tags.
The test checks scraping, Grafana's provisioned datasource and dashboard, and
process shutdown and failure handling. Release builds of older tags without
bundled monitoring fail this gate; use a release source containing the change.

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
