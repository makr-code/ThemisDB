# CHIMERA Suite

**C**omprehensive, **H**onest, **I**mpartial **M**etrics for **E**mpirical **R**eporting and **A**nalysis

[![IEEE Compliant](https://img.shields.io/badge/IEEE-Compliant-blue)](https://www.ieee.org/)
[![Color Blind Friendly](https://img.shields.io/badge/ColorBlind-Friendly-green)](https://jfly.uni-koeln.de/color/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## What is CHIMERA?

CHIMERA Suite is a **vendor-neutral, scientifically rigorous** framework for database benchmark reporting and visualization. It ensures fair, transparent, and accessible comparison of database systems without vendor bias.

### Why CHIMERA?

Database benchmarking often suffers from:
- **Vendor bias** in result presentation
- **Inconsistent methodologies** across benchmarks
- **Inaccessible visualizations** for color-blind users
- **Lack of statistical rigor** in comparisons

CHIMERA solves these problems by providing a standardized, neutral framework that any database system can use.

---

## Key Features

### 🎨 Vendor-Neutral Design
- ✓ **Color-Blind Friendly Palettes**: Uses Okabe-Ito and Paul Tol color schemes
- ✓ **No Vendor Bias**: Systems sorted alphabetically or by metric value only
- ✓ **No Branding**: No logos, brand colors, or marketing materials
- ✓ **Transparent Methodology**: All methods fully disclosed and referenced

### 📊 Statistical Rigor (IEEE Compliant)
- ✓ **Welch's t-test**: For comparing means with unequal variances
- ✓ **Mann-Whitney U test**: Non-parametric alternative
- ✓ **Cohen's d**: Effect size measurement
- ✓ **Confidence Intervals**: 95% CI using t-distribution
- ✓ **Outlier Detection**: IQR method (1.5 × IQR)
- ✓ **Comprehensive Statistics**: Mean, median, std dev, P25, P75, P95, P99

### 📄 Multiple Report Formats
- ✓ **HTML**: Interactive reports with visualizations
- ✓ **CSV**: Raw statistics for further analysis
- ✓ **PDF**: Printable reports (via HTML rendering)

### 📚 IEEE Citations
- ✓ Proper citations for all statistical methods
- ✓ References to color accessibility research
- ✓ Benchmark standard citations (TPC-C, TPC-H, YCSB)

---

## Supported Systems

CHIMERA is designed to benchmark **any database system** including:

- **Multi-Model Databases** (e.g., ThemisDB, ArangoDB, OrientDB)
- **Relational Databases** (e.g., PostgreSQL, MySQL, Oracle)
- **NoSQL Databases** (e.g., MongoDB, Cassandra, Redis)
- **Graph Databases** (e.g., Neo4j, JanusGraph, TigerGraph)
- **Vector Databases** (e.g., Pinecone, Weaviate, Milvus)
- **Time-Series Databases** (e.g., InfluxDB, TimescaleDB, QuestDB)

ThemisDB is **one of many** database systems that can be evaluated with CHIMERA.

---

## Installation

```bash
cd benchmarks/chimera
pip install -r requirements.txt
```

### Requirements
- Python 3.8+
- numpy
- scipy
- matplotlib (optional, for visualizations)
- weasyprint (optional, for PDF generation)

---

## Quick Start

### Basic Usage

```python
from chimera import ChimeraReporter

# Create reporter
reporter = ChimeraReporter(significance_level=0.05)

# Add benchmark results for each system
reporter.add_system_results(
    system_name="System A",
    metric_name="Query Throughput",
    metric_unit="queries/sec",
    data=[15000, 15200, 14800, 15100, ...]
)

reporter.add_system_results(
    system_name="System B",
    metric_name="Query Throughput",
    metric_unit="queries/sec",
    data=[12500, 12700, 12300, 12600, ...]
)

# Generate reports
reporter.generate_html_report("report.html", sort_by='alphabetical')
reporter.generate_csv_report("results.csv", sort_by='alphabetical')
```

### Run Demo

**Basic Demo** (3 systems):
```bash
cd benchmarks/chimera
python3 demo.py
```

**Multi-Vendor Demo** (9 systems - proves vendor neutrality):
```bash
cd benchmarks/chimera
python3 demo_multi_vendor.py
```

See [MULTI_VENDOR_DEMO.md](MULTI_VENDOR_DEMO.md) for details on the multi-vendor demonstration.

Reports are generated in the `demo_reports/` directory.

### Standard-Suite CLI (Compliance-focused)

Use the `list` command to inspect standards, coverage, and compliance requirements:

```bash
python chimera_cli.py list --standards
python chimera_cli.py list --search-standards rag
python chimera_cli.py list --standard-details BEIR
python chimera_cli.py list --standard-coverage
python chimera_cli.py list --compliance MLPerf-Inference
```

### Runtime Compliance Artifacts

During endurance runs, CHIMERA writes standard-focused artifacts to the result directory:

- `result_schema.json`: Canonical schema definition for standard-conform result rows.
- `standard_golden_baselines.json`: Deterministic golden baselines per benchmark standard.
- `dataset_provisioning.json`: Reproducible dataset-provisioning manifest with fingerprint.

In addition, the repository contains a static reference seed:

- `baselines/chimera/standard_golden_baselines.seed.json`: Versioned repository seed for baseline intent.

Interpretation boundary:

- The repository seed is a static reference artifact.
- `standard_golden_baselines.json` is the runtime-generated artifact for a concrete run context.

These artifacts can be versioned and used for regression gating and auditability.

### Scope Boundaries and Non-Goals

CHIMERA separates three layers for standard-related claims:

1. Standard requirements layer (required metrics/parameters per benchmark standard)
2. Runtime implementation layer (actually implemented workload set)
3. Reporting layer (schema-enriched result rows and exported artifacts)

Non-goals:

- CHIMERA does not claim official certification of external standards (for example, formal TPC or MLPerf submission status).
- CHIMERA does not perform hidden vendor-specific tuning to improve one system's rank.
- CHIMERA does not claim cross-hardware comparability without documented hardware and methodology controls.

Compliance statements should always combine requirement definitions with runtime coverage (`is_fully_implemented`) and emitted runtime artifacts.

---

## Neutrality Guarantees

### 1. System Naming
- Names normalized (remove marketing terms)
- No vendor/product identifiers in sorting
- Equal visual prominence

### 2. Color Assignment
- Colors assigned by palette order
- Never by brand/vendor
- Consistent across reports

### 3. Result Presentation
- No "winner" declared
- Statistical significance marked objectively
- Effect sizes always reported

### 4. Methodology
- All methods disclosed
- No hidden optimizations
- Reproducible analysis

---

## Documentation

- [README.md](README.md) - Technical implementation guide
- [CHIMERA_STYLEGUIDE.md](CHIMERA_STYLEGUIDE.md) - Branding and design guidelines
- [BENCHMARK_CONFIG_README.md](BENCHMARK_CONFIG_README.md) - Configuration format

---

## Standards Compliance

CHIMERA complies with:
- **IEEE Std 2807-2022**: Performance Benchmarking of DBMS
- **ISO/IEC 14756:2015**: Database Performance Measurement
- **IEEE Std 730-2014**: Software Quality Assurance Processes
- **IEEE Std 1012-2016**: System, Software, and Hardware Verification

---

## License

MIT License - see LICENSE file for details.

---

## Contributing

Contributions welcome! Please ensure:
- Statistical methods are properly referenced
- Color palettes remain color-blind friendly
- No vendor bias introduced
- Tests pass

---

## Contact

For questions or contributions, please refer to the project documentation.

---

**CHIMERA v1.0.0** - *Honest metrics for everyone*
