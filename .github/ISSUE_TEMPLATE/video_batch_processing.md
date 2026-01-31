---
name: Video Batch Processing
about: Efficient batch processing for multiple videos
title: '[VIDEO] Batch Processing: Process multiple videos efficiently'
labels: enhancement, content-processing, video, performance
assignees: ''
---

## Feature Description
Add batch processing support to the video processor for efficiently processing multiple videos concurrently with shared resources, connection pooling, and optimized throughput.

## Problem Statement
Currently, processing multiple videos requires:
- Sequential processing (one at a time)
- Repeated initialization overhead
- No resource sharing between operations
- Inefficient use of CPU/GPU resources
- No progress tracking for batch operations

For enterprise scenarios with thousands of videos:
- Sequential: 100 videos × 500ms = 50 seconds
- With batching: 100 videos / 8 threads = 6.25 seconds (8x faster)

## Proposed Solution
Add batch processing API with:

### 1. **Batch Interface**
```cpp
struct BatchResult {
    std::string id;                          // Video identifier
    ContentExtractionResult result;          // Extraction result
    std::exception_ptr error;                // Error if failed
};

class VideoProcessorBatch {
public:
    // Add videos to batch
    void addVideo(const std::string& id, const std::vector<uint8_t>& blob, 
                  const std::string& mime_type);
    
    // Process all videos
    std::vector<BatchResult> process(int max_concurrent = 4);
    
    // Progress callback
    void setProgressCallback(std::function<void(int completed, int total)> callback);
};
```

### 2. **Resource Pooling**
- Reuse FFmpeg contexts across videos
- Thread pool for concurrent processing
- Shared temporary file directory
- GPU resource pooling (if hardware acceleration enabled)

### 3. **Optimization Strategies**
- **Parallel Processing**: Process N videos concurrently
- **Prefetching**: Load next video while processing current
- **Adaptive Concurrency**: Adjust based on CPU/GPU utilization
- **Priority Queue**: Process smaller videos first

## Alternative Solutions
1. **Application-level parallelism**: Let users handle concurrency
   - Pros: Simple, flexible
   - Cons: Users must manage threading, no optimization

2. **Message Queue (RabbitMQ/Kafka)**: Distributed video processing
   - Pros: Highly scalable
   - Cons: Infrastructure overhead, complexity

3. **GPU Batch Decoding**: Batch decode on GPU
   - Pros: Very fast for supported codecs
   - Cons: Requires hardware acceleration, limited codec support

4. **MapReduce Framework**: Use Hadoop/Spark
   - Pros: Industry standard for big data
   - Cons: Heavy infrastructure, overkill for single-node

## Use Case
**Enterprise Video Library Ingestion:**
```cpp
// Batch configuration
PluginConfig config;
config.set("batch.max_concurrent", 8);        // 8 concurrent videos
config.set("batch.prefetch", true);           // Prefetch next videos
config.set("batch.timeout_ms", 60000);        // 60 second timeout per video
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

VideoProcessorBatch batch(config);

// Add videos to batch
for (const auto& video : video_library) {
    batch.addVideo(video.id, video.data, "video/mp4");
}

// Progress tracking
batch.setProgressCallback([](int completed, int total) {
    std::cout << "Progress: " << completed << "/" << total << "\n";
});

// Process all videos
auto results = batch.process();

// Handle results
for (const auto& result : results) {
    if (result.error) {
        std::cerr << "Failed: " << result.id << "\n";
    } else {
        database.insert(result.id, result.result);
    }
}
```

## Example Usage
```cpp
// Create batch processor
themis::content::PluginConfig config;
config.set("batch.max_concurrent", 8);
config.set("batch.queue_size", 100);
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

themis::content::VideoProcessorBatch batch(config);

// Add videos
std::vector<std::string> video_ids = {"video1", "video2", "video3", ...};
for (const auto& id : video_ids) {
    auto video_data = loadVideo(id);
    batch.addVideo(id, video_data, "video/mp4");
}

// Optional: Progress callback
std::atomic<int> completed{0};
batch.setProgressCallback([&](int done, int total) {
    completed = done;
    std::cout << "\rProcessing: " << done << "/" << total << std::flush;
});

// Process all videos concurrently
auto results = batch.process(8);  // Use 8 threads

std::cout << "\nBatch complete: " << results.size() << " videos processed\n";

// Check results
int successes = 0, failures = 0;
for (const auto& result : results) {
    if (result.error) {
        failures++;
        try {
            std::rethrow_exception(result.error);
        } catch (const std::exception& e) {
            std::cerr << "Error processing " << result.id << ": " 
                      << e.what() << "\n";
        }
    } else {
        successes++;
        // Store result
        saveMetadata(result.id, result.result);
    }
}

std::cout << "Successes: " << successes << ", Failures: " << failures << "\n";
```

