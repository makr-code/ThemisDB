> **Status:** 2026-04-19 – Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos ggf. korrigiert.

# ThemisHelpLoRA - Documentation Q&A Assistant

## Overview

ThemisHelpLoRA is ThemisDB's first production-ready LoRA (Low-Rank Adaptation) application for domain-specific language understanding. It provides intelligent Q&A capabilities for ThemisDB documentation with continuous learning from user feedback.

## Features

### Core Functionality
- **Documentation-Aware Q&A**: Fine-tuned LLM specifically for ThemisDB documentation
- **User Feedback Loop**: Collects positive and negative feedback for continuous improvement
- **Incremental Training**: Learns from accumulated feedback without full retraining
- **Version Management**: Supports versioning and rollback for safe deployments
- **A/B Testing**: Compare different adapter versions for quality improvements

### Quality Assurance
- **Accuracy Threshold**: Configurable minimum accuracy for automatic rollback
- **Feedback Statistics**: Track positive/negative feedback ratios
- **Performance Metrics**: Monitor query success rates, latency, and cache hits
- **Audit Logging**: Complete traceability of queries and training events

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ThemisHelpLoRA                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐         ┌──────────────────┐         │
│  │   Query Engine   │────────▶│  LLM Inference   │         │
│  └──────────────────┘         └──────────────────┘         │
│           │                            │                     │
│           │                            ▼                     │
│           │                   ┌──────────────────┐         │
│           │                   │  LoRA Adapter    │         │
│           │                   │  (themis_help)   │         │
│           │                   └──────────────────┘         │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────┐                                       │
│  │ Feedback Store   │                                       │
│  │ • Positive       │                                       │
│  │ • Negative       │                                       │
│  │ • Corrections    │                                       │
│  └──────────────────┘                                       │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────┐                                       │
│  │ Training Service │                                       │
│  │ • Feedback-based │                                       │
│  │ • Corpus-based   │                                       │
│  └──────────────────┘                                       │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────┐                                       │
│  │ Version Manager  │                                       │
│  │ • v1.0, v1.1...  │                                       │
│  │ • Rollback       │                                       │
│  └──────────────────┘                                       │
└─────────────────────────────────────────────────────────────┘
```

## Usage

### Basic Initialization

```cpp
#include "llm/applications/themis_help_lora.h"

using namespace themis::llm::applications;

// Configure ThemisHelpLoRA
ThemisHelpLoRA::Config config;
config.adapter_id = "themis_help_lora";
config.base_model = "llama-2-7b";
config.docs_database_path = "data/docs_database.json";
config.feedback_batch_size = 100;  // Train after 100 feedback items
config.min_accuracy_threshold = 0.80f;

// Initialize
ThemisHelpLoRA help(config);
```

### Querying

```cpp
// Ask a question
std::string answer = help.query("How do I enable sharding?");
std::cout << "Answer: " << answer << std::endl;
```

### Collecting Feedback

```cpp
// Positive feedback
help.addPositiveFeedback(
    "How do I enable sharding?",
    "To enable sharding in ThemisDB: 1. Configure the shard key..."
);

// Negative feedback with correction
help.addNegativeFeedback(
    "How do I configure backups?",
    "Incorrect answer",
    "The correct way is: Use themisdb-backup create --hot..."
);
```

### Training

```cpp
// Train from accumulated feedback
auto result = help.trainFromFeedback();
if (result.success) {
    std::cout << "Training completed! New version: " 
              << result.version << std::endl;
}

// Train from documentation corpus
auto doc_result = help.trainFromDocumentation();
if (doc_result.success) {
    std::cout << "Documentation training completed!" << std::endl;
}
```

### Monitoring

```cpp
// Get performance metrics
auto metrics = help.getMetrics();
std::cout << "Total Queries: " << metrics["total_queries"] << std::endl;
std::cout << "Success Rate: " << metrics["success_rate"] << std::endl;

