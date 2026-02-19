# CHIMERA Suite User Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Quick Start](#quick-start)
4. [Database Adapter Usage](#database-adapter-usage)
5. [RAG/LLM Evaluation](#ragllm-evaluation)
6. [Ethics Evaluation](#ethics-evaluation)
7. [Report Generation](#report-generation)
8. [Advanced Usage](#advanced-usage)
9. [Troubleshooting](#troubleshooting)

## Introduction

The CHIMERA Suite is a comprehensive, vendor-neutral benchmark framework for evaluating multi-model databases with AI/LLM capabilities. This guide will help you:

- Set up and run benchmarks
- Evaluate database operations (relational, vector, graph, hybrid)
- Assess RAG pipelines and LLM quality
- Check ethical AI compliance
- Generate professional benchmark reports

## Installation

### Prerequisites

- Python 3.8 or higher
- pip package manager

### Install Dependencies

```bash
cd benchmarks/chimera
pip install -r requirements.txt
```

### Verify Installation

```bash
pytest test_chimera.py -v
```

If all tests pass, you're ready to go!

## Quick Start

### 1. Basic Statistical Report

```python
from chimera import ChimeraReporter

# Create reporter
reporter = ChimeraReporter(significance_level=0.05)

# Add benchmark results
reporter.add_system_results(
    system_name="Database A",
    metric_name="Query Throughput",
    metric_unit="queries/sec",
    data=[15000, 15200, 14800, 15100, 15300]
)

reporter.add_system_results(
    system_name="Database B",
    metric_name="Query Throughput",
    metric_unit="queries/sec",
    data=[12500, 12700, 12300, 12600, 12800]
)

# Generate reports
reporter.generate_html_report("benchmark_report.html")
reporter.generate_csv_report("results.csv")
```

### 2. Run Demo

```bash
cd benchmarks/chimera
python demo.py
```

Check `demo_reports/` directory for generated reports.

## Database Adapter Usage

### Relational Operations

```python
from test_database_adapters import MockDatabaseAdapter, RelationalRow

# Create adapter
adapter = MockDatabaseAdapter()
adapter.connect("database://localhost")

# Insert single row
row = RelationalRow(columns={"id": 1, "name": "Alice", "age": 30})
result = adapter.insert_row("users", row)

if result.success:
    print(f"Inserted {result.value} row(s)")

# Batch insert
rows = [
    RelationalRow(columns={"id": i, "name": f"User{i}", "age": 20 + i})
    for i in range(100)
]
result = adapter.batch_insert("users", rows)

# Execute query
result = adapter.execute_query(
    "SELECT * FROM users WHERE age > ?",
    params=[25]
)
```

### Vector Search Operations

```python
from test_database_adapters import Vector

# Create vector index
adapter.create_index("embeddings", dimensions=128)

# Insert vectors
vectors = []
for i in range(1000):
    vec = Vector(
        data=[float(j) for j in range(128)],
        metadata={"category": f"cat_{i % 10}", "index": i}
    )
    vectors.append(vec)

result = adapter.batch_insert_vectors("embeddings", vectors)

# Search for similar vectors
query = Vector(data=[1.0] * 128)
results = adapter.search_vectors(
    collection="embeddings",
    query_vector=query,
    k=10,
    filters={"category": "cat_1"},
    metric="cosine"
)

for vec, similarity in results.value:
    print(f"Similarity: {similarity:.3f}, Metadata: {vec.metadata}")
```

### Graph Operations

```python
from test_database_adapters import GraphNode, GraphEdge

# Create nodes
node1 = GraphNode(
    id="person1",
    label="Person",
    properties={"name": "Alice", "age": 30}
)
node2 = GraphNode(
    id="person2",
    label="Person",
    properties={"name": "Bob", "age": 35}
)

adapter.insert_node(node1)
adapter.insert_node(node2)

# Create edge
edge = GraphEdge(
    id="knows1",
    source_id="person1",
    target_id="person2",
    label="KNOWS",
    properties={"since": 2020},
    weight=1.0
)
adapter.insert_edge(edge)

# Find shortest path
path_result = adapter.shortest_path("person1", "person2", max_depth=5)
if path_result.success:
    path = path_result.value
    print(f"Path length: {len(path.nodes)}")
    print(f"Total weight: {path.total_weight}")

# Traverse graph
nodes = adapter.traverse("person1", max_depth=3, edge_labels=["KNOWS"])
```

### Hybrid Workloads

```python
# Transaction spanning multiple models
txn_result = adapter.begin_transaction()
txn_id = txn_result.value

# Insert relational data
adapter.insert_row("users", RelationalRow(columns={"id": 1, "name": "Alice"}))

# Insert vector embedding
adapter.insert_vector("embeddings", Vector(data=[0.1, 0.2, 0.3]))

# Insert graph node
adapter.insert_node(GraphNode(id="user1", label="User", properties={"ref": 1}))

# Commit transaction
adapter.commit_transaction(txn_id)
```

## RAG/LLM Evaluation

### Basic RAG Pipeline

```python
from test_llm_rag_ethics import (
    MockRetriever, MockGenerator, RAGPipeline, RAGRequest
)

# Setup components
retriever = MockRetriever()
retriever.add_document(
    doc_id="doc1",
    content="Python is a high-level programming language",
    metadata={"topic": "programming"}
)
retriever.add_document(
    doc_id="doc2",
    content="Machine learning uses Python extensively",
    metadata={"topic": "ML"}
)

generator = MockGenerator(include_citations=True)
pipeline = RAGPipeline(retriever, generator)

# Process query
response = pipeline.process("What is Python?", top_k=3)

print(f"Query: {response.query}")
print(f"Generated: {response.generated_text}")
print(f"Citations: {response.citations}")
```

### LLM-as-Judge Evaluation

```python
from test_llm_rag_ethics import LLMJudge, JudgmentMode

judge = LLMJudge(mode=JudgmentMode.THOROUGH)

# Evaluate faithfulness
faithfulness = judge.evaluate_faithfulness(response)
print(f"Faithfulness score: {faithfulness['score']:.2f}")
print(f"Verdict: {faithfulness['verdict']}")

# Detect hallucination
hallucination = judge.detect_hallucination(response)
if hallucination['hallucination_detected']:
    print(f"⚠️  Hallucination detected! Confidence: {hallucination['confidence']:.2f}")

# Evaluate citation quality
citation_quality = judge.evaluate_citation_quality(response)
print(f"Citation score: {citation_quality['score']:.2f}")
if citation_quality['missing_citations']:
    print(f"Missing citations: {citation_quality['missing_citations']}")
```

### RAG Metrics (RAGBench/RAGAS-style)

```python
from test_llm_rag_ethics import RAGMetrics

# Calculate metrics
context_precision = RAGMetrics.context_precision(response)
answer_faithfulness = RAGMetrics.answer_faithfulness(response)
answer_relevance = RAGMetrics.answer_relevance(response)
toxicity = RAGMetrics.toxicity_score(response.generated_text)

print(f"Context Precision: {context_precision:.2f}")
print(f"Answer Faithfulness: {answer_faithfulness:.2f}")
print(f"Answer Relevance: {answer_relevance:.2f}")
print(f"Toxicity Score: {toxicity:.2f}")
```

## Ethics Evaluation

### Comprehensive Ethics Check

```python
from test_llm_rag_ethics import EthicsEvaluator

evaluator = EthicsEvaluator()

system_config = {
    "user_override": True,
    "stop_generation_enabled": True,
    "reject_response_enabled": True,
    "alternative_response_enabled": True,
    "undo_enabled": True
}

# Run comprehensive evaluation
ethics_result = evaluator.comprehensive_evaluation(response, system_config)

print(f"Overall Ethics Score: {ethics_result['overall_score']:.2f}")
print(f"Passed: {ethics_result['passed']}")

# Review category scores
for category, result in ethics_result['category_scores'].items():
    print(f"  {category}: {result['score']:.2f}")

# Check violations
if ethics_result['violations']:
    print(f"\n⚠️  {len(ethics_result['violations'])} violations found:")
    for violation in ethics_result['violations']:
        print(f"  [{violation.severity}] {violation.violation_type}: {violation.description}")
```

### Individual Ethics Checks

```python
# Check autonomy
autonomy = evaluator.evaluate_autonomy(response, user_override_available=True)

# Check bias
bias = evaluator.evaluate_bias(response)

# Check citation ethics
citation = evaluator.evaluate_citation_ethics(response)

# Check VETO capability
veto = evaluator.evaluate_veto_capability(system_config)
```

## Report Generation

### HTML Report with Visualizations

```python
reporter = ChimeraReporter()

# Add multiple systems
for system_name, data in benchmark_results.items():
    reporter.add_system_results(
        system_name=system_name,
        metric_name="Latency",
        metric_unit="milliseconds",
        data=data
    )

# Generate HTML with plots
reporter.generate_html_report(
    "comprehensive_report.html",
    sort_by='alphabetical',  # or 'metric'
    include_plots=True
)
```

### CSV Export for Analysis

```python
# Export raw statistics to CSV
reporter.generate_csv_report("statistics.csv")

# Can be imported into Excel, R, Python pandas, etc.
import pandas as pd
df = pd.read_csv("statistics.csv")
```

### Custom Styling

```python
from chimera import ColorBlindPalette

# Use different palette
palette = ColorBlindPalette.get_palette('tol_bright')
colors = ColorBlindPalette.get_sequential_palette(10, 'tol_muted')
```

## Advanced Usage

### Custom Statistical Analysis

```python
from chimera import StatisticalAnalyzer

analyzer = StatisticalAnalyzer(significance_level=0.01)

# Descriptive statistics with outlier removal
stats = analyzer.descriptive_statistics(data, remove_outliers=True)
print(f"Mean: {stats.mean:.2f}")
print(f"Median: {stats.median:.2f}")
print(f"95th percentile: {stats.p95:.2f}")
print(f"Outliers removed: {stats.outliers_removed}")

# Compare two systems
result = analyzer.t_test(data_a, data_b)
print(f"t-statistic: {result.statistic:.3f}")
print(f"p-value: {result.p_value:.4f}")
print(f"Significant: {result.is_significant}")
print(f"Effect size (Cohen's d): {result.effect_size:.3f}")

# Non-parametric test
result = analyzer.mann_whitney_u(data_a, data_b)
```

### Custom Citations

```python
from chimera import CitationManager
from chimera.citations import Citation

manager = CitationManager()

# Add custom citation
custom_citation = Citation(
    citation_id="smith2024",
    authors="Smith, J., & Jones, A.",
    title="Novel Database Benchmarking Techniques",
    publication="Journal of Database Systems",
    year=2024,
    url="https://example.com/paper"
)
manager.add_citation(custom_citation)

# Generate bibliography
bib = manager.format_bibliography(['cohen1988', 'smith2024'])
```

### Neutrality Verification

```bash
# Run neutrality linter
cd benchmarks/chimera
python neutrality_linter.py

# Check specific file
python neutrality_linter.py --file my_benchmark.py

# Generate neutrality report
python neutrality_linter.py --report neutrality_check.txt
```

## Troubleshooting

### Test Failures

If tests fail:

```bash
# Run with verbose output
pytest test_chimera.py -vv

# Run specific test
pytest test_chimera.py::TestStatisticalAnalyzer::test_t_test -v

# Show full traceback
pytest test_chimera.py --tb=long
```

### Import Errors

```python
# Verify package structure
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))

from chimera import ChimeraReporter
```

### Missing Dependencies

```bash
# Reinstall all dependencies
pip install -r requirements.txt --force-reinstall

# Install specific package
pip install numpy scipy matplotlib pytest
```

### Performance Issues

For large datasets:

```python
# Disable outlier removal for speed
stats = analyzer.descriptive_statistics(data, remove_outliers=False)

# Use sampling for very large datasets
import random
sample_data = random.sample(large_dataset, 10000)
```

### Visualization Issues

```python
# Generate without plots
reporter.generate_html_report("report.html", include_plots=False)

# Install matplotlib if missing
pip install matplotlib
```

## Best Practices

### 1. Sample Size
- Use at least 30 samples per system for statistical validity
- More samples = better statistical power

### 2. Outlier Handling
- Enable outlier removal for production benchmarks
- Document outlier removal in reports

### 3. Vendor Neutrality
- Always run neutrality linter before publishing
- Sort systems alphabetically by default
- Avoid marketing terms in reports

### 4. Statistical Reporting
- Always report confidence intervals
- Include effect sizes, not just p-values
- Warn about multiple comparison issues

### 5. Ethics Evaluation
- Run comprehensive ethics checks for LLM systems
- Document all VETO capabilities
- Check for bias in all generated content

## Examples

See the `demo.py` and `demo_multi_vendor.py` files for complete working examples.

## Support

For issues, questions, or contributions:
- Open a GitHub issue
- Check existing documentation in `docs/chimera/`
- Review test files for usage examples

## References

See README.md for complete list of scientific references and standards.
