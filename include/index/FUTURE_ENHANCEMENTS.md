# Index Module Headers - Future API Enhancements

This document outlines planned API additions and breaking changes for the ThemisDB Index module public headers.

## API Versioning Strategy

### Semantic Versioning
- **Major version (X.0.0)**: Breaking API changes
- **Minor version (1.X.0)**: New features, backward-compatible
- **Patch version (1.5.X)**: Bug fixes, no API changes

### Deprecation Policy
1. Mark API as deprecated with `[[deprecated("message")]]`
2. Provide migration path in documentation
3. Keep deprecated API for 2 major versions
4. Remove in version X+2

## Planned API Changes

### v1.6.0 - Non-Breaking Additions

#### VectorIndexManager Enhancements
**New Methods:**
```cpp
// Advanced search with multiple filters
std::vector<VectorSearchResult> searchKnnMultiFilter(
    const std::string& objectName,
    const std::vector<float>& query,
    size_t k,
    const std::vector<FilterExpr>& filters,  // NEW: Multiple filters
    FilterCombineMode mode = FilterCombineMode::AND);  // NEW: AND/OR combination

// Batch search with per-query k
std::vector<std::vector<VectorSearchResult>> searchKnnBatch(
    const std::string& objectName,
    const std::vector<std::vector<float>>& queries,
    const std::vector<size_t>& k_values);  // NEW: Per-query k

// Export/import index
Result<void> exportIndex(const std::string& objectName, 
                         const std::string& export_path);  // NEW
Result<void> importIndex(const std::string& objectName, 
                         const std::string& import_path);  // NEW

// Index statistics
IndexStatistics getIndexStatistics(const std::string& objectName);  // NEW

struct IndexStatistics {
    size_t num_vectors;
    size_t dimension;
    size_t memory_bytes;
    DistanceMetric metric;
    bool quantized;
    size_t hnsw_layers;
    float avg_degree;
    float index_quality;  // Recall estimate
};
```

**New Configuration:**
```cpp
struct HnswParams {
    size_t M = 16;
    size_t ef_construction = 200;
    int ef_search = 100;
    size_t max_elements = 1000000;
    
    // NEW in v1.6.0
    bool allow_replace_deleted = true;  // Reuse deleted IDs
    size_t random_seed = 100;           // Reproducible builds
    bool normalize_vectors = false;     // Auto-normalize inputs
};
```

#### SecondaryIndexManager Enhancements
**New Methods:**
```cpp
// Index statistics
IndexStats getIndexStats(const std::string& table, const std::string& field);  // NEW

struct IndexStats {
    size_t num_entries;
    size_t memory_bytes;
    float cardinality_ratio;  // unique_values / total_values
    IndexType type;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_updated;
};

// Bulk operations
Result<void> bulkInsert(const std::string& table,
                        const std::vector<std::pair<std::string, FieldValues>>& data);  // NEW

// Index verification
VerificationResult verifyIndex(const std::string& table, 
                                const std::string& field);  // NEW

struct VerificationResult {
    bool is_consistent;
    size_t missing_entries;
    size_t extra_entries;
    std::vector<std::string> inconsistent_keys;
};
```

#### GraphIndexManager Enhancements
**New Methods:**
```cpp
// Centrality measures
CentralityResult computeBetweennessCentrality(const std::string& graph_id, 
                                               size_t sample_size = 0);  // NEW
CentralityResult computeClosenessCentrality(const std::string& graph_id);  // NEW
CentralityResult computePageRank(const std::string& graph_id, 
                                  float damping = 0.85, 
                                  size_t max_iterations = 100);  // NEW

struct CentralityResult {
    std::map<std::string, float> node_scores;
    float max_score;
    float min_score;
    float avg_score;
};

// Community detection
CommunityResult detectCommunities(const std::string& graph_id, 
                                   CommunityAlgorithm algo = CommunityAlgorithm::LOUVAIN);  // NEW

struct CommunityResult {
    std::map<std::string, size_t> node_to_community;
    size_t num_communities;
    std::vector<size_t> community_sizes;
    float modularity;
};

enum class CommunityAlgorithm {
    LOUVAIN,
    LABEL_PROPAGATION,
    GIRVAN_NEWMAN
};

// Subgraph extraction
SubgraphResult extractSubgraph(const std::string& graph_id,
                                const std::vector<std::string>& node_ids);  // NEW

struct SubgraphResult {
    std::vector<std::string> nodes;
    std::vector<EdgeData> edges;
    size_t num_nodes;
    size_t num_edges;
};
```

