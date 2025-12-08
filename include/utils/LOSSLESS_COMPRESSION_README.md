# EXPERIMENTAL: Lossless Vector Compression

**⚠️ WARNING: This is a scientific experiment and may be rolled back.**

## Overview

This directory contains experimental lossless compression methods for vector storage, as documented in:
- `docs/performance/performance_vector_compression_lossless.md`

These methods are **alternatives** to the existing SQ8 (lossy) quantization and provide 100% lossless compression for specific vector types.

## Status

- **Implementation**: ✅ Complete
- **Testing**: 🧪 Experimental (unit tests available in `tests/test_vector_compression_lossless.cpp`)
- **Production Ready**: ❌ No (scientific experiment only)
- **May be rolled back**: ⚠️ Yes

## Compression Methods

| Method | Best For | Compression Ratio | Quality | Use Case |
|--------|----------|-------------------|---------|----------|
| **Sparse CSR** | Sparse vectors (>95% zeros) | 10-100x | 100% lossless | TF-IDF, one-hot encodings |
| **Delta+VarInt** | Integer features | 3-10x | 100% lossless | Histograms, count features |
| **Dictionary** | Categorical features | 5-20x | 100% lossless | Feature categories |

## Configuration

Lossless compression is **disabled by default**. To enable it, set the following configuration in the database:

### Enable Lossless Compression

```bash
# Via HTTP API (assuming ThemisDB is running on port 8765)
curl -X PUT http://localhost:8765/config/vector_compression_lossless \
  -H "Content-Type: application/json" \
  -d '{
    "enabled": true,
    "mode": "auto",
    "sparse_threshold": 0.95,
    "fallback_to_sq8": true
  }'
```

### Configuration Options

```json
{
  "enabled": false,              // Enable lossless compression (default: false)
  "mode": "auto",                // "auto", "sparse_csr", "delta_varint", "dictionary", "none"
  "sparse_threshold": 0.95,      // Auto-select CSR if >=95% zeros
  "fallback_to_sq8": true        // Use SQ8 if lossless not applicable
}
```

### Mode Options

- **`auto`** (recommended): Automatically selects the best lossless method based on vector characteristics
  - Uses Sparse CSR for vectors with >=95% zeros
  - Uses Delta+VarInt for vectors with >=90% integer values
  - Uses Dictionary for vectors with <10% unique values
  - Falls back to SQ8 or raw storage otherwise

- **`sparse_csr`**: Force Sparse CSR compression (only works well for sparse vectors)
- **`delta_varint`**: Force Delta+VarInt compression (only works well for integer features)
- **`dictionary`**: Force Dictionary compression (only works well for categorical features)
- **`none`**: Disable lossless compression (use existing SQ8 or raw storage)

## Priority and Fallback

The compression priority is:

1. **Lossless** (if enabled and applicable)
2. **SQ8 (lossy)** (existing implementation, if `fallback_to_sq8=true`)
3. **Raw storage** (no compression)

**Important**: The existing SQ8 implementation is **preserved** (not deleted, only commented as alternative). If lossless compression is disabled or not applicable, the system automatically falls back to the existing SQ8 behavior.

## Usage Example

### Auto Mode (Recommended)

```json
// Enable auto mode - system will choose the best method
{
  "enabled": true,
  "mode": "auto"
}
```

For a sparse TF-IDF vector (98% zeros):
- System automatically selects: **Sparse CSR**
- Compression ratio: ~50-100x
- Quality: 100% lossless

For a dense ML embedding:
- System selects: **None** (not suitable for lossless)
- Falls back to: **SQ8** (existing implementation)
- Compression ratio: ~4x
- Quality: ~98% cosine similarity

### Manual Mode (Advanced)

```json
// Force Sparse CSR for all vectors
{
  "enabled": true,
  "mode": "sparse_csr"
}
```

**Warning**: Only use manual mode if you know your vector characteristics. Incorrect method selection can lead to poor compression or even expansion.

## Verification

To verify that lossless compression is working:

```bash
# Check logs for messages like:
# "VectorIndexManager: Using experimental lossless compression for pk=..."
# "Lossless CSR compression: 40000 -> 458 bytes (87.3x)"

# Or query a vector and check the storage size
curl http://localhost:8765/vector/stats
```

## Rollback Plan

If issues arise, lossless compression can be immediately disabled:

```bash
# Disable lossless compression
curl -X PUT http://localhost:8765/config/vector_compression_lossless \
  -H "Content-Type: application/json" \
  -d '{
    "enabled": false
  }'
```

**After disabling**:
- New vectors will use the existing SQ8 or raw storage (as before)
- Existing lossless-compressed vectors will still be decompressed correctly on read
- No data loss occurs

**Complete rollback** (if needed):
The implementation is designed to be easily removed:
1. Remove the `#include "utils/lossless_vector_integration.h"` line from `src/index/vector_index.cpp`
2. Remove the experimental code blocks (clearly marked with `// EXPERIMENTAL` comments)
3. The existing SQ8 implementation remains intact (preserved, not deleted)

## Testing

Run the unit tests:

```bash
cd build
./tests/test_vector_compression_lossless
```

Run the benchmarks (to compare with SQ8):

```bash
./benchmarks/bench_lossy_vs_lossless
```

## Performance Considerations

**Pros**:
- 100% lossless (bit-exact reconstruction)
- High compression ratios for suitable vector types (10-100x for sparse)
- No quality loss (unlike SQ8)

**Cons**:
- Only effective for specific vector types (sparse, integer, categorical)
- Slightly higher CPU overhead for compression/decompression vs. SQ8
- Not suitable for dense random embeddings (use SQ8 instead)

**Recommendation**: Use `mode="auto"` to automatically select the best method for each vector.

## Support

For questions or issues:
- See research documentation: `docs/performance/performance_vector_compression_lossless.md`
- Check benchmark results: `benchmarks/bench_lossy_vs_lossless.cpp`
- File an issue: https://github.com/makr-code/ThemisDB/issues

## Future Work

If this experiment proves successful:
- Production-grade implementation
- Additional compression methods (e.g., FPC for scientific data)
- Hardware-accelerated compression (SIMD, GPU)
- Compression statistics and monitoring

If rollback is needed:
- Remove experimental code
- Retain SQ8 as primary compression method
- Document lessons learned
