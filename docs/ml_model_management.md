# ML Model Management and Inference

## Overview

The ML Model Management system provides a unified interface for managing and serving different types of machine learning models in ThemisDB. It extends beyond LLMs to support various model types including classifiers, regressors, embedding models, vision models, and more.

## Features

### Model Types Supported

- **LLM**: Large Language Models (integrates with existing LLM infrastructure)
- **Classifier**: Classification models
- **Regressor**: Regression models
- **Embedding**: Embedding/encoder models
- **Vision**: Vision/image processing models
- **Speech**: Speech/audio models
- **Multimodal**: Multimodal models
- **Custom**: Custom model types

### Lifecycle Management

Models go through a defined lifecycle:

```
REGISTERED → DEPLOYING → DEPLOYED → UPDATING → DEPLOYED
                                   ↓
                                RETIRED → (unregistered)
```

### Key Capabilities

1. **Model Registration**: Register models with metadata and configuration
2. **Deployment**: Deploy one or more instances of a model
3. **Inference**: Synchronous and asynchronous inference with load balancing
4. **Scaling**: Manual and automatic scaling based on load
5. **Health Monitoring**: Continuous health checks with auto-recovery
6. **Updates**: Rolling updates with zero downtime
7. **Retirement**: Graceful retirement with request draining

## Usage

### Basic Workflow

```cpp
#include "llm/ml_model_manager.h"

using namespace themis::llm;

// 1. Create manager
MLModelManager::Config config;
config.enable_health_monitoring = true;
config.enable_auto_scaling = true;

MLModelManager manager(config);
manager.start();

// 2. Register a model
MLModelConfig model_config;
model_config.model_id = "sentiment-classifier";
model_config.model_name = "Sentiment Classifier";
model_config.version = "1.0";
model_config.type = MLModelType::CLASSIFIER;
model_config.file_path = "/models/sentiment_v1.onnx";
model_config.format = "onnx";

auto reg_result = manager.registerModel(model_config);

// 3. Deploy the model
auto deploy_result = manager.deployModel("sentiment-classifier", 2);
// Now 2 instances are running

// 4. Run inference
MLInferenceRequest request;
request.model_id = "sentiment-classifier";
request.input_data = json{{"text", "This is great!"}};

auto infer_result = manager.infer(request);
if (infer_result.has_value()) {
    auto response = infer_result.value();
    if (response.success) {
        std::cout << "Result: " << response.output_data << std::endl;
    }
}

// 5. Scale the model (optional)
manager.scaleModel("sentiment-classifier", 5);

// 6. Retire and cleanup
manager.retireModel("sentiment-classifier", 5000);
manager.unregisterModel("sentiment-classifier");
```

### Async Inference

```cpp
manager.inferAsync(request, [](const MLInferenceResponse& response) {
    if (response.success) {
        std::cout << "Async result: " << response.output_data << std::endl;
    }
});
```

### Health Checks and Monitoring

```cpp
// Get model metrics
json metrics = manager.getModelMetrics("sentiment-classifier");
std::cout << "Total requests: " << metrics["total_requests"] << std::endl;
std::cout << "Success rate: " << metrics["success_rate"] << std::endl;

// Get system stats
json stats = manager.getSystemStats();
std::cout << "Total models: " << stats["total_models"] << std::endl;
std::cout << "Healthy instances: " << stats["healthy_instances"] << std::endl;

// List all models
auto models = manager.listModels();
for (const auto& model_id : models) {
    std::cout << "Model: " << model_id << std::endl;
}
```

### Auto-Scaling Configuration

```cpp
MLModelConfig config;
config.model_id = "image-classifier";
config.enable_auto_scaling = true;
config.min_instances = 1;
config.max_instances = 10;
config.scale_up_threshold = 0.8f;    // Scale up at 80% utilization
config.scale_down_threshold = 0.3f;  // Scale down at 30% utilization
```

## Configuration

### MLModelManager::Config

| Parameter | Default | Description |
|-----------|---------|-------------|
| `enable_health_monitoring` | `true` | Enable background health checks |
| `health_check_interval_ms` | `30000` | Interval between health checks (ms) |
| `enable_auto_scaling` | `false` | Enable automatic scaling |
| `scaling_check_interval_ms` | `60000` | Interval between scaling checks (ms) |

### MLModelConfig

| Parameter | Default | Description |
|-----------|---------|-------------|
| `model_id` | - | Unique model identifier |
| `model_name` | - | Human-readable name |
| `version` | - | Model version |
| `type` | - | Model type (LLM, CLASSIFIER, etc.) |
| `file_path` | - | Path to model file |
| `format` | - | Model format (gguf, onnx, pytorch, etc.) |
| `max_batch_size` | `32` | Maximum batch size |
| `max_concurrent_requests` | `100` | Max concurrent requests per instance |
| `timeout_ms` | `30000` | Request timeout |
| `enable_health_check` | `true` | Enable health checks for this model |
| `health_check_interval_ms` | `30000` | Health check interval |
| `unhealthy_threshold` | `3` | Failed checks before marking degraded |

## Integration with Existing Infrastructure

The ML Model Manager integrates seamlessly with ThemisDB's existing LLM infrastructure:

- **LLMModelStorage**: Used for storing model metadata and blobs
- **LazyModelLoader**: Used for lazy loading of LLM models
- **InferenceEngineEnhanced**: Used for LLM inference with caching and batching

For LLM models, the manager delegates to these specialized components. For other model types, it provides a consistent interface while allowing for custom inference implementations.

## Testing

Comprehensive test suite in `tests/test_ml_model_manager.cpp`:

```bash
# Run tests
./test_ml_model_manager

# Or with CTest
ctest -R MLModelManagerTests
```

Test coverage includes:
- Model registration (single and multiple)
- Deployment and undeployment
- Synchronous and asynchronous inference
- Load balancing across instances
- Scaling (up and down)
- Health checks
- Lifecycle transitions
- Error handling
- Concurrent operations

## Architecture

```
┌─────────────────────────────────────────┐
│         MLModelManager                  │
│  ┌─────────────────────────────────┐   │
│  │  Model Registry                  │   │
│  │  - Configs                       │   │
│  │  - Status                        │   │
│  │  - Instances                     │   │
│  └─────────────────────────────────┘   │
│  ┌─────────────────────────────────┐   │
│  │  Health Monitor Thread           │   │
│  │  - Periodic checks               │   │
│  │  - Auto-recovery                 │   │
│  └─────────────────────────────────┘   │
│  ┌─────────────────────────────────┐   │
│  │  Auto-Scaler Thread              │   │
│  │  - Load monitoring               │   │
│  │  - Scale decisions               │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
           │
           ├── For LLM models
           │   └── LLMModelStorage
           │       LazyModelLoader
           │       InferenceEngineEnhanced
           │
           └── For other models
               └── Custom loaders (extensible)
```

## Security Considerations

- Model files should be stored securely and validated before loading
- Access control should be enforced at the API level
- Input validation on inference requests
- Resource limits prevent DoS attacks
- Audit logging for model lifecycle events

## Performance

- Load balancing distributes requests across instances
- Auto-scaling adapts to load patterns
- Health monitoring ensures reliability
- Integration with existing LLM optimizations (caching, batching)

## Future Enhancements

- [ ] Support for model versioning and A/B testing
- [ ] Integration with model registries (MLflow, etc.)
- [ ] Enhanced metrics and observability
- [ ] Model warm-up strategies
- [ ] Circuit breaker pattern for failing models
- [ ] Rate limiting per model
- [ ] Custom inference backends for different formats
- [ ] GPU resource management
- [ ] Distributed deployment across nodes
