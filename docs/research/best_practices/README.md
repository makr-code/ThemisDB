# Best Practices Index

This directory documents engineering best practices — from open-source projects, industry standards, and internal ThemisDB learnings — that influence the codebase.

## Purpose

When a design or implementation pattern is adopted from an established external source (e.g., another open-source database, an industry blog post, a reference implementation), it should be documented here.

## Index

| Best Practice | Module(s) | Version | Status |
|---------------|-----------|---------|--------|
| *(No entries yet — see [RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) to add the first one)* | - | - | - |

## Adding a New Best Practice

1. Copy [_template_best_practice.md](_template_best_practice.md) to a new file  
   Example: `zero_copy_io.md`, `lock_free_queues.md`
2. Fill in all required fields (see [TEMPLATES.md](TEMPLATES.md))
3. Link it in the relevant module README under *Wissenschaftliche Grundlagen & Einflüsse*
4. Register it in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
<short_descriptive_name>.md
```

Examples:
- `zero_copy_io.md`
- `lock_free_queue_design.md`
- `rocksdb_compaction_tuning.md`

## See Also

- [TEMPLATES.md](TEMPLATES.md) — required fields and formatting rules
- [_template_best_practice.md](_template_best_practice.md) — copy-paste starter
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — end-to-end contributor workflow
- [../implementation_influence/README.md](../implementation_influence/README.md) — master cross-reference index
