> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# Vector Compression Benchmark Suite

Umfassende Test-Suite zum Vergleich verschiedener Komprimierungsansätze für Vektoren, einschließlich:
- **Lossless** (verlustfrei): Sparse CSR, Delta+VarInt, Dictionary Encoding
- **Lossy** (verlustbehaftet): Scalar Quantization (SQ8), Product Quantization (PQ)
- **Hardware-Optimierungen**: CPU SIMD (AVX2, AVX-512, NEON), GPU, AI-Accelerators

## Übersicht

Diese Benchmark-Suite testet und vergleicht Komprimierungsmethoden bezüglich:

1. **Compression Ratio** (Kompressionsverhältnis)
2. **Speed** (Encode/Decode-Geschwindigkeit)
3. **Quality** (Qualitätsmetriken bei lossy compression)
4. **Hardware Efficiency** (Nutzung von SIMD, GPU, etc.)
5. **Aufwand** (Implementierungskomplexität, CPU-Overhead)

## Dateien

### Tests
- `tests/test_vector_compression_lossless.cpp` - Unit-Tests für lossless Komprimierung
  - Testet Roundtrip-Korrektheit
  - Validiert Kompressionsraten
  - Prüft Edge-Cases

### Benchmarks
- `benchmarks/bench_vector_compression_lossless.cpp` - Hardware-optimierte Benchmarks
  - SIMD-Optimierungen (AVX2, AVX-512, NEON)
  - Verschiedene Vektortypen (sparse, dense, categorical)
  - Performance-Metriken

- `benchmarks/bench_lossy_vs_lossless.cpp` - Umfassender Vergleich
  - Lossy vs. Lossless side-by-side
  - Qualitätsmetriken (MSE, RMSE, Cosine Similarity)
  - Trade-off-Analyse

## Kompilierung

### Voraussetzungen

```bash
# Ubuntu/Debian
sudo apt-get install cmake g++ libbenchmark-dev libgtest-dev

# macOS
brew install cmake google-benchmark googletest
```

### Build

```bash
cd /home/runner/work/ThemisDB/ThemisDB
mkdir -p build
cd build

# Mit Tests und Benchmarks
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target test_vector_compression_lossless
cmake --build . --target bench_vector_compression_lossless
cmake --build . --target bench_lossy_vs_lossless

# Für AVX2-Optimierungen
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-mavx2"

# Für AVX-512
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-mavx512f"

# Für ARM NEON
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-mfpu=neon"
```

## Ausführung

### Unit-Tests

```bash
# Alle Tests ausführen
./tests/test_vector_compression_lossless

# Mit GTest-Filtern
./tests/test_vector_compression_lossless --gtest_filter="SparseVectorTest.*"
./tests/test_vector_compression_lossless --gtest_filter="VarIntTest.*"
./tests/test_vector_compression_lossless --gtest_filter="DictionaryTest.*"
```

### Benchmarks

#### Lossless Compression Benchmarks

```bash
# Alle Benchmarks ausführen
./benchmarks/bench_vector_compression_lossless

# Nur Sparse CSR
./benchmarks/bench_vector_compression_lossless --benchmark_filter=".*SparseCSR.*"

# Nur Delta+VarInt
./benchmarks/bench_vector_compression_lossless --benchmark_filter=".*DeltaVarInt.*"

# Mit JSON-Output
./benchmarks/bench_vector_compression_lossless --benchmark_format=json > results_lossless.json

# Mit CSV-Output
./benchmarks/bench_vector_compression_lossless --benchmark_format=csv > results_lossless.csv
```

#### Lossy vs Lossless Comparison

```bash
# Umfassender Vergleich mit Qualitätsmetriken
./benchmarks/bench_lossy_vs_lossless

# Nur Benchmarks (ohne ausführliche Vergleichstabelle)
./benchmarks/bench_lossy_vs_lossless --benchmark_filter="BM_.*"

# JSON Export
./benchmarks/bench_lossy_vs_lossless --benchmark_format=json > comparison_results.json
```

### Benchmark-Optionen

Google Benchmark unterstützt viele nützliche Optionen:

