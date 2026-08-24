# LLM Wiki Future Enhancements

## Knowledge Spheres

### Scope
- Define and enforce explicit knowledge domains for ThemisDB LLM wiki content.
- Separate canonical, process, derived, ingested, and protected knowledge into independent runtime domains.
- Support future extension, archival, removal, and migration without breaking the active knowledge graph.
- Keep all domain transitions auditable, versioned, and reversible.

### Design Constraints
- Every sphere must have a stable `sphere_id`, owner, schema version, lifecycle status, and migration target.
- Canonical core knowledge is the only domain allowed to define normative semantics by default.
- Derived data must be reproducible from canonical or ingested source data.
- External and protected knowledge must never be merged silently into the public runtime graph.
- Unknown or unsupported extension payloads must fail closed unless explicitly allowlisted.

### Required Interfaces
- `KnowledgeSphereRegistry` for registration and lookup
- `KnowledgeSphereDescriptor` with fields:
  - `sphere_id`
  - `name`
  - `status` (`active`, `deprecated`, `archived`, `migrating`)
  - `schema_version`
  - `owner`
  - `source_of_truth`
  - `access_scope`
  - `retention_policy`
  - `migration_target`
  - `audit_log_ref`
- `KnowledgeSphereValidator` for schema and lifecycle checks
- `KnowledgeSphereMigrator` for deterministic upgrade and rollback paths
- `KnowledgeSpherePolicy` for security and governance enforcement

### Implementation Notes
- Domain boundaries must be explicit in the schema layer, not implicit in business logic.
- Use a registry model rather than hard-coded conditional branches for sphere selection.
- Each sphere should expose:
  - ingestion contract
  - validation contract
  - export contract
  - deprecation date or replacement target
  - audit trail reference
- Migration paths must be explicit and idempotent; no in-place overwrites without revision history.
- Protected/private knowledge must not enter shared public indexes without explicit policy approval.

### Test Strategy
- Unit tests for registry registration and lifecycle transitions
- Migration tests for old-to-new sphere version upgrades
- Rollback tests for failed migration and policy rejection
- Access-control tests for public, protected, and derived sphere isolation
- Compatibility tests for mixed-version readers and writers
- Integration tests for shadow migration and canary rollout behavior

### Performance Targets
- Sphere registry lookup p99 < 5 ms in normal runtime traffic
- Schema validation overhead < 2% of query latency under expected load
- Migration planner overhead < 10 ms for standard domain upgrades
- No hot-path dependency on full document revalidation for unchanged objects

### Security / Reliability
- Fail closed on invalid schema, missing provenance, or unauthorized expansion.
- Preserve last-known-good state during failed migration or reload.
- Require audit records for every sphere lifecycle change.
- Do not allow ML tuning to change canonical safety invariants or governance gates.

---

## Knowledge Sphere Model

### 1. Canonical Knowledge
- Definition: stable source-of-truth objects and core semantic definitions.
- Examples: entity definitions, canonical policy metadata, normative schema objects.
- Rules:
  - authoritative
  - versioned
  - migration is explicit and logged
  - cannot be silently overwritten

### 2. Process Knowledge
- Definition: runtime control data and orchestration configuration.
- Examples: YAML process policies, stage timing plans, rollout thresholds, ML tuning knobs.
- Rules:
  - reloadable under guardrails
  - fail-safe with last-known-good fallback
  - separate from canonical semantics

### 3. Derived Knowledge
- Definition: materialized, generated, or inferred representations.
- Examples: synthesized summaries, embeddings, candidate evidence maps, extracted metadata.
- Rules:
  - reproducible from source domain
  - removable without destroying source truth
  - provenance must be retained

### 4. Ingested External Knowledge
- Definition: imported content from sources outside the canonical domain.
- Examples: Wikipedia dumps, public documents, external knowledge feeds.
- Rules:
  - normalize before insertion into canonical or derived spaces
  - preserve source provenance and import metadata
  - explicitly tag stale or non-authoritative content

### 5. Protected Knowledge
- Definition: tenant-restricted, security-sensitive, or private knowledge.
- Examples: private plugin data, compliance-specific corpora, enterprise-only policies.
- Rules:
  - never mixed into public knowledge indexes
  - isolated by ACL, policy, and storage boundaries
  - exported only under approved safe formats

---

## Future Implementation Phases

### Phase 1: Design / API Contract
- [ ] Define `KnowledgeSphereDescriptor` schema and lifecycle enumerations
- [ ] Define the registry and migration interfaces
- [ ] Specify owner, source-of-truth, and retention metadata contracts
- [ ] Document default rules for canonical, derived, ingested, and protected domains
- [ ] Define fail-closed behavior for invalid sphere metadata

### Phase 2: Core Implementation
- [ ] Implement `KnowledgeSphereRegistry` with registration and lookup
- [ ] Implement `KnowledgeSphereValidator` for schema and lifecycle checks
- [ ] Add metadata persistence for sphere state and migration target
- [ ] Wire registry validation into initialization and policy refresh flows
- [ ] Add default sphere definitions for the current LLM wiki module

### Phase 3: Error Handling & Edge Cases
- [ ] Handle duplicate sphere IDs and conflicting migration targets
- [ ] Fail gracefully when source-of-truth is missing or unreachable
- [ ] Reject protected sphere leakage into public indexes
- [ ] Preserve last known good state on failed reload or upgrade
- [ ] Add explicit diagnostics for deprecation and archived domains

### Phase 4: Tests
- [ ] Add unit tests for registry lifecycle: add, archive, remove, migrate
- [ ] Add schema tests for version mismatch handling
- [ ] Add rollback tests for partial migration failures
- [ ] Add access-control tests for protected sphere separation
- [ ] Add migration compatibility coverage across versions

### Phase 5: Performance / Hardening
- [ ] Add caching for frequent registry lookups
- [ ] Benchmark registry validation overhead under typical workloads
- [ ] Add concurrency-safe sphere state updates
- [ ] Enforce audit logging without blocking query path
- [ ] Harden migration planner for large-domain transitions

### Phase 6: Documentation & Acceptance
- [ ] Publish domain lifecycle guide for operators and maintainers
- [ ] Publish migration playbook for deprecation and removal
- [ ] Publish security checklist for protected sphere handling
- [ ] Establish release gate requiring migration plan before shipping schema changes

---

## Migration and Removal Policy

### Extension
- Add a new sphere only when a clear ownership and schema boundary is defined.
- New domains must register with the registry before runtime usage.
- Extension must be accompanied by a migration plan and governance policy.

### Removal
- A sphere may be removed only after
  - deprecation status is set,
  - replacement target is published,
  - migration or archive path is defined,
  - audit evidence is recorded.
- Any removal must preserve read compatibility for supported legacy versions.

### Migration
- Migration must be deterministic and idempotent.
- Old and new sphere versions must both be readable during transition.
- Rollback must be possible without data deletion.
- All migration steps must emit event records and reason codes.

---

## Recommended Next Steps
- [ ] Add a `KnowledgeSphereRegistry` implementation under the LLM wiki module
- [ ] Add schema metadata to `llm_wiki_entity.schema.json`
- [ ] Add policy metadata to `llm_wiki_process_policy.yaml`
- [ ] Define deprecation and migration checks in the module roadmap
- [ ] Add smoke tests for sphere lifecycle and policy-driven migrations
