> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Index Module

## Module Purpose

The Index module provides high-performance indexing infrastructure for ThemisDB, supporting multiple index types across all five data models (relational, document, graph, vector, timeseries). It implements state-of-the-art algorithms including HNSW for vector similarity search, R-tree for spatial queries, and adaptive index recommendations based on query patterns.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `hnsw_index.cpp` | HNSW vector similarity index with GPU acceleration support |
| `rtree_index.cpp` | R-tree spatial index for geospatial queries |
| `adaptive_index.cpp` | Automatic index recommendation based on query patterns |
| `graph_index_manager.cpp` | Graph adjacency list indexing and traversal support |
| `secondary_index.cpp` | B-tree, range, sparse, and composite secondary indexes |
| `inverted_index.cpp` | Full-text inverted index with BM25 scoring and positional posting lists |
| `learned_index.cpp` | ML-based learned index structures (RMI, B-tree replacement) |
| `tiered_index_manager.cpp` | Hot/warm/cold tier index migration with automatic promotion/demotion |
| `index_compression.cpp` | Index compression: delta, prefix, Bloom filter, dictionary, RLE |

## Scope

**In Scope:**
- Vector indexing (HNSW, GPU-accelerated similarity search)
- Secondary indexes (B-tree, range, sparse, composite, partial/filtered)
- Graph indexing (adjacency lists, BFS/DFS traversal)
- Spatial indexing (R-tree for geospatial queries, Z-order curves)
- Full-text search (InvertedIndex with BM25 scoring and positional posting lists)
- Adaptive indexing (automatic index recommendations, workload replay advisor)
- GPU acceleration (Vulkan, CUDA, HIP support)
- Quantization for memory efficiency (Product Quantization, Binary Quantization, Residual Quantization)
- Multi-distance metrics (L2, Cosine, Dot Product)
- Advanced features (GNN embeddings, temporal graphs, rotary embeddings)
- Tiered index migration (hot/warm/cold storage tiers)
- Index compression (delta encoding, prefix compression, dictionary encoding, RLE, Bloom filters)
- Learned indexes (ML-based RMI structures for ordered-key lookup)

**Out of Scope:**
- Query parsing and execution (handled by query module)
- Data persistence (handled by storage module)
- Network protocols and APIs (handled by server module)

## Key Components

### IndexManager
**Location:** `index_manager.cpp`, `../include/index/index_manager.h`

Unified index manager coordinating all index types with dependency injection support.

```cpp
// Create index manager with dependency injection
auto evaluator = std::make_shared<ExpressionEvaluator>();
auto storage = std::make_shared<RocksDBWrapper>(config);
auto index_manager = std::make_shared<IndexManager>(evaluator, storage);

// Or use default factory
auto index_manager = IndexManager::createDefault();

// Access specialized managers
auto vector_mgr = index_manager->getVectorIndexManager();
auto secondary_mgr = index_manager->getSecondaryIndexManager();
auto graph_mgr = index_manager->getGraphIndexManager();
```

**Features:**
- Breaks circular dependencies (Index ↔ Query ↔ Storage)
- Enables isolated unit testing with mock implementations
- Supports filter expressions via injected evaluator
- Maintains backward compatibility

### VectorIndexManager
**Location:** `vector_index.cpp`, `../include/index/vector_index.h`

High-performance vector similarity search with HNSW algorithm and optional GPU acceleration.

**Algorithm:** HNSW (Hierarchical Navigable Small World graphs) - Malkov & Yashunin (2018), IEEE TPAMI

**Features:**
- **HNSW Index**: Approximate nearest neighbor (ANN) search
- **GPU Acceleration**: Vulkan/CUDA/HIP support via `GPUVectorIndex`
- **Multiple Metrics**: L2 (Euclidean), Cosine, Dot Product
- **Quantization**: Product Quantization (PQ), Binary Quantization, Residual Quantization
- **Advanced Features**: IVF+PQ, FAISS integration, multi-vector search
- **Production Defaults**: Optimized parameters via `HnswProductionDefaults`
- **Workload Auto-Tuner**: Runtime construction-parameter selection via `HnswParameterTuner` and `WorkloadClassifier`
- **RocksDB Persistence**: Atomic updates via WriteBatch
- **Audit Logging**: Track vector operations for compliance

**Basic Usage:**
```cpp
RocksDBWrapper db(config);
VectorIndexManager vim(db);

// Initialize with dimension and metric
vim.init("embeddings", 1536, VectorIndexManager::Metric::COSINE);

// Add vectors (persisted to RocksDB)
std::vector<float> embedding = model.encode(text);
vim.addVector("embeddings", "doc123", embedding);

// Search for similar vectors
auto results = vim.search("embeddings", query_vector, /*k=*/10);
for (const auto& result : results) {
    std::cout << "ID: " << result.pk
              << " Distance: " << result.distance << std::endl;
}
```

