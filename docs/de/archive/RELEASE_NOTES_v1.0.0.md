# ThemisDB v1.0.0 Release Notes

**Release Date:** December 2, 2025

## Overview

ThemisDB v1.0.0 is the first official stable release of ThemisDB - a high-performance, multi-model database combining document, graph, vector, and spatial capabilities with enterprise-grade security and compliance features.

## What's New in v1.0.0

### Core Features
- **Multi-Model Database Engine**: Unified storage for documents, graphs, vectors, and spatial data
- **MVCC Transaction Support**: Full ACID compliance with snapshot isolation
- **Vector Search**: HNSW-based approximate nearest neighbor search with filtering
- **Spatial Indexing**: R-tree based spatial queries with PostGIS-compatible functions
- **Graph Traversal**: Bidirectional graph traversal with path finding algorithms
- **ContentFS**: Binary large object storage with streaming and range requests
- **AQL (Advanced Query Language)**: SQL-like query language with multi-model extensions

### Security & Compliance
- **Field-Level Encryption**: AES-256-GCM encryption with key rotation
- **PII Detection**: Automated detection of personally identifiable information
- **Audit Logging**: Comprehensive audit trail for all database operations
- **Rate Limiting**: Configurable rate limits for API endpoints
- **Policy Engine**: Role-based access control and policy enforcement

### Enterprise Features
- **Backup & Restore**: Point-in-time backup and recovery
- **Metrics & Monitoring**: Prometheus-compatible metrics endpoint
- **Health Checks**: Kubernetes-ready health and readiness probes
- **TLS Support**: Optional TLS encryption for client connections
- **Configurable Retention**: Automatic data retention and cleanup

## Installation

### Docker (Recommended)

Pull the official Docker image from Docker Hub:

```bash
# Standard version (Ubuntu 22.04)
docker pull themisdb/themisdb:1.0.0

# QNAP version (Ubuntu 20.04, glibc 2.31)
docker pull themisdb/themisdb:1.0.0-qnap
```

Run ThemisDB:

```bash
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -v themis-data:/data \
  themisdb/themisdb:1.0.0
```

For detailed Docker deployment instructions, see [docs/guides/guides_deployment.md](docs/guides/guides_deployment.md).

### Binary Packages

Download pre-built binaries for your platform:

| Platform | File | SHA256 Checksum |
|----------|------|------------------|
| **Linux x64** (ZIP) | [themisdb-1.0.0-linux-x64.zip](https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-linux-x64.zip) | `8B075931270487B493F9244738829CF84752D33FF7381B624044C988A00FCC80` |
| **Debian/Ubuntu** | [themisdb_1.0.0_amd64.deb](https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb_1.0.0_amd64.deb) | `D922D21C4D7EAAD2FF18F784E4C447BF8411925B14F582883E4BE22369D4B5C3` |
| **RHEL/CentOS/Fedora** | [themisdb-1.0.0-1.x86_64.rpm](https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-1.x86_64.rpm) | `3CBE36BCF3763F3450286442F00DBDAE0CD29A4F3D9175DF4D77D412C0FD5A7B` |
| **QNAP x64** | [themisdb-1.0.0-qnap-x64.zip](https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-qnap-x64.zip) | `E9BCAD4E654283274CF4C9FB78677C9310A6B7E6E67663B894B26FA869D6D881` |
| **Windows x64** | [themis-1.0.0-windows-x64.zip](https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themis-1.0.0-windows-x64.zip) | `74BC4E0836F1F870B68D4F86E7C34172AC1EBDC9367843D67A6E7380EF00848B` |

#### Debian/Ubuntu Installation (.deb)

```bash
# Download
wget https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb_1.0.0_amd64.deb

# Install
sudo dpkg -i themisdb_1.0.0_amd64.deb

# Start service
sudo systemctl start themisdb
sudo systemctl enable themisdb  # Auto-start on boot

# Check status
sudo systemctl status themisdb
```

The server will run as a systemd service on `http://0.0.0.0:8765`.

**Service Management:**
- Start: `sudo systemctl start themisdb`
- Stop: `sudo systemctl stop themisdb`
- Restart: `sudo systemctl restart themisdb`
- Logs: `sudo journalctl -u themisdb -f`

**Data Location:** `/var/lib/themisdb/data`

#### RHEL/CentOS/Fedora Installation (.rpm)

```bash
# Download
wget https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-1.x86_64.rpm

# Install
sudo rpm -ivh themisdb-1.0.0-1.x86_64.rpm
# Or with dnf/yum:
sudo dnf install themisdb-1.0.0-1.x86_64.rpm

# Start service
sudo systemctl start themisdb
sudo systemctl enable themisdb  # Auto-start on boot

# Check status
sudo systemctl status themisdb
```

The server will run as a systemd service on `http://0.0.0.0:8765`.

**Service Management:**
- Start: `sudo systemctl start themisdb`
- Stop: `sudo systemctl stop themisdb`
- Restart: `sudo systemctl restart themisdb`
- Logs: `sudo journalctl -u themisdb -f`

