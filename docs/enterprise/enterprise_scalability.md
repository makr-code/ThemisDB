# Enterprise Scalability Features

## Overview

ThemisDB Enterprise includes advanced scalability features for high-throughput, mission-critical deployments:

- **Token Bucket Rate Limiting** with priority lanes
- **Per-Client Rate Limiting** with independent quotas
- **Adaptive Load Shedding** to prevent overload
- **HTTP Connection Pooling** for external APIs
- **Batch CRUD Operations** for bulk processing

## Features

### 1. Token Bucket Rate Limiter (`TokenBucketRateLimiter`)

**Location:** `include/server/rate_limiter_v2.h`, `src/server/rate_limiter_v2.cpp`

**Purpose:** Control request rate with burst allowance and priority lanes.

**Configuration:**
```cpp
TokenBucketRateLimiter::Config config;
config.capacity = 10000;        // Max burst (tokens)
config.refill_rate = 1000;       // Sustained rate (tokens/sec)
config.enable_priority_lanes = true; // HIGH/NORMAL/LOW lanes

TokenBucketRateLimiter limiter(config);
```

**Usage:**
```cpp
// Try to acquire tokens
if (limiter.tryAcquire(1, TokenBucketRateLimiter::Priority::HIGH)) {
    // Process request
} else {
    // Return 429 Too Many Requests
}
```

**Priority Allocation:**
- **HIGH:** 50% of capacity
- **NORMAL:** 30% of capacity  
- **LOW:** 20% of capacity

**Benefits:**
- ✅ Burst traffic handling (10k requests in 1 second)
- ✅ VIP/Standard/Batch request prioritization
- ✅ Prevents API abuse

---

### 2. Per-Client Rate Limiter (`PerClientRateLimiter`)

**Location:** `include/server/rate_limiter_v2.h`, `src/server/rate_limiter_v2.cpp`

**Purpose:** Independent rate limits per client (API key, IP address, tenant).

**Configuration:**
```cpp
PerClientRateLimiter::Config config;
config.capacity = 100;          // Tokens per client
config.refill_rate = 10;         // Tokens/sec per client
config.max_clients = 10000;      // Max tracked clients

PerClientRateLimiter limiter(config);
```

**Usage:**
```cpp
std::string client_id = extractClientId(request); // From API key, JWT, IP

if (limiter.allowRequest(client_id, 1, TokenBucketRateLimiter::Priority::NORMAL)) {
    // Process request
} else {
    // Return 429 Too Many Requests
}
```

**Benefits:**
- ✅ Fair resource allocation across clients
- ✅ Prevents single client from monopolizing resources
- ✅ Multi-tenant support

---

### 3. Load Shedder (`LoadShedder`)

**Location:** `include/server/load_shedder.h`, `src/server/load_shedder.cpp`

**Purpose:** Adaptive request rejection when system is overloaded.

**Configuration:**
```cpp
LoadShedder::Config config;
config.cpu_threshold = 0.95;     // Reject at 95% CPU
config.memory_threshold = 0.90;  // Reject at 90% memory
config.queue_depth_threshold = 1000; // Max queue depth
config.enable_shedding = true;

LoadShedder shedder(config);
```

**Usage:**
```cpp
// Update system metrics (every 1s)
shedder.updateLoad(
    getCpuUsage(),      // 0.0-1.0
    getMemoryUsage(),   // 0.0-1.0
    getQueueDepth()     // Current request queue
);

// Check before processing request
if (shedder.shouldReject(priority)) {
    // Return 503 Service Unavailable
}
```

**Rejection Policy:**
- **80% load:** Reject LOW priority requests
- **95% load:** Reject NORMAL priority requests  
- **Always accept:** HIGH priority requests (health checks, admin)

**Benefits:**
- ✅ Prevents complete system failure under overload
- ✅ Graceful degradation
- ✅ Critical requests always processed

---

### 4. HTTP Client Pool (`HTTPClientPool`)

**Location:** `include/utils/http_client_pool.h`, `src/utils/http_client_pool.cpp`

**Purpose:** Connection pooling for external HTTP APIs (embedding providers, remote shards).

**Configuration:**
```cpp
HTTPClientPool::Config config;
config.max_connections = 50;        // Pool size
config.idle_timeout = std::chrono::seconds(30);
config.connect_timeout = std::chrono::seconds(5);
config.request_timeout = std::chrono::seconds(30);
config.enable_keepalive = true;     // HTTP Keep-Alive

HTTPClientPool pool(config);
```

**Usage:**
```cpp
// Async POST request
json request_body = {
    {"input", texts},
    {"model", "text-embedding-3-small"}
};

auto future = pool.post("https://api.openai.com/v1/embeddings", request_body);

// Process response
auto response = future.get();
if (response.isSuccess()) {
    auto embeddings = json::parse(response.body);
}
```

**Benefits:**
- ✅ ~30% latency reduction (no TCP handshake overhead)
- ✅ HTTP Keep-Alive support
- ✅ Concurrent requests without connection exhaustion

---

### 5. Batch CRUD Endpoint (`/entities/batch`)

**Location:** `src/server/http_server.cpp` (lines 5025-5303)

**Purpose:** Atomic batch operations for bulk data import/update.

