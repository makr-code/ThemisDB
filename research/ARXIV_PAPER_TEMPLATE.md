# <Paper Title>

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: YYYY-MM-DD  
**Target Venue**: arXiv (cs.DB / cs.LG / cs.DC)

---

## Abstract

<120-250 words: problem, method, main contribution, key evidence, limitations>

## I. Introduction

- Problem context and practical relevance
- Precise gap in prior work
- Why this gap matters in production systems
- High-level summary of your approach

### Contributions

1. <Contribution 1>
2. <Contribution 2>
3. <Contribution 3>

## II. Related Work

- Prior work area A
- Prior work area B
- Prior work area C
- Explicit novelty delta vs closest 3-5 papers

## III. System Model / Architecture

- Components and interfaces
- Execution model and assumptions
- Failure model / threat model
- Optional: architecture figure

## IV. Method / Design

- Core algorithm or protocol
- Decision function(s)
- Complexity or scaling behavior
- Edge-case handling

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | <path> | <lines/section> | <claim anchor> | <ready/pending> |
| E2 | <path> | <lines/section> | <claim anchor> | <ready/pending> |
| E3 | <path> | <lines/section> | <claim anchor> | <ready/pending> |

Rules:
- Every major claim in Sections III-VII must map to >=1 evidence ID.
- Prefer tests/benchmarks over comments as claim support.

## VI. Experimental Methodology

### A. Setup
- Hardware
- Software versions
- Dataset/corpus profile
- Reproducibility controls (seed, warm-up, run count)

### B. Workloads
- W1: <name and objective>
- W2: <name and objective>
- W3: <name and objective>

### C. Metrics
- Latency (p50/p95/p99)
- Throughput
- Quality metrics (e.g., recall@k, faithfulness)
- Reliability/failure metrics

## VII. Results

### A. Primary Results
- Table/plot summary
- Interpretation tied to RQ1-RQn

### B. Ablations / Sensitivity
- Parameter sweep
- Effect sizes, not only raw numbers

### C. Negative Results
- What did not work
- Why it likely failed

## VIII. Discussion

- Practical implications
- Threats to validity
- Operational constraints and trade-offs

### Claim Boundaries

**Supported claims:**
- <claim + evidence IDs>

**Deferred claims:**
- <claim that needs additional data>

## IX. Reproducibility & Artifact

- Repo commit/tag
- Commands to rerun benchmarks/tests
- Expected runtime
- Known environment pitfalls

## X. Limitations, Risk, Ethics

- Misuse risks
- Safety/compliance considerations
- Boundary conditions where method should not be applied

## XI. Conclusion

- Concise answer to research questions
- Concrete next steps

## References

- Use consistent citation style (numeric preferred for arXiv preprint drafts)
- Include DOI/URL where possible

---

## Appendix A. arXiv Submission Readiness Checklist

- [ ] Title is specific and technically scoped
- [ ] Abstract states measurable contribution
- [ ] All headline claims are evidence-backed
- [ ] Related work includes closest baselines and novelty delta
- [ ] Method and assumptions are explicitly stated
- [ ] Experimental setup is reproducible
- [ ] Limitations and threat model are transparent
- [ ] Figures/tables are referenced in text
- [ ] References are complete and consistent
- [ ] Artifact path and commit hash documented

## Appendix B. Quick Start for ThemisDB Drafts

1. Copy this template to a new file under `research/`.
2. Fill Sections I-V first, then define experiments.
3. Run/collect measurements and complete Sections VI-VII.
4. Finalize claim boundaries before pushing.
