---
Author: "ThemisDB Team"
Created: "2026-11-23"
Last Updated: "2026-11-23"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "include/tensor/dream_candidate_store.h"
  - "src/tensor/dream_candidate_store.cpp"
  - "tests/tensor/test_tensor_dream_candidate_store.cpp"
  - "tests/tensor/CMakeLists.txt"
  - "src/tensor/ROADMAP.md"
  - "src/tensor/THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md"
source: "include/tensor/dream_candidate_store.h; src/tensor/dream_candidate_store.cpp; tests/tensor/test_tensor_dream_candidate_store.cpp; tests/tensor/CMakeLists.txt"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# Tensor Dream-Mode — Developer LLM Wiki

This page summarizes the implemented Dream-Mode candidate-store boundary. It is
secondary context: the public header and implementation define behavior, tests
define the covered cases, and the tensor roadmap and comparative paper define
the broader intended gates. This store does not implement the complete
Dream-Mode workflow or close those gates.

## Candidate-store contract

[`DreamCandidateStore`](../../include/tensor/dream_candidate_store.h) accepts a
non-empty candidate ID, non-null JSON payload, and provenance with exactly the
required true labels `synthetic=true`, `dream_mode=true`, and a non-blank
`run_id`. An existing candidate ID cannot be overwritten. Invalid input is
rejected; read and export return no value when an item is missing or fails
validation.

The default constructor uses an in-memory backend. Callers may inject an
`ITensorStorageBackend`; records are stored as versioned envelopes beneath the
dedicated `__dream_candidate__:v1:` namespace, with candidate IDs hex-encoded
in keys. Reads re-parse and revalidate the provenance and checksum. Persistence
across process restarts depends on selecting a durable backend; the focused
tests only demonstrate retention across store instances sharing a backend.

The store has no production truth-store dependency and exposes no production
write or promotion API. The dedicated key prefix isolates candidate records
logically, but does not establish a separate physical database or access-control
boundary. One store instance serializes its operations; callers sharing a
backend across multiple writers must serialize writes because the backend
interface does not provide conditional put.

## Envelope and integrity boundaries

`exportArtifact()` emits a version-1 JSON envelope containing the candidate ID,
payload, provenance labels, checksum algorithm, and checksum.
`importArtifact()` requires the exact envelope and provenance keys, validates
field types and required labels, then verifies the checksum before writing the
artifact to the configured backend. Reads and exports also verify the stored
artifact; malformed persisted records fail closed.

The checksum is FNV-1a 64-bit over the serialized envelope without the checksum
fields. It detects accidental content changes but is not cryptographic, does
not authenticate a producer, and must not be used as evidence of trust or
authorization. `run_id` is checked only for non-blank content; the store does
not verify that it belongs to a persisted `DreamOrchestrator` run. The envelope
checksum is not authentication and no trusted promotion path is implemented.

## Tests and evidence limits

[`test_tensor_dream_candidate_store.cpp`](../../tests/tensor/test_tensor_dream_candidate_store.cpp)
covers successful store/read/export/import with provenance retention, missing
or false labels, whitespace run IDs, null payloads, malformed envelopes,
checksum mismatch, duplicate IDs, default-backend per-instance isolation,
shared-backend re-open, namespace isolation, and rejection of a corrupted
persisted envelope.
[`tests/tensor/CMakeLists.txt`](../../tests/tensor/CMakeLists.txt) discovers
`test_*.cpp` and registers the focused test target. Registration and source
coverage do not establish that the CTest target has built or passed.

The tests do not establish process-restart durability, integration with the
orchestrator, production retrieval isolation, cryptographic authenticity,
utility/nonregression, or ethics-gate behavior. The module roadmap continues
to track broader Dream-Mode work under WP-BRAIN-04 and BRAIN-GATE-02; do not
infer completion from this candidate-store component.

## Canonical references

- [Candidate store API](../../include/tensor/dream_candidate_store.h)
- [Candidate store implementation](../../src/tensor/dream_candidate_store.cpp)
- [Focused tests](../../tests/tensor/test_tensor_dream_candidate_store.cpp)
- [Tensor roadmap](../../src/tensor/ROADMAP.md)
- [Dream-Mode architecture and issue packet](../../src/tensor/THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md)