**Advanced Configuration:**
```cpp
// Enable FAISS IVF+PQ for large-scale datasets
VectorIndexManager::AdvancedIndexConfig adv_config;
adv_config.enabled = true;
adv_config.index_type = VectorIndexManager::AdvancedIndexConfig::Type::IVF_PQ;
adv_config.nlist = 1024;      // Number of IVF clusters
adv_config.nprobe = 64;       // Clusters to search
adv_config.use_pq = true;     // Enable Product Quantization
adv_config.pq_m = 8;          // 8 sub-quantizers
adv_config.use_gpu = true;    // GPU acceleration
adv_config.gpu_device = 0;

vim.setAdvancedIndexConfig(adv_config);

// Production-optimized HNSW parameters
HnswProductionDefaults defaults;
auto hnsw_config = defaults.getConfig(
    HnswProductionDefaults::UseCaseProfile::RAG,  // Use case
    1000000,                                       // Dataset size
    HnswProductionDefaults::LatencyRequirement::P99_SUB_10MS
);
vim.setHnswConfig("embeddings", hnsw_config);
```

**Workload-Adaptive Construction Parameters (Auto-Tuner):**
```cpp
// Option A: Classify workload automatically from observed traffic
WorkloadClassifier classifier;

// Record events as they occur (thread-safe)
classifier.recordInsert(1);        // single-vector inserts → OLTP signal
classifier.recordQuery(/*k=*/10);  // medium-k queries → RAG signal

// Detect workload type from accumulated stats
auto workload = classifier.detectWorkload();
// → OLTP / ANALYTICS / RAG / BATCH_INSERT / MIXED

// Option B: Ask the tuner to pick M and ef_construction automatically
HnswParameterTuner::Config tuner_cfg;
tuner_cfg.adaptive = true;
tuner_cfg.target_recall = 0.95;
HnswParameterTuner tuner(tuner_cfg);

// Record a few warm-up queries, then ask for construction params
tuner.recordQueryResult(/*k=*/10, /*ef=*/64, /*latency_ms=*/3.0, /*recall=*/0.96);
auto cp = tuner.getAutoTunedConstructionParams(/*dataset_size=*/500000);
// cp.M, cp.ef_construction, cp.detected_workload are all set

// Option C: Point-in-time target-based tuning (no warm-up needed)
auto params = HnswProductionDefaults::autoTuneParameters(
    500000,   // dataset size
    768,      // vector dimension
    100,      // sample_size (currently informational)
    10.0,     // target latency ms
    0.95      // target recall
);
// params.M, params.ef_construction, params.ef_search selected for the targets
```

**Quantization for Memory Efficiency:**
```cpp
// Product Quantization: 10-100x compression
ProductQuantizer::Config pq_config;
pq_config.num_subquantizers = 8;     // Divide 1536D into 8 subspaces
pq_config.num_centroids = 256;       // 8-bit codes
pq_config.prefer_faiss = true;       // Use FAISS K-means acceleration

auto pq = std::make_unique<ProductQuantizer>(pq_config);
pq->train(training_vectors);

// Encode vectors to 8-bit codes (1536D float32 -> 8 bytes)
std::vector<uint8_t> codes = pq->encode(vector);

// Asymmetric distance computation (ADC)
float distance = pq->compute_distance(codes, query_vector);
```

**GPU Acceleration:**
```cpp
// Cross-platform GPU acceleration via Vulkan
GPUVectorIndex::Config gpu_config;
gpu_config.backend = GPUVectorIndex::Backend::VULKAN;  // Cross-platform
gpu_config.metric = GPUVectorIndex::DistanceMetric::COSINE;
gpu_config.M = 16;                    // HNSW connections per layer
gpu_config.efConstruction = 200;      // Construction accuracy
gpu_config.efSearch = 64;             // Search accuracy
gpu_config.batchSize = 512;           // Parallel batch size

auto gpu_index = std::make_unique<GPUVectorIndex>(gpu_config);
gpu_index->addVectors(ids, vectors);

// Fast GPU search (200K+ queries/sec)
auto results = gpu_index->search(query_vector, /*k=*/10);
```

**Performance Characteristics:**
- Point queries: 0.1-1ms (HNSW)
- Batch search: 200K+ queries/sec (GPU)
- Memory: 1.5-2x of raw vectors (HNSW), 0.1-0.01x with PQ
- Recall@10: 95-99% (configurable via efSearch)

### SecondaryIndexManager
**Location:** `secondary_index.cpp`, `../include/index/secondary_index.h`

B-tree-based secondary indexes for equality, range, and composite queries.

