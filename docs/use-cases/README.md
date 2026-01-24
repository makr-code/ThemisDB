# ThemisDB Use Case Guides

This directory contains comprehensive, production-ready guides for building real-world applications with ThemisDB. Each guide includes architecture diagrams, complete schema examples, query patterns, performance optimization strategies, and best practices.

## Available Use Cases

### 1. [E-Commerce Platform](ECOMMERCE_USE_CASE.md) 🛒

Build a complete e-commerce platform leveraging ThemisDB's multi-model capabilities.

**Key Features Covered:**
- Product catalog with full-text and semantic search
- Multi-warehouse inventory management
- Order processing with ACID transactions
- Recommendation engine (collaborative + content-based)
- Customer analytics with graph queries
- Real-time product search with vector embeddings

**Technologies:**
- Document store for products and orders
- Vector search (HNSW) for semantic product discovery
- Graph queries for customer relationships
- Time-series for analytics
- Full-text search with BM25 ranking

**Perfect For:**
- Online retail platforms
- Marketplace applications
- Product catalog systems
- Recommendation engines

---

### 2. [IoT & Sensor Networks](IOT_USE_CASE.md) 📡

Build scalable IoT platforms for sensor data management and real-time analytics.

**Key Features Covered:**
- High-throughput time-series data ingestion
- Real-time sensor data aggregation
- Anomaly detection with CEP (Complex Event Processing)
- Device management and network topology
- Historical analysis and forecasting
- Edge-to-cloud architecture patterns

**Technologies:**
- Time-series collections with automatic downsampling
- Graph topology for device relationships
- Complex event processing for alerts
- Streaming data ingestion
- Multi-tier storage (hot/warm/cold)

**Perfect For:**
- Industrial IoT platforms
- Smart building systems
- Environmental monitoring
- Fleet management
- Energy management systems

---

### 3. [RAG & LLM Applications](RAG_LLM_USE_CASE.md) 🤖

Build production RAG (Retrieval-Augmented Generation) systems with native LLM integration.

**Key Features Covered:**
- Vector embeddings storage and indexing
- Semantic search with HNSW
- Document chunking strategies
- RAG pipeline implementation
- Native llama.cpp integration
- Context retrieval optimization
- Hybrid search (vector + keyword)

**Technologies:**
- Vector search with HNSW indexes
- Native LLM engine (llama.cpp)
- LoRA adapter support
- Document processing pipelines
- Query caching
- Multi-query RAG

**Perfect For:**
- AI chatbots and assistants
- Document Q&A systems
- Knowledge base search
- Code assistants
- Enterprise search applications

---

### 4. [SaaS Multi-Tenancy](SAAS_USE_CASE.md) 🏢

Build secure, scalable SaaS applications with proper tenant isolation.

**Key Features Covered:**
- Multi-tenant data isolation strategies
- Row-level security (RLS)
- Tenant-aware queries
- Resource quotas and limits
- Billing and usage tracking
- Tenant provisioning/deprovisioning
- GDPR compliance

**Technologies:**
- Row-level security policies
- Tenant-aware sharding
- Usage tracking with time-series
- Audit logging
- Quota enforcement
- Connection pooling

**Perfect For:**
- B2B SaaS platforms
- Multi-tenant applications
- Enterprise software
- Billing systems
- Customer portals

---

## How to Use These Guides

Each guide follows a consistent structure:

1. **Architecture Overview** - High-level system design with ASCII diagrams
2. **Schema Design** - Complete data models with examples
3. **Query Examples** - Production-ready AQL queries
4. **Code Examples** - C++ and other language examples
5. **Performance Optimization** - Indexing, caching, and scaling strategies
6. **Monitoring** - Metrics and observability patterns
7. **Best Practices** - Lessons learned and recommendations
8. **Related Resources** - Links to documentation and example projects

## Getting Started

1. **Choose Your Use Case** - Select the guide that matches your application
2. **Review Architecture** - Understand the overall system design
3. **Study Schema Design** - Adapt the data models to your needs
4. **Try Query Examples** - Test queries in your ThemisDB instance
5. **Implement Features** - Build incrementally using the patterns shown
6. **Optimize Performance** - Apply the optimization techniques
7. **Deploy to Production** - Use the deployment guidelines

## Example Projects

Each use case guide references working example projects in the [`examples/`](../../examples/) directory:

- **E-Commerce**: `14_ecommerce_catalog/`, `19_recommendation_engine/`
- **IoT**: `09_iot_sensor_network/`, `20_smart_home/`
- **RAG/LLM**: `07_vector_search_documents/`, `18_realtime_chat/`, `llm/`
- **SaaS**: `17_crm/`, `16_kanban_board/`

## Additional Resources

### Documentation
- [AQL Query Language](../aql/README.md)
- [Architecture Guide](../architecture/README.md)
- [Performance Tuning](../performance/README.md)
- [Security Best Practices](../security/README.md)

### Tutorials
- [Quick Start Guide](../../QUICKSTART.md)
- [Examples Index](../EXAMPLES_INDEX.md)
- [API Documentation](../api/README.md)

### Community
- [Contributing Guidelines](../../CONTRIBUTING.md)
- [GitHub Discussions](https://github.com/yourusername/ThemisDB/discussions)
- [Discord Community](https://discord.gg/themisdb)

## Use Case Comparison

| Feature | E-Commerce | IoT | RAG/LLM | SaaS |
|---------|-----------|-----|---------|------|
| **Data Models** | Doc, Graph, Vector | Time-Series, Graph | Doc, Vector | Doc, RLS |
| **Scale** | 10K-1M products | Millions of sensors | 100K-10M documents | 100-10K tenants |
| **Query Patterns** | Search, Recommendations | Aggregations, CEP | Semantic Search | Tenant-filtered |
| **Write Throughput** | Medium | Very High | Low-Medium | Medium |
| **Read Throughput** | High | Medium | High | High |
| **Consistency** | Strong (orders) | Eventual (metrics) | Eventual | Strong |
| **Special Features** | Transactions | Downsampling | LLM Integration | RLS, Quotas |

## Performance Characteristics

### E-Commerce
- **Queries/sec**: 1,000-10,000
- **Write ops/sec**: 100-1,000
- **Data size**: 100GB-10TB
- **Latency**: <100ms (search), <50ms (checkout)

### IoT
- **Queries/sec**: 100-1,000
- **Write ops/sec**: 10,000-1,000,000
- **Data size**: 1TB-100TB
- **Latency**: <1s (ingestion), <5s (aggregation)

### RAG/LLM
- **Queries/sec**: 10-1,000
- **Write ops/sec**: 10-100
- **Data size**: 10GB-1TB
- **Latency**: <100ms (retrieval), <2s (generation)

### SaaS
- **Queries/sec**: 1,000-100,000 (across all tenants)
- **Write ops/sec**: 100-10,000
- **Data size**: 1GB-10TB per tenant
- **Latency**: <50ms (CRUD operations)

## Contributing

Have a use case that should be documented? We welcome contributions!

1. Create an issue describing the use case
2. Follow the template structure from existing guides
3. Include working code examples
4. Add architecture diagrams (ASCII art)
5. Submit a pull request

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for details.

## License

These guides are part of the ThemisDB documentation and are licensed under the same terms as the project. See [LICENSE](../../LICENSE) for details.

---

**Need Help?**

- 📖 Read the [main documentation](../README.md)
- 💬 Join our [Discord community](https://discord.gg/themisdb)
- 🐛 Report issues on [GitHub](https://github.com/yourusername/ThemisDB/issues)
- 📧 Contact support at support@themisdb.com
