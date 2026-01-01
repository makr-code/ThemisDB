# ThemisDB v1.3.1 - Third-Party Attribution Documentation

**Release Date:** 20. Dezember 2025  
**Focus:** Legal & Compliance Documentation

---

## 🎉 Overview

ThemisDB v1.3.1 introduces comprehensive third-party attribution documentation, providing full transparency about the project's dependencies and clearly distinguishing ThemisDB's unique innovations from third-party library features.

---

## 📄 New Features

### Third-Party Attribution Documentation (PR #119)

- **ATTRIBUTIONS.md**: Comprehensive documentation of 15+ core dependencies
  - RocksDB - LSM-tree storage engine
  - FAISS - Vector similarity search
  - hnswlib - HNSW index implementation
  - simdjson - High-performance JSON parsing
  - Apache Arrow - Columnar data format
  - TBB (Threading Building Blocks) - Parallel computing
  - Boost - C++ libraries collection
  - OpenSSL - Cryptography and TLS

- **Innovation Documentation**: Clearly documented ThemisDB's **12 unique innovations**:
  1. Multi-model ACID transactions (property graph + document + vector + relational in one transaction)
  2. AQL (Themis Query Language) - Cypher-inspired with multi-model extensions
  3. Embedding semantic cache with fuzzy matching
  4. Hybrid search (BM25 + vector) with RRF fusion
  5. Graph-to-relational transformation engine
  6. MVCC-based snapshot isolation on top of RocksDB
  7. Native LLM integration with llama.cpp (optional)
  8. Plugin architecture for LLM backends
  9. Image analysis AI plugin system
  10. Network protocol enhancements (HTTP/2, WebSocket, MQTT, HTTP/3, PostgreSQL wire, MCP)
  11. Time-series hypertables with automatic partitioning
  12. Enterprise features (sharding, multi-master, geo-replication)

- **License Information**: Complete license details and repository links for all dependencies
- **Compliance**: Ensures proper attribution and license compatibility

---

## 🏆 Benefits

- **Legal Transparency**: Clear documentation of all third-party dependencies
- **Innovation Clarity**: Distinguishes ThemisDB's unique contributions from library features
- **Compliance**: Meets open-source license attribution requirements
- **Community Trust**: Demonstrates commitment to proper attribution and transparency

---

## 📚 Documentation

- [ATTRIBUTIONS.md](../legal/ATTRIBUTIONS.md) - Complete third-party attribution list
- [LICENSE](../../LICENSE) - ThemisDB MIT License

---

## 🔄 Upgrade Notes

No code changes in this release - documentation only. No upgrade steps required.

---

## 📦 Compatibility

- **Backward Compatible**: Yes - 100% compatible with v1.3.0
- **Database Format**: No changes
- **API**: No changes
- **Configuration**: No changes

---

## 🔗 Links

- [GitHub Release](https://github.com/makr-code/ThemisDB/releases/tag/v1.3.1)
- [Changelog](CHANGELOG.md)
- [PR #119](https://github.com/makr-code/ThemisDB/pull/119)
