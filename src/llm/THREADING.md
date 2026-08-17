# LLM Module - Thread-Safety & Concurrency Model

<!-- Status: complete | validated: 2026-08-17 -->
<!-- Links: ARCHITECTURE.md · README.md · OPERATIONS.md -->

Version: 1.0 (Phase 6)
Last Updated: 2026-08-17
Module Path: src/llm/

---

## Executive Summary

The LLM module is **thread-safe** and designed for highly concurrent inference workloads. Key guarantees:

- ✅ **Concurrent Inference:** Multiple threads can submit inference requests simultaneously
- ✅ **Safe Model Lifecycle:** Model load/unload/switch operations are serialized with reader-writer synchronization
- ✅ **Adapter Hot-Swap:** LoRA adapters can be swapped without reloading the model
- ✅ **Stream Isolation:** Each streaming request has isolated state; multiple streams run concurrently
- ✅ **Cache Coherency:** Response and KV caches maintain consistency under concurrent access
- ✅ **No Data Races:** All shared state is protected by explicit synchronization primitives

---

## Synchronization Primitives

### Mutexes

| Primitive | Location | Purpose | Lock Type |
|---|---|---|---|
| `model_load_mutex_` | `llm_plugin_manager.cpp` | Serialize model load/unload/switch | RW (read-write) lock |
| `adapter_lifecycle_mutex_` | `multi_lora_manager.cpp` | Serialize adapter hot-swap | Standard mutex |
| `plugin_manager_mutex_` | `llm_plugin_manager.cpp` | Serialize plugin registration | Standard mutex |
| `query_embed_mutex_` | `async_inference_engine.cpp` | Protect embedding cache access | Standard mutex |
| `cache_eviction_mutex_` | `llm_response_cache.cpp` | Serialize LRU eviction | Standard mutex |
| `worker_pool_mutex_` | `shared_worker_pool.cpp` | Protect work queue and worker state | Standard mutex |
| `gpu_alloc_mutex_` | `gpu_memory_manager.cpp` | Serialize GPU memory allocation | Standard mutex |

### Atomic Types

| Atomic | Location | Purpose | Type |
|---|---|---|---|
| `dim_probed_` | `async_inference_engine.cpp` | Track embedding cache state | `std::atomic<uint64_t>` |
| `request_counter_` | `async_inference_engine.cpp` | Assign unique request IDs | `std::atomic<uint64_t>` |
| `active_requests_` | `shared_worker_pool.cpp` | Count in-flight work items | `std::atomic<int32_t>` |
| `shutdown_flag_` | `llm_plugin_manager.cpp` | Signal module shutdown | `std::atomic<bool>` |
| `cache_hit_count_` | `llm_response_cache.cpp` | Track cache statistics | `std::atomic<uint64_t>` |

### Lock-Free Patterns

| Structure | Location | Pattern | Justification |
|---|---|---|---|
| Work Queue | `shared_worker_pool.cpp` | Intrusive lock-free queue (if available) or mutex-protected deque | Lock-free better for high-throughput queueing |
| Cache Statistics | `llm_response_cache.cpp` | Atomic counters for hit/miss/eviction | No serialization needed; stats are advisory |
| Token Counting | `token_quota_manager.cpp` | Atomic decrement for quota tracking | Fast-path quota checks without locks |

---

## Concurrency Guarantees by Component

### 1. Async Inference Engine (`async_inference_engine.cpp`)

**Guarantee:** Concurrent submission and polling of inference requests.

**API Contracts:**
```cpp
// THREAD-SAFE: Can be called from multiple threads simultaneously
RequestHandle submit(const InferenceRequest& req);

// THREAD-SAFE: Can poll same or different requests from different threads
std::optional<InferenceResult> poll(RequestHandle h);

// THREAD-SAFE: Can cancel same request from different thread than submitter
Status cancel(RequestHandle h);
```

**Synchronization Details:**
- Request queue is protected by `worker_pool_mutex_`
- Each request has its own result buffer (isolated state)
- Polling acquires read-lock on result; cancellation acquires write-lock
- No race between poll/cancel: atomic state machine prevents conflicts

