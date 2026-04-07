# CHIMERA Suite: Vendor-Neutral Benchmark Configuration Format

**Version:** 1.0.0  
**Status:** Implementation Complete  
**Standards Compliance:** IEEE Std 2807-2022, ISO/IEC 14756:2015  
**License:** MIT

---

## Overview

The CHIMERA Suite (Comprehensive, Honest, Impartial Metrics for Empirical Reporting and Analysis) provides a vendor-neutral configuration format for database benchmarking. This implementation fulfills the requirement for a strictly neutral benchmark configuration format that enables fair, reproducible, and scientifically rigorous comparisons of database systems.

### Key Features

✅ **Vendor-Neutral System Definitions**
- Generic system identifiers (no branding)
- Standardized adapter naming (e.g., "PostgreSQLAdapter" not "vendor_pg_adapter")
- Objective, descriptive language throughout

✅ **IEEE/ACM/ISO Standards Compliance**
- IEEE Std 2807-2022 (Performance Benchmarking of DBMS)
- ISO/IEC 14756:2015 (Database Performance Measurement)
- All methodologies properly cited

✅ **Comprehensive Workload Coverage**
- YCSB (Yahoo! Cloud Serving Benchmark)
- TPC-C (OLTP Transactions)
- TPC-H (Decision Support)
- ANN-Benchmarks (Vector Search)
- LDBC-SNB (Graph Databases)
- vLLM (LLM Inference)
- RAGBench (Retrieval-Augmented Generation)

