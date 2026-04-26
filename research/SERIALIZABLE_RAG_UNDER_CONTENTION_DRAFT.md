# Serializable RAG Under Contention: Isolation-Aware Retrieval in Hybrid DBMS

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: VLDB, SIGMOD, ICDE

---

## I. Abstract

Dieses Paper untersucht, wie sich Isolationsebenen auf RAG-Korrektheit und Laufzeit unter konkurrierenden Schreiblasten auswirken. Der Fokus liegt auf einem datenbanknativen Ansatz, bei dem Retrieval und Bewertung innerhalb transaktionaler Grenzen ausgeführt werden. Ziel ist eine messbare Zuordnung zwischen Isolation Policy, Anomalierisiko und Latenz-/Throughput-Kosten.

## II. Problem Statement

RAG-Systeme in produktiven Multi-User-Umgebungen leiden unter Inkonsistenzen, wenn Retrieval auf instabile Datenzustände zugreift. Ohne transaktionale Kopplung entstehen widersprüchliche Kontexte, die Faithfulness und Reproduzierbarkeit verschlechtern. Benötigt wird ein Modell, das Isolation explizit als RAG-Steuerparameter behandelt.

## III. Research Questions

1. Wie unterscheiden sich RC, RR/Snapshot und Serializable (SSI) bei RAG-Faithfulness unter Konfliktlast?
2. Wie hoch sind zusätzliche p95/p99-Latenzen und Throughput-Verluste je Isolationsebene?
3. Welche Abort- und Retry-Muster treten pro Workloadklasse auf?
4. Welche policy-basierte Zuordnung (z. B. Compliance-Queries -> Serializable) maximiert Qualität pro Kostenbudget?

## IV. Repository Evidence Registry

- E1: `include/transaction/transaction_manager.h`
- E2: `tests/test_transaction_ssi.cpp`
- E3: `tests/test_ssi_predicate_locking.cpp`
- E4: `benchmarks/bench_transaction_throughput.cpp`
- E5: `benchmarks/benchmark_target_mapping.json`
- E6: `benchmarks/docs/BENCHMARKS_EXECUTIVE_SUMMARY.md`
- E7: `research/ACID_CONSTRAINED_RAG_DRAFT.md`

## V. Measurement Plan

- Workloads: fact lookup, multi-hop retrieval, contended write+read.
- Modi: READ_COMMITTED, REPEATABLE_READ/Snapshot, SERIALIZABLE (SSI).
- Metriken:
  - Retrieval correctness drift
  - Faithfulness/Relevance delta
  - Abort rate, retry depth
  - p50/p95/p99 latency, throughput

## VI. Claim Boundaries

**Unterstützte Claims (nach aktueller Evidence):**
- SSI/OCC-Pfade sind implementiert und testbar.
- Transaktions-Throughput-Baselines sind vorhanden.

**Deferred Claims (benötigen neue Messläufe):**
- End-to-end Qualitätsvergleich RAG unter hoher Konfliktlast.
- Generalisierung auf alle Domänenkorpora.

## VII. Next Milestones

- M1: Isolationsvergleich mit identischem Query-Set
- M2: Anomalie-Replay und Qualitätsdelta
- M3: Policy-Routing-Tabelle für Produktionsprofile
- M4: Paper-Upgrade auf v0.2 (inkl. statistischer Auswertung)
