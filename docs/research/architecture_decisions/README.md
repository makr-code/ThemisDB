# Architecture Decisions — Index

This directory documents significant architecture decisions using an ADR-like format (Architecture Decision Records).

## Purpose

Whenever a non-trivial design choice is made — especially when multiple options were evaluated — an entry here captures:

- The context and constraints
- The options considered
- The decision made and its rationale
- The trade-offs accepted

## Decision Log

See [decision_log.md](decision_log.md) for a chronological list of all decisions.

## Index by Status

### ✅ Accepted
| ID | Decision | Date | Modules |
|----|----------|------|---------|
| *(none yet)* | | | |

### 🔄 Proposed / Under Review
| ID | Decision | Date | Modules |
|----|----------|------|---------|
| *(none yet)* | | | |

### ⛔ Superseded
| ID | Decision | Superseded By | Date |
|----|----------|---------------|------|
| *(none yet)* | | | |

## Adding a New Decision

1. Copy [_template_decision.md](_template_decision.md) to `adr_<NNN>_<short_title>.md`  
   Example: `adr_001_vector_index_choice.md`
2. Assign the next sequential ID from [decision_log.md](decision_log.md)
3. Fill in all required sections
4. Link it in the relevant module README and in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
adr_<NNN>_<short_title>.md
```

Examples:
- `adr_001_vector_index_hnsw_vs_faiss.md`
- `adr_002_storage_engine_rocksdb.md`
- `adr_003_consensus_raft.md`

## See Also

- [decision_log.md](decision_log.md) — chronological log
- [_template_decision.md](_template_decision.md) — copy-paste template
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — contributor workflow