```bash
# Wiederholungen für stabilere Ergebnisse
./benchmarks/bench_lossy_vs_lossless --benchmark_repetitions=10

# Minimale Laufzeit pro Benchmark
./benchmarks/bench_lossy_vs_lossless --benchmark_min_time=1.0

# Ausgabeformat
./benchmarks/bench_lossy_vs_lossless --benchmark_format=console  # Standard
./benchmarks/bench_lossy_vs_lossless --benchmark_format=json     # JSON
./benchmarks/bench_lossy_vs_lossless --benchmark_format=csv      # CSV

# Nur bestimmte Benchmarks
./benchmarks/bench_lossy_vs_lossless --benchmark_filter="BM_Lossy_SQ8.*"
./benchmarks/bench_lossy_vs_lossless --benchmark_filter="BM_Lossless_.*"

# Liste verfügbare Benchmarks
./benchmarks/bench_lossy_vs_lossless --benchmark_list_tests
```

## Interpretation der Ergebnisse

### Compression Ratio

**Bedeutung:** `Original Size / Compressed Size`

- **> 10x:** Exzellent (typisch für sparse vectors mit lossless)
- **4-10x:** Sehr gut (SQ8 für dense vectors, Dictionary für categorical)
- **2-4x:** Gut (Delta+VarInt, allgemeine Kompression)
- **1.0-2x:** Marginal (ZSTD auf random floats, nicht empfohlen)
- **< 1.0:** Expansion (sollte nicht vorkommen)

### Encode/Decode Speed

**Einheiten:** Microsekunden (µs) oder MB/s

**Richtwerte für 1000-dim Vektor:**
- **< 10 µs:** Sehr schnell (Scalar operations)
- **10-100 µs:** Schnell (SIMD-optimiert)
- **100-1000 µs:** Akzeptabel (komplexe Algorithmen)
- **> 1000 µs (1ms):** Langsam (nur wenn Kompression sehr hoch)

**Durchsatz-Richtwerte:**
- **> 1 GB/s:** Exzellent (SIMD, cache-friendly)
- **100-1000 MB/s:** Gut (standard implementations)
- **10-100 MB/s:** Akzeptabel (komplexe Kompression)
- **< 10 MB/s:** Langsam (zu vermeiden für hot paths)

### Qualitätsmetriken (Lossy)

#### MAE (Mean Absolute Error)
- **< 0.01:** Exzellent (kaum merklich)
- **0.01-0.1:** Gut (für ML-Embeddings akzeptabel)
- **0.1-1.0:** Akzeptabel (je nach Anwendung)
- **> 1.0:** Bedenklich (hoher Informationsverlust)

#### Cosine Similarity
- **> 0.99:** Exzellent (fast identisch)
- **0.95-0.99:** Sehr gut (SQ8-Niveau)
- **0.90-0.95:** Gut (PQ-Niveau)
- **< 0.90:** Bedenklich (signifikanter Informationsverlust)

#### PSNR (Peak Signal-to-Noise Ratio)
- **> 40 dB:** Exzellent
- **30-40 dB:** Sehr gut
- **20-30 dB:** Gut
- **< 20 dB:** Schlecht

### Hardware-Optimierungen

**SIMD Speedup (vs. Scalar):**
- **AVX2 (8x floats):** 2-4x schneller
- **AVX-512 (16x floats):** 4-8x schneller
- **NEON (4x floats):** 2-3x schneller

**Erwartete Speedups:**
- Sparse detection: 3-5x mit AVX2
- Distance calculations: 2-4x mit SIMD
- Integer operations: 4-8x mit SIMD

## Beispiel-Output

### Unit-Tests

```
[==========] Running 15 tests from 4 test suites.
[----------] 7 tests from SparseVectorTest
[ RUN      ] SparseVectorTest.RoundtripSparseVector
Sparse CSR Compression Ratio: 87.3x
Original: 40000 bytes, Compressed: 458 bytes
[       OK ] SparseVectorTest.RoundtripSparseVector (2 ms)

[----------] 5 tests from VarIntTest
[ RUN      ] VarIntTest.DeltaCompressionMonotonic
Delta+VarInt Compression Ratio (monotonic): 6.8x
[       OK ] VarIntTest.DeltaCompressionMonotonic (1 ms)

[----------] 3 tests from DictionaryTest
[ RUN      ] DictionaryTest.CompressionRatioCategorical
Dictionary Compression Ratio (categorical): 12.4x
Dictionary size: 10 unique values
[       OK ] DictionaryTest.CompressionRatioCategorical (3 ms)
```

