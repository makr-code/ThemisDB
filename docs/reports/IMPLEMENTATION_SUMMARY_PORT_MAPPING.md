# Clean Port Mapping Implementation - Summary

**Date:** December 21, 2024  
**Issue:** Port mapping for optional interfaces (MCP, MQTT, PostgreSQL Wire, etc.)  
**Status:** ✅ Complete

---

## Problem Statement (Translated from German)

"We have now added some optional interfaces (MCP, etc.). Now we need a clean port mapping for everything (with docker translate)."

---

## Solution Overview

Implemented a comprehensive, clean port mapping strategy for ThemisDB with clear separation between:
- **Core ports** (always enabled)
- **Optional protocol ports** (require explicit build flags)

All ports are now properly documented with security-first approach (opt-in for optional protocols).

---

## Port Mapping Table

| Port  | Service | Protocol | Status | Build Flag Required |
|-------|---------|----------|--------|---------------------|
| **8080** | HTTP API | HTTP/1.1, HTTP/2, GraphQL | Core | None |
| **18765** | Wire Protocol | Binary, gRPC | Core | None |
| **4318** | Metrics | OpenTelemetry/OTLP | Core | None |
| **1883** | MQTT | MQTT 3.1.1, 5.0 (plain) | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| **8883** | MQTT TLS | MQTT over TLS 1.3 | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| **8083** | MQTT WebSocket | MQTT over WebSocket | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| **5432** | PostgreSQL Wire | PostgreSQL Wire Protocol v3.0 | Optional | `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON` |
| **3000** | MCP | Model Context Protocol | Optional | `-DTHEMIS_ENABLE_MCP=ON` |

---

## Files Modified/Created

### Docker Configuration
- ✅ `/Dockerfile` - Added comprehensive EXPOSE documentation
- ✅ `/docker-compose.yml` - Updated with all port mappings and comments
- ✅ `/docker-compose-vllm.yml` - Added port documentation

### Documentation (New)
- ✅ `/docs/deployment/PORT_REFERENCE.md` - **Comprehensive 500+ line guide**
  - Complete reference for all ports
  - Usage examples (Python, JavaScript, bash)
  - Docker/Kubernetes deployment patterns
  - Security best practices
  - Troubleshooting guide
  
- ✅ `/docs/deployment/PORT_STANDARDIZATION.md` - **Migration and implementation guide**
  - Port migration from v1.2.0 to v1.3.0
  - Backwards compatibility strategy
  - Testing procedures
  - Changelog

### Documentation (Updated)
- ✅ `/docker/README.md` - Added port reference table and link
- ✅ `/docs/apis/OPTIONAL_PROTOCOLS.md` - Added port overview table
- ✅ `/README.md` - Added default ports and reference link

### Examples
- ✅ `/examples/railway/docker-compose.railway.yml` - Fixed port mappings

---

## Key Features

### 🔒 Security First
- Minimal attack surface by default (only core ports)
- Optional protocols require explicit opt-in via build flags
- TLS variants recommended over plain protocols
- Clear documentation of security implications

### 📖 Comprehensive Documentation
- Two detailed guides totaling 750+ lines
- Real-world usage examples for all protocols
- Client code examples (Python, JavaScript, bash)
- Troubleshooting and migration guides

### 🐳 Docker Ready
- Clear port mappings in Dockerfile and docker-compose
- Commented optional ports (uncomment when needed)
- Health checks configured
- Multi-instance patterns documented

### 🔄 Backwards Compatible
- Benchmark and test configurations unchanged
- Internal container ports can remain flexible
- Clear migration path documented

---

## Usage Examples

### Basic Deployment (Core Ports Only)

```yaml
version: "3.8"
services:
  themisdb:
    image: themisdb:1.3.0
    ports:
      - "8080:8080"     # HTTP API
      - "18765:18765"   # Wire Protocol
      - "4318:4318"     # Metrics
    volumes:
      - themis_data:/data
```

```bash
docker compose up -d
curl http://localhost:8080/health
```

### Full Deployment (All Optional Protocols)

**Prerequisites:** Build with all protocol flags enabled

```bash
# Build with all protocols
docker build \
  --build-arg ENABLE_MQTT=ON \
  --build-arg ENABLE_POSTGRES_WIRE=ON \
  --build-arg ENABLE_MCP=ON \
  -t themisdb:1.3.0-full .
```

```yaml
version: "3.8"
services:
  themisdb:
    image: themisdb:1.3.0-full
    ports:
      # Core ports
      - "8080:8080"
      - "18765:18765"
      - "4318:4318"
      
      # Optional protocols
      - "1883:1883"     # MQTT plain
      - "8883:8883"     # MQTT TLS
      - "8083:8083"     # MQTT WebSocket
      - "5432:5432"     # PostgreSQL Wire
      - "3000:3000"     # MCP
```

### Client Examples

**HTTP API (Python):**
```python
import requests

# Query via HTTP API
response = requests.post('http://localhost:8080/api/query', 
    json={'query': 'MATCH (n) RETURN n LIMIT 10'})
print(response.json())
```

**MQTT (Python):**
```python
import paho.mqtt.client as mqtt

client = mqtt.Client("sensor_001")
client.connect("localhost", 1883, 60)
client.publish("sensors/temp/room1", '{"value": 22.5}', qos=1)
```

**PostgreSQL Wire (psql):**
```bash
psql -h localhost -p 5432 -U themis_user -d themis
SELECT * FROM users WHERE age > 25;
```

