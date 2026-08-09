# ThemisDB Research Prioritization Snapshot — Top 5 Topics (Stand 2026-08-09)

**Status**: In Review  
**Version**: 0.2  
**Last Updated**: 2026-08-09  
**Scope**: Editorial research-prioritization memo for publication planning; not a canonical implementation-maturity source.  
**Lifecycle**: ACTIVE_DRAFT  
**Canonical source-of-truth for implementation status**:
- `ROADMAP.md`
- `ARCHITECTURE.md`
- `README.md`
- `src/rag/README.md`
- `src/llm/ROADMAP.md`
- `src/index/ARCHITECTURE.md`
- `src/training/README.md`
- `src/sharding/ROADMAP.md`
- `docs/architecture/transaction_coordinators.md`

---

## Abstract

Dieses Memo priorisiert fuenf ThemisDB-Themenfelder, die zugleich im aktuellen
Forschungsdiskurs sichtbar und im Repository bereits substanziell verankert
sind. Die Priorisierung ist **heuristisch**: Sie kombiniert (a) die Sichtbarkeit
eines Themas in aktuellen Datenbank-, Retrieval- und LLM-Research-Linien mit
(b) der im Repository belegten Evidenz aus Modul-Dokumentation, Tests und
Benchmarks. Anders als die fruehere Fassung behandelt dieses Dokument
Implementierungsreife **nicht** als frei erfundene Maturity-Skala, sondern als
evidenzgestuetzte Einordnung entlang der vorhandenen Status-Artefakte
(`Implemented`, `In Progress`, `HARDENING`, `PRODUCTION_CANDIDATE`).

Die staerksten kurzfristigen Paper-Kandidaten sind derzeit
**RAG mit integrierter Qualitaetsbewertung**, **hybrides Vektor-/Text-Retrieval**
und **datenbank-native LoRA/QLoRA-Lebenszyklen**. Themen wie
**effiziente LLM-Inferenz** und die **verteilte transaktionale Multi-Model-Datenbank**
sind ebenfalls hochrelevant, werden aber im aktuellen Repository eher durch
Hot-Path-Benchmarks, Hardening-Artefakte und Architektur-/Roadmap-Belege als
durch vollstaendige End-to-End-Vergleichsstudien getragen. Das Dokument soll
daher die Argumentationskette fuer Forschungspapiere schaerfen, ohne
weitergehende empirische Reife zu behaupten als derzeit quellenbasiert belegt
ist.

## Introduction / Einleitung

ThemisDB positioniert sich als Multi-Model-Datenbank mit nativer AI/LLM-
Integration. Fuer Forschungs- und Publikationsplanung ist damit nicht nur
entscheidend, **welche Themen technisch vorhanden sind**, sondern auch **welche
Themen einen belastbaren Nachweis in Code, Tests, Benchmarks und Architektur-
Artefakten besitzen**. Die urspruengliche Fassung dieses Dokuments vermischte
beides zu numerischen Reifegraden, ohne diese gegen die kanonischen Quellen des
Repositories abzusichern.

Die ueberarbeitete Fassung beantwortet daher eine engere Frage:

> Welche fuenf Themenfelder sind fuer ThemisDB aktuell die plausibelsten
> Forschungsschwerpunkte, wenn man Literaturrelevanz und bereits vorhandene
> Repository-Evidenz gemeinsam betrachtet?

Dabei gilt:

- **AQL** wird konsistent als **Advanced Query Language** bezeichnet.
- **Multi-Model** meint die gemeinsame Verarbeitung von relationalen, Graph-,
  Vektor-, Dokument-, Geo- und Zeitreihen-Daten innerhalb einer Architektur.
- **Konsistenzmodell** meint in diesem Dokument die in den Architektur- und
  Transaktionsartefakten dokumentierten Mechanismen wie MVCC, 2PC/3PC, SAGA und
  verteilte Koordination ueber Raft/Paxos/Gossip.

## Methodik / Ansatz

### Bewertungslogik

Die Rangfolge nutzt weiterhin eine kompakte Priorisierungsheuristik:

`Gesamt = 0.6 * Forschungseinfluss + 0.4 * Evidenzreife`

mit folgenden Regeln:

