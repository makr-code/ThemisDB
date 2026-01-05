# PagedAttention Integration in Continuous Batch Scheduler

## Overview

The Continuous Batch Scheduler integrates with PagedAttention to enable efficient memory management for KV cache in LLM inference workloads. This document describes the implementation, architecture, and usage of the PagedAttention integration.

## Architecture

### Components

1. **ContinuousBatchScheduler**: Main scheduler managing concurrent inference requests
2. **PagedKVCache**: Manages KV cache using block-based memory allocation
3. **PagedBlockManager**: Handles physical block allocation and deallocation
4. **BlockTable**: Maps logical sequence positions to physical blocks

### Data Flow

```
Request Submission → Availability Check → Block Allocation → Inference → Block Deallocation
```

## Block Management

### Block Availability Check

Before scheduling a request, the scheduler checks if sufficient blocks are available:

```cpp
bool canAddToBatch(const ScheduledRequest* request, size_t current_batch_tokens) const {
    if (kv_cache_) {
        // Calculate blocks needed using ceiling division
        size_t total_tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
        size_t blocks_needed = (total_tokens + config_.block_size_tokens - 1) / config_.block_size_tokens;
        
        auto stats = kv_cache_->getStats();
        if (stats.blocks_free < blocks_needed) {
            return false;  // Insufficient memory
        }
    }
    return true;
}
```

**Key Features:**
- Prevents out-of-memory scenarios
- Uses ceiling division for accurate block count
- Configurable block size via `block_size_tokens`

### Block Allocation

Blocks are allocated when a request enters the prefill phase:

```cpp
void allocateKVCacheBlocks(ScheduledRequest* request) {
    // Calculate blocks needed
    size_t tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
    size_t blocks_needed = (tokens + config_.block_size_tokens - 1) / config_.block_size_tokens;
    
    // Get or create block table
    auto block_table = kv_cache_->getBlockTable(request->sequence_id);
    if (block_table) {
        // Allocate through block table
        auto allocated = block_table->allocateBlocks(blocks_needed);
        request->allocated_blocks = allocated;
    } else {
        // Reserve placeholders for consistency
        request->allocated_blocks.reserve(blocks_needed);
        for (size_t i = 0; i < blocks_needed; ++i) {
            request->allocated_blocks.push_back(-1);
        }
    }
}
```

**Key Features:**
- Lazy allocation: BlockTable created on first KV store
- Placeholder tracking maintains consistency with availability checks
- Efficient block reservation

### Block Deallocation

Blocks are freed when a request completes or is cancelled:

```cpp
void freeKVCacheBlocks(ScheduledRequest* request) {
    if (kv_cache_ && !request->allocated_blocks.empty()) {
        // Remove sequence from KV cache
        kv_cache_->removeSequence(request->sequence_id);
        request->allocated_blocks.clear();
    }
}
```

**Key Features:**
- Automatic cleanup on completion/cancellation
- Releases all blocks for a sequence atomically
- Updates block availability immediately

## Performance Metrics

### Time to First Token (TTFT)

Measures latency from request start to first generated token:

```cpp
// Accurate TTFT calculation
auto ttft = std::chrono::duration_cast<std::chrono::milliseconds>(
    req->first_token_at - req->started_at
).count();
```

**Implementation:**
- Uses dedicated `first_token_at` timestamp
- Captures exact moment first token is generated
- Excludes prefill phase for decode-only metrics

### Tokens Per Second (TPS)

Measures generation throughput excluding prefill:

```cpp
// Pure generation throughput (decode phase only)
if (req->tokens_generated > 1) {
    auto generation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        req->last_token_at - req->first_token_at
    );
    double tps = tokens_generated / (generation_time.count() / 1000.0);
}
```

**Implementation:**
- Calculates time from first to last token
- Excludes prefill phase for accurate generation rate
- Applies memory pressure backpressure factor

### Memory Pressure Adjustment

Throughput estimates are adjusted based on available memory:

```cpp
if (kv_stats.blocks_free < config_.low_memory_threshold_blocks) {
    stats_.avg_tokens_per_second *= config_.memory_pressure_throughput_factor;
}
```

