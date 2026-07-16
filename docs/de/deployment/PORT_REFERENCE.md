# ThemisDB Port Reference Guide

**Stand:** 6. April 2026  
**Version:** v1.5.0-dev  
**Kategorie:** 🚀 Deployment

---

## 📑 Table of Contents

- [Overview](#overview)
- [Port Summary Table](#port-summary-table)
- [Core Ports](#core-ports-always-enabled)

This document provides a comprehensive reference for all ports used by ThemisDB and its optional protocol interfaces.

---

## Overview

ThemisDB uses a clean port mapping strategy to separate core functionality from optional protocols. This approach:
- Minimizes attack surface by default (only core ports exposed)
- Provides clear separation of concerns
- Enables flexible deployment configurations
- Follows security best practices (opt-in for additional protocols)

---

## Port Summary Table

| Port  | Protocol/Service | Status | Build Flag Required | TLS Support | Description |
|-------|-----------------|--------|---------------------|-------------|-------------|
| 8080  | HTTP/1.1, GraphQL, HTTP/2 | Core | None | ✅ Optional | REST API, GraphQL endpoint, HTTP/2 upgrade |
| 18765 | Wire Protocol, gRPC | Core | None | ✅ **Recommended** | Binary protocol, gRPC inter-shard communication |
| 4318  | OpenTelemetry/OTLP | Core | None | ⚠️ Internal | Prometheus metrics, OpenTelemetry collector |
| 1883  | MQTT | Optional | `-DTHEMIS_ENABLE_MQTT=ON` | ❌ No | MQTT broker (plain, unencrypted) |
| 8883  | MQTT over TLS | Optional | `-DTHEMIS_ENABLE_MQTT=ON` | ✅ Required | MQTT broker with TLS encryption |
| 8083  | MQTT over WebSocket | Optional | `-DTHEMIS_ENABLE_MQTT=ON` | ⚠️ Optional | MQTT broker via WebSocket transport |
| 5432  | PostgreSQL Wire Protocol | Optional | `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON` | ✅ Recommended | PostgreSQL-compatible wire protocol |
| 3000  | MCP | Optional | `-DTHEMIS_ENABLE_MCP=ON` | ⚠️ Optional | Model Context Protocol for LLM integration |

**Legend:**
- ✅ **Recommended**: TLS should be enabled for production use
- ✅ **Required**: TLS is mandatory for this protocol
- ✅ **Optional**: TLS can be enabled but is not required
- ⚠️ **Internal**: Should only be exposed on internal networks
- ❌ **No**: TLS not supported (use TLS-enabled alternative)

---

## Core Ports (Always Enabled)

### Port 8080 - HTTP/REST API

**Protocols:** HTTP/1.1, HTTP/2 (if enabled), GraphQL  
**Transport:** TCP  
**TLS:** Optional (configure via `server.tls` in config.json)

**Endpoints:**
- `GET /health` - Health check endpoint
- `POST /api/query` - Query execution
- `POST /api/entities` - Entity CRUD operations
- `POST /graphql` - GraphQL endpoint
- `GET /metrics` - Prometheus metrics (text format)
- `GET /api/sse` - Server-Sent Events for CDC
- WebSocket upgrade for real-time features (if enabled)

**Docker Mapping:**
```yaml
ports:
  - "8080:8080"  # Host:Container
```

**Usage Example:**
```bash
# Health check
curl http://localhost:8080/health

# Execute query
curl -X POST http://localhost:8080/api/query \
  -H "Content-Type: application/json" \
  -d '{"query": "MATCH (n) RETURN n LIMIT 10"}'
```

---

### Port 18765 - Wire Protocol

**Protocols:** Binary Wire Protocol, gRPC  
**Transport:** TCP  
**TLS:** ✅ **Recommended for Production** (configure via `WireProtocolServer::Config`)

**Purpose:**
- High-performance binary protocol for client SDKs
- gRPC inter-shard communication (Enterprise Edition)
- Optimized for low-latency operations
- **Default port for production deployments with TLS**

**Security Recommendations:**
- ✅ **Enable TLS** for all production deployments
- ✅ **Use mTLS** for service-to-service communication
- ✅ Configure minimum TLS version (1.2 or 1.3)
- ✅ Use strong cipher suites
- ⚠️ **Never expose unencrypted Wire Protocol to public networks**

**Configuration:**
```cpp
// C++ Server Configuration
config.enable_tls = true;
config.tls_cert_path = "/etc/themisdb/certs/server.crt";
config.tls_key_path = "/etc/themisdb/certs/server-key.pem";
config.tls_ca_cert_path = "/etc/themisdb/certs/ca.crt";
config.tls_require_client_cert = true;  // For mTLS
```

**Security Posture:**
- ✅ **Production Mode**: TLS encryption is **mandatory** - server will refuse to start without TLS
- ⚠️ **Development Mode**: TLS is optional for local testing
- 🔒 **Override**: Use `--allow-insecure-wire-protocol` flag to bypass (not recommended)

**Production Configuration:**
```yaml
# config.yaml
wire_protocol:
  enable_tls: true  # REQUIRED in production
  tls_cert_path: /path/to/server.crt
  tls_key_path: /path/to/server.key
  tls_ca_cert_path: /path/to/ca.crt  # Optional: for mTLS
  tls_require_client_cert: true      # Optional: enforce mTLS
  port: 18765
```

**Docker Mapping:**
```yaml
ports:
  - "18765:18765"
```

**Usage Example:**
```go
// Go client with TLS
import themisdb "github.com/makr-code/ThemisDB/clients/go"

tlsConfig := themisdb.NewProductionTLSConfig("/etc/themisdb/certs/ca.crt")
tlsConfig.ServerName = "themisdb.example.com"

client, err := themisdb.NewWireClientWithTLS(
    "themisdb.example.com",
    18765,
    "username",
    "password",
    tlsConfig,
)
```

**See Also:**
- [Wire Protocol Transport Security Guide](../../en/guides/WIRE_PROTOCOL_TRANSPORT_SECURITY.md)
- [TLS Setup Guide](../guides/guides_tls_setup.md)

---

### Port 4318 - OpenTelemetry/Metrics

**Protocols:** OpenTelemetry Protocol (OTLP), HTTP  
**Transport:** TCP  
**Format:** JSON, Protobuf

**Purpose:**
- Prometheus metrics scraping
- OpenTelemetry trace/metrics collection
- Integration with observability stacks (Grafana, Datadog, etc.)

**Docker Mapping:**
```yaml
ports:
  - "4318:4318"
```

**Prometheus Configuration:**
```yaml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:4318']
```

---

## Optional Protocol Ports

### Port 1883 - MQTT (Plain)

**Protocol:** MQTT 3.1.1, MQTT 5.0  
**Transport:** TCP (plain, unencrypted)  
**Build Flag:** `-DTHEMIS_ENABLE_MQTT=ON`  
**Security:** ⚠️ Not recommended for production (use 8883 with TLS)

**Purpose:**
- IoT device connectivity
- Pub/Sub messaging
- Sensor data ingestion

**Docker Mapping:**
```yaml
ports:
  - "1883:1883"  # Only if MQTT enabled
```

**Configuration (config.json):**
```json
{
  "enable_mqtt": true,
  "mqtt_port": 1883,
  "mqtt_max_clients": 10000,
  "mqtt_qos_support": [0, 1, 2]
}
```

**Client Example (Python):**
```python
import paho.mqtt.client as mqtt

client = mqtt.Client("sensor_001")
client.connect("localhost", 1883, 60)
client.publish("sensors/temperature/room1", '{"value": 22.5}', qos=1)
```

**See Also:** [OPTIONAL_PROTOCOLS.md](../apis/OPTIONAL_PROTOCOLS.md#mqtt-protocol-support)

---

### Port 8883 - MQTT over TLS

**Protocol:** MQTT 3.1.1, MQTT 5.0 with TLS  
**Transport:** TCP with TLS 1.3  
**Build Flag:** `-DTHEMIS_ENABLE_MQTT=ON`  
**Security:** ✅ Recommended for production

**Purpose:**
- Secure IoT device connectivity
- Encrypted pub/sub messaging
- Certificate-based authentication

**Docker Mapping:**
```yaml
ports:
  - "8883:8883"  # Only if MQTT enabled
```

**Configuration (config.json):**
```json
{
  "enable_mqtt": true,
  "mqtt_tls_port": 8883,
  "mqtt_ssl_enabled": true,
  "mqtt_cert_file": "/etc/themis/certs/server.crt",
  "mqtt_key_file": "/etc/themis/certs/server.key",
  "mqtt_ca_file": "/etc/themis/certs/ca.crt"
}
```

**Client Example (Python):**
```python
import paho.mqtt.client as mqtt
import ssl

client = mqtt.Client("sensor_001")
client.tls_set(
    ca_certs="/path/to/ca.crt",
    certfile="/path/to/client.crt",
    keyfile="/path/to/client.key",
    tls_version=ssl.PROTOCOL_TLSv1_2
)
client.connect("localhost", 8883, 60)
```

---

### Port 8083 - MQTT over WebSocket

**Protocol:** MQTT over WebSocket  
**Transport:** HTTP WebSocket upgrade  
**Build Flag:** `-DTHEMIS_ENABLE_MQTT=ON`

**Purpose:**
- Browser-based MQTT clients
- Web applications requiring MQTT
- Firewall-friendly MQTT connectivity

**Docker Mapping:**
```yaml
ports:
  - "8083:8083"  # Only if MQTT enabled
```

**Configuration (config.json):**
```json
{
  "enable_mqtt": true,
  "mqtt_enable_websockets": true,
  "mqtt_websocket_port": 8083
}
```

**Client Example (JavaScript):**
```javascript
const mqtt = require('mqtt');

const client = mqtt.connect('ws://localhost:8083', {
  clientId: 'webapp_' + Math.random().toString(16).substr(2, 8)
});

client.on('connect', () => {
  client.subscribe('sensors/#');
  client.publish('sensors/temperature/room1', '{"value": 22.5}');
});
```

---

### Port 5432 - PostgreSQL Wire Protocol

**Protocol:** PostgreSQL Wire Protocol v3.0  
**Transport:** TCP  
**Build Flag:** `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON`

**Purpose:**
- PostgreSQL tool compatibility (psql, pgAdmin, DBeaver)
- SQL query interface (translated to Cypher internally)
- BI tool integration (Tableau, Power BI)

**Docker Mapping:**
```yaml
ports:
  - "5432:5432"  # Only if PostgreSQL Wire Protocol enabled
```

**Configuration (config.json):**
```json
{
  "enable_postgres_wire": true,
  "postgres_port": 5432,
  "postgres_max_connections": 100,
  "postgres_ssl_enabled": true,
  "postgres_auth_method": "md5",
  "postgres_default_database": "themis",
  "postgres_server_version": "14.0"
}
```

**Client Example (psql):**
```bash
# Connect using psql
psql -h localhost -p 5432 -U themis_user -d themis

# Execute SQL queries (translated to Cypher)
SELECT * FROM users WHERE age > 25;
```

**Client Example (Python psycopg2):**
```python
import psycopg2

conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="themis",
    user="themis_user",
    password="password"
)

cursor = conn.cursor()
cursor.execute("SELECT * FROM users WHERE age > %s", (25,))
rows = cursor.fetchall()
```

**See Also:** [OPTIONAL_PROTOCOLS.md](../apis/OPTIONAL_PROTOCOLS.md#postgresql-wire-protocol-support)

---

### Port 3000 - MCP (Model Context Protocol)

**Protocol:** MCP (JSON-RPC)  
**Transports:** stdio, SSE (Server-Sent Events), WebSocket  
**Build Flag:** `-DTHEMIS_ENABLE_MCP=ON`

**Purpose:**
- LLM integration (Claude, GPT, local models)
- AI-powered database queries
- Context sharing with language models
- Tool calling for database operations

**Docker Mapping:**
```yaml
ports:
  - "3000:3000"  # Only if MCP enabled
```

**Configuration (config.json):**
```json
{
  "enable_mcp": true,
  "mcp_port": 3000,
  "mcp_enable_stdio": true,
  "mcp_enable_sse": true,
  "mcp_enable_websocket": true,
  "mcp_server_name": "ThemisDB",
  "mcp_server_version": "1.3.0"
}
```

**MCP Transports:**

**1. stdio (Claude Desktop):**
```json
// Claude Desktop config
{
  "mcpServers": {
    "themisdb": {
      "command": "docker",
      "args": ["exec", "-i", "themisdb-server", "/usr/local/bin/themis_mcp"]
    }
  }
}
```

**2. SSE (HTTP clients):**
```bash
# Connect via SSE
curl -N http://localhost:3000/mcp/sse
```

**3. WebSocket (bidirectional):**
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

**Available MCP Tools:**
- `query` - Execute Cypher/SQL queries
- `put_entity` - Create/update entities
- `get_entity` - Retrieve entities by ID
- `delete_entity` - Delete entities
- `create_index` - Create database indexes
- `get_schema` - Retrieve database schema
- `get_stats` - Get database statistics

**See Also:** 
- [MCP_PROTOCOL_SUPPORT.md](../apis/MCP_PROTOCOL_SUPPORT.md)
- [MCP_MINIMAL_INTEGRATION.md](../apis/MCP_MINIMAL_INTEGRATION.md)

---

## Docker Deployment Examples

### Minimal Deployment (Core Ports Only)

```yaml
version: "3.8"
services:
  themisdb:
    image: themisdb:1.3.0
    container_name: themisdb-server
    ports:
      - "8080:8080"     # HTTP API
      - "18765:18765"   # Wire Protocol
      - "4318:4318"     # Metrics
    volumes:
      - themis_data:/data
    restart: unless-stopped

volumes:
  themis_data:
```

**Start:**
```bash
docker compose up -d
curl http://localhost:8080/health
```

---

### Full Deployment (All Optional Protocols)

**Prerequisites:**
- Build ThemisDB with all protocol flags enabled
- Configure certificates for TLS protocols

```yaml
version: "3.8"
services:
  themisdb:
    image: themisdb:1.3.0-full  # Custom build with all protocols
    container_name: themisdb-full
    ports:
      # Core ports
      - "8080:8080"     # HTTP/REST API
      - "18765:18765"   # Wire Protocol
      - "4318:4318"     # Metrics
      
      # Optional protocol ports
      - "1883:1883"     # MQTT plain
      - "8883:8883"     # MQTT TLS
      - "8083:8083"     # MQTT WebSocket
      - "5432:5432"     # PostgreSQL Wire
      - "3000:3000"     # MCP
    volumes:
      - themis_data:/data
      - ./config:/etc/themis:ro
      - ./certs:/etc/themis/certs:ro
    environment:
      THEMIS_CONFIG_PATH: "/etc/themis/config.json"
    restart: unless-stopped

volumes:
  themis_data:
```

**Build Command:**
```bash
docker build \
  --build-arg ENABLE_MQTT=ON \
  --build-arg ENABLE_POSTGRES_WIRE=ON \
  --build-arg ENABLE_MCP=ON \
  -t themisdb:1.3.0-full .
```

---

### Railway/Cloud Deployment

For Railway, Render, or other cloud platforms with dynamic port assignment:

```yaml
version: "3.8"
services:
  themisdb:
    image: themisdb:1.3.0
    ports:
      - "${PORT:-8080}:8080"  # Railway assigns PORT dynamically
    environment:
      THEMIS_HTTP_PORT: "${PORT:-8080}"
      THEMIS_WIRE_PORT: "18765"
```

**See Also:** [examples/railway/docker-compose.railway.yml](../../examples/railway/docker-compose.railway.yml)

---

## Port Conflict Resolution

### Common Conflicts

| Port | Common Conflict | Resolution |
|------|----------------|------------|
| 8080 | Jenkins, Tomcat, other web apps | Map to 8081: `"8081:8080"` |
| 5432 | PostgreSQL | Map to 5433: `"5433:5432"` |
| 1883 | Mosquitto, other MQTT brokers | Map to 1884: `"1884:1883"` |
| 3000 | Node.js dev servers | Map to 3001: `"3001:3000"` |

### Example: Resolving Port 8080 Conflict

```yaml
services:
  themisdb:
    ports:
      - "8081:8080"  # Map host 8081 to container 8080
      - "18765:18765"
```

**Access:**
```bash
curl http://localhost:8081/health  # Note: 8081 instead of 8080
```

---

## Kubernetes Port Configuration

```yaml
apiVersion: v1
kind: Service
metadata:
  name: themisdb
spec:
  type: LoadBalancer
  ports:
    - name: http
      port: 8080
      targetPort: 8080
    - name: wire
      port: 18765
      targetPort: 18765
    - name: metrics
      port: 4318
      targetPort: 4318
    # Add optional ports as needed
  selector:
    app: themisdb
```

---

## Security Best Practices

### 1. Minimize Exposed Ports

**Default (Secure):**
```yaml
ports:
  - "8080:8080"   # Only HTTP API
  - "18765:18765" # Only Wire Protocol
```

**Avoid (Unless Needed):**
```yaml
ports:
  - "1883:1883"   # MQTT plain - unencrypted!
```

### 2. Use TLS for All Protocols

**MQTT:**
- Prefer port 8883 (MQTT TLS) over 1883 (plain)
- Configure client certificates

**PostgreSQL Wire:**
- Enable SSL in config: `"postgres_ssl_enabled": true`
- Use certificate-based authentication

**MCP:**
- Use HTTPS/WSS transports in production
- Configure authentication tokens

### 3. Network Isolation

**Docker Network:**
```yaml
networks:
  themisdb_internal:
    driver: bridge
    internal: true  # No external access

services:
  themisdb:
    networks:
      - themisdb_internal
    ports:
      - "8080:8080"  # Only HTTP exposed externally
```

### 4. Firewall Rules

**Allow only necessary ports:**
```bash
# UFW (Ubuntu)
ufw allow 8080/tcp    # HTTP API
ufw allow 18765/tcp   # Wire Protocol
ufw deny 1883/tcp     # Block unencrypted MQTT

# iptables
iptables -A INPUT -p tcp --dport 8080 -j ACCEPT
iptables -A INPUT -p tcp --dport 18765 -j ACCEPT
iptables -A INPUT -p tcp --dport 1883 -j DROP
```

---

## Monitoring and Health Checks

### Port Availability Checks

```bash
# Check if ports are listening
netstat -tuln | grep -E '8080|18765|4318'

# Check HTTP endpoint
curl -f http://localhost:8080/health || echo "HTTP port not responding"

# Check Wire Protocol (telnet)
telnet localhost 18765
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

### Prometheus Metrics

**Port Status Metrics:**
```
themis_http_port_open{port="8080"} 1
themis_wire_port_open{port="18765"} 1
themis_mqtt_port_open{port="1883"} 1
themis_postgres_port_open{port="5432"} 1
themis_mcp_port_open{port="3000"} 1
```

**Query:**
```promql
# Check all ports are open
sum(themis_http_port_open + themis_wire_port_open) == 2
```

---

## Troubleshooting

### Port Already in Use

**Error:**
```
Error starting userland proxy: listen tcp 0.0.0.0:8080: bind: address already in use
```

**Solution 1 - Find and stop conflicting process:**
```bash
# Find process using port 8080
lsof -i :8080
# or
netstat -tuln | grep 8080

# Kill the process
kill -9 <PID>
```

**Solution 2 - Use different host port:**
```yaml
ports:
  - "8081:8080"  # Map host 8081 to container 8080
```

### Port Not Accessible from Host

**Check container logs:**
```bash
docker logs themisdb-server
```

**Check port binding:**
```bash
docker port themisdb-server
```

**Check firewall:**
```bash
# Linux
sudo ufw status
sudo iptables -L

# macOS
sudo pfctl -s rules
```

### Protocol Not Available

**Error:**
```
Connection refused on port 5432
```

**Cause:** PostgreSQL Wire Protocol not enabled at build time

**Solution:**
1. Rebuild with protocol flag:
   ```bash
   cmake -B build -S . -DTHEMIS_ENABLE_POSTGRES_WIRE=ON
   docker build --build-arg ENABLE_POSTGRES_WIRE=ON -t themisdb:custom .
   ```

2. Update docker-compose.yml:
   ```yaml
   services:
     themisdb:
       image: themisdb:custom
       ports:
         - "5432:5432"
   ```

---

## References

- [OPTIONAL_PROTOCOLS.md](../apis/OPTIONAL_PROTOCOLS.md) - Detailed protocol documentation
- [MCP_PROTOCOL_SUPPORT.md](../apis/MCP_PROTOCOL_SUPPORT.md) - MCP integration guide
- [DOCKER_DEPLOYMENT.md](DOCKER_DEPLOYMENT.md) - Docker deployment guide
- [docker-compose.yml](../../docker-compose.yml) - Reference docker-compose configuration
- [Dockerfile](../../Dockerfile) - Main Dockerfile with port definitions

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.3.0 | 2024-12-21 | Initial port reference documentation |

---

**Questions or Issues?**  
Open an issue on [GitHub](https://github.com/makr-code/ThemisDB/issues) or refer to [CONTRIBUTING.md](../../CONTRIBUTING.md)