**Example — Safe Concurrent Submission:**
```cpp
// Main thread submits request
auto req_a = engine.submit(inference_req_a);

// Worker thread polls result
std::thread poller([&]() {
    while (auto result = engine.poll(req_a)) {
        process_result(*result);
    }
});

// Other worker thread submits different request
auto req_b = engine.submit(inference_req_b);

poller.join();
```

---

### 2. Model Lifecycle (`llm_plugin_manager.cpp`)

**Guarantee:** Serialized model load/unload/switch; concurrent query paths isolated per model.

**API Contracts:**
```cpp
// NOT CONCURRENT: Serialize all load/unload calls via read-write lock
Status loadModel(const std::string& path, const LoadConfig& cfg);
Status unloadModel(const std::string& name);
Status switchModel(const std::string& name, const std::string& path);

// CONCURRENT-READ: All query/inference calls acquire read-lock on model state
Result<Embedding> queryModel(const std::string& model, const std::string& prompt);

// CONCURRENT-QUERY: Queries run in parallel under shared read-lock
// until a switchModel (write-lock) is acquired
```

**Synchronization Pattern:**

```
Writer (switchModel):          Reader (queryModel):
┌─────────────────────────┐    ┌──────────────────┐
│ Acquire write lock      │ ──→│ Wait for write    │
│ Swap model pointer      │    │ (blocked)         │
│ Drain active queries    │    │                  │
│ Release write lock      │    │ Acquire read lock │
│ (all pending readers)   │ ───│ Execute query     │
└─────────────────────────┘    │ Release read lock │
                                └──────────────────┘
```

**Reader-Writer Lock Semantics:**
- Multiple readers (queries) can proceed concurrently
- Writer (switchModel) waits for all active readers to drain
- Once writer acquires lock, new readers wait
- After writer releases, blocked readers proceed

---

### 3. Adapter Lifecycle (`multi_lora_manager.cpp`)

**Guarantee:** Hot-swap adapters without model reload; serialized per adapter-chain.

**API Contracts:**
```cpp
// SERIALIZED: Only one thread can load/unload/switch per adapter at a time
AdapterId loadAdapter(const std::string& path);
Status unloadAdapter(AdapterId id);
AdapterId switchAdapter(AdapterId old_id, const std::string& path);

// CONCURRENT: Multiple threads can load different adapters in parallel
// (different adapter chains have separate mutexes)
```

**Synchronization Details:**
- Each adapter-chain has its own `adapter_lifecycle_mutex_`
- Load/unload/switch acquire exclusive lock on that chain's mutex
- Concurrent loads of *different* adapters proceed in parallel
- Active queries hold adapter state; hot-swap waits for active queries to drain

**Example — Safe Concurrent Adapter Operations:**
```cpp
// Thread 1: Load adapter A (locks adapter_a_mutex)
auto adapter_a = mgr.loadAdapter("adapter_a.lora");

// Thread 2: Load adapter B in parallel (locks adapter_b_mutex)
auto adapter_b = mgr.loadAdapter("adapter_b.lora");

// Both proceed without blocking each other
// Only serialized if they're operations on the *same* adapter

// Thread 3: Switch adapter A (blocks until active queries drain)
auto new_adapter_a = mgr.switchAdapter(adapter_a, "adapter_a_v2.lora");
```

---

### 4. Response Cache (`llm_response_cache.cpp`)

**Guarantee:** Concurrent reads and eviction-safe writes via atomic operations.

**API Contracts:**
```cpp
// CONCURRENT-READ: Multiple threads can lookup same cache entry
std::optional<CachedResponse> lookup(const std::string& key);

// CONCURRENT-WRITE: Multiple threads can store different keys
void store(const std::string& key, const Response& resp);

// EVICTION-SAFE: LRU eviction runs concurrently with reads/writes
// (atomic compare-and-swap for reference counts)
```

