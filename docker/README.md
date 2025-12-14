# ThemisDB - Official Docker Image

[![Docker Hub](https://img.shields.io/docker/pulls/themisdb/themisdb.svg)](https://hub.docker.com/r/themisdb/themisdb)
[![Docker Image Size](https://img.shields.io/docker/image-size/themisdb/themisdb/latest)](https://hub.docker.com/r/themisdb/themisdb)
[![Docker Image Version](https://img.shields.io/docker/v/themisdb/themisdb/latest)](https://hub.docker.com/r/themisdb/themisdb)

ThemisDB is a high-performance, multi-model database system built on LSM Tree architecture with native support for vector search, graph operations, geospatial queries, and full-text search.

**Current Version:** v1.0.2 (December 2025)  
**Registry:** `docker.io/themisdb/themisdb`

---

## Quick Start

```bash
# Pull the latest image
docker pull themisdb/themisdb:latest

# Run ThemisDB
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Verify it's running
curl http://localhost:18765/health
```

**Ports:**
- `8080` - REST API & Web UI
- `18765` - Binary protocol

**Volume:**
- `/data` - Database storage (persist this!)

---

## Supported Tags

| Tag | Architecture | Base Image | Use Case |
|-----|--------------|------------|----------|
| `latest` | amd64, arm64 | Ubuntu 22.04 | **Recommended** - Always latest stable |
| `v1.0.2` | amd64, arm64 | Ubuntu 22.04 | Stable patch release (Dec 2025) |
| `v1.0` | amd64, arm64 | Ubuntu 22.04 | Minor version track |
| `qnap` | amd64 | Ubuntu 20.04 | **QNAP NAS** optimized (SSE4.2 baseline) |
| `v1.0.2-qnap` | amd64 | Ubuntu 20.04 | QNAP v1.0.2 release |

**Multi-Architecture Support:**
- `linux/amd64` - Intel/AMD x64 processors
- `linux/arm64` - ARM v8 (Raspberry Pi, Apple Silicon, AWS Graviton)

Docker automatically selects the correct architecture for your platform.

---

## Features

✅ **Multi-Model Database**
- Key-Value Store
- Document Store (JSON)
- Vector Search (embeddings, similarity)
- Graph Database (vertices, edges, traversal)
- Geospatial (points, polygons, spatial queries)
- Full-Text Search

✅ **Enterprise Features**
- ACID transactions
- Horizontal scaling & sharding
- Multi-master replication
- GPU acceleration (CUDA, Vulkan, ROCm)
- Real-time analytics (CEP, OLAP)
- Client SDKs (Python, JavaScript, Rust, Go, Java, C#, Swift)

✅ **Production-Ready**
- ~150MB compressed image size
- Health checks built-in
- OpenTelemetry instrumentation
- DSGVO/GDPR compliance features
- Automated backups & recovery

---

## Docker Compose

### Basic Setup

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:latest
    container_name: themis
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_data:/data
      - ./config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: 18765
      THEMIS_CONFIG_PATH: /etc/themis/config.json
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:18765/health"]
      interval: 30s
      timeout: 5s
      retries: 3

volumes:
  themis_data:
```

**Start:**
```bash
docker-compose up -d
docker-compose logs -f themis
```

### Production Setup with Monitoring

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:latest
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_data:/data
    deploy:
      resources:
        limits:
          cpus: '4'
          memory: 8G
        reservations:
          cpus: '2'
          memory: 4G
    restart: unless-stopped
    networks:
      - themis_net

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus_data:/prometheus
    networks:
      - themis_net

networks:
  themis_net:
    driver: bridge

volumes:
  themis_data:
  prometheus_data:
```

---

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_CONFIG_PATH` | `/etc/themis/config.json` | Configuration file path |
| `THEMIS_PORT` | `18765` | Internal server port |
| `LD_LIBRARY_PATH` | `/usr/local/lib/themisdb:/usr/local/lib` | Runtime library path |

### Custom Configuration

```bash
# Mount your own config
docker run -d \
  -v /path/to/config.json:/etc/themis/config.json:ro \
  themisdb/themisdb:latest
```

**Config Example:**
```json
{
  "server": {
    "port": 18765,
    "max_connections": 1000
  },
  "storage": {
    "data_dir": "/data",
    "cache_size_mb": 512
  },
  "features": {
    "vector_search": true,
    "graph_engine": true,
    "geo_spatial": true
  }
}
```

---

## Platform-Specific Usage

### QNAP NAS

```bash
# Pull QNAP-optimized image
docker pull themisdb/themisdb:qnap

# Run (using QNAP default port 18765)
docker run -d \
  --name themis \
  -p 18765:18765 \
  -v /share/Container/themis/data:/data \
  themisdb/themisdb:qnap
```

**QNAP Notes:**
- Uses Ubuntu 20.04 base (GLIBC 2.31)
- SSE4.2 CPU baseline (broader compatibility)
- Optimized for x86_64 QNAP NAS devices
- Requires QTS 5.0+ or QuTS hero h5.0+

### Raspberry Pi / ARM

```bash
# Auto-selects ARM64 image
docker pull themisdb/themisdb:latest
docker run -d -p 8080:8080 -p 18765:18765 -v themis_data:/data themisdb/themisdb:latest
```

### macOS (Docker Desktop)

```bash
# Works on both Intel and Apple Silicon
docker pull themisdb/themisdb:latest
docker run -d -p 8080:8080 -p 18765:18765 -v themis_data:/data themisdb/themisdb:latest
```

---

## Volumes & Persistence

### Data Volume

```bash
# Create named volume
docker volume create themis_data

# Run with volume
docker run -d -v themis_data:/data themisdb/themisdb:latest

# Backup volume
docker run --rm -v themis_data:/data -v $(pwd):/backup \
  ubuntu tar czf /backup/themis_backup.tar.gz /data
```

### Important Directories

| Path | Purpose | Mount |
|------|---------|-------|
| `/data` | Database files | ✅ Required |
| `/etc/themis` | Configuration | Optional |
| `/var/log/themis` | Application logs | Optional |

---

## Health Checks & Monitoring

### Built-in Health Check

```bash
# Check container health
docker inspect --format='{{.State.Health.Status}}' themis

# View health logs
docker inspect --format='{{json .State.Health}}' themis | jq
```

### Health Endpoint

```bash
curl http://localhost:18765/health
# Response: {"status":"ok","uptime":3600,"version":"1.0.2"}
```

### Resource Monitoring

```bash
# Live stats
docker stats themis

# Detailed resource usage
docker inspect themis | jq '.[0].HostConfig.Memory'
```

---

## Troubleshooting

### Container Won't Start

```bash
# Check logs
docker logs themis

# Common fixes:
# 1. Port conflict - change port mapping
docker run -p 8081:8080 -p 18766:18765 ...

# 2. Permission issue - check volume permissions
docker run --user 0 ...  # Run as root temporarily

# 3. Resource limits - increase memory
docker run --memory 4g ...
```

### Performance Issues

```bash
# Increase resources
docker update --memory 8g --cpus 4 themis
docker restart themis

# Check resource usage
docker stats themis

# Optimize for production
docker run -d \
  --cpus="4" \
  --memory="8g" \
  --memory-swap="10g" \
  themisdb/themisdb:latest
```

### Library Path Issues

```bash
# Verify libraries
docker exec themis ldd /usr/local/bin/themis_server

# Check environment
docker exec themis printenv LD_LIBRARY_PATH
```

---

## Advanced Usage

### Resource Limits

```bash
docker run -d \
  --cpus="4" \              # 4 CPU cores
  --memory="8g" \           # 8GB RAM
  --memory-swap="10g" \     # 10GB total with swap
  --pids-limit=1000 \       # Process limit
  themisdb/themisdb:latest
```

### Logging Configuration

```bash
# JSON file driver with rotation
docker run -d \
  --log-driver json-file \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  themisdb/themisdb:latest

# Syslog driver
docker run -d \
  --log-driver syslog \
  --log-opt syslog-address=udp://localhost:514 \
  themisdb/themisdb:latest
```

### Network Modes

```bash
# Host network (better performance, less isolation)
docker run -d --network host themisdb/themisdb:latest

# Custom bridge network
docker network create themis_net
docker run -d --network themis_net themisdb/themisdb:latest
```

---

## Build Your Own Image

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build standard image
docker build -t themis:custom .

# Build QNAP-optimized image
docker build -f docker/Dockerfile.qnap -t themis:qnap .

# Multi-architecture build (requires buildx)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis:multiarch \
  --push .
```

---

## Support & Resources

**Documentation:**
- [Full Documentation](https://github.com/makr-code/ThemisDB)
- [Docker Deployment Guide](https://github.com/makr-code/ThemisDB/blob/main/DOCKER_DEPLOYMENT.md)
- [API Reference](https://github.com/makr-code/ThemisDB/tree/main/openapi)

**Get Help:**
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions

**Source Code:**
- Repository: https://github.com/makr-code/ThemisDB
- License: [See LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)

---

## Image Details

**Image Size:** ~150MB (compressed), ~400MB (uncompressed)

**Build Info:**
- Builder: Docker Buildx (multi-stage)
- Compiler: GCC 11+ / Clang 14+
- C++ Standard: C++20
- vcpkg: Package manager for dependencies

**Security:**
- Non-root user by default
- Minimal attack surface
- Regular security updates
- SBOM (Software Bill of Materials) provided

---

## License

ThemisDB is released under the terms specified in the [LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE) file.

---

**Last Updated:** December 14, 2025  
**Maintainer:** ThemisDB Team  
**Docker Hub:** https://hub.docker.com/r/themisdb/themisdb
