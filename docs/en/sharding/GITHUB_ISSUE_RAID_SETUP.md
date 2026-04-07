# GitHub Issue: ThemisDB RAID Setup and Monitoring Integration

## Issue Type
🐛 **Bug Report / Configuration Issue**

## Priority
🔴 **High** - Affects hyperscaler edition monitoring and RAID functionality

## Labels
- `bug`
- `docker`
- `raid`
- `monitoring`
- `prometheus`
- `grafana`
- `hyperscaler-edition`

---

## 📋 Issue Summary

ThemisDB RAID cluster setup experiences multiple integration and configuration issues preventing proper monitoring and cluster operation in Docker environment.

### Affected Components
- Docker RAID cluster configuration
- Prometheus metrics integration
- Grafana dashboard connectivity
- Shard referencing and coordination
- Hyperscaler edition features

---

## 🔍 Problem Description

### 1. Prometheus Integration Issues

**Problem:** Grafana cannot connect to Prometheus, metrics endpoints not responding

**Symptoms:**
- Prometheus targets showing as "down"
- `/metrics` endpoint returns 404 or connection refused
- Grafana dashboards display "No Data"
- RAID performance metrics unavailable

**Root Cause:**
- Docker image contains Windows executable (`themis_server.exe`) instead of Linux binary
- Metrics endpoint configured on wrong port (9090 vs 8080)
- `THEMIS_ENABLE_METRICS` environment variable not properly set

**Evidence:**
```bash
# Current broken state
curl http://localhost:9090/metrics
# Connection refused or 404

# Prometheus scrape config points to wrong port
targets:
  - 'themis-raid0-shard1:9090'  # ❌ Wrong port
```

### 2. Shard Configuration Issues

**Problem:** Inconsistent shard referencing across RAID groups

**Symptoms:**
- RAID0 shard1 uses `hyperscaler` image, but shards 2-3 use `latest`
- Shard discovery fails during cluster initialization
- Cross-shard queries timeout
- Data distribution unbalanced

**Configuration Inconsistency:**
```yaml
# docker-compose-sharding.yml
themis-raid0-shard1:
  image: themisdb/themisdb:hyperscaler  # ✅ Has metrics
  
themis-raid0-shard2:
  image: themisdb/themisdb:latest       # ❌ May lack features
```

### 3. Docker Build Architecture Mismatch

**Problem:** Docker image built with Windows binary, deployed on Linux containers

**Details:**
- Build uses `build-msvc/Release/themis_server.exe` (PE32+)
- Runtime container is `ubuntu:24.04` (Linux)
- Binary cannot execute in container
- REST API and metrics endpoints unavailable

**Current Dockerfile Issue:**
```dockerfile
# Problematic pattern
COPY build-msvc/Release/themis_server.exe /usr/local/bin/
# Windows binary in Linux container ❌
```

### 4. Disabled Hyperscaler Features

**Problem:** Critical features disabled in hyperscaler edition build

**Missing Features:**
- Advanced RAID synchronization
- Multi-shard transaction coordination
- Performance optimization modules
- Enhanced monitoring capabilities

**Build Configuration Issues:**
```cmake
# Some features incorrectly disabled
option(THEMIS_ENABLE_RAID_SYNC "Enable RAID sync" OFF)  # Should be ON
option(THEMIS_ENABLE_SHARDING_METRICS "Enable metrics" OFF)  # Should be ON
```

---

## 🏗️ RAID Architecture Overview

### Current Setup (docker-compose-sharding.yml)

ThemisDB implements three RAID modes for different use cases:

#### RAID 0 (Striping) - Performance
- **Nodes:** 3 shards (raid0-shard1, raid0-shard2, raid0-shard3)
- **Ports:** 18765-18767 (Wire Protocol), 8080-8082 (REST API)
- **Purpose:** Maximum throughput by distributing data
- **Redundancy:** None (data loss if any node fails)

```yaml
environment:
  THEMIS_RAID_MODE: "stripe"
  THEMIS_RAID_GROUP: "raid0"
  THEMIS_SHARDS: "themis-raid0-shard1:18765,themis-raid0-shard2:18765,themis-raid0-shard3:18765"
```

#### RAID 1 (Mirroring) - Redundancy
- **Nodes:** 2 shards (raid1-primary, raid1-secondary)
- **Ports:** 18768-18769 (Wire Protocol), 8083-8084 (REST API)
- **Purpose:** Data redundancy and high availability
- **Redundancy:** 100% (complete data copy on both nodes)