### Benchmarks

```
=== Hardware Capabilities ===
SIMD Level: AVX2
CPU Cores: 8
L1 Cache: 32 KB
L2 Cache: 256 KB
L3 Cache: 8192 KB
=============================

---------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations
---------------------------------------------------------------------------
BM_SparseCSR_Encode_Scalar/0/1000      12.3 µs         12.2 µs        57143
  CompressionRatio              87.32
  CompressedBytes                 458

BM_SparseCSR_Encode_AVX2/0/1000         4.1 µs          4.1 µs       170731
  CompressionRatio              87.32
  CompressedBytes                 458

BM_DeltaVarInt_Encode/2/1000            8.7 µs          8.7 µs        80460
  CompressionRatio               6.84
  CompressedBytes                 585
```

### Lossy vs Lossless Comparison

```
╔════════════════════════════════════════════════════════════════╗
║     COMPREHENSIVE LOSSY vs LOSSLESS COMPARISON               ║
╚════════════════════════════════════════════════════════════════╝

### Vector Type: DENSE_EMBEDDING (dim=768) ###

========================================
Method: Scalar Quantization (SQ8) (LOSSY)
Vector Type: DENSE_EMBEDDING
Dimension: 768
----------------------------------------
Compression:
  Original: 3072 bytes
  Compressed: 776 bytes
  Ratio: 3.96x
Performance:
  Encode: 12.34 µs
  Decode: 8.76 µs
Quality:
  MSE: 0.00234
  RMSE: 0.0484
  MAE: 0.0387
  Max Error: 0.156
  PSNR: 44.32 dB
  Cosine Similarity: 0.998234
  L2 Distance: 1.342
========================================

### Vector Type: SPARSE_TFIDF (dim=10000) ###

========================================
Method: Sparse CSR (Lossless) (LOSSLESS)
Vector Type: SPARSE_TFIDF
Dimension: 10000
----------------------------------------
Compression:
  Original: 40000 bytes
  Compressed: 458 bytes
  Ratio: 87.34x
Performance:
  Encode: 23.45 µs
  Decode: 15.67 µs
Quality:
  MSE: 0.00000
  RMSE: 0.00000
  MAE: 0.00000
  Max Error: 0.00000
  PSNR: inf dB
  Cosine Similarity: 1.000000
  L2 Distance: 0.000
========================================

╔════════════════════════════════════════════════════════════════╗
║                    SUMMARY COMPARISON                         ║
╚════════════════════════════════════════════════════════════════╝

                        Method        Ratio      Enc(µs)      Dec(µs)       MAE   CosineSim
----------------------------------------------------------------------------------------
SQ8 (DENSE_EMBEDDING)                3.96         12.3          8.8    3.87e-02    0.998234
PQ (DENSE_EMBEDDING)                 8.12         45.6         32.1    8.23e-02    0.965432
Sparse CSR (SPARSE_TFIDF)           87.34         23.5         15.7    0.00e+00    1.000000
Dictionary (CATEGORICAL)            12.43         18.9         12.3    0.00e+00    1.000000
```

## Empfehlungen basierend auf Ergebnissen

### Wann Lossless verwenden?

✅ **Sparse Vektoren** (>95% Nullen)
- Methode: Sparse CSR
- Erwartete Ratio: 50-100x
- Use Case: TF-IDF, One-Hot Encodings

✅ **Kategoriale Vektoren** (wenige unique values)
- Methode: Dictionary Encoding
- Erwartete Ratio: 5-20x
- Use Case: Embedding-Indizes, Feature-Categories

✅ **Integer-Features** (Histogramme)
- Methode: Delta + VarInt
- Erwartete Ratio: 3-10x
- Use Case: Histogramme, Zählfeatures

✅ **Compliance-Anforderungen**
- Methode: Beliebige Lossless-Methode
- Grund: Bit-exakte Rekonstruktion erforderlich

### Wann Lossy verwenden?

✅ **Dense ML-Embeddings** (uniform verteilt)
- Methode: Scalar Quantization (SQ8)
- Erwartete Ratio: 4x
- Qualität: 97-99% Cosine Similarity

