# ThemisDB Index Module

## Module Purpose

The Index module provides ThemisDB's high-performance indexing infrastructure for multi-model data access. It implements vector similarity search (HNSW + GPU acceleration), secondary/composite indexes, graph adjacency indexing, spatial R-tree indexes, and adaptive index selection based on query patterns. The module supports advanced features like vector quantization, rotary positional embeddings (RoPE), graph neural network (GNN) embeddings, and automatic index tuning.

## Scope

**In Scope:**
- Vector similarity search with HNSW and GPU acceleration (Vulkan/CUDA/HIP)
- Vector quantization strategies (binary, product, residual, learned)
- Secondary indexes (B-tree, hash, composite, range, fulltext, geo, TTL)
- Graph adjacency indexing with traversal algorithms (BFS, Dijkstra, A*)
- Spatial R-tree indexing with 2D/3D support
- Adaptive index management with query pattern analysis
- Rotary positional embeddings (RoPE, LoRA-RoPE, Learnable-RoPE)
- Graph neural network (GNN) embeddings (GCN, GAT, GraphSAGE)
- HNSW parameter tuning and layer optimization
- Multi-vector and approximate radius search
- Temporal graph support with time-based queries

**Out of Scope:**
- Query parsing and execution planning (handled by query module)
- Data storage and persistence primitives (handled by storage module)
- Network protocols and API endpoints (handled by server module)
- Authentication and authorization (handled by auth module)

## Key Components

### VectorIndexManager
**Location:** `vector_index.cpp`, `../include/index/vector_index.h`

Core vector similarity search engine with optional HNSW acceleration and multiple quantization strategies.

**Features:**
- **HNSW Acceleration**: Hierarchical Navigable Small World graphs for approximate nearest neighbor (ANN) search
- **Brute-Force Fallback**: When HNSW disabled or for small datasets
- **Distance Metrics**: L2 (Euclidean), COSINE similarity, DOT product (inner product)
- **RocksDB Persistence**: Atomic operations via WriteBatch, namespace: `objectName:pk`
- **Advanced Indexing**: FAISS integration (IVF_FLAT, IVF_PQ, HNSW_FLAT, IVF_HNSW_PQ)
- **Vector Quantization**: Binary, product, residual, and learned quantization
- **Batch Operations**: Bulk add/update/remove with single transaction
- **Filtered Search**: Pre-filtering and post-filtering with secondary indexes
- **Radius Search**: Find all vectors within distance threshold (epsilon neighbors)
- **Statistics**: Centroid, variance, outlier detection, cardinality
- **Audit Logging**: Track access patterns and security events

**Configuration Example:**
```cpp
VectorIndexManager vector_idx(rocksdb_wrapper);

// Initialize index with dimension and distance metric
HnswParams hnsw_params{
    .M = 16,                    // Max connections per layer
    .ef_construction = 200,     // Construction search breadth
    .ef_search = 100,           // Query search breadth
    .max_elements = 1000000     // Capacity
};

vector_idx.init("embeddings", 1536, DistanceMetric::COSINE, hnsw_params);

// Enable product quantization for 8x compression
vector_idx.trainQuantizer("embeddings", quantizer_config);

// Add vectors
vector_idx.addEntity("embeddings", "doc_123", embedding_vector);

// KNN search
auto results = vector_idx.searchKnn("embeddings", query_vector, k=10, ef_search=100);

// Filtered search with secondary index
auto filtered = vector_idx.searchKnnPreFiltered("embeddings", query_vector, 
                                                 k=10, category_filter);

// Radius search (all within threshold)
auto nearby = vector_idx.searchKnnRadius("embeddings", query_vector, 
                                          radius=0.5, max_results=100);
```

**Performance Characteristics:**
- Point lookup: 50-200μs (with cache)
- KNN search (k=10): 1-5ms (HNSW), 50-500ms (brute-force, 1M vectors)
- Batch insert: 10K-50K vectors/sec (with HNSW construction)
- Memory: ~30-50 bytes per vector (HNSW overhead)
- Quantization: 4-32x compression (accuracy trade-off)
- Recall: 95-99% (configurable via ef_search)

**Thread Safety:**
- Read-safe: Multiple concurrent searches
- Write-safe: Internal locking for modifications
- Transaction-safe: Atomic batch operations

### GPUVectorIndex
**Location:** `gpu_vector_index.cpp`, `../include/index/gpu_vector_index.h`

Cross-platform GPU-accelerated vector similarity search with automatic backend selection.

**Supported Backends:**
- **Vulkan**: Production-ready (v2.2), cross-platform compute (NVIDIA, AMD, Intel, Apple)
- **CUDA**: Planned (v2.1), NVIDIA-specific optimizations
- **HIP**: Planned (v2.3), AMD-specific optimizations
- **CPU SIMD**: Always available fallback

**Features:**
- **Backend Auto-detection**: Dynamically discover and select optimal GPU
- **Mixed Precision**: FP16/TF32 support for higher throughput
- **Batch Processing**: 512+ queries per batch for maximum GPU utilization
- **High Throughput**: 200K+ queries/sec on modern GPUs
- **HNSW Algorithm**: Graph-based ANN search optimized for GPU
- **Dynamic Switching**: Change backends at runtime without rebuild

**Configuration Example:**
```cpp
GPUVectorIndexConfig config{
    .dimension = 1536,
    .metric = DistanceMetric::L2,
    .max_elements = 1000000,
    .backend = GPUBackend::VULKAN,
    .use_mixed_precision = true,
    .batch_size = 512
};

GPUVectorIndex gpu_idx(config);

// Check available backends
auto backends = gpu_idx.getAvailableBackends();
for (auto backend : backends) {
    std::cout << "Available: " << backendToString(backend) << "\n";
}

// Add vectors in batches
std::vector<std::vector<float>> vectors = /* load data */;
gpu_idx.addVectorBatch(vectors, ids);

// Batch search for maximum throughput
std::vector<std::vector<float>> queries = /* ... */;
auto results = gpu_idx.searchBatch(queries, k=10);

// Switch backend dynamically
gpu_idx.switchBackend(GPUBackend::CUDA);
```

**Performance Characteristics:**
- Single query: 100-500μs (GPU overhead for small batches)
- Batch queries (512): 200K-500K queries/sec
- Speedup vs CPU: 10-50x (batch queries)
- Memory: GPU VRAM required (~4GB for 1M vectors, 1536D)
- Precision: FP32 (default), FP16 (2x faster, minimal accuracy loss)

