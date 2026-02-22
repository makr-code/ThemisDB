# Scientific Papers Index

This directory contains documentation of all scientific papers that have influenced ThemisDB's implementation.

## Purpose

Each paper that serves as a foundation for a ThemisDB algorithm, data structure, or design decision should be documented here using the [template](_template_paper.md).

## Index

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| *(No entries yet — see [RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) to add the first one)* | - | - | - |

## Adding a New Paper

1. Copy [_template_paper.md](_template_paper.md) to a new file named `<short_key>_<year>.md`  
   Example: `hnsw_efficient_ann_2018.md`
2. Fill in all required fields (see [TEMPLATES.md](TEMPLATES.md))
3. Link the paper in the relevant module README under *Wissenschaftliche Grundlagen & Einflüsse*
4. Register it in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
<topic>_<year>.md
```

Examples:
- `hnsw_efficient_ann_2018.md`
- `lsm_tree_rocksdb_2016.md`
- `raft_consensus_2014.md`

## See Also

- [TEMPLATES.md](TEMPLATES.md) — required fields and formatting rules
- [_template_paper.md](_template_paper.md) — copy-paste starter template
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — end-to-end contributor workflow
- [../implementation_influence/README.md](../implementation_influence/README.md) — master cross-reference index
