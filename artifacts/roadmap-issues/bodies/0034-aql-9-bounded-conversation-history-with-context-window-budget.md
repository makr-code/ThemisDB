### Context

This issue implements the roadmap item 'Bounded Conversation History with Context-Window Budget' for the aql domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.6.0.

Primary detail section: 9 · Bounded Conversation History with Context-Window Budget

### Goal

Deliver the scoped changes for Bounded Conversation History with Context-Window Budget in src/aql/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### 9 · Bounded Conversation History with Context-Window Budget
**Priority:** High
**Target Version:** v1.6.0

**Problem (from code):** `aql_conversation_context.cpp` grows `history_` (`std::vector<llm::ChatMessage>`, line 47) indefinitely with each call to `chat()` (lines 92–101). There is no `max_turns` cap, no token-budget check, and no sliding-window eviction. The `turn_count_` (line 49) is tracked but never compared against any limit. For a long interactive session this means the accumulated context eventually exceeds the model's context window length, causing either silent truncation by the backend or an OOM crash inside the inference engine. The `history_` also has no per-session mutex, making it unsafe to call `chat()` from two threads on the same `AQLConversationContext` object.

**Implementation Notes:**
- `[ ]` Add `std::size_t max_turns = 50` and `std::size_t max_history_tokens = 8192` to `AQLConversationContext::Config` (new struct); enforce in `chat()`: when either limit is reached, evict the oldest user+assistant message pair (preserve the system message)
- `[ ]` Use the `TokenEstimator` abstraction (Feature 5) to count tokens before each `chat()` call; if adding the new user message would exceed `max_history_tokens`, evict oldest pairs first
- `[ ]` Add a `std::mutex history_mutex_` to `AQLConversationContext::Impl` and hold it around all reads/writes to `history_` and `turn_count_`
- `[ ]` Expose `AQLConversationContext::tokenCount() const` so callers can observe current usage
- `[ ]` Unit-test: create a context with `max_turns=3`, drive 5 turns, assert `turn_count() == 3` and `history_.size() == 7` (system + 3×(user+assistant))

---

### Acceptance Criteria

- [ ] Add `std::size_t max_turns = 50` and `std::size_t max_history_tokens = 8192` to `AQLConversationContext::Config` (new struct); enforce in `chat()`: when either limit is reached, evict the oldest user+assistant message pair (preserve the system message)
- [ ] Use the `TokenEstimator` abstraction (Feature 5) to count tokens before each `chat()` call; if adding the new user message would exceed `max_history_tokens`, evict oldest pairs first
- [ ] Add a `std::mutex history_mutex_` to `AQLConversationContext::Impl` and hold it around all reads/writes to `history_` and `turn_count_`
- [ ] Expose `AQLConversationContext::tokenCount() const` so callers can observe current usage
- [ ] Unit-test: create a context with `max_turns=3`, drive 5 turns, assert `turn_count() == 3` and `history_.size() == 7` (system + 3×(user+assistant))

### Relationships

- Roadmap row: #34 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#9--bounded-conversation-history-with-context-window-budget
- Source key: roadmap:34:aql:v1.6.0:9-bounded-conversation-history-with-context-window-budget

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:34:aql:v1.6.0:9-bounded-conversation-history-with-context-window-budget -->
<!-- roadmap-ref: row=34;module=aql;target=v1.6.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#9--bounded-conversation-history-with-context-window-budget -->