### SecondaryIndexManager
**Location:** `secondary_index.cpp`, `../include/index/secondary_index.h`

Multi-type secondary indexing with atomic maintenance and efficient range queries.

**Index Types:**
- **Regular**: Equality lookups (B-tree equivalent)
- **Composite**: Multi-column indexes with lexicographic ordering
- **Range**: Efficient range queries via string encoding
- **Sparse**: Skips NULL/missing values to save space
- **Geo**: Point + bounding box with Geohash/Morton code
- **TTL**: Time-to-live with automatic expiration cleanup
- **Fulltext**: Inverted index with BM25 scoring

**Key Encoding:**
```
Regular:    idx:<table>:<field>:<value>:<pk>
Composite:  idx:<table>:<field1>_<field2>:<value1>_<value2>:<pk>
Range:      idx:<table>:<field>:range:<encoded_value>:<pk>
Fulltext:   ftx:<table>:<field>:<term>:<pk> → <score>
Geo:        geo:<table>:<field>:<geohash>:<pk>
```

**Usage Example:**
```cpp
SecondaryIndexManager sec_idx(rocksdb_wrapper);

// Create regular index
sec_idx.createIndex("users", "email", IndexType::REGULAR);

// Create composite index
sec_idx.createIndex("users", {"city", "age"}, IndexType::COMPOSITE);

// Create fulltext index
sec_idx.createIndex("documents", "content", IndexType::FULLTEXT);

// Query equality
auto results = sec_idx.scanKeysEqual("users", "email", "alice@example.com");

// Query range
auto range_results = sec_idx.scanKeysRange("users", "age", "25", "35");

// Query fulltext with BM25 scoring
auto scored = sec_idx.scanFulltextWithScores("documents", "content", 
                                               "vector database", k=10);

// Bulk insert with single transaction
WriteBatch batch;
for (auto& [pk, values] : data) {
    sec_idx.putBatch(batch, "users", pk, values);
}
rocksdb->write(batch);  // 10-100x faster than individual puts
```

**Performance Characteristics:**
- Point lookup: 10-50μs (cached), 100-500μs (disk)
- Range scan: 100K-500K keys/sec (sequential)
- Composite index: +10-20% overhead per additional column
- Fulltext search: 1-10ms (depends on term frequency)
- Geo search: 5-50ms (depends on bounding box size)

### GraphIndexManager
**Location:** `graph_index.cpp`, `../include/index/graph_index.h`

Adjacency index management for directed graphs with in-memory topology and persistent storage.

**Index Structure:**
```
Out-edges: graph:out:<graph_id>:<fromPk>:<edgeId> → <toPk>
In-edges:  graph:in:<graph_id>:<toPk>:<edgeId> → <fromPk>
```

**Features:**
- **O(1) Neighborhood Queries**: Instant access to adjacent nodes
- **Multi-graph Support**: Multiple independent graphs per database
- **Path Algorithms**: BFS, Dijkstra, A* with temporal variants
- **Path Constraints**: Unique vertices/edges, forbidden nodes, required vertices
- **Temporal Graphs**: Time-based edge queries and temporal aggregations
- **Edge Type Filtering**: Query specific relationship types
- **Weighted Graphs**: Support for weighted edges (via `_weight` field)
- **Audit Logging**: Track graph access for security

**Usage Example:**
```cpp
GraphIndexManager graph_idx(rocksdb_wrapper);

// Add edge (bidirectional indexing)
graph_idx.addEdge("social", "alice", "bob", "edge_1", edge_properties);

// Query neighbors
auto out_neighbors = graph_idx.outNeighbors("social", "alice");
auto in_neighbors = graph_idx.inNeighbors("social", "bob");

// BFS traversal
auto bfs_result = graph_idx.bfs("social", "alice", max_depth=3);

// Dijkstra shortest path
auto path = graph_idx.dijkstra("social", "alice", "charlie");

// Constrained path search
PathConstraints constraints{
    .max_depth = 5,
    .unique_vertices = true,
    .forbidden_nodes = {"spam_account"},
    .required_vertices = {"trusted_hub"}
};
auto constrained_path = graph_idx.bfsWithConstraints("social", "alice", 
                                                       "charlie", constraints);

// Temporal graph queries
auto edges_at_time = graph_idx.getEdgesInTimeRange("social", "alice", 
                                                     start_time, end_time);

// Temporal aggregations
auto sum = graph_idx.aggregateEdgePropertyInTimeRange("social", "alice", 
                                                        "_weight", start, end, 
                                                        AggType::SUM);
```

**Performance Characteristics:**
- Add edge: 100-500μs (two writes)
- Neighbor query: 50-200μs (prefix scan)
- BFS (depth=3): 1-10ms (depends on branching factor)
- Dijkstra (path length 5): 5-50ms (depends on graph density)
- Temporal queries: 10-100ms (depends on time range)

### SpatialIndexManager
**Location:** `spatial_index.cpp`, `../include/index/spatial_index.h`

R-tree based spatial indexing with Morton code Z-order curve for efficient 2D/3D queries.

**Features:**
- **2D/3D Support**: Z-buckets for elevation filtering
- **Morton Code Encoding**: Z-order curve for spatial locality
- **Per-PK Sidecar**: Efficient updates without full rebuild
- **Query Types**: Intersects, within, contains, nearby, KNN
- **Distance Functions**: Haversine (lat/lon) or Euclidean 3D
- **Exact Geometry Backend**: Optional for accuracy verification
- **Model-Agnostic**: Works with relational, graph, vector, timeseries

**Index Structure:**
```
R-tree:  spatial:<index_name>:<morton_code>:<pk> → <mbr_json>
Sidecar: spatial:meta:<index_name>:<pk> → <geometry_json>
```

**Usage Example:**
```cpp
SpatialIndexManager spatial_idx(rocksdb_wrapper);

// Create spatial index
SpatialConfig config{
    .dimensions = 2,
    .use_haversine = true,  // For lat/lon
    .enable_z_buckets = false
};
spatial_idx.createSpatialIndex("places", "location", config);

// Insert point
BoundingBox point{.min_x=37.7749, .min_y=-122.4194, 
                  .max_x=37.7749, .max_y=-122.4194};  // San Francisco
spatial_idx.insert("places", "location", "sf_office", point);

// Search intersects
BoundingBox search_box{.min_x=37.7, .min_y=-122.5, 
                       .max_x=37.8, .max_y=-122.4};
auto intersects = spatial_idx.searchIntersects("places", "location", search_box);

// KNN search
Point query_point{37.7749, -122.4194};
auto nearest = spatial_idx.searchKNN("places", "location", query_point, k=5);

// Radius search
auto nearby = spatial_idx.searchNearby("places", "location", query_point, 
                                        radius_km=10.0);

// 3D search with elevation
auto z_filtered = spatial_idx.searchIntersectsWithZ("places", "location", 
                                                      search_box, z_min=0, z_max=500);
```

