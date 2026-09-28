# ONNX_CLIP DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\onnx_clip\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\onnx_clip\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 8
- Compounds: 49
- Classes/Structs: 25
- Namespaces: 10
- File Compounds: 8

## Namespaces
- @132233123313131330042314110353207172004137113006
- benchmark
- std::chrono_literals
- testing
- themis
- themis::bench
- themis::bench::onnx_clip
- themis::plugins
- themis::plugins::image
- themis::plugins::image::@335235263030255166126376240242232047045052151205

## Types
### Classes
- MockOnnxClipBackendModel
- OnnxClipBackendThroughputFixture
- OnnxClipBatchSplittingFixture
- OnnxClipCpuFixture
- OnnxClipCpuWarmFixture
- OnnxClipGoldenEmbeddingsTest
- OnnxClipHotSwapTest
- OnnxClipMemoryFixture
- OnnxClipMemoryScalingFixture
- OnnxClipMmapTest
- OnnxClipThroughputScalingFixture
- themis::bench::onnx_clip::MockOnnxClipBackendModel
- themis::bench::onnx_clip::MockOnnxClipModel
- themis::bench::onnx_clip::OnnxClipBackendThroughputFixture
- themis::bench::onnx_clip::OnnxClipBatchSplittingFixture
- themis::bench::onnx_clip::OnnxClipCpuFixture
- themis::bench::onnx_clip::OnnxClipCpuWarmFixture
- themis::bench::onnx_clip::OnnxClipInitializationProfiler
- themis::bench::onnx_clip::OnnxClipLatencyRegressionFixture
- themis::bench::onnx_clip::OnnxClipMemoryFixture
- themis::bench::onnx_clip::OnnxClipMemoryScalingFixture
- themis::bench::onnx_clip::OnnxClipThroughputScalingFixture
- themis::plugins::image::ONNXClipPlugin
- themis::plugins::image::RequestGuard

### Structs
- themis::plugins::image::ONNXClipPlugin::Impl

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 146

### MockOnnxClipBackendModel

#### `MockOnnxClipBackendModel()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:91
- Brief: n/a
- Parameters: none

#### `Backend backend() const`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:187
- Brief: n/a
- Parameters: none

#### `bool cudaAvailable() const`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:188
- Brief: n/a
- Parameters: none

#### `int encodeBatch(Backend backend, int batch_size, double &elapsed_sec)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:116
- Brief: Encode batch of images on specified backend. Returns throughput (ops/sec, computed from elapsed time).
- Parameters:
  - `backend` (Backend): n/a
  - `batch_size` (int): n/a
  - `elapsed_sec` (double &): n/a

#### `std::vector< float > encodeSingleImage(Backend backend)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:101
- Brief: Encode single image on specified backend. Baseline for throughput comparison.
- Parameters:
  - `backend` (Backend): n/a

#### `uint64_t loadModelAndMeasureMemory()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:134
- Brief: Load model and return peak RSS (memory footprint).
- Parameters: none

#### `double measureBatchSplittingOverhead(int total_batch_size, int split_factor)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:173
- Brief: Measure overhead of batch-splitting strategy. Simulates splitting large batch into N smaller batches.
- Parameters:
  - `total_batch_size` (int): n/a
  - `split_factor` (int): n/a

#### `uint64_t measureRuntimeMemory(int batch_size)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:150
- Brief: Measure memory during batch inference.
- Parameters:
  - `batch_size` (int): n/a

### OnnxClipBackendThroughputFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:207
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### OnnxClipBatchSplittingFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### OnnxClipCpuFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### OnnxClipCpuWarmFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:194
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### OnnxClipGoldenEmbeddingsTest

#### `bool compareBatchVsSequential(ONNXClipPlugin &plugin, const std::vector< uint32_t > &imageSeeds, double tolerance=kReproducibilityTolerance) const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:167
- Brief: Compare batch embedding generation with sequential generation.
- Parameters:
  - `plugin` (ONNXClipPlugin &): The ONNX CLIP plugin instance (must be initialized)
  - `imageSeeds` (const std::vector< uint32_t > &): Vector of seeds for generating test images
  - `tolerance` (double): L2 distance tolerance for comparison (default: 1e-6)
