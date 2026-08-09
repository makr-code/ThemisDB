# ThemisDB: Venue-Mapping fuer wissenschaftliche Top-5-Themen (Stand 2026-04-19, Review-Update 2026-08-09)

## Abstract / Zusammenfassung

Dieses Dokument mappt fuenf priorisierte ThemisDB-Forschungsthemen auf geeignete Venue-Familien (DB-Systems, Systems/Serving, NLP-Industry) und bewertet dafuer die aktuelle Evidenzlage im Repository. Alle zentralen Aussagen wurden auf konkrete Code-, Test-, Benchmark- oder Architekturartefakte zurueckgefuehrt. Ergebnis: Die Themen sind strategisch passend, aber nicht gleich weit in der empirischen Reife. Fuer kurzfristig hohe Einreichungsreife sind vor allem (1) RAG+Evaluation und (2) ANN/Hybrid Retrieval geeignet; LoRA/QLoRA und LLM-Serving sind in Teilen evidenzstark, benoetigen aber engere produktionsnahe Messungen; das verteilte ACID-Multi-Model-Thema ist inhaltlich stark, erfordert jedoch konsolidierte End-to-End-Metriken ueber Konsistenz und AI-Pfade.

## Introduction / Einleitung

Ziel ist ein belastbares Venue-Mapping fuer folgende Top-5-Themen:

1. RAG plus RAG-Evaluation
2. Vektor-IR und ANN (inkl. Hybrid Retrieval)
3. LoRA und QLoRA im datenbank-nativen Betrieb
4. Effiziente LLM-Inferenz (PagedAttention, Speculative Decoding, Continuous Batching)
5. Verteilte ACID Multi-Model-DB fuer AI

Die Version davor war strukturell unvollstaendig und enthielt mehrere unbelegte Kurzclaims. Diese Revision fuehrt eine nachvollziehbare Kette ein: Problem -> Ansatz -> Evaluation -> Grenzen -> Priorisierung.

## Methodik / Ansatz

### Faktencheck-Regeln

- Claims wurden nur behalten, wenn mindestens eine pruefbare Quelle vorliegt:
  - **Code-/Modulbezug** (README/ROADMAP/Architektur-Dateien),
  - **Benchmark-/Gate-Artefakte**,
  - **Testabdeckung**,
  - oder **externe Primarquellen**.
- Nicht belastbare Formulierungen wie pauschales "implemented" wurden praezisiert in:
  - "code/documented",
  - "benchmark-gated",
  - "teilweise belegt",
  - "offen".

### Terminologie (vereinheitlicht)

- **AQL** = ThemisDB Advanced Query Language (multi-paradigm, ArangoDB-inspiriert) [R3].
- **Multi-Model** = relational, document, graph, vector, geospatial, time-series [R1, R3].
- **Konsistenzmodell** = ACID/MVCC/SSI plus verteilte Commit-Koordinatoren (2PC/3PC/SAGA etc.) [R1, R4].
- **Hybrid Retrieval** = Kombination lexikalisch + vektoriell (+ optional graph/spatial) [R2, R3].

## Evaluation / Experimente

### Kurzuebersicht je Thema (reviewed)

| Thema | Primaere Venue-Familie | Evidenzstatus (Repo) | Hauptrisiko |
|---|---|---|---|
| RAG plus RAG-Evaluation | VLDB/ICDE; sekund. ACL/EMNLP Industry | Mittel-Hoch | Konsolidierte, publikationsfertige End-to-End-Messreihe fehlt |
| Vektor-IR und ANN | SIGMOD/VLDB/ICDE | Mittel-Hoch | Vergleichsstudie unter einheitlichem Last-/Kostenprotokoll fehlt |
| LoRA und QLoRA in DB-Kontext | MLSys; sekund. VLDB Industry | Mittel | SLO-Nachweis fuer produktive Multi-Adapter-Workloads ausbaubeduerftig |
| Effiziente LLM-Inferenz | MLSys/ATC; sekund. Systems-Workshops | Mittel | Hardware-/backend-spezifische Vergleichsbasis noch heterogen |
| Verteilte ACID Multi-Model-DB fuer AI | VLDB/SIGMOD/ICDE | Mittel | Einheitliche Kennzahlen ueber TX-Konsistenz + AI-Pipeline fehlen |