**Performance Characteristics:**
- Insert: 200-1000μs (R-tree update + sidecar)
- Intersects query: 5-50ms (depends on density)
- KNN search: 10-100ms (depends on k and density)
- Radius search: 5-100ms (depends on radius)
- Memory: ~100-200 bytes per object (R-tree node overhead)

### AdaptiveIndexManager
**Location:** `adaptive_index.cpp`, `../include/index/adaptive_index.h`

Automatic index suggestion engine based on query pattern analysis and selectivity estimation.

**Components:**

1. **QueryPatternTracker**
   - Tracks field access frequency and operations (eq, range, in, join)
   - Records execution times and cardinality
   - Cache-aware metrics (L3 misses, cache hit penalties)

2. **SelectivityAnalyzer**
   - Analyzes field cardinality and distribution
   - Detects uniform, skewed, or sparse distributions
   - Estimates null ratio and uniqueness
   - Cache-aware: estimates index fit in L3 cache (20MB)

3. **IndexSuggestionEngine**
   - Generates index recommendations with scores
   - Estimates query impact (queries affected, speedup)
   - Suggests index types (range, hash, composite)
   - Cache-aware suggestions for hot data

4. **AdaptiveIndexManager**
   - Orchestrates all components
   - Provides unified API

**Usage Example:**
```cpp
AdaptiveIndexManager adaptive(rocksdb_wrapper);

// Track query patterns
QueryPattern pattern{
    .table = "users",
    .fields = {"email"},
    .operation = Operation::EQUALITY,
    .execution_time_ms = 150,
    .rows_examined = 1000000,
    .rows_returned = 1
};
adaptive.recordPattern(pattern);

// Analyze field selectivity
FieldSelectivity selectivity{
    .table = "users",
    .field = "email",
    .cardinality = 950000,
    .total_rows = 1000000,
    .null_count = 50000
};
adaptive.analyze(selectivity);

// Generate index suggestions
auto suggestions = adaptive.generateSuggestions();
for (auto& suggestion : suggestions) {
    std::cout << "Suggestion: " << suggestion.index_type 
              << " on " << suggestion.table << "." << suggestion.field
              << " (score: " << suggestion.score << ")\n";
    std::cout << "  Impact: " << suggestion.estimated_speedup << "x faster\n";
    std::cout << "  Queries affected: " << suggestion.queries_affected << "\n";
}

// Cache-aware suggestions
auto cache_aware = adaptive.generateCacheAwareIndexes(l3_cache_size_mb=20);
```

**Suggestion Scoring:**
```
Score = (access_frequency × 0.4) + 
        (selectivity × 0.3) + 
        (execution_time_saved × 0.2) + 
        (cardinality_factor × 0.1)
```

**Performance Impact:**
- Automatic suggestions: +10-50% query performance improvement
- Reduced manual tuning: 80-90% reduction in DBA effort
- Cache-aware indexes: +20-40% better cache hit rates

### Quantization Implementations

#### BinaryQuantizer
**Location:** `binary_quantizer.cpp`, `../include/index/binary_quantizer.h`

Maximum compression (32x) using 1-bit per dimension.

**Algorithm:** Sign-based binarization: `binary_dim = sign(value - mean)`

**Features:**
- 32x compression ratio (float32 → 1 bit/dimension)
- Optional FAISS integration for Hamming distance
- Asymmetric distance (full-precision query vs binary vectors)
- Training learns centering/scaling parameters

**Usage:**
```cpp
BinaryQuantizer quantizer(dimension=1536);

// Train on representative data
std::vector<std::vector<float>> training_data = /* ... */;
quantizer.train(training_data);

// Encode vectors
std::vector<uint8_t> binary = quantizer.encode(vector);  // 1536 → 192 bytes

// Decode (approximate)
std::vector<float> decoded = quantizer.decode(binary);

// Hamming distance (fast)
float distance = quantizer.hammingDistance(binary1, binary2);

// Asymmetric distance (higher accuracy)
float asym_dist = quantizer.asymmetricDistance(query_full, vector_binary);
```

**Performance:**
- Compression: 32x (6KB → 192 bytes for 1536D)
- Encoding speed: 1-5μs per vector
- Hamming distance: 10-50ns (SIMD)
- Recall: 80-90% at 95% recall target

#### ProductQuantizer
**Location:** `product_quantizer.cpp`, `../include/index/product_quantizer.h`

Balanced compression via k-means subquantizers.

**Algorithm:**
1. Divide vector into M subvectors
2. K-means clustering on each subvector (K centroids)
3. Replace subvector with centroid ID

**Features:**
- 8-16x compression typical
- Optional FAISS k-means (20-30% faster training)
- Asymmetric distance (no decode needed)
- Configurable subquantizer count and centroids

**Usage:**
```cpp
ProductQuantizerConfig config{
    .dimension = 1536,
    .num_subquantizers = 8,      // 8 subquantizers of 192D each
    .num_centroids = 256,         // 8-bit codes per subquantizer
    .kmeans_iterations = 25
};

ProductQuantizer pq(config);

// Train on representative data
pq.train(training_data);

// Encode (1536D float32 → 8 bytes)
std::vector<uint8_t> codes = pq.encode(vector);

// Decode (approximate)
std::vector<float> decoded = pq.decode(codes);

// Asymmetric distance (fast and accurate)
float distance = pq.computeAsymmetricDistance(query_full, vector_codes);

// Get compression ratio
float ratio = pq.getCompressionRatio();  // ~192x
```

**Performance:**
- Compression: 8-16x (6KB → 384-768 bytes for 1536D)
- Training time: 5-30 minutes (1M vectors)
- Encoding speed: 10-50μs per vector
- Asymmetric distance: 1-5μs
- Recall: 95-98% at 95% recall target

#### ResidualQuantizer
**Location:** `residual_quantizer.cpp`, `../include/index/residual_quantizer.h`

Multi-stage quantization with progressive refinement.

**Algorithm:**
1. Stage 1: Initial product quantization
2. Stage 2: Quantize residual error = original - decoded_stage1
3. Stage N: Continue refinement on residuals
4. Final approximation: sum of all stage approximations