- Return: true if all batch results match sequential results, false otherwise
- Details: Helper method for OCP-IT-09 and OCP-IT-10. Verifies that generating N images in a batch produces identical results to generating them sequentially with N separate calls. plugin The ONNX CLIP plugin instance (must be initialized) imageSeeds Vector of seeds for generating test images tolerance L2 distance tolerance for comparison (default: 1e-6) true if all batch results match sequential results, false otherwise

#### `double computeL2Distance(const std::vector< float > &a, const std::vector< float > &b) const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:102
- Brief: Compute L2 distance between two embedding vectors.
- Parameters:
  - `a` (const std::vector< float > &): First embedding vector
  - `b` (const std::vector< float > &): Second embedding vector
- Return: L2 distance (should be 0 for identical embeddings, < 1e-6 for reproducibility)
- Details: L2 distance = sqrt(sum((a[i] - b[i])^2)). Used to verify that identical inputs produce identical embeddings. a First embedding vector b Second embedding vector L2 distance (should be 0 for identical embeddings, < 1e-6 for reproducibility)

#### `double computeL2Norm(const std::vector< float > &embedding) const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:83
- Brief: Compute L2 norm of an embedding vector.
- Parameters:
  - `embedding` (const std::vector< float > &): Vector of floating-point embedding components
- Return: L2 norm (should be ≈ 1.0 for normalized embeddings)
- Details: L2 norm is sqrt(sum(v[i]^2)). For normalized embeddings, this should be approximately 1.0. embedding Vector of floating-point embedding components L2 norm (should be ≈ 1.0 for normalized embeddings)

#### `bool isL2Normalized(const std::vector< float > &embedding) const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:126
- Brief: Check if an embedding is properly L2-normalized.
- Parameters:
  - `embedding` (const std::vector< float > &): Embedding vector to check
- Return: true if norm is approximately 1.0, false otherwise
- Details: An embedding is considered normalized if its L2 norm is approximately 1.0 (within kL2NormTolerance). embedding Embedding vector to check true if norm is approximately 1.0, false otherwise

#### `std::vector< uint8_t > makeImageBytes(size_t size=kDefaultImageSize, uint32_t seed=kClipGoldenSeed) const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:60
- Brief: Generate deterministic pseudo-random image bytes.
- Parameters:
  - `size` (size_t): Number of bytes to generate (default: 256)
  - `seed` (uint32_t): Random seed for LCG (default: kClipGoldenSeed)
- Return: Vector of pseudo-random image bytes
- Details: Uses a linear congruential generator (LCG) to produce reproducible image data. The seed parameter allows different images to be generated while maintaining determinism. size Number of bytes to generate (default: 256) seed Random seed for LCG (default: kClipGoldenSeed) Vector of pseudo-random image bytes

#### `PluginConfig makeViTB32Config() const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:136
- Brief: Create a PluginConfig for ViT-B/32 (512-dimensional).
- Parameters: none
- Return: PluginConfig with ViT-B/32 settings
- Details: PluginConfig with ViT-B/32 settings

#### `PluginConfig makeViTL14Config() const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:148
- Brief: Create a PluginConfig for ViT-L/14 (768-dimensional).
- Parameters: none
- Return: PluginConfig with ViT-L/14 settings
- Details: PluginConfig with ViT-L/14 settings

#### `bool verifyConcurrentInference(ONNXClipPlugin &plugin, int threadCount=4, int imagesPerThread=2) const`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:228
- Brief: Verify concurrent inference safety.
- Parameters:
  - `plugin` (ONNXClipPlugin &): The ONNX CLIP plugin instance (must be initialized)
  - `threadCount` (int): Number of concurrent threads to spawn
  - `imagesPerThread` (int): Number of images each thread should process
- Return: true if all threads completed successfully with correct results, false otherwise
- Details: Helper method for OCP-IT-12. Spawns N concurrent threads, each generating embeddings, and verifies that all operations complete without race conditions or data corruption. plugin The ONNX CLIP plugin instance (must be initialized) threadCount Number of concurrent threads to spawn imagesPerThread Number of images each thread should process true if all threads completed successfully with correct results, false otherwise

### OnnxClipHotSwapTest

#### `void SetUp() override`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:68
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:77
- Brief: n/a
- Parameters: none

### OnnxClipMemoryFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### OnnxClipMemoryScalingFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### OnnxClipMmapTest

#### `void SetUp() override`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:168
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:174
- Brief: n/a
- Parameters: none

### OnnxClipThroughputScalingFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:287
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### bench_onnx_clip_cpu.cpp

#### `UseRealTime() -> Unit(benchmark::kMillisecond) ->Arg(1)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:328
- Brief: n/a
- Parameters: none

#### `for(auto _ :state)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `if(latencies.size() > 0)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:474
- Brief: n/a
- Parameters:
  - `size` (latencies.): n/a
  - `<unnamed>` (0): n/a

#### `model_ initialize()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:655
- Brief: n/a
- Parameters: none

#### `latencies reserve(state.max_iterations)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:462
- Brief: n/a
- Parameters:
  - `max_iterations` (state.): n/a

### bench_onnx_clip_vit_backend.cpp

#### `UseRealTime() -> Unit(benchmark::kMillisecond) ->Arg(64)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:367
- Brief: n/a
- Parameters: none

#### `for(auto _ :state)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

### test_onnx_clip_golden_embeddings_focused.cpp

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_01_InitializeViTB32Config)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:312
- Brief: OCP-IT-01: Plugin initialization with ViT-B/32 config.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_01_InitializeViTB32Config): n/a
- Details: Verifies that the ONNX CLIP plugin correctly initializes with ViT-B/32 configuration (512-dimensional embeddings). Acceptance Criteria: initialize() returns true isReady() returns true after initialization getBackend() returns CPU backend Model variant is set correctly

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_02_InitializeViTL14Config)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:342
- Brief: OCP-IT-02: Plugin initialization with ViT-L/14 config.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_02_InitializeViTL14Config): n/a
- Details: Verifies that the ONNX CLIP plugin correctly initializes with ViT-L/14 configuration (768-dimensional embeddings). Acceptance Criteria: initialize() returns true isReady() returns true after initialization getBackend() returns CPU backend Model variant is set correctly

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_03_SingleImageEmbeddingDeterministic)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:377
- Brief: OCP-IT-03: Single image embedding generation (deterministic seed).
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_03_SingleImageEmbeddingDeterministic): n/a
- Details: Verifies that the plugin generates valid embeddings for a single image using a deterministic seed. The embedding should have the correct dimensionality and be L2-normalized. Acceptance Criteria: generateEmbedding() returns success=true Embedding dimension matches expected (512 for ViT-B/32) Embedding is L2-normalized (norm ≈ 1.0) Embedding vector is not all zeros

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_04_BatchEmbeddingGenerationDeterministic)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:424
- Brief: OCP-IT-04: Batch embedding generation (deterministic seed).
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_04_BatchEmbeddingGenerationDeterministic): n/a
- Details: Verifies that the plugin generates valid embeddings for a batch of images using deterministic seeds. Each embedding should have the correct dimensionality and be L2-normalized. Acceptance Criteria: generateEmbeddingBatch() returns one result per input image All results have success=true All embeddings have correct dimension (512 for ViT-B/32) All embeddings are L2-normalized

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_05_L2NormalizationVerification)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:470
- Brief: OCP-IT-05: L2 normalization verification.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_05_L2NormalizationVerification): n/a
- Details: Verifies that all generated embeddings are properly L2-normalized. Tests both ViT-B/32 (512-dim) and ViT-L/14 (768-dim) variants. Acceptance Criteria: ViT-B/32 embedding norm ≈ 1.0 (within 1e-4) ViT-L/14 embedding norm ≈ 1.0 (within 1e-4) All batch embeddings are normalized

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_06_EmbeddingDimensionCorrectness)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:511
- Brief: OCP-IT-06: Embedding dimension correctness.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_06_EmbeddingDimensionCorrectness): n/a
- Details: Verifies that embeddings have the correct dimensionality for their respective model variants (512 for ViT-B/32, 768 for ViT-L/14). Acceptance Criteria: ViT-B/32: embedding.size() == 512 and dimension == 512 ViT-L/14: embedding.size() == 768 and dimension == 768 Batch embeddings preserve correct dimensionality

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_07_ReproducibilityIdenticalInputs)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:576
- Brief: OCP-IT-07: Reproducibility — identical inputs produce identical embeddings.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_07_ReproducibilityIdenticalInputs): n/a
- Details: Verifies that the same input image produces identical embeddings across multiple calls. This ensures the implementation is deterministic. Acceptance Criteria: Multiple calls with identical input produce L2 distance < 1e-6 Single-image and batch-of-1 produce identical results (L2 < 1e-6) Reproducibility holds across different batches

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_08_HealthCheckAndStatistics)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:632
- Brief: OCP-IT-08: Health check and statistics retrieval.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_08_HealthCheckAndStatistics): n/a
- Details: Verifies that health checks work correctly and statistics can be retrieved successfully after initialization and embedding generation. Acceptance Criteria: healthCheck() returns true after initialization healthCheck() returns false before initialization healthCheck() returns false after shutdown getStatistics() returns valid JSON with expected keys Statistics include call counts, backend info, and model info

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_09_BatchOf4VsSequentialEquivalence)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:688
- Brief: OCP-IT-09: Batch-of-4 vs 4 sequential calls produce identical embeddings.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_09_BatchOf4VsSequentialEquivalence): n/a
- Details: Verifies that generating 4 images in a single batch call produces results identical to generating the same 4 images sequentially with individual calls. This ensures batch processing does not introduce numerical differences or affect the deterministic behavior. Acceptance Criteria: Batch-of-4 results match sequential results (L2 distance < 1e-6) All 4 embeddings have correct dimension All 4 embeddings are L2-normalized success flag is true for all results

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_10_BatchOf16VsSequentialEquivalence)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:740
- Brief: OCP-IT-10: Batch-of-16 vs 16 sequential calls produce identical embeddings.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_10_BatchOf16VsSequentialEquivalence): n/a
- Details: Verifies that generating 16 images in a single batch call produces results identical to generating the same 16 images sequentially with individual calls. This tests batch processing correctness at a larger scale. Acceptance Criteria: Batch-of-16 results match sequential results (L2 distance < 1e-6) All 16 embeddings have correct dimension All 16 embeddings are L2-normalized success flag is true for all results

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_11_CrossRunReproducibilityIndependentInstances)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:795
- Brief: OCP-IT-11: Cross-run reproducibility with independent plugin instances.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_11_CrossRunReproducibilityIndependentInstances): n/a
- Details: Verifies that the same embedding can be generated consistently across multiple independent plugin instances. This tests that the deterministic behavior is not dependent on plugin state persistence. Acceptance Criteria: Create 3 independent plugin instances All 3 generate identical embeddings for the same input (L2 distance < 1e-6) All pairwise comparisons (1-2, 2-3, 1-3) show L2 < 1e-6 Batch results are also identical across instances