#### SpatialIndexManager Enhancements
**New Methods:**
```cpp
// Convex hull
ConvexHull computeConvexHull(const std::string& table, 
                              const std::string& field);  // NEW

struct ConvexHull {
    std::vector<Point> vertices;
    double area;
    double perimeter;
};

// Spatial clustering
ClusterResult spatialCluster(const std::string& table, 
                              const std::string& field, 
                              ClusterAlgorithm algo = ClusterAlgorithm::DBSCAN,
                              double epsilon = 1.0,
                              size_t min_points = 5);  // NEW

struct ClusterResult {
    std::map<std::string, int> pk_to_cluster;  // -1 = noise
    size_t num_clusters;
    std::vector<size_t> cluster_sizes;
};

enum class ClusterAlgorithm {
    DBSCAN,
    OPTICS,
    KMEANS
};

// Heatmap generation
Heatmap generateHeatmap(const std::string& table, 
                         const std::string& field, 
                         const BoundingBox& region,
                         size_t grid_width = 100,
                         size_t grid_height = 100);  // NEW

struct Heatmap {
    std::vector<std::vector<size_t>> grid;  // grid[y][x]
    size_t width;
    size_t height;
    BoundingBox bounds;
    size_t max_count;
};
```

#### GPUVectorIndex Enhancements
**New Methods:**
```cpp
// Multi-GPU support
Result<void> enableMultiGPU(const std::vector<int>& gpu_ids);  // NEW
GPUDistribution getGPUDistribution();  // NEW

struct GPUDistribution {
    std::map<int, size_t> gpu_to_vector_count;
    std::map<int, float> gpu_to_memory_usage_bytes;
    LoadBalancingStrategy strategy;
};

enum class LoadBalancingStrategy {
    ROUND_ROBIN,
    LEAST_LOADED,
    STATIC_PARTITION
};

// Performance profiling
GPUProfile getPerformanceProfile();  // NEW

struct GPUProfile {
    float avg_query_latency_ms;
    float throughput_queries_per_sec;
    float gpu_utilization_percent;
    float memory_bandwidth_gb_per_sec;
};
```

### v1.7.0 - Breaking Changes

#### VectorIndexManager API v2
**Removed Methods (Deprecated in v1.5.0):**
```cpp
// REMOVED: Use searchKnn with ef_search parameter instead
std::vector<VectorSearchResult> searchKnnWithEf(/* ... */);

// REMOVED: Use init with HnswParams instead
Result<void> initHnsw(/* ... */);
```

**Changed Signatures:**
```cpp
// BEFORE (v1.5.0):
Result<void> addEntity(const std::string& objectName, 
                       const std::string& pk, 
                       const std::vector<float>& embedding);

// AFTER (v1.7.0): Add optional metadata
Result<void> addEntity(const std::string& objectName, 
                       const std::string& pk, 
                       const std::vector<float>& embedding,
                       const VectorMetadata& metadata = {});  // NEW parameter

struct VectorMetadata {
    std::map<std::string, std::string> tags;       // User-defined tags
    std::chrono::system_clock::time_point timestamp;
    float confidence_score = 1.0;                   // Quality indicator
};
```

**New Distance Metric Enum:**
```cpp
enum class DistanceMetric {
    L2 = 0,
    COSINE = 1,
    DOT = 2,
    
    // NEW in v1.7.0
    HAMMING = 3,           // For binary vectors
    JACCARD = 4,           // For set similarity
    MAHALANOBIS = 5,       // Covariance-aware
    MINKOWSKI = 6          // Generalized metric (p-norm)
};
```