**Features:**
- 97-99% recall (vs 95-98% for single-stage PQ)
- 2-4 stages typical
- Each stage uses ProductQuantizer
- Early termination for distance computation

**Usage:**
```cpp
ResidualQuantizerConfig config{
    .dimension = 1536,
    .num_stages = 3,
    .stage_configs = {
        {.num_subquantizers=8, .num_centroids=256},  // Stage 1
        {.num_subquantizers=8, .num_centroids=256},  // Stage 2
        {.num_subquantizers=8, .num_centroids=256}   // Stage 3
    }
};

ResidualQuantizer rq(config);

// Train
rq.train(training_data);

// Encode (3 stages × 8 bytes = 24 bytes)
std::vector<uint8_t> codes = rq.encode(vector);

// Decode (sum all stages)
std::vector<float> decoded = rq.decode(codes);

// Asymmetric distance
float distance = rq.asymmetricDistance(query_full, vector_codes);
```

**Performance:**
- Compression: 8-12x (6KB → 512-768 bytes for 1536D, 3 stages)
- Training time: 15-90 minutes (3x PQ training)
- Encoding speed: 30-150μs per vector
- Recall: 97-99% at 95% recall target

#### LearnedQuantizer
**Location:** `learned_quantizer.cpp`, `../include/index/learned_quantizer.h`

**Status:** RESEARCH ONLY (NOT PRODUCTION)

Adaptive per-dimension quantization via Lloyd's algorithm.

**Algorithm:**
- Learn optimal thresholds per dimension (1D k-means)
- 2-8 bits per dimension configurable
- Initialize via percentiles or uniform spacing
- Contrastive loss training

**Usage:**
```cpp
LearnedQuantizerConfig config{
    .dimension = 1536,
    .bits_per_dim = 4,  // 16 levels per dimension
    .mode = QuantMode::PER_DIMENSION
};

LearnedQuantizer lq(config);

// Train
lq.train(training_data);

// Encode
std::vector<uint8_t> codes = lq.encode(vector);
```

### Rotary Positional Embeddings (RoPE)

#### RotaryEmbedding
**Location:** `rotary_embeddings.cpp`, `../include/index/rotary_embeddings.h`

Rotation-based positional encoding adapted for vector storage.

**Algorithm:**
```
For coordinate pairs (x₀, x₁), (x₂, x₃), ..., (x_{d-2}, x_{d-1}):
θᵢ = base^(-2i/d), base=10000
Rotation at position m:
  f(x_m) = R(x_m, mθ₀) ⊕ R(x_m, mθ₁) ⊕ ... ⊕ R(x_m, mθ_{d/2-1})
where R([x₀, x₁], θ) = [x₀cos(θ) - x₁sin(θ), x₀sin(θ) + x₁cos(θ)]
```

**Use Cases:**
- Positional awareness for sequential entities (documents, events)
- Relational embeddings for knowledge graphs (TransE-like)
- Temporal encoding for time-series data
- Multi-relational vector search

**Usage:**
```cpp
RotaryEmbedding rope(dimension=1536, base=10000.0);

// Rotate by position
std::vector<float> rotated = rope.rotate(embedding, position=42);

// Inverse rotation (for decoding)
std::vector<float> original = rope.rotateInverse(rotated, position=42);

// Batch rotation
std::vector<std::vector<float>> batch = {emb1, emb2, emb3};
auto rotated_batch = rope.rotateBatch(batch, positions={0, 1, 2});

// Relational rotation (for knowledge graphs)
auto rel_rotated = rope.rotateRelational(head_emb, relation_name);
```

**References:**
- Su et al. (2021). "RoFormer: Enhanced Transformer with Rotary Position Embedding" arXiv:2104.09864

#### LoRARotaryEmbedding
**Location:** `lora_rope.cpp`, `../include/index/lora_rope.h`

Low-rank adaptation for RoPE with domain-specific tuning.

**Architecture:**
- Base RoPE + LoRA adapter modifications
- Low-rank matrices: B (num_pairs × rank), A (rank × num_pairs)
- Learnable theta delta: θ_modified = θ_base + θ_delta
- Multiple adapters with per-domain configuration

**Features:**
- Adapter registry for multiple LoRA modules
- Adapter composition/blending with weights
- Enable/disable adapters dynamically
- Minimal memory overhead (low-rank)

**Usage:**
```cpp
LoRARotaryEmbedding lora_rope(dimension=1536, base=10000.0);

// Register adapter
LoRAConfig adapter_config{
    .rank = 8,
    .alpha = 16.0,
    .dropout = 0.1
};
lora_rope.registerAdapter("medical_domain", adapter_config);

// Rotate with adapter
auto rotated = lora_rope.rotateWithAdapter(embedding, position=10, 
                                             adapter_name="medical_domain");

// Blend multiple adapters
std::map<std::string, float> blend_weights{
    {"medical", 0.7},
    {"technical", 0.3}
};
auto blended = lora_rope.rotateWithAdapterBlend(embedding, position=10, 
                                                  blend_weights);

// Enable/disable adapter
lora_rope.setAdapterEnabled("medical_domain", false);
```

**Use Cases:**
- Domain-specific embeddings (medical, legal, technical)
- Fine-tuning for specific tasks without retraining
- Multi-task learning with adapter switching

#### LearnableRotaryEmbedding
**Location:** `learnable_rope.cpp`, `../include/index/learnable_rope.h`

Trainable theta parameters via contrastive learning.

**Features:**
- Learnable theta (frozen or trainable mode)
- Gradient computation & backpropagation
- SGD or Adam optimizer
- Early stopping with patience
- Parameter serialization (JSON)

**Training:**
```cpp
LearnableRotaryEmbedding learnable_rope(dimension=1536, base=10000.0);

// Enable training mode
learnable_rope.setTrainingMode(true);

// Training configuration
TrainingConfig config{
    .learning_rate = 0.001,
    .batch_size = 32,
    .max_epochs = 100,
    .optimizer = "adam",
    .temperature = 0.07,
    .patience = 10
};

// Train with contrastive loss
std::vector<std::vector<float>> training_data = /* ... */;
std::vector<std::vector<float>> targets = /* ... */;
auto loss_history = learnable_rope.train(training_data, targets, config);

// Save trained parameters
learnable_rope.saveParameters("rope_params.json");

// Load later
learnable_rope.loadParameters("rope_params.json");

// Inference mode
learnable_rope.setTrainingMode(false);
auto rotated = learnable_rope.rotate(embedding, position=10);
```

### Graph Neural Network (GNN) Embeddings