#### `TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_12_ConcurrentInferenceSafety)`
- Source: `tests/onnx_clip/test_onnx_clip_golden_embeddings_focused.cpp`:917
- Brief: OCP-IT-12: Concurrent inference safety with 4 threads.
- Parameters:
  - `<unnamed>` (OnnxClipGoldenEmbeddingsTest): n/a
  - `<unnamed>` (OCP_IT_12_ConcurrentInferenceSafety): n/a
- Details: Verifies that concurrent inference operations on the same plugin instance do not introduce race conditions, data corruption, or segmentation faults. Each thread generates embeddings independently, and all results must be valid and properly normalized. Acceptance Criteria: 4 concurrent threads all complete without errors All generated embeddings are valid (success=true) All embeddings are L2-normalized No race conditions detected No segmentation faults or undefined behavior

### test_onnx_clip_highcardinality_stress.cpp

#### `TEST(WaveD_OnnxClipStress, ConcurrentInferenceStress)`
- Source: `tests/onnx_clip/test_onnx_clip_highcardinality_stress.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_OnnxClipStress): n/a
  - `<unnamed>` (ConcurrentInferenceStress): n/a

#### `TEST(WaveD_OnnxClipStress, HighCardinalityImageBatch)`
- Source: `tests/onnx_clip/test_onnx_clip_highcardinality_stress.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_OnnxClipStress): n/a
  - `<unnamed>` (HighCardinalityImageBatch): n/a