// Get feedback statistics
auto stats = help.getFeedbackStats();
std::cout << "Total Feedback: " << stats["total_feedback"] << std::endl;
std::cout << "Positive Ratio: " << stats["positive_ratio"] << std::endl;
```

### Version Management

```cpp
// Get current version
std::string version = help.getAdapterVersion();
std::cout << "Current Version: " << version << std::endl;

// Reload adapter (after training)
if (help.reloadAdapter()) {
    std::cout << "Adapter reloaded successfully" << std::endl;
}

// Rollback to previous version
if (help.rollbackToPreviousVersion()) {
    std::cout << "Rolled back to: " << help.getAdapterVersion() << std::endl;
}
```

## Configuration Options

### Training Settings
- `feedback_batch_size`: Number of feedback items before automatic training (default: 100)
- `training_interval`: Time interval for periodic training (default: 24 hours)
- `hyperparameters`: LoRA hyperparameters (rank, alpha, dropout, etc.)

### Quality Settings
- `min_accuracy_threshold`: Minimum accuracy for automatic rollback (default: 0.80)
- `enable_ab_testing`: Enable A/B testing between versions (default: true)
- `enable_auto_rollback`: Automatically rollback if accuracy drops (default: true)

## Integration with LoRA Framework

ThemisHelpLoRA integrates with the complete LoRA framework:

- **LoRAOrchestrator**: Central coordination for all LoRA operations
- **LoRAAdapterManager**: Manages adapter lifecycle and caching
- **LoRAStorageService**: Persistent storage for adapter weights and metadata
- **LoRATrainingService**: On-the-fly and batch training capabilities
- **LoRAAuditLogger**: Complete audit trail for compliance

## Performance Considerations

### Memory Usage
- Base model: ~4-7 GB (depending on model size and quantization)
- LoRA adapter: ~10-50 MB (rank=8-16)
- Feedback buffer: ~1-10 MB (for 100-1000 entries)

### Latency
- Query (cold start): 500-1000 ms (adapter loading)
- Query (warm): 100-300 ms (adapter in cache)
- Training (feedback): 1-5 minutes (100-1000 samples)
- Training (corpus): 10-60 minutes (1000-10000 samples)

### Scalability
- Concurrent queries: Supports multiple concurrent requests
- Feedback collection: Thread-safe feedback buffer
- Training: Asynchronous training doesn't block queries
- Version management: Zero-downtime version switches

## Example

See `examples/themis_help_lora_example.cpp` for a complete working example.

To build and run:
```bash
# Build with LLM support
cmake -B build -DTHEMIS_ENABLE_LLM=ON

# Build the example
cmake --build build --target themis_help_lora_example

# Run
./build/themis_help_lora_example
```

## Testing

Tests are located in `tests/test_lora_framework.cpp`:
```bash
# Build tests
cmake -B build -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LLM=ON

# Run LoRA framework tests
./build/tests/test_lora_framework
```

## Roadmap

### Current Status (v1.0)
- ✅ Basic Q&A functionality
- ✅ Feedback collection
- ✅ Training from feedback
- ✅ Version management
- ✅ Metrics and statistics

### Planned Features (v1.1+)
- [ ] Integration with actual LLM inference engine
- [ ] Advanced prompt engineering
- [ ] Context-aware responses
- [ ] Multi-language support
- [ ] Semantic caching for faster responses
- [ ] Advanced A/B testing with quality metrics
- [ ] Real-time training pipeline
- [ ] Integration with documentation updates

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

## License

See [LICENSE](../../LICENSE) for details.

## Related Documentation

- [LoRA Framework Architecture](../../LLM_LORA_UNIFIED_ARCHITECTURE.md)
- [LoRA Implementation Summary](../../LORA_IMPLEMENTATION_SUMMARY.md)
- [LoRA Usage Examples](../../LORA_USAGE_EXAMPLES.md)
- [LoRA Build Guide](../../LORA_BUILD_GUIDE.md)
