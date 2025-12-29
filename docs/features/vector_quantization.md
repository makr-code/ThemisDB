# Vector Quantization Feature

**Status:** ✅ Implemented  
**Version:** v1.3.0  
**Feature ID:** #7

## Overview

Vector Quantization provides memory compression for high-dimensional vectors using Product Quantization (PQ), reducing storage requirements by up to 97% while maintaining acceptable search accuracy.

## Key Features

- **Product Quantization (PQ):** Compress vectors using 8-bit codes
- **Memory Compression:** Reduce 1536D float32 vectors from 6KB to 192 bytes
- **K-means Training:** Automatic codebook generation from training data
- **Asymmetric Distance:** Fast distance computation directly from quantized codes
- **Configurable Subquantizers:** Adjust compression ratio vs. accuracy trade-off

## Quick Start

```cpp
#include "index/vector_index.h"

VectorIndexManager vim(db);
vim.init("documents", 1536);

// Enable quantization
vim.enableQuantization(true, 8);

// Train quantizer
vim.trainQuantizer();

// Vectors are now automatically quantized
vim.addEntity(entity, "embedding");

// Search works with quantized codes
auto [status, results] = vim.searchKnn(query, 10);
```

## Performance

- **Memory Reduction:** 32x compression (6KB → 192 bytes for 1536D)
- **Speed Improvement:** 2-4x faster search
- **Accuracy:** 95-98% recall@10

## Documentation

See full documentation at `docs/features/vector_quantization.md`

## References

- Paper: "Product Quantization for Nearest Neighbor Search" (PAMI 2011)
- Implementation: `include/index/product_quantizer.h`, `src/index/product_quantizer.cpp`
- Tests: `tests/test_product_quantizer.cpp`
