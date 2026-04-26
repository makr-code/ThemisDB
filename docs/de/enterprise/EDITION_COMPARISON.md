# ThemisDB Edition Comparison

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🏢 Enterprise

---

## 📑 Table of Contents

- [Quick Comparison](#quick-comparison)
- [Features](#features)
- [Pricing](#pricing)

---

## Quick Comparison

| Feature Category | Community | Reseller | Enterprise | Hyperscaler |
|------------------|-----------|----------|------------|-------------|
| **Price** | Free | Per-app license | Per-app license | Custom | Custom |
| **Deployment** | Single node | Embedded in apps | Embedded in apps | Multi-node | Unlimited + Kubernetes |
| **Support** | Community | Email + Docs | Email + Docs | 24/7 + TAM | 24/7 + Dedicated Team |
| **License** | MIT/Apache 2.0 | Commercial (Reseller) | Commercial (Reseller) | Commercial | Commercial |
| **Use Case** | Development | Embedding programs | Embedding programs | Large deployments | Hyperscale |

---

## Feature Matrix

### ✅ = Included | ⚠️ = Limited | ❌ = Not Available

## Core Database Features

| Feature | Community | Reseller | Enterprise | Hyperscaler |
|---------|----------|------------|-------------|
| **ACID Transactions (MVCC)** | ✅ | ✅ | ✅ | ✅ |
| **Multi-Model Storage** | ✅ | ✅ | ✅ | ✅ |
| **RocksDB Engine** | ✅ | ✅ | ✅ | ✅ |
| **Secondary Indexes** | ✅ | ✅ | ✅ | ✅ |
| **Graph Traversals** | ✅ | ✅ | ✅ | ✅ |
| **Vector Search (HNSW)** | ✅ | ✅ | ✅ | ✅ |
| **Time-Series Support** | ✅ | ✅ | ✅ | ✅ |
| **AQL Query Language** | ✅ | ✅ | ✅ | ✅ |
| **REST API** | ✅ | ✅ | ✅ | ✅ |
| **TLS 1.2+ Encryption** | ✅ | ✅ | ✅ | ✅ |
| **Backup & Recovery** | ✅ | ✅ | ✅ | ✅ |
| **CDC (Change Data Capture)** | ✅ | ✅ | ✅ | ✅ |
| **Prometheus Metrics** | ✅ | ✅ | ✅ | ✅ |

## Scalability & Distribution

| Feature | Community | Reseller | Enterprise | Hyperscaler |
|---------|----------|------------|-------------|
| **Horizontal Sharding** | ❌ | ⚠️ Basic (2-3 nodes) | ✅ Advanced | ✅ Advanced |
| **Cross-Shard Joins** | ❌ | ⚠️ Limited | ✅ | ✅ |
| **Consistent Hashing** | ❌ | ⚠️ Basic | ✅ | ✅ |
| **Auto Rebalancing** | ❌ | ❌ | ✅ | ✅ |
| **P2P Gossip Protocol** | ❌ | ❌ | ✅ | ✅ |
| **etcd Integration** | ❌ | ❌ | ✅ | ✅ |
| **Kubernetes Operator** | ❌ | ❌ | ❌ | ✅ |
| **Auto-Scaling** | ❌ | ❌ | ❌ | ✅ |
| **Max Nodes** | 1 | 1-3 | 4-100 | Unlimited |
| **Max Shards per Node** | 0 | 3-5 | 10-20 | Unlimited |
| **RAID Modes** | N/A | MIRROR only | All modes | All modes |

## Performance & Acceleration

| Feature | Community | Reseller | Enterprise | Hyperscaler |
|---------|----------|------------|-------------|
| **CPU Vector Search** | ✅ | ✅ | ✅ | ✅ |
| **GPU Acceleration** | ✅ | ✅ | ✅ | ✅ |
| **CUDA Support (NVIDIA)** | ✅ | ✅ | ✅ | ✅ |
| **Vulkan Compute** | ✅ | ✅ | ✅ | ✅ |
| **HIP (AMD ROCm)** | ✅ | ✅ | ✅ | ✅ |
| **DirectX Compute** | ✅ | ✅ | ✅ | ✅ |
| **Multi-GPU Support** | ⚠️ Single GPU | ⚠️ Single GPU | ⚠️ Multi-GPU | ✅ Advanced Multi-GPU |
| **GPU Memory Management** | ⚠️ Basic | ✅ Advanced | ✅ Advanced | ✅ Advanced |
| **Worker Threads** | Max 8 | Max 16 | Max 128 | Unlimited |
| **Performance Boost** | Baseline (with GPU) | Baseline (with GPU) | 10-30x | 10-50x |

## Analytics & BI

| Feature | Community | Professional | Enterprise |
|---------|----------|------------|-------------|
| **Basic Aggregations** | ✅ | ✅ | ✅ | ✅ |
| **GROUP BY / COLLECT** | ✅ | ✅ | ✅ | ✅ |
| **OLAP (CUBE/ROLLUP)** | ❌ | ❌ | ✅ | ✅ |
| **Window Functions** | ❌ | ❌ | ✅ | ✅ |
| **CEP Streaming** | ❌ | ❌ | ✅ | ✅ |
| **Materialized Views** | ❌ | ❌ | ✅ | ✅ |
| **Recursive CTEs** | ❌ | ❌ | ✅ | ✅ |
| **Apache Arrow** | ❌ | ❌ | ✅ | ✅ |
| **Columnar Storage** | ❌ | ❌ | ✅ | ✅ |

## High Availability

| Feature | Community | Professional | Enterprise |
|---------|----------|------------|-------------|
| **Local Backups** | ✅ | ✅ | ✅ | ✅ |
| **WAL Archiving** | ✅ | ✅ | ✅ | ✅ |
| **Leader-Follower Replication** | ❌ | ❌ | ⚠️ 1 follower | ✅ Unlimited |
| **Multi-Master Replication** | ❌ | ❌ | ❌ | ✅ |
| **CRDTs** | ❌ | ❌ | ❌ | ✅ |
| **Automatic Failover** | ❌ | ❌ | ⚠️ Manual | ✅ Automatic |
| **Geo-Replication** | ❌ | ❌ | ❌ | ✅ |
| **RAID-like Redundancy** | ❌ | ❌ | ❌ | ✅ |
| **Uptime SLA** | None | 99.5% | 99.5% | 99.99% |

## Security & Compliance

| Feature | Community | Professional | Enterprise |
|---------|----------|------------|-------------|
| **TLS Encryption** | ✅ 1.2+ | ✅ 1.2+ | ✅ 1.3 | ✅ 1.3 |
| **Password Auth** | ✅ | ✅ | ✅ | ✅ |
| **Basic Audit Logs** | ✅ | ✅ | ✅ | ✅ |
| **RBAC** | ❌ | ❌ | ⚠️ Basic | ✅ Advanced |
| **Field-Level Encryption** | ❌ | ❌ | ❌ | ✅ |
| **HSM Integration (PKCS#11)** | ❌ | ❌ | ❌ | ✅ |
| **Certificate Pinning** | ❌ | ❌ | ❌ | ✅ |
| **Secrets Management (Vault)** | ❌ | ❌ | ❌ | ✅ |
| **Enhanced Audit Logging** | ❌ | ❌ | ❌ | ✅ |
| **SIEM Integration** | ❌ | ❌ | ❌ | ✅ |
| **Data Classification** | ❌ | ❌ | ❌ | ✅ |
| **GDPR/HIPAA Tools** | ❌ | ❌ | ⚠️ Basic | ✅ Complete |

## Management & Operations

| Feature | Community | Professional | Enterprise |
|---------|----------|------------|-------------|
| **CLI Tools** | ✅ | ✅ | ✅ | ✅ |
| **Web UI** | ❌ | ❌ | ⚠️ Basic | ✅ Advanced |
| **Multi-Tenancy** | ❌ | ❌ | ❌ | ✅ |
| **Rate Limiting** | ⚠️ Basic | ✅ Advanced | ✅ Advanced | ✅ Advanced |
| **Load Shedding** | ❌ | ❌ | ✅ | ✅ |
| **HTTP Connection Pool** | ❌ | ❌ | ✅ | ✅ |
| **Grafana Dashboards** | ❌ | ❌ | ⚠️ Basic | ✅ Pre-built |
| **Alert Rules** | ❌ | ❌ | ⚠️ Basic | ✅ Advanced |
| **Admin Tools Suite** | ❌ | ❌ | ❌ | ✅ 7 tools |
| **Audit Log Viewer** | ❌ | ❌ | ❌ | ✅ |
| **SAGA Verifier** | ❌ | ❌ | ❌ | ✅ |
| **PII Manager** | ❌ | ❌ | ❌ | ✅ |
| **Retention Manager** | ❌ | ❌ | ❌ | ✅ |

## Content Processing

| Feature | Community | Professional | Enterprise |
|---------|----------|------------|-------------|
| **JSON/Text** | ✅ | ✅ | ✅ | ✅ |
| **PDF Processing** | ❌ | ❌ | ✅ | ✅ |
| **Office Formats (DOCX/XLSX)** | ❌ | ❌ | ✅ | ✅ |
| **Video Processing** | ❌ | ❌ | ✅ | ✅ |
| **Audio Processing** | ❌ | ❌ | ✅ | ✅ |
| **Geo Processing (GDAL)** | ❌ | ❌ | ✅ | ✅ |
| **CAD Processing** | ❌ | ❌ | ❌ | ✅ |
| **Image Processing** | ❌ | ❌ | ✅ | ✅ |
| **LLM Integration** | ❌ | ❌ | ⚠️ Basic | ✅ Advanced |

## Support & Services

| Feature | Community | Professional | Enterprise |
|---------|----------|------------|-------------|
| **Community Forum** | ✅ | ✅ | ✅ | ✅ |
| **GitHub Issues** | ✅ | ✅ | ✅ | ✅ |
| **Email Support** | ❌ | ❌ | ✅ 48h | ✅ 4h |
| **Phone Support** | ❌ | ❌ | ❌ | ✅ 24/7 |
| **Dedicated TAM** | ❌ | ❌ | ❌ | ✅ |
| **Training** | ❌ | ❌ | ⚠️ Online | ✅ On-site |
| **Custom Development** | ❌ | ❌ | ❌ | ✅ |
| **SLA Guarantee** | ❌ | ❌ | ❌ | ✅ |

---

## Pricing

### Community Edition
- **Cost:** FREE
- **License:** MIT / Apache 2.0
- **Use Case:** Development, small projects, evaluation
- **Support:** Community forums, GitHub issues
- **Updates:** Open source releases

### Reseller Edition
- **Cost:** Per-application license (volume discounts available)
- **Contact Sales:** service@themisdb.org
- **Use Case:** Embedding ThemisDB in commercial applications/products
- **Limits:** 1-3 nodes per application instance, single GPU, basic sharding (if 2-3 nodes)
- **Support:** Email support (business hours), documentation
- **Updates:** Regular updates, security patches
- **Features:** Core database + GPU acceleration + vector search + basic sharding (MIRROR/RAID-1 only)
- **Redistribution:** Allowed with application, no standalone distribution
- **Branding:** White-label options available
- **Sharding:** Available with 2-3 nodes (RAID-1 MIRROR mode only, 3-5 shards per node)

### Enterprise Edition
- **Cost:** Custom pricing (volume discounts available)
- **Contact Sales:** service@themisdb.org
- **Use Case:** Large-scale deployments, mission-critical systems
- **Limits:** 4-100 nodes (default), custom limits available
- **Support:** 24/7 phone + email, dedicated TAM
- **Updates:** Priority access to new features
- **SLA:** 99.99% uptime guarantee
- **Features:** All 6 enterprise modules (Sharding, Analytics, Replication, Security, Management, Content)
- **Sharding:** Advanced - all RAID modes (MIRROR, STRIPE, STRIPE_MIRROR, PARITY, GEO_MIRROR), 10-20 shards per node

### Hyperscaler Edition
- **Cost:** Custom pricing (enterprise agreements)
- **Contact Sales:** service@themisdb.org
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
**Last Updated:** April 2026  
**Contact:** service@themisdb.org
