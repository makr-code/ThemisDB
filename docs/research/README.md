# ThemisDB Research Documentation - Summary

**Projekt:** ThemisDB  
**Kategorie:** Research Documentation  
**Letzte Aktualisierung:** 1. Februar 2026 (v3.1)

---

## 📋 Dokumenten-Übersicht

Diese Research-Initiative dokumentiert aktuelle Forschungsarbeiten und technische Analysen für ThemisDB.

### Verfügbare Dokumente

1. **[AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)**
   - Umfassende Recherche zu vorhandenen und fehlenden Funktionen
   - Analyse der MCP Server Implementation
   - Bewertung der LLM-Integration
   - Gap-Analyse und Empfehlungen
   - **Status:** ✅ Abgeschlossen (11. Januar 2026)

2. **[AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md](AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md)**
   - Konkrete Code-Beispiele für die Implementierung
   - Schema Manager Klasse Design
   - REST API Endpoint Implementierung
   - MCP Integration Code
   - LLM System-Prompts
   - **Status:** ✅ Design Proof-of-Concept (11. Januar 2026)

3. **[GNN_BASED_INDEXING_AND_EMBEDDINGS.md](GNN_BASED_INDEXING_AND_EMBEDDINGS.md)**
   - Graph Neural Networks für Databases
   - GNN-basierte Indexierungs-Methoden
   - Graph-Aware Embeddings für Query-Optimierung
   - Bewertung des Potenzials für ThemisDB
   - Implementierungs-Roadmap und Empfehlungen
   - **Status:** ✅ Abgeschlossen (27. Januar 2026)

4. **[KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md](KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md)** 🆕
   - Knowledge Graph Embeddings für ThemisDB
   - RotatE, QuatE, ComplEx Methoden-Vergleich
   - Temporal KG Embeddings (TComplEx, TeMP)
   - Multi-Relational Learning Ansätze
   - Integrations-Roadmap und Empfehlungen
5. **[HYBRID_SEARCH_OPTIMIZATION.md](HYBRID_SEARCH_OPTIMIZATION.md)**
   - Dense-Sparse Hybridmethoden (BM25 + Vector Search)
   - Cross-Modal Retrieval (CLIP, ALIGN, BLIP)
   - Multi-Vector Representations (ColBERT, Poly-Encoders)
   - Fusion-Strategien (RRF, Linear Combination)
   - ThemisDB Integration Roadmap
   - **Status:** ✅ Abgeschlossen (27. Januar 2026)

6. **[LEARNED_INDEX_STRUCTURES_RESEARCH.md](LEARNED_INDEX_STRUCTURES_RESEARCH.md)** 🆕
   - Neural Approximate Nearest Neighbor (NANN)
   - Learning to Hash (Deep Hashing, SONG)
   - Learned Space Partitioning (ScaNN, IVF optimization)
   - GNN-Enhanced HNSW Navigation
   - Hybrid Learned/Traditional Indexes
   - Integration mit LearnedQuantizer, LoRA-RAID, GPU Support
   - **Status:** ✅ Abgeschlossen (1. Februar 2026)

---

## 🎯 Forschungsthemen

### 1. Agentic AI Self-Awareness

> **"Gibt es eine Form der self-awareness der Themis (+llama.cpp) die kommunizieren kann welche Daten in der DB gespeichert sind und wie die benutzt werden können (Agentic AI)?"**

Konkret: Kann ein Nutzer die Datenbank fragen:
- "Was kannst du?"
- "Wo sind die Daten?"
- "Wie sind die Daten aufgebaut?"
- "Was ist deine Aufgabe?"
- **"Welche Behördendaten speicherst du?"** (Domain-spezifisch)
- **"Welche LoRA-Adapter sind geladen?"** (LoRA-RAID-Verbund)

**Dokumente:** [AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md), [AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md](AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md)

### 2. Graph Neural Networks für Databases

> **"Wie können Graph Neural Networks die Indexierungs- und Embedding-Fähigkeiten von ThemisDB verbessern?"**

