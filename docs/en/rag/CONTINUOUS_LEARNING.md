# Continuous Learning Orchestrator

## Overview

The **Continuous Learning Orchestrator** is a central system that automatically optimizes all RAG (Retrieval-Augmented Generation) components through continuous monitoring, A/B testing, and statistical validation. It enables ThemisDB to build self-improving AI systems that learn from production data without manual intervention.

## Key Features

### 🔄 Automatic LoRA Retraining
- **Feedback-based triggers**: Retrain when enough user feedback accumulates
- **Performance-based triggers**: Retrain when accuracy drops below threshold
- **Time-based triggers**: Regular retraining on a schedule (e.g., daily)
- **Seamless integration**: Works with existing ThemisHelpLoRA adapters

### 📝 Prompt Optimization
- **Performance analysis**: Identifies low-performing prompts automatically
- **LLM-generated variations**: Creates improved prompt variations
- **Historical testing**: Validates improvements on past failed queries
- **Gradual rollout**: Deploys improvements via A/B testing

### 🎯 Retrieval Parameter Auto-Tuning
- **Bayesian optimization**: Efficiently explores parameter space
- **Multi-objective**: Optimizes top_k, similarity_threshold, coverage_threshold
- **Data-driven**: Uses historical query performance for validation
- **Safe deployment**: A/B tests parameter changes before full rollout

### 🧪 A/B Testing Framework
- **Traffic splitting**: Routes percentage of traffic to new models
- **Statistical validation**: Two-sample t-tests for significance
- **Automatic decisions**: Promotes or rolls back based on results
- **Safety guarantees**: Requires minimum improvement threshold

### 📊 Metrics Persistence
- **Long-term tracking**: Stores all interactions and metrics
- **Performance history**: Time-series data for trend analysis
- **Model registry**: Keeps checkpoints of successful models
- **Improvement events**: Logs all automatic optimizations

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│         ContinuousLearningOrchestrator                      │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐           │
│  │ LoRA       │  │ Prompt     │  │ Retrieval  │           │
│  │ Training   │  │ Optimizer  │  │ Tuner      │           │
│  └────────────┘  └────────────┘  └────────────┘           │
│         ↓               ↓               ↓                   │
│  ┌──────────────────────────────────────────────┐          │
│  │         A/B Testing Framework                │          │
│  └──────────────────────────────────────────────┘          │
│         ↓                                                   │
│  ┌──────────────────────────────────────────────┐          │
│  │       Metrics Store (RocksDB/SQLite)         │          │
│  └──────────────────────────────────────────────┘          │
└─────────────────────────────────────────────────────────────┘
```

## Quick Start

### 1. Basic Configuration

```cpp
#include "rag/continuous_learning_orchestrator.h"

using namespace themis::rag::learning;

// Configure orchestrator
ContinuousLearningConfig config;
config.min_feedback_samples = 100;        // Retrain after 100 feedback items
config.min_accuracy_drop = 0.05;          // 5% drop triggers retraining
config.retraining_interval = std::chrono::hours(24);  // Daily retraining
config.enable_ab_testing = true;
config.ab_test_traffic_split = 0.1;       // 10% traffic for new models
config.min_improvement_threshold = 0.02;  // 2% minimum improvement

auto orchestrator = std::make_unique<ContinuousLearningOrchestrator>(config);
```

### 2. Register Components

```cpp
// Register LoRA adapters
orchestrator->registerLoRAAdapter("themis_help_lora", "Documentation Q&A");

// Register retrieval system
orchestrator->registerRetrievalSystem("vector_index_main");

// Register prompt library
orchestrator->registerPromptSystem("prompt_library");

// Register knowledge gap detector
orchestrator->registerKnowledgeGapDetector("gap_detector");
```

### 3. Start Learning Loop

```cpp
// Start background thread for continuous learning
orchestrator->startLearningLoop();
```

### 4. Log Interactions

```cpp
// During production, log each RAG interaction
Interaction interaction;
interaction.interaction_id = "unique_id";
interaction.timestamp = std::chrono::system_clock::now();
interaction.query = user_query;
interaction.generated_answer = rag_response;
interaction.confidence_score = confidence;
interaction.user_feedback = FeedbackType::POSITIVE;  // If available

