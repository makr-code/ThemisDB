# ThemisDB Docker Deployment Guide

**Version:** 1.3.0  
**Last Updated:** 17. Dezember 2025  
**Status:** Production-Ready

## Quick Start

### Pull & Run (Docker Hub)

```bash
# Latest version
docker pull themisdb/themisdb:latest
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Specific version (v1.3.0 - LLM Integration)
docker pull themisdb/themisdb:v1.3.0
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:v1.3.0
```

### Quick Start with GPU Support (v1.3.0+)

**NEW in v1.3.0:** Native LLM inference with GPU acceleration

```bash
# Pull GPU-enabled image
docker pull themisdb/themisdb:v1.3.0-gpu

# Run with NVIDIA GPU support
docker run -d \
  --name themis-gpu \
  --gpus all \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  -v themis_models:/models \
  themisdb/themisdb:v1.3.0-gpu

# Verify GPU access
docker exec themis-gpu nvidia-smi
```

**Requirements:**
- NVIDIA GPU with CUDA support
- [NVIDIA Container Toolkit](https://github.com/NVIDIA/nvidia-docker) installed
- Docker 19.03+ with GPU support

**Performance:** 100x faster inference vs CPU-only mode


### Verify Running

```bash
# Check container status
docker ps | grep themis

# Health check
curl http://localhost:18765/health
# Expected: 200 OK with JSON response

# View logs
docker logs -f themis
```

---

## Docker Images

### Available Tags

| Tag | Architecture | Status | Use Case |
|-----|--------------|--------|----------|
| `latest` | amd64 + arm64 | ✅ Production | Recommended for most users (v1.3.0) |
| `v1.3.0` | amd64 + arm64 | ✅ Production | LLM Integration Release (December 2025) |
| `v1.3.0-gpu` | amd64 | ✅ Production | With CUDA support for GPU acceleration |
| `qnap` | amd64 | ✅ Production | QNAP NAS optimized (Ubuntu 20.04, SSE4.2 baseline) |
| `v1.2.0` | amd64 + arm64 | ✅ Production | Previous stable release |
| `v1.0.2` | amd64 + arm64 | ✅ Production | Legacy stable release |
| `v1` | amd64 + arm64 | ✅ Production | Major version track |

### Image Specs

**Registry:** `docker.io/themisdb/themisdb`

**Multi-Architecture Support:**
- `linux/amd64` - Intel/AMD x64 processors
- `linux/arm64` - ARM v8 processors (RPi, Apple Silicon, AWS Graviton, etc.)

**Image Size:** ~150MB (compressed)

**Build Configuration:**
```dockerfile
FROM ubuntu:22.04
VCPKG_ENABLE_ONLINE=OFF          # No internet access during build
VCPKG_TRIPLET=x64-linux          # For amd64
VCPKG_TRIPLET=arm64-linux        # For arm64
```

---

## Configuration

### Environment Variables

```bash
docker run -e THEMIS_CONFIG_PATH=/etc/themis/config.json \
           -e THEMIS_PORT=18765 \
           -e LD_LIBRARY_PATH=/usr/local/lib/themisdb:/usr/local/lib \
           themisdb/themisdb:latest
```

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_CONFIG_PATH` | `/etc/themis/config.json` | Configuration file path |
| `THEMIS_PORT` | `18765` | Internal port (mapped via -p) |
| `LD_LIBRARY_PATH` | `/usr/local/lib/themisdb:/usr/local/lib` | Runtime library path |

### Custom Config

```bash
# Mount custom config
docker run -d \
  -v /path/to/config.json:/etc/themis/config.json:ro \
  themisdb/themisdb:latest
```

---

## Volumes

### Data Persistence

```bash
# Named volume (recommended)
docker volume create themis_data
docker run -d \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Bind mount (for development)
docker run -d \
  -v /local/data/path:/data \
  themisdb/themisdb:latest
```

### Directories in Container

| Path | Purpose | Persistence |
|------|---------|-------------|
| `/data` | Database storage | ✅ Volume |
| `/etc/themis` | Configuration | ✅ Config mount |
| `/var/log/themis` | Application logs | ✅ Volume |

---

## Networking

### Port Mapping

```bash
docker run -d \
  -p 8080:8080 \                # REST API (HTTP)
  -p 18765:18765 \              # Internal protocol
  themisdb/themisdb:latest
```

| Port | Protocol | Purpose | Default |
|------|----------|---------|---------|
| `8080` | HTTP | REST API, Web UI | Required |
| `18765` | Custom | Binary protocol | Required |

### Network Modes

```bash
# Host network (performance, less isolation)
docker run --network host themisdb/themisdb:latest

# Bridge network (default, recommended)
docker run --network bridge themisdb/themisdb:latest

# Custom network
docker network create themis_net
docker run --network themis_net themisdb/themisdb:latest
```

---

## Docker Compose

### Basic Setup

```yaml
# docker-compose.yml
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
      - ./config/config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: 18765
      THEMIS_CONFIG_PATH: /etc/themis/config.json
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:18765/health"]
      interval: 10s
      timeout: 5s
      retries: 3

volumes:
  themis_data:
    driver: local
```

**Start:**
```bash
docker-compose up -d
docker-compose logs -f
```

### Multi-Service Stack

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
    networks:
      - themis_net

  # Optional: monitoring
  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
    networks:
      - themis_net

networks:
  themis_net:
    driver: bridge

volumes:
  themis_data:
```

---

## Production Deployment

### Resource Allocation

```bash
docker run -d \
  --cpus="4" \                          # 4 CPU cores
  --memory="8g" \                       # 8GB RAM
  --memory-swap="10g" \                 # 10GB with swap
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### Restart Policies

```bash
# Always restart
docker run --restart=always themisdb/themisdb:latest

# Restart unless manually stopped
docker run --restart=unless-stopped themisdb/themisdb:latest

# Restart with max retry count
docker run --restart=on-failure:5 themisdb/themisdb:latest
```

### Logging

```bash
# JSON file driver (max size limit)
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

# View logs
docker logs --tail 100 --follow themis
```

---

## Platform-Specific Deployment

### Linux (x64)

```bash
# Ubuntu 22.04+
docker run -d \
  --platform linux/amd64 \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Debian
apt-get install docker.io
docker pull themisdb/themisdb:latest
```

### ARM/Raspberry Pi

```bash
# Auto-detects ARM64
docker pull themisdb/themisdb:latest

# Explicit pull
docker pull --platform linux/arm64 themisdb/themisdb:latest

# Run
docker run -d \
  --platform linux/arm64 \
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### QNAP NAS

```bash
# Pull QNAP-optimized image (Ubuntu 20.04, SSE4.2 baseline)
docker pull themisdb/themisdb:qnap

# Run on QNAP (port 18765 to avoid conflicts)
docker run -d \
  --name themis \
  -p 18765:18765 \
  -v /share/Container/themis/data:/data \
  -v /share/Container/themis/config/config.qnap.json:/etc/themis/config.json:ro \
  themisdb/themisdb:qnap

# Or use docker-compose.qnap.yml
# See docker/docker-compose.qnap.yml for full setup
```

**QNAP Notes:**
- Use `qnap` or `v1.0.2-qnap` tags (optimized for older CPUs)
- Default port 18765 avoids QNAP service conflicts
- Mount volumes to `/share/Container/themis/`
- Requires GLIBC 2.31+ (QNAP QTS 5.0+)

### macOS (Apple Silicon/Intel)

```bash
# Auto-selects correct architecture
docker pull themisdb/themisdb:latest

# Explicitly specify
docker pull --platform linux/arm64 themisdb/themisdb:latest  # M-series
docker pull --platform linux/amd64 themisdb/themisdb:latest  # Intel
```

### Windows (Docker Desktop)

```powershell
# Pull image
docker pull themisdb/themisdb:latest

# Run
docker run -d `
  -p 8080:8080 `
  -p 18765:18765 `
  -v themis_data:C:\data `
  themisdb/themisdb:latest

# View logs
docker logs -f themis
```

---

## Troubleshooting

### Container Fails to Start

```bash
# Check logs
docker logs themis

# Common issues:
# 1. Port already in use
docker ps  # Find conflicting container
docker stop <container_id>

# 2. Insufficient disk space
docker system df  # Check usage

# 3. Broken config
docker exec themis cat /etc/themis/config.json
```

### Performance Issues

```bash
# Monitor container stats
docker stats themis

# Check resource limits
docker inspect themis | grep -i memory

# Increase memory allocation
docker update --memory 16g themis
docker restart themis
```

### Network Connectivity

```bash
# Test from host
curl http://localhost:8080/api/health

# Test from within container
docker exec themis curl http://localhost:18765/health

# Check port binding
docker port themis
```

### Library Path Issues

```bash
# Verify libraries are loaded
docker exec themis ldd /usr/local/bin/themis_server

# Check library path
docker exec themis echo $LD_LIBRARY_PATH

# Rebuild with updated lib path if needed
docker pull themisdb/themisdb:latest --force
```

---

## Build Your Own Image (Advanced)

### Build from Source

```bash
# Clone repo
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build multi-arch (requires buildx setup)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis:custom:latest .

# Build single-arch
docker build \
  -f Dockerfile \
  -t themis:custom:latest .
```

### Build Arguments

```dockerfile
# Customize build
docker build \
  --build-arg VCPKG_ENABLE_ONLINE=OFF \
  --build-arg THEMIS_VERSION=1.0.2 \
  -t themis:custom .
```

### QNAP-Specific Build

```bash
# Build QNAP-optimized image (Ubuntu 20.04, baseline CPU)
docker build \
  -f docker/Dockerfile.qnap \
  -t themis:qnap \
  .

# Tag and push
docker tag themis:qnap themisdb/themisdb:qnap
docker push themisdb/themisdb:qnap
```

---

## Best Practices

✅ **DO:**
- Use named volumes for data persistence
- Set resource limits (CPU, memory)
- Use health checks
- Enable restart policies
- Log to syslog or json-file with rotation
- Use specific version tags (not just `latest`)
- Run as non-root (built-in)
- Mount config as read-only

❌ **DON'T:**
- Run containers with `--privileged`
- Use `latest` tag in production (use specific versions)
- Store secrets in environment variables
- Ignore health check failures
- Disable restart policies
- Map unnecessary ports

---

## Support & Issues

**Docker Hub:** https://hub.docker.com/r/themisdb/themisdb

**GitHub Issues:** https://github.com/makr-code/ThemisDB/issues

**Deployment Strategy:** [Full Deployment Strategy](docs/deployment/deployment_strategy.md)

---

## Related Documentation

- [README.md](README.md) - Main documentation
- [CHANGELOG.md](CHANGELOG.md) - Release notes
- [BUILD_ORGANIZATION.md](BUILD_ORGANIZATION.md) - Build system
- [Dockerfile](Dockerfile) - Build definition
