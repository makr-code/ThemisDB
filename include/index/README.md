# ThemisDB Index Module - Public Headers

This directory contains the public API headers for ThemisDB's Index module, providing interfaces for vector similarity search, secondary indexing, graph traversal, spatial queries, and advanced features like quantization and rotary positional embeddings.

## Purpose

Public interfaces and declarations for:
- Vector similarity search with HNSW and GPU acceleration
- Secondary, composite, fulltext, and spatial indexes
- Graph adjacency indexing and traversal algorithms
- Vector quantization strategies
- Rotary positional embeddings (RoPE)
- Graph neural network (GNN) embeddings
- Adaptive index management
- HNSW parameter tuning and optimization

## Header Organization

### Core Index Managers

#### vector_index.h / vector_index_manager.h
Vector similarity search with HNSW acceleration and quantization support.

**Key Classes:**
- `VectorIndexManager`: Main vector index coordinator
- `HnswParams`: HNSW configuration parameters
- `VectorSearchResult`: Query results with distances

**Key Methods:**
```cpp
// Initialize index
Result<void> init(const std::string& objectName, size_t dimension, 
                  DistanceMetric metric, const HnswParams& params);

// Add/update/remove vectors
Result<void> addEntity(const std::string& objectName, const std::string& pk, 
                       const std::vector<float>& embedding);
Result<void> updateEntity(const std::string& objectName, const std::string& pk, 
                          const std::vector<float>& embedding);
Result<void> removeByPk(const std::string& objectName, const std::string& pk);

// Search operations
std::vector<VectorSearchResult> searchKnn(const std::string& objectName, 
                                           const std::vector<float>& query, 
                                           size_t k, int ef_search = -1);
std::vector<VectorSearchResult> searchKnnFiltered(const std::string& objectName, 
                                                    const std::vector<float>& query, 
                                                    size_t k, 
                                                    const FilterExpr& filter);
// Radius search (find all within distance threshold)
// Note: "KnnRadius" combines k-nearest neighbors with radius constraint
std::vector<VectorSearchResult> searchKnnRadius(const std::string& objectName, 
                                                 const std::vector<float>& query, 
                                                 float radius, size_t max_results);
```

**Distance Metrics:**
- `L2`: Euclidean distance (default)
- `COSINE`: Cosine similarity
- `DOT`: Inner product / dot product

#### secondary_index.h
Multi-type secondary indexing with atomic maintenance.

**Key Classes:**
- `SecondaryIndexManager`: Secondary index coordinator
- `IndexType`: Enum for index types (REGULAR, COMPOSITE, RANGE, SPARSE, GEO, TTL, FULLTEXT)
- `ScanOptions`: Query configuration

**Key Methods:**
```cpp
// Index management
Result<void> createIndex(const std::string& table, const std::string& field, 
                         IndexType type = IndexType::REGULAR);
Result<void> createIndex(const std::string& table, 
                         const std::vector<std::string>& fields, 
                         IndexType type = IndexType::COMPOSITE);
Result<void> dropIndex(const std::string& table, const std::string& field);
bool hasIndex(const std::string& table, const std::string& field);

// Query operations
std::vector<std::string> scanKeysEqual(const std::string& table, 
                                        const std::string& field, 
                                        const std::string& value);
std::vector<std::string> scanKeysRange(const std::string& table, 
                                        const std::string& field, 
                                        const std::string& start, 
                                        const std::string& end);

// Fulltext search
std::vector<std::string> scanFulltext(const std::string& table, 
                                       const std::string& field, 
                                       const std::string& query);
std::vector<std::pair<std::string, float>> scanFulltextWithScores(
    const std::string& table, const std::string& field, 
    const std::string& query, size_t k = 10);

// Geo search
std::vector<std::string> scanGeoBox(const std::string& table, 
                                     const std::string& field, 
                                     const BoundingBox& box);
std::vector<std::string> scanGeoRadius(const std::string& table, 
                                        const std::string& field, 
                                        const Point& center, 
                                        double radius_km);
```

#### graph_index.h
Graph adjacency indexing with traversal algorithms.

**Key Classes:**
- `GraphIndexManager`: Graph index coordinator
- `PathConstraints`: Constraints for path queries
- `TraversalResult`: BFS/DFS results
- `ShortestPath`: Dijkstra/A* results

