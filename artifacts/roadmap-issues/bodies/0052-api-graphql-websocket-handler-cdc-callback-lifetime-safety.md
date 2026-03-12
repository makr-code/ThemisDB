### Context

This issue implements the roadmap item 'GraphQL WebSocket Handler — CDC Callback Lifetime Safety' for the api domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: GraphQL WebSocket Handler — CDC Callback Lifetime Safety

### Goal

Deliver the scoped changes for GraphQL WebSocket Handler — CDC Callback Lifetime Safety in src/api/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### GraphQL WebSocket Handler — CDC Callback Lifetime Safety
**Priority:** High
**Target Version:** v1.8.0

`graphql_ws_handler.cpp::handleSubscribe()` captures a raw `GraphQLWsHandler*` (`self`) pointer inside the CDC callback lambda that is passed to `Changefeed::subscribe()`. The `SubscriptionHandle` RAII type should cancel the subscription on destruction, but the safety of this interaction depends on CDC correctly serialising the callback teardown before the handle destructor returns.

**Implementation Notes:**
- `[ ]` **Raw `self` pointer captured in CDC callback** (`graphql_ws_handler.cpp::handleSubscribe()`): the lambda `[self, sub_id](const themis::Changefeed::ChangeEvent& ev) { ... std::lock_guard<std::mutex> lk(self->mutex_); self->pending_frames_.push_back(frame); }` is invoked by the CDC system on its own thread. If the CDC implementation allows callbacks to fire after `SubscriptionHandle` destruction (even briefly), this is a use-after-free. Add a `std::shared_ptr<std::atomic<bool>>` "alive" flag shared between the handler and the lambda; the lambda checks it before dereferencing `self`, and the flag is set to false in `GraphQLWsHandler::reset()` before subscriptions are cleared.
- `[ ]` **Missing step-2 in `handleSubscribe()` comment sequence** (`graphql_ws_handler.cpp`): the comment block labels "step 1" (reject duplicate IDs + enforce max_subscriptions) and "step 3" (parse payload), with no "step 2". This indicates a planned intermediate validation step (likely query variable type-checking against the schema) was omitted. Add schema-level argument type validation: verify that `variables` provided in the payload match the declared `VariableDefinition` types in the parsed operation before registering the subscription.

**Performance Targets:**
- Zero use-after-free races under 10,000 concurrent subscription setup/teardown cycles.

---

### Acceptance Criteria

- [ ] **Raw `self` pointer captured in CDC callback** (`graphql_ws_handler.cpp::handleSubscribe()`): the lambda `[self, sub_id](const themis::Changefeed::ChangeEvent& ev) { ... std::lock_guard<std::mutex> lk(self->mutex_); self->pending_frames_.push_back(frame); }` is invoked by the CDC system on its own thread. If the CDC implementation allows callbacks to fire after `SubscriptionHandle` destruction (even briefly), this is a use-after-free. Add a `std::shared_ptr<std::atomic<bool>>` "alive" flag shared between the handler and the lambda; the lambda checks it before dereferencing `self`, and the flag is set to false in `GraphQLWsHandler::reset()` before subscriptions are cleared.
- [ ] **Missing step-2 in `handleSubscribe()` comment sequence** (`graphql_ws_handler.cpp`): the comment block labels "step 1" (reject duplicate IDs + enforce max_subscriptions) and "step 3" (parse payload), with no "step 2". This indicates a planned intermediate validation step (likely query variable type-checking against the schema) was omitted. Add schema-level argument type validation: verify that `variables` provided in the payload match the declared `VariableDefinition` types in the parsed operation before registering the subscription.
- [ ] Zero use-after-free races under 10,000 concurrent subscription setup/teardown cycles.

### Relationships

- Roadmap row: #52 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#graphql-websocket-handler--cdc-callback-lifetime-safety
- Source key: roadmap:52:api:v1.8.0:graphql-websocket-handler-cdc-callback-lifetime-safety

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:52:api:v1.8.0:graphql-websocket-handler-cdc-callback-lifetime-safety -->
<!-- roadmap-ref: row=52;module=api;target=v1.8.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#graphql-websocket-handler--cdc-callback-lifetime-safety -->