- **Forschungseinfluss (0-10)**: qualitative Einordnung der Sichtbarkeit und
  Aktualitaet eines Themas in einschlaegigen Paper-Linien und System-Communities.
- **Evidenzreife (0-10)**: Einordnung anhand von kanonischen Repo-Artefakten:
  Modul-Docs, `ROADMAP.md`, explizite Tests, Benchmarks, Architektur- oder
  Vertragsdokumente.

### Was hier ausdruecklich nicht behauptet wird

- Die Scores sind **keine** offizielle Release-Maturity.
- Root-Dokumente wie `README.md` und `ARCHITECTURE.md` dienen nur als
  zusammenfassende Orientierung; Modulstatus wird gegen `ROADMAP.md` und
  modulnahe Dokumente abgeglichen.
- Mikrobenchmarks, Hot-Path-Benchmarks und fokussierte Tests zaehlen als
  Evidenz, ersetzen aber **keine** vollstaendige publikationsreife
  Vergleichsstudie.

## Top-5-Ranking

| Rang | Themenfeld | Forschungseinfluss | Evidenzreife | Gesamt | Kurzbegruendung |
|---|---|---:|---:|---:|---|
| 1 | RAG mit integrierter Qualitaetsbewertung | 9.6 | 8.9 | 9.3 | Starkes Forschungsthema; im Repo durch RAG-Runtime, Qualitaets-Pipeline und Wiki-RAG-Tests direkt belegt |
| 2 | Hybrides Vektor-/Text-Retrieval (HNSW, FAISS, BM25+RRF) | 9.4 | 8.8 | 9.2 | Hohe Relevanz fuer Retrieval-Systeme; Evidenz in Index-/Search-/RAG-Artefakten und Benchmarks |
| 3 | Datenbank-native LoRA/QLoRA-Lebenszyklen | 9.3 | 8.6 | 9.0 | LoRA/QLoRA sind forschungsnah und im Training-/LLM-Stack breit verankert, aber Paper-grade Betriebsmetriken sind noch selektiv |
| 4 | Effiziente LLM-Inferenz (Paged KV Cache, Continuous Batching, Speculative Decoding) | 9.1 | 8.5 | 8.9 | Architektur- und Hardening-Nachweis ist stark; viele Messungen sind derzeit Hot-Path- statt Vollsystem-Evaluationen |
| 5 | Verteilte transaktionale Multi-Model-Datenbank fuer AI (MVCC, 2PC/3PC, SAGA, Raft, Sharding) | 8.9 | 8.8 | 8.9 | Systemisch hoch differenzierend; belegt durch Sharding-/Transaktionsdokumente, fokussierte Tests und Release-Gate-Benchmarks |

## Evaluation / Experimente

### Repository-basierte Evidenzmatrix

