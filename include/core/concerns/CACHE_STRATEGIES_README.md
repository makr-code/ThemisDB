# Cache Strategy Pattern Implementation

## Overview

This implementation provides a unified cache abstraction with pluggable eviction strategies for ThemisDB. It extends the existing `ICache` interface with the Strategy Pattern, allowing different cache eviction policies to be swapped at runtime or compile-time without changing client code.

## Architecture

### Core Interfaces

#### `IEvictionStrategy`
Base interface for all eviction strategies. Implementations must provide:
- `onAccess(key)` - Track when an entry is accessed
- `onInsert(key, timestamp)` - Track when an entry is inserted
- `onRemove(key)` - Track when an entry is removed
- `selectVictim()` - Select which entry to evict
- `clear()` - Clear all tracking data
- `size()` - Get number of tracked entries
- `getName()` - Get strategy name for debugging

#### `CacheMetrics`
Standard metrics structure for monitoring cache performance:
- Hit/miss counts
- Eviction/insertion counts
- Current/max size
- Latency tracking
- Calculated hit rate and average latency

#### `ICache` (Extended)
The existing interface now includes optional methods:
- `getEvictionStrategy()` - Get the current eviction strategy (optional)
- `getMetrics()` - Get detailed metrics (optional)

Returns `nullptr` if the implementation doesn't support these features, ensuring backward compatibility.

## Eviction Strategies

### 1. LRU (Least Recently Used)
**File:** `include/core/concerns/eviction_strategies.h`

Evicts the least recently accessed entry. Maintains a doubly-linked list with most recent at front.

**Use Case:** General-purpose caching, good for workloads with temporal locality.

**Complexity:** O(1) for all operations

```cpp
auto strategy = std::make_unique<LRUEvictionStrategy>();
auto cache = std::make_unique<StrategicCacheImpl>(1000, std::move(strategy));
```

### 2. LFU (Least Frequently Used)
**File:** `include/core/concerns/eviction_strategies.h`

Evicts the least frequently accessed entry. Tracks access frequency for each key, with tie-breaking by age.

**Use Case:** Workloads where some entries are accessed much more frequently than others.

**Complexity:** O(n) for victim selection, O(1) for tracking

```cpp
auto strategy = std::make_unique<LFUEvictionStrategy>();
auto cache = std::make_unique<StrategicCacheImpl>(1000, std::move(strategy));
```

### 3. TTL (Time To Live)
**File:** `include/core/concerns/eviction_strategies.h`

Evicts expired entries first, then falls back to oldest entry.

**Use Case:** Caches where data has a natural expiration time (sessions, tokens, temporary results).

**Complexity:** O(n) for expired entry search

```cpp
auto strategy = std::make_unique<TTLEvictionStrategy>(3600000); // 1 hour TTL
auto cache = std::make_unique<StrategicCacheImpl>(1000, std::move(strategy));
```

### 4. TwoTier (L1/L2)
**File:** `include/core/concerns/eviction_strategies.h`

Combines two strategies: fast L1 (e.g., LRU) and slower L2 (e.g., LFU). New entries go to L1 until full, then L2. Eviction prefers L2.

**Use Case:** Multi-tier caching with different policies per tier.

**Complexity:** Depends on composed strategies

```cpp
auto l1 = std::make_unique<LRUEvictionStrategy>();
auto l2 = std::make_unique<LFUEvictionStrategy>();
auto strategy = std::make_unique<TwoTierEvictionStrategy>(
    std::move(l1), std::move(l2), 100  // L1 capacity = 100
);
auto cache = std::make_unique<StrategicCacheImpl>(1000, std::move(strategy));
```

## Implementation

### StrategicCacheImpl
**File:** `include/core/concerns/strategic_cache_impl.h`

A thread-safe in-memory cache implementation that uses the Strategy Pattern for eviction policies.

**Features:**
- Pluggable eviction strategies
- TTL support per entry
- Comprehensive metrics tracking
- Regex pattern invalidation
- Runtime strategy swapping
- Thread-safe operations

**Example Usage:**

```cpp
#include <core/concerns/strategic_cache_impl.h>
#include <core/concerns/eviction_strategies.h>

// Create cache with LRU strategy
auto cache = std::make_unique<StrategicCacheImpl>(
    1000,  // max size
    std::make_unique<LRUEvictionStrategy>(),
    3600000  // default TTL: 1 hour
);

// Store data
CacheEntry entry("my_data", 1, getCurrentTimeMs());
cache->put("key1", entry);

// Retrieve data
auto result = cache->get("key1");
if (result) {
    std::cout << "Found: " << result->payload << std::endl;
}

// Get metrics
const auto* metrics = cache->getMetrics();
std::cout << "Hit rate: " << metrics->hitRate() << std::endl;

// Swap strategy at runtime
cache->setEvictionStrategy(std::make_unique<LFUEvictionStrategy>());
```