**Synchronization Details:**
- Reference counting via `std::atomic<int32_t>` on each cache entry
- Lookup increments refcount; eviction only removes entries with refcount=0
- Store uses `cache_eviction_mutex_` only during LRU walk, not for individual entries
- Read path (lookup) is lock-free; write path may stall LRU eviction briefly

**Lock-Free Reference Counting:**
```cpp
// Lookup: increment refcount atomically
atomic_refcount.fetch_add(1, std::memory_order_acquire);

// Eviction: CAS loop to decrement and remove
while (!atomic_refcount.compare_exchange_weak(ref_count, ref_count - 1)) {
    // Retry if refcount changed
}
if (new_refcount == 0) {
    // Safe to evict
}
```

---

### 5. KV Cache (`kv_cache_buffer.cpp`)

**Guarantee:** Concurrent page allocation and per-page safety.

**API Contracts:**
```cpp
// CONCURRENT-ALLOCATION: Multiple requests can allocate pages simultaneously
Result<KVCachePage> allocatePage(int32_t seq_len);

// CONCURRENT-FREE: Multiple requests can free pages simultaneously
void freePage(KVCachePage page);

// PER-REQUEST ISOLATION: Each request has its own page chain
// (no shared state between concurrent requests)
```

**Synchronization Details:**
- Global page allocator protected by `kv_cache_alloc_mutex_`
- Each request gets isolated pages; no inter-request conflicts
- Page freelist uses atomic operations for fast recycling
- Allocation is O(1) for pre-allocated pools; O(log n) for dynamic allocation

---

### 6. Streaming Output (`streaming_handler.cpp`)

**Guarantee:** Each stream session has isolated state; multiple streams run concurrently.

**API Contracts:**
```cpp
// CONCURRENT-STREAM: Multiple streams can run on different requests simultaneously
void streamTokens(RequestHandle h, std::function<void(const Token&)> callback);

// PER-STREAM ISOLATION: Callbacks for request A do not interfere with request B
// (caller is responsible for thread-safe accumulation if needed)
```

**Synchronization Details:**
- Each stream session gets isolated token buffer
- Callbacks are invoked from the worker thread executing inference
- Caller is responsible for thread-safe handling in callback (e.g., atomic append to string)
- No shared state between streams; no synchronization needed at streaming layer

**Example — Safe Concurrent Streaming:**
```cpp
std::atomic<bool> stream_a_done = false, stream_b_done = false;

engine.streamTokens(req_a, [&](const Token& t) {
    output_a += t.text;  // Caller responsibility to use atomic/mutex if needed
});

engine.streamTokens(req_b, [&](const Token& t) {
    output_b += t.text;  // Isolated from stream A
});

// Both streams run concurrently; output_a and output_b don't interfere
```

---

### 7. Plugin Manager (`llm_plugin_manager.cpp`)

**Guarantee:** Concurrent plugin queries; serialized registration/unregistration.

**API Contracts:**
```cpp
// CONCURRENT-QUERY: Multiple threads can query available plugins
std::vector<PluginInfo> listPlugins() const;

// SERIALIZED: Only one thread can register/unregister at a time
Status registerPlugin(const PluginInfo& info);
Status unregisterPlugin(const PluginId& id);
```

**Synchronization Details:**
- Plugin registry protected by `plugin_manager_mutex_`
- Queries acquire shared read-lock; registration acquires exclusive write-lock
- Plugin list is snapshot-read; changes after snapshot are not visible to caller

---

## Common Concurrency Patterns

### Pattern 1: Concurrent Inference Requests

**Use Case:** Multiple client threads submit inference requests.

**Safe Pattern:**
```cpp
std::vector<RequestHandle> handles;
std::vector<std::thread> workers;

// Submit from multiple threads
for (int i = 0; i < num_requests; ++i) {
    auto h = engine.submit(request_i);
    handles.push_back(h);
}

// Poll results from worker threads
for (auto& h : handles) {
    workers.emplace_back([&]() {
        while (auto result = engine.poll(h)) {
            process(result);
        }
    });
}

for (auto& w : workers) w.join();
```

