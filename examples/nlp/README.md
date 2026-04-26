> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# NLP Text Analyzer Examples

This directory contains examples demonstrating the ThemisDB NLP Text Analyzer capabilities introduced in PR #317.

## Overview

The NLP Text Analyzer is a lightweight, CPU-efficient NLP component for ThemisDB that provides text analysis features optimized for AQL query optimization and execution plan orchestration.

### Key Features

- ✅ **Lightweight** - No heavy ML frameworks required
- ✅ **CPU-Only** - Works without GPU
- ✅ **Fast** - Millisecond latency for typical queries
- ✅ **Thread-Safe** - Parallelizable for multi-query scenarios
- ✅ **AQL-Optimized** - Specifically designed for database query analysis

## Prerequisites

- ThemisDB server running (v1.5.0+)
- Python 3.8+ or C++ compiler (GCC 11+, Clang 15+, MSVC 19.30+)
- ThemisDB client library (for Python examples)

## Quick Start

### Using the C++ API

```cpp
#include "analytics/nlp_text_analyzer.h"

using namespace themis::analytics;

// Initialize analyzer
NlpTextAnalyzer::Config config;
config.enable_stemming = true;
config.enable_stopwords = true;
config.max_keywords = 10;

NlpTextAnalyzer analyzer(config);

// Detect language
auto lang = analyzer.detectLanguage("Der schnelle braune Fuchs...");
// Returns: Language::GERMAN

// Extract keywords
auto keywords = analyzer.extractKeywords("Database query optimization...");
// Returns: ["database", "query", "optimization"]

// Analyze query complexity
auto complexity = analyzer.analyzeQueryComplexity(aql_query);
```

### Using with ThemisDB Queries

```cpp
// In query optimizer
auto analyzer = std::make_unique<NlpTextAnalyzer>();
auto analysis = analyzer->analyzeAqlQuery(query_text);

if (analysis.complexity > 0.8) {
    // Use advanced optimization strategies
    optimizer->enableAdvancedOptimizations();
}
```

## Examples

Currently, this directory serves as a placeholder for future Python and C++ examples. The NLP analyzer is integrated into ThemisDB core and used automatically by:

1. **AQL Query Engine** - Query complexity estimation
2. **Query Optimizer** - Semantic hints for optimization
3. **Index Selection** - Suggesting appropriate indexes
4. **Query Orchestration** - Planning multi-step queries

## Supported Languages

The analyzer supports the following languages:

- 🇩🇪 German (de)
- 🇬🇧 English (en)
- 🇫🇷 French (fr)
- 🇪🇸 Spanish (es)
- 🇮🇹 Italian (it)
- 🇳🇱 Dutch (nl)

## API Functions

| Function | Description | Use Case |
|----------|-------------|----------|
| `detectLanguage(text)` | Identifies text language | Multi-language query support |
| `extractKeywords(text, max)` | Extracts key terms | Index selection hints |
| `analyzeQueryComplexity(query)` | Estimates query cost | Optimization decisions |
| `tokenize(text)` | Splits text into tokens | Query parsing |
| `removeStemming(text)` | Normalizes word forms | Semantic matching |

## Full Documentation

For comprehensive documentation including architecture, API reference, performance metrics, and implementation details, see:

📖 **[NLP Text Analyzer Documentation](../../docs/de/analytics/NLP_TEXT_ANALYZER.md)**

## Contributing

To add examples to this directory:

1. Create example files (Python or C++ preferred)
2. Add documentation explaining the use case
3. Include sample queries and expected outputs
4. Submit PR referencing the NLP feature

## Status

- **Version:** 1.0
- **Status:** ✅ Production Ready
- **PR Reference:** #317
- **Examples:** 📝 Coming soon

## Related Examples

- [Query Optimization Examples](../07_vector_search_documents/) - Vector search with NLP
- [Analytics Examples](../../docs/de/analytics/) - Advanced analytics features
- [AQL Documentation](../../docs/de/aql/) - AQL query language guide