Fokus-Bereiche:
- **GNN-basiertes Indexing:** 10-100x schnellere Subgraph-Queries
- **Graph-Aware Embeddings:** Semantische Suche und Query-Optimierung
- **Multi-Modal Embeddings:** Integration mit LLM und Vector Search
- **Production-Integration:** ONNX Runtime, GPU-Acceleration, RocksDB Cache

**Dokument:** [GNN_BASED_INDEXING_AND_EMBEDDINGS.md](GNN_BASED_INDEXING_AND_EMBEDDINGS.md)

### 3. Knowledge Graph Embeddings

> **"Welche Knowledge Graph Embedding-Methoden eignen sich am besten für ThemisDB, insbesondere für komplexe und temporale Graph-Strukturen?"**

Fokus-Bereiche:
- **RotatE, QuatE, ComplEx:** State-of-the-Art Embedding-Methoden
- **Temporal KG Embeddings:** TComplEx, TeMP für zeitabhängige Wissensgraphen
- **Multi-Relational Learning:** CompGCN, MetaR für heterogene Relationen
- **Production-Integration:** ONNX Runtime, Vector Search, LLM Integration

**Dokument:** [KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md](KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md)

### 4. Hybrid Search Optimization

> **"Wie können Dense- und Sparse-Ansätze kombiniert werden für optimale Suchperformance?"**

Fokus-Bereiche:
- **Dense-Sparse Hybridmethoden:** BM25 + Vector Search mit RRF Fusion
- **Cross-Modal Retrieval:** Text-Image Search mit CLIP, ALIGN, BLIP
- **Multi-Vector Representations:** ColBERT, Poly-Encoders, Aspect-based Search
- **Production Implementation:** API Design, Performance Optimization

**Dokument:** [HYBRID_SEARCH_OPTIMIZATION.md](HYBRID_SEARCH_OPTIMIZATION.md)

### 5. Learned Index Structures

> **"Können Learned Index Structures die Vector Search Performance von ThemisDB signifikant verbessern?"**

Fokus-Bereiche:
- **Neural Approximate Nearest Neighbor (NANN):** End-to-end trainierte Modelle für k-NN
- **Learning to Hash:** Deep Hashing (SONG, HashNet) für kompakte binäre Codes
- **Learned Space Partitioning:** ScaNN, neuronale IVF-Optimierung
- **GNN-Enhanced HNSW:** Learned routing in HNSW graphs (+5-10% recall)
- **Production Integration:** ONNX Runtime, LibTorch, GPU acceleration mit LoRA-RAID

**Dokument:** [LEARNED_INDEX_STRUCTURES_RESEARCH.md](LEARNED_INDEX_STRUCTURES_RESEARCH.md)

---

## ✅ Wichtigste Erkenntnisse

### Agentic AI Self-Awareness

#### 1. Vorhandene Basis (Already Implemented)

ThemisDB hat **bereits eine solide Grundlage** für Agentic AI Self-Awareness:

✅ **Model Context Protocol (MCP) Server** (v1.3.0)
- Vollständige Transport-Layer (stdio, SSE, WebSocket)
- Tool/Resource/Prompt-Architektur
- JSON-RPC Protocol-Handling

✅ **LLM Integration** (v1.3.0+, optional)
- llama.cpp vollständig integriert
- Grammar-Constrained Generation
- ReAct Agent Grammar implementiert
- Vision Support, Flash Attention, Speculative Decoding (v1.4.0-alpha)

✅ **Secondary Index System**
- 7 verschiedene Index-Typen
- Introspection-Methods im Code vorhanden

✅ **Multi-Protocol Support**
- HTTP/REST, gRPC, PostgreSQL Wire, MCP, GraphQL

#### 2. Hauptproblem (Core Issue)

⚠️ **MCP-Integration ist nur "Minimal"**

Die kritischen Komponenten sind als **Stubs** implementiert:

```cpp
// Aktuell:
json McpServer::toolGetSchema(const json& args) {
    return {
        {"nodes", json::array()},  // ❌ Leer!
        {"message", "Schema discovery requires full query engine integration"}
    };
}
```

