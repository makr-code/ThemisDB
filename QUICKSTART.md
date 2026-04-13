# ThemisDB — Quick Start

> **Version:** 1.8.x  
> Get ThemisDB running in minutes. For a full development-environment setup see [SETUP.md](SETUP.md).

---

## Prerequisites

| Tool | Minimum version | Notes |
|---|---|---|
| Docker | 24.x | Recommended for fastest start |
| CMake | 3.20 | Required for building from source |
| C++ compiler | GCC 12 / Clang 16 / MSVC 17 | C++20 required |
| Python | 3.8+ | For tooling scripts |
| Git | 2.x | |

---

## Option 1 — Docker (recommended)

### 1. Pull and run

```bash
docker pull ghcr.io/makr-code/themisdb:latest
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -p 8766:8766 \
  -v themis-data:/data \
  ghcr.io/makr-code/themisdb:latest
```

| Port | Protocol |
|---|---|
| `8765` | HTTP/REST API + WebSocket |
| `8766` | ThemisDB Wire Protocol V2 (mTLS) |
| `8770` | QUIC/HTTP3 transport (optional) |
| `8771` | gRPC (optional) |

### 2. Verify the server is healthy

```bash
curl http://localhost:8765/health
# {"status":"ok","version":"1.8.x"}
```

### 3. Run your first query (AQL)

```bash
curl -X POST http://localhost:8765/v2/query \
  -H 'Content-Type: application/json' \
  -d '{"query": "SELECT 1 AS hello"}'
```

### 4. Docker Compose (with Prometheus + Grafana)

```bash
docker compose up -d
```

The bundled [`docker-compose.yml`](docker-compose.yml) starts ThemisDB, Prometheus, and Grafana with pre-configured dashboards.

---

## Option 2 — Dev Container (VS Code)

1. Install **Docker Desktop** and the **Dev Containers** VS Code extension.
2. Clone the repository and open it in VS Code.
3. Press `Ctrl+Shift+P` → **Dev Containers: Reopen in Container**.

The container installs all toolchains and dependencies automatically.

---

## Option 3 — Build from Source

### 1. Clone

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

### 2. Install pre-commit hooks and bootstrap third-party dependencies

```bash
# Linux / macOS
./scripts/setup-pre-commit.sh
pwsh ./scripts/setup-third-party.ps1   # requires PowerShell 7+

# Windows (PowerShell)
.\scripts\setup-pre-commit.ps1
.\scripts\setup-third-party.ps1
```

This installs vcpkg dependencies, llama.cpp, whisper.cpp, and ffmpeg into the repository tree.

### 3. Configure and build (Community edition)

```bash
cmake --preset community-release
cmake --build --preset community-release --parallel
```

Available presets (see [`CMakePresets.json`](CMakePresets.json)):

| Preset | Edition | Build type |
|---|---|---|
| `community-release` | COMMUNITY | Release |
| `community-debug` | COMMUNITY | Debug |
| `minimal-release` | MINIMAL | Release |

### 4. Run the server

```bash
./build/community-release/themisdb_server --data-dir ./data
```

### 5. Run the tests

```bash
ctest --preset community-release --output-on-failure
```

---

## First Steps

### Create a collection and insert a document

```bash
curl -X POST http://localhost:8765/v2/collections \
  -H 'Content-Type: application/json' \
  -d '{"name": "products", "schema": {}}'

curl -X POST http://localhost:8765/v2/collections/products/documents \
  -H 'Content-Type: application/json' \
  -d '{"name": "Widget A", "price": 9.99, "tags": ["sale"]}'
```

### Query with AQL

```bash
curl -X POST http://localhost:8765/v2/query \
  -H 'Content-Type: application/json' \
  -d '{"query": "FOR doc IN products FILTER doc.price < 10 RETURN doc"}'
```

### Vector search

```bash
curl -X POST http://localhost:8765/v2/search/vector \
  -H 'Content-Type: application/json' \
  -d '{"collection": "products", "vector": [0.1, 0.2, 0.3], "top_k": 5}'
```

---

## Next Steps

| Topic | Resource |
|---|---|
| Full API reference | [docs/api/API_REFERENCE.md](docs/api/API_REFERENCE.md) |
| AQL language guide | [aql/](aql/) |
| Architecture | [ARCHITECTURE.md](ARCHITECTURE.md) |
| Configuration | [config/](config/) |
| Security hardening | [SECURITY.md](SECURITY.md) |
| Production deployment | [docs/de/guides/guides_deployment.md](docs/de/guides/guides_deployment.md) |
| Performance tuning | [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) |
| Examples | [examples/](examples/) |
| Full documentation | [docs/Home.md](docs/Home.md) |

---

## Troubleshooting

**Container exits immediately (`Exit 139` / SIGSEGV)**  
Set `enable_response_cache = false` in your config, or use image tag `≥ 1.8.1-rc2`. See [CHANGELOG.md](CHANGELOG.md) for details.

**Port already in use**  
Change the host port mapping: `-p 18765:8765`.

**Build fails: missing vcpkg package**  
Run `pwsh ./scripts/setup-third-party.ps1` again; vcpkg bootstrap sometimes requires a second pass.

**More help:** [SUPPORT.md](SUPPORT.md) · [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions) · [FAQ](docs/FAQ.md)
