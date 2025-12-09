# Architecture Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Architecture

---

## Übersicht

ThemisDB ist eine Multi-Model-Datenbank basierend auf RocksDB (LSM-Tree) mit ACID-Garantien und umfassender Security-Architektur.

## Source-Code Basis

| Komponente | Headers | Sources | LOC | Beschreibung |
|------------|---------|---------|-----|--------------|
| server | 20 | 20 | 18,282 | HTTP Server, REST API |
| index | 12 | 11 | 14,629 | Secondary, Vector, Graph |
| query | 12 | 12 | 12,560 | AQL Parser, Optimizer |
| sharding | 21 | 19 | 12,278 | Horizontal Scaling |
| content | 16 | 15 | 9,091 | Content Pipeline |
| security | 16 | 16 | 8,138 | Encryption, RBAC |
| storage | 9 | 10 | 4,591 | RocksDB, Entities |
| analytics | 3 | 2 | 3,742 | OLAP, CEP |
| timeseries | 7 | 8 | 2,767 | Gorilla, Retention |
| replication | 2 | 1 | 1,612 | Multi-Master, CRDTs |

**Gesamt:** 132 Header, 124 Sources, ~91,000 LOC

## Kern-Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                      HTTP Server                             │
│  (Boost.Beast, REST API, Rate Limiting, Auth Middleware)    │
├─────────────────────────────────────────────────────────────┤
│                      Query Layer                             │
│  (AQL Parser, Optimizer, Execution Engine, Semantic Cache)  │
├──────────────┬──────────────┬──────────────┬────────────────┤
│  Secondary   │   Vector     │    Graph     │   Fulltext     │
│   Index      │   Index      │   Index      │    Index       │
│  (B-Tree)    │  (HNSW)      │ (Adjacency)  │ (Inverted)     │
├──────────────┴──────────────┴──────────────┴────────────────┤
│                   Transaction Manager                        │
│  (MVCC, Snapshot Isolation, Write-Write Conflict Detection) │
├─────────────────────────────────────────────────────────────┤
│                    RocksDB Wrapper                           │
│  (LSM-Tree, WAL, BlobDB, Column Families, Compression)      │
└─────────────────────────────────────────────────────────────┘
```

## Multi-Model Support

| Modell | Logical Entity | Physical Storage | Key Format |
|--------|----------------|------------------|------------|
| Relational | Row | (PK, Blob) | `table:pk` |
| Document | JSON Document | (PK, Blob) | `collection:pk` |
| Graph Node | Vertex | (PK, Blob) | `node:pk` |
| Graph Edge | Edge | (PK, Blob) | `edge:pk` |
| Vector | Embedding | (PK, Blob) | `object:pk` |
| TimeSeries | DataPoint | (TS Key, Value) | `ts:metric:entity:timestamp` |

## Dokumentation in diesem Ordner

### Core Architecture
| Datei | Beschreibung |
|-------|--------------|
| [architecture_overview.md](architecture_overview.md) | System-Architektur Übersicht |
| [architecture_multi_model.md](architecture_multi_model.md) | Multi-Model Design |
| [architecture_ecosystem.md](architecture_ecosystem.md) | Ecosystem Overview |

### Storage
| Datei | Beschreibung |
|-------|--------------|
| [architecture_base_entity.md](architecture_base_entity.md) | BaseEntity Storage |
| [architecture_mvcc.md](architecture_mvcc.md) | MVCC Transaction Design |

### Content Pipeline
| Datei | Beschreibung |
|-------|--------------|
| [architecture_content.md](architecture_content.md) | Content Management |
| [architecture_content_pipeline.md](architecture_content_pipeline.md) | Processing Pipeline |

### Caching
| Datei | Beschreibung |
|-------|--------------|
| [architecture_cache_invalidation.md](architecture_cache_invalidation.md) | Cache Invalidation |
| [architecture_caching_patterns.md](architecture_caching_patterns.md) | Lookup Patterns |
| [architecture_caching_structures.md](architecture_caching_structures.md) | Data Structures |

## Verwandte Dokumentation

- [Storage Module](../storage/README.md) - RocksDB Wrapper Details
- [Index Module](../index/README.md) - Index Implementations
- [Query Module](../query/README.md) - Query Engine
- [Features Overview](../features/features_overview.md) - Feature-Liste