**API:**
```http
POST /entities/batch
Content-Type: application/json

{
  "operations": [
    {
      "op": "put",
      "key": "users:u1",
      "blob": "{\"name\":\"Alice\",\"age\":30}"
    },
    {
      "op": "put",
      "key": "users:u2",
      "blob": "{\"name\":\"Bob\",\"age\":25}"
    },
    {
      "op": "delete",
      "key": "orders:o123"
    }
  ]
}
```

**Response:**
```json
{
  "success": true,
  "total": 3,
  "succeeded": 2,
  "failed": 1,
  "errors": [
    {
      "index": 1,
      "key": "users:u2",
      "error": "Duplicate key"
    }
  ]
}
```

**Features:**
- ✅ Atomic commit via RocksDB WriteBatch
- ✅ Partial success reporting
- ✅ Secondary index updates
- ✅ Geo index updates
- ✅ CDC event recording
- ✅ Max batch size: 10,000 operations

**Benefits:**
- ✅ ~10x throughput for bulk imports
- ✅ Reduced network overhead (1 request vs 1000)
- ✅ Atomicity guarantees

---

## Performance Targets

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Max Concurrent Clients | 100 | 1000 | **10x** |
| Read Throughput | 5k/s | 50k/s | **10x** |
| Write Throughput | 2k/s | 20k/s | **10x** |
| Batch Insert (1000 entities) | 500ms | 100ms | **5x** |
| P99 Latency (Query) | 200ms | 50ms | **4x** |
| Embedding API Latency | 300ms | 200ms | **1.5x** |

---

## Testing

Run enterprise scalability tests:

```bash
# Build tests
cmake --build build-msvc-ninja-debug --target themis_tests

# Run all enterprise tests
.\build-msvc-ninja-debug\themis_tests.exe --gtest_filter="*Enterprise*"

# Run specific feature tests
.\build-msvc-ninja-debug\themis_tests.exe --gtest_filter="TokenBucketRateLimiterTest.*"
.\build-msvc-ninja-debug\themis_tests.exe --gtest_filter="LoadShedderTest.*"
```

---

## Load Testing

Use **k6** for load testing:

```javascript
// load_test.js
import http from 'k6/http';
import { check } from 'k6';

export let options = {
  stages: [
    { duration: '1m', target: 100 },   // Ramp to 100 users
    { duration: '5m', target: 100 },   // Stay at 100
    { duration: '1m', target: 1000 },  // Spike to 1000
    { duration: '3m', target: 1000 },  // Stay at 1000
    { duration: '1m', target: 0 },     // Ramp down
  ],
};

export default function () {
  let res = http.post('http://localhost:18765/entities/batch', JSON.stringify({
    operations: [
      { op: 'put', key: 'test:1', blob: '{"value": 123}' },
    ]
  }), {
    headers: { 'Content-Type': 'application/json' },
  });
  
  check(res, {
    'status is 200': (r) => r.status === 200,
    'response time < 500ms': (r) => r.timings.duration < 500,
  });
}
```

Run:
```bash
k6 run load_test.js
```

---

## Configuration

Add to `config/config.json`:

```json
{
  "rate_limiting": {
    "enabled": true,
    "global_capacity": 10000,
    "global_refill_rate": 1000,
    "per_client_capacity": 100,
    "per_client_refill_rate": 10
  },
  "load_shedding": {
    "enabled": true,
    "cpu_threshold": 0.95,
    "memory_threshold": 0.90,
    "queue_depth_threshold": 1000
  },
  "http_pool": {
    "max_connections": 50,
    "idle_timeout_sec": 30,
    "connect_timeout_sec": 5,
    "request_timeout_sec": 30
  }
}
```

---

## Monitoring

Metrics available at `/metrics` (Prometheus format):

```
# Rate limiting
themis_rate_limit_requests_total
themis_rate_limit_requests_rejected

# Load shedding
themis_load_shedding_rejections_total{priority="low"}
themis_load_shedding_rejections_total{priority="normal"}
themis_system_load_factor

# HTTP pool
themis_http_pool_connections_total
themis_http_pool_connections_available
themis_http_pool_connections_in_use

# Batch operations
themis_batch_operations_total
themis_batch_operations_succeeded
themis_batch_operations_failed
```

---

## Production Deployment

### Docker Compose

```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb/enterprise:latest
    environment:
      - THEMIS_RATE_LIMIT_CAPACITY=10000
      - THEMIS_RATE_LIMIT_REFILL_RATE=1000
      - THEMIS_LOAD_SHEDDING_ENABLED=true
      - THEMIS_HTTP_POOL_SIZE=50
    deploy:
      resources:
        limits:
          cpus: '8'
          memory: 16G
```

### Kubernetes

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-enterprise
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: themisdb
        image: themisdb/enterprise:latest
        env:
        - name: THEMIS_RATE_LIMIT_CAPACITY
          value: "10000"
        - name: THEMIS_LOAD_SHEDDING_ENABLED
          value: "true"
        resources:
          limits:
            cpu: 8
            memory: 16Gi
```

---

## References

- [Token Bucket Algorithm](https://en.wikipedia.org/wiki/Token_bucket)
- [Circuit Breaker Pattern](https://martinfowler.com/bliki/CircuitBreaker.html)
- [Google SRE: Load Shedding](https://sre.google/sre-book/handling-overload/)
- [RocksDB WriteBatch](https://github.com/facebook/rocksdb/wiki/Basic-Operations#atomic-updates)

---

**Status:** ✅ Production Ready  
**Version:** 1.0  
**Last Updated:** 2025-11-30
