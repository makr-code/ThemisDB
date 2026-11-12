---
Author: makr-code
Created: 2026-11-11
Last Updated: 2026-11-12
Status: draft
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: makr-code
---

> Exact model backend not verifiable from final document text alone.

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
  New writes also persist chunk text in the dedicated relational
  `ingestion_sources` collection keyed by the exact `VectorRecord::chunk_id`.
  The vector entity points to that exact key and retains the source-file SHA-256
  as provenance. RAG resolves the record through one exact RocksDB lookup and
  validates the stored source-file identity before replacing the copied snippet.
  Missing, mismatched, or empty source records are rejected. Existing vectors
  written with the older `ingestion_vector` reference retain their prior snippet
  behavior; the new collection is additive and requires no key migration. The
  sink now requires a `SecondaryIndexManager` sharing the RAG retrieval database,
  which is a source/ABI change for its callers.
- ContentManager stores chunk text under `chunk:<id>` and its vector entity under
  `chunks:<id>`. The current work puts the chunk text and an exact
  `source_ref_key=chunk:<id>` on that vector entity, with `chunks` as the source
  collection and `content_id` retained as the owner identity in the vector
  entity. QueryEngine only hydrates a vector entity from its
  vector key when the initialized vector-index namespace matches the query
  collection; canonical relational and legacy table-key reads remain earlier
  lookup paths. The current RAG provider additionally resolves typed
  `content_chunk` references by one exact `chunk:<id>` RocksDB read and verifies
  stored chunk ID, owner `content_id`, and any supplied source version;
  missing/mismatched/stale sources are rejected rather than replaced by the
  vector's copied text. This private bridge adds no public API or layout change.
  Normal ingestion now has the equivalent exact-key source dereference through
  `ingestion_sources`, validating `source_file_id` against vector provenance.
- `RAGSourceReference` is an optional typed locator on `RAGCandidate`.
  `makeRAGCandidate` parses complete, nonempty source-reference type, collection,
  and key fields and materializes text from `content`, `text`, `body`, or
  `text_snippet`; the helper performs no source lookup.
- `LLMAQLHandler::executeRAG()` passes its timeout manager cancellation token to
  `RAGProvider`. It now also passes a per-execution deadline derived from the
  configured AQL RAG timeout; both checks remain cooperative around QueryEngine
  calls and cannot interrupt a blocking backend operation. It also passes the
  configured collection checker as a request-scoped authorization predicate.
- The direct HTTP `/api/v1/llm/rag` route checks `data:read`/`data.read` before
  constructing its request-local `QueryEngine`; that engine now rejects
  collections other than the one authorized by the route. The route also passes
  the authorized collection as a request-scoped retrieval predicate. This guards
  the collection boundary. A validated bearer-token tenant claim is also applied
  as a `tenant_id` equality filter through the shared retrieval execution
  context; caller-supplied tenant headers are ignored. The provider enforces this
  tenant for materialized evidence and adds it to vector query predicates. This
  tenant filter does not provide general row-level policy or propagate caller
  identity through the retrieval context to other consumers.
- `search`, `LayeredRetrievalOrchestrator`, `ProcessGraphRag`, and
  `ethics_ai/RAGContextEngine` have specialized behavior. Preserve their domain
  policies; this task does not migrate all of them.

Recheck these facts against the current source, not generated wiki summaries.
Update or narrow this task if source evidence differs before coding.

## Verified incremental work in the current implementation diff

The MCP server now retains the graph index manager for the lifetime of the
`QueryEngine` that borrows it, and a focused MCP AQL graph traversal regression
covers the attached query path, including database reattachment after graph
data is written. Reattachment releases the old query/index borrowers before
replacing their database. This fixes that specific lifetime defect; it does not
complete server-wide provider composition or MCP retrieval authorization.

