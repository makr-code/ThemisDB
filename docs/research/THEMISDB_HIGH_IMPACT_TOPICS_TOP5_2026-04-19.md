# ThemisDB: Wissenschaftliche Top-5-Themen nach Impact x Reifegrad (Stand 2026-04-19)

## Ziel

Dieses Dokument priorisiert die wissenschaftlich wirksamsten Themenfelder von ThemisDB anhand zweier Dimensionen:

- Wissenschaftlicher Impact (0-10): Relevanz im aktuellen Forschungsdiskurs, Zitationsdichte, Top-Venue-Naehe.
- Reifegrad in ThemisDB (0-10): Nachweisbare Implementierung/Adoption im Repository.

Gesamtscore:

`Gesamt = 0.6 * Impact + 0.4 * Reifegrad`

## Top-5 Ranking

| Rang | Themenfeld | Impact | Reifegrad | Gesamt |
|---|---|---:|---:|---:|
| 1 | RAG plus RAG-Evaluation | 9.6 | 9.2 | 9.4 |
| 2 | Vektor-IR und ANN (HNSW, FAISS, Hybrid Search) | 9.4 | 9.1 | 9.3 |
| 3 | LoRA und QLoRA (parameter-effizientes Fine-Tuning) | 9.3 | 9.0 | 9.2 |
| 4 | Effiziente LLM-Inferenz (PagedAttention, Speculative Decoding, Continuous Batching, KV-Cache) | 9.1 | 8.8 | 9.0 |
| 5 | Verteilte transaktionale Multi-Model-DB fuer AI (Raft, MVCC, SAGA, Sharding) | 8.9 | 9.1 | 9.0 |

## Evidenz je Themenfeld (Repository)

### 1) RAG plus RAG-Evaluation

- Forschungseinfluss und Implementierungsstatus:
  - Lewis et al. (RAG): `src/rag/` als implemented
  - Es et al. (RAGAS): `src/llm/monitoring/` als implemented
- Architektur-/Produktnachweis:
  - RAG evaluation in Architektur dokumentiert
  - RAG im Capability-Block des Projekts aufgefuehrt

Relevante Quellen:
- docs/research/implementation_influence/by_paper.md (Lewis et al., Es et al.)
- ARCHITECTURE.md
- README.md

### 2) Vektor-IR und ANN (HNSW, FAISS, Hybrid Search)

- Forschungseinfluss und Implementierungsstatus:
  - ADR HNSW over FAISS accepted
  - Malkov/Yashunin (HNSW) mit `src/index/`, `src/vector/`, `src/rag/` als implemented
- Architektur-/Produktnachweis:
  - HNSW, FAISS-Integration und Hybrid Search explizit dokumentiert

Relevante Quellen:
- docs/research/implementation_influence/by_paper.md (ADR-001, Malkov/Yashunin)
- ARCHITECTURE.md
- README.md

### 3) LoRA und QLoRA

- Forschungseinfluss und Implementierungsstatus:
  - Hu et al. (LoRA): `src/llm/lora/`, `src/training/` implemented
  - Dettmers et al. (QLoRA): `src/llm/lora/` implemented
- Architektur-/Produktnachweis:
  - LoRA-Framework und Multi-LoRA-Management als Kernfaehigkeit dokumentiert

Relevante Quellen:
- docs/research/implementation_influence/by_paper.md (Hu et al., Dettmers et al.)
- ARCHITECTURE.md
- README.md

### 4) Effiziente LLM-Inferenz

- Forschungseinfluss und Implementierungsstatus:
  - Kwon et al. (PagedAttention): `src/llm/` implemented
  - Chen et al. (Speculative Decoding): `src/llm/` implemented
- Architektur-/Produktnachweis:
  - Continuous batching, paged KV-cache, Flash Attention dokumentiert

Relevante Quellen:
- docs/research/implementation_influence/by_paper.md (Kwon et al., Chen et al., Dao et al.)
- ARCHITECTURE.md

### 5) Verteilte transaktionale Multi-Model-DB fuer AI

- Architektur-/Produktnachweis:
  - Distributed Core: Raft/Paxos/Gossip, Sharding
  - Transaction Core: MVCC, SAGA
  - Positionierung als ACID + AI/LLM-native Multi-Model-System

Relevante Quellen:
- README.md
- ARCHITECTURE.md

## Einordnung fuer Publikationsstrategie

- Hoechste Publikationschancen (kurzfristig):
  - RAG plus Evaluation
  - ANN/Hybrid Retrieval
  - LLM-Serving-Optimierungen
- Differenzierungsstarker DB-Systems-Track:
  - ACID-konforme, verteilte AI-native Multi-Model-Architektur

## Hinweise zur Interpretation

- In Informatik ist klassischer Journal-Impact-Factor nur bedingt geeignet; Top-Konferenzwirkung (z. B. ICML, ICLR, OSDI, SOSP, VLDB, ICDE) ist meist aussagekraeftiger.
- Die Scores sind bewusst als priorisierende Entscheidungsheuristik fuer Roadmap, Paper-Planung und Benchmark-Fokus gedacht.
