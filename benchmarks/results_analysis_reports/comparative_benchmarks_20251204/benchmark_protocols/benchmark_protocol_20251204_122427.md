> ⚠️ **Historisches Benchmark-Protokoll** – Beschreibt eine einzelne Messreihe vom 2025-12-04.

# Benchmark Protocol Report
**Generated**: 2025-12-04T12:24:11.647437

## Executive Summary

- **System**: Intel(R) Core(TM) i9-10900K CPU @ 3.70GHz
- **RAM**: 0.0 GB
- **Storage**: 1906.42 GB
- **OS**: Microsoft Windows 11 Pro (10.0.26100)

---

## 1. Hardware Specification

### 1.1 Processor

| Property | Value |
|----------|-------|
| **Model** | Intel(R) Core(TM) i9-10900K CPU @ 3.70GHz |
| **Manufacturer** | GenuineIntel |
| **Physical Cores** | 10 |
| **Logical Processors** | 20 |
| **Base Clock Speed** | 3696 MHz |
| **L2 Cache** | 2560 KB |
| **L3 Cache** | 20480 KB |
| **Family** | 207 |
| **Model Number** | 0 |
| **Stepping** | None |

### 1.2 Memory

| Property | Value |
|----------|-------|
| **Total RAM** | 0.0 GB |
| **Number of Modules** | 0 |
| **Module Type** | DDR4 |

#### Memory Modules Details


**Module 1**:
- Capacity: 16 GB
- Manufacturer: Micron
- Speed: 2933 MHz
- Type: DDR4
- Form Factor: DIMM


### 1.3 Storage

| Property | Value |
|----------|-------|
| **Drive** | C: |
| **File System** | NTFS |
| **Total Capacity** | 1906.42 GB |
| **Used Space** | 1631.97 GB |
| **Free Space** | 274.46 GB |
| **Usage** | 85.6% |
| **Available for Benchmarks** | 219.56 GB |

---

## 2. Software & Operating System

### 2.1 Operating System

| Property | Value |
|----------|-------|
| **Name** | Microsoft Windows 11 Pro |
| **Version** | 10.0.26100 |
| **Build** | 26100 |
| **Architecture** | 64bit |

### 2.2 Containerization

| Tool | Version |
|------|---------|
| Docker | Docker version 29.0.1, build eedd969 |
| Docker Compose | Docker Compose version v2.40.3-desktop.1 |

### 2.3 Python Environment

| Property | Value |
|----------|-------|
| **Python Version** | 3.13.6 |
| **Implementation** | CPython |
| **Total Packages Installed** | 423 |

#### Benchmark-Relevant Python Packages

| Package | Version |
|---------|---------|
| aiohttp | 3.13.0 |
| elasticsearch | 9.2.0 |
| grpcio | 1.75.1 |
| httpx | 0.28.1 |
| matplotlib | 3.10.6 |
| numpy | 2.3.4 |
| pandas | 2.3.3 |
| protobuf | 6.32.1 |
| psycopg2-binary | 2.9.11 |
| pymongo | 4.15.5 |
| requests | 2.32.5 |
| scipy | 1.16.2 |


---

## 3. Network Configuration

### 3.1 Active Network Adapters


**vEthernet (WSL (Hyper-V firewall))**:
- Description: Hyper-V Virtual Ethernet Adapter #2
- Link Speed: 10 Gbps


**Ethernet 3**:
- Description: Intel(R) Ethernet Connection (12) I219-V
- Link Speed: 100 Mbps


**vEthernet (Default Switch)**:
- Description: Hyper-V Virtual Ethernet Adapter
- Link Speed: 10 Gbps


---

## 4. Benchmark Environment Configuration

### 4.1 Test Dataset Specifications

| Dataset | Size | Records | Purpose |
|---------|------|---------|---------|
| Wikipedia | 5 GB | 2M articles | Hybrid Vector + Filter Search |
| OpenStreetMap | 5 GB | 8M POIs | Geospatial + Graph Traversal |
| Amazon Reviews | 5 GB | 8M reviews | Multi-Model Text + Vector |
| Financial Ticks | 5 GB | 60M ticks | Time-Series OLAP |

**Total Dataset Size**: 20 GB

### 4.2 Docker Resource Allocation

| Database | CPU Cores | Memory | Storage |
|----------|-----------|--------|---------|
| ThemisDB | 6 | 8 GB | 5 GB |
| PostgreSQL | 4 | 6 GB | 5 GB |
| Elasticsearch | 4 | 6 GB | 5 GB |
| MongoDB | 4 | 6 GB | 5 GB |
| **Total** | **18** | **26 GB** | **20 GB** |

### 4.3 System Resource Utilization

- **CPU Utilization**: 18 cores allocated of 20 available (90%)
- **Memory Utilization**: 26 GB allocated of 64 GB total (40%)
- **Storage Utilization**: 80 GB required of 274.46 GB available (29%)

---

## 5. Benchmark Execution Details

### 5.1 Test Parameters

- **Number of Iterations**: 50 per scenario
- **Warmup Iterations**: 5 per scenario
- **Test Repetitions**: 3 runs per benchmark
- **Reporting Metrics**: Min, Max, Mean, Median, P95, P99

### 5.2 Performance Metrics Captured

For each query/operation:
- Response time (milliseconds)
- Throughput (operations/second)
- Memory consumption (MB)
- CPU utilization (%)
- Network I/O (bytes transferred)
- Cache hit ratio (where applicable)

---

## 6. Reproducibility Information

### 6.1 Software Versions (Locked for Reproducibility)

```json
{
  "timestamp": "2025-12-04T12:24:11.647437",
  "hardware_fingerprint": "i9-10900K-64GB-1906.42GB",
  "docker_version": "Docker version 29.0.1, build eedd969",
  "python_version": "3.13.6"
}
```

### 6.2 Environment Variables

```bash
# Benchmark Configuration
BENCHMARK_MODE=production
ITERATIONS_PER_SCENARIO=50
WARMUP_ITERATIONS=5
TEST_REPETITIONS=3

# Docker Configuration
DOCKER_MEMORY_LIMIT_THEMIS=8g
DOCKER_MEMORY_LIMIT_POSTGRES=6g
DOCKER_MEMORY_LIMIT_ELASTICSEARCH=6g
DOCKER_MEMORY_LIMIT_MONGODB=6g
```

---

## 7. Certification

**System Administrator**: [To be filled]  
**Benchmark Date**: 04.12.2025  
**Test Time**: 12:24:11  
**System Verified**: ✓ Yes  
**Hardware Certified**: ✓ Yes  
**Software Versions Locked**: ✓ Yes  

---

## 8. Notes & Observations

- System meets all requirements for comprehensive database benchmarking
- Network configuration supports high-speed inter-container communication
- Storage capacity allows for dataset scaling to 550GB if required
- RAM allocation provides 40% headroom for system processes and caching
- CPU allocation at 90% capacity ensures no thread starvation

---

**Report Generated**: 2025-12-04T12:24:27.361774  
**Generator Version**: 1.0  
**Format Version**: 1.0
