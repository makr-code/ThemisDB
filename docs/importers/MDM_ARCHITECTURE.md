# MDM Architecture & Design

> Alignment note (2026-05-31): This document is a secondary architecture explainer.
> Authoritative current workload and target behavior are defined in:
> - `src/importers/FUTURE_ENHANCEMENTS.md`
> - `src/importers/MODULE_GAPS.md`
> - `src/importers/ROADMAP.md`
> If this document conflicts with newer planning docs, planning docs take precedence.

## Components

```
┌────────────────────────────────────────────────────────────┐
│                      MDMEngine                             │
│  (orchestrates the four-phase workflow)                    │
│                                                            │
│  ┌──────────────────────┐  ┌───────────────────────────┐  │
│  │  HybridEntityMatcher │  │  CanonicalEntityResolver  │  │
│  │  ┌────────────────┐  │  │  ┌─────────────────────┐  │  │
│  │  │ Deterministic  │  │  │  │  Field Reconciler   │  │  │
│  │  │ Matcher        │  │  │  └─────────────────────┘  │  │
│  │  ├────────────────┤  │  └───────────────────────────┘  │
│  │  │ Semantic       │  │                                  │
│  │  │ Matcher        │  │  ┌───────────────────────────┐  │
│  │  └────────────────┘  │  │  EntityLinker             │  │
│  └──────────────────────┘  └───────────────────────────┘  │
│                                                            │
│  ┌──────────────────────┐  ┌───────────────────────────┐  │
│  │  MDMAuditTrail       │  │  MDMMetrics               │  │
│  └──────────────────────┘  └───────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

### 1. DeterministicMatcher

Matches incoming entities to existing ones using hard key equality:

- **Primary key** matching: extracts the configured PK field value and looks it
  up in the existing-entity index.
- **Unique-field** matching: checked independently for each configured unique
  field (e.g., email, SSN).
- **Custom identifier** matching: uses a caller-supplied JSON `{"source_field":
  "target_field"}` mapping.

All matches return `confidence_score = 1.0`.

### 2. SemanticMatcher

Assigns a per-field similarity score using configurable string-distance
algorithms:

| Algorithm | Complexity | Notes |
|---|---|---|
| Jaro-Winkler | O(n) | Optimised for short strings; prefix bonus |
| Levenshtein | O(n·m) | Edit distance; good for typos |
| Soundex | O(n) | Phonetic code; language-agnostic |
| Email | O(n) | Domain equality gate + local-part JW |
| Phone | O(n) | E.164 normalisation before comparison |
| Cosine | O(d) | Optional embedding-based path (d = dim) |

Per-field weights are normalised to sum to 1.0 before the weighted average is
computed.

### 3. HybridEntityMatcher

Runs the deterministic and semantic passes according to the chosen strategy:

```
DETERMINISTIC_FIRST:  exact → fuzzy fallback
SEMANTIC_FIRST:       fuzzy → exact confirmation
WEIGHTED_ENSEMBLE:    both in parallel, DET×0.6 + SEM×0.4
```

Validates all deterministic matches against the `existing_entities` list to
prevent spurious self-matches when the incoming entity set is disjoint from
the existing set.

### 4. EntityLinker

Stores directed `EntityLink` objects in memory during the import session.
Each link carries:

- `link_type` (8 variants: SAME_AS, DUPLICATE_OF, …)
- `status` (UNRESOLVED / RESOLVED / MANUAL_REVIEW / ARCHIVED)
- `confidence` ∈ [0, 1]
- `matching_evidence` (JSON)
- `chain_hash` (audit)

### 5. CanonicalEntityResolver

Builds a `GoldenRecord` from a group of linked entities:

1. Selects a **base entity** according to the resolution policy.
2. Pre-applies **protected fields** from entity[0] (original/existing) onto the
   base regardless of policy.
3. Iterates over all other contributing entities; applies field-level rules or
   the default policy-based merge logic.
4. Computes `completeness_score` = non-null fields / total fields.

### 6. MDMAuditTrail

Immutable, append-only event log:

- Each event gets an auto-generated UUID and RFC 3339 timestamp.
- Chain hash: FNV-1a 64-bit over `previous_hash + event_id + timestamp +
  source_id + target_id`.
- `verifyAuditChain()` recomputes all hashes for tamper detection.

### 7. MDMMetrics

Stateless adapter that converts a `MDMMetricSnapshot` to:

- Prometheus-style gauge/counter callbacks (12 named metrics, all labelled with
  `collection`).
- A structured JSON dashboard object.

---

## Data Flow (Sequence)

```
Caller
  │
  ├─ executeMDMWorkflow(incoming, existing, config)
  │
  │  executeMatchingPhase()
  │    │  for each incoming entity:
  │    │    HybridEntityMatcher.findMatchingEntities(...)
  │    │      DeterministicMatcher.findExactMatches(...)  ← validated against existing
  │    │      SemanticMatcher.findSimilarEntities(...)
  │    └─ returns [[HybridMatchResult, …], …]
  │
  │  executeLinkingPhase()
  │    │  for each match:
  │    │    EntityLinker.createLink(...)  ← skip if dry_run
  │    │    EntityLinker.createLink(reverse) [if create_reverse_links]
  │    └─ returns [EntityLink, …]
  │
  │  executeResolutionPhase()
  │    │  group links by source_id
  │    │  for each group:
  │    │    CanonicalEntityResolver.createGoldenRecord(...)
  │    └─ returns [GoldenRecord, …]
  │
  └─ returns MDMWorkflowResult
```

---

## Performance Characteristics

| Operation | Complexity | Notes |
|---|---|---|
| Deterministic matching (indexed) | O(1) per entity | Hash-map or B-tree index |
| Deterministic matching (scan) | O(n) per entity | Without index support |
| Semantic matching | O(n × m) | n = existing entities, m = avg fields |
| Link creation | O(1) per link | In-memory append |
| Resolution | O(k × f²) | k = matched entities, f = fields |
| Audit chain verify | O(e) | e = number of events |

---

## Guarantees

- **Correctness**: deterministic matches are always validated against the actual
  `existing_entities` list; no spurious self-matches.
- **Protected fields**: entity[0]'s (original) values for protected fields are
  never replaced, regardless of resolution policy.
- **Dry-run safety**: `dry_run = true` prevents any persistent storage (no links
  stored, `links_created = 0` in the result).
- **Audit immutability**: the chain hash makes any tampering with historical
  events detectable via `verifyAuditChain()`.
- **Thread safety**: `MDMAuditTrail` is protected by a mutex; all other
  components are stateless and safe for concurrent read access.

---

## Breaking Changes vs. Prior Versions

None – all MDM types are additive.  The `PostgreSQLImporter` interface is
unchanged; MDM is opt-in via `ImportOptions.entity_linking.enabled`.