**Was fehlt:**
- ❌ Vollständige Schema-Discovery
- ❌ Echte Statistiken (Node/Edge Count)
- ❌ Property-Graph Schema-Informationen
- ❌ Index-Metadaten-Abfragen
- ❌ Natural Language Self-Awareness

---

## 🔧 Empfohlene Lösungen

### Implementierungs-Roadmap

#### ✅ **Phase 1: Core Schema Manager** (Priorität: HOCH)

**Aufwand:** ~500 LOC  
**Zeitrahmen:** 1-2 Sprints

- `SchemaManager` Klasse erstellen
- RocksDB Key-Scanning für Table-Discovery
- Property-Type Detection
- Index-Metadaten-Sammlung

#### ✅ **Phase 2: REST API Endpoints** (Priorität: HOCH)

**Aufwand:** ~300 LOC  
**Zeitrahmen:** 1 Sprint

- `GET /api/v1/schema` - Vollständiges Schema
- `GET /api/v1/schema/tables` - Tabellen-Liste
- `GET /api/v1/schema/tables/:name` - Einzelne Tabelle
- `GET /api/v1/capabilities` - Datenbank-Fähigkeiten

#### ✅ **Phase 3: MCP Full Integration** (Priorität: HOCH)

**Aufwand:** ~200 LOC Updates  
**Zeitrahmen:** 1 Sprint

- `toolGetSchema()` mit echten Daten
- `resourceSchema()` auf `SchemaManager` umstellen
- `toolGetStats()` mit RocksDB Statistics

#### ⏰ **Phase 4: Natural Language Self-Awareness** (Priorität: MITTEL)

**Aufwand:** ~400 LOC  
**Zeitrahmen:** 2-3 Sprints

- System-Prompts für Self-Awareness
- LLM Context-Injection (Schema, Stats)
- `toolIntrospectDatabase()` implementieren
- ReAct Agent Loop für "Was kannst du?"-Fragen

#### 🔮 **Phase 5: Domain-Specific Semantic Awareness** (Priorität: MITTEL-HOCH)

**Aufwand:** ~800 LOC  
**Zeitrahmen:** 3-4 Sprints

- Semantic Metadata Store für Business Context
- LLM Content Analysis (Sample-based)
- Entity Extraction (Fachbegriffe, Rechtsnormen)
- Behörden-Use-Cases: "Welche Genehmigungsverfahren verwaltest du?"

#### 🔮 **Phase 6: LoRA-RAID Verbund Awareness** (Priorität: MITTEL-HOCH)

**Aufwand:** ~1300 LOC  
**Zeitrahmen:** 4-5 Sprints

- LoRA Introspection API
- REST API Endpoints (`/api/v1/lora/*`)
- MCP Tools & Resources für LoRA
- Natural Language: "Welche LoRA-Adapter sind geladen?"
- GPU/RAID-Verteilung Transparenz

#### 🔮 **Phase 7: Query Explanation** (Priorität: NIEDRIG)

**Aufwand:** Hoch (Query Planner benötigt)  
**Zeitrahmen:** 4+ Sprints

- EXPLAIN Command für AQL/Cypher
- Query Plan Visualization
- Performance Cost Estimation

---

## 📊 Feature-Matrix

| Feature | Status | Priorität | Aufwand |
|---------|--------|-----------|---------|
| MCP Server Basis | ✅ Vollständig | - | - |
| LLM Integration | ✅ Optional | - | - |
| ReAct Agent Grammar | ✅ Implementiert | - | - |
| **Schema Manager** | ❌ Fehlt | **HOCH** | **MITTEL** |
| **REST /schema API** | ❌ Fehlt | **HOCH** | **NIEDRIG** |
| **MCP Full Integration** | ⚠️ Stub | **HOCH** | **NIEDRIG** |
| **Capabilities Endpoint** | ❌ Fehlt | MITTEL | NIEDRIG |
| **Natural Language Q&A** | ❌ Fehlt | MITTEL | MITTEL-HOCH |
| **Domain-Semantic Awareness** | ❌ Fehlt | MITTEL-HOCH | HOCH |
| **LoRA-RAID Verbund Awareness** | ❌ Fehlt | MITTEL-HOCH | HOCH |
| Query Explanation | ❌ Fehlt | NIEDRIG | HOCH |