**Key Schema:**
```
Single-column:  idx:table:column:value:PK
Composite:      idx:table:col1+col2:val1:val2:PK
Range:          range:table:column:value:PK (lexicographic ordering)
Sparse:         sparse:table:column:value:PK (skips NULL values)
Geo:            geo:table:column:morton_code:PK (Z-order curve)
TTL:            ttl:table:column:expire_ts:PK (time-to-live)
```

**Basic Usage:**
```cpp
SecondaryIndexManager sim(db);

// Create single-column index
sim.createIndex("users", "email", /*unique=*/true);

// Create composite index
sim.createCompositeIndex("orders", {"customer_id", "order_date"});

// Atomic updates via WriteBatch
auto batch = db.createWriteBatch();
BaseEntity user;
user.set("id", "user123");
user.set("email", "john@example.com");
user.set("name", "John Doe");

sim.updateIndex("users", user, batch);
batch.commit();

// Lookup by index
auto pks = sim.lookupByIndex("users", "email", "john@example.com");
```

**Range Queries:**
```cpp
// Create range index for ordered scans
sim.createRangeIndex("events", "timestamp");

// Range query [start, end]
auto results = sim.rangeQuery(
    "events", "timestamp",
    "2024-01-01", "2024-12-31",
    /*limit=*/100
);
```

**Sparse Indexes:**
```cpp
// Sparse index: skip NULL/missing values (reduces index size)
sim.createSparseIndex("products", "discount_code");

// Only indexes products with discount codes
// Products without discount_code are not in the index
```

**Geo Indexes (Z-order curves):**
```cpp
// Geospatial index with bounding box queries
sim.createGeoIndex("locations", "coordinates");

// Bounding box search
geo::MBR bbox{37.7, -122.5, 37.8, -122.4};  // San Francisco area
auto results = sim.geoQuery("locations", "coordinates", bbox);

// Radius search (within 5km of point)
auto nearby = sim.radiusQuery(
    "locations", "coordinates",
    /*lat=*/37.7749, /*lon=*/-122.4194, /*radius_km=*/5.0
);
```

**TTL Indexes:**
```cpp
// Automatic expiration after TTL
sim.createTTLIndex("sessions", "expires_at", /*ttl_seconds=*/3600);

// Cleanup expired entries (call periodically)
sim.cleanupExpired("sessions", "expires_at");
```

**Performance Characteristics:**
- Point lookups: 10-50μs (with RocksDB cache)
- Range scans: 100K-500K keys/sec
- Unique constraint checks: O(1) via bloom filters
- Space overhead: 20-40% of data size

### GraphIndexManager
**Location:** `graph_index.cpp`, `../include/index/graph_index.h`

Adjacency list indexes for graph traversal with temporal and property graph support.

**Key Schema:**
```
Outbound edges:  graph:out:<from_pk>:<edge_id>  -> <to_pk>
Inbound edges:   graph:in:<to_pk>:<edge_id>     -> <from_pk>
Edge properties: edge:<edge_id>                 -> properties
```

**Basic Usage:**
```cpp
GraphIndexManager gim(db);

// Add edge (bidirectional index)
BaseEntity edge;
edge.set("id", "edge123");
edge.set("_from", "user:alice");
edge.set("_to", "user:bob");
edge.set("type", "follows");
edge.set("since", "2024-01-01");

gim.addEdge(edge);

// Query neighbors
auto [status, out_neighbors] = gim.outNeighbors("user:alice");
for (const auto& neighbor_id : out_neighbors) {
    std::cout << "Alice follows: " << neighbor_id << std::endl;
}

auto [status2, in_neighbors] = gim.inNeighbors("user:bob");
// In-neighbors: users who follow Bob
```

**Graph Traversal:**
```cpp
// BFS traversal (breadth-first)
std::vector<std::string> visited;
gim.bfs("user:alice", [&visited](const std::string& node) {
    visited.push_back(node);
    return true;  // Continue traversal
}, /*max_depth=*/3);

// DFS traversal (depth-first)
gim.dfs("user:alice", [](const std::string& node) {
    std::cout << "Visiting: " << node << std::endl;
    return true;
}, /*max_depth=*/5);

// Shortest path (BFS-based)
auto path = gim.shortestPath("user:alice", "user:charlie");
```

**Multi-Graph Support:**
```cpp
// Multiple graphs in single database
edge.set("graphId", "social_network");
gim.addEdge(edge);

edge.set("graphId", "org_chart");
gim.addEdge(edge);

// Query specific graph
auto neighbors = gim.outNeighbors("user:alice", "social_network");
```

**Temporal Graphs:**
```cpp
// Temporal edges with time validity
TemporalGraph temporal_graph(db);
temporal_graph.addEdge(
    "user:alice", "user:bob", "follows",
    /*valid_from=*/"2024-01-01", /*valid_until=*/"2024-12-31"
);

// Time-travel queries (graph state at specific time)
auto neighbors_at = temporal_graph.getNeighborsAt(
    "user:alice", /*timestamp=*/"2024-06-01"
);
```