```yaml
environment:
  THEMIS_RAID_MODE: "mirror"
  THEMIS_RAID_GROUP: "raid1"
  THEMIS_MIRROR_PEER: "themis-raid1-secondary:18765"
```

#### RAID 5 (Striping + Parity) - Balance
- **Nodes:** 3 shards (raid5-shard1, raid5-shard2, raid5-shard3)
- **Ports:** 18770-18772 (Wire Protocol), 8085-8087 (REST API)
- **Purpose:** Balance between performance and redundancy
- **Redundancy:** (n-1)/n (can survive one node failure)

```yaml
environment:
  THEMIS_RAID_MODE: "parity"
  THEMIS_RAID_GROUP: "raid5"
  THEMIS_SHARDS: "themis-raid5-shard1:18765,themis-raid5-shard2:18765,themis-raid5-shard3:18765"
```

### Shard Communication Model

```
┌─────────────────────────────────────────────────────────┐
│              ThemisDB RAID Cluster                      │
│                 (Docker Network)                         │
├─────────────────────────────────────────────────────────┤
│                                                          │
│   Client Application                                    │
│         ↓                                               │
│   Load Balancer / Router                               │
│         ↓                                               │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│   │ RAID0-S1 │──│ RAID0-S2 │──│ RAID0-S3 │            │
│   │ :18765   │  │ :18765   │  │ :18765   │            │
│   │ :8080    │  │ :8080    │  │ :8080    │            │
│   └──────────┘  └──────────┘  └──────────┘            │
│        │             │              │                   │
│        └─────────────┴──────────────┘                  │
│              Data Striping                              │
│                                                          │
│   ┌──────────┐          ┌──────────┐                   │
│   │ RAID1-P  │◄────────►│ RAID1-S  │                   │
│   │ :18768   │  Mirror  │ :18769   │                   │
│   │ :8083    │          │ :8084    │                   │
│   └──────────┘          └──────────┘                   │
│                                                          │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│   │ RAID5-S1 │──│ RAID5-S2 │──│ RAID5-S3 │            │
│   │ :18770   │  │ :18771   │  │ :18772   │            │
│   │ :8085    │  │ :8086    │  │ :8087    │            │
│   └─────┬────┘  └─────┬────┘  └─────┬────┘            │
│         │             │              │                   │
│         └──────Parity Striping───────┘                 │
│                       ↓                                  │
│              Prometheus:9090                            │
│                       ↓                                  │
│               Grafana:3000                              │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

---

## 🔧 Technical Details

### Port Mapping Summary

| Container | Host Port | Container Port | Service | Status |
|-----------|-----------|----------------|---------|--------|
| raid0-shard1 | 18765 | 18765 | Wire Protocol | ✅ |
| raid0-shard1 | 8080 | 8080 | REST API + /metrics | ❌ Not working |
| raid0-shard1 | 9091 | 9090 | Metrics (unused) | ⚠️ Misconfigured |
| raid0-shard2 | 18766 | 18765 | Wire Protocol | ✅ |
| raid0-shard2 | 8081 | 8080 | REST API + /metrics | ❌ Not working |
| raid0-shard3 | 18767 | 18765 | Wire Protocol | ✅ |
| raid0-shard3 | 8082 | 8080 | REST API + /metrics | ❌ Not working |
| raid1-primary | 18768 | 18765 | Wire Protocol | ✅ |
| raid1-primary | 8083 | 8080 | REST API + /metrics | ❌ Not working |
| raid1-secondary | 18769 | 18765 | Wire Protocol | ✅ |
| raid1-secondary | 8084 | 8080 | REST API + /metrics | ❌ Not working |
| raid5-shard1 | 18770 | 18765 | Wire Protocol | ✅ |
| raid5-shard1 | 8085 | 8080 | REST API + /metrics | ❌ Not working |
| raid5-shard2 | 18771 | 18765 | Wire Protocol | ✅ |
| raid5-shard2 | 8086 | 8080 | REST API + /metrics | ❌ Not working |
| raid5-shard3 | 18772 | 18765 | Wire Protocol | ✅ |
| raid5-shard3 | 8087 | 8080 | REST API + /metrics | ❌ Not working |
| prometheus | 9090 | 9090 | Prometheus Server | ✅ |
| grafana | 3000 | 3000 | Grafana UI | ✅ (No data) |

### Metrics Endpoint Details

**Expected Behavior:**
```bash
curl http://localhost:8080/metrics
# Should return Prometheus format metrics

