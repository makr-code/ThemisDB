# ThemisDB Enterprise Edition

## Overview

ThemisDB offers both a **Community Edition** (free and open source) and an **Enterprise Edition** with advanced features for large-scale deployments.

## Community Edition vs Enterprise Edition

### ✅ Community Edition (Free & Open Source)

The Community Edition includes all core database functionality:

- **ACID Transactions** with MVCC (Multi-Version Concurrency Control)
- **Multi-Model Storage** (Relational, Document, Graph, Vector)
- **RocksDB Storage Engine** with LSM-Tree architecture
- **Secondary Indexes** (Range, Composite, Spatial)
- **Graph Traversals** (BFS, Dijkstra, shortest path)
- **Vector Search** (HNSW, FAISS integration)
- **GPU Acceleration** (CUDA, Vulkan, HIP, DirectX - single GPU)
- **Time-Series Support** with Gorilla compression
- **AQL Query Language** (FOR/FILTER/SORT/LIMIT/RETURN)
- **TLS 1.2+ Encryption** and password authentication
- **Backup & Recovery** via RocksDB checkpoints
- **Change Data Capture (CDC)**
- **REST API** and GraphQL
- **Prometheus Metrics** for basic observability

**Limits:**
- Single-node deployment only
- Maximum 8 worker threads
- Single GPU support only
- No distributed features

### 💎 Enterprise Edition (Commercial License)

The Enterprise Edition adds advanced features for production deployments:

#### 1. Horizontal Scalability & Distribution
- **VCC-URN/PKI Sharding** - Multi-node horizontal scaling
- **Consistent Hashing** - Automatic data distribution
- **Cross-Shard Joins** - Distributed query execution
- **Shard Rebalancing** - Dynamic capacity management
- **P2P Gossip Protocol** - Peer discovery
- **etcd Integration** - Enterprise-grade metadata store
- **mTLS** - Secure inter-node communication

#### 2. Advanced Analytics (OLAP/CEP)
- **OLAP Engine** (CUBE, ROLLUP, Window Functions)
- **CEP Streaming** (Complex Event Processing)
- **Materialized Views** - Pre-computed aggregations
- **Recursive CTEs** - Complex hierarchical queries
- **Apache Arrow Integration** - High-performance analytics
- **Columnar Storage** - OLAP-optimized data layout

#### 3. High Availability & Replication
- **Leader-Follower Replication** - Automatic failover
- **Multi-Master Replication** (CRDTs)
- **WAL Replication** - Real-time data sync
- **Geo-Replication** - Cross-datacenter deployments
- **RAID-like Redundancy** - Data durability
- **Automatic Failover** - Zero-downtime operations

#### 4. Advanced Security
- **HSM Integration** - Hardware security modules
- **Field-Level Encryption** - Granular data protection
- **Advanced RBAC** - Fine-grained access control
- **Compliance Reporting** (SOC 2, HIPAA, GDPR)
- **Secrets Management** (HashiCorp Vault integration)

#### 5. GPU Impact Analysis Plugin
- **Advanced geospatial analysis** for infrastructure planning
- **Multi-layer spatial analysis** with environmental risk assessment
- **Hybrid search** combining vector similarity and graph traversal
- **LLM integration** for natural language queries

#### 6. Performance & Management
- **Multi-GPU Support** - Scale across multiple GPUs
- **Advanced Load Balancing** - Intelligent request routing
- **Batch Operations** - High-throughput bulk processing
- **Advanced Monitoring** - Grafana dashboards, alerting
- **Kubernetes Operator** - Cloud-native deployments

## Obtaining Enterprise Edition

### Enterprise Source Code

The enterprise source code is **not included** in the public GitHub repository. This follows industry best practices used by GitLab, MongoDB, and other commercial open-source projects.

#### Why Separate Distribution?

1. **Protects Intellectual Property** - Enterprise features represent significant R&D investment
2. **License Compliance** - Ensures only licensed customers access enterprise code
3. **Professional Support** - Customers receive dedicated support and updates
4. **Quality Assurance** - Enterprise releases undergo additional testing and validation

#### Distribution Methods

**For Licensed Customers:**

**Option 1: Enterprise Source Package (Recommended)**
```bash
# Receive enterprise source as versioned package
themisdb-enterprise-v1.3.0.tar.gz

# Extract and integrate with community edition
tar -xzf themisdb-enterprise-v1.3.0.tar.gz
cp -r themisdb-enterprise-v1.3.0/src/enterprise ThemisDB/src/
cp -r themisdb-enterprise-v1.3.0/include/enterprise ThemisDB/include/
```

**Option 2: Private Repository Access**
- Access to private ThemisDB-Enterprise repository
- Pull updates via git
- Version controlled integration

**Option 3: Binary Distribution**
- Pre-compiled enterprise DLLs
- Fastest deployment
- No build required

#### How to Obtain Access

1. **Contact Sales**: Email sales@themisdb.com or visit https://themisdb.com/enterprise
2. **License Agreement**: Sign a commercial license agreement
3. **Access Grant**: Receive access credentials and enterprise package
4. **Integration Support**: Follow provided integration documentation
5. **Ongoing Support**: Access enterprise support portal and updates