#### `TEST(WaveD_OnnxClipStress, ModelReloadStress)`
- Source: `tests/onnx_clip/test_onnx_clip_highcardinality_stress.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_OnnxClipStress): n/a
  - `<unnamed>` (ModelReloadStress): n/a

### test_onnx_clip_hot_swap_focused.cpp

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_01_BasicReloadSucceeds)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:89
- Brief: OCP-HS-01: Reload with valid config succeeds, plugin remains operational.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_01_BasicReloadSucceeds): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_02_HealthCheckBeforeAfter)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:107
- Brief: OCP-HS-02: Health check before/after reload returns healthy status.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_02_HealthCheckBeforeAfter): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_03_StateTransitionsCorrect)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:127
- Brief: OCP-HS-03: Reload from Ready state succeeds (state machine transitions).
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_03_StateTransitionsCorrect): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_04_SequentialReloads)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:144
- Brief: OCP-HS-04: Multiple sequential reloads (A → B → A) all succeed.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_04_SequentialReloads): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_05_InFlightCounterTracking)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:174
- Brief: OCP-HS-05: In-flight request counter increments/decrements correctly.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_05_InFlightCounterTracking): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_06_RequestDraining)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:199
- Brief: OCP-HS-06: Reload waits for in-flight requests to complete.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_06_RequestDraining): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_07_DrainingTimeout)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:244
- Brief: OCP-HS-07: Verify reload timeout prevents indefinite hangs.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_07_DrainingTimeout): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_08_NoDroppingRequests)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:265
- Brief: OCP-HS-08: Request draining doesn't drop/corrupt pending requests.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_08_NoDroppingRequests): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_09_ConcurrentInferenceReload)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:312
- Brief: OCP-HS-09: 4 concurrent inference threads + 1 reload thread all succeed.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_09_ConcurrentInferenceReload): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_10_EmbeddingsBeforeReloadValid)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:361
- Brief: OCP-HS-10: Embeddings generated before reload are valid.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_10_EmbeddingsBeforeReloadValid): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_11_EmbeddingsAfterReloadValid)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:376
- Brief: OCP-HS-11: Embeddings generated after reload are valid.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_11_EmbeddingsAfterReloadValid): n/a

#### `TEST_F(OnnxClipHotSwapTest, OCP_HS_12_NoRaceConditionsConcurrent)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:396
- Brief: OCP-HS-12: No race conditions detected in concurrent scenario.
- Parameters:
  - `<unnamed>` (OnnxClipHotSwapTest): n/a
  - `<unnamed>` (OCP_HS_12_NoRaceConditionsConcurrent): n/a

#### `json createTestConfig(const std::string &model_name="clip-vit-base-patch32", int embedding_dim=kDefaultEmbeddingDim, const std::string &backend="cpu")`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:48
- Brief: Create basic plugin config for testing.
- Parameters:
  - `model_name` (const std::string &): n/a
  - `embedding_dim` (int): n/a
  - `backend` (const std::string &): n/a

#### `std::vector< uint8_t > generateTestImage(size_t seed=0)`
- Source: `tests/onnx_clip/test_onnx_clip_hot_swap_focused.cpp`:39
- Brief: Generate deterministic test image data.
- Parameters:
  - `seed` (size_t): n/a

### test_onnx_clip_mmap_focused.cpp

#### `std::string CreateMockModelFile(size_t file_size, const std::string &prefix="test_model")`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:96
- Brief: Create a mock ONNX model file with deterministic content.
- Parameters:
  - `file_size` (size_t): n/a
  - `prefix` (const std::string &): n/a

#### `std::vector< uint8_t > CreateTestImageData(size_t size=256, uint32_t seed=kMmapTestSeed)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:146
- Brief: Create deterministic test image data.
- Parameters:
  - `size` (size_t): n/a
  - `seed` (uint32_t): n/a

#### `bool DeleteMockModelFile(const std::string &path)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:134
- Brief: Delete a mock model file.
- Parameters:
  - `path` (const std::string &): n/a

#### `uint64_t GetRSSBytes()`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:65
- Brief: Read RSS memory usage from /proc/self/status (Linux only).
- Parameters: none

#### `TEST_F(OnnxClipMmapTest, OCP_MM_01_InitSucceedsWithValidConfig)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:193
- Brief: OCP-MM-01: Mmap initialization with valid config succeeds.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_01_InitSucceedsWithValidConfig): n/a
- Details: Verifies that: Plugin initializes with enable_mmap_loading=true Plugin is ready after initialization Mmap path can be configured

#### `TEST_F(OnnxClipMmapTest, OCP_MM_02_FallbackWorks)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:221
- Brief: OCP-MM-02: Mmap gracefully falls back to traditional loading.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_02_FallbackWorks): n/a
- Details: Verifies that: Plugin initializes even with invalid mmap path (graceful fallback) Plugin is ready (using traditional loading) No error is returned (fallback is silent)