## Configuration

### Scheduler Configuration

```cpp
ContinuousBatchScheduler::SchedulerConfig config;

// Memory management
config.block_size_tokens = 16;  // Must match PagedKVCache config
config.low_memory_threshold_blocks = 10;  // Memory pressure trigger
config.memory_pressure_throughput_factor = 0.8;  // Throughput reduction under pressure

// Batch configuration
config.max_batch_size = 256;
config.max_concurrent_requests = 128;
config.max_tokens_per_batch = 8192;

// Scheduling policy
config.enable_preemption = true;
config.enable_chunked_prefill = true;
config.prefill_chunk_size = 512;
```

### PagedKVCache Configuration

```cpp
PagedKVCache::Config cache_config;
cache_config.block_size = 16;  // Must match scheduler block_size_tokens
cache_config.num_blocks = 4096;
cache_config.num_layers = 32;
cache_config.head_dim = 128;
cache_config.num_kv_heads = 8;
cache_config.enable_prefix_caching = true;
```

## Usage Example

### Basic Setup

```cpp
// Create block manager
PagedBlockManager::Config bm_config;
bm_config.total_blocks = 4096;
bm_config.block_size_tokens = 16;
auto block_manager = std::make_shared<PagedBlockManager>(bm_config);

// Create KV cache
PagedKVCache::Config cache_config;
cache_config.num_blocks = 4096;
cache_config.block_size = 16;
auto kv_cache = std::make_unique<PagedKVCache>(cache_config, block_manager);

// Create scheduler
ContinuousBatchScheduler::SchedulerConfig sched_config;
sched_config.block_size_tokens = 16;  // Must match
auto scheduler = std::make_unique<ContinuousBatchScheduler>(sched_config, kv_cache.get());

scheduler->start();
```

### Request Submission

```cpp
// Create inference request
InferenceRequest req;
req.prompt = "Once upon a time";
req.max_tokens = 100;
req.temperature = 0.7f;

// Submit with callback
auto request_id = scheduler->submitRequest(req, 
    ContinuousBatchScheduler::RequestPriority::NORMAL,
    [](const InferenceResponse& resp) {
        std::cout << "Generated: " << resp.text << std::endl;
    }
);
```

### Batch Processing

```cpp
// Main inference loop
while (scheduler->isRunning()) {
    // Schedule next batch
    auto batch = scheduler->scheduleNextBatch();
    
    if (!batch.empty()) {
        // Run inference on batch
        std::vector<InferenceResponse> responses = runInference(batch);
        
        // Process results
        scheduler->processBatchResults(batch, responses);
    }
    
    // Check statistics
    auto stats = scheduler->getStats();
    std::cout << "Active: " << stats.active_requests << std::endl;
    std::cout << "TTFT: " << stats.avg_time_to_first_token_ms << "ms" << std::endl;
    std::cout << "TPS: " << stats.avg_tokens_per_second << std::endl;
}
```

## Performance Characteristics

### Memory Efficiency

- **Block-based allocation**: ~95% memory utilization vs. contiguous allocation
- **Copy-on-Write**: Efficient prefix sharing for similar prompts
- **Dynamic allocation**: No pre-allocation overhead

### Throughput

- **Continuous batching**: Mix prefill and decode in same batch
- **Memory pressure awareness**: Prevents OOM thrashing
- **Priority scheduling**: High-priority requests processed first

### Latency

- **TTFT optimization**: Accurate measurement and optimization
- **Chunked prefill**: Large prompts don't block small requests
- **Preemption support**: Low-priority requests can be paused

## Testing

### Unit Tests

Located in `tests/test_continuous_batch_scheduler.cpp`:

1. **BlockAllocationDeallocation**: Verifies allocation/deallocation lifecycle
2. **BlockAvailabilityCheck**: Ensures OOM prevention
3. **MultipleBatchRequests**: Tests concurrent request handling
4. **RequestCompletionBlockDeallocation**: Validates cleanup
5. **StatisticsUpdate**: Checks metric accuracy
6. **OutOfMemoryHandling**: Tests memory exhaustion scenarios
7. **PrioritySchedulingWithBlocks**: Verifies priority ordering