**Key Methods:**
```cpp
// Edge management
Result<void> addEdge(const std::string& graph_id, const std::string& fromPk, 
                     const std::string& toPk, const std::string& edgeId, 
                     const EdgeProperties& props = {});
Result<void> deleteEdge(const std::string& graph_id, const std::string& edgeId);

// Neighborhood queries
std::vector<std::string> outNeighbors(const std::string& graph_id, 
                                       const std::string& pk);
std::vector<std::string> inNeighbors(const std::string& graph_id, 
                                      const std::string& pk);

// Traversal algorithms
TraversalResult bfs(const std::string& graph_id, const std::string& start_pk, 
                    size_t max_depth = std::numeric_limits<size_t>::max());
ShortestPath dijkstra(const std::string& graph_id, 
                      const std::string& start_pk, 
                      const std::string& end_pk);
ShortestPath aStar(const std::string& graph_id, 
                   const std::string& start_pk, 
                   const std::string& end_pk, 
                   const HeuristicFunc& heuristic);

// Constrained traversal
TraversalResult bfsWithConstraints(const std::string& graph_id, 
                                    const std::string& start_pk, 
                                    const std::string& end_pk, 
                                    const PathConstraints& constraints);

// Temporal queries
std::vector<EdgeData> getEdgesInTimeRange(const std::string& graph_id, 
                                           const std::string& pk, 
                                           Timestamp start, Timestamp end);
AggregateResult aggregateEdgePropertyInTimeRange(const std::string& graph_id, 
                                                   const std::string& pk, 
                                                   const std::string& property, 
                                                   Timestamp start, Timestamp end, 
                                                   AggType agg_type);
```

#### spatial_index.h
R-tree spatial indexing with 2D/3D support.

**Key Classes:**
- `SpatialIndexManager`: Spatial index coordinator
- `BoundingBox`: Rectangle/cuboid representation
- `Point`: 2D/3D point
- `SpatialConfig`: Configuration for spatial index

**Key Methods:**
```cpp
// Index management
Result<void> createSpatialIndex(const std::string& table, 
                                 const std::string& field, 
                                 const SpatialConfig& config);
Result<void> dropSpatialIndex(const std::string& table, 
                               const std::string& field);

// Insert/update/remove
Result<void> insert(const std::string& table, const std::string& field, 
                    const std::string& pk, const BoundingBox& mbr);
Result<void> update(const std::string& table, const std::string& field, 
                    const std::string& pk, const BoundingBox& mbr);
Result<void> remove(const std::string& table, const std::string& field, 
                    const std::string& pk);

// Query operations
std::vector<std::string> searchIntersects(const std::string& table, 
                                           const std::string& field, 
                                           const BoundingBox& query);
std::vector<std::string> searchWithin(const std::string& table, 
                                       const std::string& field, 
                                       const BoundingBox& query);
std::vector<std::string> searchKNN(const std::string& table, 
                                    const std::string& field, 
                                    const Point& query, size_t k);
std::vector<std::string> searchNearby(const std::string& table, 
                                       const std::string& field, 
                                       const Point& query, 
                                       double radius_km);
```

#### adaptive_index.h
Automatic index suggestion based on query patterns.

**Key Classes:**
- `AdaptiveIndexManager`: Adaptive index coordinator
- `QueryPattern`: Tracked query pattern
- `FieldSelectivity`: Field statistics
- `IndexSuggestion`: Recommended index

**Key Methods:**
```cpp
// Pattern tracking
void recordPattern(const QueryPattern& pattern);
std::vector<QueryPattern> getPatterns(const std::string& table = "");
std::vector<QueryPattern> getTopPatterns(size_t n, const std::string& table = "");

// Selectivity analysis
void analyze(const FieldSelectivity& selectivity);

// Index suggestions
std::vector<IndexSuggestion> generateSuggestions();
std::vector<IndexSuggestion> generateCacheAwareIndexes(size_t cache_size_mb = 20);
```

### GPU Acceleration

#### gpu_vector_index.h
GPU-accelerated vector similarity search.

**Key Classes:**
- `GPUVectorIndex`: GPU vector index
- `GPUVectorIndexConfig`: Configuration
- `GPUBackend`: Enum (VULKAN, CUDA, HIP, CPU_SIMD)