### Pricing Tiers

- **Reseller Edition**: For embedding in commercial applications
- **Enterprise Edition**: For large-scale deployments (4-100 nodes)
- **Hyperscaler Edition**: For unlimited scale with Kubernetes

Contact sales@themisdb.com for pricing details.

## Building with Enterprise Features

If you have obtained enterprise source code, you can build with enterprise features enabled:

### Linux/macOS

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Add enterprise source (provided separately)
# Copy enterprise files to:
#   - src/enterprise/
#   - include/enterprise/
#   - plugins/enterprise/

# Build with enterprise features
cmake -B build -S . \
  -DTHEMIS_BUILD_ENTERPRISE=ON \
  -DTHEMIS_ENTERPRISE_SHARDING=ON \
  -DTHEMIS_ENTERPRISE_ANALYTICS=ON \
  -DTHEMIS_ENTERPRISE_REPLICATION=ON \
  -DTHEMIS_ENTERPRISE_SECURITY=ON

cmake --build build --target themis_enterprise_all
```

### Windows

```powershell
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Add enterprise source (provided separately)
# Copy enterprise files to appropriate directories

# Build with enterprise features
cmake -B build -S . -G "Visual Studio 17 2022" `
  -DTHEMIS_BUILD_ENTERPRISE=ON `
  -DTHEMIS_ENTERPRISE_SHARDING=ON `
  -DTHEMIS_ENTERPRISE_ANALYTICS=ON

cmake --build build --config Release --target themis_enterprise_all
```

## Enterprise Documentation

High-level enterprise feature documentation is available in the public repository:

- [Enterprise Feature Analysis](docs/enterprise/ENTERPRISE_FEATURE_ANALYSIS.md)
- [Edition Comparison](docs/enterprise/EDITION_COMPARISON.md)
- [Architecture Overview](docs/architecture/ARCHITECTURE_OVERVIEW.md)

Detailed implementation guides and examples are provided with the enterprise source code.

## License Validation

Enterprise features require a valid license key. The license is validated at runtime:

```yaml
# config/enterprise_license.yaml
license:
  key: "YOUR_LICENSE_KEY"
  customer: "Your Organization"
  tier: "enterprise"  # reseller, enterprise, or hyperscaler
  expiry: "2026-12-31"
```

Without a valid license, enterprise features will not load.

## Support

### Community Edition Support
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Community Discussions: https://github.com/makr-code/ThemisDB/discussions
- Documentation: https://makr-code.github.io/ThemisDB/

### Enterprise Edition Support
- 24/7 Support Portal (license holders only)
- Dedicated Technical Account Manager (Hyperscaler tier)
- Priority bug fixes and feature requests
- Custom integrations and consulting

Contact: support@themisdb.com

## FAQ

**Q: Can I use Community Edition in production?**  
A: Yes! The Community Edition is fully functional and production-ready for single-node deployments.

**Q: What happens if I try to enable enterprise features without a license?**  
A: The build will warn that enterprise source is not available, and enterprise features will be disabled. The core database functionality remains unaffected.

**Q: Can I migrate from Community to Enterprise Edition?**  
A: Yes, migration is seamless. Your existing databases and configurations are fully compatible.

**Q: Is the Community Edition limited or crippled?**  
A: No! The Community Edition includes all core database features with no artificial limitations. Enterprise Edition adds features specifically designed for large-scale, multi-node deployments.

**Q: Do I need enterprise features for GPU acceleration?**  
A: No, GPU acceleration is available in Community Edition. Enterprise Edition adds multi-GPU support and advanced GPU memory management.

## License

- **Community Edition**: MIT License (open source)
- **Enterprise Edition**: Commercial License (proprietary)

See [LICENSE](LICENSE) for Community Edition terms.

---

## Best Practices & Architecture

### Why Separate Repositories?

ThemisDB follows the industry-standard "GitLab model" for commercial open-source projects:

**✅ Benefits:**
- **Community Edition** is fully functional and self-contained
- **Enterprise code** is protected and only available to licensed customers
- **Clear separation** makes licensing and support straightforward
- **No destructive updates** - existing installations keep working

**📚 Used by:**
- GitLab (Community vs Enterprise Edition)
- MongoDB (Community vs Enterprise Server)
- Grafana (OSS vs Enterprise)
- Redis (OSS vs Enterprise)

### Distribution Model

**Public Repository (GitHub):**
- Full Community Edition source code
- High-level enterprise feature documentation
- Plugin interfaces and APIs
- Build system with optional enterprise support

**Private Distribution (Licensed Customers Only):**
- Enterprise implementation source code
- Detailed integration documentation
- Enterprise examples and tests
- Priority support and updates

For detailed technical information, see:
- [Distribution Best Practices](docs/enterprise/DISTRIBUTION_BEST_PRACTICES.md)
- [Implementation Summary](implementation-history/summaries/IMPLEMENTATION_SUMMARY.md)

---

**Built with ❤️ by the ThemisDB team**