**Legende:**
- ✅ Vollständig implementiert
- ⚠️ Teilweise implementiert (Stub)
- ❌ Nicht implementiert

---

## 💡 Schlussfolgerungen

### Agentic AI Self-Awareness

1. **Gute Nachricht:** 
   - ThemisDB hat bereits die **technische Infrastruktur** für Agentic AI Self-Awareness
   - MCP Server, LLM Integration, und Index-System sind vorhanden

2. **Herausforderung:**
   - Die **kritischen Komponenten sind nur Stubs**
   - Schema-Discovery fehlt komplett
   - MCP Tools liefern keine echten Daten

3. **Lösung:**
   - **~1100 LOC neuer Code** + ~200 LOC Updates
   - Implementierung in **3-4 Sprints** möglich
   - Kein komplettes Redesign nötig

**Empfehlung:** Priorität 1 ist Schema-Discovery implementieren. Dies ist der kritische Pfad für alle weiteren Self-Awareness-Features.

### Graph Neural Networks für Databases

1. **Enormes Potenzial:**
   - **10-100x schnellere** Subgraph-Queries (vs. traditionelle Methoden)
   - Neue Features: Semantic Search, Fraud Detection, Recommendation
   - Production-ready Frameworks verfügbar (PyTorch Geometric, ONNX Runtime)

2. **Ideale Basis in ThemisDB:**
   - ✅ Multi-Model (Graph + Vector + Relational)
   - ✅ GPU Support bereits vorhanden
   - ✅ LLM Integration für Semantic Features
   - ✅ RocksDB als robustes Storage-Backend
   - ✅ LoRA-RAID für Multi-GPU Training

3. **Implementierungs-Roadmap:**
   - **Phase 1 (Q2 2026):** Proof of Concept - 3 Monate
   - **Phase 2 (Q3 2026):** Production Features - 3 Monate
   - **Phase 3 (Q4 2026+):** Advanced Features - 4 Monate
   - **Gesamt:** ~18,000 LOC, 10 Monate

**Empfehlung:** ✅ **GRÜNES LICHT** für GNN-Integration. Die Investition ist gerechtfertigt durch signifikante Performance-Verbesserungen und neue Produktfeatures.

### Knowledge Graph Embeddings

1. **Beste Baseline:**
   - **RotatE** bietet exzellente Performance bei geringem Ressourcenaufwand
   - Robuste Modellierung aller wichtigen Relationstypen
   - 10-100x schnellere Subgraph-Queries möglich

2. **Höchste Expressivität:**
   - **QuatE** ermöglicht komplexe Beziehungen durch Quaternionen-Algebra
   - Best-in-Class für komplexe Relationen
   - Höherer Rechenaufwand als RotatE

3. **Temporale Daten:**
   - **TComplEx** ist essentiell für zeitabhängige Knowledge Graphs
   - State-of-the-Art für evolvierende Wissensgraphen
   - Natürliche Erweiterung von ComplEx

4. **Implementierungs-Roadmap:**
   - **Phase 1 (Q2 2026):** RotatE + ComplEx - 2 Monate
   - **Phase 2 (Q3 2026):** TComplEx + QuatE - 3 Monate
   - **Phase 3 (Q4 2026+):** CompGCN/MetaR - 2 Monate
   - **Gesamt:** ~10,000 LOC, 7 Monate

**Empfehlung:** ✅ **GRÜNES LICHT** für KG Embedding Integration. ThemisDB ist ideal positioniert durch bestehende Vector Search, GPU Support, und LLM Integration.
### Hybrid Search Optimization

1. **Essentiell für moderne Suche:**
   - **Hybrid Search (BM25 + Dense Vector)** verbessert Recall um 5-15%
   - **RRF (Reciprocal Rank Fusion)** als robuste, einfache Fusion-Methode
   - ThemisDB fehlt aktuell BM25 Sparse Retrieval