| Themenfeld | Kanonische Repo-Evidenz | Was tatsaechlich belegt ist | Was fuer ein Paper noch fehlt |
|---|---|---|---|
| RAG mit Qualitaetsbewertung | `src/rag/README.md`; `tests/llm/test_wiki_rag_quality.cpp`; `benchmarks/rag/bench_rag_hybrid_retriever.cpp`; `research/implementation_influence/by_module.md` | RAG-Runtime, Qualitaets-Gates, Recall@5-Ziel von mindestens 80 %, Latenz-Ziel unter 200 ms fuer 10 Queries auf 100 Chunks, Hybrid-Fusion-Mikrobenchmark | Groessere reale Korpora, konkurrierende Baselines, belastbare End-to-End-Studie unter Last |
| Hybrides Vektor-/Text-Retrieval | `src/index/ARCHITECTURE.md`; `benchmarks/bench_vector_search.cpp`; `README.md`; ADR-001 in `research/architecture_decisions/adr_001_hnsw_over_faiss_vector_index.md` | HNSW-basierte Vektorsuche, RocksDB-gestuetzte Benchmark-Umgebung, FAISS als dokumentierte Integrationslinie, BM25+RRF-Hybridisierung im RAG-Pfad | Vergleich gegen externe ANN-Systeme und konsistente Recall/QPS-Auswertung auf Standarddatensaetzen |
| LoRA/QLoRA | `src/training/README.md`; `src/llm/ROADMAP.md`; `research/implementation_influence/by_module.md`; `benchmarks/llm/bench_llm_inference_performance.cpp` | LoRA-, AdaLoRA- und QLoRA-nahe Trainings-/Adapter-Artefakte, Hot-Swap- und Lifecycle-Nachweise im LLM-/Training-Stack | Durchgaengige Betriebsmetriken fuer Fine-Tuning-Kosten, Adapter-Wechsel unter Last und Persistenz-/Rollback-Pfade |
| Effiziente LLM-Inferenz | `src/llm/ROADMAP.md`; `benchmarks/llm/bench_llm_hotpaths.cpp`; `ARCHITECTURE.md`; `include/llm/paged_kv_cache.h`; `src/llm/speculative_decoder.cpp` | Paged-KV-Cache-, Continuous-Batching- und Speculative-Decoding-Pfade sind source-backed; es existieren acht LLM-Hot-Path-Gates (`LLM-01..08`) | Reale Modell- und Multi-Tenant-End-to-End-Messungen, offene Vergleichsbasis gegen dedizierte Serving-Systeme |
| Verteilte transaktionale Multi-Model-Datenbank | `src/sharding/ROADMAP.md`; `docs/architecture/transaction_coordinators.md`; `benchmarks/sharding/bench_sharding_release_gates.cpp`; `README.md` | 2PC/3PC-/SAGA-Koordinatoren, Recovery-/Failover-Tests (`TXC-01..32`, `FLR-01..20`, `FI-01..40`), sechs Sharding-Release-Gates (`SRG-01..06`) | Mehr Multi-Node-Live-Evaluation, laenger laufende Topologie-Churn-Szenarien und Cross-Shard-Workload-Vergleiche |

### Einordnung pro Rang

#### 1) RAG mit integrierter Qualitaetsbewertung

Dies ist derzeit das am besten vermarktbare Forschungsthema fuer ThemisDB, weil
es zwei aktive Linien verbindet: RAG als dominantes Anwendungsparadigma und
qualitative Evaluation mit Metriken wie Faithfulness, Relevance oder Recall.
Repository-seitig ist das Thema nicht nur architektonisch, sondern auch durch
Tests und Benchmarks abgestuetzt. Besonders wichtig ist, dass die aktuelle
Evidenz **nicht** nur Retrieval, sondern auch Bewertungsbausteine umfasst.

#### 2) Hybrides Vektor-/Text-Retrieval

Der zweite Rang ist durch die Kombination aus HNSW, FAISS-Integrationslinie und
hybrider BM25-/Vektor-Fusion begruendet. ThemisDB kann hier ein klassisches
IR-/ANN-Thema mit Datenbankperspektive verbinden. Die aktuelle Evidenz ist
technisch solide, aber fuer eine wissenschaftliche Vergleichsarbeit fehlen noch
normierte Baselines und standardisierte Datensatzberichte.

#### 3) Datenbank-native LoRA/QLoRA-Lebenszyklen

LoRA und QLoRA sind im aktuellen LLM-Oekosystem weiterhin hochrelevant. Im
ThemisDB-Repository ist ihre Implementierung nicht auf ein isoliertes
Experiment beschraenkt, sondern ueber `src/training/` und `src/llm/` verteilt:
Training, Adapter-Management, Lifecycle, Mixed Precision und Serving-Uebergaben
sind als zusammenhaengender Stack sichtbar. Fuer ein starkes Paper fehlt jedoch
noch eine konsolidierte betriebliche Auswertung ueber Kosten, Latenz und
Fehlerfaelle hinweg.

#### 4) Effiziente LLM-Inferenz

Paged KV Cache, Continuous Batching und Speculative Decoding gehoeren zu den
wichtigsten Systemthemen aktueller LLM-Inferenz. ThemisDB besitzt dafuer
quellenbasierte Architektur- und Code-Pfade sowie release-nahe Hot-Path-
Benchmarks. Diese Evidenz ist stark genug fuer einen Systems-Paper-Entwurf,
aber noch nicht identisch mit einer voll ausgearbeiteten End-to-End-Serving-
Studie mit realen Modellen und externen Baselines.

#### 5) Verteilte transaktionale Multi-Model-Datenbank fuer AI

