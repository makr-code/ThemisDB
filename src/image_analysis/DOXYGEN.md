# IMAGE_ANALYSIS DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\image_analysis\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\image_analysis\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 7
- Compounds: 28
- Classes/Structs: 7
- Namespaces: 8
- File Compounds: 7

## Namespaces
- @151230031322134133215035200117200361122313102316
- testing
- themis
- themis::plugins
- themis::plugins::image
- themis::plugins::image::@360102160021325120250363223243311306320042230314
- themis::plugins::image::TesseractOCRPlugin
- themis::plugins::image::YOLOv8OnnxPlugin

## Types
### Classes
- BenchmarkMockPlugin
- MockImageAnalysisPlugin
- RealisticLatencyPlugin
- TesseractPluginTest
- YOLOv8PluginTest

### Structs
- themis::plugins::image::TesseractOCRPlugin::Impl
- themis::plugins::image::YOLOv8OnnxPlugin::Impl

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 132

### BenchmarkMockPlugin

#### `BenchmarkMockPlugin()=default`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:31
- Brief: n/a
- Parameters: none

#### `CaptionResult generateCaption(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata=nullptr, int max_length=50) override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:82
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a
  - `max_length` (int): n/a

#### `EmbeddingResult generateEmbedding(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata=nullptr) override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:56
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a

#### `std::vector< EmbeddingResult > generateEmbeddingBatch(const std::vector< std::vector< uint8_t > > &images) override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:102
- Brief: n/a
- Parameters:
  - `images` (const std::vector< std::vector< uint8_t > > &): n/a

#### `BackendType getBackend() const override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:54
- Brief: n/a
- Parameters: none

#### `PluginInfo getInfo() const override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:33
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatistics() const override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:115
- Brief: n/a
- Parameters: none

#### `bool healthCheck() const override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:114
- Brief: n/a
- Parameters: none

#### `bool initialize(const PluginConfig &config, BackendType backend) override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:47
- Brief: n/a
- Parameters:
  - `config` (const PluginConfig &): n/a
  - `backend` (BackendType): n/a

#### `bool isReady() const override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:53
- Brief: n/a
- Parameters: none

#### `void shutdown() override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:52
- Brief: n/a
- Parameters: none

#### `void warmup() override`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:116
- Brief: n/a
- Parameters: none

### MockImageAnalysisPlugin

#### `MockImageAnalysisPlugin()`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:33
- Brief: n/a
- Parameters: none

#### `CaptionResult generateCaption(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata, int max_length) override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:111
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a
  - `max_length` (int): n/a

#### `EmbeddingResult generateEmbedding(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata) override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:78
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a

#### `std::vector< EmbeddingResult > generateEmbeddingBatch(const std::vector< std::vector< uint8_t > > &images) override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:134
- Brief: n/a
- Parameters:
  - `images` (const std::vector< std::vector< uint8_t > > &): n/a

#### `BackendType getBackend() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:74
- Brief: n/a
- Parameters: none

#### `size_t getInferenceCount() const`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:166
- Brief: n/a
- Parameters: none

#### `PluginInfo getInfo() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:35
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatistics() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:153
- Brief: n/a
- Parameters: none

#### `bool healthCheck() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:149
- Brief: n/a
- Parameters: none

#### `bool initialize(const PluginConfig &config, BackendType backend) override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:60
- Brief: n/a
- Parameters:
  - `config` (const PluginConfig &): n/a
  - `backend` (BackendType): n/a

#### `bool isReady() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:70
- Brief: n/a
- Parameters: none

#### `void shutdown() override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:66
- Brief: n/a
- Parameters: none

#### `void warmup() override`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:161
- Brief: n/a
- Parameters: none

### RealisticLatencyPlugin

#### `RealisticLatencyPlugin()`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:84
- Brief: n/a
- Parameters: none

#### `CaptionResult generateCaption(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata, int max_length) override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:184
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a
  - `max_length` (int): n/a

#### `EmbeddingResult generateEmbedding(const std::vector< uint8_t > &image_data, const ImageMetadata *metadata) override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:128
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `metadata` (const ImageMetadata *): n/a

#### `std::vector< EmbeddingResult > generateEmbeddingBatch(const std::vector< std::vector< uint8_t > > &images) override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:214
- Brief: n/a
- Parameters:
  - `images` (const std::vector< std::vector< uint8_t > > &): n/a

#### `BackendType getBackend() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:124
- Brief: n/a
- Parameters: none

#### `PluginInfo getInfo() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:86
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatistics() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:232
- Brief: n/a
- Parameters: none

#### `bool healthCheck() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:228
- Brief: n/a
- Parameters: none

