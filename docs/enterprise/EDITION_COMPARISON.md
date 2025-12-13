# ThemisDB Edition Comparison

**Version:** 1.0.0  
**Last Updated:** December 2025

---

## Quick Comparison

| Feature Category | Community | Enterprise | Hyperscaler |
|------------------|-----------|------------|-------------|
| **Price** | Free | Custom | Custom |
| **Deployment** | Single node | Multi-node | Unlimited + Kubernetes |
| **Support** | Community | 24/7 + TAM | 24/7 + Dedicated Team |
| **License** | MIT/Apache 2.0 | Commercial | Commercial |

---

## Feature Matrix

### ✅ = Included | ⚠️ = Limited | ❌ = Not Available

## Core Database Features

| Feature | Community | Enterprise | Hyperscaler |
|---------|-----------|------------|-------------|
| **ACID Transactions (MVCC)** | ✅ | ✅ | ✅ |
| **Multi-Model Storage** | ✅ | ✅ | ✅ |
| **RocksDB Engine** | ✅ | ✅ | ✅ |
| **Secondary Indexes** | ✅ | ✅ | ✅ |
| **Graph Traversals** | ✅ | ✅ | ✅ |
| **Vector Search (HNSW)** | ✅ | ✅ | ✅ |
| **Time-Series Support** | ✅ | ✅ | ✅ |
| **AQL Query Language** | ✅ | ✅ | ✅ |
| **REST API** | ✅ | ✅ | ✅ |
| **TLS 1.2+ Encryption** | ✅ | ✅ | ✅ |
| **Backup & Recovery** | ✅ | ✅ | ✅ |
| **CDC (Change Data Capture)** | ✅ | ✅ | ✅ |
| **Prometheus Metrics** | ✅ | ✅ | ✅ |

## Scalability & Distribution

| Feature | Community | Enterprise | Hyperscaler |
|---------|-----------|------------|-------------|
| **Horizontal Sharding** | ❌ | ✅ | ✅ |
| **Cross-Shard Joins** | ❌ | ✅ | ✅ |
| **Consistent Hashing** | ❌ | ✅ | ✅ |
| **Auto Rebalancing** | ❌ | ✅ | ✅ |
| **P2P Gossip Protocol** | ❌ | ✅ | ✅ |
| **etcd Integration** | ❌ | ✅ | ✅ |
| **Kubernetes Operator** | ❌ | ❌ | ✅ |
| **Auto-Scaling** | ❌ | ❌ | ✅ |
| **Max Nodes** | 1 | 100 | Unlimited |
| **Max Shards** | 0 | 1000 | Unlimited |

## Performance & Acceleration

| Feature | Community | Enterprise | Hyperscaler |
|---------|-----------|------------|-------------|
| **CPU Vector Search** | ✅ | ✅ | ✅ |
| **GPU Acceleration** | ❌ | ✅ | ✅ |
| **CUDA Support (NVIDIA)** | ❌ | ✅ | ✅ |
| **Vulkan Compute** | ❌ | ✅ | ✅ |
| **HIP (AMD ROCm)** | ❌ | ✅ | ✅ |
| **DirectX Compute** | ❌ | ✅ | ✅ |
| **Multi-GPU Support** | ❌ | ⚠️ Limited | ✅ |
| **Worker Threads** | Max 8 | Max 128 | Unlimited |
| **Performance Boost** | Baseline | 10-30x | 10-50x |

## Analytics & BI

| Feature | Community | Professional | Enterprise |
|---------|-----------|------------|-------------|
| **Basic Aggregations** | ✅ | ✅ | ✅ |
| **GROUP BY / COLLECT** | ✅ | ✅ | ✅ |
| **OLAP (CUBE/ROLLUP)** | ❌ | ✅ | ✅ |
| **Window Functions** | ❌ | ✅ | ✅ |
| **CEP Streaming** | ❌ | ✅ | ✅ |
| **Materialized Views** | ❌ | ✅ | ✅ |
| **Recursive CTEs** | ❌ | ✅ | ✅ |
| **Apache Arrow** | ❌ | ✅ | ✅ |
| **Columnar Storage** | ❌ | ✅ | ✅ |

## High Availability

| Feature | Community | Professional | Enterprise |
|---------|-----------|------------|-------------|
| **Local Backups** | ✅ | ✅ | ✅ |
| **WAL Archiving** | ✅ | ✅ | ✅ |
| **Leader-Follower Replication** | ❌ | ⚠️ 1 follower | ✅ Unlimited |
| **Multi-Master Replication** | ❌ | ❌ | ✅ |
| **CRDTs** | ❌ | ❌ | ✅ |
| **Automatic Failover** | ❌ | ⚠️ Manual | ✅ Automatic |
| **Geo-Replication** | ❌ | ❌ | ✅ |
| **RAID-like Redundancy** | ❌ | ❌ | ✅ |
| **Uptime SLA** | None | 99.5% | 99.99% |

## Security & Compliance