**Key Methods:**
```cpp
// Vector management
Result<void> addVector(const std::string& id, const std::vector<float>& vector);
Result<void> addVectorBatch(const std::vector<std::vector<float>>& vectors, 
                             const std::vector<std::string>& ids);
Result<void> removeVector(const std::string& id);
Result<void> updateVector(const std::string& id, const std::vector<float>& vector);

// Search operations
std::vector<VectorSearchResult> search(const std::vector<float>& query, size_t k);
std::vector<std::vector<VectorSearchResult>> searchBatch(
    const std::vector<std::vector<float>>& queries, size_t k);

// Backend management
std::vector<GPUBackend> getAvailableBackends();
Result<void> switchBackend(GPUBackend backend);
GPUBackend getCurrentBackend();
```

### Quantization

#### binary_quantizer.h
Binary quantization (1-bit per dimension).

**Key Classes:**
- `BinaryQuantizer`: Binary quantization implementation
- `BinaryQuantizerConfig`: Configuration

**Key Methods:**
```cpp
// Training and encoding
Result<void> train(const std::vector<std::vector<float>>& training_data);
std::vector<uint8_t> encode(const std::vector<float>& vector);
std::vector<float> decode(const std::vector<uint8_t>& binary);

// Distance computation
float hammingDistance(const std::vector<uint8_t>& a, 
                      const std::vector<uint8_t>& b);
float asymmetricDistance(const std::vector<float>& query, 
                         const std::vector<uint8_t>& binary);
```

#### product_quantizer.h
Product quantization (k-means subquantizers).

**Key Classes:**
- `ProductQuantizer`: Product quantization implementation
- `ProductQuantizerConfig`: Configuration

**Key Methods:**
```cpp
// Training and encoding
Result<void> train(const std::vector<std::vector<float>>& training_data);
std::vector<uint8_t> encode(const std::vector<float>& vector);
std::vector<float> decode(const std::vector<uint8_t>& codes);

// Distance computation
float computeAsymmetricDistance(const std::vector<float>& query, 
                                 const std::vector<uint8_t>& codes);

// Metrics
float getCompressionRatio();
size_t getEncodedSize();
```

#### residual_quantizer.h
Residual quantization (multi-stage refinement).

**Key Classes:**
- `ResidualQuantizer`: Residual quantization implementation
- `ResidualQuantizerConfig`: Configuration

**Key Methods:**
```cpp
// Training and encoding
Result<void> train(const std::vector<std::vector<float>>& training_data);
std::vector<uint8_t> encode(const std::vector<float>& vector);
std::vector<float> decode(const std::vector<uint8_t>& codes);

// Distance computation
float asymmetricDistance(const std::vector<float>& query, 
                         const std::vector<uint8_t>& codes);

// Stage management
ProductQuantizer& getStageQuantizer(size_t stage);
size_t getNumStages();
```

#### learned_quantizer.h
Learned quantization (RESEARCH ONLY).

**Key Classes:**
- `LearnedQuantizer`: Learned quantization implementation
- `LearnedQuantizerConfig`: Configuration

**Key Methods:**
```cpp
// Training and encoding
Result<void> train(const std::vector<std::vector<float>>& training_data);
std::vector<uint8_t> encode(const std::vector<float>& vector);
std::vector<float> decode(const std::vector<uint8_t>& codes);
```

### Rotary Positional Embeddings

#### rotary_embeddings.h
Standard RoPE implementation.

**Key Classes:**
- `RotaryEmbedding`: RoPE implementation
- `RoPEConfig`: Configuration

**Key Methods:**
```cpp
// Rotation operations
std::vector<float> rotate(const std::vector<float>& embedding, int position);
std::vector<float> rotateInverse(const std::vector<float>& rotated, int position);
std::vector<std::vector<float>> rotateBatch(
    const std::vector<std::vector<float>>& embeddings, 
    const std::vector<int>& positions);

// Relational rotation
std::vector<float> rotateRelational(const std::vector<float>& embedding, 
                                     const std::string& relation);
```

#### lora_rope.h
LoRA-adapted RoPE.

**Key Classes:**
- `LoRARotaryEmbedding`: LoRA-RoPE implementation
- `LoRAConfig`: LoRA adapter configuration

**Key Methods:**
```cpp
// Adapter management
Result<void> registerAdapter(const std::string& name, const LoRAConfig& config);
Result<void> unregisterAdapter(const std::string& name);
bool hasAdapter(const std::string& name);
void setAdapterEnabled(const std::string& name, bool enabled);

// Rotation with adapter
std::vector<float> rotateWithAdapter(const std::vector<float>& embedding, 
                                      int position, 
                                      const std::string& adapter_name);
std::vector<float> rotateWithAdapterBlend(
    const std::vector<float>& embedding, int position, 
    const std::map<std::string, float>& adapter_weights);
```

