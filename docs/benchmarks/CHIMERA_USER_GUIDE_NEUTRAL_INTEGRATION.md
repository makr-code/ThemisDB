# CHIMERA Suite: Neutral System Integration User Guide

**Version:** 1.0.0  
**Last Updated:** April 2026  
**Standards Compliance:** IEEE Std 2807-2022, ISO/IEC 14756:2015

---

## Table of Contents

1. [Introduction](#introduction)
2. [Core Principles](#core-principles)
3. [Configuration Format Overview](#configuration-format-overview)
4. [System Configuration](#system-configuration)
5. [Hardware Specification](#hardware-specification)
6. [Workload Definitions](#workload-definitions)
7. [Statistical Methodology](#statistical-methodology)
8. [Execution Parameters](#execution-parameters)
9. [Metrics Definition](#metrics-definition)
10. [Vendor Neutrality Guidelines](#vendor-neutrality-guidelines)
11. [Complete Examples](#complete-examples)
12. [Best Practices](#best-practices)
13. [Troubleshooting](#troubleshooting)
14. [References](#references)

---

## 1. Introduction

The CHIMERA Suite (Comprehensive, Honest, Impartial Metrics for Empirical Reporting and Analysis) provides a vendor-neutral configuration format for database benchmarking. This guide helps you create fair, reproducible, and scientifically rigorous benchmark configurations.

### 1.1 Purpose

This user guide explains how to:
- Configure vendor-neutral benchmark tests
- Define systems under test without branding bias
- Specify hardware configurations objectively
- Map workloads to standard benchmarks
- Ensure statistical rigor and reproducibility

### 1.2 Target Audience

- Database researchers
- Performance engineers
- System architects
- Benchmark operators
- Academic institutions
- Independent reviewers

### 1.3 Prerequisites

- Basic understanding of database systems
- Familiarity with YAML or TOML syntax
- Knowledge of benchmark methodologies (YCSB, TPC-C, etc.)
- Understanding of statistical concepts

---

## 2. Core Principles

### 2.1 Vendor Neutrality

**Principle:** All configuration elements must be free from vendor bias, branding, or favoritism.

**Guidelines:**
- Use generic system identifiers (e.g., "system_a", "system_multimodel_a")
- Avoid brand names in adapter naming
- Use descriptive, not promotional, language
- Sort results alphabetically or by metric value only
- Never declare a "winner" or "best" system

**Example - Correct:**
```yaml
systems:
  - system_id: "system_multimodel_a"
    adapter:
      type: "NativeAdapter"
```

**Example - Incorrect:**
```yaml
systems:
  - system_id: "themisdb_production"  # Contains vendor name
    adapter:
      type: "themisdb_native_adapter"  # Branded naming
```

### 2.2 Scientific Rigor

**Principle:** All methodologies must be based on established scientific standards with proper citations.

**Requirements:**
- Reference IEEE/ACM/ISO standards
- Cite original research papers
- Document all statistical methods
- Ensure reproducibility
- Report confidence intervals and effect sizes

### 2.3 Accessibility

**Principle:** Reports must be accessible to all audiences, including those with color blindness.

**Requirements:**
- Use color-blind friendly palettes (Okabe-Ito, Paul Tol)
- Provide alternative visual indicators
- Ensure high contrast
- Use clear typography

### 2.4 Reproducibility

**Principle:** All benchmarks must be fully reproducible by independent parties.

**Requirements:**
- Document complete hardware specifications
- Record all software versions
- Use fixed random seeds
- Include configuration files
- Archive raw data

---

## 3. Configuration Format Overview

### 3.1 File Formats

CHIMERA supports two configuration formats:

**YAML (Recommended):**
- Human-readable
- Wide tool support
- Hierarchical structure
- Comments supported

**TOML:**
- Type-safe
- Less ambiguous
- Growing adoption
- Good for complex configs

### 3.2 Top-Level Structure

All CHIMERA configuration files contain these sections:

```yaml
metadata:           # Benchmark identification
systems:            # Systems under test
hardware:           # Hardware specifications
workloads:          # Benchmark workloads
methodology:        # Statistical methods
execution:          # Runtime parameters
metrics:            # Performance metrics
references:         # Citations
```

### 3.3 Required vs Optional Sections

**Required:**
- `metadata`: Benchmark identification
- `systems`: At least one system under test
- `hardware`: Complete hardware specification
- `workloads`: At least one workload
- `methodology`: Statistical parameters

**Optional:**
- `references`: Citation database (recommended)
- `execution`: Runtime config (has defaults)
- `metrics`: Metric definitions (has defaults)

---

## 4. System Configuration

### 4.1 System Identification

Each system must have a unique, vendor-neutral identifier.

**Naming Convention:**
```
system_<type>_<letter>
```

Where:
- `<type>`: Generic type (e.g., multimodel, relational, document, graph)
- `<letter>`: Sequential letter (a, b, c, ...)

**Examples:**
- `system_multimodel_a`
- `system_relational_b`
- `system_document_c`
- `system_graph_d`

### 4.2 System Classification

Define system characteristics objectively:

```yaml
systems:
  - system_id: "system_multimodel_a"
    system_type: "multi_model_database"
    model: "transactional_analytical_hybrid"
    
    architecture:
      type: "native_multi_model"
      storage_engine: "lsm_tree"
      transaction_model: "mvcc"
      consistency: "acid"
```

**Valid System Types:**
- `multi_model_database`
- `relational_database`
- `document_database`
- `graph_database`
- `vector_database`
- `key_value_store`
- `time_series_database`

**Valid Storage Engines:**
- `lsm_tree` (Log-Structured Merge Tree)
- `btree` (B-Tree)
- `hybrid` (Combination)
- `column_store`
- `row_store`

**Valid Transaction Models:**
- `mvcc` (Multi-Version Concurrency Control)
- `2pl` (Two-Phase Locking)
- `occ` (Optimistic Concurrency Control)
- `timestamp_ordering`

**Valid Consistency Models:**
- `acid` (ACID transactions)
- `eventual` (Eventual consistency)
- `causal` (Causal consistency)
- `strong` (Strong consistency)

### 4.3 Adapter Configuration

Adapters connect benchmarks to systems. Use generic names:

**Standard Adapter Types:**

```yaml
adapter:
  type: "NativeAdapter"           # System's native protocol
  protocol: "binary_wire"
  connection_pool_size: 20
  timeout_seconds: 30
```

```yaml
adapter:
  type: "PostgreSQLAdapter"       # PostgreSQL wire protocol
  protocol: "postgres_wire"
  connection_pool_size: 20
  timeout_seconds: 30
```

```yaml
adapter:
  type: "MongoDBAdapter"          # MongoDB wire protocol
  protocol: "mongodb_wire"
  connection_pool_size: 20
  timeout_seconds: 30
```

```yaml
adapter:
  type: "HTTPAdapter"             # HTTP/REST API
  protocol: "http_rest"
  connection_pool_size: 20
  timeout_seconds: 30
```

```yaml
adapter:
  type: "gRPCAdapter"             # gRPC protocol
  protocol: "grpc"
  connection_pool_size: 20
  timeout_seconds: 30
```

**Adapter Naming Rules:**
1. Use protocol/standard name (e.g., "PostgreSQLAdapter" not "pg_adapter")
2. Capitalize adapter type names
3. No vendor-specific prefixes
4. Generic for native protocols

### 4.4 Data Model Support

List supported data models:

```yaml
data_models:
  - "relational"
  - "document"
  - "graph"
  - "vector"
  - "key_value"
  - "time_series"
```

---

## 5. Hardware Specification

### 5.1 Complete Hardware Profile

Hardware specifications must be complete and precise for reproducibility.

**IEEE Std 2807-2022 Requirements:**
- Processor specifications
- Memory configuration
- Storage details
- Network configuration
- System settings

### 5.2 Processor Specification

```yaml
hardware:
  processor:
    architecture: "x86_64"        # or "ARM64", "RISC-V"
    vendor: "Generic"              # Anonymized
    model: "8-Core High-Performance CPU"
    
    cores:
      physical: 8
      logical: 16
      
    frequency:
      base_ghz: 3.6
      turbo_ghz: 5.0
      
    cache:
      l1_kb: 64
      l2_kb: 512
      l3_mb: 16
      
    features:
      - "AVX2"
      - "AVX512"
      - "AES-NI"
      
    tdp_watts: 125
```

**Important Notes:**
- Anonymize vendor names
- Include all CPU features relevant to databases
- Specify both base and turbo frequencies
- Document all cache levels

### 5.3 Memory Specification

```yaml
memory:
  total_gb: 64
  type: "DDR4"                    # DDR4, DDR5, etc.
  speed_mhz: 3200
  channels: 4                     # Dual-channel, Quad-channel
  ecc_enabled: true               # Error-correcting code
  numa_nodes: 1                   # NUMA configuration
```

**Critical Parameters:**
- Total capacity
- Memory type and speed
- Number of channels (affects bandwidth)
- ECC status (affects reliability)
- NUMA topology (affects performance)

### 5.4 Storage Specification

```yaml
storage:
  primary:
    type: "NVMe SSD"
    capacity_gb: 1000
    interface: "PCIe 4.0 x4"
    form_factor: "M.2"
    
    performance:
      sequential_read_mbps: 7000
      sequential_write_mbps: 5000
      random_read_iops: 1000000
      random_write_iops: 700000
      queue_depth: 32
      
    filesystem: "ext4"
    mount_options: "noatime,nodiratime"
```

**Performance Metrics:**
- Sequential read/write speeds
- Random IOPS (4K blocks)
- Queue depth
- Filesystem and mount options

### 5.5 Network Configuration

```yaml
network:
  interface: "10GbE"
  bandwidth_gbps: 10
  mtu_bytes: 9000                 # Jumbo frames
  latency_us: 10
  topology: "single_node"         # or "cluster", "distributed"
```

### 5.6 System Configuration

```yaml
system:
  operating_system: "Linux"
  distribution: "Generic Linux Distribution"
  kernel_version: "6.5.0"
  page_size_kb: 4
  huge_pages_enabled: true
  cpu_governor: "performance"     # CPU frequency scaling
  power_profile: "maximum_performance"
```

---

## 6. Workload Definitions

### 6.1 Standard Workload Families

CHIMERA supports these standard benchmark families:

1. **YCSB** - Yahoo! Cloud Serving Benchmark
2. **TPC-C** - Transaction Processing Performance Council C
3. **TPC-H** - Transaction Processing Performance Council H
4. **ANN-Benchmarks** - Approximate Nearest Neighbor
5. **LDBC-SNB** - Linked Data Benchmark Council Social Network
6. **vLLM** - LLM Inference Serving
7. **RAGBench** - Retrieval-Augmented Generation

### 6.2 YCSB Workloads

**Reference:** Cooper et al., "Benchmarking Cloud Serving Systems with YCSB", SoCC 2010, DOI: 10.1145/1807128.1807152

**Workload A - Update Heavy (50% read, 50% update):**
```yaml
workloads:
  - workload_id: "ycsb_workload_a"
    workload_family: "YCSB"
    standard_reference: "Cooper2010"
    description: "Update Heavy Workload"
    
    parameters:
      record_count: 1000000
      operation_count: 1000000
      field_count: 10
      field_length: 100
      
      operations:
        read: 50
        update: 50
        insert: 0
        scan: 0
        
      distribution: "zipfian"
      zipfian_constant: 0.99
      
      thread_count: 16
      target_throughput: 0  # unlimited
```

**Workload B - Read Heavy (95% read, 5% update):**
```yaml
operations:
  read: 95
  update: 5
  insert: 0
  scan: 0
```

**Workload C - Read Only (100% read):**
```yaml
operations:
  read: 100
  update: 0
  insert: 0
  scan: 0
```

**Workload D - Read Latest (95% read latest, 5% insert):**
```yaml
operations:
  read: 95
  update: 0
  insert: 5
  scan: 0
distribution: "latest"
```

**Workload E - Scan Heavy (95% scan, 5% insert):**
```yaml
operations:
  read: 0
  update: 0
  insert: 5
  scan: 95
```

**Workload F - Read-Modify-Write (50% read, 50% read-modify-write):**
```yaml
operations:
  read: 50
  read_modify_write: 50
  insert: 0
  scan: 0
```

**Distribution Types:**
- `uniform`: Equal probability for all records
- `zipfian`: 80/20 rule (configurable with zipfian_constant)
- `latest`: Recently inserted items more likely
- `hotspot`: Configurable hot region

### 6.3 TPC-C Benchmark

**Reference:** TPC Benchmark C Standard Specification, Revision 5.11, http://www.tpc.org/tpcc/

```yaml
workloads:
  - workload_id: "tpc_c"
    workload_family: "TPC-C"
    standard_reference: "TPC-C-v5.11"
    description: "OLTP Benchmark - Order Entry Environment"
    
    parameters:
      warehouses: 10
      duration_seconds: 300
      ramp_up_seconds: 60
      thread_count: 16
      think_time_ms: 0
      
      transaction_mix:
        new_order: 45       # 45% New-Order transactions
        payment: 43         # 43% Payment transactions
        order_status: 4     # 4% Order-Status queries
        delivery: 4         # 4% Delivery transactions
        stock_level: 4      # 4% Stock-Level queries
        
      data_generation:
        realistic_names: true
        variable_length_strings: true
        random_seed: 42
        include_original_string: true
```

**Key Parameters:**
- `warehouses`: Determines database size (1 warehouse ≈ 100MB)
- `duration_seconds`: Measurement period
- `ramp_up_seconds`: Warm-up period before measurement
- `thread_count`: Number of concurrent users
- `think_time_ms`: Delay between transactions (0 = max throughput)

**Transaction Mix:**
Must follow TPC-C specification (percentages must sum to 100).

### 6.4 TPC-H Benchmark

**Reference:** TPC Benchmark H Standard Specification, Revision 3.0.0, http://www.tpc.org/tpch/

```yaml
workloads:
  - workload_id: "tpc_h_sf1"
    workload_family: "TPC-H"
    standard_reference: "TPC-H-v3.0.0"
    description: "Decision Support Benchmark"
    
    parameters:
      scale_factor: 1           # 1 = 1GB database
      query_streams: 1
      refresh_streams: 1
      random_seed: 42
      
      queries:
        - 1   # Pricing Summary Report Query
        - 2   # Minimum Cost Supplier Query
        - 3   # Shipping Priority Query
        - 6   # Forecasting Revenue Change Query
        - 14  # Promotion Effect Query
```

**Scale Factors:**
- SF1 = 1 GB
- SF10 = 10 GB
- SF100 = 100 GB
- SF1000 = 1 TB

**Query Selection:**
Include all 22 queries or specify subset for quick evaluation.

### 6.5 ANN-Benchmarks

**Reference:** Aumüller et al., "ANN-Benchmarks: A Benchmarking Tool for Approximate Nearest Neighbor Algorithms", Information Systems 2020, DOI: 10.1016/j.is.2019.02.006

```yaml
workloads:
  - workload_id: "ann_sift1m"
    workload_family: "ANN-Benchmarks"
    standard_reference: "Aumueller2020"
    description: "Vector Similarity Search"
    
    parameters:
      dataset: "SIFT1M"         # or GIST, GloVe, Deep1B, etc.
      dataset_size: 1000000
      dimensions: 128
      distance_metric: "euclidean"
      
      query_set_size: 10000
      k_nearest: 10
      recall_target: 0.95
      
      index_parameters:
        index_type: "hnsw"      # or ivf, flat, pq
        ef_construction: 200
        m: 16
        ef_search: 100
```

**Standard Datasets:**
- SIFT1M: 1M 128D SIFT descriptors
- GIST: 1M 960D GIST descriptors
- GloVe: Word vectors
- Deep1B: 1B deep learning features

**Distance Metrics:**
- `euclidean`: L2 distance
- `cosine`: Cosine similarity
- `dot_product`: Inner product
- `manhattan`: L1 distance

### 6.6 LDBC-SNB

**Reference:** Erling et al., "The LDBC Social Network Benchmark", arXiv:2001.02299

```yaml
workloads:
  - workload_id: "ldbc_snb_interactive"
    workload_family: "LDBC-SNB"
    standard_reference: "Erling2020"
    description: "Graph Database Social Network Benchmark"
    
    parameters:
      scale_factor: "SF1"
      workload_type: "interactive"
      
      query_mix:
        short_reads: 50
        complex_reads: 40
        updates: 10
        
      thread_count: 16
```

### 6.7 vLLM Benchmark

**Reference:** Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention", SOSP 2023

```yaml
workloads:
  - workload_id: "vllm_serving"
    workload_family: "vLLM"
    standard_reference: "Kwon2023"
    description: "LLM Inference Serving Benchmark"
    
    parameters:
      model_name: "llama-2-7b"
      request_rate: 1.0           # requests per second
      num_prompts: 1000
      
      input_length:
        mean: 512
        std: 128
        
      output_length:
        mean: 256
        std: 64
        
      batch_size: "dynamic"
```

### 6.8 RAGBench

**Reference:** Chen et al., "Benchmarking Large Language Models in Retrieval-Augmented Generation", AAAI 2024

```yaml
workloads:
  - workload_id: "rag_qa"
    workload_family: "RAGBench"
    standard_reference: "Chen2024"
    description: "RAG System End-to-End Performance"
    
    parameters:
      dataset: "NaturalQuestions"
      num_questions: 1000
      
      retrieval:
        top_k: 5
        retrieval_method: "dense"
        
      generation:
        model: "llama-2-7b"
        max_tokens: 256
        temperature: 0.7
        
      metrics:
        - "answer_accuracy"
        - "retrieval_precision"
        - "end_to_end_latency"
```

---

## 7. Statistical Methodology

### 7.1 Statistical Testing

```yaml
methodology:
  statistical_testing:
    alpha: 0.05                 # Significance level
    confidence_level: 0.95      # 95% confidence intervals
    min_sample_size: 30         # Minimum samples required
```

### 7.2 Hypothesis Tests

**Welch's t-test:**
```yaml
tests:
  - name: "Welch's t-test"
    reference: "Welch, B. L. (1947). Biometrika, 34(1-2), 28-35"
    use_case: "Comparing means with unequal variances"
```

Use for:
- Comparing mean performance between two systems
- When variances may differ
- Normally distributed data

**Mann-Whitney U test:**
```yaml
  - name: "Mann-Whitney U test"
    reference: "Mann & Whitney (1947). Ann. Math. Stat., 18(1), 50-60"
    use_case: "Non-parametric comparison"
```

Use for:
- Non-normally distributed data
- Ordinal data
- Robust to outliers

### 7.3 Effect Sizes

**Cohen's d:**
```yaml
effect_sizes:
  - name: "Cohen's d"
    reference: "Cohen, J. (1988). Statistical Power Analysis"
    thresholds:
      small: 0.2
      medium: 0.5
      large: 0.8
```

Interpretation:
- < 0.2: Negligible effect
- 0.2-0.5: Small effect
- 0.5-0.8: Medium effect
- > 0.8: Large effect

### 7.4 Outlier Detection

```yaml
outlier_detection:
  method: "IQR"
  reference: "Tukey, J. W. (1977). Exploratory Data Analysis"
  threshold: 1.5
  action: "remove_and_report"
```

**IQR Method:**
- Lower bound: Q1 - 1.5 × IQR
- Upper bound: Q3 + 1.5 × IQR
- Report number of outliers removed

### 7.5 Measurement Protocol

```yaml
measurement:
  warm_up_seconds: 60           # System warm-up
  measurement_duration_seconds: 300  # Actual measurement
  cool_down_seconds: 30         # Cool down
  num_runs: 5                   # Independent runs
  inter_run_delay_seconds: 60   # Delay between runs
  sampling_interval_ms: 100     # Metrics collection
```

**Best Practices:**
- Warm-up: Allow caches to stabilize
- Multiple runs: Ensure reproducibility
- Inter-run delay: Prevent thermal throttling
- Cool down: Clean shutdown

---

## 8. Execution Parameters

### 8.1 Execution Mode

```yaml
execution:
  mode: "sequential"  # or "parallel", "distributed"
```

**Sequential:** Run workloads one at a time (recommended for accuracy)  
**Parallel:** Run multiple workloads simultaneously  
**Distributed:** Run across multiple machines

### 8.2 Resource Isolation

```yaml
isolation:
  cpu_pinning: true           # Pin to specific CPUs
  numa_binding: true          # NUMA-aware binding
  cgroup_isolation: true      # cgroup resource limits
```

**Benefits:**
- Reduces variance
- Prevents interference
- Ensures reproducibility

### 8.3 System Monitoring

```yaml
monitoring:
  system_metrics:
    - "cpu_utilization"
    - "memory_usage"
    - "disk_io"
    - "network_io"
    - "cache_hit_ratio"
    
  collection_interval_seconds: 1
  
  tools:
    - "perf"
    - "iostat"
    - "vmstat"
```

### 8.4 Result Validation

```yaml
validation:
  verify_data_integrity: true
  verify_transaction_semantics: true
  verify_result_correctness: true
  max_error_rate_percent: 0.1
```

---

## 9. Metrics Definition

### 9.1 Throughput Metrics

```yaml
metrics:
  - metric_id: "throughput"
    name: "Operations Throughput"
    unit: "operations/second"
    aggregation: "mean"
    higher_is_better: true
```

### 9.2 Latency Metrics

```yaml
  - metric_id: "latency_p95"
    name: "95th Percentile Latency"
    unit: "milliseconds"
    aggregation: "p95"
    higher_is_better: false
```

### 9.3 Resource Metrics

```yaml
  - metric_id: "cpu_utilization"
    name: "CPU Utilization"
    unit: "percent"
    aggregation: "mean"
```

---

## 10. Vendor Neutrality Guidelines

### 10.1 Naming Conventions

**DO:**
- Use generic system identifiers
- Use protocol/standard names for adapters
- Use descriptive, objective language

**DON'T:**
- Use vendor/product names
- Use marketing terms
- Use branded naming schemes

### 10.2 Result Presentation

**DO:**
- Sort alphabetically or by metric value
- Use color-blind friendly colors
- Report all statistical details
- Include confidence intervals

**DON'T:**
- Declare winners
- Use vendor colors
- Hide unfavorable results
- Truncate axes misleadingly

### 10.3 Citation Requirements

All methodologies must be cited:

```yaml
references:
  - id: "Cooper2010"
    authors: ["Cooper, B. F.", "et al."]
    title: "Benchmarking cloud serving systems with YCSB"
    venue: "ACM SoCC"
    year: 2010
    doi: "10.1145/1807128.1807152"
```

---

## 11. Complete Examples

See the following example files:
- `benchmark_config_schema.yaml` - Complete schema with all options
- `benchmark_example.yaml` - Practical ready-to-use example
- `benchmark_config_example.toml` - TOML format example

---

## 12. Best Practices

### 12.1 Hardware Configuration

1. Use identical hardware for all systems
2. Disable power management features
3. Use performance CPU governor
4. Enable huge pages for databases
5. Disable NUMA balancing (use explicit binding)
6. Use dedicated benchmark machine

### 12.2 Workload Configuration

1. Run multiple independent trials (minimum 5)
2. Include warm-up period
3. Use realistic workload mixes
4. Document all random seeds
5. Verify data correctness

### 12.3 Statistical Analysis

1. Always report confidence intervals
2. Calculate effect sizes
3. Use appropriate statistical tests
4. Report outlier handling
5. Include raw data

### 12.4 Reporting

1. Use vendor-neutral language
2. Sort results consistently
3. Use accessible color schemes
4. Include all citations
5. Document limitations

---

## 13. Troubleshooting

### 13.1 Common Issues

**High Variance in Results:**
- Increase warm-up period
- Enable CPU pinning
- Disable background services
- Check for thermal throttling

**Outliers:**
- Check system logs for interruptions
- Verify no background tasks running
- Check for network issues
- Consider hardware problems

**Statistical Significance:**
- Increase sample size
- Reduce measurement noise
- Check for confounding factors

---

## 14. References

### 14.1 Standards

1. **IEEE Std 2807-2022**: IEEE Standard for Framework for Performance Benchmarking of Database Management Systems

2. **ISO/IEC 14756:2015**: Information technology - Measurement and rating of performance of computer-based software systems

3. **IEEE Std 730-2014**: IEEE Standard for Software Quality Assurance Processes

4. **IEEE Std 1012-2016**: IEEE Standard for System, Software, and Hardware Verification and Validation

### 14.2 Benchmark Standards

1. **TPC-C**: Transaction Processing Performance Council Benchmark C, Revision 5.11, http://www.tpc.org/tpcc/

2. **TPC-H**: Transaction Processing Performance Council Benchmark H, Revision 3.0.0, http://www.tpc.org/tpch/

3. **YCSB**: Cooper et al., "Benchmarking Cloud Serving Systems with YCSB", ACM SoCC 2010, DOI: 10.1145/1807128.1807152

4. **ANN-Benchmarks**: Aumüller et al., Information Systems 2020, DOI: 10.1016/j.is.2019.02.006

5. **LDBC-SNB**: Erling et al., "The LDBC Social Network Benchmark", arXiv:2001.02299

### 14.3 Statistical Methods

1. **Welch, B. L. (1947)**: "The generalization of 'Student's' problem when several different population variances are involved", Biometrika, 34(1-2), 28-35

2. **Mann, H. B., & Whitney, D. R. (1947)**: "On a test of whether one of two random variables is stochastically larger than the other", The Annals of Mathematical Statistics, 18(1), 50-60

3. **Cohen, J. (1988)**: Statistical Power Analysis for the Behavioral Sciences, 2nd Edition, Lawrence Erlbaum Associates

4. **Tukey, J. W. (1977)**: Exploratory Data Analysis, Addison-Wesley

### 14.4 Color Accessibility

1. **Okabe, M., & Ito, K. (2008)**: Color Universal Design (CUD), https://jfly.uni-koeln.de/color/

2. **Tol, P. (2021)**: Colour Schemes, https://personal.sron.nl/~pault/

---

## Appendix A: Quick Reference

### System Types
- `multi_model_database`
- `relational_database`
- `document_database`
- `graph_database`
- `vector_database`
- `key_value_store`
- `time_series_database`

### Adapter Types
- `NativeAdapter`
- `PostgreSQLAdapter`
- `MongoDBAdapter`
- `HTTPAdapter`
- `gRPCAdapter`
- `RedisAdapter`
- `CassandraAdapter`

### Workload Families
- `YCSB`
- `TPC-C`
- `TPC-H`
- `ANN-Benchmarks`
- `LDBC-SNB`
- `vLLM`
- `RAGBench`

---

**Document Version:** 1.0.0  
**Last Updated:** April 2026  
**License:** MIT  
**Maintainer:** CHIMERA Development Team
