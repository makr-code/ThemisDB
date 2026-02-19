# CHIMERA Suite

**C**omprehensive **H**ybrid **I**nferencing & **M**ulti-model **E**valuation **R**esource **A**ssessment

_"Benchmark the Unbenchmarkable"_

> The CHIMERA Suite is a scientifically rigorous, vendor-neutral benchmark framework for multi-model databases with native AI/LLM integration. Like the mythical Chimera - a hybrid creature - this suite evaluates the diverse capabilities of modern database systems: Graph, Vector, Relational, Document models, combined with LLM/LoRA inferencing capabilities.

## NEW: C++ Adapter Architecture

The CHIMERA Suite now includes a **vendor-neutral C++ adapter architecture** for integrating arbitrary database systems into benchmark workloads. This provides:

- **Complete Vendor Neutrality**: No system-specific names, colors, or concepts
- **Multi-Model Support**: Interfaces for Relational, Vector, Graph, Document, and Transaction operations
- **Factory Pattern**: Dynamic adapter registration and instantiation
- **Capability Discovery**: Runtime querying of supported features
- **IEEE Compliance**: Standards-compliant design and documentation

### Quick Start (C++ Adapter)

```cpp
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

// Register your database adapter
AdapterFactory::register_adapter("MyDatabase",
    []() { return std::make_unique<MyDatabaseAdapter>(); });

// Create and use adapter
auto adapter = AdapterFactory::create("MyDatabase");
adapter->connect("connection_string");

// Check capabilities
if (adapter->has_capability(Capability::VECTOR_SEARCH)) {
    // Perform vector operations
}
```

### Documentation

- **Full API Reference**: See [docs/chimera/ARCHITECTURE_INTERFACE.md](../../docs/chimera/ARCHITECTURE_INTERFACE.md)
- **Example Program**: [chimera_example.cpp](chimera_example.cpp)
- **Headers**: [include/chimera/database_adapter.hpp](../../include/chimera/database_adapter.hpp)

### Building the Example

```bash
cd benchmarks/chimera
g++ -std=c++17 -I../../include chimera_example.cpp \
    ../../src/chimera/adapter_factory.cpp \
    ../../src/chimera/themisdb_adapter.cpp \
    -o chimera_example
./chimera_example
```

---