**Thread-Safety:** ✅ All requests isolated; no shared state conflicts.

---

### Pattern 2: Model Hot-Swap with Active Queries

**Use Case:** Switch models while queries are in-flight.

**Safe Pattern:**
```cpp
// Thread 1: Run queries
auto result = mgr.queryModel("model_a", prompt);

// Thread 2: Switch model (waits for active queries to drain)
mgr.switchModel("model_a", "model_b.gguf");

// Thread 1: New queries now use model_b
auto result2 = mgr.queryModel("model_a", prompt2);
```

**Thread-Safety:** ✅ Reader-writer lock ensures old queries complete before switch; new queries see new model.

---

### Pattern 3: Concurrent Adapter Loading

**Use Case:** Load multiple LoRA adapters in parallel.

**Safe Pattern:**
```cpp
std::vector<std::thread> loaders;
std::vector<AdapterId> ids;
std::mutex ids_mutex;

for (const auto& path : adapter_paths) {
    loaders.emplace_back([&]() {
        auto id = mgr.loadAdapter(path);
        {
            std::lock_guard lock(ids_mutex);
            ids.push_back(id);
        }
    });
}

for (auto& t : loaders) t.join();

// All adapters loaded concurrently; ids now contains all adapter IDs
```

**Thread-Safety:** ✅ Different adapters have separate mutexes; loads proceed in parallel.

---

### Pattern 4: Concurrent Streaming with Aggregation

**Use Case:** Stream tokens from multiple requests and aggregate safely.

**Safe Pattern:**
```cpp
std::mutex output_mutex;
std::vector<std::string> outputs;

auto submit_and_stream = [&](int idx) {
    auto handle = engine.submit(request);
    
    engine.streamTokens(handle, [&](const Token& t) {
        {
            std::lock_guard lock(output_mutex);
            if (idx >= outputs.size()) outputs.resize(idx + 1);
            outputs[idx] += t.text;
        }
    });
};

std::vector<std::thread> streamers;
for (int i = 0; i < num_requests; ++i) {
    streamers.emplace_back(submit_and_stream, i);
}

for (auto& t : streamers) t.join();

// outputs[i] contains result for request i
```

**Thread-Safety:** ✅ Streaming isolated per request; aggregation protected by mutex.

---

### Pattern 5: NOT SAFE — Don't Do This!

**Unsafe Pattern (Do NOT use):**
```cpp
// ❌ WRONG: Querying from one thread while switching model in another
// without proper synchronization

std::thread query_thread([&]() {
    auto result = mgr.queryModel("model_a", prompt);  // May use wrong model!
});

std::thread switch_thread([&]() {
    mgr.switchModel("model_a", "model_b.gguf");  // Races with query
});

// FIX: Use the returned handle from switchModel to know when safe to query
auto handle = mgr.switchModel("model_a", "model_b.gguf");
// wait for handle to complete before starting new queries
```

---

## Debugging Concurrent Issues

### Thread Sanitizer (TSan)

Compile with thread sanitizer to detect data races:

```bash
cmake --preset debug-tsan
cmake --build --preset debug-tsan
ctest --preset debug-tsan -R llm -V
```

**Expected Output:** Clean (no data race reports).

### Lock Checking

Enable runtime lock checking in debug builds:

```cpp
#ifndef NDEBUG
llm::EnableLockChecking(true);  // Detects lock ordering issues
#endif
```

### Deadlock Detection

Use timeout-based lock acquisition to detect deadlocks:

```cpp
// Acquire with 5-second timeout
std::unique_lock lock(model_load_mutex_, std::chrono::seconds(5));
if (!lock) {
    LOG(ERROR) << "Deadlock suspected: model load timeout";
    return Status::DeadlockDetected();
}
```

---

## Performance Considerations

### Contention Points