#### SecondaryIndexManager API v2
**Changed Index Types:**
```cpp
// BEFORE (v1.5.0):
enum class IndexType {
    REGULAR,
    COMPOSITE,
    RANGE,
    SPARSE,
    GEO,
    TTL,
    FULLTEXT
};

// AFTER (v1.7.0):
enum class IndexType {
    BTREE,              // Renamed from REGULAR
    HASH,               // NEW: Hash index for exact matches
    COMPOSITE,          // Unchanged
    RANGE,              // Unchanged
    BITMAP,             // NEW: For low-cardinality fields
    SPARSE,             // Unchanged
    GEO,                // Unchanged
    TTL,                // Unchanged
    FULLTEXT            // Unchanged
};
```

**New Create Method:**
```cpp
// v1.7.0: Extended configuration
Result<void> createIndex(const std::string& table, 
                         const std::string& field, 
                         const IndexConfig& config);  // NEW

struct IndexConfig {
    IndexType type = IndexType::BTREE;
    bool unique = false;                    // Enforce uniqueness
    bool sparse = false;                    // Skip NULLs
    std::string collation = "binary";       // String comparison
    CompressionType compression = CompressionType::SNAPPY;
    size_t cache_size_mb = 100;             // Index cache
};
```

### v1.8.0 - Advanced Features

#### Learned Index API
**New Header:** `learned_index.h`

```cpp
class LearnedIndexManager {
public:
    // Create learned index
    Result<void> createLearnedIndex(const std::string& table,
                                     const std::string& field,
                                     const LearnedIndexConfig& config);
    
    // Query (same interface as regular index)
    std::vector<std::string> scanKeysEqual(const std::string& table,
                                            const std::string& field,
                                            const std::string& value);
    
    // Train/update model
    Result<void> trainModel(const std::string& table,
                            const std::string& field,
                            const std::vector<TrainingExample>& data);
    
    // Model info
    LearnedModelInfo getModelInfo(const std::string& table,
                                   const std::string& field);
};

struct LearnedIndexConfig {
    ModelType model_type = ModelType::RMI;  // Recursive Model Index
    size_t num_models = 100;                 // Number of sub-models
    size_t training_data_size = 10000;       // Samples for training
    float error_bound = 64;                  // Position error tolerance
};

struct LearnedModelInfo {
    ModelType type;
    size_t num_models;
    float avg_error;
    float max_error;
    float speedup_vs_btree;
    size_t memory_bytes;
};
```

#### Federated Index API
**New Header:** `federated_index.h`

```cpp
class FederatedIndexManager {
public:
    // Register remote index
    Result<void> registerRemoteIndex(const std::string& name,
                                      const RemoteIndexConfig& config);
    
    // Federated search
    std::vector<VectorSearchResult> federatedSearch(
        const std::vector<std::string>& index_names,
        const std::vector<float>& query,
        size_t k,
        FederationStrategy strategy = FederationStrategy::MERGE_TOP_K);
    
    // Privacy-preserving search
    std::vector<VectorSearchResult> privateSearch(
        const std::string& index_name,
        const std::vector<float>& query,
        size_t k,
        PrivacyLevel privacy = PrivacyLevel::DIFFERENTIAL_PRIVACY);
};

struct RemoteIndexConfig {
    std::string endpoint;           // Remote server URL
    std::string auth_token;          // Authentication
    EncryptionType encryption;       // Transport encryption
    size_t timeout_ms = 5000;        // Query timeout
};

enum class FederationStrategy {
    MERGE_TOP_K,                     // Merge results, return top k
    ROUND_ROBIN,                     // Query each index in turn
    LOAD_BALANCED                    // Based on index load
};

enum class PrivacyLevel {
    NONE,                            // No privacy protection
    DIFFERENTIAL_PRIVACY,            // DP noise injection
    HOMOMORPHIC_ENCRYPTION,          // Encrypted computation
    SECURE_MULTI_PARTY               // SMC protocol
};
```

### v2.0.0 - Major Redesign

