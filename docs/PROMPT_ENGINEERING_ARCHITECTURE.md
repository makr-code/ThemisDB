# Prompt Engineering System - Architecture and Usage

## Overview

The Prompt Engineering system in ThemisDB provides a comprehensive framework for managing, optimizing, and tracking prompt templates used with LLM integrations. The system supports autonomous self-improvement through performance tracking and iterative optimization.

## Namespace

All prompt engineering components are organized under the `themis::prompt_engineering` namespace to clearly separate them from other LLM infrastructure components.

```cpp
namespace themis {
namespace prompt_engineering {
    // All prompt engineering classes live here
}
}
```

## Core Components

### 1. PromptManager (`prompt_manager.h`)

**Purpose**: Template storage and management with variable injection

**Key Features**:
- In-memory and RocksDB-backed storage
- YAML configuration loading
- Context variable injection (`{variable}` → value)
- Template versioning and metadata
- Schema-aware context building

**Example Usage**:
```cpp
using namespace themis::prompt_engineering;

// Create a prompt manager
PromptManager pm;

// Create a template
PromptManager::PromptTemplate t;
t.name = "summarize";
t.version = "v1";
t.content = "Summarize the following text: {text}";
auto created = pm.createTemplate(t);

// Inject context
std::unordered_map<std::string, std::string> context;
context["text"] = "Long document...";
auto prompt = pm.getPromptWithContext(created.id, context);
```

### 2. PromptOptimizer (`prompt_optimizer.h`)

**Purpose**: Iterative prompt improvement using feedback loops

**Key Features**:
- DSPy-inspired optimization framework
- Multi-round iterative refinement
- Convergence detection
- Version history tracking
- Configurable evaluation functions

**Example Usage**:
```cpp
OptimizationConfig config;
config.max_iterations = 5;
config.target_score = 0.9;

PromptOptimizer optimizer(config);

std::vector<TestCase> test_cases = {
    {"input1", "expected1", {}},
    {"input2", "expected2", {}}
};

auto result = optimizer.optimize(
    "Initial prompt",
    test_cases,
    evaluationFunction
);

std::cout << "Final score: " << result.final_score << std::endl;
std::cout << "Optimized prompt: " << result.optimized_prompt << std::endl;
```

### 3. PromptEvaluator (`prompt_evaluator.h`)

**Purpose**: Metrics-based evaluation of prompt quality

**Key Features**:
- Semantic similarity (Jaccard, extensible to embeddings)
- Exact and partial matching (Levenshtein distance)
- Relevance scoring
- Statistical significance testing
- Batch evaluation

**Example Usage**:
```cpp
PromptEvaluator evaluator;

// Single evaluation
auto metrics = evaluator.evaluateSingle(
    "actual output",
    "expected output"
);

// Batch evaluation
std::vector<std::string> outputs = {...};
std::vector<std::string> expected = {...};
auto aggregated = evaluator.evaluateBatch(outputs, expected);

std::cout << "Overall score: " << aggregated.overall_score << std::endl;
```

### 4. MetaPromptGenerator (`meta_prompt_generator.h`)

**Purpose**: Generate improvement suggestions using meta-prompting

**Key Features**:
- Template-based meta-prompt generation
- Feedback incorporation
- Pattern extraction from successful prompts
- Multiple improvement strategies (iterative, analytical, creative)

**Example Usage**:
```cpp
MetaPromptGenerator generator;

auto result = generator.generateImprovementPrompt(
    "Original prompt",
    "Feedback: needs more specificity",
    0.6  // current score
);

std::cout << "Improvement suggestions: " 
          << result.improvement_suggestion << std::endl;
```

### 5. PromptPerformanceTracker (`prompt_performance_tracker.h`) ⭐ **NEW**

**Purpose**: Track execution metrics for autonomous optimization

**Key Features**:
- Success rate tracking
- Latency measurement
- User feedback collection
- Low-performer identification
- RocksDB persistence
- Thread-safe metric recording

**Example Usage**:
```cpp
PromptPerformanceTracker tracker;

// Record executions
tracker.recordExecution("prompt_id", true, 123.5);  // success, 123.5ms
tracker.recordExecution("prompt_id", false, 250.0); // failure, 250ms
tracker.recordExecution("prompt_id", true, 100.0, 0.9); // with user feedback

// Get metrics
auto metrics = tracker.getMetrics("prompt_id");
if (metrics) {
    std::cout << "Success rate: " << metrics->success_rate << std::endl;
    std::cout << "Avg latency: " << metrics->avg_latency_ms << "ms" << std::endl;
}

// Find low performers
auto low_performers = tracker.getLowPerformingPrompts(0.7, 10);
for (const auto& id : low_performers) {
    std::cout << "Low performer: " << id << std::endl;
}
```

