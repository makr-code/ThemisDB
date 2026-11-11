# Core Provider/Consumer Integration: Implementation Task

## Status and purpose

This document is the implementation contract for the follow-up work requested
after PRs [#8051](https://github.com/makr-code/ThemisDB/pull/8051) and
[#8055](https://github.com/makr-code/ThemisDB/pull/8055). PR #8051 is merged
and provides the existing RAG provider, typed retrieval request/context/result,
evidence candidate, and shared llama.cpp prompt-preparation contracts. PR #8055
is also merged, but its diff only refreshed generated developer-wiki files; it
did not deliver the Core integration described in its summary.

**This is a task definition, not evidence that the implementation is complete.**
The implementation PR must change production source, relevant canonical
documentation, and focused tests, and must leave the checkboxes below open until
each criterion has been verified.

## Required outcome

Provide one internal, typed retrieval path that uses ThemisDB's existing
RocksDB-backed storage and specialized indexes (relational, graph, geo, and
vector) and can be consumed by real HTTP, AQL, and YAML-agentic execution paths.
Retrieval must not depend on which endpoint asks for evidence. Inference is
optional and remains a separate consumer of prepared evidence.

Keep the ABI/API surface shallow: reuse the contracts delivered by #8051 and
existing storage/index/query types. Do not add a second RAG manager, a universal
data bus, another factory/manager layer, or a stack of endpoint-specific
wrappers.

## Current source facts to revalidate before implementation

- `QueryEngineBuilder` accepts `IStorageEnginePtr` and `IIndexManagerPtr`, and
  constructs the interface-based `QueryEngine`. The constructor stores those
  interfaces, while established retrieval paths still use the concrete
  RocksDB/index manager members. The interface-only builder path therefore
  must not be presented as a working replacement for the production backend
  composition without fixing and testing that boundary.
- The HTTP RAG path constructs a retrieval `QueryEngine` and `RAGProvider`
  locally. AQL and YAML-related consumers expose injection APIs; trace actual
  startup/composition callers and wire the real consumers rather than adding
  dummy consumers or assuming the setters are called.
- `VectorIndexSinkAdapter` writes vector entities to `VectorIndexManager`.
  Hybrid entity materialization currently reads canonical relational keys and
  legacy table keys. Vector/content identity is not thereby guaranteed to
  resolve to source text.
- Content chunk text is stored under `chunk:<id>`; chunk vectors use a separate
  `chunks:<id>` identity and index metadata. A vector match alone is not the
  evidence text.
- `search`, `LayeredRetrievalOrchestrator`, `ProcessGraphRag`, and
  `ethics_ai/RAGContextEngine` have specialized behavior. Preserve their domain
  policies; this task does not migrate all of them.

Recheck these facts against the current source, not generated wiki summaries.
Update or narrow this task if source evidence differs before coding.

## Work packages

### A. Functional production composition

1. Make the required retrieval/query path use the actual RocksDB-backed storage
   and index instances that the server owns. A non-null interface pointer is
   not proof that query execution works.
2. Extend the existing bootstrap/composition surface minimally. Preserve
   ownership and shutdown order; do not create a second backend or introduce
   unsafe downcasts to make the builder appear generic.
3. Keep the remaining limitations of interface-based QueryEngine dependency
   injection explicit. A universal refactor of all QueryEngine execution
   methods is not required.

### B. Canonical source identity and materialization

1. Define an explicit typed source reference that connects an indexed entity to
   its authoritative source, collection, key, provenance, and any existing
   version/freshness information.
2. Implement the lookup path for normal vector ingestion and ContentManager
   chunks so a retrieved vector can be resolved to its actual stored text.
3. Preserve existing key schemas and on-disk compatibility, or provide an
   explicit migration. Do not guess keys using arbitrary prefix/colon
   heuristics, scan unrelated collections, or read an unvalidated vector
   namespace.
4. Do not treat embeddings, metadata, identifiers, or error messages as
   evidence text. Preserve existing error and missing-source semantics.

### C. Real consumer wiring and request context

1. Trace the actual startup and request paths for HTTP, AQL, and YAML-agentic
   consumers. Wire each to the shared typed retrieval provider/contract instead
   of implementing endpoint-local retrieval or materialization.
2. Carry caller authorization, tenant/collection scope, deadline, cancellation,
   budget, and trace context through the affected calls using existing typed
   contexts or narrowly scoped extensions.
3. Shared backend ownership must not imply shared mutable caller identity.
   Concurrent requests with different authorization contexts must remain
   isolated. Never accept credentials or ACLs from untrusted YAML.
4. Keep YAML as orchestration of existing domain operations. Do not introduce a
   second workflow executor or YAML language.

### D. Inference and helper boundaries

- Retrieval must work without a loaded model or GPU. The llama.cpp integration
  consumes already prepared evidence; it does not own document retrieval.
- Keep model lifecycle, GPU selection, flash-attention, KV-cache management,
  tokenization, and generation in the inference provider. These are not
  retrieval helpers and are not part of this task unless a directly caused
  regression must be fixed.
- Helpers remain pure transformations: no storage/index I/O, model calls, or
  retries. `RAGCandidate` remains typed evidence, not a general-purpose row,
  mutation, CDC event, or model-artifact contract.
- Propagate provider failures explicitly. Do not silently fall back to
  ungrounded generation or success-shaped empty data.

### E. Documentation and plan

Update the existing canonical Core/RAG documentation rather than publishing a
second architecture report. Keep implementation status separate from target
design and mark only verified work complete. Include:

- Provider, Consumer, and Helper roles as behaviors of components, not
  exclusive base classes; storage/index modules may both provide and consume.
- An evidence-backed role map for all current direct `src/` directories,
  including exceptions and directories without runtime source.
- The current and target Core composition, ownership/shutdown, trust boundaries,
  request-context flow, and the explicit limits of the interface-DI path.
- The source-identity/materialization contract, legacy compatibility, API/ABI
  impact, and migration requirements.
- A phased roadmap with completed, remaining, and blocked items kept honest;
  synchronize relevant Core architecture, RAG API/README, include-level
  architecture references, and the `[Unreleased]` changelog entry.

## Acceptance criteria

- [ ] Production bootstrap produces a functional retrieval/query path against
  the real RocksDB-backed server state; tests verify successful reads, not only
  constructed objects or non-null pointers.
- [ ] A record written through the normal vector-ingestion sink can be retrieved
  and materialized as its authoritative source text without a test-only
  duplicate relational write.
- [ ] A ContentManager chunk vector resolves to the corresponding chunk text
  and preserves source identity/provenance.
- [ ] Real HTTP, AQL, and YAML integration points given the same authorized
  typed request produce matching evidence IDs, sources, and content before
  consumer-specific prompt formatting. Mock-only proxy tests do not satisfy
  this criterion.
- [ ] Missing sources, wrong collection/namespace, access denial, expired
  deadline, cancellation, and budget exhaustion have explicit outcomes; none
  fabricate evidence or silently generate without grounding.
- [ ] Parallel requests with different tenants/permissions cannot affect one
  another's authorization.
- [ ] Existing relational/node paths and documented legacy reads remain
  compatible. Any on-disk migration is explicit and tested.
- [ ] Retrieval tests run without an LLM/GPU; inference tests use existing
  approved test mechanisms and do not claim GPU/flash-attention validation
  unless those paths were actually exercised.
- [ ] Focused tests are added to existing test targets and registrations.
  Record actual commands/results and any blocked gates. Run applicable LLM
  enabled/disabled and modular/monolithic registration checks.
- [ ] Canonical documentation, role map, roadmap status, API impact, trust
  boundaries, and changelog match the code that was actually delivered.

## Explicitly out of scope

- A 73-module refactor or conversion of every specialized retrieval subsystem.
- New universal Provider/Consumer base classes, a generic `DataResult`, a
  service-locator context, a new RAG manager, or plugin ABI redesign.
- Changes to unrelated llama.cpp GPU/flash-attention behavior, automatic
  self-RAG/FLARE loops, or model lifecycle.
- A new YAML execution language, general Core workflow engine, or unsupported
  CDC/transaction/Exactly-once guarantee.
- Removing domain-specific behavior from `process`, `ethics_ai`, or `search`.
