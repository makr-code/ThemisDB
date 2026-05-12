# ThemisDB GraphQL and AQL: A Dual-Layer Query Architecture for Multi-Model Data Systems

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-05-12  
**Target Venue**: arXiv cs.DB / cs.SE  
**Scope**: Query architecture, API design, resolver pushdown, schema mapping, and AI-assisted query planning

## Abstract

ThemisDB exposes two complementary query languages: GraphQL as a client-facing API layer and AQL as the database-native multi-model execution language. In conventional systems, these layers are often treated as interchangeable frontends. We argue that ThemisDB benefits from a stricter separation of concerns: GraphQL should optimise developer ergonomics, schema introspection, and external integration, while AQL should retain full expressive power, cost-based optimisation, and engine-native execution semantics.

This paper-style draft formalises that separation as a dual-layer query architecture. We analyse how GraphQL requests can be mapped onto AQL plans through resolver pushdown, schema projection, and cost-aware routing. We further identify synergy points where both languages can share metadata, observability, caching, and AI-assisted tooling without collapsing into a single abstraction. The result is a pragmatic architecture that preserves external simplicity while keeping execution close to storage, indexing, and multi-model operators.

**Keywords**: GraphQL, AQL, multi-model databases, query planning, resolver pushdown, schema mapping, database-native AI, cost-based optimisation

## 1. Introduction

ThemisDB combines a GraphQL API surface with a database-native AQL execution layer. This creates an important architectural question: should GraphQL merely mirror AQL, or should the two languages be treated as distinct layers with different optimisation goals?

Our answer is that they should remain distinct. GraphQL is best suited for external consumers: it gives clients explicit field selection, schema introspection, and typed access patterns. AQL is better suited for execution: it expresses traversals, joins, vector search, LLM calls, and multi-model data access in a form the engine can optimise directly.

The practical implication is that GraphQL should not be a second database language competing with AQL. Instead, GraphQL should be viewed as a declarative contract that can be compiled into AQL or into other engine-native execution plans where appropriate. This preserves usability without sacrificing performance.

### Contributions

1. We define a dual-layer model for ThemisDB in which GraphQL and AQL occupy different architectural roles.
2. We describe a resolver-pushdown pipeline that translates GraphQL selections into AQL-friendly execution units.
3. We identify shared infrastructure for metadata, caching, observability, and AI-assisted tooling.
4. We discuss limitations, including schema drift, pushdown boundaries, and cross-model cost estimation.

## 2. System Context

ThemisDB already separates its query-facing subsystems:

- `api/` provides the GraphQL API and related transport handlers.
- `aql/` defines the AQL grammar, helper functions, and query-language assets.
- `query/` contains the parser, optimiser, and execution engine.
- `llm/`, `rag/`, `prompt_engineering/`, and `ethics_ai/` add AI-specific query and evaluation paths.

The key design property is that GraphQL is external-facing, while AQL is execution-facing. A GraphQL request may be accepted from a client, mapped into one or more AQL fragments, and then executed under the database's native planner and storage operators.

## 3. GraphQL as an External Contract

GraphQL solves the problem of client usability. It offers:

- precise field selection,
- strong schema introspection,
- typed client generation,
- a single endpoint model,
- nested selection trees for object graphs.

For ThemisDB, this is especially useful when external systems need to query documents, graph structures, vector search results, or AI-related metadata without learning AQL.

GraphQL should therefore be treated as a stable contract layer. It defines what a caller may ask for, not how the engine should execute the request.

## 4. AQL as the Execution Language

AQL is the natural internal language for ThemisDB because it is multi-model and engine-aware. It can express:

- relational selection, filtering, aggregation, and joins,
- graph traversals and shortest-path style operators,
- document access on flexible JSON-like structures,
- vector similarity search,
- time-series access,
- LLM extensions such as inference, embeddings, RAG, and model management.

The advantage of AQL is that it can be optimised by the query engine using storage-aware and model-aware strategies. Unlike GraphQL, AQL does not need to remain client-friendly; it needs to remain executable, optimisable, and composable.

## 5. Dual-Layer Architecture

We model ThemisDB as a pipeline with three stages:

1. **GraphQL parsing and validation**
2. **Resolver and schema mapping**
3. **AQL planning and execution**

In this model, GraphQL handles developer intent, while AQL handles execution strategy.

### 5.1 Resolver Pushdown

The most important synergy point is resolver pushdown. A GraphQL resolver can translate:

- field selections into projection lists,
- filter arguments into AQL predicates,
- pagination into LIMIT/OFFSET style execution,
- graph-shaped nested selections into traversal operators,
- vector search arguments into similarity operators,
- AI-related fields into LLM-aware AQL extensions.

