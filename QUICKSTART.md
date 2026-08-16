# ThemisDB — Quick Start

> **Version:** 2.4.0-rc1  
> Get ThemisDB running in minutes. This is the canonical **Step 1** from [README.md](README.md). For full development-environment setup, continue with [SETUP.md](SETUP.md).

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

## Option 0 — Windows Package Manager (WinGet)

The fastest way to install ThemisDB on Windows:

```powershell
winget install ThemisDB.ThemisDB
```

This installs the latest stable Community release as a portable executable.  
After installation `themis_server` is available on the PATH.

```powershell
# Start the server
themis_server --config %LOCALAPPDATA%\ThemisDB\config.yaml

# Upgrade to a newer release
winget upgrade ThemisDB.ThemisDB
```

> **Note:** WinGet packaging is currently under review at [microsoft/winget-pkgs #392825](https://github.com/microsoft/winget-pkgs/pull/392825).  
> Until merged, install via Docker (Option 1) or from source (Option 3).

## Option 1 — Docker (recommended)

### 1. Pull and run

```bash
# Latest stable Community release
docker pull themisdb/themisdb:latest

docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -v themisdb_data:/data \
  themisdb/themisdb:latest

# Pin to a specific version
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -v themisdb_data:/data \
  themisdb/themisdb:2.4.0-rc1
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
# {"status":"ok","version":"2.4.0-rc1"}
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

## Option 1b — Linux Native Packages

For server deployments without Docker:

```bash
# Debian / Ubuntu
wget https://github.com/makr-code/ThemisDB/releases/download/v2.4.0-rc1/themisdb_2.4.0-rc1_amd64.deb
sudo dpkg -i themisdb_2.4.0-rc1_amd64.deb

# RHEL / Fedora
wget https://github.com/makr-code/ThemisDB/releases/download/v2.4.0-rc1/themisdb-2.4.0-rc1-1.x86_64.rpm
sudo rpm -i themisdb-2.4.0-rc1-1.x86_64.rpm

# Generic (TGZ)
wget https://github.com/makr-code/ThemisDB/releases/download/v2.4.0-rc1/themisdb-2.4.0-rc1-Linux-x86_64.tar.gz
tar xzf themisdb-2.4.0-rc1-Linux-x86_64.tar.gz
cd themisdb-2.4.0-rc1-Linux-x86_64
./bin/themis_server --config ./config/config.yaml --data-dir ./data
```

> DEB/RPM packages are built in `.github/workflows/ci-release.yml` (Linux `linux-release` packaging lane) and attached to each GitHub Release.

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
git submodule update --init --recursive
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

The repository uses a pinned `vcpkg` submodule for consistent Windows/Linux/Docker dependency resolution.

### 3. Configure and build (Community edition)

**Linux x64:**
```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

**Windows x64** (aus einer VS Developer Command Prompt):
```bash
cmake --preset windows-release
cmake --build --preset windows-release
```

Available presets (see [`CMakePresets.json`](CMakePresets.json)):

| Preset | Platform | Build type |
|---|---|---|
| `linux-release` | Linux x64 (GCC) | Release |
| `linux-debug` | Linux x64 (GCC) | Debug |
| `linux-arm64-release` | Linux arm64 (GCC cross) | Release |
| `linux-arm64-debug` | Linux arm64 (GCC cross) | Debug |
| `windows-release` | Windows x64 (MSVC) | Release |
| `windows-debug` | Windows x64 (MSVC) | Debug |

Local overrides (custom compiler paths, feature flags) go in `CMakeUserPresets.json` — copy from `CMakeUserPresets.json.example`. That file is gitignored.

### 4. Run the server

```bash
./build/linux-release/themis_server --data-dir ./data
```

### 5. Run the tests

```bash
ctest --preset linux-release --output-on-failure
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
| Performance tuning | [docs/performance/PERFORMANCE_EXPECTATIONS.md](docs/performance/PERFORMANCE_EXPECTATIONS.md) |
| Examples | [examples/](examples/) |
| Full documentation | [docs/Home.md](docs/Home.md) |

---

## Troubleshooting

**Container exits immediately (`Exit 139` / SIGSEGV)**  
Set `enable_response_cache = false`, or move to the current tagged image line documented in [CHANGELOG.md](CHANGELOG.md) / [`VERSION`](VERSION) (`2.4.0-rc1` at this sync point).

**Port already in use**  
Change the host port mapping: `-p 18765:8765`.

**Build fails: missing vcpkg package**  
Run `pwsh ./scripts/setup-third-party.ps1` again; vcpkg bootstrap sometimes requires a second pass.

**More help:** [SUPPORT.md](SUPPORT.md) · [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions) · [FAQ](docs/FAQ.md)

---
Zuletzt geprueft (Root-Sync): 2026-07-27
