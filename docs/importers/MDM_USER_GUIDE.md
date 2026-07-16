# Master Data Management (MDM) User Guide

> Alignment note (2026-05-31): This document is a secondary user/operations guide.
> Authoritative current workload and target behavior are defined in:
> - `src/importers/FUTURE_ENHANCEMENTS.md`
> - `src/importers/MODULE_GAPS.md`
> - `src/importers/ROADMAP.md`
> If this guide conflicts with newer planning docs, planning docs take precedence.

## Overview

ThemisDB's MDM subsystem automatically deduplicates and links imported data with
existing entities in the database.  When you import a PostgreSQL dump (or any
other source), the MDM pipeline runs transparently after the standard import
phase to:

1. **Match** incoming entities against existing ones using exact-key or fuzzy
   string-distance algorithms.
2. **Link** matched entity pairs with a typed, directed relationship in the graph.
3. **Resolve** conflicts by producing a single *golden record* from all
   contributing source records.
4. **Audit** every decision with an immutable, chain-linked audit trail.

---

## Matching Strategies

### Deterministic Matching (Exact Key)

Uses primary keys, unique field constraints, or custom identifier mappings to
find exact matches.

| Property | Value |
|---|---|
| Confidence | Always 1.0 |
| Speed | O(1) per entity with index support |
| Use case | Email, UUID, SKU, national ID |

### Semantic Matching (Fuzzy / String Distance)

Applies text-similarity algorithms field by field:

| Algorithm | Best for | Example |
|---|---|---|
| `jaro_winkler` | Names, short strings | "John Smith" ↔ "Jon Smith" |
| `levenshtein` | Typos, edit distance | "Smith" ↔ "Smyth" |
| `soundex` | Phonetic equivalence | "Smith" ↔ "Smythe" |
| `email` | Email local-parts | "alice@co.com" ↔ "alce@co.com" |
| `phone` | Phone numbers | "+1(202)555-1234" ↔ "2025551234" |

### Hybrid Ensemble

Combines both strategies with configurable weights:

```yaml
ensemble:
  method: "weighted_voting"
  deterministic_weight: 0.6
  semantic_weight: 0.4
  final_threshold: 0.85
```

---

## Configuration

### Minimal Example (`import_config.yaml`)

```yaml
entity_linking:
  enabled: true
  strategy: "hybrid"               # deterministic | semantic | hybrid
  deterministic_threshold: 1.0
  semantic_threshold: 0.85

  collections:
    users:
      deterministic:
        primary_key: ["id"]
        unique_fields: ["email"]
      semantic:
        text_fields: ["first_name", "last_name"]
        field_matchers:
          first_name:
            algorithm: "jaro_winkler"
            weight: 0.4
          last_name:
            algorithm: "jaro_winkler"
            weight: 0.4
          email:
            algorithm: "email"
            weight: 0.2
      resolution:
        policy: "richest_merge"
        protected_fields: ["id", "created_at"]
```

### All Resolution Policies

| Policy | Behaviour |
|---|---|
| `newest_first` | Prefer the record with the most-recent timestamp |
| `most_complete` | Prefer the record with the fewest null fields |
| `existing_preferred` | Keep the record already in ThemisDB |
| `incoming_preferred` | Replace with the newly imported record |
| `richest_merge` | Field-level: keep the longer / non-null value |
| `custom_rules` | Per-field rules (max, min, sum, concatenate, etc.) |

### Per-Field Rules (for `custom_rules` policy)

```yaml
field_rules:
  updated_at: "take_newest"    # Keep later timestamp
  balance:    "take_sum"       # Add both balances
  notes:      "concatenate"    # Join with " | "
  score:      "take_max"       # Keep highest
  status:     "keep_existing"  # Never overwrite
```

---

## Workflow

The MDM pipeline runs four phases in sequence:

```
Incoming entities
      │
      ▼
 ┌─────────────────┐
 │  1. Match phase  │   Deterministic + Semantic → HybridMatchResults
 └──────┬──────────┘
        │
        ▼
 ┌──────────────────┐
 │  2. Link phase   │   Create EntityLink objects (+ optional reverse links)
 └──────┬───────────┘
        │
        ▼
 ┌──────────────────────┐
 │  3. Resolution phase │   Build GoldenRecords from matched groups
 └──────┬───────────────┘
        │
        ▼
 ┌──────────────────┐
 │  4. Audit phase  │   MDMAuditTrail events for every decision
 └──────────────────┘
```

---

## Actions on Match

Configure what happens when a match is found:

| Confidence range | Recommended action |
|---|---|
| 1.0 (deterministic) | `auto_merge` |
| 0.85 – 0.99 | `auto_merge` |
| 0.75 – 0.85 | `review` (manual queue) |
| 0.0 – 0.75 | `skip` |

Set `auto_resolve_conflicts: false` to route all matches below
`deterministic_threshold` to the manual-review queue.

---

## Review Queue

When `auto_resolve_conflicts: false`, entities that produce matches below the
deterministic threshold are placed in `MDMWorkflowResult::review_queue`.

Retrieve and process the queue programmatically:

```cpp
auto result = engine.executeMDMWorkflow(...);

for (const auto& entity : result.review_queue) {
    // Inspect entity and decide: approve or skip
}
```

---

## Audit Trail

Every MDM decision is recorded in an `MDMAuditTrail`:

```cpp
MDMAuditTrail audit;

// Record a manual review decision
MDMAuditTrail::AuditEvent ev;
ev.operation        = MDMAuditTrail::Operation::REVIEW_COMPLETED;
ev.collection_name  = "users";
ev.source_entity_id = "incoming-123";
ev.target_entity_id = "existing-456";
ev.confidence_score = 0.92;
ev.status           = "approved";
audit.recordEvent(ev);

// Verify integrity
bool intact = audit.verifyAuditChain();

// Export for compliance
auto report = audit.exportAuditReport("users", "2026-01-01", "2026-12-31");
```

---

## Metrics

```cpp
MDMMetricSnapshot snap;
snap.deterministic_matches = result.deterministic_matches;
snap.semantic_matches      = result.semantic_matches;
snap.links_created         = result.links_created;
// ... populate remaining fields ...

// Emit Prometheus-style metrics
MDMMetrics::emitMetrics(snap, "users", [](const std::string& name,
                                           const std::map<std::string, std::string>& labels,
                                           double value) {
    // Push to your metrics backend
});

// Get dashboard JSON
auto dash = MDMMetrics::getDashboardMetrics(snap, "users");
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| All incoming entities appear as "new" | Matching threshold too high or wrong key fields | Lower `semantic_threshold`; verify `primary_key` |
| Too many manual reviews | Threshold too low | Raise `semantic_threshold` |
| Protected fields get overwritten | Entity list order wrong | Entity[0] must be the "original/existing" entity |
| Self-links created | Incoming entity is matched against itself | Pass only EXISTING entities to `existing_entities`; do not include the incoming batch |
| Golden records missing fields | `most_complete` base selection picked wrong entity | Switch to `richest_merge` policy |
