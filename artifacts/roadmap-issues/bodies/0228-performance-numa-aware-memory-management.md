### Context

This issue implements the roadmap item 'NUMA-Aware Memory Management' for the performance domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: NUMA-Aware Memory Management

### Goal

Deliver the scoped changes for NUMA-Aware Memory Management in src/performance/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### NUMA-Aware Memory Management
**Priority:** Medium  
**Target Version:** v1.9.0  
**Research Basis:** "NUMA-aware Memory Management" (ASPLOS'15)

Optimize memory allocation and data placement for NUMA architectures.

**Features:**
- **Topology Detection**: Automatic NUMA node discovery
- **Affinity-Based Allocation**: Allocate memory on local node
- **Data Migration**: Move hot data to accessing thread's node
- **Thread Binding**: Pin threads to NUMA nodes
- **Remote Access Minimization**: Co-locate data and compute

**Architecture:**
```cpp
class NUMAMemoryManager {
public:
    struct NUMATopology {
        size_t num_nodes;
        std::vector<size_t> node_memory_mb;
        std::vector<std::vector<size_t>> node_distances;  // Latency matrix
    };
    
    struct AllocationHint {
        int preferred_node = -1;  // -1 = auto-detect
        bool allow_migration = true;
        bool use_huge_pages = false;
    };
    
    // Allocate on specific NUMA node
    void* allocate_on_node(size_t size, int node);
    
    // Allocate on thread's local node
    void* allocate_local(size_t size);
    
    // Migrate data to different node
    void migrate_to_node(void* ptr, size_t size, int target_node);
    
    // Get current node
    int get_current_node() const;
    
    // Get topology
    NUMATopology get_topology() const;
    
    // Statistics
    struct NUMAStats {
        uint64_t local_accesses;
        uint64_t remote_accesses;
        double locality_ratio;
        std::vector<uint64_t> per_node_allocations;
    };
    
    NUMAStats get_stats() const;
};

// Example usage
NUMAMemoryManager numa_mgr;

// Bind thread to NUMA node
int node = numa_mgr.get_current_node();
bind_thread_to_node(std::this_thread::get_id(), node);

// Allocate on local node
void* buffer = numa_mgr.allocate_local(1024 * 1024);

// Check locality
auto stats = numa_mgr.get_stats();
if (stats.locality_ratio < 0.8) {
    // High remote access - consider migration
    numa_mgr.migrate_to_node(buffer, size, target_node);
}
```

**Performance Targets:**
- **Local access ratio**: >90%
- **Remote access penalty**: -60% vs. unoptimized
- **Throughput**: +30-80% on NUMA systems

---

### Acceptance Criteria

- [ ] **Topology Detection**: Automatic NUMA node discovery
- [ ] **Affinity-Based Allocation**: Allocate memory on local node
- [ ] **Data Migration**: Move hot data to accessing thread's node
- [ ] **Thread Binding**: Pin threads to NUMA nodes
- [ ] **Remote Access Minimization**: Co-locate data and compute
- [ ] **Local access ratio**: >90%
- [ ] **Remote access penalty**: -60% vs. unoptimized
- [ ] **Throughput**: +30-80% on NUMA systems

### Relationships

- Roadmap row: #228 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/performance/FUTURE_ENHANCEMENTS.md#numa-aware-memory-management
- Source key: roadmap:228:performance:v1.9.0:numa-aware-memory-management

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:228:performance:v1.9.0:numa-aware-memory-management -->
<!-- roadmap-ref: row=228;module=performance;target=v1.9.0 -->
<!-- roadmap-detail: src/performance/FUTURE_ENHANCEMENTS.md#numa-aware-memory-management -->