#### Unified Index Interface
**New Base Class:**
```cpp
// All index managers inherit from this
class IIndexManager {
public:
    virtual ~IIndexManager() = default;
    
    // Common operations
    virtual Result<void> create(const IndexDescriptor& descriptor) = 0;
    virtual Result<void> drop(const IndexDescriptor& descriptor) = 0;
    virtual bool exists(const IndexDescriptor& descriptor) = 0;
    
    // Query interface
    virtual QueryResult query(const IndexQuery& query) = 0;
    
    // Maintenance
    virtual Result<void> rebuild(const IndexDescriptor& descriptor) = 0;
    virtual Result<void> optimize(const IndexDescriptor& descriptor) = 0;
    
    // Statistics
    virtual IndexStatistics getStatistics(const IndexDescriptor& descriptor) = 0;
};

struct IndexDescriptor {
    std::string name;
    IndexType type;
    std::string table;
    std::vector<std::string> columns;
    IndexConfig config;
};

struct IndexQuery {
    IndexDescriptor index;
    QueryOperation operation;
    std::vector<QueryValue> values;
    QueryOptions options;
};

enum class QueryOperation {
    EXACT_MATCH,
    RANGE,
    PREFIX,
    SIMILARITY,
    CONTAINS,
    INTERSECTS,
    NEARBY
};
```

#### Async API
**All index operations support async:**
```cpp
class VectorIndexManager : public IIndexManager {
public:
    // Synchronous (existing)
    std::vector<VectorSearchResult> searchKnn(/* ... */);
    
    // Asynchronous (NEW in v2.0.0)
    std::future<std::vector<VectorSearchResult>> searchKnnAsync(/* ... */);
    
    // Callback-based (NEW in v2.0.0)
    void searchKnnAsync(/* ... */, 
                        std::function<void(Result<std::vector<VectorSearchResult>>)> callback);
    
    // Streaming results (NEW in v2.0.0)
    ResultStream<VectorSearchResult> searchKnnStreaming(/* ... */);
};

// Stream API for large result sets
template<typename T>
class ResultStream {
public:
    bool hasNext();
    T next();
    void close();
    
    // Range-based for loop support
    Iterator begin();
    Iterator end();
};
```

## Backward Compatibility

### Compatibility Matrix

| Client Version | Server v1.5.x | Server v1.6.x | Server v1.7.x | Server v2.0.x |
|----------------|---------------|---------------|---------------|---------------|
| v1.5.x         | ✅ Full       | ✅ Full       | ⚠️ Deprecated¹ | ❌ Removed    |
| v1.6.x         | ✅ Full       | ✅ Full       | ✅ Full       | ⚠️ Deprecated¹ |
| v1.7.x         | ⚠️ Limited²   | ✅ Full       | ✅ Full       | ✅ Full       |
| v2.0.x         | ❌ Incompatible³| ⚠️ Limited²  | ✅ Full       | ✅ Full       |

**Notes:**
1. **Deprecated**: Old APIs still work but emit deprecation warnings. New features unavailable.
2. **Limited**: Core APIs work, but new features (e.g., learned indexes, federated search) unavailable. Performance optimizations may not be available.
3. **Incompatible**: Breaking changes in protocol or API. Client must upgrade.

### Migration Guides

#### v1.5.x → v1.6.0
No breaking changes. All APIs backward compatible.

**Recommended Updates:**
```cpp
// Old (still works):
vector_idx.searchKnn(objectName, query, k);

// New (recommended):
vector_idx.searchKnn(objectName, query, k, ef_search);
```

#### v1.6.x → v1.7.0
Some breaking changes in enum values and method signatures.

**Required Changes:**
```cpp
// BEFORE (v1.6.x):
sec_idx.createIndex(table, field, IndexType::REGULAR);

// AFTER (v1.7.0):
sec_idx.createIndex(table, field, IndexType::BTREE);

// OR use config:
IndexConfig config{.type = IndexType::BTREE};
sec_idx.createIndex(table, field, config);
```

#### v1.7.x → v2.0.0
Major API redesign. Requires significant refactoring.

**Migration Tool:**
```bash
# Use automated migration tool
themis-migrate --from 1.7 --to 2.0 --input src/ --output src_migrated/

# Review changes
git diff --no-index src/ src_migrated/
```

## Feature Flags

### Compile-Time Flags
```cmake
# Enable experimental APIs
option(THEMIS_ENABLE_EXPERIMENTAL_API "Enable experimental index APIs" OFF)

# Enable API v2 (breaking changes)
option(THEMIS_ENABLE_API_V2 "Enable v2.0 unified index API" OFF)

# Backward compatibility
option(THEMIS_ENABLE_LEGACY_API "Enable deprecated v1.x APIs" ON)
```