#### GNNEmbeddingManager
**Location:** `gnn_embeddings.cpp`, `../include/index/gnn_embeddings.h`

Generate and manage graph neural network embeddings for nodes, edges, and entire graphs.

**Supported Models:**
- **GCN** (Graph Convolutional Network): Aggregate neighborhood features
- **GraphSAGE**: Inductive representation learning with sampling
- **GAT** (Graph Attention Network): Attention-weighted aggregation
- **GIN** (Graph Isomorphism Network): Maximally expressive GNN
- **Custom Models**: User-defined GNN architectures

**Features:**
- Node embeddings based on features + graph structure
- Edge embeddings from edge features + connected nodes
- Graph-level embeddings via pooling (mean, max, sum, attention)
- Batch processing for efficiency
- Incremental updates when graph changes
- Multiple model versions support
- Metadata tracking (model version, timestamp, source)

**Usage:**
```cpp
GNNEmbeddingManager gnn_mgr(property_graph, vector_index);

// Register GNN model
GNNModelConfig model_config{
    .model_type = "GraphSAGE",
    .input_dim = 128,
    .hidden_dim = 256,
    .output_dim = 512,
    .num_layers = 3,
    .aggregator = "mean"
};
gnn_mgr.registerModel("user_embeddings", model_config);

// Generate node embeddings
gnn_mgr.generateNodeEmbeddings("social_graph", node_ids, "user_embeddings");

// Update single node
gnn_mgr.updateNodeEmbedding("social_graph", "alice", "user_embeddings");

// Generate edge embeddings
gnn_mgr.generateEdgeEmbeddings("social_graph", edge_ids, "user_embeddings");

// Graph-level embedding
auto graph_emb = gnn_mgr.generateGraphEmbedding("social_graph", 
                                                  "user_embeddings",
                                                  AggType::ATTENTION);

// Find similar nodes
auto similar = gnn_mgr.findSimilarNodes("social_graph", "alice", 
                                         "user_embeddings", k=10);

// Batch generation
gnn_mgr.generateNodeEmbeddingsBatch("social_graph", all_node_ids, 
                                     "user_embeddings", batch_size=100);
```

**Storage Keys:**
```
Node: node_emb:<graph_id>:<node_pk>:<model_name> → <embedding_vector>
Edge: edge_emb:<graph_id>:<edge_id>:<model_name> → <embedding_vector>
```

**Performance:**
- Node embedding generation: 1-10ms per node (depends on neighborhood size)
- Batch generation: 10-100 nodes/sec (depends on model complexity)
- Similarity search: Same as VectorIndexManager (1-5ms for k=10)

### HNSW Optimizations

#### HnswParameterTuner
**Location:** `hnsw_parameter_tuner.cpp`, `../include/index/hnsw_parameter_tuner.h`

Adaptive HNSW search parameter tuning for query performance optimization.

**Workload Types:**
- **OLTP**: High-throughput, low-latency, small k values
- **ANALYTICS**: Large k, batch queries, higher latency tolerance
- **MIXED**: Balanced workload
- **RAG**: Retrieval-Augmented Generation (medium k, high recall)
- **BATCH_INSERT**: Bulk indexing optimization

**Performance Gains:**
- Optimal ef_search: +15-25% faster at same recall
- Reduced over-searching: -10-20% CPU usage
- Workload-specific: +20-35% throughput improvement

**Usage:**
```cpp
HnswParameterTuner tuner(workload_type=WorkloadType::RAG);

// Get optimal ef_search for query
int ef = tuner.getOptimalEfSearch(k=10, target_recall=0.95);

// Record query result for adaptation
QueryResult result{
    .query_id = "query_1",
    .k = 10,
    .ef_search = 100,
    .recall = 0.97,
    .latency_ms = 2.5
};
tuner.recordQueryResult(result);

// Get create-time recommendations
int M = tuner.getRecommendedM(dataset_size=1000000);
int ef_construction = tuner.getRecommendedEfConstruction(dataset_size=1000000);

// Get workload-optimized config
auto config = tuner.getWorkloadOptimizedConfig();
std::cout << "ef_search_default: " << config.ef_search_default << "\n";
std::cout << "ef_search_min: " << config.ef_search_min << "\n";
std::cout << "ef_search_max: " << config.ef_search_max << "\n";

// Get statistics
auto stats = tuner.getStats();
std::cout << "Average recall: " << stats.avg_recall << "\n";
std::cout << "Average latency: " << stats.avg_latency_ms << "ms\n";
```

**Key Parameters:**
- **Fixed at creation**: M, ef_construction
- **Tunable at query**: ef_search_min, ef_search_max, ef_search_default
- **Optimization targets**: target_recall, target_latency

#### HnswLayerOptimizer
**Location:** `hnsw_layer_optimizer.cpp`, `../include/index/hnsw_layer_optimizer.h`

Layer pruning and adaptive layer selection to reduce traversal complexity.

**Optimizations:**
1. **Layer Pruning**: Skip deeper layers when sufficient candidates found
   - When `candidate_count > k × threshold`, skip remaining layers
2. **Adaptive Layer Selection**: Choose optimal entry layer based on statistics
   - Higher layers for exploratory queries
   - Lower layers when sufficient density
3. **Batch Insert Optimization**: Optimized bulk loading

**Complexity Reduction:**
- Baseline HNSW: O(log²N) per query
- Optimized: ~O(log N) per query
- Speedup: 20-40% for typical queries

**Usage:**
```cpp
HnswLayerOptimizer optimizer(num_layers=5);

// Record layer access during search
LayerAccessInfo access{
    .layer = 3,
    .candidates_found = 25,
    .search_time_ms = 0.5,
    .pruned = false
};
optimizer.recordLayerAccess(access);

// Record query statistics
QueryStats stats{
    .query_id = "query_1",
    .entry_layer = 4,
    .ef_used = 100,
    .layers_traversed = 3,
    .k = 10,
    .total_time_ms = 2.5
};
optimizer.recordQueryStats(stats);

// Get optimal entry layer
int entry_layer = optimizer.getOptimalEntryLayer();

// Get optimal ef
int ef = optimizer.getOptimalEf(k=10);

// Check if layer should be pruned
bool should_prune = optimizer.shouldPruneLayer(layer=2, candidate_count=50, k=10);

// Get layer statistics
auto layer_stats = optimizer.getLayerStats(layer=3);
std::cout << "Access count: " << layer_stats.access_count << "\n";
std::cout << "Efficiency score: " << layer_stats.efficiency_score << "\n";
```

