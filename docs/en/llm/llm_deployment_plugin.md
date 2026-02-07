# LLM Deployment Plugin for ThemisDB

## Overview

The LLM Deployment Plugin provides production-ready model deployment capabilities for ThemisDB, inspired by Ollama. It enables enterprise-grade model lifecycle management with support for offline/online/auto deployment modes, comprehensive caching, integrity verification, and audit logging.

## Current Limitations

### Ollama Model Export

The Ollama model export functionality is currently in development. When using the Ollama source type, the plugin can:
- ✅ Pull models via Ollama's `/api/pull` endpoint
- ✅ List available models via `/api/tags`
- ✅ Get model manifests via `/api/show`
- ⚠️ Model export to GGUF format is not yet fully implemented

**Workaround**: Use Ollama CLI to manually export models to GGUF format:
```bash
ollama show <model-name> --modelfile > model.gguf
```

### User Context Tracking

The audit logging system currently logs all operations with a "system" user. For full enterprise audit compliance, user context tracking needs to be implemented to capture the actual user or service account initiating operations.

**Future Enhancement**: Integration with ThemisDB's authentication system to capture user identity.

## Features

### Core Capabilities

- **Multiple Deployment Modes**
  - **Offline**: Use only locally cached models (air-gapped environments)
  - **Online**: Always download from remote sources
  - **Auto**: Try local cache first, download if missing (recommended)