### Runtime Flags
```cpp
// Enable experimental features at runtime
IndexFeatures::enable(Feature::LEARNED_INDEXES);
IndexFeatures::enable(Feature::FEDERATED_SEARCH);

// Check availability
if (IndexFeatures::isAvailable(Feature::QUANTUM_SEARCH)) {
    // Use quantum-inspired search
}
```

## API Stability Levels

### Stability Ratings
- **Stable**: Guaranteed backward compatibility within major version
- **Beta**: API may change in minor releases, with deprecation warnings
- **Experimental**: API may change or be removed without notice

### Current Ratings (v1.5.0)

#### Stable APIs
- `VectorIndexManager` core methods
- `SecondaryIndexManager` core methods
- `GraphIndexManager` core methods
- `SpatialIndexManager` core methods
- `HnswParameterTuner` core methods

#### Beta APIs
- `AdaptiveIndexManager`
- `GNNEmbeddingManager`
- `LoRARotaryEmbedding`
- `LearnableRotaryEmbedding`
- `GPUVectorIndex` (CUDA/HIP backends)
- Temporal graph methods
- Fulltext search
- 3D spatial indexing

#### Experimental APIs
- `LearnedQuantizer`
- Quantum-inspired algorithms
- Neuromorphic computing integration
- Federated indexing

## Deprecation Notices

### Deprecated in v1.5.0 (Remove in v1.7.0)
```cpp
// VectorIndexManager
[[deprecated("Use searchKnn with ef_search parameter")]]
std::vector<VectorSearchResult> searchKnnWithEf(/* ... */);

[[deprecated("Use init with HnswParams struct")]]
Result<void> initHnsw(/* ... */);
```

### Deprecated in v1.6.0 (Remove in v1.8.0)
```cpp
// SecondaryIndexManager
[[deprecated("Use IndexType::BTREE instead")]]
const IndexType REGULAR = IndexType::BTREE;

// GraphIndexManager
[[deprecated("Use computePageRank method")]]
std::map<std::string, float> calculatePageRank(/* ... */);
```

## Testing Strategy

### API Compatibility Tests
```cpp
// Automated compatibility testing
TEST(ApiCompatibility, v1_5_to_v1_6) {
    // Test that v1.5 client code works with v1.6 server
    ASSERT_TRUE(testBackwardCompatibility("1.5.0", "1.6.0"));
}

TEST(ApiCompatibility, v1_6_to_v1_7) {
    // Test migration paths
    ASSERT_TRUE(testMigrationPath("1.6.0", "1.7.0"));
}
```

### Performance Regression Tests
```cpp
// Ensure new APIs don't regress performance
BENCHMARK(VectorSearch_v1_5_API) {
    // Baseline performance
}

BENCHMARK(VectorSearch_v1_6_API) {
    // Must be <= 5% slower than v1.5
}
```

## Documentation

### API Documentation
- Doxygen comments on all public APIs
- Usage examples for each method
- Performance characteristics
- Thread safety guarantees
- Deprecation notices with migration paths

### Migration Guides
- Separate guide for each major version transition
- Code examples (before/after)
- Automated migration tools
- Breaking changes summary

## Community Feedback

We value community input on API design:

1. **RFC Process**: Propose API changes via GitHub RFC issues
2. **Beta Feedback**: Test beta APIs and provide feedback
3. **Breaking Change Review**: All breaking changes reviewed by community
4. **Voting**: Major API changes require community vote

**Contact:**
- GitHub Issues: [ThemisDB/ThemisDB](https://github.com/ThemisDB/ThemisDB/issues)
- Discussion Forum: [ThemisDB Discussions](https://github.com/ThemisDB/ThemisDB/discussions)
- Discord: [ThemisDB Community](https://discord.gg/themisdb)

## See Also

- [Implementation Future Enhancements](../../src/index/FUTURE_ENHANCEMENTS.md) - Implementation roadmap
- [API Versioning Policy](../../docs/api_versioning.md) - Detailed versioning policy
- [Breaking Changes Log](../../CHANGELOG.md) - Historical breaking changes
- [Migration Tools](../../tools/migration/README.md) - Automated migration tools
