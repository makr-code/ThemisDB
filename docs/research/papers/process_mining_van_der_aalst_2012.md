# Process Mining: Data Science in Action

**Metadaten:**
- Author(en): Wil M. P. van der Aalst
- Konferenz/Journal: Book — Springer-Verlag Berlin Heidelberg (2nd edition)
- Jahr: 2016 (1st ed. 2011; 2nd ed. 2016)
- Link: [Springer](https://link.springer.com/book/10.1007/978-3-662-49851-4) · [DOI: 10.1007/978-3-662-49851-4]
- Zitierweise: `vanderaalst2016processmining`
- Tags: `process-mining`, `petri-nets`, `bpmn`, `event-log`, `conformance-checking`, `process-discovery`, `alpha-algorithm`
- ThemisDB-Versionen: v1.9.0+; core reference for `src/process/`
- Status: [x] Partially Implemented (process graph storage + BPMN import) · [ ] Process discovery algorithms planned

## 📋 Executive Summary

This book is the definitive reference on process mining — the discipline that bridges data science and process management. It covers the full lifecycle: process *discovery* (learning BPMN/Petri nets from event logs), *conformance checking* (comparing observed behavior to reference models), and *enhancement* (improving models with performance data). ThemisDB's `src/process/` module uses this book as the primary algorithmic reference for process graph storage, BPMN 2.0 compliance, conformance checking, and planned process mining features.

Directly referenced in `src/process/FUTURE_ENHANCEMENTS.md` (P6: Object-Centric Process Mining — van der Aalst 2022 extension).

## 🎯 Key Findings

- **α-Algorithm**: Extracts Petri net from event log via directly-follows relation; simple but noise-sensitive; superseded by Inductive Miner (Leemans 2013).
- **Inductive Miner (IM)**: Recursively partitions event log into sub-logs; produces block-structured Petri net guaranteed to be sound; noise-tolerant via infrequent variant filtering.
- **Conformance checking via token replay**: Simulates event log on Petri net; missed transitions (token deficits) and extra tokens (remainders) quantify fitness.
- **Alignment-based conformance**: Optimal edit distance between observed trace and model-compliant trace; more accurate than token replay but computationally expensive.
- **Event log structure**: An event log is a set of cases; each case is an ordered sequence of events with at least `case_id`, `activity`, `timestamp`; additional attributes (resource, cost, compliance_flag) are optional.
- **ProM framework**: Open-source platform with 350+ process mining algorithms; reference implementation for ThemisDB process algorithms.
- **Object-centric extension (2022)**: van der Aalst's later work on Object-Centric Process Mining (OCPM) extends the single-case-notion model to multiple interacting object types — directly relevant to ThemisDB's multi-object Verwaltungsvorgänge.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Process module → `src/process/` (process graph storage, BPMN 2.0 import, event log structure)
- [ ] Process discovery → `src/process/` (planned: Inductive Miner implementation)
- [ ] Conformance checking → `src/process/` (planned: token replay + alignment)
- [x] Analytics module → `src/analytics/` (directly-follows graph computation)
- [x] CDC module → `src/cdc/` (event log as CDC event stream)

### What Was Adopted?

1. **Event log data model**: ThemisDB's process events use the XES-compatible structure: `{case_id, activity, timestamp, resource, attributes}` — directly from van der Aalst's event log definition.
2. **Process graph node taxonomy**: `ActionNode`, `DecisionNode`, `GatewayNode`, `EventNode` map to the Petri net `transition`, `place`, `AND-split`, `XOR-split` concepts.
3. **BPMN 2.0 alignment**: BPMN import/export uses the process mining community's BPMN-to-Petri-net mapping (defined in this book) to ensure algorithmic compatibility.
4. **Directly-follows graph (DFG)**: Analytics module computes DFG as first-pass process discovery; used for process visualization in the admin UI.

### How Was It Adapted?

| Process Mining Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Flat event log (XES format) | XDOMEA + FIM event format | German administrative process events have XÖV structure |
| Single-case-notion (one object per case) | Multi-object cases (Antragsteller, Grundstück, Bescheid) | German Verwaltungsvorgänge involve multiple related objects |
| ProM (Java) algorithms | C++ implementation via `src/process/` | ThemisDB is C++; ProM not embeddable |
| Petri net process models | BPMN 2.0 + EPK + VCC-VPB | German public administration uses BPMN and EPK notation |

### Performance Impact

| Metric | Book's Typical Results | ThemisDB Target | Status |
|--------|----------------------|-----------------|--------|
| Inductive Miner on 1,000-case log | ~0.5 s | <2 s | ⏳ Planned |
| Conformance fitness (token replay) | O(n×|model|) | <5 s for 10,000 events | ⏳ Planned |
| DFG computation | O(n) | <100 ms for 100,000 events | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- Classic process mining assumes a single case notion (one object per trace); German Verwaltungsvorgänge have multiple related objects.
  - ThemisDB solution: Object-Centric Process Mining (van der Aalst 2022) — planned as P6, Target Q3 2026.
- Inductive Miner requires all activities in the log to be in the model; rare activities inflate model complexity.
  - ThemisDB solution: IMf (Inductive Miner infrequent) variant with configurable noise threshold.
- Conformance checking is computationally expensive for large logs (>100,000 events).
  - ThemisDB solution: Approximate conformance via DFG comparison for real-time monitoring; exact alignment for offline audit.

## 🔬 Validation

- [x] Event log data model reviewed against book
- [ ] Inductive Miner unit tests written
- [ ] Conformance checking benchmark executed
- [x] BPMN import documentation updated
- [ ] Module README linked with paper reference
- [ ] implementation_influence index updated

## 📚 Related Work

- [ProcessGPT — Busch et al. (2023)](processgpt_busch_2023.md)
- [ProcessTransformer — Bukhsh et al. (2021)](processtransformer_bukhsh_2021.md)
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [OMG BPMN 2.0.2 Standard](https://www.omg.org/spec/BPMN/2.0.2/)
- [ProM Framework](https://promtools.org)
- [`src/process/FUTURE_ENHANCEMENTS.md`](../../../src/process/FUTURE_ENHANCEMENTS.md)
- [`docs/de/process/STATE_OF_THE_ART.md`](../../de/process/STATE_OF_THE_ART.md)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