The direct HTTP RAG route now carries its configured live request timeout into
`RetrievalExecutionContext` from request dispatch through query embedding and
provider retrieval; an expired retrieval deadline returns HTTP 504. The deadline
is cooperative and cannot interrupt a blocking embedding or backend call. The
route now also builds an equality filter from the validated bearer-token tenant
claim for vector retrieval; it does not trust a caller-supplied tenant header.
The claim is passed through `RetrievalExecutionContext`; the provider filters
materialized relational, content, and vector evidence, and injects the tenant
predicate before vector search. Tenant-scoped graph retrieval now fails closed
before traversal: QueryEngine currently enforces collection policy before graph
vertex expansion, but does not apply row-level tenant policy there. This does not
populate trusted tenant context for AQL/YAML server consumers, establish general
row-level policy, propagate caller identity, or add HTTP cancellation. Focused
provider regressions cover foreign/missing tenants, indexed top-1 behavior, and
tenant-scoped graph denial, but have not been built or executed. Adding the
context field is source-compatible for ordinary member assignment/default
construction but changes the struct layout; binary consumers must rebuild.

The current source diff adds `RAGSourceReference` and candidate parsing for exact
source-reference fields; persists normal ingestion chunk text in
`ingestion_sources` and resolves it by exact key plus source-file identity;
stores ContentManager chunk text and its exact `chunk:<id>` locator on the
corresponding vector entity; gates vector-key hydration on an exact initialized
vector-index/query namespace match; propagates the AQL RAG timeout
as a cooperative retrieval deadline; and supports a per-call collection
authorization predicate, wired from the AQL checker and direct HTTP route.
QueryEngine applies that predicate before graph vertex loading and expansion.
ContentManager `content_chunk` references now resolve exact source records and
reject missing, mismatched, or stale chunk sources. Focused test-source changes
cover source materialization, including a sink-only vector write that retrieves
persisted source text without a test-only duplicate write; source corruption;
matching-provenance source records with whitespace-only text;
deadline expiry after a blocking QueryEngine call with both matching and empty
results; cancellation after a blocking QueryEngine call with no matches; an
incomplete/malformed persisted ingestion-source record returning
`ERR_STORAGE_CORRUPTION`, covering both invalid JSON and a well-formed source
record with an array, null, or string root; missing required content; or
identity/content stored with non-string types, plus a truncated object header,
truncated field-name payload, unsupported type tag, wrong binary root container,
trailing bytes after a valid binary object, and truncated required source text;
and request-isolated collection authorization. These C++ tests have not been
built or executed.
For YAML-configured agentic modes, `AIOrchestrator::runAgentic()` now routes
retrieval-enabled modes through its existing `runRag()` pipeline before
preserving the existing tool-call parsing and dispatch. This reuses the typed
provider and the per-call `RetrievalExecutionContext`; it adds no workflow
executor and does not populate authorization from YAML.
The MCP `llm_orchestrate` bridge currently constructs its context from tool
arguments without a trusted caller authorization context; retrieval-enabled MCP
modes now fail closed before invoking the orchestrator. Wiring authenticated
request context remains required for MCP retrieval.
The focused target has not been built or executed: diagnostic CMake configuration
stopped at the required `fmt` dependency, which is not installed in the current
environment.

This is partial implementation progress, not completion of this task or its
acceptance criteria. Normal ingestion source rows contain the canonical chunk
text carried by `VectorRecord`; this source is validated against the originating
file SHA-256, while no independent source-file/version record is available at
the sink boundary. Existing vectors with the old `ingestion_vector` marker
retain their prior snippet behavior. The new sink constructor requires callers
to provide a `SecondaryIndexManager` on the same retrieval database.

Still pending: trace and verify real HTTP/AQL/YAML startup and request wiring;
complete production composition; complete caller/tenant/row-level authorization
and full context propagation (the direct HTTP vector filter is route-specific and
does not establish general row-level policy);
verify application startup configures the orchestrator with the actual server
QueryEngine and supplies trusted authorization context, including a safe MCP
caller-context path; establish the
evidence-backed role map for all direct `src/` directories; complete source
materialization/compatibility and remaining acceptance cases; and run and record
the required focused tests and registration checks. Keep every acceptance
checkbox below open until its criterion is independently verified.

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
