# ThemisDB Edition Comparison

**Version:** 1.5.0-dev  
**Last Updated:** 2026-04-06

---

## Quick Comparison

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **Price** | Free | Free | Commercial | Custom |
| **License** | MIT | MIT | Commercial | Commercial |
| **Binary Size** | ~30-50 MB | ~80-150 MB | ~150-250 MB | ~200-300 MB |
| **Build Time** | ~5-10 min | ~20-30 min | ~30-40 min | ~40-60 min |
| **RAM (Idle)** | ~100-200 MB | ~300-500 MB | ~500-800 MB | ~800-1200 MB |
| **Use Case** | Embedded/IoT | Development/SMB | Enterprise | Hyperscale |

---

## Core Database Features

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **ACID Transactions** | ✅ | ✅ | ✅ | ✅ |
| **MVCC** | ✅ | ✅ | ✅ | ✅ |
| **Multi-Model Storage** | ✅ | ✅ | ✅ | ✅ |
| **RocksDB Engine** | ✅ | ✅ | ✅ | ✅ |
| **Secondary Indexes** | ✅ | ✅ | ✅ | ✅ |
| **Graph Traversals** | ✅ Basic | ✅ Full | ✅ Full | ✅ Advanced |
| **Vector Search (CPU)** | ✅ Basic | ✅ Full | ✅ Full | ✅ Advanced |
| **Time-Series** | ✅ Basic | ✅ Full | ✅ Advanced | ✅ Advanced |
| **AQL Query Language** | ✅ Basic | ✅ Full | ✅ Full | ✅ Full |

---

## AI/ML Features

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **LLM Integration (llama.cpp)** | ❌ | ✅ Optional | ✅ Optional | ✅ Full |
| **GPU Acceleration** | ❌ | ✅ Optional | ✅ Full | ✅ Multi-GPU |
| **CUDA Support** | ❌ | ✅ | ✅ | ✅ |
| **Vulkan/HIP/OpenCL** | ❌ | ✅ | ✅ | ✅ |
| **Image Analysis Plugin** | ❌ | ✅ | ✅ | ✅ |
| **Voice Assistant** | ❌ | ❌ | ✅ | ✅ |
| **Content Processors** | ❌ | ✅ | ✅ | ✅ |
| **Hybrid Search (RAG)** | ❌ | ✅ | ✅ | ✅ |

---

## Network Protocols

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **HTTP/1.1 (REST)** | ✅ | ✅ | ✅ | ✅ |
| **GraphQL** | ✅ Basic | ✅ | ✅ | ✅ |
| **SSE (Server-Sent Events)** | ❌ | ✅ | ✅ | ✅ |
| **HTTP/2 + Server Push** | ❌ | ✅ | ✅ | ✅ |
| **WebSocket** | ❌ | ✅ | ✅ | ✅ |
| **gRPC** | ❌ | ✅ | ✅ | ✅ |
| **MQTT** | ❌ | ✅ | ✅ | ✅ |
| **PostgreSQL Wire** | ❌ | 🚧 Alpha | 🚧 Alpha | ✅ |
| **HTTP/3 (QUIC)** | ❌ | 🚧 Planned v1.6+ | 🚧 Planned v1.6+ | ✅ Planned v1.6+ |
| **MCP (Model Context)** | ❌ | ✅ | ✅ | ✅ |

---

## Distribution & Scaling

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **Max Nodes** | 1 | 1 | 100 | Unlimited |
| **Horizontal Sharding** | ❌ | ❌ | ✅ | ✅ |
| **Replication** | ❌ | ❌ | ✅ | ✅ |
| **Multi-Master** | ❌ | ❌ | ✅ | ✅ |
| **RAID Modes** | ❌ | ❌ | ✅ All | ✅ All |
| **Auto-Rebalancing** | ❌ | ❌ | ✅ | ✅ |
| **Kubernetes Operator** | ❌ | ❌ | ❌ | ✅ |
| **Cross-Region** | ❌ | ❌ | ❌ | ✅ |

---