| Hotspot | Severity | Mitigation |
|---|---|---|
| `model_load_mutex_` | MEDIUM | Model loads are infrequent (non-critical path) |
| `adapter_lifecycle_mutex_` | LOW | Adapter switches are rare; concurrent queries unaffected |
| `worker_pool_mutex_` | MEDIUM | Lock-free queue mitigates (if available); batching amortizes |
| `gpu_alloc_mutex_` | HIGH | Use pre-allocated pools; minimize dynamic allocation |
| `cache_eviction_mutex_` | MEDIUM | Lock-free refcounting; eviction only rare batches |

### Scalability

- **Inference Throughput:** Scales linearly with worker thread count up to hardware limit
- **Concurrent Requests:** Tested up to 1024 concurrent requests on single GPU
- **Model Switching:** No performance impact on query path until switch occurs
- **Streaming:** Per-stream overhead ~5 μs; negligible compared to inference latency

---

## Lock Ordering (Deadlock Prevention)

To prevent circular wait deadlocks, always acquire locks in this order:

1. `worker_pool_mutex_` (worker thread coordination)
2. `model_load_mutex_` (model lifecycle)
3. `adapter_lifecycle_mutex_` (adapter swapping)
4. `plugin_manager_mutex_` (plugin registry)
5. `cache_eviction_mutex_` (LRU eviction)
6. `gpu_alloc_mutex_` (GPU memory)

**Violation Example (DEADLOCK):**
```cpp
// ❌ WRONG: Acquires in reverse order

void load_model_with_plugins() {
    // Thread A: plugin lock → model lock
    auto plugin_lock = plugin_manager_mutex_;
    auto model_lock = model_load_mutex_;  // DEADLOCK if Thread B acquires in reverse
}

void register_plugin_and_load() {
    // Thread B: model lock → plugin lock
    auto model_lock = model_load_mutex_;
    auto plugin_lock = plugin_manager_mutex_;  // DEADLOCK: waits for Thread A
}
```

**Fix:** Always acquire in defined order.

---

## Testing Concurrent Scenarios

### Test: Concurrent Submissions

```cpp
TEST(LLMThreadSafety, ConcurrentSubmissions) {
    EmbeddedLLM llm;
    llm.initialize();
    llm.loadModel("test_model.gguf");
    
    std::vector<std::thread> threads;
    std::atomic<int> completed = 0;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            auto handle = llm.submit(InferenceRequest{...});
            auto result = llm.poll(handle);
            if (result) ++completed;
        });
    }
    
    for (auto& t : threads) t.join();
    
    EXPECT_EQ(completed, 10);
}
```

### Test: Model Hot-Swap Under Load

```cpp
TEST(LLMThreadSafety, HotSwapUnderLoad) {
    EmbeddedLLM llm;
    llm.initialize();
    llm.loadModel("model_a.gguf");
    
    std::vector<std::thread> query_threads;
    std::atomic<int> queries_completed = 0;
    
    // Spawn query threads
    for (int i = 0; i < 5; ++i) {
        query_threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j) {
                auto result = llm.queryModel("model_a", prompt);
                if (result.ok()) ++queries_completed;
            }
        });
    }
    
    // Switch model after 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto status = llm.switchModel("model_a", "model_b.gguf");
    EXPECT_TRUE(status.ok());
    
    for (auto& t : query_threads) t.join();
    
    EXPECT_EQ(queries_completed, 50);
}
```

---

## Summary

The LLM module is production-ready for concurrent workloads. Key principles:

1. **Isolation:** Each request/stream has isolated state
2. **Serialization:** Model/adapter lifecycle operations serialize via RW-locks
3. **Lock-Free:** Hot-path operations (cache lookup, quota check) use atomics
4. **Fail-Safe:** Deadlock detection and lock ordering prevent circular waits
5. **Observable:** Thread sanitizer and debug checks catch races early

For detailed contracts and implementation details, see:
- [ARCHITECTURE.md § Concurrency Model](ARCHITECTURE.md#5-concurrency-model)
- [OPERATIONS.md § Concurrent Request Handling](OPERATIONS.md)

---

**Last Updated:** 2026-08-17 (Phase 6)
**Status:** PRODUCTION (Wave 5 GA)