**Metrics Tracked:**
- Per-layer: access_count, candidates_found, search_time_ms, efficiency_score
- Per-query: entry_layer, ef_used, layers_traversed, k, total_time_ms

### Additional Components

#### Multi-Vector Search
**Location:** `multi_vector_search.cpp`, `../include/index/multi_vector_search.h`

**Features:**
- Ensemble search (multiple query vectors)
- Query expansion (generate variants)
- Hybrid search (combine dense + sparse)
- Learned fusion (trainable combination)

#### Approximate Radius Search
**Location:** `approximate_radius_search.cpp`, `../include/index/approximate_radius_search.h`

**Features:**
- Find all vectors within distance threshold
- HNSW-based approximation
- Configurable precision vs recall trade-off

#### Edge Types
**Location:** `edge_types.cpp`, `../include/index/edge_types.h`

**Features:**
- Typed edges for heterogeneous graphs
- Edge type filtering in queries
- Multi-relational graph support

#### Property Graph
**Location:** `property_graph.cpp`, `../include/index/property_graph.h`

**Features:**
- Property storage for nodes and edges
- Schema-flexible attributes
- Integration with GraphIndexManager

#### Temporal Graph
**Location:** `temporal_graph.cpp`, `../include/index/temporal_graph.h`

**Features:**
- Time-based edge queries
- Temporal aggregations (COUNT, SUM, AVG, MIN, MAX)
- Snapshot queries (graph at specific time)

## Architecture

### Layered Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                      Index API Layer                            │
│   (VectorIndexManager, SecondaryIndexManager, etc.)            │
└────────────────────────────────────────────────────────────────┘
                             ↓
┌────────────────────────────────────────────────────────────────┐
│                   Optimization Layer                            │
│   (HNSW Tuner, Layer Optimizer, Adaptive Manager)             │
└────────────────────────────────────────────────────────────────┘
                             ↓
┌────────────────────────────────────────────────────────────────┐
│                   Algorithm Layer                               │
│   (HNSW, R-tree, B-tree, Graph Traversal)                     │
└────────────────────────────────────────────────────────────────┘
                             ↓
┌────────────────────────────────────────────────────────────────┐
│                   Acceleration Layer                            │
│   (GPU: Vulkan/CUDA/HIP, Quantization, RoPE)                  │
└────────────────────────────────────────────────────────────────┘
                             ↓
┌────────────────────────────────────────────────────────────────┐
│                   Storage Layer                                 │
│   (RocksDB via storage module)                                 │
└────────────────────────────────────────────────────────────────┘
```

### Vector Search Data Flow

```
Query Request
     ↓
[HnswParameterTuner] → Determine optimal ef_search
     ↓
[HnswLayerOptimizer] → Select entry layer
     ↓
[HNSW Algorithm] → Traverse graph layers
     ↓
[Quantization] → Compute distances (asymmetric distance)
     ↓
[GPU Acceleration] → Batch process on GPU (if enabled)
     ↓
[Secondary Index] → Apply filters (if pre-filtering)
     ↓
[RocksDB] → Fetch vector data
     ↓
[Post-Filter] → Apply filters (if post-filtering)
     ↓
Results (top-k vectors)
```

### Graph Search Data Flow

```
Traversal Request (BFS/Dijkstra/A*)
     ↓
[GraphIndexManager] → Load adjacency lists
     ↓
[Path Constraints] → Check forbidden/required vertices
     ↓
[Edge Type Filter] → Filter by edge types
     ↓
[Temporal Filter] → Filter by time range (if temporal)
     ↓
[Property Filter] → Check edge properties (weights, etc.)
     ↓
[RocksDB] → Fetch neighbor data
     ↓
[Traversal Algorithm] → Expand frontier
     ↓
Path Result
```

### Spatial Search Data Flow

```
Spatial Query (Intersects/Within/KNN)
     ↓
[SpatialIndexManager] → Encode query region (Morton code)
     ↓
[R-tree Traversal] → Find candidate MBRs
     ↓
[Z-Bucket Filter] → Filter by elevation (if 3D)
     ↓
[Distance Calculation] → Haversine or Euclidean
     ↓
[RocksDB] → Fetch geometry data (sidecar)
     ↓
[Exact Geometry Check] → Verify intersection (if enabled)
     ↓
Results (matching geometries)
```

### Thread Safety Model

**All Index Managers:**
- Multiple concurrent readers (thread-safe)
- Multiple concurrent writers (internal locking)
- Transaction-safe (atomic WriteBatch operations)
- Iterator-safe (reference counting)

**GPU Vector Index:**
- Thread-safe backend switching
- Thread-safe batch operations
- Internal queue for GPU command submission

**Quantizers:**
- Thread-safe encode/decode (stateless after training)
- Training NOT thread-safe (single-threaded)

**RoPE Implementations:**
- Thread-safe rotation (stateless)
- Training NOT thread-safe (LearnableRoPE)

## Integration Points

### With Storage Module
All indexes persist to RocksDB via storage module:
```cpp
VectorIndexManager vector_idx(rocksdb_wrapper);
SecondaryIndexManager sec_idx(rocksdb_wrapper);
GraphIndexManager graph_idx(rocksdb_wrapper);
SpatialIndexManager spatial_idx(rocksdb_wrapper);
```

### With Core Module
Uses ConcernsContext for observability:
```cpp
vector_idx.setConcerns(concerns_context);
// Enables logging, tracing, metrics for index operations
```

### With Query Module
Provides IExpressionEvaluator for filtered queries:
```cpp
auto results = vector_idx.searchKnnFiltered(query_vector, k, filter_expr);
```

### With Property Graph
GNN embeddings integrate with PropertyGraphManager:
```cpp
GNNEmbeddingManager gnn(property_graph, vector_index);
```

## API/Usage Examples

### Basic Vector Search

```cpp
#include "index/vector_index.h"

// Create vector index
VectorIndexManager vector_idx(rocksdb_wrapper);

// Initialize with HNSW
HnswParams params{.M=16, .ef_construction=200, .ef_search=100};
vector_idx.init("embeddings", 1536, DistanceMetric::COSINE, params);

// Add vectors
std::vector<float> embedding(1536);
vector_idx.addEntity("embeddings", "doc_1", embedding);

// Search
auto results = vector_idx.searchKnn("embeddings", query, k=10);
for (auto& [pk, distance] : results) {
    std::cout << pk << ": " << distance << "\n";
}
```

### GPU-Accelerated Search

```cpp
#include "index/gpu_vector_index.h"

