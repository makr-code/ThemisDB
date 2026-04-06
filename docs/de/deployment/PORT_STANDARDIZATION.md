# Port Standardization Summary

**Stand:** 6. April 2026  
**Version:** v1.5.0-dev  
**Kategorie:** 🚀 Deployment

---

## 📑 Table of Contents

- [Overview](#overview)
- [Standard Port Mapping](#standard-port-mapping)
- [Migration](#migration-from-previous-versions)
- [Implementation Status](#implementation-status)

## Overview

This document summarizes the port standardization effort for ThemisDB to provide clean port mappings for all optional interfaces.

---

## Standard Port Mapping

### Production Ports (v1.3.0+)

| Port  | Service | Protocol | Status | Build Flag |
|-------|---------|----------|--------|------------|
| 8080  | HTTP API | HTTP/1.1, HTTP/2, GraphQL | Core | None |
| 18765 | Wire Protocol | Binary, gRPC | Core | None |
| 4318  | Metrics | OpenTelemetry/OTLP | Core | None |
| 1883  | MQTT | MQTT 3.1.1, 5.0 (plain) | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| 8883  | MQTT TLS | MQTT over TLS 1.3 | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| 8083  | MQTT WebSocket | MQTT over WebSocket | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| 5432  | PostgreSQL Wire | PostgreSQL Wire Protocol v3.0 | Optional | `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON` |
| 3000  | MCP | Model Context Protocol | Optional | `-DTHEMIS_ENABLE_MCP=ON` |

---

## Migration from Previous Versions

### Port Changes

**v1.2.0 and earlier:**
- Used port 8765 for HTTP API (some configs)
- No standardized port for wire protocol
- No metrics port

**v1.3.0+:**
- **8080**: HTTP API (standardized across all configs)
- **18765**: Wire Protocol (new standard, avoids conflicts)
- **4318**: Metrics (OTLP standard port)

### Backwards Compatibility

For backwards compatibility in benchmarks and tests:
- Internal container port 8765 may still be used
- External port mappings should use the new standard (8080, 18765)
- Examples: `"8080:8765"` maps host 8080 to container 8765

---

## Implementation Status

### ✅ Updated Files

**Docker:**
- `/Dockerfile` - EXPOSE statements with documentation
- `/docker-compose.yml` - Standard port mappings
- `/docker-compose-vllm.yml` - Updated port comments
- `/docker/README.md` - Port reference table

**Documentation:**
- `/docs/deployment/PORT_REFERENCE.md` - Comprehensive 500+ line guide
- `/docs/apis/OPTIONAL_PROTOCOLS.md` - Port overview table
- `/README.md` - Quick reference to default ports

**Examples:**
- `/examples/railway/docker-compose.railway.yml` - Standardized ports

### 🔄 Files with Legacy Ports (Acceptable)

**Benchmarks:**
- `/benchmarks/docker-compose.benchmark.yml` - Uses 8765 internally (OK)
- `/benchmarks/docker-compose.multi-shard-raid.yml` - Multi-shard with port ranges (OK)
- `/benchmarks/docker-compose.raid-phase1.yml` - Custom port ranges for testing (OK)
- `/benchmarks/docker-compose.raid-phase2.yml` - Custom port ranges for testing (OK)

**Rationale:** Benchmark and test configurations can use custom port mappings for their specific testing scenarios. The internal container ports don't need to change as long as the documentation clearly states the production standard.

---

## Docker Compose Patterns

### Standard Production Pattern

```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb:1.3.0
    ports:
      # Core ports (always enabled)
      - "8080:8080"      # HTTP API
      - "18765:18765"    # Wire Protocol
      - "4318:4318"      # Metrics
      
      # Optional protocol ports (uncomment when enabled)
      # - "1883:1883"    # MQTT plain
      # - "8883:8883"    # MQTT TLS
      # - "8083:8083"    # MQTT WebSocket
      # - "5432:5432"    # PostgreSQL Wire
      # - "3000:3000"    # MCP
```

### Multi-Instance Pattern (Sharding)

```yaml
version: '3.8'
services:
  shard-0:
    image: themisdb:1.3.0
    ports:
      - "8080:8080"      # HTTP API (shard 0)
      - "18765:18765"    # Wire Protocol (shard 0)
  
  shard-1:
    image: themisdb:1.3.0
    ports:
      - "8081:8080"      # HTTP API (shard 1)
      - "18766:18765"    # Wire Protocol (shard 1)
  
  shard-2:
    image: themisdb:1.3.0
    ports:
      - "8082:8080"      # HTTP API (shard 2)
      - "18767:18765"    # Wire Protocol (shard 2)
```

### Test/Benchmark Pattern (Legacy Ports OK)

```yaml
version: '3.8'
services:
  themisdb-test:
    image: themisdb:1.3.0
    ports:
      - "9000:8765"      # Custom mapping for testing (acceptable)
```

---

## Configuration Files

### config.json Port Settings

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 18765,           // Wire protocol port
    "http_port": 8080,       // HTTP API port
    "metrics_port": 4318     // Metrics port
  },
  
  "optional_protocols": {
    "mqtt": {
      "enabled": false,
      "port": 1883,
      "tls_port": 8883,
      "websocket_port": 8083
    },
    "postgres_wire": {
      "enabled": false,
      "port": 5432
    },
    "mcp": {
      "enabled": false,
      "port": 3000
    }
  }
}
```

---

## Security Considerations

### Port Exposure Strategy

**Default (Secure):**
- Only core ports exposed (8080, 18765, 4318)
- Minimal attack surface
- Optional protocols require explicit opt-in

**Production Hardening:**
- Use TLS for all protocols
- Prefer encrypted variants (8883 over 1883 for MQTT)
- Network isolation where possible
- Firewall rules to restrict access

### Firewall Configuration Example

```bash
# Allow core ports
ufw allow 8080/tcp    # HTTP API
ufw allow 18765/tcp   # Wire Protocol
ufw allow 4318/tcp    # Metrics

# Block unencrypted protocols
ufw deny 1883/tcp     # MQTT plain (use 8883 instead)

# Optional: Allow encrypted protocols
ufw allow 8883/tcp    # MQTT TLS (if enabled)
ufw allow 5432/tcp    # PostgreSQL Wire (if enabled)
```

---

## Troubleshooting

### Port Conflicts

**Problem:** Port already in use

**Solution:** Use docker-compose port mapping to avoid conflicts
```yaml
ports:
  - "8081:8080"  # Map host 8081 to container 8080
  - "18766:18765"
```

### Port Not Accessible

**Check list:**
1. Container is running: `docker ps`
2. Port is exposed in Dockerfile: `EXPOSE 8080`
3. Port is mapped in docker-compose: `"8080:8080"`
4. Firewall allows traffic: `ufw status`
5. Service is listening: `netstat -tuln | grep 8080`

### Protocol Not Available

**Problem:** Connection refused on optional protocol port

**Cause:** Protocol not enabled at build time

**Solution:** Rebuild with appropriate flag:
```bash
cmake -B build -S . -DTHEMIS_ENABLE_MQTT=ON
docker build --build-arg ENABLE_MQTT=ON -t themisdb:custom .
```

---

## Testing Port Configuration

### Health Check All Ports

```bash
#!/bin/bash
# test-ports.sh - Verify all core ports are accessible

echo "Testing core ports..."

# HTTP API
curl -f http://localhost:8080/health && echo "✓ HTTP API (8080)" || echo "✗ HTTP API (8080)"

# Wire Protocol (telnet test)
timeout 2 bash -c "echo -n '' | telnet localhost 18765 2>&1 | grep -q Connected" && \
  echo "✓ Wire Protocol (18765)" || echo "✗ Wire Protocol (18765)"

# Metrics
curl -f http://localhost:4318/metrics && echo "✓ Metrics (4318)" || echo "✗ Metrics (4318)"
```

### Docker Health Check

```yaml
healthcheck:
  test: |
    curl -fsS http://localhost:8080/health || exit 1
  interval: 30s
  timeout: 5s
  retries: 3
  start_period: 10s
```

---

## References

- **[PORT_REFERENCE.md](PORT_REFERENCE.md)** - Complete port documentation with examples
- **[OPTIONAL_PROTOCOLS.md](../apis/OPTIONAL_PROTOCOLS.md)** - Optional protocol configuration
- **[docker-compose.yml](../../docker-compose.yml)** - Reference configuration
- **[Dockerfile](../../Dockerfile)** - Port definitions

---

## Changelog

### v1.3.0 (2024-12-21)
- Standardized core ports: 8080 (HTTP), 18765 (Wire), 4318 (Metrics)
- Documented optional protocol ports: 1883/8883/8083 (MQTT), 5432 (PostgreSQL), 3000 (MCP)
- Created comprehensive PORT_REFERENCE.md guide
- Updated all production docker-compose files
- Added port reference to documentation

### v1.2.0 (2024-12-18)
- Mixed port usage (8765, 8080 in different configs)
- No standardized metrics port
- Limited optional protocol documentation

---

**Questions?** Open an issue on [GitHub](https://github.com/makr-code/ThemisDB/issues)
