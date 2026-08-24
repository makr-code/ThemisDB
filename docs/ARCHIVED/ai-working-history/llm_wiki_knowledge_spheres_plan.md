# LLM Wiki Knowledge Spheres — Implementation Plan

## Objective
Create an explicit, versioned, and auditable knowledge-sphere model for ThemisDB LLM wiki data so that knowledge domains can be extended, deprecated, removed, or migrated without destabilizing runtime behavior.

## Scope
- LLM wiki module only
- applies to canonical, process, derived, ingested, and protected knowledge domains
- future-ready for plugin-driven and multi-tenant runtime evolution

## Principles
1. Canonical semantics stay authoritative.
2. Derived data remains reproducible.
3. Ingested data remains explicitly attributable.
4. Protected/private knowledge stays isolated.
5. All lifecycle changes are logged and reversible.

## Proposed Architecture

### 1. Knowledge Sphere Registry
- central registry for all domain declarations
- stores descriptor metadata and lifecycle status
- validates uniqueness of sphere IDs and target migration paths

### 2. Sphere Descriptor Schema
Fields:
- `sphere_id`
- `name`
- `status`
- `schema_version`
- `source_of_truth`
- `owner`
- `access_scope`
- `retention_policy`
- `migration_target`
- `audit_log_ref`

### 3. Validation Layer
- validates schema compatibility
- checks protection boundaries
- rejects invalid or unauthorized transitions
- enforces fail-closed behavior on malformed metadata

### 4. Migration Layer
- supports explicit upgrades, rollbacks, and archive transitions
- required for any major schema change or domain relocation
- must be idempotent and auditable

### 5. Runtime Integration
- Registry is consulted at startup and on policy refresh
- Query and ingest flows validate domain membership before write or read access
- Policy changes can update active spheres without breaking existing objects

## Phase Plan

### Phase 1 — Design and API Contract
- Define `KnowledgeSphereDescriptor`
- Define `KnowledgeSphereRegistry` interface
- Define status lifecycle values and migration semantics
- Document protected/public domain separation

### Phase 2 — Core Implementation
- Implement registry and descriptor storage
- Implement validation hooks with fail-closed semantics
- Wire the registry into initialization and policy reload logic
- Add default sphere declarations for existing LLM wiki domains

### Phase 3 — Error Handling and Safety
- Implement duplicate ID rejection
- Implement invalid migration target rejection
- Preserve last-known-good state during failed reloads
- Enforce protected-domain isolation

### Phase 4 — Test Coverage
- registry lifecycle tests
- migration and rollback tests
- schema validity tests
- protected/private isolation tests
- mixed-version compatibility tests

### Phase 5 — Performance and Hardening
- cache registry lookups
- benchmark sphere validation overhead
- concurrency-safe updates
- production audit logging without query-path regression

### Phase 6 — Documentation and Release Gate
- publish migration and retirement guide
- publish operator safety checklist
- require migration plan for future schema evolution

## Acceptance Criteria
- A sphere can be added without changing core logic elsewhere
- A sphere can be deprecated without deleting active data
- A sphere can be migrated with explicit target and rollback step
- Protected domains remain isolated from public runtime indexes
- All lifecycle changes are recorded as audit evidence

## Known Risks
- accidental mixing of canonical and derived data
- missing provenance during migration
- unauthorized access to protected knowledge domains
- stale registry data after hot reload

## Immediate Next Step
Implement the registry and descriptor model as a first code-level foundation, then integrate it into the LLM wiki config and policy validation flow.