**Property Graphs:**
```cpp
// Rich edge properties
PropertyGraph prop_graph(db);
std::unordered_map<std::string, std::string> properties = {
    {"weight", "0.8"},
    {"label", "friend"},
    {"created", "2024-01-01"}
};
prop_graph.addEdge("user:alice", "user:bob", properties);

// Query by edge properties
auto edges = prop_graph.findEdgesByProperty("weight", "0.8");
```

**Graph Analytics:**
```cpp
// PageRank
GraphAnalytics analytics(gim);
auto scores = analytics.pageRank(/*iterations=*/10, /*damping=*/0.85);

// Community detection (Louvain algorithm)
auto communities = analytics.detectCommunities();

// Centrality measures
auto betweenness = analytics.betweennessCentrality("user:alice");
auto closeness = analytics.closenessCentrality("user:alice");
```

**Performance Characteristics:**
- Neighbor queries: 10-50μs (in-memory cache)
- BFS/DFS traversal: 100K-500K nodes/sec
- Shortest path: O(|V| + |E|) with BFS
- Space overhead: 2x edges (bidirectional index)

### SpatialIndexManager
**Location:** `spatial_index.cpp`, `../include/index/spatial_index.h`

R-tree implementation for efficient geospatial queries with Morton code (Z-order curve) optimization.

**Features:**
- **R-tree**: Hierarchical spatial index for 2D/3D data
- **Z-order curves**: Morton codes for spatial locality
- **Bounding boxes**: MBR (Minimum Bounding Rectangle) queries
- **Distance queries**: Radius search, nearest neighbors
- **GeoJSON support**: Point, LineString, Polygon
- **EWKB encoding**: Efficient binary geometry format

**Usage:**
```cpp
SpatialIndexManager::Config config;
config.max_entries_per_node = 16;
config.min_entries_per_node = 4;
config.total_bounds = geo::MBR{-180, -90, 180, 90};  // World bounds

SpatialIndexManager spatial(db, config);

// Insert geometry (GeoJSON Point)
spatial.insert(
    "locations", "poi123",
    /*lat=*/37.7749, /*lon=*/-122.4194,
    /*z=*/std::nullopt  // Optional Z-coordinate
);

// Bounding box search
geo::MBR query_bbox{37.7, -122.5, 37.8, -122.4};
auto results = spatial.searchIntersects("locations", query_bbox);

// Radius search (within 5km)
auto nearby = spatial.searchRadius(
    "locations",
    /*center_lat=*/37.7749, /*center_lon=*/-122.4194,
    /*radius_km=*/5.0,
    /*limit=*/10
);

// Nearest neighbors
auto nearest = spatial.kNearestNeighbors(
    "locations",
    /*lat=*/37.7749, /*lon=*/-122.4194,
    /*k=*/5
);
```

**Z-order Curves (Morton Encoding):**
```cpp
// Convert 2D coordinates to 1D Morton code
geo::MBR world_bounds{-180, -90, 180, 90};
uint64_t morton = MortonEncoder::encode2D(
    /*lon=*/-122.4194, /*lat=*/37.7749, world_bounds
);

// Range queries on Morton codes (efficient for nearby points)
auto ranges = MortonEncoder::getRanges(query_bbox, world_bounds);
for (const auto& [start, end] : ranges) {
    // Scan RocksDB range [start, end]
}
```

**Performance Characteristics:**
- Point queries: 10-100μs
- Bounding box queries: 1-10ms (depends on result size)
- Radius search: 1-20ms (depends on radius)
- R-tree height: O(log_M N) where M=16
- Space overhead: 30-50% of data size

### AdaptiveIndexManager
**Location:** `adaptive_index.cpp`, `../include/index/adaptive_index.h`

Automatic index recommendation based on query pattern analysis.

**Features:**
- **Query Pattern Tracking**: Field access frequency, filter predicates, join conditions
- **Cache-Aware Metrics**: L3 cache misses and miss penalties
- **Automatic Recommendations**: Suggests indexes based on cost-benefit analysis
- **Index Lifecycle**: Create/drop indexes based on usage patterns
- **Anomaly Detection**: Identifies slow queries requiring indexes

**Usage:**
```cpp
AdaptiveIndexManager adaptive(db);

// Track query patterns
adaptive.recordQuery(
    "users", "email",
    QueryPatternTracker::Operation::EQUALITY,
    /*execution_time_ms=*/45.2,
    /*cache_miss=*/true,
    /*cache_miss_penalty_ms=*/12.3
);

// Get index recommendations
auto recommendations = adaptive.getRecommendations(/*min_benefit=*/100.0);
for (const auto& rec : recommendations) {
    std::cout << "Create index on " << rec.collection
              << "." << rec.field
              << " (estimated benefit: " << rec.benefit_score << "ms/query)"
              << std::endl;
}

// Auto-create recommended indexes
adaptive.applyRecommendations(recommendations, secondary_index_manager);

// Periodic maintenance
adaptive.cleanupUnusedIndexes(/*unused_days=*/30);
```

