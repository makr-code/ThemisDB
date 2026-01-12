---
name: 🏥 Separate Health/Error Service on Alternate Port
about: Run a dedicated health and error introspection service on a separate port for maximum availability
title: "[FEATURE] Separate Health/Error Service on Alternate Port"
labels: ["type:feature", "priority:P3", "area:server", "effort:medium"]
assignees: []
---

## 📋 Summary

Implement a separate, lightweight health and error introspection service running on an alternate port (e.g., 9090) to ensure error diagnostics remain accessible even when the main HTTP server experiences issues.

## 🎯 Problem Statement

Currently, the Error API endpoints (`/api/v1/errors/*`) are integrated into the main HTTP server. While these endpoints are designed to be resilient (using in-memory ErrorRegistry with no external dependencies), they become unavailable if:

- The main HTTP server crashes completely
- The main port (8080) is blocked or unavailable
- The server fails to start due to fatal errors
- Resource exhaustion prevents the main server from accepting connections

For production environments requiring maximum observability, having error diagnostics available on a separate port ensures troubleshooting capabilities even during severe system issues.

## 💡 Proposed Solution

Create a minimal, dedicated service that:

1. **Runs on a separate port** (e.g., 9090) independent of the main HTTP server
2. **Exposes error introspection endpoints:**
   - `GET /errors` - List all registered errors
   - `GET /errors/:code` - Get specific error details
   - `GET /errors/categories` - List error categories
   - `GET /errors/search?q=keyword` - Search errors
3. **Includes basic health check:**
   - `GET /health` - Overall system health status
   - `GET /health/components` - Individual component health status
4. **Minimal dependencies:**
   - Uses ErrorRegistry (already in-memory)
   - No RocksDB, LLM, or other heavy dependencies
   - Lightweight HTTP server (could use a simpler implementation than main server)

## 🏗️ Architecture

### Option 1: Separate Thread with Lightweight Server
```cpp
class HealthErrorService {
    boost::asio::io_context ioc_;
    tcp::acceptor acceptor_;
    std::thread service_thread_;
    
public:
    HealthErrorService(uint16_t port = 9090);
    void start();  // Non-blocking, runs in separate thread
    void stop();
};
```

### Option 2: Embedded HTTP Server
Use a minimal HTTP server implementation (e.g., Crow, httplib) for the health/error service to keep it independent from the main server.

### Option 3: Unix Domain Socket
For containerized deployments, expose via Unix socket for sidecar monitoring.

## 📝 Implementation Details

### Step 1: Create HealthErrorService Class
- Minimal HTTP server on configurable port
- Reuse existing ErrorApiHandler logic
- Add basic health check endpoints

### Step 2: Integration Points
- Start service in HttpServer constructor (or separate startup)
- Configuration via environment variable: `THEMIS_HEALTH_PORT=9090`
- Graceful shutdown coordination

### Step 3: Health Check Endpoints
```cpp
// GET /health
{
  "status": "healthy" | "degraded" | "unhealthy",
  "timestamp": "2026-01-12T05:24:34Z",
  "uptime_seconds": 12345,
  "components": {
    "http_server": "healthy",
    "storage": "healthy",
    "llm": "degraded"
  }
}

// GET /health/components
{
  "http_server": { "status": "healthy", "requests_per_sec": 100 },
  "storage": { "status": "healthy", "connections": 5 },
  "llm": { "status": "degraded", "reason": "High memory usage" }
}
```

### Step 4: Configuration
```yaml
# config/server.yaml
health_error_service:
  enabled: true
  port: 9090
  bind_address: "127.0.0.1"  # Localhost only for security
  endpoints:
    - /health
    - /errors
```

## ✅ Acceptance Criteria

- [ ] Separate service runs on configurable port (default: 9090)
- [ ] Service starts/stops independently of main HTTP server
- [ ] All error introspection endpoints available
- [ ] Basic health check endpoints implemented
- [ ] Service remains accessible when main server is down
- [ ] Minimal resource overhead (< 10MB memory, < 1% CPU)
- [ ] Configuration via environment variables and config file
- [ ] Documentation updated with deployment instructions
- [ ] Docker Compose example with health checks
- [ ] Kubernetes liveness/readiness probe examples

## 🧪 Testing

### Manual Testing
```bash
# Start ThemisDB with health service
THEMIS_HEALTH_PORT=9090 ./themis-server

# Test main server
curl http://localhost:8080/api/v1/errors

# Test health service (should work independently)
curl http://localhost:9090/errors
curl http://localhost:9090/health

# Simulate main server failure
# Kill main server but keep health service running
# Health service should still respond
```

### Integration Tests
- Verify health service starts on correct port
- Test error endpoints via health service
- Verify independence from main server failures
- Test graceful shutdown coordination

## 📊 Success Metrics

- Health service uptime > 99.9%
- Error diagnostics available during main server incidents
- Response time < 10ms for health checks
- Resource overhead < 1% of system resources
- Zero crashes in health service over 30 days

## 🔄 Related Issues

- Error Registry implementation (#378)
- Error API integration (current PR)
- Health check framework
- Observability improvements

## 🎯 Priority

**Priority:** P3 (Low) - Nice to have for production deployments  
**Effort:** Medium (2-3 days)  
**Target Release:** v1.5.0 or later

## 💭 Additional Considerations

### Security
- Bind to localhost by default (127.0.0.1)
- Optional mTLS for external access
- Rate limiting to prevent abuse

### Deployment
- Docker: Expose both ports (8080, 9090)
- Kubernetes: Separate service definition
- Systemd: Independent service unit

### Monitoring
- Prometheus metrics on health service port
- Grafana dashboard for health service
- Alert when health service becomes unavailable

## 📚 References

- [Error API Integration Documentation](../../docs/ERROR_API_INTEGRATION.md)
- [Health Check Best Practices](https://microservices.io/patterns/observability/health-check-api.html)
- [Kubernetes Liveness/Readiness Probes](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)

---

**Note:** This is a future enhancement for production environments. The current Error API implementation (integrated into main HTTP server) is sufficient for most use cases.