2. **Cross-Modal Retrieval als Differentiator:**
   - ✅ CLIP Integration für Text-Image Search
   - ✅ Neue Use Cases: E-Commerce, Media, Compliance
   - ✅ Python Microservice oder ONNX Runtime Integration möglich

3. **Implementierungs-Roadmap:**
   - **Phase 1 (3 Monate):** BM25 + RRF Fusion - ~2000 LOC
   - **Phase 2 (4 Monate):** CLIP Cross-Modal - ~3000 LOC
   - **Phase 3 (3 Monate):** Multi-Vector - ~1500 LOC
   - **Gesamt:** ~6500 LOC, 10 Monate

**Empfehlung:** ✅ **P0-PRIORITÄT** für Hybrid Search (Phase 1). Schließt Feature-Gap zu Weaviate/Vespa und ist Industry Standard.

---

## 📚 Nächste Schritte

### Für Entwickler

**Agentic AI:**
1. **Lesen:** [AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)
2. **Design Review:** [AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md](AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md)
3. **Issue erstellen:** "Implement Full MCP Schema Integration"
4. **Proof-of-Concept:** Schema-Discovery aus RocksDB

**GNN Research:**
1. **Lesen:** [GNN_BASED_INDEXING_AND_EMBEDDINGS.md](GNN_BASED_INDEXING_AND_EMBEDDINGS.md)
2. **Evaluation:** Bewertung der GNN-Ansätze für spezifische Use Cases
3. **Proof-of-Concept:** GNN Training Pipeline (Python/PyTorch)
4. **Prototype:** ONNX Inference Integration in C++

**KG Embeddings Research:**
1. **Lesen:** [KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md](KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md)
2. **Evaluation:** Vergleich von RotatE, QuatE, ComplEx für ThemisDB Use Cases
3. **Proof-of-Concept:** RotatE Training Pipeline und Link Prediction
4. **Prototype:** ONNX Integration für Embedding Inference
**Hybrid Search:**
1. **Lesen:** [HYBRID_SEARCH_OPTIMIZATION.md](HYBRID_SEARCH_OPTIMIZATION.md)
2. **Spike:** BM25 Proof-of-Concept in RocksDB (1 Sprint)
3. **Design:** Hybrid Search API Design Review
4. **Benchmark:** BEIR Evaluation Setup

### Für Product Owner

**Agentic AI:**
1. **Priorisierung:** Schema-Discovery als High-Priority Feature
2. **Sprint Planning:** 3-4 Sprints für vollständige Implementation
3. **Milestone:** "Self-Aware ThemisDB v1.5"

**GNN Integration:**
1. **Team-Aufbau:** 2-3 ML Engineers für GNN-Integration
2. **Infrastruktur:** GPU-Ressourcen bereitstellen
3. **Sprint Planning:** 10 Monate für vollständige GNN-Integration
4. **Milestone:** "GNN-Enhanced ThemisDB v1.6"

**KG Embeddings Integration:**
1. **Team-Aufbau:** 2-3 ML Engineers für KG Embeddings
2. **Infrastruktur:** GPU-Ressourcen (bestehende LoRA-RAID nutzbar)
3. **Sprint Planning:** 7 Monate für vollständige KG Embedding Integration
4. **Milestone:** "KG Embedding-Enhanced ThemisDB v1.5"
**Hybrid Search:**
1. **Priorisierung:** Hybrid Search als P0 Feature für v1.5
2. **Sprint Planning:** 3-4 Monate für Phase 1 (BM25 + RRF)
3. **Cross-Modal:** 4 Monate für Phase 2 (CLIP Integration)
4. **Milestone:** "Hybrid Search ThemisDB v1.5"

### Für Community

1. **Feedback:** Welche Features sind am wichtigsten?
2. **Use Cases:** Konkrete Anwendungsszenarien für GNN-Indexing und KG Embeddings
2. **Use Cases:** Konkrete Anwendungsszenarien für GNN-Indexing und Hybrid Search
3. **Testing:** Beta-Testing für neue Features
4. **Benchmarks:** BEIR und MTEB Evaluation Results

---

## 🔗 Weitere Ressourcen

### ThemisDB Dokumentation