# Example expected output:
# HELP themis_raid_io_bytes_total Total bytes processed by RAID
# TYPE themis_raid_io_bytes_total counter
themis_raid_io_bytes_total{raid_mode="stripe",shard_id="raid0-1"} 1234567890

# HELP themis_operation_duration_seconds Operation duration
# TYPE themis_operation_duration_seconds histogram
themis_operation_duration_seconds_bucket{le="0.001"} 1250
themis_operation_duration_seconds_bucket{le="0.01"} 2450
```

**Current Behavior:**
```bash
curl http://localhost:8080/metrics
# curl: (7) Failed to connect to localhost port 8080: Connection refused
```

### Environment Variables Required

```yaml
environment:
  # Core Configuration
  THEMIS_PORT: "18765"                    # Wire protocol port
  THEMIS_ROLE: "shard"                    # Node role
  THEMIS_SHARD_ID: "raid0-1"             # Unique shard identifier
  
  # RAID Configuration
  THEMIS_RAID_MODE: "stripe"              # stripe|mirror|parity
  THEMIS_RAID_GROUP: "raid0"              # RAID group identifier
  THEMIS_SHARDS: "host1:port,host2:port" # Cluster members
  
  # Metrics Configuration
  THEMIS_ENABLE_METRICS: "true"           # Enable Prometheus metrics
  THEMIS_METRICS_PORT: "9090"             # Internal metrics port
  
  # Storage
  THEMIS_DATA_DIR: "/var/lib/themisdb"   # Data directory
```

---

## 📊 Monitoring Stack Configuration

### Prometheus Configuration (prometheus.yml)

**Current (Broken):**
```yaml
scrape_configs:
  - job_name: 'raid0-stripe'
    static_configs:
      - targets:
        - 'themis-raid0-shard1:9090'  # ❌ Wrong port
        - 'themis-raid0-shard2:9090'
        - 'themis-raid0-shard3:9090'
```

**Required (Fixed):**
```yaml
scrape_configs:
  - job_name: 'raid0-stripe'
    static_configs:
      - targets:
        - 'themis-raid0-shard1:8080'  # ✅ Correct port
        - 'themis-raid0-shard2:8080'
        - 'themis-raid0-shard3:8080'
    metrics_path: '/metrics'           # ✅ Explicit path
    scrape_interval: 15s
    scrape_timeout: 10s
    
  - job_name: 'raid1-mirror'
    static_configs:
      - targets:
        - 'themis-raid1-primary:8080'
        - 'themis-raid1-secondary:8080'
    metrics_path: '/metrics'
    scrape_interval: 15s
    
  - job_name: 'raid5-parity'
    static_configs:
      - targets:
        - 'themis-raid5-shard1:8080'
        - 'themis-raid5-shard2:8080'
        - 'themis-raid5-shard3:8080'
    metrics_path: '/metrics'
    scrape_interval: 15s
```

### Grafana Dashboard Configuration

**Dashboard Location:** `docker/compose/grafana/dashboards/themis_raid_benchmark_dashboard.json`

**Key Panels:**
1. **RAID I/O Throughput** - `rate(themis_raid_io_bytes_total[5m])`
2. **Operation Latency (p95/p99)** - `histogram_quantile(0.95, themis_operation_duration_seconds)`
3. **Operations/sec** - `rate(themis_io_operations_total[1m])`
4. **Cluster Health** - `themis_shard_health_status`

---

## 🔨 Proposed Solutions

### Solution 1: Fix Docker Image Build

**Create:** `Dockerfile.themis-metrics-enabled`

```dockerfile
FROM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    pkg-config \
    curl \
    zip \
    unzip \
    tar

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh

# Copy source
WORKDIR /build
COPY . .

# Configure with metrics enabled
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_ENABLE_METRICS=ON \
    -DTHEMIS_ENABLE_PROMETHEUS=ON \
    -DTHEMIS_ENABLE_SHARDING=ON \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
RUN cmake --build build --config Release --target themis_server -j$(nproc)

# Runtime stage
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/build/Release/themis_server /usr/local/bin/

EXPOSE 18765 8080 9090

ENV THEMIS_ENABLE_METRICS=true
ENV THEMIS_METRICS_PORT=9090

ENTRYPOINT ["/usr/local/bin/themis_server"]
```

**Build Command:**
```bash
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled .
```

### Solution 2: Update Docker Compose Configuration

**File:** `docker/compose/docker-compose-sharding.yml`

**Changes Required:**

1. **Use consistent image across all shards:**
```yaml
services:
  themis-raid0-shard1:
    image: themisdb/themisdb:metrics-enabled  # ✅ Updated
    
  themis-raid0-shard2:
    image: themisdb/themisdb:metrics-enabled  # ✅ Updated
    
  themis-raid0-shard3:
    image: themisdb/themisdb:metrics-enabled  # ✅ Updated
