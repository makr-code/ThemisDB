# ThemisDB Edition Comparison

**Version:** 1.0.0  
**Last Updated:** December 2025

---

## Quick Comparison

| Feature Category | Community | Professional | Enterprise |
|------------------|-----------|--------------|------------|
| **Price** | Free | $500/node/month | Custom |
| **Deployment** | Single node | Up to 10 nodes | Unlimited |
| **Support** | Community | Email (48h) | 24/7 + TAM |
| **License** | MIT/Apache 2.0 | Commercial | Commercial |

---

## Feature Matrix

### ✅ = Included | ⚠️ = Limited | ❌ = Not Available

## Core Database Features

| Feature | Community | Professional | Enterprise |
|---------|-----------|--------------|------------|
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

| Feature | Community | Professional | Enterprise |
|---------|-----------|--------------|------------|
| **Horizontal Sharding** | ❌ | ✅ | ✅ |
| **Cross-Shard Joins** | ❌ | ✅ | ✅ |
| **Consistent Hashing** | ❌ | ✅ | ✅ |
| **Auto Rebalancing** | ❌ | ✅ | ✅ |
| **P2P Gossip Protocol** | ❌ | ✅ | ✅ |
| **etcd Integration** | ❌ | ✅ | ✅ |
| **Max Nodes** | 1 | 10 | Unlimited |
| **Max Shards** | 0 | 100 | 1000+ |

## Performance & Acceleration

| Feature | Community | Professional | Enterprise |
|---------|-----------|--------------|------------|
| **CPU Vector Search** | ✅ | ✅ | ✅ |
| **GPU Acceleration** | ❌ | ✅ | ✅ |
| **CUDA Support (NVIDIA)** | ❌ | ✅ | ✅ |
| **Vulkan Compute** | ❌ | ✅ | ✅ |
| **HIP (AMD ROCm)** | ❌ | ✅ | ✅ |
| **DirectX Compute** | ❌ | ✅ | ✅ |
| **Worker Threads** | Max 8 | Max 32 | Unlimited |
| **Performance Boost** | Baseline | 10-20x | 10-50x |

## Analytics & BI

| Feature | Community | Professional | Enterprise |
|---------|-----------|--------------|------------|
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
|---------|-----------|--------------|------------|
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
|---------|-----------|--------------|------------|
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
|---------|-----------|--------------|------------|
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
|---------|-----------|--------------|------------|
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
|---------|-----------|--------------|------------|
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

### Professional Edition
- **Cost:** $500 per node per month
- **Billed:** Monthly or annual ($5,000/year per node)
- **Minimum:** 1 node
- **Use Case:** Production deployments up to 10 nodes
- **Support:** Email support (48h response time)
- **Updates:** Regular updates, security patches

### Enterprise Edition
- **Cost:** Custom pricing (volume discounts available)
- **Contact Sales:** enterprise@themisdb.io
- **Use Case:** Large-scale deployments, mission-critical systems
- **Support:** 24/7 phone + email, dedicated TAM
- **Updates:** Priority access to new features
- **SLA:** 99.99% uptime guarantee

### Trial License
- **Duration:** 30 days
- **Includes:** All enterprise features
- **No Credit Card:** Required
- **Sign Up:** https://themisdb.io/trial

---

## Upgrade Paths

### Community → Professional
1. Purchase Professional licenses for desired nodes
2. Download enterprise DLLs
3. Install license file
4. Restart server with enterprise modules
5. Gradual rollout (canary deployments supported)

### Professional → Enterprise
1. Contact sales for enterprise license
2. No code changes required
3. Unlock additional modules via license
4. Scale beyond 10 nodes
5. Activate 24/7 support

---

## Frequently Asked Questions

### Can I use Community Edition in production?
Yes, but you're limited to single-node deployments and won't have access to enterprise features like sharding, GPU acceleration, or advanced security.

### What happens if my license expires?
Enterprise modules will stop loading after a 14-day grace period. Core functionality (Community Edition features) continues to work.

### Can I mix editions (e.g., Community + select enterprise modules)?
Yes, with Professional/Enterprise licenses you can selectively enable only the modules you need.

### Do I need a license for development/testing?
No, Community Edition is free for all use cases. Enterprise trial licenses are available for 30 days.

### Is the source code for enterprise modules available?
Enterprise modules are distributed as compiled DLLs/SOs. Source code access is available under NDA for Enterprise customers.

### Can I downgrade from Enterprise to Community?
Yes, simply remove the license file and enterprise DLLs. Your data remains intact but advanced features become unavailable.

---

**Document Version:** 1.0  
**Last Updated:** December 13, 2025  
**Contact:** sales@themisdb.io
