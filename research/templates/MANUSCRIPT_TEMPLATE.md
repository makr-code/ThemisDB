# ThemisDB Manuscript Template

**Status**: Template  
**Version**: 1.0  
**Last Updated**: 2026-08-10  
**Target Venue**: arXiv / conference draft

---

## Metadata

- **Scientific Delta**:
- **Canonical Evidence Sources**:
- **Required Experiments**:
- **Open Risks / Claim Boundaries**:
- **Overlap / Successor / Predecessor**:

## Abstract

<120-250 words: problem, method, main contribution, evidence status, limits>

## I. Introduction

- problem context
- gap in prior work
- why the gap matters in production systems
- scope boundaries

### Contributions

1. <contribution 1>
2. <contribution 2>
3. <contribution 3>

## II. Related Work

- prior work cluster A
- prior work cluster B
- novelty delta vs closest baselines

## III. System Model / Repository Scope

- affected ThemisDB subsystems
- execution assumptions
- failure / threat / workload model

## IV. Method / Design

- main algorithm / protocol / mechanism
- decision functions and invariants
- scaling behavior
- edge-case handling

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `<path>` | `<section>` | `<what it proves>` | ready/pending |
| E2 | `<path>` | `<section>` | `<what it proves>` | ready/pending |
| E3 | `<path>` | `<section>` | `<what it proves>` | ready/pending |

## VI. Experimental Methodology

### A. Setup
- hardware
- software versions
- dataset / workload profile
- reproducibility controls

### B. Workloads
- W1:
- W2:
- W3:

### C. Metrics
- p50/p95/p99 latency
- throughput
- quality / correctness
- failure / recovery metrics

## VII. Results

### A. Primary Results
- measured outcomes
- interpretation tied to research questions

### B. Ablations / Sensitivity
- parameter sweep
- condition comparison

### C. Negative Results
- what failed
- why it likely failed

## VIII. Discussion

- operational implications
- threats to validity
- trade-offs

### Supported claims
- <claim + evidence IDs>

### Deferred claims
- <claim needing more data>

## IX. Reproducibility & Artifact

- repo commit/tag
- commands to rerun
- expected runtime
- known environment pitfalls

## X. Limitations, Risk, Ethics

- misuse risks
- safety / compliance boundaries
- unsupported scenarios

## XI. Conclusion

- concise answer to the research questions
- concrete next steps
