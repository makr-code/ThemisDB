# CHIMERA Suite: Workload Reference Guide

**Version:** 1.0.0  
**Standards Compliance:** IEEE Std 2807-2022, ISO/IEC 14756:2015  
**Last Updated:** April 2026

---

## Table of Contents

1. [Introduction](#introduction)
2. [YCSB Workloads](#ycsb-workloads)
3. [TPC Benchmarks](#tpc-benchmarks)
4. [Vector Search Benchmarks](#vector-search-benchmarks)
5. [Graph Benchmarks](#graph-benchmarks)
6. [LLM Benchmarks](#llm-benchmarks)
7. [Custom Workload Definition](#custom-workload-definition)

---

## 1. Introduction

This guide provides detailed reference documentation for all standard workloads supported by the CHIMERA Suite vendor-neutral benchmark configuration format.

### 1.1 Workload Naming Convention

All workloads follow this naming pattern:
```
<family>_<variant>_<scale>
```

Examples:
- `ycsb_workload_a`
- `tpc_c_10w`
- `ann_sift1m`
- `ldbc_snb_sf1`

### 1.2 Required Fields

Every workload must specify:
- `workload_id`: Unique identifier
- `workload_family`: Standard benchmark family
- `standard_reference`: Citation to standard or paper
- `description`: Human-readable description
- `parameters`: Workload-specific configuration

---

## 2. YCSB Workloads

### 2.1 Overview

**Yahoo! Cloud Serving Benchmark (YCSB)**

**Citation:** Cooper, B. F., Silberstein, A., Tam, E., Ramakrishnan, R., & Sears, R. (2010). Benchmarking cloud serving systems with YCSB. In Proceedings of the 1st ACM symposium on Cloud computing (pp. 143-154). DOI: 10.1145/1807128.1807152

**Purpose:** Evaluate performance of key-value and NoSQL databases under various workload patterns.

### 2.2 Workload A: Update Heavy

**Characteristics:**
- 50% Read operations
- 50% Update operations
- Simulates a session store with both reads and writes

**Configuration:**
```yaml
workloads:
  - workload_id: "ycsb_workload_a"
    workload_family: "YCSB"
    standard_reference: "Cooper et al., SoCC 2010, DOI: 10.1145/1807128.1807152"
    description: "Update Heavy Workload (50% reads, 50% updates)"
    
    parameters:
      # Dataset size
      record_count: 1000000           # Number of records to load
      operation_count: 1000000        # Number of operations to execute
      
      # Record structure
      field_count: 10                 # Fields per record
      field_length: 100               # Bytes per field
      
      # Operation mix
      operations:
        read: 50
        update: 50
        insert: 0
        scan: 0
        
      # Access pattern
      distribution: "zipfian"         # Options: uniform, zipfian, latest, hotspot
      zipfian_constant: 0.99          # Skew parameter (0.99 = realistic)
      
      # Concurrency
      thread_count: 16                # Concurrent threads
      target_throughput: 0            # 0 = unlimited, >0 = rate limit
```

**Use Cases:**
- Session stores
- User profile databases
- Shopping cart systems

**Expected Behavior:**
- High contention on popular items
- Mixed read/write workload
- Cache-friendly for hot items

### 2.3 Workload B: Read Heavy

**Characteristics:**
- 95% Read operations
- 5% Update operations
- Simulates photo tagging or similar read-heavy applications

**Configuration:**
```yaml
operations:
  read: 95
  update: 5
  insert: 0
  scan: 0
```

**Use Cases:**
- Photo tagging applications
- News aggregators
- Product catalog browsing

### 2.4 Workload C: Read Only

**Characteristics:**
- 100% Read operations
- No writes
- Simulates user profile cache

**Configuration:**
```yaml
operations:
  read: 100
  update: 0
  insert: 0
  scan: 0
```

**Use Cases:**
- Caching layers
- Read replicas
- Archive systems

### 2.5 Workload D: Read Latest

**Characteristics:**
- 95% Read operations (latest records)
- 5% Insert operations
- Simulates user status updates

**Configuration:**
```yaml
operations:
  read: 95
  update: 0
  insert: 5
  scan: 0
distribution: "latest"              # Read recently inserted items
```

**Use Cases:**
- Social media feeds
- Activity streams
- News feeds

### 2.6 Workload E: Scan Heavy

**Characteristics:**
- 95% Scan operations (range queries)
- 5% Insert operations
- Simulates threaded conversations

**Configuration:**
```yaml
operations:
  read: 0
  update: 0
  insert: 5
  scan: 95
  
scan_parameters:
  max_scan_length: 100              # Maximum records per scan
  mean_scan_length: 50              # Average records per scan
```

**Use Cases:**
- Threaded conversations
- Time-series data retrieval
- Log analysis

### 2.7 Workload F: Read-Modify-Write

**Characteristics:**
- 50% Read operations
- 50% Read-Modify-Write operations
- Simulates user database with profile updates

**Configuration:**
```yaml
operations:
  read: 50
  read_modify_write: 50
  insert: 0
  scan: 0
```

**Use Cases:**
- User profile updates
- Counter increments
- Transactional updates

### 2.8 Distribution Types

**Uniform Distribution:**
```yaml
distribution: "uniform"
```
- Equal probability for all records
- No hot spots
- Use for: Cache effectiveness testing

**Zipfian Distribution:**
```yaml
distribution: "zipfian"
zipfian_constant: 0.99              # 0.99 = realistic, 1.0 = extreme skew
```
- 80/20 rule (or more extreme)
- Realistic access patterns
- Use for: Real-world simulation

**Latest Distribution:**
```yaml
distribution: "latest"
```
- Recently inserted items more likely
- Temporal locality
- Use for: Time-series, social feeds

**Hotspot Distribution:**
```yaml
distribution: "hotspot"
hotspot_data_fraction: 0.2          # 20% of data
hotspot_access_fraction: 0.8        # 80% of accesses
```
- Configurable hot region
- Controllable skew
- Use for: Specific contention testing

---

## 3. TPC Benchmarks

### 3.1 TPC-C: OLTP Benchmark

**Transaction Processing Performance Council Benchmark C**

**Citation:** TPC Benchmark C Standard Specification, Revision 5.11.0, February 2010. http://www.tpc.org/tpcc/

**Purpose:** Measure performance of Online Transaction Processing (OLTP) systems simulating an order-entry environment.

**Configuration:**
```yaml
workloads:
  - workload_id: "tpc_c"
    workload_family: "TPC-C"
    standard_reference: "TPC-C Revision 5.11, http://www.tpc.org/tpcc/"
    description: "OLTP Benchmark - Order Entry Environment"
    
    parameters:
      # Database scale
      warehouses: 10                  # 1 warehouse ≈ 100MB
      
      # Execution parameters
      duration_seconds: 300           # Measurement period
      ramp_up_seconds: 60             # Warm-up before measurement
      cool_down_seconds: 30           # Cool down after measurement
      
      # Concurrency
      thread_count: 16                # Number of terminals
      
      # Think time
      think_time_ms: 0                # Delay between transactions (0 = max throughput)
      
      # Transaction mix (must sum to 100)
      transaction_mix:
        new_order: 45                 # 45% New-Order transactions
        payment: 43                   # 43% Payment transactions
        order_status: 4               # 4% Order-Status queries
        delivery: 4                   # 4% Delivery transactions
        stock_level: 4                # 4% Stock-Level queries
        
      # Data generation
      data_generation:
        realistic_names: true         # Use realistic names (slower)
        variable_length_strings: true # Variable string lengths
        random_seed: 42               # For reproducibility
        include_original_string: true # TPC-C requirement
```

**Transaction Types:**

1. **New-Order (45%):**
   - Enter a new order with 5-15 order lines
   - 1% of orders have non-existent items (rollback test)
   - Most complex transaction

2. **Payment (43%):**
   - Update customer balance and warehouse/district YTD
   - By customer ID (60%) or last name (40%)
   - Tests update performance

3. **Order-Status (4%):**
   - Query order status by customer
   - Read-only transaction
   - Tests query performance

4. **Delivery (4%):**
   - Process 10 pending orders
   - Batch transaction
   - Tests batch processing

5. **Stock-Level (4%):**
   - Determine stock level for items below threshold
   - Read-only transaction
   - Tests complex queries

**Metrics:**
- **tpmC:** Transactions per minute C (New-Order transactions)
- **Price/tpmC:** Cost per performance unit
- Response time requirements: Mean < 5s, 90th percentile < 80s per transaction type

**Database Size:**
- 1 warehouse ≈ 100 MB
- 10 warehouses ≈ 1 GB
- 100 warehouses ≈ 10 GB
- 1000 warehouses ≈ 100 GB

### 3.2 TPC-H: Decision Support Benchmark

**Transaction Processing Performance Council Benchmark H**

**Citation:** TPC Benchmark H Standard Specification, Revision 3.0.0, February 2021. http://www.tpc.org/tpch/

**Purpose:** Measure performance of decision support systems with complex ad-hoc queries.

**Configuration:**
```yaml
workloads:
  - workload_id: "tpc_h_sf1"
    workload_family: "TPC-H"
    standard_reference: "TPC-H Revision 3.0.0, http://www.tpc.org/tpch/"
    description: "Decision Support Benchmark - 1GB Dataset"
    
    parameters:
      # Database scale
      scale_factor: 1                 # 1 = 1GB, 10 = 10GB, etc.
      
      # Execution parameters
      query_streams: 1                # Parallel query streams
      refresh_streams: 1              # Concurrent refresh operations
      
      # Reproducibility
      random_seed: 42
      
      # Query selection (all 22 or subset)
      queries:
        - 1   # Pricing Summary Report Query
        - 2   # Minimum Cost Supplier Query
        - 3   # Shipping Priority Query
        - 4   # Order Priority Checking Query
        - 5   # Local Supplier Volume Query
        - 6   # Forecasting Revenue Change Query
        - 7   # Volume Shipping Query
        - 8   # National Market Share Query
        - 9   # Product Type Profit Measure Query
        - 10  # Returned Item Reporting Query
        - 11  # Important Stock Identification Query
        - 12  # Shipping Modes and Order Priority Query
        - 13  # Customer Distribution Query
        - 14  # Promotion Effect Query
        - 15  # Top Supplier Query
        - 16  # Parts/Supplier Relationship Query
        - 17  # Small-Quantity-Order Revenue Query
        - 18  # Large Volume Customer Query
        - 19  # Discounted Revenue Query
        - 20  # Potential Part Promotion Query
        - 21  # Suppliers Who Kept Orders Waiting Query
        - 22  # Global Sales Opportunity Query
```

**Scale Factors:**
- SF1 = 1 GB (6M rows in LINEITEM)
- SF10 = 10 GB (60M rows)
- SF100 = 100 GB (600M rows)
- SF1000 = 1 TB (6B rows)
- SF10000 = 10 TB (60B rows)

**Query Characteristics:**
- Complex multi-table joins
- Aggregations and grouping
- Sorting operations
- Subqueries and derived tables
- Wide range of selectivity

**Metrics:**
- **QphH:** Queries per hour H
- **Price/QphH:** Cost per performance unit
- Power test: Single-stream query execution
- Throughput test: Multiple concurrent streams

---

## 4. Vector Search Benchmarks

### 4.1 ANN-Benchmarks

**Approximate Nearest Neighbor Benchmarks**

**Citation:** Aumüller, M., Bernhardsson, E., & Faithfull, A. (2020). ANN-Benchmarks: A benchmarking tool for approximate nearest neighbor algorithms. Information Systems, 87, 101374. DOI: 10.1016/j.is.2019.02.006

**Purpose:** Evaluate vector similarity search performance and accuracy trade-offs.

**Configuration:**
```yaml
workloads:
  - workload_id: "ann_sift1m"
    workload_family: "ANN-Benchmarks"
    standard_reference: "Aumüller et al., Information Systems 2020"
    description: "Vector Similarity Search - SIFT1M Dataset"
    
    parameters:
      # Dataset
      dataset: "SIFT1M"               # Standard dataset name
      dataset_size: 1000000           # Number of vectors
      dimensions: 128                 # Vector dimensionality
      
      # Distance metric
      distance_metric: "euclidean"    # euclidean, cosine, dot_product
      
      # Query parameters
      query_set_size: 10000           # Number of queries
      k_nearest: 10                   # Number of neighbors to retrieve
      
      # Accuracy target
      recall_target: 0.95             # Target recall@k
      
      # Index parameters (algorithm-specific)
      index_parameters:
        index_type: "hnsw"            # HNSW, IVF, Flat, PQ, etc.
        
        # HNSW parameters
        ef_construction: 200          # Build-time parameter
        m: 16                         # Max connections per node
        ef_search: 100                # Query-time parameter
```

**Standard Datasets:**

1. **SIFT1M:**
   - 1M 128-dimensional SIFT descriptors
   - Image feature vectors
   - Euclidean distance

2. **GIST:**
   - 1M 960-dimensional GIST descriptors
   - Image feature vectors
   - Euclidean distance

3. **GloVe:**
   - Word embeddings
   - Various dimensions (25, 50, 100, 200)
   - Cosine or dot product similarity

4. **Deep1B:**
   - 1B deep learning features
   - 96 dimensions
   - Euclidean distance

**Index Types:**

1. **HNSW (Hierarchical Navigable Small World):**
```yaml
index_parameters:
  index_type: "hnsw"
  ef_construction: 200              # Higher = better quality, slower build
  m: 16                             # Higher = better quality, more memory
  ef_search: 100                    # Higher = better recall, slower query
```

2. **IVF (Inverted File Index):**
```yaml
index_parameters:
  index_type: "ivf"
  nlist: 100                        # Number of clusters
  nprobe: 10                        # Clusters to search
```

3. **Flat (Exact Search):**
```yaml
index_parameters:
  index_type: "flat"                # Baseline for accuracy
```

4. **PQ (Product Quantization):**
```yaml
index_parameters:
  index_type: "pq"
  m: 8                              # Number of subquantizers
  nbits: 8                          # Bits per subquantizer
```

**Metrics:**
- **QPS (Queries Per Second):** Throughput
- **Recall@k:** Accuracy (fraction of true k-NN found)
- **Build Time:** Index construction time
- **Index Size:** Memory/disk usage
- **Recall-QPS Curve:** Trade-off visualization

---

## 5. Graph Benchmarks

### 5.1 LDBC Social Network Benchmark

**Linked Data Benchmark Council Social Network Benchmark**

**Citation:** Erling, O., Averbuch, A., Larriba-Pey, J., Chafi, H., Gubichev, A., Prat, A., ... & Boncz, P. (2015). The LDBC social network benchmark: Interactive workload. In Proceedings of the 2015 ACM SIGMOD International Conference on Management of Data (pp. 619-630). arXiv:2001.02299

**Purpose:** Evaluate graph database performance on social network-like workloads.

**Configuration:**
```yaml
workloads:
  - workload_id: "ldbc_snb_interactive"
    workload_family: "LDBC-SNB"
    standard_reference: "Erling et al., SIGMOD 2015, arXiv:2001.02299"
    description: "Graph Database Social Network Benchmark"
    
    parameters:
      # Database scale
      scale_factor: "SF1"             # SF0.1, SF1, SF3, SF10, SF30, SF100
      
      # Workload type
      workload_type: "interactive"    # interactive, bi, graphanalytics
      
      # Query mix
      query_mix:
        short_reads: 50               # Simple lookups
        complex_reads: 40             # Complex graph queries
        updates: 10                   # Insert operations
        
      # Concurrency
      thread_count: 16
      
      # Execution
      duration_seconds: 300
      ramp_up_seconds: 60
```

**Scale Factors:**
- SF0.1: ~1K persons, ~10K posts
- SF1: ~11K persons, ~90K posts
- SF3: ~30K persons, ~270K posts
- SF10: ~90K persons, ~900K posts
- SF30: ~280K persons, ~2.7M posts
- SF100: ~930K persons, ~9M posts

**Workload Types:**

1. **Interactive (LDBC-SNB-I):**
   - Short read queries (lookups)
   - Complex read queries (multi-hop traversals)
   - Insert operations
   - Simulates social network usage

2. **Business Intelligence (LDBC-SNB-BI):**
   - Complex analytical queries
   - Aggregations over large datasets
   - Multi-dimensional analysis

3. **Graph Analytics (LDBC-SNB-GA):**
   - PageRank
   - Community Detection
   - Shortest Paths
   - Graph algorithms

**Query Categories:**

**Short Reads (7 queries):**
- Person profile
- Person's recent messages
- Person's friends
- Message details

**Complex Reads (14 queries):**
- Friend recommendations
- Pattern matching
- Path finding
- Aggregations

**Updates (8 operations):**
- Add person
- Add post/comment
- Add friendship
- Add like

---

## 6. LLM Benchmarks

### 6.1 vLLM Serving Benchmark

**Large Language Model Inference Serving**

**Citation:** Kwon, W., Li, Z., Zhuang, S., Sheng, Y., Zheng, L., Yu, C. H., ... & Stoica, I. (2023). Efficient Memory Management for Large Language Model Serving with PagedAttention. In Proceedings of the 29th Symposium on Operating Systems Principles (pp. 611-626).

**Purpose:** Evaluate LLM inference throughput and latency under various load conditions.

**Configuration:**
```yaml
workloads:
  - workload_id: "vllm_serving"
    workload_family: "vLLM"
    standard_reference: "Kwon et al., SOSP 2023"
    description: "LLM Inference Serving Benchmark"
    
    parameters:
      # Model
      model_name: "llama-2-7b"        # Model identifier
      
      # Load generation
      request_rate: 1.0               # Requests per second
      num_prompts: 1000               # Total requests
      
      # Input length distribution
      input_length:
        distribution: "normal"        # normal, uniform, poisson
        mean: 512
        std: 128
        min: 128
        max: 2048
        
      # Output length distribution
      output_length:
        distribution: "normal"
        mean: 256
        std: 64
        min: 32
        max: 512
        
      # Serving parameters
      batch_size: "dynamic"           # dynamic or fixed number
      max_batch_size: 32
      
      # Generation parameters
      temperature: 1.0
      top_p: 1.0
      top_k: -1
```

**Models:**
- llama-2-7b, llama-2-13b, llama-2-70b
- mistral-7b
- falcon-7b, falcon-40b
- Any compatible model

**Metrics:**
- **Throughput:** Requests/second or tokens/second
- **Latency:** Time to first token (TTFT), time per output token (TPOT)
- **P50/P90/P99 latency:** Percentile latencies
- **GPU Utilization:** GPU memory and compute usage
- **Batch Size Distribution:** Dynamic batching efficiency

### 6.2 RAGBench

**Retrieval-Augmented Generation Benchmark**

**Citation:** Chen, J., Lin, H., Han, X., & Sun, L. (2024). Benchmarking Large Language Models in Retrieval-Augmented Generation. In Proceedings of the AAAI Conference on Artificial Intelligence.

**Purpose:** Evaluate end-to-end RAG system performance including retrieval accuracy and generation quality.

**Configuration:**
```yaml
workloads:
  - workload_id: "rag_qa"
    workload_family: "RAGBench"
    standard_reference: "Chen et al., AAAI 2024"
    description: "RAG System End-to-End Performance"
    
    parameters:
      # Dataset
      dataset: "NaturalQuestions"     # NQ, HotpotQA, MSMARCO, etc.
      num_questions: 1000
      
      # Retrieval configuration
      retrieval:
        top_k: 5                      # Documents to retrieve
        retrieval_method: "dense"     # dense, sparse, hybrid
        
        # Dense retrieval
        embedding_model: "sentence-transformers"
        index_type: "hnsw"
        
        # Sparse retrieval
        bm25_k1: 1.2
        bm25_b: 0.75
        
      # Generation configuration
      generation:
        model: "llama-2-7b"
        max_tokens: 256
        temperature: 0.7
        
        # Context window
        max_context_length: 2048
        
      # Metrics to evaluate
      metrics:
        - "answer_accuracy"           # F1, EM
        - "retrieval_precision"       # Precision@k
        - "retrieval_recall"          # Recall@k
        - "end_to_end_latency"        # Total time
        - "relevance_score"           # NDCG
```

**Datasets:**
- **NaturalQuestions:** Google search questions
- **HotpotQA:** Multi-hop reasoning
- **MSMARCO:** Microsoft Machine Reading Comprehension
- **SQuAD:** Reading comprehension

**Metrics:**
- **Answer Quality:**
  - Exact Match (EM)
  - F1 Score
  - BLEU/ROUGE scores
  
- **Retrieval Quality:**
  - Precision@k
  - Recall@k
  - NDCG (Normalized Discounted Cumulative Gain)
  - MRR (Mean Reciprocal Rank)
  
- **Performance:**
  - End-to-end latency
  - Retrieval latency
  - Generation latency
  - Throughput

---

## 7. Custom Workload Definition

### 7.1 Template

```yaml
workloads:
  - workload_id: "custom_workload_name"
    workload_family: "Custom"
    standard_reference: "Internal specification or paper citation"
    description: "Detailed description of workload purpose and characteristics"
    
    parameters:
      # Define all configurable parameters
      parameter1: value1
      parameter2: value2
      
      # Nested parameters
      nested_config:
        sub_param1: value1
        sub_param2: value2
```

### 7.2 Best Practices

1. **Use descriptive names:**
   - Good: `ecommerce_checkout_simulation`
   - Bad: `test1`

2. **Document all parameters:**
   - Include comments
   - Specify units
   - Provide valid ranges

3. **Cite sources:**
   - Reference papers or specifications
   - Include DOI when available
   - Credit original authors

4. **Ensure reproducibility:**
   - Use random seeds
   - Document all dependencies
   - Include data generation details

5. **Follow vendor neutrality:**
   - No brand names
   - Generic terminology
   - Objective descriptions

---

## Appendix: Quick Reference

### Workload Family Table

| Family | Standard | Use Case | Complexity |
|--------|----------|----------|------------|
| YCSB | Cooper 2010 | Key-value operations | Low |
| TPC-C | TPC-C v5.11 | OLTP transactions | High |
| TPC-H | TPC-H v3.0 | Analytical queries | High |
| ANN-Benchmarks | Aumüller 2020 | Vector search | Medium |
| LDBC-SNB | Erling 2015 | Graph operations | High |
| vLLM | Kwon 2023 | LLM inference | Medium |
| RAGBench | Chen 2024 | RAG systems | High |

---

**Document Version:** 1.0.0  
**Last Updated:** April 2026  
**License:** MIT  
**Maintainer:** CHIMERA Development Team
