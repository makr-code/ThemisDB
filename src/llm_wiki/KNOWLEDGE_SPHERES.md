# Knowledge Spheres Configuration Model

## Purpose
This document defines the recommended configuration model for ThemisDB knowledge domains in the LLM wiki space. It separates the runtime contract from the active policy and the concrete runtime instance.

## Model Layers

### 1. Schema layer
The schema defines the legal structure and allowed values for a sphere definition.

- File: `schema/knowledge_spheres.schema.json`
- Role: contract validation and compatibility enforcement
- Purpose: prevent invalid or unsafe sphere definitions

### 2. Policy layer
The YAML policy defines the active control plane for how spheres are arranged and managed at runtime.

- File: `process/knowledge_spheres_policy.yaml`
- Role: active operational policy
- Purpose: define which domains are active, which are archived, and which safety invariants must hold

### 3. Runtime layer
The JSON runtime file describes the concrete live configuration for a deployment or workspace.

- File: `runtime/knowledge_spheres.runtime.json`
- Role: concrete instance data
- Purpose: represent the actual active spheres for a deployment session or environment

---

## Configuration Responsibilities

### Schema
Schema is responsible for:
- allowed sphere types
- required fields
- legal status values
- contract for migration targets
- protected/private boundary rules
- lifecycle compatibility rules

### YAML
YAML is responsible for:
- active policy selection
- migration requirements
- rollout mode
- safety invariants
- canary/shadow strategy
- tunable knobs and hard bounds

### JSON
JSON is responsible for:
- specific runtime instance values
- active deployment mapping
- current sphere assignments
- audit references
- version snapshots of the active configuration

---

## Spheres

### Canonical Knowledge
Authoritative domain for stable semantics and core definitions.

### Process Knowledge
Runtime policy and orchestration state, including YAML-controlled workflows.

### Derived Knowledge
Generated, inferred, or synthesized data derived from a source domain.

### Ingested External Knowledge
Imported public or external knowledge sources such as Wikipedia or partner feeds.

### Protected Knowledge
Restricted private or enterprise-only knowledge that must never leak into public collections.

---

## Governance Rules

1. Canonical knowledge is source of truth by default.
2. Derived knowledge must preserve provenance and be reproducible.
3. External knowledge must be normalized and explicitly tagged.
4. Protected knowledge must remain isolated by policy and access scope.
5. Migration is explicit, logged, and reversible.
6. Invalid sphere config must fail closed.
7. ML tuning cannot override hard safety invariants.

---

## Recommended Future Usage

Use this model in ThemisDB as a policy-driven domain registry:
- active sphere policy via YAML
- runtime state via JSON
- legality and compatibility via schema validation

This enables future extension, retirement, and migration of knowledge domains without hardcoding logic into the runtime engine.