#### `TEST_F(OnnxClipMmapTest, OCP_MM_03_InvalidPathHandled)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:249
- Brief: OCP-MM-03: Mmap with invalid file path returns error with meaningful diagnostic.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_03_InvalidPathHandled): n/a
- Details: Verifies that: Plugin can detect path issues Health check reflects the state

#### `TEST_F(OnnxClipMmapTest, OCP_MM_04_CorruptedFileHandled)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:277
- Brief: OCP-MM-04: Mmap with corrupted file handles error gracefully.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_04_CorruptedFileHandled): n/a
- Details: Verifies that: Very small/corrupt files don't cause crashes Plugin handles gracefully

#### `TEST_F(OnnxClipMmapTest, OCP_MM_05_EmbeddingsIdentical)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:321
- Brief: OCP-MM-05: Mmap'd model produces identical embeddings vs traditional loading.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_05_EmbeddingsIdentical): n/a
- Details: Verifies that: Two plugins (one mmap'd, one traditional) produce identical embeddings Deterministic seed ensures reproducibility

#### `TEST_F(OnnxClipMmapTest, OCP_MM_06_BatchInferenceCorrect)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:378
- Brief: OCP-MM-06: Mmap'd model handles batch inference correctly.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_06_BatchInferenceCorrect): n/a
- Details: Verifies that: Batch inference works with mmap'd models All embeddings have correct dimensions

#### `TEST_F(OnnxClipMmapTest, OCP_MM_07_TextEmbeddingsWork)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:419
- Brief: OCP-MM-07: Text embeddings work with mmap'd model.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_07_TextEmbeddingsWork): n/a
- Details: Verifies that: Text embedding generation works with mmap'd models Embeddings have correct dimensions

#### `TEST_F(OnnxClipMmapTest, OCP_MM_08_ConcurrentThreadsCorrect)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:459
- Brief: OCP-MM-08: Concurrent inference threads produce correct results.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_08_ConcurrentThreadsCorrect): n/a
- Details: Verifies that: Multiple threads can safely use mmap'd model concurrently All embeddings are valid and correctly sized

#### `TEST_F(OnnxClipMmapTest, OCP_MM_09_PeakMemoryLower)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:518
- Brief: OCP-MM-09: Peak memory with mmap is < traditional loading.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_09_PeakMemoryLower): n/a
- Details: Verifies that: Mmap'd model uses less peak memory than traditional loading Memory difference is measurable (at least a few MB)

#### `TEST_F(OnnxClipMmapTest, OCP_MM_10_MemorySavingViTB32)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:592
- Brief: OCP-MM-10: Memory reduction for ViT-B/32 >= 10% (target 10-15%).
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_10_MemorySavingViTB32): n/a
- Details: Verifies that: Small model shows measurable memory reduction with mmap Reduction is at least 10% (simulation target)

#### `TEST_F(OnnxClipMmapTest, OCP_MM_11_MemorySavingViTL14)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:632
- Brief: OCP-MM-11: Memory reduction for ViT-L/14 >= 25% (target 30-40%).
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_11_MemorySavingViTL14): n/a
- Details: Verifies that: Large model shows more significant memory reduction with mmap Reduction is at least 25% (simulation target)

#### `TEST_F(OnnxClipMmapTest, OCP_MM_12_MemoryTrackingCorrect)`
- Source: `tests/onnx_clip/test_onnx_clip_mmap_focused.cpp`:675
- Brief: OCP-MM-12: Memory tracking works correctly across batch operations.
- Parameters:
  - `<unnamed>` (OnnxClipMmapTest): n/a
  - `<unnamed>` (OCP_MM_12_MemoryTrackingCorrect): n/a
- Details: Verifies that: Memory usage remains bounded during batch inference No memory leaks occur across multiple batches

### themis::bench::onnx_clip

#### `uint64_t getRssBytes()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:60
- Brief: Utility to read RSS (resident set size) from /proc/self/status. Used for memory footprint tracking.
- Parameters: none
- Return: RSS in bytes, or 0 if unable to read.
- Details: RSS in bytes, or 0 if unable to read.

### themis::bench::onnx_clip::MockOnnxClipBackendModel

#### `MockOnnxClipBackendModel()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:91
- Brief: n/a
- Parameters: none

#### `Backend backend() const`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:187
- Brief: n/a
- Parameters: none

