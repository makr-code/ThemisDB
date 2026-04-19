# Cost-Aware Hybrid Retrieval Planning in AQL

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: SIGMOD, VLDB, EDBT

---

## I. Abstract

Dieses Paper adressiert die Planungsfrage für hybride Retrieval-Pipelines, die lexikalische, vektorbasierte und graphbasierte Operatoren in AQL kombinieren. Wir schlagen ein kostenbasiertes Planungsmodell vor, das Latenz, Recall und Ressourcenverbrauch gemeinsam optimiert. Ziel ist reproduzierbare Planwahl statt statischer Heuristik.

## II. Problem Statement

Hybride Suche wird oft als feste Pipeline implementiert, obwohl Datenverteilung und Query-Typ stark variieren. Dadurch entstehen unnötige Latenz oder Recall-Verluste. Ein Optimierer benötigt belastbare Kostenfunktionen für Multi-Operator-Pläne.

## III. Research Questions

1. Welche Kostenkomponenten erklären Planqualität am besten (CPU, IO, ANN, graph expansion)?
2. Wann ist hybrid (BM25+vector+graph) besser als partielle Pfade?
3. Wie robust bleiben Pläne bei Distribution Shift (neue Domänen, andere Top-k)?
4. Wie gut korrelieren geschätzte Kosten mit realer Laufzeit und Recall?

## IV. Repository Evidence Registry

- E1: `aql/README.md`
- E2: `benchmarks/bench_rag_hybrid_retriever.cpp`
- E3: `compendium/docs/chapter_34_query_optimization.md`
- E4: `benchmarks/ann/README.md`
- E5: `research/QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md`
- E6: `research/ACID_CONSTRAINED_RAG_DRAFT.md`

## V. Measurement Plan

- Queryklassen: fact lookup, semantic expansion, graph-constrained retrieval.
- Planvarianten: lexical-only, vector-only, graph-only, hybrid (kombiniert).
- Metriken:
  - Recall@k, NDCG@k
  - p50/p95/p99 latency
  - Ressourcenverbrauch pro Plan
  - Planstabilität unter Workload-Shift

## VI. Claim Boundaries

**Unterstützte Claims:**
- AQL und Query-Optimierungsgrundlagen sind dokumentiert.
- Hybrid-Retrieval-Benchmark-Harness ist vorhanden.

**Deferred Claims:**
- Vollständig kalibriertes Kostenmodell für alle Datendomänen.
- Universelle Dominanz eines einzelnen Planmusters.

## VII. Next Milestones

- M1: Kostenfeature-Definition und Logging
- M2: Planvergleich auf 3 Workloadfamilien
- M3: Fit von Kostenmodell vs Runtime
- M4: v0.2 mit reproduzierbarer Evaluationssuite