**Index Recommendation Algorithm:**
```cpp
// Cost-benefit analysis
struct IndexRecommendation {
    std::string collection;
    std::string field;
    IndexType type;
    double benefit_score;      // Estimated query speedup (ms)
    double creation_cost;      // Index build time (ms)
    double space_cost_mb;      // Storage overhead
    double cache_improvement;  // Reduction in cache misses
};

// Ranking factors:
// 1. Query frequency (high = better)
// 2. Execution time improvement (high = better)
// 3. Cache miss reduction (high = better)
// 4. Selectivity (low cardinality = better for B-tree)
// 5. Space cost (low = better)
```

**Performance Characteristics:**
- Pattern tracking: < 1μs overhead per query
- Recommendation generation: 10-100ms
- Auto-index creation: 1-60s (depends on data size)

## Architecture

### Dependency Injection

The Index module uses dependency injection to break circular dependencies:

```
┌─────────────────┐
│  IndexManager   │
│  (Coordinator)  │
└────────┬────────┘
         │
         ├─► IExpressionEvaluator (injected from Query module)
         ├─► IStorageEngine (injected from Storage module)
         │
         ├─────────────────────────────────┐
         │                                 │
         ▼                                 ▼
┌────────────────────┐          ┌────────────────────┐
│ VectorIndexManager │          │SecondaryIndexMgr   │
└────────────────────┘          └────────────────────┘
         │                                 │
         ├─► GPUVectorIndex               ├─► SpatialIndexManager
         ├─► ProductQuantizer             ├─► RocksDBWrapper
         ├─► HnswLayerOptimizer           │
         └─► RotaryEmbedding              │
                                          ▼
                                 ┌────────────────────┐
                                 │ GraphIndexManager  │
                                 └────────────────────┘
                                          │
                                          ├─► TemporalGraph
                                          ├─► PropertyGraph
                                          └─► GraphAnalytics
```

### GPU Acceleration Pipeline

```
┌──────────────┐
│ Query Vector │
└──────┬───────┘
       │
       ▼
┌──────────────────────────┐
│  GPUVectorIndex          │
│  (Backend Selection)     │
└──────┬───────────────────┘
       │
       ├─► Vulkan Backend (Cross-platform: NVIDIA, AMD, Intel, Apple)
       ├─► CUDA Backend (NVIDIA GPUs)
       ├─► HIP Backend (AMD GPUs)
       └─► CPU Fallback (SIMD acceleration)
       │
       ▼
┌──────────────────────────┐
│  Compute Kernel          │
│  (Distance Computation)  │
└──────┬───────────────────┘
       │
       ▼
┌──────────────────────────┐
│  Top-K Selection         │
│  (Heap-based)            │
└──────┬───────────────────┘
       │
       ▼
┌──────────────┐
│   Results    │
└──────────────┘
```

### Quantization Pipeline

```
┌──────────────────┐
│  Float32 Vectors │
│  (1536D × 4B)    │
│  = 6KB/vector    │
└────────┬─────────┘
         │
         ▼
┌──────────────────────────┐
│  Product Quantization    │
│  (8 subquantizers)       │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────┐
│  8-bit Codes     │
│  (8 × 1B)        │
│  = 8B/vector     │
│  750x compression│
└────────┬─────────┘
         │
         ▼
┌──────────────────────────┐
│  Asymmetric Distance     │
│  Computation (ADC)       │
│  - Query: float32        │
│  - Database: uint8       │
│  - Precompute tables     │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────┐
│  Search Results  │
│  (95-99% recall) │
└──────────────────┘
```

## Integration Points

### With Storage Module

```cpp
// RocksDB persistence for all index types
RocksDBWrapper db(config);

// Vector index persistence
VectorIndexManager vim(db);
vim.init("embeddings", 1536, VectorIndexManager::Metric::COSINE);
vim.addVector("embeddings", "doc123", vector);  // Atomic WriteBatch

// Secondary index atomicity
SecondaryIndexManager sim(db);
auto batch = db.createWriteBatch();
sim.updateIndex("users", entity, batch);  // Index update in same batch
batch.commit();  // Atomic: data + indexes
```

### With Query Module

```cpp
// Expression evaluation for filter pushdown
auto evaluator = std::make_shared<ExpressionEvaluator>();
vim.setExpressionEvaluator(evaluator);

// Filter during vector search
auto results = vim.searchWithFilter(
    "embeddings", query_vector, /*k=*/10,
    /*filter=*/"metadata.category = 'science' AND year > 2020"
);
```