#### learnable_rope.h
Trainable RoPE.

**Key Classes:**
- `LearnableRotaryEmbedding`: Learnable RoPE implementation
- `TrainingConfig`: Training configuration

**Key Methods:**
```cpp
// Training mode
void setTrainingMode(bool training);
bool isTraining();

// Training operations
std::vector<float> train(const std::vector<std::vector<float>>& training_data, 
                         const std::vector<std::vector<float>>& targets, 
                         const TrainingConfig& config);
void computeGradients(const std::vector<float>& loss_gradient);
void updateParameters(float learning_rate);

// Parameter management
Result<void> saveParameters(const std::string& path);
Result<void> loadParameters(const std::string& path);

// Validation
float computeValidationLoss(const std::vector<std::vector<float>>& val_data, 
                             const std::vector<std::vector<float>>& val_targets);
```

### Graph Neural Networks

#### gnn_embeddings.h
GNN embedding generation and management.

**Key Classes:**
- `GNNEmbeddingManager`: GNN coordinator
- `GNNModelConfig`: Model configuration
- `EmbeddingMetadata`: Metadata tracking
- `AggregationType`: Pooling strategy

**Key Methods:**
```cpp
// Model management
Result<void> registerModel(const std::string& model_name, 
                           const GNNModelConfig& config);
GNNModelConfig getModelInfo(const std::string& model_name);

// Node embeddings
Result<void> generateNodeEmbeddings(const std::string& graph_id, 
                                     const std::vector<std::string>& node_ids, 
                                     const std::string& model_name);
Result<void> updateNodeEmbedding(const std::string& graph_id, 
                                  const std::string& node_id, 
                                  const std::string& model_name);
Result<void> generateNodeEmbeddingsBatch(const std::string& graph_id, 
                                          const std::vector<std::string>& node_ids, 
                                          const std::string& model_name, 
                                          size_t batch_size = 100);

// Edge embeddings
Result<void> generateEdgeEmbeddings(const std::string& graph_id, 
                                     const std::vector<std::string>& edge_ids, 
                                     const std::string& model_name);

// Graph-level embeddings
std::vector<float> generateGraphEmbedding(const std::string& graph_id, 
                                           const std::string& model_name, 
                                           AggregationType agg = AggregationType::MEAN_POOLING);

// Similarity search
std::vector<std::pair<std::string, float>> findSimilarNodes(
    const std::string& graph_id, const std::string& node_id, 
    const std::string& model_name, size_t k = 10);
```

### HNSW Optimization

#### hnsw_parameter_tuner.h
Adaptive HNSW parameter tuning.

**Key Classes:**
- `HnswParameterTuner`: Parameter tuner
- `WorkloadType`: Workload classification
- `TunerConfig`: Configuration
- `TunerStats`: Performance statistics

**Key Methods:**
```cpp
// Parameter recommendations
int getOptimalEfSearch(size_t k, float target_recall = 0.95);
int getRecommendedM(size_t dataset_size);
int getRecommendedEfConstruction(size_t dataset_size);

// Feedback loop
void recordQueryResult(const QueryResult& result);

// Workload optimization
WorkloadOptimizedConfig getWorkloadOptimizedConfig();
TunerStats getStats();
```

#### hnsw_layer_optimizer.h
HNSW layer pruning and optimization.

**Key Classes:**
- `HnswLayerOptimizer`: Layer optimizer
- `LayerAccessInfo`: Layer statistics
- `QueryStats`: Query execution statistics

**Key Methods:**
```cpp
// Optimization decisions
int getOptimalEntryLayer();
int getOptimalEf(size_t k);
bool shouldPruneLayer(int layer, size_t candidate_count, size_t k);

// Feedback loop
void recordLayerAccess(const LayerAccessInfo& info);
void recordQueryStats(const QueryStats& stats);

// Statistics
LayerStats getLayerStats(int layer);
std::vector<QueryStats> getRecentQueryStats(size_t n = 100);
```

### Additional Components

#### multi_vector_search.h
Multi-vector and hybrid search.

**Key Classes:**
- `MultiVectorSearch`: Multi-vector coordinator
- `SearchStrategy`: Strategy enum (ENSEMBLE, QUERY_EXPANSION, HYBRID, LEARNED_FUSION)

