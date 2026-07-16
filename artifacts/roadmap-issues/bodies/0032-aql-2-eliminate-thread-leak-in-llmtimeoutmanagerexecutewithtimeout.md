### Context

This issue implements the roadmap item 'Eliminate Thread Leak in `LLMTimeoutManager::executeWithTimeout()`' for the aql domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.6.0.

Primary detail section: 2 · Eliminate Thread Leak in `LLMTimeoutManager::executeWithTimeout()`

### Goal

Deliver the scoped changes for Eliminate Thread Leak in `LLMTimeoutManager::executeWithTimeout()` in src/aql/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### 2 · Eliminate Thread Leak in `LLMTimeoutManager::executeWithTimeout()`
**Priority:** High
**Target Version:** v1.6.0

**Problem (from code):** `include/aql/llm_timeout_manager.h:executeWithTimeout()` (line ~90) calls `worker.detach()` when the timeout fires. The comment on that line explicitly acknowledges: *"the worker thread is detached and may continue executing"*. A detached thread holds all resources it has captured by reference or value and cannot be joined. Under sustained load a burst of LLM timeouts will accumulate many detached threads, each one consuming a stack (~8 MB default on Linux) and holding a reference to the plugin manager. The `executeWithCancelToken()` variant sets the cancel token before detaching but still has the same thread-leak problem if the worker ignores the token.

**Implementation Notes:**
- `[ ]` Replace the `std::thread` + `std::packaged_task` approach in `executeWithTimeout()` with `std::jthread` (C++20) and a `std::stop_token`; `jthread::request_stop()` signals the token and the destructor joins automatically — no detach needed
- `[ ]` Where C++20 is unavailable, use an `std::atomic<bool>` shutdown flag combined with a `std::future::wait_for()` loop that joins on expiry rather than detaching
- `[ ]` Add a test asserting that after `executeWithTimeout()` throws `TIMEOUT`, the associated worker thread has terminated within `timeout + 500 ms` (use a latch decremented by the worker on exit)
- `[ ]` Document in the `TimeoutConfig` struct that `infer_timeout{300}`, `rag_timeout{600}`, `embed_timeout{60}`, and `model_load_timeout{900}` are soft defaults and show how to override them via `LLMTimeoutManager::setConfig()`

**Performance Targets:**
- Zero leaked threads after 1 000 sequential timeout events in the test suite

---

### Acceptance Criteria

- [ ] Replace the `std::thread` + `std::packaged_task` approach in `executeWithTimeout()` with `std::jthread` (C++20) and a `std::stop_token`; `jthread::request_stop()` signals the token and the destructor joins automatically — no detach needed
- [ ] Where C++20 is unavailable, use an `std::atomic<bool>` shutdown flag combined with a `std::future::wait_for()` loop that joins on expiry rather than detaching
- [ ] Add a test asserting that after `executeWithTimeout()` throws `TIMEOUT`, the associated worker thread has terminated within `timeout + 500 ms` (use a latch decremented by the worker on exit)
- [ ] Document in the `TimeoutConfig` struct that `infer_timeout{300}`, `rag_timeout{600}`, `embed_timeout{60}`, and `model_load_timeout{900}` are soft defaults and show how to override them via `LLMTimeoutManager::setConfig()`
- [ ] Zero leaked threads after 1 000 sequential timeout events in the test suite

### Relationships

- Roadmap row: #32 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#2--eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout
- Source key: roadmap:32:aql:v1.6.0:2-eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:32:aql:v1.6.0:2-eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout -->
<!-- roadmap-ref: row=32;module=aql;target=v1.6.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#2--eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout -->