## Security & Compliance

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **TLS 1.2/1.3** | ✅ | ✅ | ✅ | ✅ |
| **Basic Authentication** | ✅ | ✅ | ✅ | ✅ |
| **JWT Tokens** | ✅ | ✅ | ✅ | ✅ |
| **RBAC** | ❌ | ❌ | ✅ | ✅ |
| **Field-Level Encryption** | ❌ | ❌ | ✅ | ✅ |
| **HSM Support** | ❌ | ❌ | ✅ | ✅ |
| **Audit Logging** | ✅ Basic | ✅ | ✅ Advanced | ✅ Advanced |
| **Secrets Management** | ❌ | ❌ | ✅ | ✅ |
| **Certificate Pinning** | ❌ | ❌ | ✅ | ✅ |

---

## Analytics & Processing

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **Basic Aggregations** | ✅ | ✅ | ✅ | ✅ |
| **GROUP BY / COLLECT** | ✅ | ✅ | ✅ | ✅ |
| **OLAP (CUBE/ROLLUP)** | ❌ | ❌ | ✅ | ✅ |
| **Window Functions** | ❌ | ❌ | ✅ | ✅ |
| **CEP (Complex Events)** | ❌ | ❌ | ✅ | ✅ |
| **Materialized Views** | ❌ | ❌ | ✅ | ✅ |
| **CDC (Change Data Capture)** | ❌ | ✅ Basic | ✅ Advanced | ✅ Advanced |
| **Stream Processing** | ❌ | ❌ | ✅ | ✅ |

---

## Observability

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **Prometheus Metrics** | ✅ Basic | ✅ | ✅ | ✅ |
| **OpenTelemetry Tracing** | ❌ | ✅ | ✅ | ✅ |
| **Grafana Dashboards** | ❌ | ✅ | ✅ | ✅ |
| **Health Checks** | ✅ | ✅ | ✅ | ✅ |
| **Performance Counters** | ✅ Basic | ✅ | ✅ | ✅ Advanced |
| **SIEM Integration** | ❌ | ❌ | ✅ | ✅ |

---

## Support & Licensing

| Aspect | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|--------|---------|-----------|------------|-------------|
| **Support** | Community | Community | 24/7 + TAM | Dedicated Team |
| **SLA** | None | None | 99.9% | 99.99%+ |
| **Updates** | Self-service | Self-service | Managed | Managed |
| **Training** | Documentation | Documentation | Included | Customized |
| **Professional Services** | ❌ | ❌ | ✅ | ✅ |
| **Custom Development** | ❌ | ❌ | Optional | Included |

---

## Use Case Recommendations

### Choose MINIMAL if:
- 🔹 You need **embedded database** for IoT/edge devices
- 🔹 You want **fast builds** (<10 minutes) for CI/CD
- 🔹 You have **limited resources** (RAM, storage, CPU)
- 🔹 You only need **core CRUD** operations
- 🔹 You're **learning** ThemisDB internals
- 🔹 You need **smallest binary** for distribution

### Choose COMMUNITY if:
- 🆓 You need **full database features** without cost
- 🆓 You want **optional LLM/GPU** acceleration
- 🆓 You're building **single-server** applications
- 🆓 You need **modern protocols** (WebSocket, gRPC)
- 🆓 You're a **startup** or **SMB**
- 🆓 You need **vector search** with GPU support

### Choose ENTERPRISE if:
- 🔒 You need **horizontal scaling** (2-100 nodes)
- 🔒 You require **high availability** with replication
- 🔒 You need **advanced security** (RBAC, HSM)
- 🔒 You want **24/7 support** with SLA
- 🔒 You're a **large enterprise** or government
- 🔒 You need **compliance** features (audit, encryption)

### Choose HYPERSCALER if:
- 🌐 You're operating at **massive scale** (100+ nodes)
- 🌐 You need **multi-datacenter** deployment
- 🌐 You require **custom features** and dedicated support
- 🌐 You're a **cloud provider** or large platform
- 🌐 You need **unlimited scaling** and customization
- 🌐 You want **strategic partnership** with vendor

---

## Migration Path

