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

# Scheduling Multithreaded Computations by Work Stealing

**Metadaten:**
- Author(en): Robert D. Blumofe, Charles E. Leiserson
- Konferenz/Journal: Journal of the ACM, Vol. 46, No. 5, pp. 720–748
- Jahr: 1999
- Link: [DOI: 10.1145/324133.324234](https://doi.org/10.1145/324133.324234)
- Zitierweise: `blumofe1999workstealing`
- Tags: `work-stealing`, `parallel-scheduling`, `concurrency`, `performance`
- ThemisDB-Versionen: v3.0.0
- Status: [x] Not Started

## 📋 Executive Summary

This paper analyzes work stealing for fully strict, dependency-structured
multithreaded computations. It provides expected-time bounds in terms of serial
work and critical-path length, motivating workload-specific evaluation of
stealing rather than an assumption that it will accelerate arbitrary database
tasks.

## 🎯 Key Findings

- The analyzed work-stealing scheduler has expected execution time bounded by
  the serial work divided across processors plus a term proportional to the
  critical path.
- The result depends on the fully strict computation model and does not
  establish the same bound for arbitrary independent or database-managed tasks.
- Work stealing should be evaluated against the current work-sharing baseline
  with both balanced and skewed workloads and tail-latency measurements.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] Execution scheduler research → `src/execution/`

### What Was Adopted?

No algorithm or implementation was adopted. The paper motivates the proposed
v3.0.0 work-stealing experiment and its explicit applicability limits.

### How Was It Adapted?

The roadmap treats work stealing as conditional on a measured bottleneck and
requires comparison with the existing central dispatch queue. The scheduler
must preserve bounded admission, clear shutdown behavior, and measured tail
latency; the paper's theoretical bounds are not claimed for ThemisDB.

### Performance Impact

| Metric | Paper Claim | ThemisDB Result | Delta | Reason |
|---|---|---|---|---|
| Execution time | Expected bound for fully strict computations | Not measured | N/A | The current pool is a central-queue implementation; production steal-path evidence does not exist. |

## ⚠️ Limitations & Open Questions

- The proof assumptions do not establish fairness, deadline ordering, or
  performance for general database task queues.
- Whether a production steal path is useful for ThemisDB depends on Wave 1
  workload evidence and remains open.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written
- [ ] Benchmark executed
- [x] Documentation updated to state applicability limits
- [x] Module README linked
- [x] implementation_influence index updated

## 📚 Related Work

- [Concurrent-query scheduling preprint](wu_concurrent_query_scheduling_2025.md)
- [Execution module research note](../../src/execution/RESEARCH.md)

---
**Last Updated:** 2026-11-06
**Next Review:** Before v3.0.0 work begins
