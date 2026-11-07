---
Author: "ThemisDB Team"
Created: "2026-11-06"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Improving DBMS Scheduling Decisions with Fine-grained Performance Prediction on Concurrent Queries

**Metadaten:**
- Author(en): Ziniu Wu, Markos Markakis, Chunwei Liu, Peter Baile Chen, Balakrishnan Narayanaswamy, Tim Kraska, Samuel Madden
- Konferenz/Journal: arXiv preprint
- Jahr: 2025
- Link: [arXiv:2501.16256](https://arxiv.org/abs/2501.16256)
- Zitierweise: `wu2025concurrentqueryscheduling`
- Tags: `database-scheduling`, `query-latency`, `concurrency`, `tail-latency`
- ThemisDB-Versionen: v2.5.0, v3.0.0
- Status: [x] Not Started

## 📋 Executive Summary

This work studies using fine-grained execution-time predictions for scheduling
concurrent database queries under changing system states and query mixes. The
reported results are specific to its PostgreSQL and Redshift evaluations; for
ThemisDB the relevant takeaway is to measure representative mixes and tail
latency before considering adaptive scheduling.

## 🎯 Key Findings

- Query runtime and resource behavior vary with concurrent workload and system
  state, so scheduling based only on static labels can miss relevant context.
- The paper evaluates prediction-informed scheduling end to end on database
  workloads and reports improvements specific to those experiments.
- Prediction overhead, model maintenance, and workload drift are practical
  constraints; adding prediction to a critical path requires independent
  evaluation.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] Execution scheduler research → `src/execution/`

### What Was Adopted?

No predictor or scheduling policy was adopted. The source supports prioritizing
production workload and tail-latency evidence before any adaptive policy is
proposed.

### How Was It Adapted?

The execution roadmap separates v2.5.0 measurement from v3.0.0 policy
experimentation. It does not import the paper's result percentages as ThemisDB
targets and does not propose a prediction model for the current queue.

### Performance Impact

| Metric | Paper Claim | ThemisDB Result | Delta | Reason |
|---|---|---|---|---|
| End-to-end query latency | Improvements reported on evaluated PostgreSQL and Redshift workloads | Not measured | N/A | The execution module is not integrated into a live request path and has no production-runtime baseline. |

## ⚠️ Limitations & Open Questions

- The paper's workload, DBMS integration, and prediction costs differ from this
  module's bounded in-process scheduler.
- Whether adaptive scheduling is useful for ThemisDB is open until end-to-end
  workloads and the current baseline exist.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written
- [ ] Benchmark executed
- [x] Documentation updated to distinguish source results from ThemisDB evidence
- [x] Module README linked
- [x] implementation_influence index updated

## 📚 Related Work

- [Work-stealing scheduling](blumofe_leiserson_work_stealing_1999.md)
- [Execution module research note](../../src/execution/RESEARCH.md)

---
**Last Updated:** 2026-11-06
**Next Review:** Before v2.5.0 workload evaluation
