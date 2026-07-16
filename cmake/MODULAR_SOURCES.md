# ThemisDB CMake Modular Sources Documentation
# =============================================
# This document maps modular CMake files to THEMIS_* configuration options

## CMake Source Modules Mapping

### 1. AccelerationBackends.cmake
- **Enabled by**: THEMIS_ENABLE_GPU
- **Default**: ON
- **Purpose**: GPU acceleration backends (CUDA, HIP, OpenCL, OneAPI, DirectX, NCCL, RCCL)
- **Sources**: 
  - cpu_backend_mt.cpp, cpu_backend_tbb.cpp (multi-threaded CPU)
  - directx_backend_full.cpp (Windows DirectX)
  - hip_backend.cpp, hip_fused_kernels.cpp (AMD GPUs)
  - nccl_backend.cpp (NVIDIA multi-GPU)
  - rccl_backend.cpp (AMD multi-GPU)
  - oneapi_backend.cpp (Intel GPUs)
  - opencl_backend.cpp (cross-platform)
  - faiss_gpu_backend.cpp (vector search)
  - paged_memory_manager.cpp, custom_allreduce.cpp (memory management)

### 2. BlobStorage.cmake
- **Enabled by**: Individual backends (THEMIS_ENABLE_S3, THEMIS_ENABLE_AZURE, THEMIS_ENABLE_WEBDAV)
- **Default**: S3/Azure/WebDAV disabled, Filesystem always included
- **Purpose**: Blob storage backends for different cloud providers
- **Sources**:
  - blob_transfer_handler.cpp (core, always included)
  - blob_backend_s3.cpp (requires THEMIS_ENABLE_S3)
  - blob_backend_azure.cpp (requires THEMIS_ENABLE_AZURE)
  - blob_backend_filesystem.cpp (always included)
  - blob_backend_webdav.cpp (requires THEMIS_ENABLE_WEBDAV)

### 3. BufferManagement.cmake
- **Enabled by**: Always (core functionality)
- **Purpose**: Buffer API handlers and changefeed buffering
- **Sources**:
  - buffer_api_handler.cpp
  - buffer_binary_protocol.cpp
  - changefeed_buffer.cpp

### 4. ContentProcessors.cmake
- **Enabled by**: THEMIS_ENABLE_CONTENT
- **Default**: OFF
- **Purpose**: Media and document processors
- **Sources**:
  - audio_processor.cpp
  - image_processor.cpp
  - pdf_processor.cpp
  - cad_processor.cpp
  - geo_processor.cpp

### 5. DistributedTraining.cmake
- **Enabled by**: THEMIS_ENABLE_DISTRIBUTED_TRAINING
- **Default**: OFF
- **Purpose**: Multi-GPU distributed training support
- **Sources**:
  - distributed_dataloader.cpp
  - multi_gpu_trainer.cpp
  - multi_gpu.cpp

### 6. EditionFeatures.cmake
- **Enabled by**: THEMIS_EDITION (ENTERPRISE or HYPERSCALER)
- **Default**: Community Edition (disabled)
- **Purpose**: Enterprise-specific features
- **Sources**:
  - plugin_system_edition.cpp
  - sharding_manager_edition.cpp (ENTERPRISE/HYPERSCALER)
  - gpu_memory_manager_edition.cpp (ENTERPRISE/HYPERSCALER + GPU)

### 7. ErrorHealthServices.cmake
- **Enabled by**: Always (core functionality)
- **Purpose**: Error handling and health monitoring
- **Sources**:
  - error_api_handler.cpp
  - health_error_service.cpp

### 8. IndexQueryEnhancements.cmake
- **Enabled by**: Always (core functionality)
- **Purpose**: Advanced indexing and query functionality
- **Sources**:
  - advanced_vector_index.cpp
  - aggregates.cpp
  - process_mining_functions.cpp
  - process_pattern_matcher.cpp

### 9. LLMIntegration.cmake
- **Enabled by**: THEMIS_ENABLE_LLM
- **Default**: ON
- **Purpose**: LLM inference, LoRA fine-tuning, and assistant functionality
- **Sources**:
  - docs_assistant.cpp, docs_assistant_functions.cpp
  - gguf_converter.cpp
  - llama_resource_manager.cpp
  - llm_process_analyzer.cpp
  - lora_api_handler.cpp, lora_functions.cpp, lora_security_validator.cpp
  - multi_gpu_lora_layer.cpp
  - content_manager_llm.cpp

### 10. MiscellaneousFeatures.cmake
- **Enabled by**: Always (core functionality)
- **Purpose**: Plugin feedback, graph optimization, HTTP adapters, etc.
- **Sources**:
  - feedback_plugin_basic.cpp, feedback_store.cpp
  - graph_auto_buffer.cpp
  - http_type_adapter.cpp
  - sampling_strategy.cpp
  - module_loader.cpp (src/themis/), module_loader_win32.cpp, module_loader_linux.cpp, module_security.cpp
  - ab_test_manager.cpp
  - plugin_dependency_graph.cpp

### 11. RPCServices.cmake
- **Enabled by**: Always (core functionality)
- **Purpose**: RPC service implementation layer
- **Sources**:
  - rpc_service_impl.cpp
  - rpc_service_registry.cpp

### 12. StorageEnhancements.cmake
- **Enabled by**: Always (core functionality)
- **Purpose**: Advanced storage and data management
- **Sources**:
  - differential_update_engine.cpp
  - hybrid_retention_manager.cpp
  - hypertable.cpp
  - paged_optimizer.cpp

## Summary Statistics
- Total .cmake modules: 12
- Always included (core): 7 modules
- Conditionally included (features): 5 modules
- Total new .cpp files added: 79
- Build option coverage: 100% of missing source files

## Usage Example
```cmake
# Enable all acceleration backends
cmake -DTHEMIS_ENABLE_GPU=ON \
       -DTHEMIS_ENABLE_S3=ON \
       -DTHEMIS_ENABLE_AZURE=ON \
       -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=ON \
       -DTHEMIS_ENABLE_CONTENT=ON \
       -DTHEMIS_EDITION=ENTERPRISE
```