#### `bool initialize(const PluginConfig &config, BackendType backend) override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:108
- Brief: n/a
- Parameters:
  - `config` (const PluginConfig &): n/a
  - `backend` (BackendType): n/a

#### `bool isReady() const override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:120
- Brief: n/a
- Parameters: none

#### `void shutdown() override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:115
- Brief: n/a
- Parameters: none

#### `void warmup() override`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:240
- Brief: n/a
- Parameters: none

### TesseractPluginTest

#### `void SetUp() override`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:206
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:213
- Brief: n/a
- Parameters: none

### YOLOv8PluginTest

#### `void SetUp() override`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:99
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:106
- Brief: n/a
- Parameters: none

### bench_image_analysis.cpp

#### `Arg(0) -> Arg(1) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(1) -> Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(224) -> Arg(384) ->Arg(512) ->Arg(1024) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (224): n/a

#### `Args({224, 1}) -> Args({224, 4}) ->Args({224, 8}) ->Args({384, 1}) ->Args({384, 4}) ->Args({512, 1}) ->Args({512, 4}) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` ({224, 1}): n/a

#### `BENCHMARK(BM_ImageEmbedding_MemoryAllocation) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:464
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ImageEmbedding_MemoryAllocation): n/a

#### `BENCHMARK(BM_Plugin_GetStatistics)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:546
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Plugin_GetStatistics): n/a

#### `BENCHMARK(BM_Plugin_HealthCheck)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:528
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Plugin_HealthCheck): n/a

#### `BENCHMARK(BM_Plugin_Initialization) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:429
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Plugin_Initialization): n/a

#### `BENCHMARK(BM_Plugin_Warmup) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:442
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Plugin_Warmup): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:548
- Brief: n/a
- Parameters: none

