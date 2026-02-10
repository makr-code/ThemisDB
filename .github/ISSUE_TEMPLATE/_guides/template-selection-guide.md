# Systematic Review Template Selection

Quick reference guide for choosing the right issue template for your ThemisDB component review.

## 🎯 Quick Selection

**Ask yourself: Which category does my component belong to?**

### 1️⃣ Core Database Operations?
- Storage, RocksDB, Persistence
- Transactions, ACID, MVCC
- Query Engine, Optimizer, Executor
- Indexes (B-Tree, LSM, HNSW)
- AQL Parser

**→ Use:** `core_database_component_review.md`

---

### 2️⃣ AI/Machine Learning/LLM?
- LLM Integration (llama.cpp)
- Embeddings, Vector Search
- RAG (Retrieval-Augmented Generation)
- Voice Processing
- Ethics, Bias Detection
- Responsible AI

**→ Use:** `ai_llm_component_review.md`

---

### 3️⃣ Distributed Systems?
- Sharding, Partitioning
- Replication (Master-Slave, Multi-Master)
- Consensus (Raft, Paxos, Gossip)
- CDC (Change Data Capture)
- Distributed Transactions (2PC, 3PC)
- Cross-Shard Operations

**→ Use:** `distributed_systems_component_review.md`

---

### 4️⃣ Network/API/Protocols?
- HTTP/REST APIs
- gRPC
- WebSocket
- MQTT
- PostgreSQL Wire Protocol
- GraphQL
- Protocol implementations

**→ Use:** `network_api_component_review.md`

---

### 5️⃣ Something Else?
- Security/Authentication
- Cache Management
- Analytics
- Content Processing
- Observability/Monitoring
- Geo-spatial
- Time-series
- Graph-specific features
- Any other component

**→ Use:** `SYSTEMATIC_COMPONENT_REVIEW_TEMPLATE.md` (Master Template)

---

## 📋 Component Path → Template Mapping

| Component Path | Template |
|----------------|----------|
| `src/storage/` | core_database_component_review.md |
| `src/transaction/` | core_database_component_review.md |
| `src/query/` | core_database_component_review.md |
| `src/index/` | core_database_component_review.md |
| `src/aql/` | core_database_component_review.md |
| `src/llm/` | ai_llm_component_review.md |
| `src/embeddings/` | ai_llm_component_review.md |
| `src/rag/` | ai_llm_component_review.md |
| `src/voice/` | ai_llm_component_review.md |
| `src/governance/` | ai_llm_component_review.md |
| `src/ethics/` | ai_llm_component_review.md |
| `src/sharding/` | distributed_systems_component_review.md |
| `src/replication/` | distributed_systems_component_review.md |
| `src/cdc/` | distributed_systems_component_review.md |
| `src/api/` | network_api_component_review.md |
| `src/network/` (protocol handlers) | network_api_component_review.md |
| `src/plugins/` (graphql, etc.) | network_api_component_review.md |
| All other paths | SYSTEMATIC_COMPONENT_REVIEW_TEMPLATE.md |

---

## 🔍 When to Use Which Template

### Use Specialized Templates When:
- ✅ Component clearly fits one of the 4 specialized categories
- ✅ You want domain-specific review sections
- ✅ You need detailed checklists for that domain

### Use Master Template When:
- ✅ Component doesn't clearly fit specialized categories
- ✅ Component spans multiple categories
- ✅ You need flexibility to customize sections
- ✅ Component is new and doesn't have a specialized template yet

---

## 🚀 Creating a Review Issue

1. **Choose Template** (use this guide)
2. **Go to GitHub Issues** → New Issue
3. **Select Template** from the list
4. **Fill in Header:**
   - Component Name
   - Component Path
   - Review Period
   - Reviewer(s)
5. **Complete All Sections** systematically
6. **Add Action Items** with owners and dates
7. **Get Sign-Offs** from relevant teams
8. **Schedule Next Review**

---

## 📚 Need More Help?

- **Read:** `SYSTEMATIC_REVIEW_GUIDE.md` - Comprehensive guide
- **Read:** `TEMPLATES_README.md` - All template documentation
- **Ask:** Technical Lead or Architecture Team

---

**Version:** 1.0.0  
**Created:** 2026-02-01  
**Quick Reference by:** ThemisDB Core Team