### From MINIMAL → COMMUNITY
```bash
# Rebuild with COMMUNITY edition
cmake -DTHEMIS_EDITION=COMMUNITY ...
```
**Zero downtime:** Data format is compatible, just rebuild and restart.

### From COMMUNITY → ENTERPRISE
```bash
# Contact sales@themisdb.com for license
# Then rebuild with ENTERPRISE edition
cmake -DTHEMIS_EDITION=ENTERPRISE ...
```
**Downtime required:** May need data migration for sharding setup.

### From ENTERPRISE → HYPERSCALER
```bash
# Custom deployment with vendor support
# Includes architecture review and migration plan
```
**Professional Services:** Vendor-managed migration with zero data loss guarantee.

---

## Frequently Asked Questions

### Can I upgrade from MINIMAL to COMMUNITY later?
**Yes!** The data format is identical. Just rebuild with `THEMIS_EDITION=COMMUNITY` and restart.

### What happens if I exceed MINIMAL limitations?
The server will continue to work but won't provide features disabled in MINIMAL (LLM, GPU, etc.).

### Can I use MINIMAL in production?
**Yes!** MINIMAL is production-ready for single-node, resource-constrained deployments.

### Is MINIMAL truly open source?
**Yes!** MINIMAL is fully open source under MIT license, same as COMMUNITY.

### Can I build MINIMAL with custom features?
**Yes!** You can selectively enable features (e.g., `THEMIS_ENABLE_TRACING=ON`) even in MINIMAL.

### How much smaller is MINIMAL compared to COMMUNITY?
**~50-80% smaller** in binary size and memory footprint, ~50% faster build times.

---

## Performance Comparison

### Single-Node Performance (Standard Hardware)

| Metric | MINIMAL | COMMUNITY | ENTERPRISE |
|--------|---------|-----------|------------|
| **Entity Writes** | ~20k ops/s | ~45k ops/s | ~45k ops/s |
| **Entity Reads** | ~60k ops/s | ~120k ops/s | ~120k ops/s |
| **Vector Search (CPU)** | ~100k q/s | ~3M q/s | ~3M q/s |
| **Vector Search (GPU)** | N/A | ~60M q/s | ~60M q/s |
| **Graph Traverse** | ~2M ops/s | ~9M ops/s | ~9M ops/s |
| **Query Latency** | ~2-10 μs | ~0.3-2 μs | ~0.3-2 μs |

*Hardware: 4-core CPU @ 3.7 GHz, 16 GB RAM, NVMe SSD*

### Multi-Node Performance (ENTERPRISE/HYPERSCALER)

| Metric | ENTERPRISE (10 nodes) | HYPERSCALER (100 nodes) |
|--------|----------------------|-------------------------|
| **Total Writes** | ~450k ops/s | ~4.5M ops/s |
| **Total Reads** | ~1.2M ops/s | ~12M ops/s |
| **Cross-Shard Latency** | ~5-15 ms | ~10-30 ms |
| **Replication Lag** | <100 ms | <500 ms |

---

## License Compliance

### MINIMAL & COMMUNITY (MIT License)
```
- ✅ Commercial use allowed
- ✅ Modification allowed
- ✅ Distribution allowed
- ✅ Private use allowed
- ⚠️ No warranty
- ⚠️ No liability
```

### ENTERPRISE & HYPERSCALER (Commercial License)
```
- ✅ All MIT rights
- ✅ + Commercial support
- ✅ + SLA guarantees
- ✅ + Professional services
- ✅ + Warranty included
- ✅ + Liability coverage
```

---

## Summary

| When to use... | Edition |
|----------------|---------|
| **Learning ThemisDB** | MINIMAL or COMMUNITY |
| **Embedded/IoT devices** | MINIMAL |
| **Single-server apps** | COMMUNITY |
| **SMB/Startup (free)** | COMMUNITY |
| **Enterprise (2-100 nodes)** | ENTERPRISE |
| **Cloud provider/platform** | HYPERSCALER |
| **Need 24/7 support** | ENTERPRISE or HYPERSCALER |
| **Custom development** | HYPERSCALER |

---

**For detailed pricing and licensing information, contact sales@themisdb.com**

**[← Back to Documentation](../README.md)**