#### `bool cudaAvailable() const`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:188
- Brief: n/a
- Parameters: none

#### `int encodeBatch(Backend backend, int batch_size, double &elapsed_sec)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:116
- Brief: Encode batch of images on specified backend. Returns throughput (ops/sec, computed from elapsed time).
- Parameters:
  - `backend` (Backend): n/a
  - `batch_size` (int): n/a
  - `elapsed_sec` (double &): n/a

#### `std::vector< float > encodeSingleImage(Backend backend)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:101
- Brief: Encode single image on specified backend. Baseline for throughput comparison.
- Parameters:
  - `backend` (Backend): n/a

#### `uint64_t loadModelAndMeasureMemory()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:134
- Brief: Load model and return peak RSS (memory footprint).
- Parameters: none

#### `double measureBatchSplittingOverhead(int total_batch_size, int split_factor)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:173
- Brief: Measure overhead of batch-splitting strategy. Simulates splitting large batch into N smaller batches.
- Parameters:
  - `total_batch_size` (int): n/a
  - `split_factor` (int): n/a

#### `uint64_t measureRuntimeMemory(int batch_size)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:150
- Brief: Measure memory during batch inference.
- Parameters:
  - `batch_size` (int): n/a

### themis::bench::onnx_clip::MockOnnxClipModel

#### `MockOnnxClipModel()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:66
- Brief: n/a
- Parameters: none

#### `std::vector< std::vector< float > > encodeBatch(int batch_size)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:109
- Brief: Encode batch of images (target: varies by batch size).
- Parameters:
  - `batch_size` (int): Number of images in batch
- Return: Batch embeddings (batch_size x 768)
- Details: batch_size Number of images in batch Batch embeddings (batch_size x 768)

#### `std::vector< float > encodeImage(int height=224, int width=224)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:90
- Brief: Encode single image to embedding (target: ≤ 150 ms).
- Parameters:
  - `height` (int): Image height (default 224)
  - `width` (int): Image width (default 224)
- Return: Embedding vector (768 dimensions for CLIP ViT-B/32)
- Details: height Image height (default 224) width Image width (default 224) Embedding vector (768 dimensions for CLIP ViT-B/32)

#### `std::vector< float > encodeText(const std::string &text)`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:122
- Brief: Encode text prompt to embedding (target: ≤ 5 ms).
- Parameters:
  - `text` (const std::string &): Text to encode
- Return: Embedding vector (768 dimensions)
- Details: text Text to encode Embedding vector (768 dimensions)

#### `bool healthCheck()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:138
- Brief: Health-check operation (smoke test). Measures overhead of model availability check.
- Parameters: none

#### `void initialize()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:72
- Brief: Initialize model (simulates weight loading + graph construction). Target: < 500 ms (cold start, no caching).
- Parameters: none

#### `void reset()`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:146
- Brief: Reset model state between benchmark iterations. Called in TearDown to ensure clean slate.
- Parameters: none

### themis::bench::onnx_clip::OnnxClipBackendThroughputFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:207
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipBatchSplittingFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipCpuFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipCpuWarmFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:194
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipInitializationProfiler

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipLatencyRegressionFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:245
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipMemoryFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipMemoryScalingFixture

#### `void SetUp(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::bench::onnx_clip::OnnxClipThroughputScalingFixture

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:287
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &) override`
- Source: `benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (::benchmark::State &): n/a

### themis::plugins::image::ONNXClipPlugin

#### `ONNXClipPlugin()`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:29
- Brief: n/a
- Parameters: none

#### `EmbeddingResult generateEmbedding(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata=nullptr) override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:40
- Brief: Generate Embedding.
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): Input parameter.
  - `metadata` (const ImageMetadata *): Input parameter.
- Return: Return value.
- Details: image_data Input parameter. metadata Input parameter. Return value. Calls: std::chrono::steady_clock::now(), pg(), req_guard(), lock(), computeEmbedding(), count().

#### `std::vector< EmbeddingResult > generateEmbeddingBatch(const std::vector< std::vector< uint8_t > > &images) override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:45
- Brief: Generate Embedding Batch.
- Parameters:
  - `images` (const std::vector< std::vector< uint8_t > > &): Input parameter.