[![IEEE Compliant](https://img.shields.io/badge/IEEE-Compliant-blue)](https://www.ieee.org/)
[![Color Blind Friendly](https://img.shields.io/badge/ColorBlind-Friendly-green)](https://jfly.uni-koeln.de/color/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

The CHIMERA Suite is a comprehensive benchmark framework designed for evaluating modern multi-model databases with AI capabilities. It provides scientifically rigorous, vendor-neutral reporting and visualization that complies with IEEE/ACM standards, ensuring fair, transparent, and accessible reporting of benchmark results without vendor bias.

### What Makes CHIMERA Unique

- **Multi-Model Coverage**: Benchmarks for Relational, Graph, Vector (embeddings), and Document models
- **AI/LLM Integration**: Native support for evaluating LLM inference, LoRA adapters, and RAG workflows
- **Hybrid Workloads**: Tests that span multiple data models in single transactions
- **Scientific Rigor**: IEEE-compliant statistical methodology with proper citations
- **Vendor Neutrality**: No branding, fair comparisons, color-blind friendly visualizations

## Key Features

### 🧪 Comprehensive Test Coverage (68 Tests)
- ✓ **Multi-Model Database Operations**: Full coverage of relational, vector, graph, document, and hybrid workloads
- ✓ **RAG Pipeline Testing**: Retrieval, generation, and end-to-end RAG workflows
- ✓ **LLM-as-Judge Evaluation**: Faithfulness, relevance, hallucination detection, citation quality
- ✓ **Ethical AI Guardrails**: Autonomy, bias detection, citation ethics, VETO capability
- ✓ **RAG Metrics**: Context precision/recall, answer faithfulness/relevance, toxicity checks
- ✓ **Statistical Framework**: All statistical methods validated with unit tests

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

### 🤖 LLM/RAG/Ethics Evaluation
- ✓ **RAG Pipeline Support**: End-to-end retrieval-augmented generation testing
- ✓ **LLM-as-Judge**: Automated quality assessment (faithfulness, relevance, hallucination detection)
- ✓ **Ethics Compliance**: IEEE P7000-aligned ethical AI evaluation (autonomy, bias, transparency)
- ✓ **RAG Metrics**: RAGBench and RAGAS-style evaluation metrics
- ✓ **Citation Quality**: Proper attribution and plagiarism detection
- ✓ **Toxicity Screening**: Content safety and bias detection

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

```bash
cd benchmarks/chimera
python3 demo.py
```

This generates example reports in `demo_reports/` directory.

## Report Contents

### 1. Neutrality Seal
Every report includes a prominent neutrality certification:
- Confirmation of vendor-neutral methodology
- List of neutrality guarantees
- Disclosure of all methods used

### 2. Executive Summary
- System names (normalized)
- Descriptive statistics for each system
- Confidence intervals
- Sample sizes

### 3. Statistical Analysis
- Pairwise comparisons using t-tests and Mann-Whitney U
- Effect sizes (Cohen's d, rank-biserial correlation)
- Clear interpretation of results
- Warning: "Statistical significance ≠ practical significance"

### 4. Visualizations
- Box plots with color-blind friendly colors
- Distribution comparisons
- No misleading scales or truncated axes

### 5. Methodology Disclosure
- Complete description of all statistical methods
- Significance levels and confidence intervals
- Outlier removal procedures
- Data quality metrics

### 6. IEEE Citations
- Proper citations for all methods (IEEE style)
- References to validation standards
- Benchmark specifications

## Sorting Options

### Alphabetical (Default)
Systems sorted A-Z by name:
```python
reporter.generate_html_report("report.html", sort_by='alphabetical')
```

### By Metric
Systems sorted by mean performance (descending):
```python
reporter.generate_html_report("report.html", sort_by='metric')
```

**Important**: Neither sorting implies "winner" - both are neutral presentations.

## Color Palettes

### Okabe-Ito Palette (Default)
Designed specifically for color-blind accessibility:
- Blue: `#0072B2`
- Vermillion: `#D55E00`
- Bluish Green: `#009E73`
- Yellow: `#F0E442`
- Sky Blue: `#56B4E9`
- Reddish Purple: `#CC79A7`
- Orange: `#E69F00`

### Paul Tol's Muted Palette
Professional, softer colors:
- Indigo, Cyan, Teal, Green, Olive, Sand, Rose, Wine, Purple

All palettes tested for:
- Deuteranopia (red-green color blindness)
- Protanopia (red color blindness)
- Tritanopia (blue-yellow color blindness)

## Statistical Methods

### Outlier Detection
**Method**: Interquartile Range (IQR)
- Lower bound: Q1 - 1.5 × IQR
- Upper bound: Q3 + 1.5 × IQR
- **Reference**: Tukey (1977)

### Hypothesis Testing
**Welch's t-test**: For comparing means
- Handles unequal variances
- Two-tailed test
- α = 0.05 (default)
- **Reference**: Welch (1947)

**Mann-Whitney U test**: Non-parametric alternative
- No normality assumption
- Robust to outliers
- **Reference**: Mann & Whitney (1947)

### Effect Size
**Cohen's d**: Standardized difference
- Small: 0.2
- Medium: 0.5
- Large: 0.8
- **Reference**: Cohen (1988)

**Rank-biserial correlation**: For Mann-Whitney
- Range: -1 to 1
- Similar interpretation to correlation

### Confidence Intervals
- 95% confidence level (default)
- Uses t-distribution
- Accounts for sample size

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

## API Reference

### ChimeraReporter

```python
class ChimeraReporter(significance_level: float = 0.05)
```

Main reporting engine.

**Methods**:
- `add_system_results(system_name, metric_name, metric_unit, data, metadata=None)`
- `generate_html_report(output_path, sort_by='alphabetical', include_plots=True)`
- `generate_csv_report(output_path, sort_by='alphabetical')`
- `generate_pdf_report(output_path, sort_by='alphabetical')`

### StatisticalAnalyzer

```python
class StatisticalAnalyzer(significance_level: float = 0.05)
```

Statistical analysis engine.

**Methods**:
- `descriptive_statistics(data, remove_outliers=True) -> DescriptiveStats`
- `t_test(data1, data2, paired=False) -> StatisticalResult`
- `mann_whitney_u(data1, data2) -> StatisticalResult`
- `cohens_d(data1, data2) -> float`
- `confidence_interval(data, confidence=0.95) -> Tuple[float, float]`
- `compare_systems(system_a_data, system_b_data, ...) -> Dict`

### ColorBlindPalette

```python
class ColorBlindPalette
```

Color palette manager.

**Methods**:
- `get_palette(name='okabe_ito') -> List[str]`
- `get_sequential_palette(n, palette='tol_muted') -> List[str]`
- `get_diverging_palette() -> Dict[str, str]`
- `get_matplotlib_colors(n, palette='tol_muted') -> List[Tuple]`

### CitationManager

```python
class CitationManager
```

IEEE citation manager.

**Methods**:
- `add_citation(citation: Citation)`
- `get_citation(citation_id: str) -> Citation`
- `format_bibliography(citation_ids: List[str]) -> str`
- `get_neutrality_statement() -> str`

## Testing

The CHIMERA Suite includes comprehensive test coverage for all components:

### Test Suites

1. **test_chimera.py** - Core framework tests (22 tests)
   - Statistical analysis (descriptive stats, outlier removal, t-tests, Mann-Whitney U, Cohen's d, confidence intervals)
   - Color-blind friendly palettes (Okabe-Ito, Tol palettes, sequential/diverging colors)
   - IEEE citation management
   - Report generation (HTML, CSV)

2. **test_database_adapters.py** - Multi-model database adapter tests (28 tests)
   - Relational CRUD operations (connect, insert, batch insert, queries)
   - Vector search operations (kNN, ANN, metadata filtering, distance metrics)
   - Graph traversal (node/edge insertion, shortest path, BFS traversal, edge filters)
   - Transaction management (begin, commit, rollback, isolation levels)
   - Hybrid workloads (cross-model operations, transactions spanning multiple models)
   - Capability discovery and error handling

3. **test_llm_rag_ethics.py** - LLM/RAG/Ethics evaluation tests (18 tests)
   - RAG pipeline (retrieval, generation, end-to-end)
   - LLM-as-judge evaluation (faithfulness, relevance, hallucination detection, citation quality)
   - Ethical AI evaluation (autonomy, bias detection, citation ethics, VETO capability)
   - RAG metrics (context precision/recall, answer faithfulness/relevance, toxicity)
   - Comprehensive ethics evaluation workflow

### Running Tests

Run all tests:
```bash
cd benchmarks/chimera
pytest test_chimera.py test_database_adapters.py test_llm_rag_ethics.py -v
```

Run specific test suite:
```bash
pytest test_chimera.py -v                  # Core framework tests
pytest test_database_adapters.py -v        # Database adapter tests
pytest test_llm_rag_ethics.py -v           # LLM/RAG/Ethics tests
```

Run with coverage:
```bash
pytest --cov=. --cov-report=html
```

## Neutrality Documentation

CHIMERA provides comprehensive documentation on vendor neutrality:

- **[NEUTRALITY_GUARANTEES.md](NEUTRALITY_GUARANTEES.md)** - Complete neutrality principles and guarantees
- **[NEUTRALITY_STYLEGUIDE.md](NEUTRALITY_STYLEGUIDE.md)** - Style guide for contributors
- **[ADAPTER_API.md](ADAPTER_API.md)** - API documentation for integrating new systems

### Neutrality Verification

Check compliance with neutrality standards:

```bash
cd benchmarks/chimera
python3 neutrality_linter.py
```

The linter checks for:
- Vendor-specific names in code/config
- Marketing terminology
- Non-neutral color schemes
- Biased report presentation

## References

### Statistical Methods

1. **Cohen, J. (1988)**. Statistical Power Analysis for the Behavioral Sciences. Lawrence Erlbaum Associates.

2. **Mann, H. B., & Whitney, D. R. (1947)**. On a test of whether one of two random variables is stochastically larger than the other. The Annals of Mathematical Statistics, 18(1), 50-60.

3. **Welch, B. L. (1947)**. The generalization of 'Student's' problem when several different population variances are involved. Biometrika, 34(1-2), 28-35.

4. **Tukey, J. W. (1977)**. Exploratory Data Analysis. Addison-Wesley.

### Color Accessibility

5. **Okabe, M., & Ito, K. (2008)**. Color Universal Design (CUD). J*FLY. https://jfly.uni-koeln.de/color/

6. **Tol, P. (2021)**. Colour Schemes. Personal webpage. https://personal.sron.nl/~pault/

### RAG and LLM Evaluation

7. **Es, S., James, J., Espinosa-Anke, L., & Schockaert, S. (2023)**. RAGAS: Automated Evaluation of Retrieval Augmented Generation. arXiv:2309.15217. https://arxiv.org/abs/2309.15217

8. **Chen, J., Lin, H., Han, X., & Sun, L. (2024)**. Benchmarking Large Language Models in Retrieval-Augmented Generation. In Proceedings of AAAI 2024. arXiv:2407.11005. https://arxiv.org/abs/2407.11005

9. **Liu, N. F., Lin, K., Hewitt, J., Paranjape, A., Bevilacqua, M., Petroni, F., & Liang, P. (2023)**. Lost in the Middle: How Language Models Use Long Contexts. arXiv:2307.03172. https://arxiv.org/abs/2307.03172

10. **Gao, Y., Xiong, Y., Gao, X., Jia, K., Pan, J., Bi, Y., ... & Wang, H. (2023)**. Retrieval-Augmented Generation for Large Language Models: A Survey. arXiv:2312.10997. https://arxiv.org/abs/2312.10997

### Ethical AI Standards

11. **IEEE P7000™**. Model Process for Addressing Ethical Concerns During System Design. IEEE Standards Association. https://standards.ieee.org/project/7000.html

12. **IEEE P7001™**. Transparency of Autonomous Systems. IEEE Standards Association.

13. **Jobin, A., Ienca, M., & Vayena, E. (2019)**. The global landscape of AI ethics guidelines. Nature Machine Intelligence, 1(9), 389-399.

14. **Hagendorff, T. (2020)**. The Ethics of AI Ethics: An Evaluation of Guidelines. Minds and Machines, 30(1), 99-120.

### Database Benchmarking Standards

15. **Gray, J. (1993)**. The Benchmark Handbook for Database and Transaction Systems. Morgan Kaufmann.

16. **Cooper, B. F., Silberstein, A., Tam, E., Ramakrishnan, R., & Sears, R. (2010)**. Benchmarking cloud serving systems with YCSB. In Proceedings of the 1st ACM symposium on Cloud computing (pp. 143-154).

17. **IEEE Std 730-2014**. IEEE Standard for Software Quality Assurance Processes.

18. **IEEE Std 1012-2016**. IEEE Standard for System, Software, and Hardware Verification and Validation.

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions welcome! Please ensure:
- Statistical methods are properly referenced
- Color palettes remain color-blind friendly
- No vendor bias introduced
- Tests pass

## Contact

For questions or issues, please open a GitHub issue.

---

**CHIMERA v1.0.0** - *Honest metrics for everyone*