// Create GPU index
GPUVectorIndexConfig config{
    .dimension = 1536,
    .metric = DistanceMetric::L2,
    .backend = GPUBackend::VULKAN,
    .batch_size = 512
};

GPUVectorIndex gpu_idx(config);

// Add vectors in batch
gpu_idx.addVectorBatch(vectors, ids);

// Batch search
std::vector<std::vector<float>> queries(512);
auto results = gpu_idx.searchBatch(queries, k=10);
```

### Secondary Index Query

```cpp
#include "index/secondary_index.h"

// Create index
SecondaryIndexManager sec_idx(rocksdb_wrapper);
sec_idx.createIndex("users", "email", IndexType::REGULAR);

// Query
auto results = sec_idx.scanKeysEqual("users", "email", "alice@example.com");
```

### Graph Traversal

```cpp
#include "index/graph_index.h"

// Create graph index
GraphIndexManager graph_idx(rocksdb_wrapper);

// Add edges
graph_idx.addEdge("social", "alice", "bob", "edge_1", {});

// BFS
auto bfs_result = graph_idx.bfs("social", "alice", max_depth=3);

// Dijkstra
auto path = graph_idx.dijkstra("social", "alice", "charlie");
```

### Spatial Query

```cpp
#include "index/spatial_index.h"

// Create spatial index
SpatialIndexManager spatial_idx(rocksdb_wrapper);
SpatialConfig config{.dimensions=2, .use_haversine=true};
spatial_idx.createSpatialIndex("places", "location", config);

// KNN search
Point query{37.7749, -122.4194};  // San Francisco
auto nearest = spatial_idx.searchKNN("places", "location", query, k=5);
```

### Quantized Vector Search

```cpp
#include "index/product_quantizer.h"
#include "index/vector_index.h"

// Train quantizer
ProductQuantizerConfig config{
    .dimension = 1536,
    .num_subquantizers = 8,
    .num_centroids = 256
};
ProductQuantizer pq(config);
pq.train(training_vectors);

// Enable in vector index
vector_idx.setQuantizer("embeddings", std::make_shared<ProductQuantizer>(pq));

// Searches now use quantized vectors automatically
auto results = vector_idx.searchKnn("embeddings", query, k=10);
```

### RoPE Positional Encoding

```cpp
#include "index/rotary_embeddings.h"

// Create RoPE
RotaryEmbedding rope(dimension=1536);

// Rotate by position
auto rotated = rope.rotate(embedding, position=42);

// Relational rotation for knowledge graphs
auto rel_rotated = rope.rotateRelational(head_embedding, "works_at");
```

### GNN Embeddings

```cpp
#include "index/gnn_embeddings.h"

// Create GNN manager
GNNEmbeddingManager gnn(property_graph, vector_index);

// Register model
GNNModelConfig config{
    .model_type = "GraphSAGE",
    .hidden_dim = 256,
    .output_dim = 512
};
gnn.registerModel("user_model", config);

// Generate embeddings
gnn.generateNodeEmbeddings("social", node_ids, "user_model");

// Find similar nodes
auto similar = gnn.findSimilarNodes("social", "alice", "user_model", k=10);
```

### Adaptive Index Suggestions

```cpp
#include "index/adaptive_index.h"

// Create adaptive manager
AdaptiveIndexManager adaptive(rocksdb_wrapper);

// Track patterns
QueryPattern pattern{
    .table = "users",
    .fields = {"email"},
    .operation = Operation::EQUALITY,
    .execution_time_ms = 150
};
adaptive.recordPattern(pattern);

// Get suggestions
auto suggestions = adaptive.generateSuggestions();
for (auto& suggestion : suggestions) {
    std::cout << "Create index on " << suggestion.table 
              << "." << suggestion.field << "\n";
}
```

## Dependencies

### Internal Dependencies
- **themis/base/interfaces**: Index interface definitions
- **storage/rocksdb_wrapper**: RocksDB persistence layer
- **core/concerns**: Logging, tracing, metrics
- **utils/expected**: Result types
- **utils/tracing**: Tracing utilities

### External Dependencies
- **RocksDB** (required): Persistent storage backend
- **FAISS** (optional): Advanced vector indexing and quantization
- **Vulkan SDK** (optional): GPU acceleration via Vulkan
- **CUDA Toolkit** (optional): GPU acceleration via CUDA
- **HIP SDK** (optional): GPU acceleration via HIP
- **Eigen** (optional): Matrix operations for GNN
- **fmt** (required): String formatting
- **spdlog** (optional): Logging

### Build Configuration
```cmake
# Vector index options
option(THEMIS_ENABLE_HNSW "Enable HNSW support" ON)
option(THEMIS_ENABLE_FAISS "Enable FAISS integration" ON)

# GPU backend options
option(THEMIS_ENABLE_GPU_VULKAN "Enable Vulkan GPU backend" ON)
option(THEMIS_ENABLE_GPU_CUDA "Enable CUDA GPU backend" OFF)
option(THEMIS_ENABLE_GPU_HIP "Enable HIP GPU backend" OFF)