- Return: Return value.
- Details: images Input parameter. Return value. Calls: std::chrono::steady_clock::now(), pg(), req_guard(), lock(), reserve(), size(), push_back(), std::move().

#### `EmbeddingResult generateTextEmbedding(const std::string &text) override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:49
- Brief: Generate Text Embedding.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: std::chrono::steady_clock::now(), pg(), req_guard(), lock(), computeTextEmbedding(), count().

#### `BackendType getBackend() const override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:37
- Brief: n/a
- Parameters: none

#### `PluginInfo getInfo() const override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:33
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatistics() const override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:53
- Brief: n/a
- Parameters: none

#### `bool healthCheck() const override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:52
- Brief: n/a
- Parameters: none

#### `bool initialize(const PluginConfig &config, BackendType backend=BackendType::AUTO) override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:34
- Brief: Initialize.
- Parameters:
  - `config` (const PluginConfig &): Input parameter.
  - `backend` (BackendType): Input parameter.
- Return: True when the operation succeeds.
- Details: config Input parameter. backend Input parameter. True when the operation succeeds. Calls: lock(), std::max(), empty(), sha256HexOfFile(), lk(), fn(), loadModelMmap().

#### `bool isReady() const override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:36
- Brief: n/a
- Parameters: none

#### `bool reloadModel(const PluginConfig &new_config)`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:61
- Brief: -------------------------------------------------------------------- Hot-Swap Model Reloading (Phase 3B) --------------------------------------------------------------------
- Parameters:
  - `new_config` (const PluginConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: Reload Model. new_config Input parameter. True when the operation succeeds. new_config Input parameter. True when the operation succeeds. Calls: std::max(), empty(), sha256HexOfFile(), lk(), fn(), pg(), ol(), std::chrono::seconds().

#### `void setModelHashFn(ModelHashFn fn)`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:72
- Brief: Set Model Hash Fn.
- Parameters:
  - `fn` (ModelHashFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lk(), std::move().

#### `void shutdown() override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:35
- Brief: Shutdown.
- Parameters: none
- Details: Calls: lock(), cleanupMmap().

#### `void warmup() override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:54
- Brief: Warmup.
- Parameters: none
- Details: Calls: lock(), computeEmbedding().

#### `~ONNXClipPlugin() override`
- Source: `src/onnx_clip/onnx_clip_plugin.h`:30
- Brief: n/a
- Parameters: none

### themis::plugins::image::ONNXClipPlugin::Impl

#### `void cleanupMmap() noexcept`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:403
- Brief: n/a
- Parameters: none

#### `EmbeddingResult computeEmbedding(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata, const std::string &model, int embedding_dim_value) const`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:438
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a
  - `model` (const std::string &): n/a
  - `embedding_dim_value` (int): n/a

#### `EmbeddingResult computeTextEmbedding(const std::string &text, const std::string &model, int embedding_dim_value) const`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:474
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a
  - `model` (const std::string &): n/a
  - `embedding_dim_value` (int): n/a

#### `bool loadModelMmap(const std::string &model_path)`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:269
- Brief: Load Model Mmap.
- Parameters:
  - `model_path` (const std::string &): Path to the model.
- Return: True when the operation succeeds.
- Details: model_path Path to the model. True when the operation succeeds. Calls: file_check(), is_open(), seekg(), tellg(), close(), open(), c_str(), mmap().

#### `~Impl()`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:434
- Brief: n/a
- Parameters: none

### themis::plugins::image::RequestGuard

#### `RequestGuard(RequestGuard &&)=delete`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:542
- Brief: n/a
- Parameters:
  - `<unnamed>` (RequestGuard &&): n/a

#### `RequestGuard(const RequestGuard &)=delete`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RequestGuard &): n/a

#### `RequestGuard(std::shared_ptr< ONNXClipPlugin::Impl > impl)`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:526
- Brief: Request Guard.
- Parameters:
  - `impl` (std::shared_ptr< ONNXClipPlugin::Impl >): Input parameter.
- Return: Return value.
- Details: impl Input parameter. Return value.

#### `RequestGuard & operator=(RequestGuard &&)=delete`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (RequestGuard &&): n/a

#### `RequestGuard & operator=(const RequestGuard &)=delete`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:541
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RequestGuard &): n/a

#### `~RequestGuard()`
- Source: `src/onnx_clip/onnx_clip_plugin.cpp`:531
- Brief: n/a
- Parameters: none

