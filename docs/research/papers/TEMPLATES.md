# Paper Documentation Templates

Use the structure below for every new scientific paper entry.  
A ready-to-copy starter file is available at [_template_paper.md](_template_paper.md).

---

## Required Metadata Fields

| Field | Description | Example |
|-------|-------------|---------|
| `Author(en)` | Comma-separated author names | `Malkov, Y. A., Yashunin, D. A.` |
| `Konferenz/Journal` | Publication venue | `IEEE TPAMI` |
| `Jahr` | Publication year | `2018` |
| `Link` | DOI, ArXiv URL, or PDF link | `https://arxiv.org/abs/1603.09320` |
| `Zitierweise` | BibTeX-style short citation key | `malkov2018efficient` |
| `Tags` | Comma-separated topic tags | `vector-search, graph-index, approximate-nn` |
| `ThemisDB-Versionen` | First version that used this paper | `v1.4.1+` |
| `Status` | Implementation progress | `Fully Implemented` |

---

## Full Template Structure

```markdown
# [Paper Title]

**Metadaten:**
- Author(en): 
- Konferenz/Journal: 
- Jahr: 
- Link: [PDF/ArXiv]()
- Zitierweise: 
- Tags: [vector-search, performance, security, ...]
- ThemisDB-Versionen: [v1.4.1+, v2.0+, ...]
- Status: [ ] Not Started | [ ] Partially Implemented | [ ] Fully Implemented

## 📋 Executive Summary
(2–3 sentences: what does this paper cover and why is it relevant to ThemisDB?)

## 🎯 Key Findings
- Finding 1
- Finding 2
- Finding 3

## 🔗 Direct Influence on ThemisDB

### Affected Modules
- [ ] Module 1 → `src/module1/`
- [ ] Module 2 → `src/module2/`

### What Was Adopted?
(Code concepts, data structures, algorithms)

### How Was It Adapted?
(ThemisDB-specific adjustments, trade-offs, limitations)

### Performance Impact
| Metric | Paper Claim | ThemisDB Result | Delta | Reason |
|--------|-------------|-----------------|-------|--------|

## ⚠️ Limitations & Open Questions
- Limitation 1:
  - ThemisDB solution: 

## 🔬 Validation
- [ ] Code reviewed against paper
- [ ] Unit tests written
- [ ] Benchmark executed
- [ ] Documentation updated
- [ ] Module README linked

## 📚 Related Work
- [Related Paper 1](related_paper.md)
- [Best Practice: XYZ](../best_practices/xyz.md)

---
**Last Updated:** YYYY-MM-DD  
**Next Review:** YYYY-MM-DD
```

---

## Status Values

| Value | Meaning |
|-------|---------|
| `Not Started` | Paper identified but not yet implemented |
| `Partially Implemented` | Some aspects implemented, work in progress |
| `Fully Implemented` | All relevant aspects implemented and validated |

## Tag Vocabulary

Common tags to use (add new ones as needed):

- `vector-search` · `graph-index` · `approximate-nn`
- `lsm-tree` · `storage-engine` · `compaction`
- `mvcc` · `transaction` · `isolation`
- `raft` · `consensus` · `replication`
- `security` · `encryption` · `authentication`
- `performance` · `cache` · `memory`
- `llm` · `rag` · `embeddings`
- `geospatial` · `gpu` · `cuda`
