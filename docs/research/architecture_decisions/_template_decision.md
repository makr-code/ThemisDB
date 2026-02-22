# ADR-NNN: [Short Decision Title]

**Status:** Proposed | Accepted | Deprecated | Superseded  
**Date:** YYYY-MM-DD  
**Deciders:** @github-handle1, @github-handle2  
**Modules Affected:** `src/module1/`, `src/module2/`  
**Related Research:** [Paper Title](../papers/related.md) | [Best Practice](../best_practices/related.md)

---

## Context

*(Describe the situation and constraints that make this decision necessary.  
What problem are we solving? What requirements must be met?)*

## Decision Drivers

- Driver 1 (e.g., must support 10M vectors at < 10ms p99 query latency)
- Driver 2 (e.g., must run on CPU-only environments)
- Driver 3 (e.g., must support incremental updates without full rebuild)

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| Option A | Pro 1, Pro 2 | Con 1, Con 2 |
| Option B | Pro 1 | Con 1, Con 2, Con 3 |
| Option C | Pro 1, Pro 2, Pro 3 | Con 1 |

## Decision

**Chosen: Option A**

*(Why was this option selected? Reference the decision drivers above.)*

## Consequences

### Positive
- Consequence 1
- Consequence 2

### Negative / Trade-offs
- Trade-off 1 — *mitigation: ...*
- Trade-off 2 — *accepted because: ...*

### Neutral
- Neutral observation 1

## Validation

- [ ] Prototype built and benchmarked
- [ ] Code review completed
- [ ] Unit and integration tests written
- [ ] Module README updated
- [ ] implementation_influence index updated

## Follow-up Actions

- [ ] Action 1
- [ ] Action 2

## Related Decisions

- [ADR-NNN: Related Decision](adr_NNN_related.md)

---
**Last Updated:** YYYY-MM-DD
