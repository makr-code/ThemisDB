# CHIMERA Suite

**C**omprehensive **H**ybrid **I**nferencing & **M**ulti-model **E**valuation **R**esource **A**ssessment

_"Benchmark the Unbenchmarkable"_

> The CHIMERA Suite is a scientifically rigorous, vendor-neutral benchmark framework for multi-model databases with native AI/LLM integration. Like the mythical Chimera - a hybrid creature - this suite evaluates the diverse capabilities of modern database systems: Graph, Vector, Relational, Document models, combined with LLM/LoRA inferencing capabilities.

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

Run the test suite:

```bash
cd benchmarks/chimera
pytest test_chimera.py -v
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

1. **Cohen, J. (1988)**. Statistical Power Analysis for the Behavioral Sciences. Lawrence Erlbaum Associates.

2. **Mann, H. B., & Whitney, D. R. (1947)**. On a test of whether one of two random variables is stochastically larger than the other. The Annals of Mathematical Statistics, 18(1), 50-60.

3. **Welch, B. L. (1947)**. The generalization of 'Student's' problem when several different population variances are involved. Biometrika, 34(1-2), 28-35.

4. **Okabe, M., & Ito, K. (2008)**. Color Universal Design (CUD). J*FLY. https://jfly.uni-koeln.de/color/

5. **Tol, P. (2021)**. Colour Schemes. Personal webpage. https://personal.sron.nl/~pault/

6. **Tukey, J. W. (1977)**. Exploratory Data Analysis. Addison-Wesley.

7. **IEEE Std 730-2014**. IEEE Standard for Software Quality Assurance Processes.

8. **IEEE Std 1012-2016**. IEEE Standard for System, Software, and Hardware Verification and Validation.

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