### Running Tests

```bash
# Build tests
cmake --build build --target test_continuous_batch_scheduler

# Run tests
./build/tests/test_continuous_batch_scheduler

# Run with verbose output
./build/tests/test_continuous_batch_scheduler --gtest_verbose
```

## Troubleshooting

### Out of Memory Errors

**Symptom**: Requests stuck in waiting queue, no scheduling progress

**Diagnosis**:
```cpp
auto stats = scheduler->getStats();
auto kv_stats = kv_cache->getStats();
std::cout << "Free blocks: " << kv_stats.blocks_free << std::endl;
std::cout << "Waiting requests: " << stats.total_requests - stats.active_requests << std::endl;
```

**Solutions**:
1. Increase `num_blocks` in PagedKVCache config
2. Reduce `max_tokens` in requests
3. Enable preemption to free blocks from low-priority requests

### Low Throughput

**Symptom**: `avg_tokens_per_second` below expected

**Diagnosis**:
```cpp
auto stats = scheduler->getStats();
auto kv_stats = kv_cache->getStats();
std::cout << "Batch size: " << stats.current_batch_size << std::endl;
std::cout << "Free blocks: " << kv_stats.blocks_free << std::endl;
```

**Solutions**:
1. Increase `max_batch_size` if blocks available
2. Adjust `memory_pressure_throughput_factor` (currently reduces by 20%)
3. Enable `enable_chunked_prefill` for better batch mixing

### High TTFT

**Symptom**: `avg_time_to_first_token_ms` too high

**Diagnosis**:
```cpp
auto stats = scheduler->getStats();
std::cout << "TTFT: " << stats.avg_time_to_first_token_ms << "ms" << std::endl;
std::cout << "Active requests: " << stats.active_requests << std::endl;
```

**Solutions**:
1. Enable `enable_priority_scheduling` for important requests
2. Reduce `prefill_chunk_size` for faster initial response
3. Increase `max_concurrent_requests` to reduce queuing

## Implementation Notes

### Block Size Consistency

**Critical**: `block_size_tokens` must match between:
- `ContinuousBatchScheduler::SchedulerConfig`
- `PagedKVCache::Config`
- `PagedBlockManager::Config`

Mismatch causes incorrect availability calculations and potential OOM.

### Thread Safety

All public methods are thread-safe via internal mutex. However:
- Avoid holding external locks when calling scheduler methods
- Callbacks execute on scheduler thread - keep them lightweight

### Memory Ordering

Block operations follow strict ordering:
1. Availability check (read lock)
2. Allocation (write lock)
3. Usage (no lock on blocks)
4. Deallocation (write lock)

This ensures consistency without deadlocks.

## Future Enhancements

### Planned Features

1. **Block Prefetching**: Pre-allocate blocks for likely next requests
2. **Intelligent Eviction**: Smart selection of which requests to preempt
3. **Cross-Sequence Sharing**: Share identical prefixes across requests
4. **Adaptive Block Size**: Dynamic block size based on workload

### Performance Optimizations

1. **Lock-free availability check**: Use atomic counters
2. **Batch allocation**: Allocate multiple request blocks together
3. **Lazy deallocation**: Defer freeing until memory pressure

## References

- [vLLM PagedAttention Paper](https://arxiv.org/abs/2309.06180)
- [Continuous Batching in LLM Inference](https://www.anyscale.com/blog/continuous-batching-llm-inference)
- `include/llm/paged_kv_cache.h` - PagedKVCache interface
- `include/llm/continuous_batch_scheduler.h` - Scheduler interface
- `PRODUCTION_READINESS_REVIEW.md` - Production requirements

## Support

For issues or questions:
1. Check troubleshooting section above
2. Review unit tests for usage examples
3. Check logs with `spdlog::set_level(spdlog::level::debug)`
4. File issue on GitHub with reproduction steps