**Key Methods:**
```cpp
std::vector<VectorSearchResult> ensembleSearch(
    const std::vector<std::vector<float>>& query_vectors, size_t k);
std::vector<VectorSearchResult> hybridSearch(
    const std::vector<float>& dense_vector, 
    const std::string& sparse_query, size_t k);
```

#### approximate_radius_search.h
Radius-based vector search.

**Key Classes:**
- `ApproximateRadiusSearch`: Radius search coordinator

**Key Methods:**
```cpp
std::vector<VectorSearchResult> searchRadius(const std::vector<float>& query, 
                                               float radius, 
                                               size_t max_results = std::numeric_limits<size_t>::max());
```

#### property_graph.h
Property graph management.

**Key Classes:**
- `PropertyGraphManager`: Property graph coordinator
- `NodeProperties`: Node attribute storage
- `EdgeProperties`: Edge attribute storage

#### temporal_graph.h
Temporal graph extensions.

**Key Classes:**
- `TemporalGraphManager`: Temporal graph coordinator
- `TemporalQuery`: Time-based query
- `TemporalAggregation`: Time-based aggregation

#### edge_types.h
Typed edge management.

**Key Classes:**
- `EdgeTypeManager`: Edge type coordinator
- `EdgeType`: Type definition

## Usage Patterns

### Vector Search Pattern
```cpp
#include "index/vector_index.h"

// Initialize
VectorIndexManager vector_idx(rocksdb_wrapper);
HnswParams params{.M=16, .ef_construction=200};
vector_idx.init("embeddings", 1536, DistanceMetric::COSINE, params);

// Insert
vector_idx.addEntity("embeddings", "doc_1", embedding);

// Search
auto results = vector_idx.searchKnn("embeddings", query, k=10);
```

### Secondary Index Pattern
```cpp
#include "index/secondary_index.h"

// Create
SecondaryIndexManager sec_idx(rocksdb_wrapper);
sec_idx.createIndex("users", "email", IndexType::REGULAR);

// Query
auto results = sec_idx.scanKeysEqual("users", "email", "alice@example.com");
```

### Graph Traversal Pattern
```cpp
#include "index/graph_index.h"

// Create
GraphIndexManager graph_idx(rocksdb_wrapper);
graph_idx.addEdge("social", "alice", "bob", "edge_1", {});

// Traverse
auto path = graph_idx.dijkstra("social", "alice", "charlie");
```

### GPU Acceleration Pattern
```cpp
#include "index/gpu_vector_index.h"

// Create
GPUVectorIndexConfig config{.backend = GPUBackend::VULKAN};
GPUVectorIndex gpu_idx(config);

// Batch search
auto results = gpu_idx.searchBatch(queries, k=10);
```

## Implementation

See `../../src/index/` for implementation code.

## Documentation

- [Module README](../../src/index/README.md) - Comprehensive module documentation
- [Future Enhancements](../../src/index/FUTURE_ENHANCEMENTS.md) - Planned features
- [Vector Advanced Features](../../src/index/VECTOR_ADVANCED_FEATURES_README.md) - Advanced vector search guide
- [Component Docs](../../docs/de/src/index/) - Detailed component documentation

## Thread Safety

All index managers provide:
- Thread-safe read operations (concurrent queries)
- Thread-safe write operations (internal locking)
- Atomic batch operations (via RocksDB WriteBatch/Transaction)
- Iterator safety (reference counting)

## Performance Considerations

### Memory Management
- Vector indexes: ~50 bytes overhead per vector (HNSW)
- Secondary indexes: ~50-100 bytes per entry
- Graph indexes: ~100-200 bytes per edge
- Spatial indexes: ~100-200 bytes per object
- Quantization: 4-32x compression (accuracy trade-off)

### Query Optimization
- Use pre-filtering for highly selective filters
- Use post-filtering for low-selectivity filters
- Enable quantization for large datasets (>1M vectors)
- Use GPU acceleration for batch queries (>100 queries)
- Tune HNSW parameters based on workload

### Batch Operations
- Batch inserts: 10-100x faster than individual inserts
- Batch searches: 2-10x faster with GPU acceleration
- Use WriteBatch for atomic multi-operation commits

## See Also

- [Storage Module Headers](../storage/README.md) - Storage layer interfaces
- [Core Module Headers](../core/README.md) - Cross-cutting concerns interfaces
- [ARCHITECTURE.md](../../ARCHITECTURE.md) - System architecture overview