**Data Location:** `/var/lib/themisdb/data`

#### Linux Binary Installation (ZIP)

```bash
# Download and extract
wget https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-linux-x64.zip
unzip themisdb-1.0.0-linux-x64.zip
cd themisdb-1.0.0-linux-x64

# Make executable and run
chmod +x themis_server
./themis_server
```

The server will start on `http://0.0.0.0:8765` by default.

#### QNAP Installation

For QNAP Container Station, see [QNAP_QUICKSTART.md](QNAP_QUICKSTART.md) for detailed instructions.

Binary installation:
```bash
# Download and extract
wget https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-qnap-x64.zip
unzip themisdb-1.0.0-qnap-x64.zip
cd themisdb-1.0.0-qnap-x64

# Make executable and run
chmod +x themis_server
./themis_server
```

#### Windows Installation

```powershell
# Download and extract themis-1.0.0-windows-x64.zip
# Navigate to extracted directory

# Run the server
.\themis_server.exe
```

The server will start on `http://0.0.0.0:8765` by default.

## Configuration

ThemisDB can be configured via:
- **Environment Variables**: See [docs/configuration.md](docs/configuration.md)
- **Configuration File**: `config/config.json` (optional)
- **Command-Line Flags**: Run `themis_server --help` for options

### Key Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_PORT` | `8765` | HTTP server port |
| `THEMIS_DB_PATH` | `./data/themis_server` | Database storage path |
| `THEMIS_TOKEN_ADMIN` | - | Admin authentication token |
| `THEMIS_ENABLE_TLS` | `false` | Enable TLS encryption |
| `THEMIS_CORS_ALLOW_ALL` | `false` | Allow all CORS origins (dev only) |

## API Endpoints

ThemisDB provides a RESTful HTTP API with the following main endpoints:

- `GET /health` - Health check
- `POST /entities` - Create entity
- `GET /entities/:key` - Retrieve entity
- `PUT /entities/:key` - Update entity
- `DELETE /entities/:key` - Delete entity
- `POST /query` - Execute AQL query
- `POST /vector/search` - Vector similarity search
- `POST /graph/traverse` - Graph traversal
- `POST /transaction` - Execute transaction
- `PUT /contentfs/:pk` - Store binary content
- `GET /contentfs/:pk` - Retrieve binary content

For complete API documentation, see [docs/API.md](docs/API.md).

## Examples

### Create a Document

```bash
curl -X POST http://localhost:8765/entities \
  -H "Content-Type: application/json" \
  -d '{
    "key": "user:alice",
    "data": {
      "name": "Alice",
      "email": "alice@example.com",
      "age": 30
    }
  }'
```

### Execute AQL Query

```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{
    "aql": "SELECT * FROM entities WHERE data.age > 25"
  }'
```

### Vector Search

```bash
curl -X POST http://localhost:8765/vector/search \
  -H "Content-Type: application/json" \
  -d '{
    "vector": [0.1, 0.2, 0.3, ...],
    "k": 10,
    "metric": "cosine"
  }'
```

For more examples, see [examples/](examples/) directory.

## Upgrading

This is the first stable release. Future upgrade instructions will be provided in subsequent release notes.

## Known Issues

- ARM64/Raspberry Pi support is in development and not included in this release
- Vector index auto-save is disabled by default (enable via config)
- Some EPSG coordinate system codes may require manual database initialization

## Performance Benchmarks

Tested on Intel i7-9700K, 32GB RAM, NVMe SSD:

- **Document Insert**: ~50,000 ops/sec
- **Vector Search (1M vectors)**: <5ms p99 latency
- **Graph Traversal**: ~10,000 nodes/sec
- **Spatial Query**: <10ms p99 latency

See [benchmarks/](benchmarks/) for detailed benchmark results.

## Documentation

- **Getting Started**: [README.md](README.md)
- **API Reference**: [docs/API.md](docs/API.md)
- **Configuration**: [docs/configuration.md](docs/configuration.md)
- **Docker Deployment**: [docs/guides/guides_deployment.md](docs/guides/guides_deployment.md)
- **QNAP Setup**: [QNAP_QUICKSTART.md](QNAP_QUICKSTART.md)
- **Security Guide**: [SECURITY.md](SECURITY.md)
- **Contributing**: [CONTRIBUTING.md](CONTRIBUTING.md)

## Support

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Documentation**: https://github.com/makr-code/ThemisDB/tree/main/docs

## License

ThemisDB is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Acknowledgments

ThemisDB is built with the following open-source libraries:
- RocksDB - Persistent key-value store
- Boost - C++ libraries
- nlohmann/json - JSON parsing
- spdlog - Fast logging
- hnswlib - Vector search
- Apache Arrow & Parquet - Columnar storage
- And many more listed in [vcpkg.json](vcpkg.json)

Thank you to all contributors and the open-source community!

---

**Full Changelog**: https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md