Dieses Thema ist der systemische Differenzierer von ThemisDB. Es kombiniert
verteilte Koordination, ACID-orientierte Transaktionsmechanismen und
AI-nahe Workloads in einer Architektur. Die Belege sind fuer ein
Architektur-/Systems-Paper bereits beachtlich: Koordinatoren, Recovery-Pfade,
Fault-Injection-Tests und Release-Gate-Benchmarks sind vorhanden. Gleichwohl
zeigen die Modul-Roadmaps, dass weiterfuehrende verteilte Hardening-Arbeiten
noch laufen; deshalb sollte das Thema argumentativ stark, aber empirisch
vorsichtig formuliert werden.

## Limitations / Known Issues

- Dieses Dokument ist **nicht kanonisch** fuer Release- oder Modulreife.
- Die numerischen Scores bleiben eine **heuristische Redaktionseinschaetzung**.
- Ein Teil der belegten Performance stammt aus **Mikro- oder Hot-Path-
  Benchmarks**, nicht aus vollstaendigen produktionsnahen Vergleichsstudien.
- Die Forschungs-Mappings unter `research/implementation_influence/` sind
  nuetzliche Querverweise, enthalten aber einzelne veraltete Pfadangaben und
  duerfen deshalb Level-1-Moduldokumente nicht uebersteuern.
- Fuer mehrere Themen fehlen noch systematische Multi-Node-, Langzeit- oder
  Standarddatensatz-Evaluationen.

## Fazit

Fuer kurzfristige, review-faehige Publikationsarbeit sind derzeit **RAG mit
Qualitaetsbewertung**, **hybrides Retrieval** und **LoRA/QLoRA im
Datenbankkontext** die tragfaehigsten Themen. **Effiziente LLM-Inferenz** und
die **verteilte transaktionale Multi-Model-Architektur** bleiben strategisch
hochattraktiv, sollten aber in Einreichungen klar zwischen bereits
repo-belegten Claims und noch ausstehenden, breit angelegten Evaluationen
trennen.

## References / Quellen

### Externe Literatur

1. Lewis, P. et al. (2020). *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks*. NeurIPS. URL: https://arxiv.org/abs/2005.11401
2. Es, S. et al. (2023/2024). *RAGAS: Automated Evaluation of Retrieval Augmented Generation*. EACL Findings. URL: https://arxiv.org/abs/2309.15217
3. Malkov, Y. A., & Yashunin, D. A. (2020). *Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs*. IEEE TPAMI. URL: https://arxiv.org/abs/1603.09320
4. Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models*. ICLR. URL: https://arxiv.org/abs/2106.09685
5. Dettmers, T. et al. (2023). *QLoRA: Efficient Finetuning of Quantized LLMs*. URL: https://arxiv.org/abs/2305.14314
6. Dao, T. et al. (2022). *FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness*. NeurIPS. URL: https://arxiv.org/abs/2205.14135
7. Chen, C. et al. (2023). *Accelerating Large Language Model Decoding with Speculative Sampling*. URL: https://arxiv.org/abs/2302.01318
8. Kwon, W. et al. (2023). *Efficient Memory Management for Large Language Model Serving with PagedAttention*. SOSP. URL: https://arxiv.org/abs/2309.06180
9. Ongaro, D., & Ousterhout, J. (2014). *In Search of an Understandable Consensus Algorithm (Raft)*. USENIX ATC. URL: https://www.usenix.org/conference/atc14/technical-sessions/presentation/ongaro
10. Garcia-Molina, H., & Salem, K. (1987). *SAGAS*. ACM SIGMOD. DOI: https://doi.org/10.1145/38713.38742

### Interne Artefakte (Repository-Evidenz)

- `ROADMAP.md`
- `ARCHITECTURE.md`
- `README.md`
- `src/rag/README.md`
- `src/llm/ROADMAP.md`
- `src/index/ARCHITECTURE.md`
- `src/training/README.md`
- `src/sharding/ROADMAP.md`
- `docs/architecture/transaction_coordinators.md`
- `tests/llm/test_wiki_rag_quality.cpp`
- `benchmarks/rag/bench_rag_hybrid_retriever.cpp`
- `benchmarks/bench_vector_search.cpp`
- `benchmarks/llm/bench_llm_hotpaths.cpp`
- `benchmarks/sharding/bench_sharding_release_gates.cpp`