### With Server Module

```cpp
// API handler integration
void handleVectorSearch(const Request& req, Response& resp) {
    auto query_vector = req.getVector("vector");
    auto k = req.getInt("k", 10);

    auto results = vim.search("embeddings", query_vector, k);

    resp.setJson({
        {"results", results},
        {"count", results.size()}
    });
}
```

## API/Usage Examples

### Complete RAG Pipeline

```cpp
// Setup
RocksDBWrapper db(config);
VectorIndexManager vim(db);
SecondaryIndexManager sim(db);

// Initialize indexes
vim.init("documents", 1536, VectorIndexManager::Metric::COSINE);
sim.createIndex("documents", "category");
sim.createIndex("documents", "year");

// Ingest documents
for (const auto& doc : documents) {
    // Generate embedding
    auto embedding = model.encode(doc.text);

    // Store in vector index
    vim.addVector("documents", doc.id, embedding);

    // Store metadata
    BaseEntity entity;
    entity.set("id", doc.id);
    entity.set("category", doc.category);
    entity.set("year", doc.year);
    entity.set("text", doc.text);

    auto batch = db.createWriteBatch();
    sim.updateIndex("documents", entity, batch);
    batch.commit();
}

// Query with filters
auto query_embedding = model.encode(query_text);
auto results = vim.searchWithFilter(
    "documents", query_embedding, /*k=*/10,
    /*filter=*/"category = 'science' AND year > 2020"
);

// Retrieve full documents
for (const auto& result : results) {
    auto doc = db.get("documents:" + result.pk);
    std::cout << "Score: " << result.distance << std::endl;
    std::cout << "Text: " << doc["text"] << std::endl;
}
```

### Graph Knowledge Base

```cpp
// Build knowledge graph
GraphIndexManager gim(db);
VectorIndexManager vim(db);

// Add entities with embeddings
vim.init("entities", 768, VectorIndexManager::Metric::COSINE);
vim.addVector("entities", "entity:einstein", einstein_embedding);
vim.addVector("entities", "entity:relativity", relativity_embedding);

// Add relationships
BaseEntity edge;
edge.set("id", "edge123");
edge.set("_from", "entity:einstein");
edge.set("_to", "entity:relativity");
edge.set("type", "DISCOVERED");
edge.set("year", "1905");
gim.addEdge(edge);

// Hybrid query: vector similarity + graph traversal
auto similar = vim.search("entities", query_embedding, /*k=*/5);
for (const auto& entity : similar) {
    auto neighbors = gim.outNeighbors(entity.pk);
    // Expand context via graph
}
```

### Geospatial Search

```cpp
// Index locations
SpatialIndexManager spatial(db, config);
SecondaryIndexManager sim(db);

sim.createGeoIndex("restaurants", "location");

// Add restaurants
BaseEntity restaurant;
restaurant.set("id", "rest123");
restaurant.set("name", "Luigi's Pizza");
restaurant.set("lat", 37.7749);
restaurant.set("lon", -122.4194);
restaurant.set("category", "Italian");

auto batch = db.createWriteBatch();
sim.updateIndex("restaurants", restaurant, batch);
spatial.insert("restaurants", "rest123", 37.7749, -122.4194);
batch.commit();

// Find nearby restaurants
auto nearby = spatial.searchRadius(
    "restaurants",
    /*lat=*/37.7749, /*lon=*/-122.4194,
    /*radius_km=*/2.0,
    /*limit=*/10
);

// Filter by category
std::vector<SpatialResult> italian_restaurants;
for (const auto& result : nearby) {
    auto doc = db.get("restaurants:" + result.primary_key);
    if (doc["category"] == "Italian") {
        italian_restaurants.push_back(result);
    }
}
```

## Dependencies

### Required
- **RocksDB**: Key-value storage (via `storage/rocksdb_wrapper.h`)
- **nlohmann/json**: JSON serialization
- **Eigen3**: Linear algebra for vector operations (optional, fallback available)

### Optional
- **FAISS**: Facebook AI Similarity Search (advanced vector indexes, IVF+PQ)
  - Enables: IVF clustering, Product Quantization, GPU acceleration
  - License: MIT
- **HNSWlib**: HNSW implementation (if not using FAISS)
  - License: Apache 2.0
- **Vulkan**: Cross-platform GPU compute (via `acceleration/compute_backend.h`)
  - Linux: vulkan-devel, vulkan-loader
  - Windows: Vulkan SDK
  - macOS: MoltenVK
- **CUDA**: NVIDIA GPU acceleration (optional)
  - Requires: CUDA Toolkit 11.0+