### 1) RAG plus RAG-Evaluation

- **Venue-Fit:** Stark fuer DB-Venues mit Industry-Anschluss.
- **Belegte Basis:** RAG-Module und Evaluationsoberflaechen sind im Modul dokumentiert; RAGAS-Bezug ist in der Research-Influence-Matrix verankert [R2, R5].
- **Metriken fuer Submission:** Faithfulness/Answer-Relevancy/Context-Metriken, p95/p99, Throughput unter Contention.
- **Restluecke:** Ein publikationsreifer, versionierter Benchmark-Report pro Workload ist noch als Artefakt zu buendeln.

### 2) Vektor-IR und ANN (HNSW, Hybrid Retrieval)

- **Venue-Fit:** Klassisches DB/IR-Thema mit klaren Repro-Anforderungen.
- **Belegte Basis:** AQL und Query-Ausfuehrung nennen Hybridpfade (Vector+Geo, Fulltext+Geo) und Optimizer-/Cost-Model-Flows [R3]; HNSW ist als wissenschaftliche Grundlage im Mapping hinterlegt [R5].
- **Metriken fuer Submission:** Recall@k, MRR/nDCG, QPS, Tail-Latenz, Speicher, Build-/Indexzeit.
- **Restluecke:** Einheitliche Vergleichsmatrix (HNSW/Hybrid unter identischen Lastprofilen) fehlt als abgeschlossener Experimentblock.

### 3) LoRA und QLoRA im datenbank-nativen Betrieb

- **Venue-Fit:** MLSys fuer Systems-Beitrag, DB-Industry fuer Betriebsintegration.
- **Belegte Basis:** LLM- und Training-Module enthalten Adapter-Lifecycle- und Trainingsoberflaechen; LoRA/QLoRA sind als Einflussquellen und Implementierungsziel dokumentiert [R5, R6, R7].
- **Metriken fuer Submission:** Adapter-Switch-Latenz, Qualitaetsdelta, Ressourcenverbrauch (z. B. VRAM), Recovery-Zeit.
- **Restluecke:** Vergleichbare Lastprofile mit reproduzierbarer SLO-Auswertung ueber mehrere Adapter-Domaenen.

### 4) Effiziente LLM-Inferenz (PagedAttention, Speculative Decoding, Continuous Batching)

- **Venue-Fit:** Systems/Serving-orientierte Venues.
- **Belegte Basis:** Entsprechende Konzepte sind als Forschungsbezug und Modulziel in der Influence-Matrix verankert; LLM-Runtime-Flaechen fuer Routing/Asynchronitaet/Streaming existieren [R5, R6, R9, R10].
- **Metriken fuer Submission:** TTFT, Token/s, p99 End-to-End, Akzeptanzrate bei spekulativer Dekodierung, Lastspitzen-Degradation.
- **Restluecke:** Vollstaendige Vergleichsstudie gegen klar definierte Baseline-Serving-Topologien.

### 5) Verteilte ACID Multi-Model-DB fuer AI

- **Venue-Fit:** Kernbeitrag fuer VLDB/SIGMOD/ICDE.
- **Belegte Basis:** Multi-Model- und ACID-Positionierung ist in Root-Docs verankert [R1]; Koordinatoren und Protokolle (2PC/3PC/SAGA etc.) sind architekturseitig ausdetailliert [R4]; Wave-5/6/7-Artefakte dokumentieren Hardening/Gates [R8, R11, R12].
- **Metriken fuer Submission:** Commit-Latenz, Abort-Rate, Cross-Shard-Skalierung, Replikations-/Failover-Zeit, Konsistenzverletzungen unter Fehlern.
- **Restluecke:** Zusammenhaengender Experimentreport, der TX-Konsistenzkosten direkt gegen AI/RAG-Serving-Pfade ausweist.