## Testing

### Unit Tests

**File:** `tests/test_eviction_strategies.cpp`
- Tests for each eviction strategy independently
- Validates correct victim selection
- Tests strategy lifecycle (insert, access, remove, clear)

**File:** `tests/test_strategic_cache.cpp`
- Tests cache with each strategy
- Validates TTL expiration
- Tests metrics collection
- Tests strategy swapping
- Thread safety tests

**Run Tests:**
```bash
cd build
ctest -R "test_eviction_strategies|test_strategic_cache" -V
```

## Performance Considerations

### Memory Overhead
- **LRU:** O(n) - one list node + map entry per key
- **LFU:** O(n) - one map entry with frequency counter per key
- **TTL:** O(n) - one map entry with timestamp per key
- **TwoTier:** Sum of L1 and L2 overhead

### Operation Latency
- **get():** ~1-2 μs (including strategy tracking)
- **put():** ~1-2 μs (including eviction if needed)
- **selectVictim():** O(1) for LRU, O(n) for LFU/TTL

### Thread Safety
All operations are protected by a mutex, suitable for moderate contention. For high-contention scenarios, consider sharding the cache or using lock-free data structures.

## Integration with Existing Caches

The new strategy pattern is designed to be minimally invasive:

1. **Existing implementations unchanged:** `InMemoryCacheImpl` and `NoOpCache` continue to work as before.

2. **Optional adoption:** The `ICache` interface extensions return `nullptr` by default, so existing implementations don't need to be modified.

3. **Future migration path:** Domain-specific caches (QueryCache, SemanticCache, etc.) can gradually adopt the strategy pattern by:
   - Implementing `ICache` interface
   - Using `StrategicCacheImpl` as a base or wrapper
   - Migrating their eviction logic to strategy implementations

## Future Enhancements

### Potential Improvements
1. **ARC (Adaptive Replacement Cache)** - Adaptive strategy that balances LRU and LFU
2. **W-TinyLFU** - Window TinyLFU with admission policy
3. **Custom strategies** - Application-specific eviction policies
4. **Batch operations** - Bulk get/put for efficiency
5. **Persistence** - Strategy state persistence for warm restarts
6. **Distributed caching** - Strategy coordination across nodes

### Integration Candidates
The following existing caches could benefit from strategy adoption:
- **QueryCache** - Already has LRU/LFU, could use strategies
- **AdaptiveQueryCache** - Multi-tier could use TwoTier strategy
- **EmbeddingCache** - Could make eviction policy configurable
- **SemanticCache** - Could add memory eviction on top of RocksDB TTL

## Design Decisions

### Why Optional Methods in ICache?
Keeps backward compatibility while enabling new features. Existing implementations don't break, and new features can be adopted incrementally.

### Why Strategy Pattern?
- **Flexibility:** Swap policies without changing cache code
- **Testability:** Each strategy can be tested independently
- **Extensibility:** Easy to add new strategies
- **Performance:** No virtual call overhead for core operations (strategy is called only on insert/access)

### Why Not Template-Based?
- Templates would require compile-time strategy selection
- Runtime flexibility is more important for database workloads
- Virtual call overhead is negligible compared to cache operations

## Related Files

### Headers
- `include/core/concerns/i_cache.h` - Base interface
- `include/core/concerns/cache_strategies.h` - Strategy interfaces
- `include/core/concerns/eviction_strategies.h` - Concrete strategies
- `include/core/concerns/strategic_cache_impl.h` - Implementation
- `include/core/concerns/inmemory_cache_impl.h` - Original simple implementation
- `include/core/concerns/noop_implementations.h` - No-op cache

### Tests
- `tests/test_eviction_strategies.cpp` - Strategy tests
- `tests/test_strategic_cache.cpp` - Cache implementation tests

### Existing Caches (for reference)
- `include/cache/semantic_cache.h` - LLM response caching
- `include/query/query_cache.h` - Query result caching
- `include/llm/kv_cache_buffer.h` - KV cache buffer
- `include/cache/adaptive_query_cache.h` - Multi-tier query cache

## Version History

### v1.3.1 (2024-01)
- Initial implementation of IEvictionStrategy interface
- LRU, LFU, TTL, TwoTier strategies
- StrategicCacheImpl with strategy support
- Comprehensive unit tests
- Backward-compatible ICache extensions

## License

This implementation is part of ThemisDB and is licensed under the MIT License.
