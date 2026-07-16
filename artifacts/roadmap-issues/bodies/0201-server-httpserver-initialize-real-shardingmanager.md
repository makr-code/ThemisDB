### Context

This issue implements the roadmap item '`HttpServer`: Initialize Real `ShardingManager`' for the server domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `HttpServer`: Initialize Real `ShardingManager`

### Goal

Deliver the scoped changes for `HttpServer`: Initialize Real `ShardingManager` in src/server/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `HttpServer`: Initialize Real `ShardingManager`
**Priority:** Medium
**Target Version:** v1.8.0

`http_server.cpp` line 587: "TODO: Initialize actual `ShardingManager` here when available". Sharding-related admin endpoints (`/v1/admin/shards/*`) are wired but receive a null or stub `ShardingManager`, meaning all shard admin calls silently fail or return empty results.

**Implementation Notes:**
- `[ ]` Inject the live `ShardingManager*` from the `DatabaseServer` construction path into `HttpServer`; remove the TODO and null-check guard.
- `[ ]` Add integration test: create 3 shards via HTTP, verify they appear in `GET /v1/admin/shards`.

---


**Priority:** High  
**Target Version:** v1.7.0

Add GraphQL endpoint alongside REST API for flexible client queries.

**Features:**
- Full GraphQL schema generation from data model
- Query, mutation, subscription support
- DataLoader for batch loading and caching
- Apollo Federation for distributed graphs
- GraphQL Playground for interactive queries

**Implementation:**
```cpp
GraphQLServer gql_server(storage, schema);
gql_server.registerQuery("users", user_resolver);
gql_server.registerMutation("createUser", create_user_resolver);
gql_server.registerSubscription("userChanges", user_subscription_resolver);

// Mount at /graphql endpoint
server.registerHandler("/graphql", gql_server.handler());
```

**Benefits:**
- Clients fetch exactly what they need (no over/under-fetching)
- Single request for complex data requirements
- Strong typing and introspection
- Better mobile app performance

---

### Acceptance Criteria

- [ ] Inject the live `ShardingManager*` from the `DatabaseServer` construction path into `HttpServer`; remove the TODO and null-check guard.
- [ ] Add integration test: create 3 shards via HTTP, verify they appear in `GET /v1/admin/shards`.
- [ ] Full GraphQL schema generation from data model
- [ ] Query, mutation, subscription support
- [ ] DataLoader for batch loading and caching
- [ ] Apollo Federation for distributed graphs
- [ ] GraphQL Playground for interactive queries
- [ ] Clients fetch exactly what they need (no over/under-fetching)
- [ ] Single request for complex data requirements
- [ ] Strong typing and introspection
- [ ] Better mobile app performance

### Relationships

- Roadmap row: #201 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/server/FUTURE_ENHANCEMENTS.md#httpserver-initialize-real-shardingmanager
- Source key: roadmap:201:server:v1.8.0:httpserver-initialize-real-shardingmanager

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:201:server:v1.8.0:httpserver-initialize-real-shardingmanager -->
<!-- roadmap-ref: row=201;module=server;target=v1.8.0 -->
<!-- roadmap-detail: src/server/FUTURE_ENHANCEMENTS.md#httpserver-initialize-real-shardingmanager -->