## Limitations / Known Issues

1. Mehrere Evidenzquellen sind governance-/modulorientierte Dokumente statt einzelner paper-figurenfertiger Resultat-Tabellen.
2. Wave-7-Manifest enthaelt bekannte Limitations (u. a. Stub-bezogene Vektor/Graph-Hinweise), die bei paper claims explizit offengelegt werden muessen [R11].
3. Das Venue-Mapping bewertet strategischen Fit und Evidenzreife, ersetzt aber keine vollstaendige Submission-Checkliste (Artefaktpaket, Repro-Appendix, statistische Signifikanz).

## Fazit und priorisierte Reihenfolge (naechste 6 Monate)

Bei Ziel **hohe Annahmewahrscheinlichkeit kurzfristig**:

1. RAG plus Evaluation
2. ANN/Hybrid Retrieval
3. Verteilte ACID Multi-Model-DB fuer AI
4. LLM-Inferenz-Optimierung
5. LoRA/QLoRA Betriebsmodell

Bei Ziel **maximale Neuheit im AI-Serving**: Position 3 und 4 tauschen.

## References / Quellen

### Repository-Artefakte (ThemisDB)

- [R1] ThemisDB README (Multi-Model, ACID, Architekturueberblick): https://github.com/makr-code/ThemisDB/blob/develop/README.md
- [R2] RAG-Modul README: https://github.com/makr-code/ThemisDB/blob/develop/src/rag/README.md
- [R3] Query-Modul README (AQL, Hybrid-Execution): https://github.com/makr-code/ThemisDB/blob/develop/src/query/README.md
- [R4] Transaction Coordinators Architektur: https://github.com/makr-code/ThemisDB/blob/develop/docs/architecture/transaction_coordinators.md
- [R5] Research Influence Matrix (by_module): https://github.com/makr-code/ThemisDB/blob/develop/research/implementation_influence/by_module.md
- [R6] LLM-Modul README: https://github.com/makr-code/ThemisDB/blob/develop/src/llm/README.md
- [R7] Training-Modul README: https://github.com/makr-code/ThemisDB/blob/develop/src/training/README.md
- [R8] Wave-5 Coverage: https://github.com/makr-code/ThemisDB/blob/develop/tests/integration/WAVE5_TEST_COVERAGE.md
- [R9] LLM Processing Patterns Draft: https://github.com/makr-code/ThemisDB/blob/develop/research/LLM_PROCESSING_OPTIMIZATION_PATTERNS.md
- [R10] DB-native LLM Serving Optimization Draft: https://github.com/makr-code/ThemisDB/blob/develop/research/DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md
- [R11] Wave-7 Gate Manifest: https://github.com/makr-code/ThemisDB/blob/develop/benchmarks/wave7/release_gate_manifest_w7.json
- [R12] Wave-6 Coverage: https://github.com/makr-code/ThemisDB/blob/develop/tests/integration/WAVE6_TEST_COVERAGE.md

### Externe wissenschaftliche Quellen (Begriffs- und Methodenbasis)

- [E1] Malkov, Yashunin (2020). Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs. IEEE TPAMI. DOI: https://doi.org/10.1109/TPAMI.2018.2889473
- [E2] Hu et al. (2022). LoRA: Low-Rank Adaptation of Large Language Models. arXiv: https://arxiv.org/abs/2106.09685
- [E3] Dettmers et al. (2023). QLoRA: Efficient Finetuning of Quantized LLMs. arXiv: https://arxiv.org/abs/2305.14314
- [E4] Es et al. (2023). RAGAS: Automated Evaluation of Retrieval Augmented Generation. arXiv: https://arxiv.org/abs/2309.15217
- [E5] Kwon et al. (2023). Efficient Memory Management for Large Language Model Serving with PagedAttention (vLLM). arXiv: https://arxiv.org/abs/2309.06180
- [E6] Chen et al. (2023). Accelerating Large Language Model Decoding with Speculative Sampling. arXiv: https://arxiv.org/abs/2302.01318