## Implementation Considerations
### Technical Details
- **Threading Strategy**:
  ```cpp
  // Thread pool for concurrent processing
  std::vector<std::thread> workers;
  std::queue<VideoTask> task_queue;
  std::mutex queue_mutex;
  std::condition_variable queue_cv;
  
  for (int i = 0; i < max_concurrent; i++) {
      workers.emplace_back([&]() {
          while (true) {
              VideoTask task = getNextTask();
              if (task.done) break;
              
              try {
                  auto result = processVideo(task);
                  results.push_back(result);
              } catch (...) {
                  recordError(task.id, std::current_exception());
              }
          }
      });
  }
  ```

- **Resource Management**:
  - Per-thread FFmpeg context (not thread-safe to share)
  - Shared temporary directory with unique file names
  - Connection pooling for GPU (if hardware acceleration)

- **Memory Management**:
  ```
  Queue Size × Video Size × (1 + Prefetch Factor)
  Example: 100 videos × 50MB × 1.5 = 7.5GB RAM
  
  Mitigation:
  - Limit queue size (config.set("batch.queue_size", 100))
  - Stream from disk instead of loading all to memory
  - Process in chunks (e.g., 100 videos per batch)
  ```

### Dependencies
- Existing: FFmpeg, C++17 threading
- Optional: Thread pool library (e.g., BS::thread_pool)

### Configuration Options
```cpp
// Batch processing options
config.set("batch.max_concurrent", int);       // Max parallel videos (default: CPU cores)
config.set("batch.queue_size", int);           // Max videos in queue (default: 1000)
config.set("batch.prefetch", bool);            // Prefetch next videos (default: true)
config.set("batch.timeout_ms", int);           // Per-video timeout (default: 60000)
config.set("batch.retry_on_error", bool);      // Retry failed videos (default: false)
config.set("batch.retry_count", int);          // Max retries (default: 3)

// Sorting strategy
config.set("batch.sort", string);              // "size" | "duration" | "fifo" | "lifo"
// "size" = process small videos first (better for average latency)
// "duration" = process short videos first
// "fifo" = first-in-first-out
// "lifo" = last-in-first-out
```

### Performance Estimates
**1000 videos, average 50MB, 2-minute duration:**

| Configuration | Total Time | Throughput |
|---------------|------------|------------|
| Sequential | 500s (8.3min) | 2 videos/s |
| 4 threads | 125s (2.1min) | 8 videos/s |
| 8 threads | 62s (1.0min) | 16 videos/s |
| 16 threads | 31s (0.5min) | 32 videos/s |

**Limiting factors:**
- CPU: ~8-16 concurrent videos before CPU saturation
- GPU: ~20-50 concurrent with hardware acceleration
- I/O: Disk throughput (SSD recommended)
- Memory: Queue size × video size

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Hardware Acceleration** (Future work - significantly improves batch throughput)
- **Distributed Sharding** (Batch processing per shard)
- **Job Queue** (Async background processing)

## Additional Context
### References
- [Intel TBB Parallel Algorithms](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html)
- [C++17 Parallel Algorithms](https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t)
- [Thread Pool Patterns](https://github.com/bshoshany/thread-pool)

### Use Cases
1. **Video Library Migration**: Bulk import from legacy system
2. **Nightly Processing**: Process uploaded videos overnight
3. **Content Moderation**: Batch scan for policy violations
4. **Analytics Pipeline**: Extract features from video corpus
5. **Thumbnail Generation**: Generate thumbnails for video gallery

### API Design Options
**Option 1: Batch class (proposed)**
```cpp
VideoProcessorBatch batch(config);
batch.addVideo(...);
auto results = batch.process();
```

**Option 2: Static batch method**
```cpp
auto results = VideoProcessor::processBatch(videos, config);
```

**Option 3: Async iterator**
```cpp
VideoProcessor processor(config);
auto results = processor.processBatchAsync(videos);
for (auto& future : results) {
    auto result = future.get();  // Blocks until ready
}
```

**Recommendation**: Option 1 for better resource management and progress tracking.

### Monitoring
```cpp
// Batch statistics
json batch_stats = batch.getStatistics();
batch_stats["total_videos"] = total;
batch_stats["completed"] = completed;
batch_stats["failed"] = failed;
batch_stats["avg_process_time_ms"] = avg_time;
batch_stats["throughput_videos_per_sec"] = throughput;
batch_stats["peak_memory_mb"] = peak_memory;
```

### Error Handling
```cpp
// Per-video errors don't stop batch
batch.setErrorHandler([](const std::string& id, std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (const std::exception& e) {
        LOG_ERROR("Video {} failed: {}", id, e.what());
        // Optionally: Add to retry queue
    }
});
```

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