# Feature options
option(THEMIS_ENABLE_GNN "Enable GNN embeddings" ON)
option(THEMIS_ENABLE_SPATIAL "Enable spatial indexing" ON)
option(THEMIS_ENABLE_QUANTIZATION "Enable vector quantization" ON)
```

## Performance Characteristics

### Vector Search Performance

| Operation | Brute-Force | HNSW (CPU) | GPU (Vulkan) |
|-----------|-------------|------------|--------------|
| Point lookup | 50-200μs | 50-200μs | 100-500μs |
| KNN (k=10, 1M vectors) | 50-500ms | 1-5ms | 500μs-2ms |
| Batch (512 queries) | 30-200s | 0.5-2.5s | 100-500ms |
| Insert (single) | 50-200μs | 1-5ms | 100-500μs |
| Batch insert (1K) | 50-200ms | 1-5s | 100-500ms |
| Memory (1M vectors, 1536D) | 6GB | 6.5GB | 6GB (RAM) + 4GB (VRAM) |

### Quantization Trade-offs

| Quantizer | Compression | Recall@95 | Encoding | Distance | Use Case |
|-----------|-------------|-----------|----------|----------|----------|
| None | 1x | 100% | - | 1-5μs | Small datasets |
| Binary | 32x | 80-90% | 1-5μs | 10-50ns | Maximum compression |
| Product | 8-16x | 95-98% | 10-50μs | 1-5μs | Balanced |
| Residual | 8-12x | 97-99% | 30-150μs | 3-15μs | High accuracy |
| Learned | 8-16x | 96-99% | 20-100μs | 2-10μs | Research only |

### Secondary Index Performance

| Operation | Hash Index | B-tree Index | Composite Index |
|-----------|------------|--------------|-----------------|
| Equality lookup | 10-50μs | 50-200μs | 100-500μs |
| Range scan (1K keys) | N/A | 5-20ms | 10-30ms |
| Insert | 50-200μs | 100-500μs | 200-1000μs |
| Update | 100-500μs | 200-1000μs | 500-2000μs |

### Graph Traversal Performance

| Operation | Performance | Notes |
|-----------|-------------|-------|
| Add edge | 100-500μs | Two writes (in + out) |
| Neighbor query | 50-200μs | Prefix scan |
| BFS (depth 3) | 1-10ms | Depends on branching factor |
| Dijkstra (path len 5) | 5-50ms | Depends on graph density |
| Temporal query (1h range) | 10-100ms | Depends on edge count |

### Spatial Index Performance

| Operation | 2D | 3D |
|-----------|----|----|
| Insert | 200-1000μs | 500-2000μs |
| Intersects query | 5-50ms | 10-100ms |
| KNN (k=10) | 10-100ms | 20-200ms |
| Radius search (10km) | 5-100ms | 10-200ms |

### Memory Footprint

| Component | Memory per Entry |
|-----------|------------------|
| Vector (1536D) | 6KB (float32) |
| Vector + HNSW | 6.5KB (50 bytes overhead) |
| Vector + Quantized (PQ) | 384-768 bytes (8-16x compression) |
| Secondary index entry | 50-100 bytes |
| Graph edge | 100-200 bytes (in + out indices) |
| Spatial index entry | 100-200 bytes (R-tree node) |
| GNN embedding | Same as vector |

## Known Limitations

### General Limitations

1. **Single-Node Only**
   - All indexes are local to single RocksDB instance
   - No built-in distributed indexing
   - Sharding requires external coordination

2. **RocksDB Constraints**
   - No cross-shard transactions
   - Limited secondary index support (manual management)
   - Write amplification from LSM-tree

3. **Memory Requirements**
   - HNSW requires holding graph structure in memory
   - GPU indexes require VRAM (limits capacity)
   - Large datasets may require quantization

### Vector Search Limitations

1. **HNSW Characteristics**
   - Approximate search (not exact)
   - Recall depends on ef_search parameter
   - Build time increases with dataset size (O(N log N))

2. **Quantization Trade-offs**
   - Accuracy loss (5-20% depending on method)
   - Training requires representative data
   - Not suitable for all distance metrics

3. **GPU Acceleration**
   - Batch queries required for efficiency
   - VRAM limits dataset size
   - Backend availability varies by platform

### Graph Limitations

1. **Traversal Algorithms**
   - No distributed graph traversal
   - Path algorithms limited by memory
   - Large graphs may require sampling

2. **Temporal Graphs**
   - Time-based queries slower than static queries
   - Historical data increases storage

### Spatial Limitations

1. **R-tree Characteristics**
   - Approximate queries (MBR candidates)
   - Degenerate geometries affect performance
   - 3D queries slower than 2D

2. **Distance Calculations**
   - Haversine assumes spherical Earth (not oblate spheroid)
   - Euclidean 3D doesn't account for Earth curvature

### Known Issues

1. **LearnedQuantizer**: Research-only, not production-ready
2. **GPU Backend**: CUDA and HIP support experimental
3. **GNN Embeddings**: Requires external model training
4. **Composite Indexes**: Limited to 5 columns
5. **Fulltext Search**: No phrase proximity scoring

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- Vector indexing with HNSW
- GPU acceleration (Vulkan backend)
- Secondary indexes (regular, composite, range, sparse)
- Graph adjacency indexing with BFS/Dijkstra
- Spatial R-tree indexing (2D)
- Product quantization
- Binary quantization
- Residual quantization
- Rotary positional embeddings
- HNSW parameter tuning
- HNSW layer optimization
- Multi-vector search
- Approximate radius search
- Temporal graph queries

⚠️ **Beta Features:**
- GPU acceleration (CUDA backend)
- Adaptive index management
- GNN embeddings
- LoRA-RoPE
- Learnable RoPE
- Spatial indexing (3D with Z-buckets)
- Fulltext search with BM25
- TTL indexes with auto-cleanup

🔬 **Experimental:**
- GPU acceleration (HIP backend)
- Learned quantization
- Graph attention networks (GAT)
- Exact geometry backend for spatial queries
- Erasure coding for vector backups

## Related Documentation

### Quick Links

- **Core Components:**
  - [Vector Index](../../docs/de/src/index/vector_index.cpp.md) - Vector similarity search
  - [Secondary Index](../../docs/de/src/index/secondary_index.cpp.md) - Secondary indexing
  - [Graph Index](../../docs/de/src/index/graph_index.cpp.md) - Graph adjacency indexing
  - [Spatial Index](../../docs/de/src/index/spatial_index.cpp.md) - Spatial R-tree indexing
  - [Adaptive Index](../../docs/de/src/index/adaptive_index.cpp.md) - Adaptive index selection
  - [GNN Embeddings](../../docs/de/src/index/gnn_embeddings.cpp.md) - Graph neural network embeddings
- **Advanced Features:**
  - [Vector Advanced Features](./VECTOR_ADVANCED_FEATURES_README.md) - Detailed vector search guide
  - [Approximate Radius Search](../../docs/ApproximateRadiusSearch.md) - Radius-based search
  - [Multi-Vector Search](../../docs/multi_vector_search.md) - Ensemble and hybrid search
  - [HNSW Persistence](../../docs/hnsw_persistence.md) - HNSW index persistence
- **Architecture:**
  - [Vector Indexing Architecture](../../VECTOR_INDEXING_ARCHITECTURE.md) - High-level architecture
  - [Storage Module](../storage/README.md) - Storage layer integration
  - [Core Module](../core/README.md) - Cross-cutting concerns

## Contributing

When contributing to the index module:

1. Maintain thread safety guarantees for all operations
2. Add benchmarks for new index types
3. Update key schema documentation for new encodings
4. Test with large datasets (1M+ entries)
5. Consider memory footprint and build time
6. Document accuracy/performance trade-offs
7. Add integration tests with storage module

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned index improvements
- [Core Module](../core/README.md) - Cross-cutting concerns infrastructure
- [Storage Module](../storage/README.md) - Persistent storage layer
- [Query Module](../query/README.md) - Query execution engine