✅ **Statistical Rigor**
- Hypothesis testing (Welch's t-test, Mann-Whitney U)
- Effect size measures (Cohen's d, rank-biserial)
- Confidence intervals
- Outlier detection (IQR method)
- Proper significance testing

✅ **Complete Hardware Specification**
- Processor configuration (cores, frequency, cache, features)
- Memory configuration (type, speed, ECC, NUMA)
- Storage specification (NVMe/SATA, performance metrics)
- Network configuration (bandwidth, latency, topology)
- System settings (CPU governor, huge pages, I/O scheduler)

---

## Quick Start

### 1. Choose Your Configuration Format

**YAML (Recommended):**
```bash
cp benchmarks/chimera/benchmark_example.yaml my_benchmark.yaml
```

**TOML:**
```bash
cp benchmarks/chimera/benchmark_config_example.toml my_benchmark.toml
```

### 2. Configure Systems Under Test

```yaml
systems:
  - system_id: "system_multimodel_a"
    system_type: "multi_model_database"
    adapter:
      type: "NativeAdapter"
      protocol: "binary_wire"
      connection_string: "binary://localhost:18765"
```

### 3. Select Workloads

```yaml
workloads:
  - workload_id: "ycsb_workload_a"
    workload_family: "YCSB"
    parameters:
      record_count: 1000000
      operation_count: 1000000
```

### 4. Run Benchmark

```bash
chimera-benchmark --config my_benchmark.yaml
```

---

## File Structure

```
benchmarks/chimera/
├── benchmark_config_schema.yaml       # Complete schema specification
├── benchmark_config_example.toml      # TOML format example
├── benchmark_example.yaml             # Ready-to-use YAML example
├── README.md                          # This file
├── reporter.py                        # CHIMERA reporting engine
├── statistics.py                      # Statistical analysis
└── test_chimera.py                    # Test suite

docs/benchmarks/
├── CHIMERA_USER_GUIDE_NEUTRAL_INTEGRATION.md          # Main user guide
├── CHIMERA_WORKLOAD_REFERENCE.md                      # Workload details
├── CHIMERA_HARDWARE_SPECIFICATION_GUIDE.md            # Hardware specs
├── CHIMERA_STATISTICAL_METHODOLOGY_GUIDE.md           # Statistical methods
└── CHIMERA_SCIENTIFIC_FOUNDATION.md                   # Scientific background
```

---

## Documentation

### Core Documentation

1. **[User Guide: Neutral System Integration](../../docs/benchmarks/CHIMERA_USER_GUIDE_NEUTRAL_INTEGRATION.md)**
   - Complete guide to creating vendor-neutral configurations
   - System configuration guidelines
   - Hardware specification instructions
   - Statistical methodology overview
   - Best practices and troubleshooting

2. **[Workload Reference Guide](../../docs/benchmarks/CHIMERA_WORKLOAD_REFERENCE.md)**
   - Detailed documentation for all workload families
   - YCSB workload configurations (A, B, C, D, E, F)
   - TPC-C and TPC-H specifications
   - Vector search benchmarks (ANN-Benchmarks)
   - Graph benchmarks (LDBC-SNB)
   - LLM benchmarks (vLLM, RAGBench)
   - Custom workload creation

3. **[Hardware Specification Guide](../../docs/benchmarks/CHIMERA_HARDWARE_SPECIFICATION_GUIDE.md)**
   - IEEE Std 2807-2022 compliant hardware documentation
   - Complete processor specifications
   - Memory configuration guidelines
   - Storage performance measurement
   - Network configuration
   - System tuning for benchmarks

4. **[Statistical Methodology Guide](../../docs/benchmarks/CHIMERA_STATISTICAL_METHODOLOGY_GUIDE.md)**
   - Hypothesis testing procedures
   - Effect size measures
   - Confidence interval calculation
   - Outlier detection methods
   - Measurement protocols
   - Reporting standards

### Configuration Files

- **`benchmark_config_schema.yaml`** - Complete schema with all options documented
- **`benchmark_example.yaml`** - Practical ready-to-use example
- **`benchmark_config_example.toml`** - TOML format alternative

---

## Vendor Neutrality Guidelines

### ✅ DO

**System Naming:**
```yaml
system_id: "system_multimodel_a"    # Generic identifier
system_id: "system_relational_b"    # Type-based identifier
system_id: "system_document_c"      # Neutral naming
```

**Adapter Naming:**
```yaml
adapter:
  type: "PostgreSQLAdapter"         # Protocol/standard name
  type: "NativeAdapter"             # Generic for proprietary
  type: "MongoDBAdapter"            # Wire protocol name
```

**Descriptive Language:**
```yaml
description: "Multi-model database with native AI support"
description: "Traditional relational database system"
```

### ❌ DON'T

**Avoid Brand Names:**
```yaml
system_id: "themisdb_production"    # ❌ Contains vendor name
system_id: "our_database_system"    # ❌ Shows bias
```

**Avoid Branded Adapters:**
```yaml
adapter:
  type: "themisdb_native_adapter"   # ❌ Branded naming
  type: "our_special_connector"     # ❌ Marketing language
```

**Avoid Marketing Language:**
```yaml
description: "Best-in-class enterprise solution"  # ❌ Marketing
description: "Industry-leading performance"       # ❌ Promotional
```

---

## Configuration Sections

### 1. Metadata

```yaml
metadata:
  benchmark_suite: "Database Performance Evaluation"
  version: "1.0.0"
  date: "2026-01-19T12:00:00Z"
  operator: "Research Institution"
  standards:
    - "IEEE Std 2807-2022"
    - "ISO/IEC 14756:2015"
```

### 2. Systems Under Test

```yaml
systems:
  - system_id: "system_multimodel_a"
    system_type: "multi_model_database"
    architecture:
      type: "native_multi_model"
      storage_engine: "lsm_tree"
      transaction_model: "mvcc"
      consistency: "acid"
    adapter:
      type: "NativeAdapter"
      protocol: "binary_wire"
    data_models:
      - "relational"
      - "document"
      - "graph"
      - "vector"
```

### 3. Hardware Configuration

```yaml
hardware:
  processor:
    architecture: "x86_64"
    cores:
      physical: 8
      logical: 16
    frequency:
      base_ghz: 3.6
      turbo_ghz: 5.0
    cache:
      l3_mb: 16
    features: ["AVX2", "AVX512"]
    
  memory:
    total_gb: 64
    type: "DDR4"
    speed_mhz: 3200
    ecc_enabled: true
    
  storage:
    primary:
      type: "NVMe SSD"
      capacity_gb: 1000
      performance:
        sequential_read_mbps: 7000
        random_read_iops: 1000000
```

### 4. Workloads

```yaml
workloads:
  - workload_id: "ycsb_workload_a"
    workload_family: "YCSB"
    standard_reference: "Cooper et al., SoCC 2010, DOI: 10.1145/1807128.1807152"
    parameters:
      record_count: 1000000
      operations:
        read: 50
        update: 50
```

### 5. Statistical Methodology

```yaml
methodology:
  statistical_testing:
    alpha: 0.05
    confidence_level: 0.95
    tests:
      - name: "Welch's t-test"
        reference: "Welch, B. L. (1947). Biometrika, 34(1-2), 28-35"
    effect_sizes:
      - name: "Cohen's d"
        reference: "Cohen, J. (1988). Statistical Power Analysis"
  outlier_detection:
    method: "IQR"
    threshold: 1.5
  measurement:
    warm_up_seconds: 60
    measurement_duration_seconds: 300
    num_runs: 5
```

---

## Standard Workload Families

### YCSB (Yahoo! Cloud Serving Benchmark)

**Citation:** Cooper et al., "Benchmarking Cloud Serving Systems with YCSB", SoCC 2010

**Workloads:**
- **Workload A:** Update Heavy (50% read, 50% update)
- **Workload B:** Read Heavy (95% read, 5% update)
- **Workload C:** Read Only (100% read)
- **Workload D:** Read Latest (95% read, 5% insert)
- **Workload E:** Scan Heavy (95% scan, 5% insert)
- **Workload F:** Read-Modify-Write (50% read, 50% RMW)

### TPC Benchmarks

**TPC-C:** OLTP transactions (order entry simulation)
- **Citation:** TPC-C Revision 5.11, http://www.tpc.org/tpcc/
- **Metrics:** tpmC (transactions per minute C)

**TPC-H:** Decision support queries
- **Citation:** TPC-H Revision 3.0.0, http://www.tpc.org/tpch/
- **Metrics:** QphH (queries per hour H)

### Vector Search (ANN-Benchmarks)

**Citation:** Aumüller et al., Information Systems 2020

**Datasets:**
- SIFT1M (128D image features)
- GIST (960D image features)
- GloVe (word embeddings)

**Metrics:** Recall@k, QPS, Build Time

### Graph (LDBC-SNB)

**Citation:** Erling et al., SIGMOD 2015

**Workload Types:**
- Interactive (short reads, complex reads, updates)
- Business Intelligence (analytical queries)
- Graph Analytics (PageRank, community detection)

### LLM Benchmarks

**vLLM:** Inference serving
- **Citation:** Kwon et al., SOSP 2023
- **Metrics:** Throughput, latency, GPU utilization

**RAGBench:** Retrieval-Augmented Generation
- **Citation:** Chen et al., AAAI 2024
- **Metrics:** Answer accuracy, retrieval quality, latency

---

## Statistical Methods

### Hypothesis Testing

**Welch's t-test:**
- Comparing means of two systems
- Handles unequal variances
- Parametric test

**Mann-Whitney U test:**
- Non-parametric alternative
- Robust to outliers
- Ordinal data

### Effect Sizes

**Cohen's d:**
- Standardized mean difference
- Thresholds: 0.2 (small), 0.5 (medium), 0.8 (large)

**Rank-biserial correlation:**
- Non-parametric effect size
- Range: -1 to 1

### Confidence Intervals

- 95% confidence level (default)
- t-distribution or bootstrap methods
- Quantifies uncertainty

### Outlier Detection

**IQR Method:**
- Q1 - 1.5 × IQR (lower bound)
- Q3 + 1.5 × IQR (upper bound)
- Document removed outliers

---

## Acceptance Criteria Status

### ✅ Completed

- [x] **Complete YAML Schema Exists**
  - `benchmark_config_schema.yaml` with all sections
  - `benchmark_example.yaml` ready-to-use example
  - `benchmark_config_example.toml` TOML alternative

- [x] **All Parameters System-Neutral and Documented**
  - Generic system identifiers throughout
  - IEEE/ACM/ISO references for all methodologies
  - No vendor-identifying terminology

- [x] **Clearly Separated Sections**
  - Hardware specification (IEEE Std 2807-2022 compliant)
  - Statistical methodology (with citations)
  - Workload standardization (standard benchmark families)

- [x] **No Names Allowing Conclusions About Primary System**
  - System IDs: `system_multimodel_a`, `system_relational_b`, etc.
  - Adapter types: Generic or protocol-based names only
  - All language objective and descriptive

### 📋 Pending

- [ ] **External Community Review**
  - Requires submission to at least 2 external reviewers
  - Community feedback incorporation
  - Final approval process

---

## Review Checklist for External Reviewers

### Vendor Neutrality

- [ ] All system identifiers are vendor-neutral
- [ ] Adapter naming follows protocol/standard conventions
- [ ] No marketing or promotional language
- [ ] No vendor logos or branding in configurations
- [ ] No implied favoritism in system ordering

### Scientific Rigor

- [ ] All methodologies properly cited (IEEE/ACM/ISO)
- [ ] Statistical methods appropriate and documented
- [ ] Hardware specification complete per IEEE Std 2807-2022
- [ ] Measurement protocols ensure reproducibility
- [ ] Workload definitions match standard specifications

### Completeness

- [ ] All major workload families covered
- [ ] Hardware specification comprehensive
- [ ] Statistical methodology documented
- [ ] Examples are functional and clear
- [ ] Documentation is accessible and thorough

### Accessibility

- [ ] Color-blind friendly reporting (Okabe-Ito palette)
- [ ] Clear documentation structure
- [ ] Examples provided
- [ ] Terminology explained
- [ ] Citation format consistent

---

## Usage Examples

### Example 1: Comparing Two Systems (YCSB)

```yaml
metadata:
  benchmark_suite: "Multi-Model vs Relational Comparison"
  version: "1.0.0"

systems:
  - system_id: "system_multimodel_a"
    adapter:
      type: "NativeAdapter"
      connection_string: "binary://localhost:18765"
      
  - system_id: "system_relational_b"
    adapter:
      type: "PostgreSQLAdapter"
      connection_string: "postgresql://localhost:5432/benchmark"

workloads:
  - workload_id: "ycsb_workload_a"
    workload_family: "YCSB"
    parameters:
      record_count: 1000000
      operation_count: 1000000
      operations:
        read: 50
        update: 50

methodology:
  statistical_testing:
    alpha: 0.05
    tests:
      - name: "Welch's t-test"
      - name: "Mann-Whitney U test"
  measurement:
    num_runs: 5
```

### Example 2: Vector Search Comparison

```yaml
workloads:
  - workload_id: "ann_sift1m"
    workload_family: "ANN-Benchmarks"
    standard_reference: "Aumüller et al., Information Systems 2020"
    parameters:
      dataset: "SIFT1M"
      dataset_size: 1000000
      dimensions: 128
      distance_metric: "euclidean"
      k_nearest: 10
      recall_target: 0.95
      index_parameters:
        index_type: "hnsw"
        ef_construction: 200
        m: 16
```

---

## Contributing

We welcome contributions to improve the CHIMERA Suite configuration format:

1. **Fork the repository**
2. **Create a feature branch**
3. **Make your changes**
4. **Ensure vendor neutrality**
5. **Add proper citations**
6. **Submit a pull request**

### Contribution Guidelines

- Maintain vendor neutrality
- Cite all methodologies
- Follow existing format conventions
- Update documentation
- Add tests for new features

---

## License

MIT License - See LICENSE file for details

---

## Citation

If you use the CHIMERA Suite configuration format in your research, please cite:

```bibtex
@software{chimera_config_2026,
  title = {CHIMERA Suite: Vendor-Neutral Benchmark Configuration Format},
  author = {CHIMERA Development Team},
  year = {2026},
  version = {1.0.0},
  url = {https://github.com/chimera-benchmark/chimera-suite},
  note = {IEEE Std 2807-2022 compliant}
}
```

---

## References

### Standards

1. **IEEE Std 2807-2022**: IEEE Standard for Framework for Performance Benchmarking of Database Management Systems

2. **ISO/IEC 14756:2015**: Information technology - Measurement and rating of performance of computer-based software systems

3. **IEEE Std 730-2014**: IEEE Standard for Software Quality Assurance Processes

### Benchmarks

1. **Cooper et al. (2010)**: "Benchmarking Cloud Serving Systems with YCSB", ACM SoCC, DOI: 10.1145/1807128.1807152

2. **TPC-C**: Transaction Processing Performance Council Benchmark C, Revision 5.11

3. **TPC-H**: Transaction Processing Performance Council Benchmark H, Revision 3.0.0

4. **Aumüller et al. (2020)**: "ANN-Benchmarks: A Benchmarking Tool for Approximate Nearest Neighbor Algorithms", Information Systems

5. **Erling et al. (2015)**: "The LDBC Social Network Benchmark: Interactive Workload", ACM SIGMOD

### Statistical Methods

1. **Welch (1947)**: "The generalization of 'Student's' problem", Biometrika, 34(1-2), 28-35

2. **Mann & Whitney (1947)**: "On a test of whether one of two random variables is stochastically larger", Ann. Math. Stat., 18(1), 50-60

3. **Cohen (1988)**: Statistical Power Analysis for the Behavioral Sciences, 2nd Edition

4. **Tukey (1977)**: Exploratory Data Analysis, Addison-Wesley

---

## Support

For questions, issues, or suggestions:

- **GitHub Discussions:** [CHIMERA Suite Community](https://github.com/chimera-benchmark/chimera-suite/discussions)
- **Documentation:** Full documentation available in the docs/ directory
- **Examples:** See the chimera/ directory for configuration examples

---

**CHIMERA v1.0.0** - *Honest metrics for everyone*

**Last Updated:** April 2026  
**Maintainer:** CHIMERA Development Team  
**Status:** Ready for Community Review
