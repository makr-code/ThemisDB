### Context

This issue implements the roadmap item 'Parallel Replication' for the replication domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Parallel Replication

### Goal

Deliver the scoped changes for Parallel Replication in src/replication/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Parallel Replication
**Priority:** High  
**Target Version:** v1.6.0

Enable parallel application of replication changes on followers to reduce replication lag and improve throughput.

**Features:**
- Multi-threaded WAL application on followers
- Dependency tracking to maintain consistency
- Configurable parallelism (2-64 threads)
- Transaction grouping for batch application
- Conflict-free parallel writes (different keys)

**Architecture:**
```cpp
class ParallelReplicationWorker {
public:
    struct ParallelConfig {
        uint32_t worker_threads = 4;
        uint32_t queue_size = 10000;
        bool use_dependency_tracking = true;
        bool group_transactions = true;
    };
    
    explicit ParallelReplicationWorker(const ParallelConfig& config);
    
    // Submit WAL entry for parallel application
    void submit(const WALEntry& entry);
    
    // Wait for all pending entries to be applied
    void sync();
    
    // Get statistics
    struct Stats {
        uint64_t entries_applied;
        uint64_t dependencies_detected;
        uint64_t average_latency_us;
        double parallelism_factor;  // Effective parallelism
    };
    Stats getStats() const;
    
private:
    // Dependency graph
    struct Dependency {
        std::string document_id;
        uint64_t sequence_number;
        std::vector<uint64_t> depends_on;
    };
    
    // Worker thread pool
    std::vector<std::thread> workers_;
    std::queue<WALEntry> work_queue_;
    std::map<std::string, std::vector<uint64_t>> dependency_graph_;
    
    void workerLoop(int worker_id);
    bool hasDependencies(const WALEntry& entry);
};

// Example: Enable parallel replication
ParallelConfig pconfig;
pconfig.worker_threads = 8;
pconfig.use_dependency_tracking = true;

ParallelReplicationWorker parallel(pconfig);

// Follower applies entries in parallel
for (const auto& entry : wal_batch) {
    parallel.submit(entry);
}
parallel.sync();

auto stats = parallel.getStats();
std::cout << "Parallelism: " << stats.parallelism_factor << "x" << std::endl;
```

**Performance Targets:**
- 3-10x throughput improvement for independent writes
- Reduce replication lag from seconds to milliseconds
- Support 100K+ writes/sec on followers

**Implementation Notes:**
- Use document_id as dependency key
- Group transactions to apply atomically
- Handle DDL operations serially (no parallelism)

---

### Acceptance Criteria

- [ ] Multi-threaded WAL application on followers
- [ ] Dependency tracking to maintain consistency
- [ ] Configurable parallelism (2-64 threads)
- [ ] Transaction grouping for batch application
- [ ] Conflict-free parallel writes (different keys)
- [ ] 3-10x throughput improvement for independent writes
- [ ] Reduce replication lag from seconds to milliseconds
- [ ] Support 100K+ writes/sec on followers
- [ ] Use document_id as dependency key
- [ ] Group transactions to apply atomically
- [ ] Handle DDL operations serially (no parallelism)

### Relationships

- Roadmap row: #97 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/replication/FUTURE_ENHANCEMENTS.md#parallel-replication
- Source key: roadmap:97:replication:v1.6.0:parallel-replication

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:97:replication:v1.6.0:parallel-replication -->
<!-- roadmap-ref: row=97;module=replication;target=v1.6.0 -->
<!-- roadmap-detail: src/replication/FUTURE_ENHANCEMENTS.md#parallel-replication -->