- **HIP**: AMD GPU acceleration (optional)
  - Requires: ROCm 5.0+

### Build Configuration
```cmake
# Enable GPU acceleration
option(THEMIS_GPU_ENABLED "Enable GPU acceleration" ON)
option(THEMIS_HAS_FAISS "Enable FAISS integration" ON)
option(THEMIS_CUDA_ENABLED "Enable CUDA backend" ON)
option(THEMIS_VULKAN_ENABLED "Enable Vulkan backend" ON)

# Find packages
find_package(FAISS QUIET)
find_package(Vulkan QUIET)
find_package(CUDAToolkit QUIET)
```

## Performance Characteristics

### Vector Index (HNSW)
- **Construction**: O(N log N) where N = number of vectors
- **Memory**: 1.5-2x of raw vectors (HNSW graph), 0.01-0.1x with PQ
- **Search Latency**:
  - CPU (brute-force): 10-100ms per query (1M vectors)
  - CPU (HNSW): 0.1-1ms per query
  - GPU (Vulkan): 0.01-0.1ms per query (batch)
- **Throughput**:
  - CPU: 1K-10K queries/sec
  - GPU: 100K-500K queries/sec (batch size 512)
- **Recall@10**: 95-99% (configurable via efSearch)

### Secondary Index (B-tree)
- **Lookup**: O(log N) via RocksDB LSM-tree
- **Point query**: 10-50μs (with cache)
- **Range scan**: 100K-500K keys/sec
- **Space overhead**: 20-40% of data size
- **Unique constraint**: O(1) check via bloom filters

### Graph Index (Adjacency List)
- **Add edge**: O(1) with in-memory cache
- **Neighbor query**: O(degree) = 10-50μs typically
- **BFS/DFS**: O(|V| + |E|) = 100K-500K nodes/sec
- **Shortest path**: O(|V| + |E|) with BFS
- **Space overhead**: 2x edges (bidirectional index)

### Spatial Index (R-tree)
- **Construction**: O(N log N)
- **Point query**: O(log N) = 10-100μs
- **Bounding box**: O(log N + k) where k = result count
- **Radius search**: O(log N + k)
- **Tree height**: O(log_M N) where M=16 (fanout)
- **Space overhead**: 30-50% of data size

### Quantization Performance
- **Product Quantization (PQ)**:
  - Compression: 10-100x (e.g., 1536D float32 → 8B codes)
  - Search speedup: 2-5x (via ADC tables)
  - Recall: 95-99% (configurable)
  - Training: 1-10 minutes (100K vectors)
- **Binary Quantization**:
  - Compression: 256x (float32 → 1-bit)
  - Search speedup: 10-50x (bitwise ops)
  - Recall: 90-95%

## Known Limitations

### Vector Index
1. **HNSW Construction**: Cannot efficiently remove vectors (requires rebuild)
   - Workaround: Mark as deleted, rebuild periodically
2. **Memory Usage**: HNSW requires all vectors in memory
   - Workaround: Use IVF+PQ for disk-based indexes
3. **High-Dimensional Data**: Performance degrades beyond 2048D
   - Workaround: Use dimensionality reduction (PCA, UMAP)
4. **GPU Memory**: Limited by VRAM (typically 8-24GB)
   - Workaround: Batch processing, multi-GPU sharding

### Secondary Index
1. **Composite Index Ordering**: Fixed column order for prefix scans
   - Workaround: Create multiple indexes for different orderings
2. **Unique Constraint Race**: Possible with concurrent writes
   - Workaround: Use pessimistic transactions

### Graph Index
1. **No Distributed Traversal**: Single-node only
   - Workaround: Use distributed vector index sharding for ANN workloads
2. **Limited Cycle Detection**: Expensive on large graphs
   - Workaround: Set max_depth parameter
3. **Memory Usage**: In-memory cache for topology
   - Workaround: Disable cache for large graphs (> 10M edges)

### Spatial Index
1. **2D/3D Only**: No support for higher dimensions
   - Workaround: Use vector index for high-dimensional spatial data
2. **Global Bounds**: Must know bounds in advance
   - Workaround: Use conservative bounds, update periodically
3. **LineString/Polygon**: Limited to MBR approximation
   - Workaround: Post-filter results with exact geometry checks

## Status

**Current Version:** v1.7.0+

**Maturity:**
- Vector Index: ✅ **Production Ready** (v1.3.0+)
  - HNSW: Stable, well-tested
  - GPU Acceleration: Stable (Vulkan/CUDA/HIP)
  - FAISS Integration: Stable (v1.5.0+)
  - DiskANN / ScaNN: Stable (v1.6.0+)
  - Multi-GPU distributed search: Stable (v1.6.0+)
- Secondary Index: ✅ **Production Ready** (v1.0.0+)
  - Index Compression: Stable (v1.7.0+)
