---
Author: "ThemisDB Team"
Created: "2026-04-04"
Last Updated: "2026-11-24"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

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

## Ports

| Port  | Purpose |
|-------|---------|
| 8080  | HTTP REST/AQL API, `/health`, `/metrics` |
| 9090  | Health/error service (`/health`, `/errors`) |
| 50051 | gRPC; starts only with `THEMIS_WAL_GRPC_ENABLE_MTLS=true` or `THEMIS_WAL_GRPC_ALLOW_INSECURE=true` |
| 9091 | Bundled Prometheus (opt-in) |
| 3000 | Bundled Grafana (opt-in) |

MCP (`/v1/ai/*`), WebSocket, SSE, GraphQL and the gRPC-Web proxy (`/grpc-web/*`) share the HTTP port 8080. The optional tensor-fragment shard RPC (`sharding.tensor_fragment_rpc.listen_address`, mTLS) needs its own port, e.g. 50052. HTTP/3 is not enabled; ports 18765, 8529 and 5432 are not served.

## Configuration

The image ships a default config at `/etc/themis/config.yaml` and stores data in the volume `/var/lib/themis/data`.
Precedence: CLI flag > environment variable > config file > default.

| Environment | Default | Purpose |
|-------------|---------|---------|
| `THEMIS_CONFIG` | `/etc/themis/config.yaml` | Config file (YAML or JSON) |
| `THEMIS_DATA_DIR` | `/var/lib/themis/data` | RocksDB data directory |
| `THEMIS_HOST` / `THEMIS_PORT` | `0.0.0.0` / `8080` | HTTP listener |
| `THEMIS_WORKER_THREADS` | `0` (auto) | HTTP worker threads |
| `THEMIS_HEALTH_BIND_ADDRESS` / `THEMIS_HEALTH_PORT` | `0.0.0.0` / `9090` | Health/error service |
| `THEMIS_WAL_GRPC_HOST` / `THEMIS_WAL_GRPC_PORT` | `0.0.0.0` / `50051` | gRPC listener (opt-in) |
| `THEMIS_GRPC_WEB_BACKEND` | `127.0.0.1:<gRPC port>` | gRPC-Web proxy target |
| `THEMIS_MONITORING_ENABLED` | `false` | Start bundled Prometheus and Grafana |
| `THEMIS_MONITORING_SCRAPE_PORT` | `THEMIS_PORT` or `8080` | HTTP port Prometheus scrapes locally |
| `GF_SECURITY_ADMIN_PASSWORD` | unset | Required initial Grafana password when monitoring is enabled; alternatively use `GF_SECURITY_ADMIN_PASSWORD__FILE` |

The image includes Prometheus and Grafana from their official images. Enable
them with the environment variables above, publish `9091` and `3000` only to a
trusted interface, and persist `/var/lib/prometheus` and `/var/lib/grafana` in
named volumes. Grafana's initial password is not reset when its existing data
volume is reused. No additional image pulls are needed at container startup.

```bash
docker run -d --name themisdb \
  -p 8080:8080 -p 9090:9090 \
  -v themis_data:/var/lib/themis/data \
  -v "$(pwd)/config.yaml:/etc/themis/config.yaml:ro" \
  themisdb/themisdb:latest
```

Upgrading from older images: data previously defaulted to `/app/data/themis_server`; keep it with `-e THEMIS_DATA_DIR=/app/data/themis_server`. Mounts at `/data` or `/etc/themis` (directory) were never read.

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