```

2. **Ensure all containers expose port 8080:**
```yaml
ports:
  - "18765:18765"   # Wire Protocol
  - "8080:8080"     # REST API + /metrics ✅
  - "9091:9090"     # Optional metrics port
```

### Solution 3: Fix Prometheus Configuration

**File:** `docker/compose/prometheus.yml`

Update all targets to use port 8080 with explicit `/metrics` path.

### Solution 4: Enable Hyperscaler Features

**File:** `CMakeLists.txt`

```cmake
# Ensure features are enabled for hyperscaler edition
option(THEMIS_ENABLE_RAID_SYNC "Enable RAID synchronization" ON)
option(THEMIS_ENABLE_SHARDING_METRICS "Enable sharding metrics" ON)
option(THEMIS_ENABLE_PROMETHEUS "Enable Prometheus metrics" ON)
option(THEMIS_ENABLE_CLUSTER_COORDINATION "Enable cluster features" ON)

# Hyperscaler edition specific
if(THEMIS_HYPERSCALER_EDITION)
    set(THEMIS_ENABLE_RAID_SYNC ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_SHARDING_METRICS ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_PROMETHEUS ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_CLUSTER_COORDINATION ON CACHE BOOL "" FORCE)
endif()
```

---

## ✅ Verification Steps

### 1. Verify Docker Image
```bash
# Build new image
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled .

# Verify binary is Linux ELF
docker run --rm themisdb/themisdb:metrics-enabled file /usr/local/bin/themis_server
# Expected: ELF 64-bit LSB executable, x86-64
```

### 2. Test Metrics Endpoint
```bash
# Start single container for testing
docker run -d --name test-metrics \
    -p 8080:8080 \
    -e THEMIS_ENABLE_METRICS=true \
    themisdb/themisdb:metrics-enabled

# Wait for startup
sleep 10

# Test metrics endpoint
curl http://localhost:8080/metrics

# Should return Prometheus metrics
# Expected: HTTP 200, content-type: text/plain
```

### 3. Verify RAID Cluster
```bash
# Start full cluster
cd docker/compose
docker-compose -f docker-compose-sharding.yml up -d

# Wait for cluster initialization
sleep 30

# Check all shard health
for port in 8080 8081 8082 8083 8084 8085 8086 8087; do
    echo "Checking port $port:"
    curl -s http://localhost:$port/health | jq .
done

# All should return: {"status": "healthy", "shard_id": "..."}
```

### 4. Verify Prometheus Scraping
```bash
# Check Prometheus targets
curl -s http://localhost:9090/api/v1/targets | \
    jq '.data.activeTargets[] | {job: .labels.job, health: .health}'

# All targets should show: "health": "up"
```

### 5. Verify Grafana Dashboard
```bash
# Open Grafana
open http://localhost:3000

# Login: admin / admin
# Navigate to: Dashboards → Themis RAID Benchmark