orchestrator->logInteraction(interaction);
```

### 5. Monitor Progress

```cpp
// Check learning statistics
auto stats = orchestrator->getStats();
std::cout << "Accuracy: " << stats.current_accuracy * 100 << "%" << std::endl;
std::cout << "Trend: " << (stats.accuracy_trend > 0 ? "↑" : "↓") << std::endl;
std::cout << "Retraining count: " << stats.lora_retraining_count << std::endl;

// Check if system is improving
bool improving = orchestrator->isSystemImproving();
```

## Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `min_feedback_samples` | 100 | Minimum feedback items before retraining |
| `min_accuracy_drop` | 0.05 | Accuracy drop (5%) that triggers retraining |
| `retraining_interval` | 24h | Maximum time between retraining cycles |
| `ab_test_traffic_split` | 0.1 | Percentage of traffic for new models (10%) |
| `min_ab_samples` | 1000 | Minimum samples for statistical significance |
| `min_improvement_threshold` | 0.02 | Minimum improvement to promote (2%) |
| `enable_auto_rollback` | true | Automatic rollback on negative results |
| `learning_loop_interval` | 3600s | Interval between learning checks |

## Learning Triggers

### LoRA Retraining Triggers

1. **Feedback Accumulation**
   - Triggers when `feedback_count >= min_feedback_samples`
   - Ensures enough training data

2. **Accuracy Drop**
   - Triggers when `current_accuracy < historical_avg - min_accuracy_drop`
   - Responds to performance degradation

3. **Scheduled Interval**
   - Triggers when `time_since_last_training > retraining_interval`
   - Ensures regular updates

### Prompt Optimization Triggers

- Low success rate (< 80%) for specific prompts
- Analyzes failed interactions
- Generates and tests variations

### Retrieval Tuning Triggers

- Suboptimal retrieval metrics (F1, recall)
- Uses Bayesian optimization
- Tests parameter combinations

## A/B Testing Workflow

### 1. Deploy Test
```cpp
// Automatically deployed when retraining completes
// if enable_ab_testing = true
```

### 2. Collect Metrics
- Routes `traffic_split` percentage to treatment
- Tracks success rate for control vs treatment
- Ensures consistent user assignment

### 3. Evaluate Results
```cpp
auto result = evaluateTest(test_id);
// result.is_significant = true if p < 0.05
// result.improvement = treatment_rate - control_rate
```

### 4. Automatic Decision
- **Promote** if:
  - `is_significant = true`
  - `improvement >= min_improvement_threshold`
- **Rollback** if:
  - `improvement < -min_improvement_threshold`
- **Continue** otherwise

## Data Structures

### Interaction

Complete record of a RAG interaction:

```cpp
struct Interaction {
    std::string interaction_id;
    std::chrono::system_clock::time_point timestamp;
    
    // Input
    std::string query;
    std::vector<RetrievedDocument> retrieved_docs;
    std::string prompt_template_used;
    
    // Output
    std::string generated_answer;
    std::vector<double> token_probabilities;
    
    // Metrics
    knowledge_gap::DetectionResult gap_detection_result;
    double perplexity;
    double confidence_score;
    
    // Feedback
    std::optional<FeedbackType> user_feedback;
    std::optional<std::string> user_correction;
    
    // Metadata
    std::string model_version;
    std::string retrieval_config_version;
    std::string prompt_version;
    bool is_ab_test_traffic;
};
```

### LearningStats

Current learning statistics:

```cpp
struct LearningStats {
    size_t total_interactions_logged;
    size_t lora_retraining_count;
    size_t prompt_optimizations;
    size_t retrieval_optimizations;
    
