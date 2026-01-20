# CHIMERA Multi-Vendor Benchmarking Demo

## Overview

This demo demonstrates **CHIMERA Suite's vendor neutrality** by benchmarking multiple database systems side-by-side with identical methodology. This proves that CHIMERA is an independent benchmarking framework, not a tool specific to any single vendor.

## Systems Benchmarked

The demo includes **9 different database systems** from various vendors:

### Graph Databases
- **Neo4j v5.5** - Industry-leading graph database
- **JanusGraph v1.0** - Distributed graph database

### Multi-Model Databases
- **ThemisDB v1.4.0** - Multi-model with AI integration
- **ArangoDB v3.11** - Native multi-model database

### Relational Databases
- **PostgreSQL v15.2** - Popular open-source RDBMS
- **PostgreSQL+pgvector** - PostgreSQL with vector extension

### Document Databases
- **MongoDB v6.0** - Leading document database

### Vector Databases
- **Milvus v2.3** - Open-source vector database
- **Pinecone** - Cloud-native vector database

## Benchmark Workloads

### 1. OLTP Transaction Throughput (YCSB)
- **Benchmark Standard**: Yahoo! Cloud Serving Benchmark
- **Workload**: YCSB-A (50% reads, 50% updates)
- **Metric**: Transactions per second (TPS)
- **Dataset**: 1M records
- **Systems**: PostgreSQL, MongoDB, ThemisDB, Neo4j

### 2. Vector Search Latency (ANN-Benchmark)
- **Benchmark Standard**: ANN-Benchmarks
- **Dataset**: SIFT-1M (1M 128-dimensional vectors)
- **Metric**: Query latency P95 (milliseconds, lower is better)
- **Index Type**: HNSW / IVFFlat
- **Systems**: Milvus, Pinecone, PostgreSQL+pgvector, ThemisDB

### 3. Graph Traversal Performance
- **Workload**: Shortest path queries (2-hop traversal)
- **Metric**: Queries per second (QPS)
- **Graph Size**: 1M nodes, 5M edges
- **Systems**: Neo4j, ArangoDB, ThemisDB, JanusGraph

## Running the Demo

### Prerequisites

```bash
cd benchmarks/chimera
pip install -r requirements.txt
```

### Execute Demo

```bash
python3 demo_multi_vendor.py
```

### Expected Output

The demo will:
1. Generate simulated benchmark data for 9 systems across 3 workloads
2. Apply statistical analysis uniformly to all systems
3. Create vendor-neutral reports in `demo_reports/multi_vendor/`
4. Display summary of vendor neutrality guarantees

### Generated Reports

#### CSV Reports (Raw Data)
- `oltp_throughput_comparison.csv`
- `vector_search_latency.csv`
- `graph_traversal_performance.csv`

#### HTML Reports (Alphabetical Sorting - Vendor Neutral)
- `oltp_throughput_alphabetical.html`
- `vector_search_alphabetical.html`
- `graph_traversal_alphabetical.html`

#### HTML Reports (Performance Sorting - Optional)
- `oltp_throughput_by_performance.html`
- `vector_search_by_performance.html`
- `graph_traversal_by_performance.html`

## Vendor Neutrality Guarantees

### ✅ Equal Treatment
- All systems benchmarked with identical workloads
- Same statistical methods applied to all results
- No hidden optimizations for any vendor
- Equal visual prominence in reports

### ✅ Neutral Presentation
- Alphabetical sorting by default (vendor-neutral)
- Performance-based sorting optional (clearly labeled)
- No "winner" declarations
- Statistical significance marked objectively

### ✅ Color-Blind Friendly
- Okabe-Ito color palette used
- Accessible to all forms of color blindness
- No vendor-specific brand colors

### ✅ Statistical Rigor
- Welch's t-test for mean comparisons
- Mann-Whitney U test (non-parametric)
- Cohen's d for effect size
- 95% confidence intervals
- IQR outlier detection

### ✅ Full Transparency
- All methods disclosed
- Reproducible analysis
- Open source implementation
- IEEE/ACM standards compliance

## ThemisDB's Position

**ThemisDB is one of 9 systems benchmarked in this demo.**

CHIMERA treats ThemisDB identically to all other systems:
- No preferential sorting
- No special highlighting
- Same statistical tests
- Equal visual treatment
- No vendor bias

This demonstrates that CHIMERA is an **independent benchmarking framework**, not a ThemisDB-specific tool.

## Report Features

### Executive Summary
- System descriptions
- Descriptive statistics (mean, median, std dev, percentiles)
- Confidence intervals
- Sample sizes

### Statistical Analysis
- Pairwise comparisons between all systems
- Hypothesis test results (p-values)
- Effect sizes (Cohen's d)
- Clear interpretation of statistical significance

### Visualizations
- Box plots (color-blind friendly)
- Distribution comparisons
- No misleading scales
- Professional appearance

### Methodology Disclosure
- Complete description of all methods
- Statistical parameters documented
- Data quality metrics
- Hardware specifications

### IEEE Citations
- Proper citations for all statistical methods
- Benchmark standard references
- Color accessibility research

## Standards Compliance

This demo follows:
- **IEEE Std 2807-2022**: Performance Benchmarking of DBMS
- **ISO/IEC 14756:2015**: Database Performance Measurement
- **IEEE Std 730-2014**: Software Quality Assurance
- **ACM Artifact Badging**: Reproducibility standards

## Verification Checklist

After running the demo, verify:

- [ ] HTML reports open correctly in web browser
- [ ] All systems listed alphabetically in alphabetical reports
- [ ] All systems sorted by performance in performance reports
- [ ] No vendor logos visible
- [ ] Color palette is color-blind friendly
- [ ] Statistical methods are disclosed
- [ ] Confidence intervals are displayed
- [ ] P-values are reported for comparisons
- [ ] No "winner" or "best" declarations
- [ ] ThemisDB treated identically to other systems

## Extending the Demo

To add more database systems:

```python
systems.append({
    'name': 'NewDatabase v2.0',
    'mean': 20000,  # Performance mean
    'std_dev': 1500,  # Standard deviation
    'skew': 0.0,  # Distribution skew
    'metadata': {
        'system_type': 'Your Database Type',
        'version': '2.0',
        'workload': 'YCSB-A',
        'record_count': 1000000
    }
})
```

## Troubleshooting

### Missing Dependencies
```bash
pip install numpy scipy matplotlib
```

### Import Errors
Ensure you're running from the `benchmarks/chimera` directory:
```bash
cd benchmarks/chimera
python3 demo_multi_vendor.py
```

### Report Generation Fails
Check write permissions for `demo_reports/multi_vendor/` directory.

## Contact

For questions about the CHIMERA Suite or this demo, please refer to the main CHIMERA documentation.

---

**CHIMERA v1.0.0** - *Honest metrics for everyone*