#### `void BM_ImageCaptioning(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:309
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_BackendComparison(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:366
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_Batch(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:266
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_Concurrent(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:393
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_MemoryAllocation(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:448
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_RawVsCompressed(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:337
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_SingleImage(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:234
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageEmbedding_Throughput(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:470
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Plugin_GetStatistics(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:530
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Plugin_HealthCheck(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:516
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Plugin_Initialization(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:420
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Plugin_Warmup(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:431
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> Threads(2) ->Threads(4) ->Threads(8) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `std::vector< uint8_t > generate_mock_image(size_t width, size_t height, int channels=3)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:189
- Brief: Generate mock image data of specified size.
- Parameters:
  - `width` (size_t): n/a
  - `height` (size_t): n/a
  - `channels` (int): n/a
- Details: Creates synthetic image data with realistic size and patterns to simulate actual image processing workloads.

#### `std::vector< uint8_t > generate_mock_jpeg(size_t width, size_t height)`
- Source: `benchmarks/image_analysis/bench_image_analysis.cpp`:213
- Brief: Generate mock JPEG-like compressed image data.
- Parameters:
  - `width` (size_t): n/a
  - `height` (size_t): n/a

### bench_image_analysis_latency.cpp

#### `Arg(0) -> Arg(1) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(1) -> Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->Iterations(200) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:452
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(224) -> Arg(384) ->Arg(512) ->Arg(1024) ->Iterations(300) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:494
- Brief: n/a
- Parameters:
  - `<unnamed>` (224): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:551
- Brief: n/a
- Parameters: none

#### `void BM_Batch_LatencyPerImage(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:411
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Caption_LatencyDistribution(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:373
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Embedding_ColdStartVsWarm(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:257
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Embedding_GPUvsCPU_Latency(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:333
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Embedding_LatencyDistribution_224(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:289
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ImageSize_LatencyImpact(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:461
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SustainedLoad_LatencyStability(benchmark::State &state)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:502
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Iterations(1000) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (1000): n/a

#### `Iterations(500) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (500): n/a

#### `std::vector< uint8_t > generate_test_image(size_t width, size_t height)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:64
- Brief: n/a
- Parameters:
  - `width` (size_t): n/a
  - `height` (size_t): n/a

#### `void simulate_computation(size_t work_units)`
- Source: `benchmarks/image_analysis/bench_image_analysis_latency.cpp`:49
- Brief: Simulate computation work for realistic latency.
- Parameters:
  - `work_units` (size_t): n/a

### benchmark_image_analysis.cpp

#### `Arg(1) -> Arg(2) ->Arg(4) ->Arg(8) ->Arg(16) ->Arg(32)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK(BM_Backend_CPU)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Backend_CPU): n/a

#### `BENCHMARK(BM_Backend_CUDA)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Backend_CUDA): n/a

#### `BENCHMARK(BM_CaptionGeneration)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CaptionGeneration): n/a

#### `BENCHMARK(BM_MemoryAllocation_Embeddings)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MemoryAllocation_Embeddings): n/a

#### `BENCHMARK(BM_PluginInitialization)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PluginInitialization): n/a

#### `BENCHMARK(BM_SingleInference_LargeImage)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SingleInference_LargeImage): n/a

#### `BENCHMARK(BM_SingleInference_MediumImage)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SingleInference_MediumImage): n/a

#### `BENCHMARK(BM_SingleInference_SmallImage)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SingleInference_SmallImage): n/a

#### `BENCHMARK(BM_Throughput_ImagesPerSecond)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Throughput_ImagesPerSecond): n/a

#### `BENCHMARK(BM_WarmupEffect)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WarmupEffect): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:419
- Brief: n/a
- Parameters: none

#### `void BM_Backend_CPU(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:381
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Backend_CUDA(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:398
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BatchInference_VaryingBatchSize(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:204
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CaptionGeneration(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:232
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MemoryAllocation_Embeddings(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:306
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ParallelInference(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:252
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PluginInitialization(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:288
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SingleInference_LargeImage(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:183
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SingleInference_MediumImage(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:166
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SingleInference_SmallImage(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:149
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Throughput_ImagesPerSecond(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:327
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WarmupEffect(benchmark::State &state)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:353
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `std::vector< uint8_t > generateRandomImage(size_t size_bytes)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:126
- Brief: n/a
- Parameters:
  - `size_bytes` (size_t): n/a

#### `std::vector< std::vector< uint8_t > > generateRandomImages(size_t count, size_t size_bytes)`
- Source: `benchmarks/image_analysis/benchmark_image_analysis.cpp`:136
- Brief: n/a
- Parameters:
  - `count` (size_t): n/a
  - `size_bytes` (size_t): n/a

### test_image_analysis_highcardinality_stress.cpp

#### `TEST(ImageAnalysisStress, ConcurrentObjectDetectionStress)`
- Source: `tests/image_analysis/test_image_analysis_highcardinality_stress.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (ImageAnalysisStress): n/a
  - `<unnamed>` (ConcurrentObjectDetectionStress): n/a

#### `TEST(ImageAnalysisStress, HighCardinalityOCRWorkload)`
- Source: `tests/image_analysis/test_image_analysis_highcardinality_stress.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (ImageAnalysisStress): n/a
  - `<unnamed>` (HighCardinalityOCRWorkload): n/a

#### `TEST(ImageAnalysisStress, MixedOCRDetectionEdgeCaseStress)`
- Source: `tests/image_analysis/test_image_analysis_highcardinality_stress.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (ImageAnalysisStress): n/a
  - `<unnamed>` (MixedOCRDetectionEdgeCaseStress): n/a

### test_image_analysis_phase1_focused.cpp

#### `TEST(TesseractPluginNoFixtureTest, IMP_OCR_02_InitStub)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginNoFixtureTest): n/a
  - `<unnamed>` (IMP_OCR_02_InitStub): n/a

#### `TEST(TesseractPluginNoFixtureTest, IMP_OCR_03_ReadyLifecycle)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginNoFixtureTest): n/a
  - `<unnamed>` (IMP_OCR_03_ReadyLifecycle): n/a

#### `TEST(YOLOv8PluginNoFixtureTest, IMP_YOL_02_InitStub)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginNoFixtureTest): n/a
  - `<unnamed>` (IMP_YOL_02_InitStub): n/a

#### `TEST(YOLOv8PluginNoFixtureTest, IMP_YOL_03_ReadyLifecycle)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginNoFixtureTest): n/a
  - `<unnamed>` (IMP_YOL_03_ReadyLifecycle): n/a

#### `TEST_F(TesseractPluginTest, IMP_OCR_01_PluginInfo)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginTest): n/a
  - `<unnamed>` (IMP_OCR_01_PluginInfo): n/a

#### `TEST_F(TesseractPluginTest, IMP_OCR_04_DetectReturnStructure)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginTest): n/a
  - `<unnamed>` (IMP_OCR_04_DetectReturnStructure): n/a

#### `TEST_F(TesseractPluginTest, IMP_OCR_05_EmbeddingUnsupported)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginTest): n/a
  - `<unnamed>` (IMP_OCR_05_EmbeddingUnsupported): n/a

#### `TEST_F(TesseractPluginTest, IMP_OCR_06_HealthCheck)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginTest): n/a
  - `<unnamed>` (IMP_OCR_06_HealthCheck): n/a

#### `TEST_F(TesseractPluginTest, IMP_OCR_07_StatisticsKeys)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginTest): n/a
  - `<unnamed>` (IMP_OCR_07_StatisticsKeys): n/a

#### `TEST_F(TesseractPluginTest, IMP_OCR_08_LastOcrResultConsistency)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (TesseractPluginTest): n/a
  - `<unnamed>` (IMP_OCR_08_LastOcrResultConsistency): n/a

#### `TEST_F(YOLOv8PluginTest, IMP_YOL_01_PluginInfo)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginTest): n/a
  - `<unnamed>` (IMP_YOL_01_PluginInfo): n/a

#### `TEST_F(YOLOv8PluginTest, IMP_YOL_04_DetectReturnStructure)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginTest): n/a
  - `<unnamed>` (IMP_YOL_04_DetectReturnStructure): n/a

#### `TEST_F(YOLOv8PluginTest, IMP_YOL_05_EmbeddingUnsupported)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginTest): n/a
  - `<unnamed>` (IMP_YOL_05_EmbeddingUnsupported): n/a

#### `TEST_F(YOLOv8PluginTest, IMP_YOL_06_HealthCheck)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginTest): n/a
  - `<unnamed>` (IMP_YOL_06_HealthCheck): n/a

#### `TEST_F(YOLOv8PluginTest, IMP_YOL_07_StatisticsKeys)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginTest): n/a
  - `<unnamed>` (IMP_YOL_07_StatisticsKeys): n/a

#### `TEST_F(YOLOv8PluginTest, IMP_YOL_08_HighThresholdNoDetections)`
- Source: `tests/image_analysis/test_image_analysis_phase1_focused.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (YOLOv8PluginTest): n/a
  - `<unnamed>` (IMP_YOL_08_HighThresholdNoDetections): n/a

### themis::plugins::image::TesseractOCRPlugin::Impl

#### `bool init(const PluginConfig &config, BackendType)`
- Source: `src/image_analysis/tesseract_ocr_plugin.cpp`:96
- Brief: Init.
- Parameters:
  - `config` (const PluginConfig &): Input parameter.
  - `<unnamed>` (BackendType): n/a
- Return: True when the operation succeeds.
- Details: config Input parameter. BackendType Input parameter. True when the operation succeeds. Calls: empty(), c_str(), Init(), reset(), SetPageSegMode(), SetVariable(), store().

#### `OcrResult runOcr(const std::vector< uint8_t > &image_data, float effective_conf_01)`
- Source: `src/image_analysis/tesseract_ocr_plugin.cpp`:135
- Brief: -------------------------------------------------------------------- Perform OCR on raw image bytes --------------------------------------------------------------------
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): Input parameter.
  - `effective_conf_01` (float): Input parameter.
- Return: Return value.
- Details: image_data Input parameter. effective_conf_01 Input parameter. Return value. Calls: lk(), encoded(), size(), data(), cv::imdecode(), empty(), SetImage(), SetSourceResolution().

### themis::plugins::image::YOLOv8OnnxPlugin::Impl

#### `bool init(const PluginConfig &config, BackendType requested_backend)`
- Source: `src/image_analysis/yolov8_onnx_plugin.cpp`:176
- Brief: Init.
- Parameters:
  - `config` (const PluginConfig &): Input parameter.
  - `requested_backend` (BackendType): Input parameter.
- Return: True when the operation succeeds.
- Details: config Input parameter. requested_backend Input parameter. True when the operation succeeds. Calls: empty(), loadLabels(), std::move(), SetGraphOptimizationLevel(), SetIntraOpNumThreads(), AppendExecutionProvider_CUDA(), wpath(), begin().

#### `DetectionResult postprocess(const std::vector< float > &raw, int orig_w, int orig_h, float effective_conf) const`
- Source: `src/image_analysis/yolov8_onnx_plugin.cpp`:290
- Brief: n/a
- Parameters:
  - `raw` (const std::vector< float > &): n/a
  - `orig_w` (int): n/a
  - `orig_h` (int): n/a
  - `effective_conf` (float): n/a

#### `bool preprocess(const std::vector< uint8_t > &image_data, std::vector< float > &tensor_out, int &orig_w, int &orig_h) const`
- Source: `src/image_analysis/yolov8_onnx_plugin.cpp`:245
- Brief: n/a
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): n/a
  - `tensor_out` (std::vector< float > &): n/a
  - `orig_w` (int &): n/a
  - `orig_h` (int &): n/a

#### `DetectionResult runInference(const std::vector< uint8_t > &image_data, float effective_conf)`
- Source: `src/image_analysis/yolov8_onnx_plugin.cpp`:408
- Brief: Run Inference.
- Parameters:
  - `image_data` (const std::vector< uint8_t > &): Input parameter.
  - `effective_conf` (float): Input parameter.
- Return: Return value.
- Details: image_data Input parameter. effective_conf Input parameter. Return value. Calls: preprocess(), Ort::MemoryInfo::CreateCpu(), data(), size(), c_str(), Run(), std::string(), what().