**Agentic AI:**
- [MCP Protocol Support](../apis/MCP_PROTOCOL_SUPPORT.md)
- [LLM Integration README](../llm/README.md)
- [HTTP API Reference](../apis/HTTP_API_REFERENCE.md)

**GNN & Machine Learning:**
- [Vector Search Documentation](../features/) (if exists)
- [LoRA-RAID System](../../LORA_ADAPTER_IMPLEMENTATION_COMPLETE.md)
- [GPU Acceleration Guide](../performance/)
- [Knowledge Graph Embeddings Research](./KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md)

**Hybrid Search:**
- [Vector Operations Guide](../de/features/features_vector_ops.md)
- [Vector Search Example](../../examples/07_vector_search_documents/)
- [HNSW Index Configuration](../de/features/features_vector_ops.md)

### External Resources

**Agentic AI:**
- [Model Context Protocol Specification](https://modelcontextprotocol.io/)
- [Anthropic Claude Desktop Integration](https://docs.anthropic.com/claude/docs)
- [llama.cpp Documentation](https://github.com/ggerganov/llama.cpp)

**GNN & Graph ML:**
- [PyTorch Geometric](https://pytorch-geometric.readthedocs.io/) - GNN Training Framework
- [Deep Graph Library (DGL)](https://www.dgl.ai/) - Alternative GNN Framework
- [ONNX Runtime](https://onnxruntime.ai/) - C++ Inference
- [Stanford CS224W](http://web.stanford.edu/class/cs224w/) - Graph ML Course
- [Open Graph Benchmark](https://ogb.stanford.edu/) - GNN Benchmarks

**Hybrid Search & Embeddings:**
- [Sentence Transformers](https://www.sbert.net/) - Dense Embeddings
- [OpenCLIP](https://github.com/mlfoundations/open_clip) - Cross-Modal Models
- [Haystack](https://haystack.deepset.ai/) - Hybrid Search Pipeline
- [BEIR Benchmark](https://github.com/beir-cellar/beir) - Retrieval Evaluation
- [MTEB Leaderboard](https://huggingface.co/spaces/mteb/leaderboard) - Embedding Models

**Research Papers:**
- Kraska et al. (2018): "The Case for Learned Index Structures"
- Marcus et al. (2019): "Neo: A Learned Query Optimizer"
- Sun et al. (2020): "Neural Subgraph Matching"
- Hamilton et al. (2017): "GraphSAGE: Inductive Representation Learning"
- Sun et al. (2019): "RotatE: Knowledge Graph Embedding by Relational Rotation"
- Zhang et al. (2019): "Quaternion Knowledge Graph Embeddings"
- Trouillon et al. (2016): "Complex Embeddings for Simple Link Prediction"
- Cormack et al. (2009): "Reciprocal Rank Fusion"
- Khattab & Zaharia (2020): "ColBERT: Contextualized Late Interaction"
- Radford et al. (2021): "CLIP: Learning Transferable Visual Models"

**Learned Index Structures:**
- [SONG Implementation](https://github.com/amazon-science/nearest-neighbor-search) - GPU-optimized learned hashing
- [Learned Index Structures](https://github.com/learnedsystems/RMI) - Original RMI implementation
- [PyTorch](https://pytorch.org/) - Deep learning framework
- [ONNX Runtime](https://onnxruntime.ai/) - Fast inference
- [ANN Benchmarks](http://ann-benchmarks.com/) - Comprehensive evaluation

---

**Erstellt:** 11. Januar 2026  
**Letzte Aktualisierung:** 1. Februar 2026  
**Autor:** Research Team  
**Version:** 3.1

---

## 📝 Changelog

| Datum | Version | Änderungen |
|-------|---------|------------|
| 2026-02-01 | 3.1 | Learned Index Structures Research hinzugefügt |
| 2026-01-27 | 3.0 | KG Embeddings Research hinzugefügt |
| 2026-01-27 | 3.0 | Hybrid Search Optimization Research hinzugefügt |
| 2026-01-27 | 2.0 | GNN Research hinzugefügt, README umstrukturiert |
| 2026-01-11 | 1.0 | Initiale Research Documentation (Agentic AI) |