- **Multiple Source Support**
  - Ollama API integration (http://localhost:11434)
  - Direct HTTP/HTTPS downloads
  - Local filesystem sources

- **Model Integrity & Security**
  - SHA256/MD5 checksum verification
  - Secure proxy and authentication support
  - Comprehensive audit logging for compliance

- **Intelligent Caching**
  - Configurable cache directory and size limits
  - Automatic cleanup based on age and usage
  - Version tracking and management

- **Enterprise Features**
  - Audit trail for all operations
  - Version management
  - Progress tracking for downloads
  - Resume capability for interrupted downloads

## Configuration

### Example Configuration (YAML)

```yaml
deployment:
  mode: auto  # offline, online, or auto
  cache_directory: "./models"
  enable_cache: true
  max_cache_size_gb: 100
  
  # Ollama integration
  ollama_url: "http://localhost:11434"
  ollama_timeout_seconds: 600
  
  # Security
  verify_checksums: true
  enable_audit_log: true
  audit_log_path: "./logs/model_deployment.log"
  
  # Cleanup policy
  auto_cleanup: false
  max_model_age_days: 90
  keep_versions: 3
  
  # Model sources (priority order)
  sources:
    - type: ollama
      location: "http://localhost:11434"
      priority: 100
    
    - type: local
      location: "/mnt/models"
      priority: 90
```

See `config/llm_deployment.example.yaml` for complete configuration options.

## Usage

### C++ API

#### 1. Initialize Plugin

```cpp
#include "llm/llm_deployment_plugin.h"

using namespace themis::llm;

// Load configuration from YAML
auto config = LLMDeploymentPlugin::loadConfigFromYAML("config/llm_deployment.yaml");
if (!config) {
    // Handle error
}

// Create plugin instance
LLMDeploymentPlugin plugin(*config);
```

#### 2. Deploy a Model

```cpp
// Deploy model (download if not cached)
auto status = plugin.deployModel("llama2:7b");

if (status) {
    std::cout << "Model deployed: " << status->model_path << std::endl;
    std::cout << "Size: " << status->size_bytes << " bytes" << std::endl;
} else {
    std::cerr << "Deployment failed" << std::endl;
}

// Force re-download
auto updated_status = plugin.deployModel("llama2:7b", true);
```

#### 3. Load Model into LLM Plugin

```cpp
#include "llm/llm_plugin_interface.h"

// Assuming you have an ILLMPlugin instance
ILLMPlugin* llm_plugin = /* ... */;

// Load deployed model
if (plugin.loadModel("llama2:7b", llm_plugin)) {
    std::cout << "Model loaded successfully" << std::endl;
} else {
    std::cerr << "Failed to load model" << std::endl;
}
```

#### 4. Manage Models

```cpp
// List available models from sources
auto available = plugin.listAvailableModels();
for (const auto& model : available) {
    std::cout << "Available: " << model << std::endl;
}

// List cached models
auto cached = plugin.listCachedModels();
for (const auto& status : cached) {
    std::cout << "Cached: " << status.model_id 
              << " (" << status.size_bytes << " bytes)" << std::endl;
}

// Get model status
auto model_status = plugin.getModelStatus("llama2:7b");
if (model_status) {
    std::cout << "Model: " << model_status->model_id << std::endl;
    std::cout << "Path: " << model_status->model_path << std::endl;
    std::cout << "Format: " << model_status->format << std::endl;
    std::cout << "Loaded: " << (model_status->is_loaded ? "yes" : "no") << std::endl;
}

// Verify model integrity
if (plugin.verifyModel("llama2:7b")) {
    std::cout << "Checksum verification passed" << std::endl;
}

// Update model to latest version
auto updated = plugin.updateModel("llama2:7b");

// Remove model from cache
if (plugin.removeModel("llama2:7b")) {
    std::cout << "Model removed" << std::endl;
}
```

#### 5. Cache Management

```cpp
// Get cache statistics
auto stats = plugin.getCacheStats();
std::cout << "Total models: " << stats["total_models"] << std::endl;
std::cout << "Cache size: " << stats["total_size_bytes"] << " bytes" << std::endl;

// Clean up old models
int removed = plugin.cleanupOldModels();
std::cout << "Removed " << removed << " old models" << std::endl;

// Get current cache size
size_t cache_size = plugin.getCacheSize();
```

#### 6. Audit Log

```cpp
// Get audit log entries
auto audit = plugin.getAuditLog(50);  // Last 50 entries
for (const auto& entry : audit) {
    std::cout << entry.timestamp << " - " 
              << entry.operation << " - "
              << entry.model_id << " - "
              << (entry.success ? "SUCCESS" : "FAILED") << std::endl;
}
```

### Model Downloader API

For lower-level control, use the `ModelDownloader` class directly:

```cpp
#include "llm/model_downloader.h"

ModelDownloader downloader;

// Download from Ollama
ModelDownloadConfig config;
config.model_name = "llama2:7b";
config.ollama_url = "http://localhost:11434";
config.download_dir = "./models";
config.use_cache = true;

// Optional: Add progress callback
config.progress_callback = [](size_t downloaded, size_t total, const std::string& status) {
    std::cout << status << std::endl;
};

auto result = downloader.downloadFromOllama(config);
if (result.success) {
    std::cout << "Downloaded to: " << result.model_path << std::endl;
    std::cout << "Time: " << result.download_time_seconds << "s" << std::endl;
}

// Download from URL
auto url_result = downloader.downloadFromURL(
    "https://example.com/model.gguf",
    "./models/model.gguf",
    progress_callback
);

// Check if model is available
if (ModelDownloader::isModelAvailable("./models/model.gguf")) {
    std::cout << "Model is ready" << std::endl;
}
```

## Deployment Scenarios

### 1. Air-Gapped/Offline Environment

```yaml
deployment:
  mode: offline
  cache_directory: "/opt/themis/models"
  verify_checksums: true
  sources:
    - type: local
      location: "/mnt/offline-models"
      priority: 100
```

### 2. Cloud Environment with Auto-Cleanup

```yaml
deployment:
  mode: auto
  cache_directory: "/data/models"
  max_cache_size_gb: 50
  auto_cleanup: true
  max_model_age_days: 30
  sources:
    - type: ollama
      location: "http://ollama-service:11434"
      priority: 100
```

### 3. Enterprise with Proxy and Authentication

```yaml
deployment:
  mode: online
  proxy_url: "http://corporate-proxy:8080"
  proxy_username: "service-account"
  proxy_password: "${PROXY_PASSWORD}"
  auth_token: "${MODEL_API_TOKEN}"
  verify_checksums: true
  enable_audit_log: true
  sources:
    - type: https
      location: "https://secure-models.company.com"
      priority: 100
```

## Integration with ThemisDB

The deployment plugin integrates seamlessly with ThemisDB's existing LLM infrastructure:

1. **Build Integration**: Automatically built when `THEMIS_ENABLE_LLM=ON`
2. **Configuration**: Extends `llm_config.yaml` with deployment section
3. **Logging**: Uses ThemisDB's unified logging system
4. **Error Handling**: Consistent error reporting with ThemisDB conventions

## Best Practices

### Security

1. **Always enable checksum verification** in production:
   ```yaml
   verify_checksums: true
   ```

2. **Use HTTPS sources** for remote models:
   ```yaml
   sources:
     - type: https
       location: "https://..."
       checksum_type: sha256
       checksum_value: "actual-checksum-here"
   ```

3. **Enable audit logging** for compliance:
   ```yaml
   enable_audit_log: true
   audit_log_path: "/var/log/themis/deployment.log"
   ```

### Performance

1. **Use caching** to avoid redundant downloads:
   ```yaml
   enable_cache: true
   cache_directory: "/fast-ssd/models"
   ```

2. **Configure appropriate cache limits**:
   ```yaml
   max_cache_size_gb: 100
   ```

3. **Enable auto-cleanup** for long-running deployments:
   ```yaml
   auto_cleanup: true
   max_model_age_days: 90
   ```

### Deployment Modes

- Use **offline** for air-gapped, high-security environments
- Use **online** for development/testing with latest models
- Use **auto** (recommended) for production with fallback capability

## Troubleshooting

### Model Download Fails

1. Check Ollama service is running:
   ```bash
   curl http://localhost:11434/api/version
   ```

2. Verify network connectivity and proxy settings

3. Check audit log for detailed error messages

### Checksum Verification Fails

1. Re-download the model:
   ```cpp
   plugin.deployModel("model-name", true);  // force download
   ```

2. Verify the expected checksum in your configuration

3. Check for file corruption during download

### Cache Issues

1. Check disk space:
   ```cpp
   auto stats = plugin.getCacheStats();
   ```

2. Manually clean cache:
   ```cpp
   plugin.cleanupOldModels();
   ```

3. Check cache directory permissions

## API Reference

For complete API documentation, see the header files:

- `include/llm/llm_deployment_plugin.h` - Main plugin interface
- `include/llm/model_downloader.h` - Model downloader
- `include/utils/checksum_utils.h` - Checksum utilities

## Testing

Run the unit tests:

```bash
cd build
ctest -R test_llm_deployment_plugin -V
```

## Contributing

See `CONTRIBUTING.md` for guidelines on contributing to the LLM deployment plugin.

## License

This plugin is part of ThemisDB and is licensed under the MIT License. See `LICENSE` for details.