# Verify panels show data:
# - RAID I/O Throughput: Should show graph
# - Operation Latency: Should show histogram
# - Operations/sec: Should show rate
```

---

## 📚 Related Documentation

### Internal References
- **RAID Setup Guide:** `benchmarks/DOCKER_RAID_IMPLEMENTATION_SUMMARY.md`
- **RAID Quickstart:** `benchmarks/RAID_SHARDING_QUICKSTART.md`
- **Test Plan:** `benchmarks/RAID_SHARDING_TEST_PLAN.md`
- **Prometheus Integration:** `PROMETHEUS_INTEGRATION_COMPLETE.md`
- **Docker Compose:** `docker/compose/docker-compose-sharding.yml`

### Benchmark Files
- **C++ Benchmark:** `benchmarks/bench_docker_raid_comprehensive.cpp`
- **Python Test Suite:** `benchmarks/raid_sharding_test_suite.py`
- **Analysis Tool:** `benchmarks/analyze_raid_benchmarks.py`
- **PowerShell Runner:** `benchmarks/run_docker_raid_benchmark.ps1`

### Configuration Files
- **Prometheus Config:** `docker/compose/prometheus.yml`
- **Grafana Datasources:** `docker/compose/grafana/datasources.yml`
- **Grafana Dashboards:** `docker/compose/grafana/dashboards.yml`
- **RAID Dashboard:** `docker/compose/grafana/dashboards/themis_raid_benchmark_dashboard.json`

---

## 🎯 Acceptance Criteria

Issue is resolved when:

- [ ] Docker image builds successfully with Linux binary
- [ ] All 8 RAID shards start without errors
- [ ] Metrics endpoint (`/metrics`) responds on port 8080 for all shards
- [ ] Prometheus successfully scrapes all targets (all "up")
- [ ] Grafana dashboard displays real-time metrics
- [ ] RAID cluster can process read/write operations
- [ ] Cross-shard queries work correctly
- [ ] Failover scenarios execute within 5 seconds
- [ ] All benchmark tests pass
- [ ] Documentation updated with working configuration

---

## 📝 Testing Plan

### Phase 1: Unit Tests (30 minutes)
1. Build Docker image with metrics enabled
2. Start single shard, verify `/metrics` endpoint
3. Verify Prometheus can scrape metrics
4. Confirm Grafana can query Prometheus

### Phase 2: Integration Tests (2 hours)
1. Start full RAID cluster (8 shards + monitoring)
2. Verify all shards register with each other
3. Test write operations to RAID0
4. Test read operations from RAID1
5. Verify RAID5 parity calculation
6. Confirm metrics reflect operations

### Phase 3: Load Tests (4+ hours)
1. Run comprehensive C++ benchmark suite
2. Execute Python RAID test suite
3. Monitor performance metrics in Grafana
4. Verify system stability under load
5. Test failover scenarios
6. Validate data integrity after failover

---

## 🚨 Impact Assessment

### Current Impact
- **Severity:** High
- **Users Affected:** All hyperscaler edition users
- **Features Impacted:** 
  - Monitoring and observability
  - RAID performance tracking
  - Cluster health management
  - Production debugging capabilities

### Business Impact
- Cannot monitor production RAID clusters
- No visibility into performance bottlenecks
- Difficult to troubleshoot issues
- Limited capacity planning data
- Reduced operational confidence

---

## 🔍 Debugging Information

### Container Logs
```bash
# Check for errors in shard logs
docker logs themis-raid0-shard1 --tail 100

# Common error patterns to look for:
# - "Failed to bind to port 8080"
# - "Metrics registry not initialized"
# - "Unable to connect to peer shard"
# - "Exec format error" (Windows binary issue)
```

### Network Connectivity
```bash
# Verify internal Docker network
docker network inspect themis-network

# Test connectivity between shards
docker exec themis-raid0-shard1 \
    curl -s http://themis-raid0-shard2:8080/health
```

### Prometheus Debugging
```bash
# Check Prometheus configuration
curl http://localhost:9090/api/v1/status/config

# View scrape errors
curl http://localhost:9090/api/v1/targets | \
    jq '.data.activeTargets[] | select(.health != "up")'
```

---

## 👥 Additional Context

### Team Coordination
- **Platform Team:** Docker image build and deployment
- **Backend Team:** Metrics implementation and API endpoints
- **DevOps Team:** Prometheus/Grafana configuration
- **QA Team:** End-to-end testing and validation

### Timeline Estimate
- **Fix Implementation:** 1-2 days
- **Testing:** 1 day
- **Documentation:** 0.5 days
- **Deployment:** 0.5 days
- **Total:** 3-4 days

### Dependencies
- Requires Docker build infrastructure
- Needs access to CI/CD pipeline
- Requires Prometheus/Grafana expertise
- Depends on vcpkg package availability

---

## 📎 Attachments

### Screenshots
_(To be added during testing)_
- [ ] Prometheus targets page (before/after)
- [ ] Grafana dashboard with no data
- [ ] Grafana dashboard with live metrics
- [ ] Docker container logs showing errors

### Configuration Files
- [docker-compose-sharding.yml](../docker/compose/docker-compose-sharding.yml)
- [prometheus.yml](../docker/compose/prometheus.yml)
- [Dockerfile.themis-metrics-enabled](../Dockerfile.themis-metrics-enabled)

---

## 🔗 External References

- [Prometheus Documentation](https://prometheus.io/docs/)
- [Grafana Documentation](https://grafana.com/docs/)
- [Docker Compose Networking](https://docs.docker.com/compose/networking/)
- [RAID Concepts](https://en.wikipedia.org/wiki/RAID)
- [ThemisDB Architecture](../README.md)

---

**Issue Created:** 2026-01-04  
**Last Updated:** 2026-04-06  
**Status:** Open  
**Assignee:** TBD  
**Milestone:** v1.4.0 Hyperscaler Edition

