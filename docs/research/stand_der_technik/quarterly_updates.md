# Quarterly State-of-the-Art Update Process

## Overview

Every quarter the ThemisDB team reviews relevant literature and industry developments to ensure the implementation stays current.

## Schedule

| Quarter | Review Period | Responsible | Deadline |
|---------|---------------|-------------|----------|
| 2026 Q1 | Jan–Mar 2026 | ThemisDB Research Team | 2026-03-31 ✅ |
| 2026 Q2 | Apr–Jun 2026 | TBD | 2026-06-30 |
| 2026 Q3 | Jul–Sep 2026 | TBD | 2026-09-30 |
| 2026 Q4 | Oct–Dec 2026 | TBD | 2026-12-31 |

## Process for Each Quarter

1. **Create report file**: Copy template below to `<year>_q<n>_landscape.md`
2. **Review key areas**: For each area in scope, survey recent papers (ArXiv, conference proceedings, blog posts)
3. **Identify gaps**: Compare findings against ThemisDB's current implementation
4. **Create action items**:
   - New paper entry in [papers/](../papers/) if a specific algorithm should be adopted
   - New architecture decision in [architecture_decisions/](../architecture_decisions/) if a design change is warranted
   - New GitHub issue with label `research-backlog` if implementation work is needed
5. **Update this schedule** with the "Responsible" person and mark the report as complete

## Quarterly Report Template

```markdown
# State of the Art — [Year] Q[N]

**Period:** [Month] – [Month] [Year]  
**Author:** @github-handle  
**Status:** 🔄 In Progress | ✅ Complete

## Areas Reviewed

- [ ] Vector Search & ANN Indexes
- [ ] Storage Engines & LSM-Trees
- [ ] Distributed Systems & Consensus
- [ ] LLM Integration & RAG
- [ ] GPU-Accelerated Operations
- [ ] Security & Encryption
- [ ] Query Optimization

## Key Findings

### Vector Search & ANN Indexes
- ...

### Storage Engines & LSM-Trees
- ...

(repeat for each area)

## Gap Analysis

| Finding | ThemisDB Current | Action Required | Priority |
|---------|-----------------|-----------------|----------|
|         |                 |                 |          |

## Action Items Created

- [ ] Paper: [Title](../papers/title.md) — created
- [ ] ADR: [Decision](../architecture_decisions/decision.md) — created
- [ ] Issue: #NNN — opened

---
**Completed:** YYYY-MM-DD
```

## Review Areas

The following areas should be covered every quarter:

| Area | Primary Sources | ThemisDB Modules |
|------|----------------|------------------|
| Vector Search & ANN | ArXiv cs.DB, VLDB, SIGMOD | `src/index/`, `src/vector/` |
| Storage Engines | RocksDB blog, OSDI, FAST | `src/storage/` |
| Distributed Systems | OSDI, SOSP, Raft paper updates | `src/replication/`, `src/raft/` |
| LLM & RAG | ArXiv cs.CL, HuggingFace blog | `src/llm/`, `src/rag/` |
| GPU Computing | CUDA docs, NeurIPS, SC | `src/gpu/` |
| Security | CVE, NIST, OWASP | `src/auth/`, `src/security/` |
| Query Optimization | VLDB, SIGMOD | `src/query/` |