**MCP (WebSocket JavaScript):**
```javascript
const ws = new WebSocket('ws://localhost:3000/mcp/ws');
ws.onopen = () => {
  ws.send(JSON.stringify({
    jsonrpc: "2.0",
    method: "tools/list",
    id: 1
  }));
};
```

---

## Security Best Practices

### 1. Minimal Exposure
```yaml
# Default (Secure) - Only core ports
ports:
  - "8080:8080"
  - "18765:18765"
  - "4318:4318"
```

### 2. Use TLS for Optional Protocols
```yaml
# Prefer encrypted variants
ports:
  - "8883:8883"    # MQTT TLS (not 1883)
```

### 3. Network Isolation
```yaml
networks:
  internal:
    driver: bridge
    internal: true  # No external access
```

### 4. Firewall Rules
```bash
# Allow only necessary ports
ufw allow 8080/tcp
ufw allow 18765/tcp
ufw deny 1883/tcp    # Block unencrypted MQTT
```

---

## Migration Guide

### From v1.2.0 to v1.3.0

**Old Configuration:**
```yaml
ports:
  - "8765:8765"  # Mixed usage
```

**New Configuration:**
```yaml
ports:
  - "8080:8080"     # HTTP API (standardized)
  - "18765:18765"   # Wire Protocol (standardized)
  - "4318:4318"     # Metrics (new)
```

**Changes Required:**
1. Update port mappings in docker-compose.yml
2. Update client connection strings
3. Update firewall rules if applicable

**No Breaking Changes:**
- Internal container ports can remain unchanged
- Backwards compatible mappings supported (e.g., "8080:8765")

---

## Documentation Structure

```
docs/
├── deployment/
│   ├── PORT_REFERENCE.md           # Complete port reference (500+ lines)
│   ├── PORT_STANDARDIZATION.md     # Migration guide (250+ lines)
│   └── DOCKER_DEPLOYMENT.md        # Docker deployment guide
└── apis/
    ├── OPTIONAL_PROTOCOLS.md       # Protocol implementations
    ├── MCP_PROTOCOL_SUPPORT.md     # MCP specific docs
    └── README.md                   # API overview
```

---

## Testing

### Port Availability Test

```bash
#!/bin/bash
# Test all core ports are accessible

echo "Testing core ports..."

# HTTP API
curl -f http://localhost:8080/health && echo "✓ HTTP (8080)" || echo "✗ HTTP (8080)"

# Wire Protocol
timeout 2 bash -c "echo '' | telnet localhost 18765 2>&1 | grep -q Connected" && \
  echo "✓ Wire (18765)" || echo "✗ Wire (18765)"

# Metrics
curl -f http://localhost:4318/metrics && echo "✓ Metrics (4318)" || echo "✗ Metrics (4318)"
```

### Docker Health Check

```yaml
healthcheck:
  test: ["CMD-SHELL", "curl -fsS http://localhost:8080/health || exit 1"]
  interval: 30s
  timeout: 5s
  retries: 3
  start_period: 10s
```

---

## Troubleshooting

### Port Already in Use

**Error:**
```
Error: bind: address already in use
```

**Solution:**
```yaml
# Use different host port
ports:
  - "8081:8080"  # Map host 8081 to container 8080
```

### Protocol Not Available

**Error:**
```
Connection refused on port 5432
```

**Cause:** Protocol not enabled at build time

**Solution:**
```bash
# Rebuild with protocol flag
cmake -B build -S . -DTHEMIS_ENABLE_POSTGRES_WIRE=ON
docker build --build-arg ENABLE_POSTGRES_WIRE=ON -t themisdb:custom .
```

### Firewall Blocking Ports

```bash
# Check firewall status
sudo ufw status

# Allow port
sudo ufw allow 8080/tcp
```

---

## References

### Documentation Links
- **[PORT_REFERENCE.md](docs/deployment/PORT_REFERENCE.md)** - Complete port guide
- **[PORT_STANDARDIZATION.md](docs/deployment/PORT_STANDARDIZATION.md)** - Migration guide
- **[OPTIONAL_PROTOCOLS.md](docs/apis/OPTIONAL_PROTOCOLS.md)** - Protocol implementations
- **[MCP_PROTOCOL_SUPPORT.md](docs/apis/MCP_PROTOCOL_SUPPORT.md)** - MCP integration
- **[README.md](README.md)** - Project overview

### Configuration Files
- **[Dockerfile](Dockerfile)** - Port definitions
- **[docker-compose.yml](docker-compose.yml)** - Reference configuration
- **[docker-compose-vllm.yml](docker-compose-vllm.yml)** - vLLM co-location

---

## Success Criteria

All objectives met:

✅ **Clean Port Mapping** - All ports clearly defined and documented  
✅ **Optional Protocols** - MCP, MQTT, PostgreSQL Wire properly mapped  
✅ **Docker Integration** - docker-compose.yml fully updated  
✅ **Documentation** - Comprehensive guides (750+ lines total)  
✅ **Security** - Opt-in approach for optional protocols  
✅ **Backwards Compatible** - Migration path documented  
✅ **Examples** - Real-world usage for all protocols  
✅ **Troubleshooting** - Common issues covered  

---

## Commits

1. **Initial plan** - d6b9b6f
2. **Add comprehensive port mapping documentation** - 283aa48
3. **Update port references in documentation** - dd6d38b
4. **Add port standardization summary** - 8e39a94

---

**Total Changes:**
- Files Modified: 8
- Files Created: 3
- Lines Added: ~1100
- Documentation: 750+ lines

---

**Status:** ✅ Ready for Review and Merge

