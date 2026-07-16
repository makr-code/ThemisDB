---
name: "dokifi"
description: "Level-based docs sync (L0=source scan, L1=module docs, L2=aggregates, L3=root). Minimal input with prerequisite verification."
argument-hint: "level (L0|L1|L2|L3) OR operation + target path (legacy); optional: scope tags, milestone"
agent: "doc-orchestrator"
---

Run level-based documentation orchestration with minimal input.

Fallback: if the configured agent is unavailable in-session, run with identical policy and report fallback mode.

Minimum input: level (L0 | L0.5 | L1 | L2 | L3)

Optional:
- file/directory paths (narrows scope; e.g., `src/graph`, `ai_working/changelog_*.md`, `CHANGELOG.md`)
- scope tags: module, tests, benchmark, api, security, release
- milestone override

**NEW:** L0.5 (Gap Verification) — **Strongly recommended** between L0 and L1 to eliminate false-positives and re-assess severity

Scope Examples (Smart Module Discovery + Explicit Paths):
```
/dokifi L0              # Full gap_scanner on entire sourcecode
/dokifi L0 graph        # Auto-discover graph module (src/graph, include/graph, tests/graph, benchmarks/graph)
/dokifi L0 src/graph    # Gap_scanner scoped to src/graph/ only (explicit path)

/dokifi L1              # Update all 127+ module docs (all locations)
/dokifi L1 graph        # Auto-discover all graph-module docs (src, include, tests, benchmarks)
/dokifi L1 src/graph    # Update only src/graph/ module docs (explicit)
/dokifi L1 src/cache    # Update only src/cache/ module docs (explicit)

/dokifi L2              # Aggregate all ai_working/ snapshots
/dokifi L2 ai_working/changelog_*.md   # Only changelog snapshots

/dokifi L3              # Update all root docs
/dokifi L3 CHANGELOG.md # Only CHANGELOG.md (explicit)
```

**Smart Scope Discovery:**
- Bare name (no `/`): auto-discover across `src/`, `include/`, `tests/`, `benchmarks/`
- Explicit path (has `/`): use exact path only
- Example: `/dokifi L1 graph` finds & updates src/graph, include/graph, tests/graph, benchmarks/graph if present

Level-Based Commands (with Smart Module Discovery):
```
/dokifi L0                 # Trigger gap_scanner + header writer (full sourcecode)
/dokifi L0 graph           # Gap_scanner scoped to graph module (all locations)
/dokifi L0 src/graph       # Gap_scanner scoped to src/graph/ only (explicit)

/dokifi L0.5 graph         # **VERIFY findings:** re-assess severity, eliminate false-positives
/dokifi L0.5 src/graph     # **VERIFY scoped:** verify findings for src/graph/ only

/dokifi L1                 # Update module docs (all 127+ modules, all locations)
/dokifi L1 graph           # Update all graph-module docs (auto-discover src/graph, include/graph, tests/graph, benchmarks/graph)
/dokifi L1 src/cache       # Update only src/cache/ module docs (explicit)

/dokifi L2                 # Update aggregates (all ai_working/ snapshots)
/dokifi L2 ai_working/changelog_*.md   # Only changelog snapshots

/dokifi L3                 # Update root docs (CHANGELOG.md, README.md, SECURITY.md)
/dokifi L3 CHANGELOG.md    # Only CHANGELOG.md (explicit)
```

**Recommended Flow:**
1. `/dokifi L0 graph` (scan for gaps)
2. `/dokifi L0.5 graph` (verify + re-assess severity) ← **BLOCKS false-positives**
3. `/dokifi L1 graph` (update docs with verified findings only)
4. `/dokifi L2 graph` (aggregate L1 snapshots)
5. `/dokifi L3` (update root docs)

Return:
1. **level status** (L0-L3 age and availability — informative)
2. **operations** (files affected, changes summary)
3. **warnings** (if upstream docs are old, but not blocking)
4. **canonical sources** (data sources for this level)
5. **validation** (naming, structure, duktus, SOT consistency)
6. **conformance** (pass/fail + deviations)
7. **follow-up** (suggested next level or release milestone)