| Feature | Community | Professional | Enterprise |
|---------|-----------|------------|-------------|
| **TLS Encryption** | ✅ 1.2+ | ✅ 1.3 | ✅ 1.3 |
| **Password Auth** | ✅ | ✅ | ✅ |
| **Basic Audit Logs** | ✅ | ✅ | ✅ |
| **RBAC** | ❌ | ⚠️ Basic | ✅ Advanced |
| **Field-Level Encryption** | ❌ | ❌ | ✅ |
| **HSM Integration (PKCS#11)** | ❌ | ❌ | ✅ |
| **Certificate Pinning** | ❌ | ❌ | ✅ |
| **Secrets Management (Vault)** | ❌ | ❌ | ✅ |
| **Enhanced Audit Logging** | ❌ | ❌ | ✅ |
| **SIEM Integration** | ❌ | ❌ | ✅ |
| **Data Classification** | ❌ | ❌ | ✅ |
| **GDPR/HIPAA Tools** | ❌ | ⚠️ Basic | ✅ Complete |

## Management & Operations

| Feature | Community | Professional | Enterprise |
|---------|-----------|------------|-------------|
| **CLI Tools** | ✅ | ✅ | ✅ |
| **Web UI** | ❌ | ⚠️ Basic | ✅ Advanced |
| **Multi-Tenancy** | ❌ | ❌ | ✅ |
| **Rate Limiting** | ⚠️ Basic | ✅ Advanced | ✅ Advanced |
| **Load Shedding** | ❌ | ✅ | ✅ |
| **HTTP Connection Pool** | ❌ | ✅ | ✅ |
| **Grafana Dashboards** | ❌ | ⚠️ Basic | ✅ Pre-built |
| **Alert Rules** | ❌ | ⚠️ Basic | ✅ Advanced |
| **Admin Tools Suite** | ❌ | ❌ | ✅ 7 tools |
| **Audit Log Viewer** | ❌ | ❌ | ✅ |
| **SAGA Verifier** | ❌ | ❌ | ✅ |
| **PII Manager** | ❌ | ❌ | ✅ |
| **Retention Manager** | ❌ | ❌ | ✅ |

## Content Processing

| Feature | Community | Professional | Enterprise |
|---------|-----------|------------|-------------|
| **JSON/Text** | ✅ | ✅ | ✅ |
| **PDF Processing** | ❌ | ✅ | ✅ |
| **Office Formats (DOCX/XLSX)** | ❌ | ✅ | ✅ |
| **Video Processing** | ❌ | ✅ | ✅ |
| **Audio Processing** | ❌ | ✅ | ✅ |
| **Geo Processing (GDAL)** | ❌ | ✅ | ✅ |
| **CAD Processing** | ❌ | ❌ | ✅ |
| **Image Processing** | ❌ | ✅ | ✅ |
| **LLM Integration** | ❌ | ⚠️ Basic | ✅ Advanced |

## Support & Services

| Feature | Community | Professional | Enterprise |
|---------|-----------|------------|-------------|
| **Community Forum** | ✅ | ✅ | ✅ |
| **GitHub Issues** | ✅ | ✅ | ✅ |
| **Email Support** | ❌ | ✅ 48h | ✅ 4h |
| **Phone Support** | ❌ | ❌ | ✅ 24/7 |
| **Dedicated TAM** | ❌ | ❌ | ✅ |
| **Training** | ❌ | ⚠️ Online | ✅ On-site |
| **Custom Development** | ❌ | ❌ | ✅ |
| **SLA Guarantee** | ❌ | ❌ | ✅ |

---

## Pricing

### Community Edition
- **Cost:** FREE
- **License:** MIT / Apache 2.0
- **Use Case:** Development, small projects, evaluation
- **Support:** Community forums, GitHub issues
- **Updates:** Open source releases

### Enterprise Edition
- **Cost:** Custom pricing (volume discounts available)
- **Contact Sales:** enterprise@themisdb.io
- **Use Case:** Large-scale deployments, mission-critical systems
- **Limits:** Up to 100 nodes (default), custom limits available
- **Support:** 24/7 phone + email, dedicated TAM
- **Updates:** Priority access to new features
- **SLA:** 99.99% uptime guarantee
- **Features:** All 7 enterprise modules (Sharding, GPU, Analytics, Replication, Security, Management, Content)

### Hyperscaler Edition
- **Cost:** Custom pricing (enterprise agreements)
- **Contact Sales:** hyperscaler@themisdb.io
- **Use Case:** Hyperscale deployments, Kubernetes clusters, cloud-native architectures
- **Limits:** Unlimited nodes and shards
- **Support:** 24/7 phone + email, dedicated engineering team
- **Updates:** Early access to new features
- **SLA:** 99.999% uptime guarantee
- **Features:** All enterprise modules + Kubernetes Operator + Auto-Scaling + Multi-Region support

### Trial License
- **Duration:** 30 days
- **Includes:** All enterprise and hyperscaler features
- **No Credit Card:** Required
- **Sign Up:** https://themisdb.io/trial

---

## Upgrade Paths

### Community → Enterprise
1. Contact sales for enterprise license
2. Download enterprise DLLs
3. Install license file
4. Restart server with enterprise modules
5. Gradual rollout (canary deployments supported)

### Enterprise → Hyperscaler
1. Contact sales for hyperscaler upgrade
2. Update license file
3. Deploy Kubernetes operator
4. Configure auto-scaling
5. Enable multi-region replication

---

## Frequently Asked Questions

### Can I use Community Edition in production?
Yes, but you're limited to single-node deployments and won't have access to enterprise features like sharding, GPU acceleration, or advanced security.

### What happens if my license expires?
Enterprise modules will stop loading after a 14-day grace period. Core functionality (Community Edition features) continues to work.

### Can I mix editions (e.g., Community + select enterprise modules)?
Yes, with Enterprise/Hyperscaler licenses you can selectively enable only the modules you need.

### Do I need a license for development/testing?
No, Community Edition is free for all use cases. Enterprise trial licenses are available for 30 days.

### Is the source code for enterprise modules available?
Enterprise modules are distributed as compiled DLLs/SOs. Source code access is available under NDA for Enterprise and Hyperscaler customers.

### Can I downgrade from Enterprise to Community?
Yes, simply remove the license file and enterprise DLLs. Your data remains intact but advanced features become unavailable.

---

**Document Version:** 1.0  
**Last Updated:** December 13, 2025  
**Contact:** sales@themisdb.io