## Integration Points

### HTTP Server Integration

The PromptManager is integrated into the HTTP server for API-based template management:

```cpp
// In HTTP Server initialization
prompt_manager_ = std::make_shared<prompt_engineering::PromptManager>(storage_.get());

// API endpoints available:
// POST /prompt_template - Create template
// GET /prompt_template - List templates
// GET /prompt_template/:id - Get template
// PUT /prompt_template/:id - Update template
```

### MCP Server Integration

The Model Context Protocol server uses PromptManager for dynamic prompt generation:

```cpp
auto context = prompt_engineering::PromptManager::buildContextFromSchema(
    schema_mgr_.get(),
    "Community",
    "1.5.0"
);
```

## Autonomous Self-Improvement Workflow

The system supports autonomous optimization through the following workflow:

```
┌─────────────────────────────────────────────────────────────────┐
│                     Execution Phase                              │
│  1. LLM generates response using prompt template                 │
│  2. PromptPerformanceTracker records metrics                     │
│     - Success/failure                                            │
│     - Latency                                                    │
│     - User feedback                                              │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Analysis Phase                               │
│  1. Identify low-performing prompts                              │
│  2. Check if optimization threshold met                          │
│     - Min executions (e.g., 100)                                 │
│     - Success rate < threshold (e.g., 0.7)                       │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Optimization Phase                             │
│  1. PromptOptimizer runs improvement cycle                       │
│  2. MetaPromptGenerator suggests improvements                    │
│  3. PromptEvaluator validates changes                            │
│  4. New version created if improved                              │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                      A/B Testing Phase                           │
│  1. Deploy both old and new prompts                              │
│  2. Track comparative performance                                │
│  3. Rollback if new version underperforms                        │
└─────────────────────────────────────────────────────────────────┘
```

## Configuration Example

Example YAML configuration for prompt templates:

```yaml
prompts:
  query_enhancement_v1:
    name: "Query Enhancement"
    version: "1.0"
    description: "Enhances user queries with context"
    content: |
      Given the following context about ThemisDB {version}:
      - Tables: {tables}
      - Capabilities: {capabilities}
      
      Enhance this user query: {query}
      
      Provide a more specific, database-aware query.
    metadata:
      category: "query_processing"
      model: "gpt-4"
    active: true

  summarization_v2:
    name: "Document Summarization"
    version: "2.0"
    description: "Summarizes documents with key points"
    content: |
      Summarize the following document in {max_length} words:
      
      {document}
      
      Focus on:
      - Key findings
      - Action items
      - Important dates
    active: true
```

## Performance Considerations

1. **In-Memory Storage**: Default mode, fastest access
2. **RocksDB Persistence**: Enable for durability across restarts
3. **Concurrent Access**: All components are thread-safe
4. **Metric Overhead**: ~0.1-1% overhead for performance tracking

## Future Enhancements

### Phase 3: Self-Improvement Orchestration (Planned)
- `SelfImprovementOrchestrator`: Automated optimization scheduling
- A/B testing framework
- Automatic rollback on performance degradation

### Phase 4: Feedback Collection (Planned)
- `FeedbackCollector`: Structured feedback aggregation
- Hallucination detection
- Failed query analysis

### Phase 5: Version Control (Planned)
- `PromptVersionControl`: Git-like version management
- Branching and merging
- Diff visualization

### Phase 6: Integration Layer (Planned)
- `PromptEngineeringIntegration`: Seamless LLM integration
- Automatic prompt enhancement hooks
- Background optimization triggers

## Testing

All components have comprehensive unit tests:

```bash
# Run prompt engineering tests
ctest -R prompt

# Specific test suites
ctest -R test_prompt_manager
ctest -R test_prompt_optimizer
ctest -R test_prompt_evaluator
ctest -R test_meta_prompt_generator
ctest -R test_prompt_performance_tracker
```

## Best Practices

1. **Start with Templates**: Use PromptManager to organize prompts
2. **Track Everything**: Enable PromptPerformanceTracker from day one
3. **Iterate Often**: Use PromptOptimizer for continuous improvement
4. **Validate Changes**: Always use PromptEvaluator before deployment
5. **Monitor Metrics**: Check performance trends regularly
6. **A/B Test**: Never replace a working prompt without testing

## References

- DSPy: Stanford's prompt optimization framework
- AutoPrompt: Automatic prompt engineering research
- ThemisDB Architecture: `ARCHITECTURE.md`
- LLM Integration: `LLAMA_CPP_INTEGRATION_SUMMARY.md`

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