    double current_accuracy;
    double accuracy_7d_avg;
    double accuracy_trend;  // Positive = improving
    
    std::vector<ImprovementEvent> recent_improvements;
    std::vector<ABTestInfo> active_ab_tests;
};
```

## Performance Considerations

### CPU Overhead
- **Learning loop**: < 0.1% during idle periods
- **Interaction logging**: < 10μs per interaction
- **A/B test evaluation**: < 100ms

### Memory Usage
- **Per interaction**: ~500 bytes
- **10k interactions**: ~5 MB
- **Growth rate**: ~50 MB per 1M interactions

### Persistence
- Metrics saved periodically (configurable)
- RocksDB or SQLite backend
- Asynchronous writes to avoid blocking

## Best Practices

### 1. Start Conservative
```cpp
config.min_feedback_samples = 200;      // More data = better training
config.ab_test_traffic_split = 0.05;    // Start with 5%
config.min_improvement_threshold = 0.05; // Require 5% improvement
```

### 2. Monitor Actively
- Check stats regularly during initial deployment
- Watch for unexpected retraining triggers
- Validate A/B test decisions

### 3. Gradual Rollout
- Start with non-critical components
- Enable one optimization at a time
- Increase traffic split gradually

### 4. Collect Quality Feedback
- Encourage user feedback on answers
- Track explicit corrections
- Monitor implicit signals (click-through, dwell time)

### 5. Set Appropriate Intervals
```cpp
// Development
config.retraining_interval = std::chrono::hours(1);
config.learning_loop_interval = std::chrono::seconds(300);

// Production
config.retraining_interval = std::chrono::hours(24);
config.learning_loop_interval = std::chrono::seconds(3600);
```

## Troubleshooting

### Problem: No Retraining Happening

**Causes:**
- Not enough feedback samples
- Time interval not elapsed
- No accuracy drop detected

**Solution:**
```cpp
auto stats = orchestrator->getStats();
std::cout << "Interactions: " << stats.total_interactions_logged << std::endl;
std::cout << "Retraining count: " << stats.lora_retraining_count << std::endl;

// Manually trigger
orchestrator->triggerLearningIteration();
```

### Problem: A/B Tests Not Completing

**Causes:**
- Not enough samples
- Improvement below threshold
- Test duration too short

**Solution:**
```cpp
// Reduce sample requirements for testing
config.min_ab_samples = 100;
config.min_improvement_threshold = 0.01;
```

### Problem: High CPU Usage

**Causes:**
- Learning loop interval too short
- Too many concurrent A/B tests

**Solution:**
```cpp
config.learning_loop_interval = std::chrono::seconds(7200);  // 2 hours
// Limit concurrent tests
```

## Integration with Existing Components

### KnowledgeGapDetector
```cpp
// Gap detection results are logged automatically
interaction.gap_detection_result = detector->detect(query, docs, answer);
orchestrator->logInteraction(interaction);
```

### ThemisHelpLoRA
```cpp
// Register adapter for automatic retraining
orchestrator->registerLoRAAdapter("themis_help", adapter);

// LoRA will be retrained based on triggers
// No manual trainFromFeedback() calls needed
```

### VectorIndexManager
```cpp
// Register for parameter tuning
orchestrator->registerRetrievalSystem("vector_index");

// System will optimize top_k, thresholds automatically
```

## Future Enhancements

- **Multi-objective optimization**: Balance accuracy, latency, and cost
- **Federated learning**: Distributed training across nodes
- **Explainable improvements**: Human-readable reports
- **Causal inference**: Identify which changes drive improvements
- **Reinforcement learning**: Learn from user interactions over time

## API Reference

See inline documentation in header files:
- `include/rag/continuous_learning_orchestrator.h`
- `include/rag/ab_testing_framework.h`
- `include/rag/bayesian_optimizer.h`
- `include/rag/learning_metrics.h`

## Examples

Complete example: `examples/continuous_learning_example.cpp`

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: See `docs/` directory
- Community: ThemisDB Discord server