✅ **Sehr große Vektormengen** (>10M Vektoren)
- Methode: Product Quantization (PQ)
- Erwartete Ratio: 8-32x
- Qualität: 90-95% Recall@10

✅ **Speicher-kritische Anwendungen**
- Methode: SQ8 oder PQ
- Grund: Bessere Kompression als Lossless bei dense data

### Hardware-Optimierungen nutzen

✅ **x86-64 CPU mit AVX2**
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma")
```
- Speedup: 3-5x für Sparse detection
- Lohnt sich ab: 1000+ dim Vektoren

✅ **ARM CPU mit NEON**
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
```
- Speedup: 2-3x
- Wichtig für: Mobile/Edge devices

✅ **GPU für Batch-Processing**
- Sinnvoll ab: 10000+ Vektoren gleichzeitig
- Use Case: Batch-Compression bei Ingestion

## Performance-Tuning

### Für maximale Geschwindigkeit

```cpp
// 1. Nutze SIMD-Optimierungen
#ifdef __AVX2__
    auto compressed = SparseVectorCodec::compress_avx2(vec);
#else
    auto compressed = SparseVectorCodec::compress_scalar(vec);
#endif

// 2. Pre-Allocation
result.values.reserve(estimated_non_zero_count);
result.indices.reserve(estimated_non_zero_count);

// 3. Batch-Processing
for (auto& vec : batch) {
    // Process multiple vectors at once
}
```

### Für maximale Kompression

```cpp
// 1. Hybride Ansätze
auto sparse = SparseCSR::compress(vec);
auto sparse_indices_compressed = VarInt::compress(sparse.indices);
auto sparse_values_compressed = ZSTD::compress(sparse.values);

// 2. Adaptive Schwellwerte
float sparsity = compute_sparsity(vec);
float epsilon = sparsity > 0.99 ? 1e-9 : 1e-6;

// 3. Dictionary vor RLE
auto dict = Dictionary::compress(vec);
auto dict_rle = RLE::compress(dict.indices);
```

## Troubleshooting

### Problem: Benchmarks laufen sehr langsam

**Lösung:**
```bash
# Release-Build verwenden
cmake .. -DCMAKE_BUILD_TYPE=Release

# Kürzere Benchmark-Zeit
./benchmark --benchmark_min_time=0.1
```

### Problem: Kompressionsrate niedriger als erwartet

**Diagnose:**
```cpp
// Vektorcharakteristiken analysieren
float sparsity = compute_sparsity(vec);
size_t unique_values = count_unique(vec);
bool mostly_integer = check_integer_pattern(vec);

std::cout << "Sparsity: " << sparsity << "\n";
std::cout << "Unique values: " << unique_values << "\n";
std::cout << "Integer pattern: " << mostly_integer << "\n";
```

### Problem: SIMD-Optimierungen nicht verfügbar

**Lösung:**
```bash
# Prüfe Compiler-Flags
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON

# Für AVX2
cmake .. -DCMAKE_CXX_FLAGS="-mavx2"

# Runtime-Check
if (__builtin_cpu_supports("avx2")) {
    // Use AVX2 path
}
```

## Integration in ThemisDB

Die Benchmark-Ergebnisse sollten in die Komprimierungsstrategie einfließen:

```cpp
// In VectorIndexManager::addEntity()
CompressionMethod method = selectOptimalMethod(entity.embedding);

switch (method) {
    case CompressionMethod::SPARSE_CSR:
        // Best for sparse vectors (>95% zeros)
        compressed = SparseCSR::compress(entity.embedding);
        break;
        
    case CompressionMethod::SCALAR_QUANT:
        // Best for dense ML embeddings
        compressed = SQ8::compress(entity.embedding);
        break;
        
    case CompressionMethod::DICTIONARY:
        // Best for categorical features
        compressed = Dictionary::compress(entity.embedding);
        break;
        
    default:
        // Fallback to ZSTD or no compression
        break;
}
```

## Weiterführende Informationen

- **Dokumentation:** `docs/performance/performance_vector_compression_lossless.md`
- **Komprimierungsstrategie:** `docs/performance/performance_compression_strategy.md`
- **Benchmarks:** `docs/performance/performance_compression_benchmarks.md`

## License

Teil des ThemisDB Projekts. Siehe `LICENSE` im Repository-Root.