This reduces duplicated business logic and makes GraphQL less of an application-layer translation burden.

### 5.2 Schema Projection

GraphQL schemas should be derived from database metadata where possible:

- collections and document types,
- graph nodes and edge descriptors,
- vector index metadata,
- time-series capabilities,
- AI-enabled fields and query extensions.

This approach helps prevent divergence between the API contract and the storage model.

### 5.3 Cost-Aware Routing

Not every GraphQL request should be translated in the same way. Some requests are cheap to push down; others may require partial evaluation in the resolver layer. A cost-aware planner can evaluate:

- traversal depth,
- join fan-out,
- vector search selectivity,
- post-processing cost,
- AI-assisted enrichment overhead.

The goal is to select the cheapest correct execution path, not to force every request into the same translation template.

## 6. Shared Infrastructure and Synergies

The two languages should share infrastructure where that improves correctness or maintainability.

### 6.1 Metadata

GraphQL and AQL can consume the same schema and capability metadata. This allows the system to answer questions such as:

- Which models are supported?
- Which fields are traversable?
- Which indices are available?
- Which AI extensions can be pushed down?

### 6.2 Caching

Query result caching, embedding caching, and resolver result caching can be unified across the two layers. For example:

- GraphQL selection caching can reuse AQL result fragments.
- AQL subplans can share cached intermediate state.
- AI-assisted query suggestions can cache schema-derived hints.

### 6.3 Observability

Both layers should emit comparable metrics:

- parse latency,
- pushdown rate,
- AQL execution latency,
- cache hit rate,
- resolver fan-out,
- failure mode classification.

This enables traceability from external API call to internal execution plan.

### 6.4 AI-Assisted Query Support

The GraphQL/AQL split also creates a natural entry point for ML features:

- query suggestion and completion,
- GraphQL-to-AQL synthesis,
- execution-plan explanations,
- schema-aware prompt generation,
- retrieval assistance for query debugging.

This aligns with ThemisDB's broader AI subsystem design.

## 7. Discussion

The dual-layer model has three practical advantages.

First, it preserves API ergonomics. External users can remain in GraphQL while internal execution benefits from AQL expressiveness.

Second, it preserves optimisation freedom. AQL can continue to evolve as the database-native execution language without being constrained by GraphQL semantics.

Third, it keeps AI integration local to the engine. Query assistance, RAG support, and plan explanation can operate near the data and the planner instead of being distributed across separate services.

However, the architecture only works if translation boundaries are explicit. If GraphQL becomes a leaky mirror of AQL, the system risks duplicating semantics in two places. If AQL becomes the public contract, the API becomes less accessible to external consumers.

## 8. Limitations

This draft assumes that:

- GraphQL schema generation remains synchronised with database metadata,
- resolver pushdown can be implemented without semantic loss,
- AQL retains sufficient expressiveness for internal execution,
- cost estimation is good enough to choose between pushdown strategies.

Open problems include:

- handling schema drift under frequent model evolution,
- estimating the cost of nested GraphQL projections over multi-model data,
- defining safe pushdown boundaries for AI-assisted features,
- supporting subscriptions and changefeeds without excessive backpressure.

## 9. Related Work

This draft is grounded in the ThemisDB documentation corpus rather than external literature alone. The most relevant internal sources are:

1. The GraphQL API specification and resolver documentation.
2. The AQL grammar and query-engine documentation.
3. The system architecture documents that describe the separation of `api/`, `aql/`, and `query/`.
4. AI-oriented research drafts that connect query planning with RAG, LLM serving, and prompt engineering.

From a broader database perspective, the design is related to work on API/query separation, schema projection, and cost-based query compilation. For an arXiv submission, those external comparisons should be added in the next revision together with a formal bibliography.

## 10. Conclusion

GraphQL and AQL are complementary in ThemisDB. GraphQL improves usability and integration; AQL provides execution power and optimisation freedom. The best architecture is therefore not a merger but a staged pipeline that compiles GraphQL intent into AQL-aware execution paths.

This separation supports both product quality and research direction: ThemisDB can remain easy to integrate while still exploiting engine-native optimisation, AI-assisted query tooling, and multi-model execution semantics.

## References

[1] ThemisDB GraphQL API Specification, internal documentation.

[2] ThemisDB GraphQL API documentation, internal documentation.

[3] ThemisDB AQL README and grammar reference, internal documentation.

[4] ThemisDB architecture overview, internal documentation.

[5] Query planning and AI integration drafts in the ThemisDB research corpus.
