# ThemisDB Port Mapping Reference

This document describes all network ports used by ThemisDB and provides examples for deployment configurations.

## Default Port Mapping

### Core Services

| Port | Service | Protocol | Description | Default Bind |
|------|---------|----------|-------------|--------------|
| **8765** | HTTP API | TCP/HTTP | Main REST/AQL endpoint | 0.0.0.0 |
| **9090** | Health/Error Service | TCP/HTTP | Independent diagnostics service | 127.0.0.1 |

### Optional Services

| Port | Service | Protocol | Enabled By | Default Bind |
|------|---------|----------|------------|--------------|
| 8080 | Alternative HTTP | TCP/HTTP | Configuration | 0.0.0.0 |
| 50051 | gRPC | TCP/gRPC | `THEMIS_ENABLE_GRPC=ON` | 0.0.0.0 |
| 9091 | Prometheus Metrics | TCP/HTTP | Metrics enabled | 0.0.0.0 |
| Custom | HTTP/3 (QUIC) | UDP | `enable_http3: true` | Configurable |

## Health/Error Service (Port 9090)

The Health/Error Service runs on a **separate port** to ensure diagnostics remain accessible even when the main HTTP server experiences issues.

### Features
- Runs independently on port 9090 (default)
- Binds to localhost (127.0.0.1) by default for security
- Provides error introspection and health check endpoints
- Minimal dependencies (no RocksDB, LLM, or external services)

### Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Overall system health status |
| `/health/components` | GET | Individual component health details |
| `/errors` | GET | List all registered error codes |
| `/errors/:code` | GET | Get specific error details |
| `/errors/categories` | GET | List error categories |
| `/errors/search?q=keyword` | GET | Search errors by keyword |

## Configuration

### Environment Variables

```bash
# Main HTTP server port (default: 8765)
export THEMIS_HTTP_PORT=8765

# Health/Error service port (default: 9090)
export THEMIS_HEALTH_PORT=9090

# Health/Error service bind address (default: 127.0.0.1)
export THEMIS_HEALTH_BIND_ADDRESS="127.0.0.1"
```

### Configuration File (config.yaml)

```yaml
server:
  # Main HTTP API
  host: "0.0.0.0"
  port: 8765
  
  # Health/Error Service Configuration
  health_error_service_enabled: true
  health_error_service_bind_address: "127.0.0.1"  # Localhost for security
  health_error_service_port: 9090
```

## Docker Deployment

### Docker Compose

```yaml
services:
  themisdb:
    image: themisdb:latest
    ports:
      - "8765:8765"  # Main HTTP API
      - "9090:9090"  # Health/Error Service
    environment:
      - THEMIS_HTTP_PORT=8765
      - THEMIS_HEALTH_PORT=9090
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:9090/health"]
      interval: 30s
      timeout: 10s
      retries: 3
```

## Kubernetes Deployment

### Service with Probes

```yaml
apiVersion: v1
kind: Service
metadata:
  name: themisdb
spec:
  ports:
    - name: http
      port: 8765
      targetPort: 8765
    - name: health
      port: 9090
      targetPort: 9090
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb
spec:
  template:
    spec:
      containers:
      - name: themisdb
        ports:
        - containerPort: 8765
        - containerPort: 9090
        livenessProbe:
          httpGet:
            path: /health
            port: 9090
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/v1/health
            port: 8765
          initialDelaySeconds: 10
          periodSeconds: 5
```

## Troubleshooting

### Check Port Usage

```bash
# Check if ports are in use
netstat -tuln | grep -E "8765|9090"
```

### Test Health Service

```bash
# Test health endpoint
curl http://localhost:9090/health

# Test from Docker container
docker exec themisdb curl http://localhost:9090/health
```

## Security Best Practices

1. **Bind to localhost in production**: `health_error_service_bind_address: "127.0.0.1"`
2. **Use internal networking in containers**: Don't expose port 9090 externally
3. **Implement rate limiting**: Prevent abuse of diagnostic endpoints

## References

- [Main Configuration Guide](../../config/config.yaml)
- [Docker Deployment Guide](../../docker/README.md)
- [Kubernetes Probes Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)