- Full-Text Index: ✅ **Production Ready** (v1.5.0+)
  - BM25 scoring: Stable
  - Positional posting lists: Stable
- Graph Index: ✅ **Production Ready** (v1.2.0+)
- Spatial Index: ⚠️ **Beta** (v1.4.0+)
  - R-tree: Stable
  - GeoJSON: Stable
  - Full geometry operations: In progress
- Adaptive Index: ✅ **Production Ready** (v1.5.0+)
  - Workload replay advisor: Stable (v1.6.0+)
- Tiered Index Manager: ✅ **Production Ready** (v1.6.0+)
- Learned Indexes: ✅ **Production Ready** (v1.7.0+)

**Recent Changes:**
- v1.7.0: Index compression (delta, prefix, dictionary, RLE, Bloom filters), learned index structures
- v1.6.0: Tiered index migration (hot/warm/cold), DiskANN/ScaNN, multi-GPU distributed index, workload replay advisor, HNSW incremental re-indexing
- v1.5.0: Full-text inverted index (BM25), FAISS integration, IVF+PQ, ADC tables, Product Quantization
- v1.4.2: Residual Quantization, Learned Quantization
- v1.4.0: Spatial index (R-tree), Z-order curves
- v1.3.0: GPU acceleration (Vulkan), rotary embeddings
- v1.2.0: Graph analytics, temporal graphs, property graphs
- v1.1.0: Composite indexes, sparse indexes, TTL indexes

**Stability:**
- Test Coverage: 85%+ (comprehensive unit + integration tests)
- Memory Leaks: Clean (Valgrind verified)
- Thread Safety: All managers are thread-safe
- Production Usage: Running in multiple deployments

## Related Documentation

### Module Documentation
- [Storage Module](../storage/README.md) - RocksDB integration, persistence
- [Query Module](../query/README.md) - Query execution, expression evaluation
- [AQL Module](../aql/README.md) - AQL syntax for index usage
- [Core Module](../core/README.md) - Dependency injection, concerns context

### Technical Documentation
- [Vector Indexing Architecture](../../VECTOR_INDEXING_ARCHITECTURE.md) - Deep dive into HNSW, FAISS, GPU acceleration
- [Vector Advanced Features](VECTOR_ADVANCED_FEATURES_README.md) - Quantization, rotary embeddings, multi-vector search
- [Benchmark Best Practices](../../BENCHMARK_BEST_PRACTICES.md) - Performance tuning, benchmarking

### API References
- [IIndexManager Interface](../../include/themis/base/interfaces/index_interface.h)
- [IExpressionEvaluator Interface](../../include/themis/base/interfaces/query_interface.h)
- [IStorageEngine Interface](../../include/themis/base/interfaces/storage_interface.h)

### Examples
- [RAG Pipeline Example](../../examples/rag_pipeline.cpp) - End-to-end RAG implementation
- [Graph Knowledge Base](../../examples/graph_knowledge_base.cpp) - Hybrid vector + graph
- [Geospatial Queries](../../examples/geospatial_queries.cpp) - Spatial index usage
- [GPU Acceleration](../../examples/gpu_vector_search.cpp) - Vulkan/CUDA usage

### External Resources
- [HNSW Paper](https://arxiv.org/abs/1603.09320) - Malkov & Yashunin (2018)
- [FAISS Documentation](https://github.com/facebookresearch/faiss/wiki)
- [Product Quantization Paper](https://hal.inria.fr/inria-00514462) - Jégou et al. (2011)
- [R-tree Paper](http://www-db.deis.unibo.it/courses/SI-LS/papers/Gut84.pdf) - Guttman (1984)

## Scientific References

1. Malkov, Y. A., & Yashunin, D. A. (2020). **Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs**. *IEEE Transactions on Pattern Analysis and Machine Intelligence*, 42(4), 824–836. https://doi.org/10.1109/TPAMI.2018.2889473

2. Guttman, A. (1984). **R-Trees: A Dynamic Index Structure for Spatial Searching**. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266

3. Bayer, R., & McCreight, E. (1972). **Organization and Maintenance of Large Ordered Indexes**. *Acta Informatica*, 1(3), 173–189. https://doi.org/10.1007/BF00288683

4. Beckmann, N., Kriegel, H.-P., Schneider, R., & Seeger, B. (1990). **The R*-Tree: An Efficient and Robust Access Method for Points and Rectangles**. *Proceedings of the 1990 ACM SIGMOD International Conference on Management of Data*, 322–331. https://doi.org/10.1145/93597.98741

5. Johnson, J., Douze, M., & Jégou, H. (2019). **Billion-Scale Similarity Search with GPUs**. *IEEE Transactions on Big Data*, 7(3), 535–547. https://doi.org/10.1109/TBDATA.2019.2921572

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
