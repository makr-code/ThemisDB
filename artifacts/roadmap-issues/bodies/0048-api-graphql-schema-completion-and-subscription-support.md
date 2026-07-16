### Context

This issue implements the roadmap item 'GraphQL Schema Completion and Subscription Support' for the api domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: GraphQL Schema Completion and Subscription Support

### Goal

Deliver the scoped changes for GraphQL Schema Completion and Subscription Support in src/api/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### GraphQL Schema Completion and Subscription Support
**Priority:** High
**Target Version:** v1.7.0

`graphql.cpp` implements a full parser and query executor but lacks mutation resolvers, schema introspection (`__schema`, `__type`), and subscription over WebSocket. Complete the schema to cover documents, graph edges, vector search, and geospatial queries; add `subscription` operation support backed by `cdc::Changefeed`.

**Implementation Notes:**
- `[x]` Add `SchemaRegistry` class to `graphql.cpp`; auto-build from registered `TypeDefinition` objects at server start.
- `[x]` Implement `__schema` and `__type` introspection resolvers; required by all major GraphQL clients (Apollo, Relay).
- `[x]` Subscription transport: use Boost.Beast WebSocket upgrades; create `graphql_ws_handler.cpp` implementing the `graphql-transport-ws` protocol (not the legacy `subscriptions-transport-ws`).
- `[x]` Wire `cdc::Changefeed::subscribe(filter)` as the event source for `subscription { onChange(collection: "...") { ... } }`. Implemented: `Changefeed::subscribe(SubscriptionFilter, SubscriptionCallback)` + `SubscriptionHandle` RAII type in `changefeed.h/cpp`; wired in `GraphQLWsHandler::handleSubscribe()` via `extractOnChangeCollection()`.
- `[x]` Enforce `QueryLimits::maxSubscriptions` per connection to prevent fan-out DoS.
- `[ ]` In `graphql.h`, the `Parser` class explicitly documents "Not yet supported: Fragments, Directives, Inline fragments." Implement `parseFragmentDefinition()` and `parseInlineFragment()` in `graphql.cpp` — without fragment support, clients using Apollo's automatic persisted query fragments or any relay-style fragment composition will fail at parse time.
- `[ ]` `graphql.h::Parser::error()` is documented as **deprecated** ("Deprecated: Use `Result<T>` return types instead of `error()` method") but the method still exists in the class definition. Remove it after migrating all call sites in `graphql.cpp` to return `themis::Result<T>` with structured `ParseError` objects to eliminate the dual error-reporting path.
- `[ ]` `Schema::introspect()` in `graphql.cpp` only handles `__schema` and `__type` fields. The GraphQL June 2018 spec also requires `__typename` on every composite type, `__Field`, `__InputValue`, `__EnumValue`, and `__Directive` meta-types. Add these to `Schema::introspect()` so introspection-based tooling (code generators, schema diffing tools) works fully.
- `[ ]` `Executor::executeSelections()` in `graphql.cpp` resolves fields serially in a range-for loop. For independent sibling fields that each invoke storage I/O, this means sequential round-trips. Add parallel field resolution via `std::async` or a small task graph; guard behind a `QueryLimits::parallel_fields_enabled` flag to allow gradual rollout.

**Performance Targets:**
- GraphQL parse + validate + execute for a 10-field document query in < 2 ms (p99) under 500 concurrent HTTP/2 connections.
- Subscription event delivery latency < 50 ms from `Changefeed` event emission to WebSocket frame sent.

**API Sketch:**
```graphql
# New subscription type (graphql.cpp SchemaRegistry)
type Subscription {
  onChange(collection: String!, filter: ChangeFilter): ChangeEvent!
}

type ChangeEvent {
  sequence: Int!
  type: ChangeType!
  key: String!
  document: JSON
  timestampMs: Int!
}
```

---

### Acceptance Criteria

- [ ] Add `SchemaRegistry` class to `graphql.cpp`; auto-build from registered `TypeDefinition` objects at server start.
- [ ] Implement `__schema` and `__type` introspection resolvers; required by all major GraphQL clients (Apollo, Relay).
- [ ] Subscription transport: use Boost.Beast WebSocket upgrades; create `graphql_ws_handler.cpp` implementing the `graphql-transport-ws` protocol (not the legacy `subscriptions-transport-ws`).
- [ ] Wire `cdc::Changefeed::subscribe(filter)` as the event source for `subscription { onChange(collection: "...") { ... } }`. Implemented: `Changefeed::subscribe(SubscriptionFilter, SubscriptionCallback)` + `SubscriptionHandle` RAII type in `changefeed.h/cpp`; wired in `GraphQLWsHandler::handleSubscribe()` via `extractOnChangeCollection()`.
- [ ] Enforce `QueryLimits::maxSubscriptions` per connection to prevent fan-out DoS.
- [ ] In `graphql.h`, the `Parser` class explicitly documents "Not yet supported: Fragments, Directives, Inline fragments." Implement `parseFragmentDefinition()` and `parseInlineFragment()` in `graphql.cpp` — without fragment support, clients using Apollo's automatic persisted query fragments or any relay-style fragment composition will fail at parse time.
- [ ] `graphql.h::Parser::error()` is documented as **deprecated** ("Deprecated: Use `Result<T>` return types instead of `error()` method") but the method still exists in the class definition. Remove it after migrating all call sites in `graphql.cpp` to return `themis::Result<T>` with structured `ParseError` objects to eliminate the dual error-reporting path.
- [ ] `Schema::introspect()` in `graphql.cpp` only handles `__schema` and `__type` fields. The GraphQL June 2018 spec also requires `__typename` on every composite type, `__Field`, `__InputValue`, `__EnumValue`, and `__Directive` meta-types. Add these to `Schema::introspect()` so introspection-based tooling (code generators, schema diffing tools) works fully.
- [ ] `Executor::executeSelections()` in `graphql.cpp` resolves fields serially in a range-for loop. For independent sibling fields that each invoke storage I/O, this means sequential round-trips. Add parallel field resolution via `std::async` or a small task graph; guard behind a `QueryLimits::parallel_fields_enabled` flag to allow gradual rollout.
- [ ] GraphQL parse + validate + execute for a 10-field document query in < 2 ms (p99) under 500 concurrent HTTP/2 connections.
- [ ] Subscription event delivery latency < 50 ms from `Changefeed` event emission to WebSocket frame sent.

### Relationships

- Roadmap row: #48 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#graphql-schema-completion-and-subscription-support
- Source key: roadmap:48:api:v1.7.0:graphql-schema-completion-and-subscription-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:48:api:v1.7.0:graphql-schema-completion-and-subscription-support -->
<!-- roadmap-ref: row=48;module=api;target=v1.7.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#graphql-schema-completion-and-subscription-support -->
