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

## Implementation Status

### ✅ Phase 1-2: Foundation (Complete)
- PromptManager, PromptOptimizer, PromptEvaluator, MetaPromptGenerator
- PromptPerformanceTracker

### ✅ Phase 3: Self-Improvement Orchestration (Complete)
- `SelfImprovementOrchestrator`: Automated optimization scheduling
- A/B testing framework
- Automatic rollback on performance degradation

### ✅ Phase 4: Feedback Collection (Complete)
- `FeedbackCollector`: Structured feedback aggregation
- Hallucination detection
- Failed query analysis

### ✅ Phase 5: Version Control (Complete)
- `PromptVersionControl`: Git-like version management
- Branching and merging
- Diff visualization

### ✅ Phase 6: Integration Layer (Complete)
- `PromptEngineeringIntegration`: Seamless LLM integration
- Automatic prompt enhancement hooks
- Background optimization triggers

### ✅ Phase 2 (Security): Prompt Injection Detection (Complete)
- `PromptInjectionDetector`: Pattern-based injection attack detection for prompts and responses
- 10 built-in injection patterns + pluggable custom patterns
- Sanitization via `[REDACTED]` replacement

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

### 6. SelfImprovementOrchestrator (`self_improvement_orchestrator.h`) ⭐ **NEW - Phase 3**

**Purpose**: Orchestrate autonomous prompt optimization with A/B testing and rollback

**Key Features**:
- Automatic optimization triggering based on performance thresholds
- Manual optimization on-demand
- A/B testing framework with statistical analysis
- Automatic rollback on performance degradation
- Optimization history tracking
- Configurable safety guards

**Example Usage**:
```cpp
// Initialize orchestrator
ImprovementConfig config;
config.min_success_rate = 0.7;        // Trigger if below 70%
config.min_executions = 100;          // Need 100 samples
config.enable_ab_testing = true;      // Enable A/B testing
config.ab_test_sample_size = 1000;    // 1000 samples per test

auto orchestrator = std::make_shared<SelfImprovementOrchestrator>(
    config, tracker, optimizer, manager, evaluator
);

// Automatic optimization scan
auto results = orchestrator->runAutoOptimization();
for (const auto& result : results) {
    std::cout << "Optimized " << result.prompt_id 
              << " with " << (result.improvement * 100) << "% improvement\n";
}

// Manual optimization with test cases
std::vector<TestCase> test_cases = {...};
auto result = orchestrator->optimizePrompt("prompt_id", test_cases);

// A/B testing
std::string test_id = orchestrator->startABTest(
    "prompt_id", "version_a", "version_b"
);

// Record observations
orchestrator->recordABTestObservation(test_id, "a", true, 120.5);
orchestrator->recordABTestObservation(test_id, "b", true, 105.2);

// Check results
auto test = orchestrator->getABTestResults(test_id);
if (test && test->is_significant) {
    std::cout << "Version B is significantly better!\n";
}

// Rollback if needed
if (performance_degraded) {
    orchestrator->rollbackPrompt("prompt_id");
}
```

**Configuration Options**:
- `min_success_rate`: Trigger optimization if below this (default: 0.8)
- `min_executions`: Minimum samples before optimization (default: 100)
- `reoptimize_interval`: Hours between re-optimizations (default: 24)
- `max_iterations`: Maximum optimization iterations (default: 5)
- `target_improvement`: Target improvement percentage (default: 0.1 = 10%)
- `enable_ab_testing`: Enable A/B testing before deployment (default: true)
- `ab_test_sample_size`: Samples for A/B test (default: 1000)
- `ab_test_confidence`: Confidence level for significance (default: 0.95)
- `enable_auto_rollback`: Enable automatic rollback (default: true)
- `rollback_threshold`: Rollback if performance < this factor (default: 0.9)

## Autonomous Self-Improvement Workflow (Complete)

With Phase 3 complete, the full autonomous workflow is now operational:

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
│  1. SelfImprovementOrchestrator.shouldOptimize()                 │
│  2. Check if optimization threshold met                          │
│     - Min executions (e.g., 100)                                 │
│     - Success rate < threshold (e.g., 0.7)                       │
│     - Cooldown period elapsed                                    │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Optimization Phase                             │
│  1. SelfImprovementOrchestrator.optimizePrompt()                 │
│  2. PromptOptimizer runs improvement cycle                       │
│  3. MetaPromptGenerator suggests improvements                    │
│  4. PromptEvaluator validates changes                            │
│  5. New version created if improved                              │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                      A/B Testing Phase                           │
│  1. Start A/B test with original vs. optimized                   │
│  2. Route traffic 50/50 between versions                         │
│  3. Track performance for each version                           │
│  4. Perform statistical significance test                        │
│  5. Deploy winner or rollback                                    │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Deployment & Monitoring                       │
│  1. Deploy optimized version to production                       │
│  2. Continue monitoring performance                              │
│  3. Auto-rollback if performance degrades                        │
│  4. Record in optimization history                               │
└─────────────────────────────────────────────────────────────────┘
```

## Complete Integration Example

```cpp
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/self_improvement_orchestrator.h"

using namespace themis::prompt_engineering;

// Initialize all components
auto manager = std::make_shared<PromptManager>(db, cf);
auto tracker = std::make_shared<PromptPerformanceTracker>(db, cf);
auto optimizer = std::make_shared<PromptOptimizer>();
auto evaluator = std::make_shared<PromptEvaluator>();

// Configure autonomous improvement
ImprovementConfig config;
config.min_success_rate = 0.8;
config.enable_ab_testing = true;

auto orchestrator = std::make_shared<SelfImprovementOrchestrator>(
    config, tracker, optimizer, manager, evaluator
);

// In your LLM call wrapper:
void executeLLMQuery(const std::string& prompt_id, const std::string& query) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Get prompt template
    auto prompt = manager->getPromptWithContext(prompt_id, {{"query", query}});
    
    // Execute LLM
    auto response = llm->generate(prompt.value());
    
    auto end = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Track performance
    bool success = !response.empty() && response.find("Error") == std::string::npos;
    tracker->recordExecution(prompt_id, success, latency);
    
    // Periodic check (e.g., every hour)
    static auto last_check = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    if (now - last_check > std::chrono::hours(1)) {
        orchestrator->runAutoOptimization();
        last_check = now;
    }
}
```

## API Endpoints ✅

The prompt engineering system is fully accessible via both REST API and gRPC:

### REST API (HTTP/JSON)

All REST endpoints follow the `/api/v1/prompt_engineering/*` pattern.

### gRPC API (Binary Protocol)

All gRPC methods are defined in `proto/prompt_engineering_service.proto` under the `PromptEngineeringService` service.

---

### Optimization Endpoint

**POST /api/v1/prompt_engineering/optimize**

Trigger manual optimization for a specific prompt.

Request:
```json
{
    "prompt_id": "query_enhancement_v1",
    "strategy": "auto",
    "test_cases": [
        {
            "input": "sample query",
            "expected_output": "expected result",
            "context": {}
        }
    ]
}
```

Response:
```json
{
    "status": "success",
    "prompt_id": "query_enhancement_v1",
    "improvement": 0.15,
    "old_score": 0.75,
    "new_score": 0.90,
    "iterations": 3,
    "ab_testing": true,
    "ab_test_id": "test_12345"
}
```

### A/B Testing Endpoints

**GET /api/v1/prompt_engineering/ab_tests**

List all active A/B tests.

Response:
```json
[
    {
        "test_id": "test_12345",
        "prompt_id": "query_enhancement_v1",
        "version_a": "v1.0",
        "version_b": "v1.1",
        "samples_a": 523,
        "samples_b": 477,
        "score_a": 0.82,
        "score_b": 0.88,
        "is_significant": false,
        "confidence": 0.89
    }
]
```

**GET /api/v1/prompt_engineering/ab_tests/:id**

Get details for a specific A/B test.

**POST /api/v1/prompt_engineering/feedback**

Submit feedback about prompt execution (types: USER_POSITIVE, USER_NEGATIVE, HALLUCINATION_DETECTED, etc.)

**GET /api/v1/prompt_engineering/stats**

Get comprehensive system statistics including integration, performance, and feedback metrics.

**GET /api/v1/prompt_engineering/history/:id**

Get optimization history for a specific prompt.

**GET /api/v1/prompt_engineering/versions/:id**

Get version history for a specific prompt.

**POST /api/v1/prompt_engineering/rollback**

Rollback a prompt to its previous version.

---

## API Comparison: REST vs gRPC

Both HTTP REST and gRPC APIs provide identical functionality. Choose based on your needs:

| Feature | REST API | gRPC API | When to Use |
|---------|----------|----------|-------------|
| **Protocol** | HTTP/JSON | HTTP/2 + Protobuf | |
| **Performance** | Good | Excellent (binary) | gRPC for high-throughput |
| **Ease of Use** | Easy (curl, browsers) | Requires client SDK | REST for ad-hoc testing |
| **Streaming** | Limited (SSE) | Full duplex | gRPC for real-time |
| **Type Safety** | Runtime validation | Compile-time | gRPC for service-to-service |
| **Browser Support** | Native | Requires gRPC-Web | REST for web apps |

### Endpoint Mapping

| Operation | HTTP REST | gRPC Method |
|-----------|-----------|-------------|
| Trigger optimization | `POST /optimize` | `Optimize()` |
| List A/B tests | `GET /ab_tests` | `ListABTests()` |
| Get A/B test details | `GET /ab_tests/:id` | `GetABTest()` |
| Submit feedback | `POST /feedback` | `SubmitFeedback()` |
| Get statistics | `GET /stats` | `GetStats()` |
| Get history | `GET /history/:id` | `GetOptimizationHistory()` |
| Get versions | `GET /versions/:id` | `GetVersions()` |
| Rollback | `POST /rollback` | `Rollback()` |

**Protocol Buffers:** See `proto/prompt_engineering_service.proto` for complete message definitions.

## Prometheus Metrics ✅

The system exports comprehensive metrics in Prometheus format:

### Key Metrics
- `themis_prompt_engineering_optimization_attempts_total` - Total optimization attempts
- `themis_prompt_engineering_ab_tests_active` - Currently active A/B tests  
- `themis_prompt_engineering_prompt_success_rate` - Overall success rate
- `themis_prompt_engineering_hallucination_detections_total` - Hallucinations detected
- `themis_prompt_engineering_version_commits_total` - Version commits
- And 20+ additional metrics for comprehensive observability

See implementation in `prompt_engineering_metrics.h` for complete list.

## Production Deployment Checklist (All Phases)

Before deploying the autonomous self-improvement system:

- [ ] Configure `ImprovementConfig` for your workload
- [ ] Set up RocksDB persistence for metrics
- [ ] Define test cases for critical prompts
- [ ] Enable A/B testing for production safety
- [ ] Configure rollback thresholds
- [ ] Set up monitoring and alerting
- [ ] Schedule periodic `runAutoOptimization()` calls
- [ ] Test rollback mechanism
- [ ] Document prompt templates in YAML
- [ ] Set up logging and audit trails

## Performance Impact

### Phase 3 Addition:
- **Orchestrator overhead**: Negligible (~0.1%)
- **A/B testing**: No additional overhead (routing decision only)
- **Memory usage**: ~1KB per active A/B test
- **Optimization frequency**: Configurable (default: once per 24h)

## Additional Enhancements

### ✅ Completed
- **REST API Endpoints** - Full HTTP/JSON API for all operations
- **gRPC API** - High-performance binary protocol with identical functionality
- **Prometheus Metrics Export** - Comprehensive metrics for all operations
- **Real-time Performance Monitoring** - Via metrics and stats endpoints

### Future Possibilities
- **Grafana Dashboards** - Pre-built dashboards for visualization (can be created using exported metrics)
- **Advanced Analytics**:
  - Machine learning for pattern detection
  - Predictive failure analysis
  - Anomaly detection in prompt performance
  - Long-term trend analysis
- **Alert Integration** - Webhooks for critical events (low success rates, hallucination spikes)


### 7. FeedbackCollector (`feedback_collector.h`) ⭐ **NEW - Phase 4**

**Purpose**: Collect and analyze feedback for quality-driven optimization

**Key Features**:
- 10 feedback types (user feedback, system errors, hallucinations)
- Complete context capture (query, response, metadata)
- Failed query analysis with pattern extraction
- Statistical aggregation per prompt and system-wide
- RocksDB persistence for durability
- Problem identification and prioritization

**Example Usage**:
```cpp
FeedbackCollector collector;

// Record user feedback
collector.recordFeedback(
    "prompt_id",
    "What is AI?",
    "AI stands for...",
    FeedbackType::USER_POSITIVE,
    "Very helpful!",
    0.9  // High satisfaction
);

// Record system-detected issue
collector.recordFeedback(
    "prompt_id",
    "Capital of Atlantis?",
    "Poseidon City...",
    FeedbackType::HALLUCINATION_DETECTED,
    "Fabricated information",
    0.8  // High severity
);

// Get statistics
auto stats = collector.getStats("prompt_id");
std::cout << "Positive ratio: " << stats.positive_ratio << "\n";
std::cout << "Hallucinations: " << stats.hallucination_count << "\n";

// Identify problematic prompts
auto problematic = collector.getPromptsWithNegativeFeedback(0.3, 10);
for (const auto& id : problematic) {
    // Trigger optimization
}

// Analyze failure patterns
auto patterns = collector.analyzeFailurePatterns("prompt_id", 3);
for (const auto& pattern : patterns) {
    std::cout << "Pattern: " << pattern.pattern 
              << " (x" << pattern.occurrences << ")\n";
}

// Get failed queries for test case generation
auto failed = collector.getFailedQueries("prompt_id", 100);
std::vector<TestCase> test_cases;
for (const auto& [query, response, type] : failed) {
    test_cases.push_back({query, response, {}});
}
```

**Feedback Types**:
- `USER_POSITIVE`: Explicitly marked as helpful
- `USER_NEGATIVE`: Explicitly marked as unhelpful
- `HALLUCINATION_DETECTED`: System detected false information
- `TIMEOUT`: Query execution timeout
- `PARSE_ERROR`: Failed to parse response
- `VALIDATION_FAILED`: Response validation failed
- `CONTEXT_MISSING`: Required context missing
- `AMBIGUOUS_OUTPUT`: Unclear output
- `SECURITY_ISSUE`: Security concern
- `PERFORMANCE_ISSUE`: Performance degradation

**Integration with Optimization**:
```cpp
// In optimization workflow
auto problematic = feedback_collector->getPromptsWithNegativeFeedback();

for (const auto& prompt_id : problematic) {
    // Get failure context
    auto failed = feedback_collector->getFailedQueries(prompt_id);
    auto patterns = feedback_collector->analyzeFailurePatterns(prompt_id);
    
    // Generate test cases from failures
    std::vector<TestCase> test_cases;
    for (const auto& [query, response, type] : failed) {
        test_cases.push_back({query, response, {}});
    }
    
    // Optimize with context
    auto result = orchestrator->optimizePrompt(prompt_id, test_cases);
    
    THEMIS_INFO("Optimized {} addressing {} failure patterns",
                prompt_id, patterns.size());
}
```

### 8. PromptVersionControl (`prompt_version_control.h`) ⭐ **NEW - Phase 5**

**Purpose**: Git-like version control system for prompt templates

**Key Features**:
- Commit versions with descriptive messages
- SHA-256 based version IDs (32 hex characters)
- Branch and merge support
- Rollback capabilities
- Diff visualization (line-by-line)
- Tagging system for releases
- Performance score tracking per version
- Complete version genealogy

**Example Usage**:
```cpp
PromptVersionControl vcs(db, cf);

// Commit a new version
std::string version_id = vcs.commit(
    "prompt_id",
    "Summarize: {text}",
    "Initial version",
    "user@example.com",
    "main"
);

// Create a branch for experimentation
vcs.createBranch("prompt_id", "experiment", version_id);

// Make changes on the branch
std::string exp_version = vcs.commit(
    "prompt_id",
    "Provide a concise summary: {text}",
    "Improved wording",
    "user@example.com",
    "experiment"
);

// Compare versions
auto diff = vcs.diff(version_id, exp_version);
std::cout << "+" << diff.additions << " -" << diff.deletions << "\n";
std::cout << diff.unified_diff << "\n";

// Merge back to main if successful
auto merge_result = vcs.merge(
    "prompt_id",
    "experiment",  // source
    "main",        // target
    MergeStrategy::AUTO,
    "Merge improved wording"
);

if (merge_result.success) {
    std::cout << "Merged successfully!\n";
    // Tag for production
    vcs.tag(merge_result.merged_version_id, "production-v1.0");
}

// Rollback if needed
vcs.rollback("prompt_id", version_id, "Reverting to previous version");
```

**Rollback Options**:
- By version ID: `rollback(prompt_id, version_id, message)`
- By count: `rollbackN(prompt_id, n_versions, branch)` - go back N commits

**Merge Strategies**:
- `AUTO`: Intelligent automatic merge (default)
- `OURS`: Keep target branch content
- `THEIRS`: Accept source branch content

**History & Analytics**:
```cpp
// Get version history
auto history = vcs.getHistory("prompt_id", "main", 10);
for (const auto& version : history) {
    std::cout << version.version_id << ": " 
              << version.commit_message << "\n";
}

// Get all branches
auto branches = vcs.listBranches("prompt_id");

// Get version by tag
auto prod_version = vcs.getByTag("prompt_id", "production");

// View genealogy
auto genealogy = vcs.getGenealogy("prompt_id");
```

### 9. PromptEngineeringIntegration (`prompt_engineering_integration.h`) ⭐ **NEW - Phase 6**

**Purpose**: Unified integration layer orchestrating all prompt engineering components

**Key Features**:
- Pre-execution hooks (prompt enhancement)
- Post-execution hooks (metrics and feedback recording)
- Automatic versioning on every execution
- Background optimization worker
- Lifecycle management (start/stop)
- Comprehensive status reporting
- Configurable behavior

**Example Usage**:
```cpp
// Configure integration
IntegrationConfig config;
config.enable_auto_versioning = true;
config.enable_auto_optimization = true;
config.background_worker_enabled = true;
config.background_worker_interval = std::chrono::hours(1);

// Initialize (orchestrates all components)
auto integration = std::make_shared<PromptEngineeringIntegration>(
    config,
    prompt_manager,
    prompt_optimizer,
    performance_tracker,
    orchestrator,
    feedback_collector,
    version_control
);

// Start the integration layer
integration->start();

// Use in LLM workflow
// Before LLM execution
auto ctx = integration->beforeExecution(
    "query_optimizer",
    {{"table", "users"}, {"limit", "10"}}
);

// Execute with enhanced prompt
auto response = llm_wrapper->generate(ctx.enhanced_prompt);

// After LLM execution
integration->afterExecution(
    ctx,
    response,
    true,      // success
    120.0,     // latency_ms
    0.9        // optional feedback score
);

// Monitor system health
auto status = integration->getStatus();
std::cout << "Total executions: " << status.total_executions << "\n";
std::cout << "Total optimizations: " << status.total_optimizations << "\n";

// Get detailed statistics
auto stats = integration->getStats();
std::cout << stats.dump(2) << "\n";

// Stop gracefully
integration->stop();
```

**Background Optimization Worker**:
```cpp
// Worker automatically runs on schedule
// Checks for prompts needing optimization
// Triggers SelfImprovementOrchestrator
// Records results in version control

// Manual trigger
integration->startBackgroundOptimization();

// Check worker status
auto worker_status = integration->getBackgroundWorkerStatus();
std::cout << "Cycles completed: " << worker_status.cycles_completed << "\n";
std::cout << "Next run: " << worker_status.next_scheduled_run << "\n";

// Stop worker
integration->stopBackgroundOptimization();
```

**ExecutionContext** (tracks single execution):
```cpp
struct ExecutionContext {
    std::string execution_id;      // Unique UUID
    std::string prompt_id;
    std::string original_prompt;
    std::string enhanced_prompt;   // With versioning + context
    nlohmann::json context;
    std::string version_id;        // Version used
    std::chrono::system_clock::time_point start_time;
};
```

**IntegrationStatus** (system health):
```cpp
struct IntegrationStatus {
    bool running;
    bool background_worker_active;
    size_t total_executions;
    size_t total_optimizations;
    std::chrono::system_clock::time_point last_optimization;
    size_t active_prompts;
    std::unordered_map<std::string, size_t> executions_by_prompt;
};
```

## Enhanced Autonomous Workflow (Phases 1-6 Complete)

With all 6 phases complete, the system provides a fully integrated, autonomous prompt engineering solution:

```
┌─────────────────────────────────────────────────────────────────┐
│                     Execution Phase (Phase 6)                    │
│  1. PromptEngineeringIntegration.beforeExecution()               │
│     - Loads prompt template (Phase 1: PromptManager)             │
│     - Gets latest version (Phase 5: PromptVersionControl)        │
│     - Injects context variables                                  │
│  2. LLM generates response using enhanced prompt                 │
│  3. PromptEngineeringIntegration.afterExecution()                │
│     - Records metrics (Phase 2: PromptPerformanceTracker)        │
│     - Records feedback (Phase 4: FeedbackCollector)              │
│     - Auto-commits version (Phase 5: PromptVersionControl)       │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Analysis Phase                               │
│  1. PromptPerformanceTracker analyzes metrics                    │
│  2. FeedbackCollector identifies patterns                        │
│  3. SelfImprovementOrchestrator checks triggers (Phase 3)        │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Optimization Phase (Phase 3)                    │
│  1. SelfImprovementOrchestrator.shouldOptimize()                 │
│  2. Collect failure cases from FeedbackCollector                 │
│  3. PromptOptimizer + MetaPromptGenerator improve prompt         │
│  4. PromptVersionControl creates new version                     │
│  5. A/B test new vs old version                                  │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Deployment Phase (Phase 3 & 5)                 │
│  1. Statistical analysis confirms improvement                    │
│  2. Deploy winner automatically                                  │
│  3. Tag version for production (Phase 5)                         │
│  4. OR rollback if degradation detected                          │
└─────────────────────────────────────────────────────────────────┘
```
│  1. Performance analysis (success rate, latency)                 │
│  2. Feedback analysis (patterns, common issues)                  │
│  3. Problem identification (low performers)                      │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Trigger Decision                               │
│  1. SelfImprovementOrchestrator.shouldOptimize()                 │
│  2. Check performance AND feedback thresholds                    │
│  3. Retrieve failure context from FeedbackCollector              │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Optimization Phase                             │
│  1. Generate test cases from failed queries                      │
│  2. PromptOptimizer with failure context                         │
│  3. MetaPromptGenerator with pattern insights                    │
│  4. PromptEvaluator validates improvements                       │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                      A/B Testing Phase                           │
│  1. Start A/B test with original vs. optimized                   │
│  2. Route traffic, collect metrics AND feedback                  │
│  3. Statistical analysis of performance + satisfaction           │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Deployment & Monitoring                       │
│  1. Deploy optimized version                                     │
│  2. Continue collecting feedback                                 │
│  3. Monitor for quality regression                               │
│  4. Auto-rollback if issues increase                             │
└─────────────────────────────────────────────────────────────────┘
```

## Complete Integration Example (Phases 1-6)

```cpp
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/self_improvement_orchestrator.h"
#include "prompt_engineering/feedback_collector.h"

using namespace themis::prompt_engineering;

// Initialize all components
auto manager = std::make_shared<PromptManager>(db, cf);
auto tracker = std::make_shared<PromptPerformanceTracker>(db, cf);
auto optimizer = std::make_shared<PromptOptimizer>();
auto evaluator = std::make_shared<PromptEvaluator>();
auto feedback_collector = std::make_shared<FeedbackCollector>(db, cf);

// Configure autonomous improvement
ImprovementConfig config;
config.min_success_rate = 0.8;
config.enable_ab_testing = true;

auto orchestrator = std::make_shared<SelfImprovementOrchestrator>(
    config, tracker, optimizer, manager, evaluator
);

// In your LLM call wrapper:
void executeLLMQuery(const std::string& prompt_id, const std::string& query) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Get prompt template
    auto prompt = manager->getPromptWithContext(prompt_id, {{"query", query}});
    
    // Execute LLM
    std::string response;
    try {
        response = llm->generate(prompt.value());
    } catch (const std::exception& e) {
        // Record error feedback
        feedback_collector->recordFeedback(
            prompt_id, query, "",
            FeedbackType::PARSE_ERROR,
            e.what(),
            0.8
        );
        throw;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Track performance (Phase 2)
    bool success = !response.empty();
    tracker->recordExecution(prompt_id, success, latency);
    
    // Detect hallucinations (Phase 4)
    if (detectHallucination(response)) {
        feedback_collector->recordFeedback(
            prompt_id, query, response,
            FeedbackType::HALLUCINATION_DETECTED,
            "Contradiction with knowledge base",
            0.9
        );
    }
}

// Periodic optimization with feedback (e.g., hourly):
void scheduledOptimization() {
    // Get prompts with performance issues
    auto low_performers = tracker->getLowPerformingPrompts(0.7, 100);
    
    // Get prompts with negative feedback
    auto negative_feedback = feedback_collector->getPromptsWithNegativeFeedback(0.3, 10);
    
    // Combine and deduplicate
    std::unordered_set<std::string> candidates(low_performers.begin(), low_performers.end());
    candidates.insert(negative_feedback.begin(), negative_feedback.end());
    
    for (const auto& prompt_id : candidates) {
        if (orchestrator->shouldOptimize(prompt_id)) {
            // Get failure context from feedback
            auto failed_queries = feedback_collector->getFailedQueries(prompt_id, 50);
            auto patterns = feedback_collector->analyzeFailurePatterns(prompt_id);
            
            // Generate test cases from failures
            std::vector<TestCase> test_cases;
            for (const auto& [query, response, type] : failed_queries) {
                test_cases.push_back({query, response, {}});
            }
            
            // Log optimization context
            THEMIS_INFO("Optimizing {} with {} failure patterns",
                       prompt_id, patterns.size());
            for (const auto& pattern : patterns) {
                THEMIS_DEBUG("  Pattern: {} ({} occurrences)",
                            pattern.pattern, pattern.occurrences);
            }
            
            // Optimize with context
            auto result = orchestrator->optimizePrompt(prompt_id, test_cases);
            
            THEMIS_INFO("Optimization complete: {}% improvement",
                       result.improvement * 100);
        }
    }
}

// User feedback collection:
void recordUserFeedback(const std::string& prompt_id,
                       const std::string& query,
                       const std::string& response,
                       int rating,  // 1-5
                       const std::string& comment) {
    FeedbackType type = (rating >= 4) ? 
        FeedbackType::USER_POSITIVE : 
        FeedbackType::USER_NEGATIVE;
    
    double severity = 1.0 - (rating / 5.0);
    
    feedback_collector->recordFeedback(
        prompt_id, query, response, type, comment, severity
    );
}
```

## Production Deployment Checklist (Updated for All Phases)

Before deploying the autonomous self-improvement system:

- [ ] Configure `ImprovementConfig` for your workload
- [ ] Set up RocksDB persistence for metrics and feedback
- [ ] Define test cases for critical prompts
- [ ] Enable A/B testing for production safety
- [ ] Configure rollback thresholds
- [ ] **NEW: Set up feedback collection triggers**
- [ ] **NEW: Configure hallucination detection**
- [ ] **NEW: Define feedback aggregation schedules**
- [ ] Set up monitoring and alerting
- [ ] Schedule periodic `runAutoOptimization()` calls
- [ ] Test rollback mechanism
- [ ] Document prompt templates in YAML
- [ ] Set up logging and audit trails
- [ ] **NEW: Configure feedback retention policies**
- [ ] **NEW: Set up quality dashboards**
- [x] **NEW: Instantiate `PromptInjectionDetector` and call `detect()` on every user-supplied prompt before LLM dispatch**
- [x] **NEW: Call `detectInResponse()` on LLM responses when the output is forwarded to further LLM calls**

## Performance Impact (All Phases)

### Complete System Integration (All 6 Phases)
- **Overall overhead**: ~0.5-1.5% (all components)
- **PromptPerformanceTracker**: ~0.1% (Phase 2)
- **SelfImprovementOrchestrator**: ~0.1% (Phase 3)
- **FeedbackCollector**: ~0.1-0.5% (Phase 4)
- **PromptVersionControl**: ~0.1% (Phase 5, commit operations)
- **PromptEngineeringIntegration**: ~0.2% (Phase 6, coordination layer)
- **Memory per prompt**: ~3-5KB (all metadata + version history)
- **Optimization frequency**: Configurable (default: 24h)
- **Storage growth**: ~1-2MB per 1000 prompts (with version history)


## Examples and Demos

Complete working examples are available in the `examples/` directory:

### Phase 1-2: Basic Prompt Management
- **`prompt_optimization_example.cpp`**: Demonstrates basic prompt optimization workflow
- **`domain_prompts_usage_example.cpp`**: Shows domain-specific prompt templates

### Phase 3: Self-Improvement
- **`complete_self_improvement_example.cpp`**: Full autonomous optimization with A/B testing
  - Demonstrates trigger-based optimization
  - Shows A/B testing workflow
  - Includes rollback scenarios

### Phase 4: Feedback Collection
- **`feedback_collection_example.cpp`**: Comprehensive feedback system usage
  - Records various feedback types
  - Analyzes failure patterns
  - Generates test cases from failures

### Phase 5: Version Control
- **`version_control_example.cpp`**: Git-like prompt versioning
  - Commit and rollback operations
  - Branch creation and merging
  - Diff visualization
  - Tagging for releases

### Phase 6: Complete Integration
- **`complete_integration_example.cpp`**: End-to-end system integration
  - Shows all components working together
  - Background optimization worker
  - Status monitoring and reporting
  - Production deployment pattern

### Running Examples

```bash
# Build examples
cd build
make prompt_optimization_example
make complete_self_improvement_example
make feedback_collection_example
make version_control_example
make complete_integration_example

# Run an example
./examples/complete_integration_example
```

## Test Suite

Comprehensive test coverage across all phases:

### Unit Tests
- `test_prompt_manager.cpp`: Template management (Phase 1)
- `test_prompt_optimizer.cpp`: Optimization algorithms (Phase 1)
- `test_meta_prompt_generator.cpp`: Meta-prompting (Phase 1)
- `test_prompt_evaluator.cpp`: Quality metrics (Phase 1)
- `test_prompt_performance_tracker.cpp`: Performance tracking (Phase 2)
- `test_self_improvement_orchestrator.cpp`: Autonomous optimization (Phase 3)
- `test_feedback_collector.cpp`: Feedback collection (Phase 4)
- `test_prompt_version_control.cpp`: Version control (Phase 5)
- `test_prompt_engineering_integration.cpp`: Integration layer (Phase 6)

### Running Tests

```bash
# Run all prompt engineering tests
ctest -R prompt

# Run specific phase tests
ctest -R test_prompt_manager
ctest -R test_self_improvement_orchestrator
ctest -R test_feedback_collector
ctest -R test_prompt_version_control
ctest -R test_prompt_engineering_integration

# Verbose output
ctest -R prompt -V
```

## Quick Start Guide

### 1. Basic Setup (Phases 1-2)

```cpp
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_performance_tracker.h"

using namespace themis::prompt_engineering;

// Initialize
auto manager = std::make_shared<PromptManager>(db, cf);
auto tracker = std::make_shared<PromptPerformanceTracker>(db, cf);

// Create a prompt template
PromptManager::PromptTemplate tmpl;
tmpl.name = "summarize";
tmpl.content = "Summarize: {text}";
auto created = manager->createTemplate(tmpl);

// Use it
auto prompt = manager->getPromptWithContext(created.id, {{"text", "..."}});
// ... execute with LLM ...
tracker->recordExecution(created.id, success, latency_ms, feedback);
```

### 2. Enable Autonomous Optimization (Phase 3)

```cpp
#include "prompt_engineering/self_improvement_orchestrator.h"

ImprovementConfig config;
config.min_success_rate = 0.8;
config.enable_ab_testing = true;

auto orchestrator = std::make_shared<SelfImprovementOrchestrator>(
    config, tracker, optimizer, manager, evaluator
);

// Automatic optimization
auto results = orchestrator->runAutoOptimization();
```

### 3. Add Feedback Collection (Phase 4)

```cpp
#include "prompt_engineering/feedback_collector.h"

auto collector = std::make_shared<FeedbackCollector>(db, cf);

// Record feedback
collector->recordFeedback(
    prompt_id, query, response,
    FeedbackType::USER_POSITIVE,
    "Very helpful!", 0.9
);

// Analyze for optimization
auto problematic = collector->getPromptsWithNegativeFeedback(0.3, 10);
```

### 4. Enable Version Control (Phase 5)

```cpp
#include "prompt_engineering/prompt_version_control.h"

auto vcs = std::make_shared<PromptVersionControl>(db, cf);

// Auto-commit on changes
auto version_id = vcs->commit(prompt_id, content, "Updated for clarity", "user");

// Tag production versions
vcs->tag(version_id, "production-v1.2");

// Rollback if needed
vcs->rollback(prompt_id, previous_version_id, "Reverting due to issue");
```

### 5. Full Integration (Phase 6)

```cpp
#include "prompt_engineering/prompt_engineering_integration.h"

IntegrationConfig config;
config.background_worker_enabled = true;
config.enable_auto_versioning = true;

auto integration = std::make_shared<PromptEngineeringIntegration>(
    config, manager, optimizer, tracker, orchestrator, collector, vcs
);

integration->start();

// Use in your LLM workflow
auto ctx = integration->beforeExecution(prompt_id, context);
auto response = llm->generate(ctx.enhanced_prompt);
integration->afterExecution(ctx, response, true, 120.0, 0.9);

// System runs autonomously!
```

### 6. Using the REST API

Access all features via HTTP endpoints:

```bash
# Trigger optimization
curl -X POST http://localhost:8080/api/v1/prompt_engineering/optimize \
  -H "Content-Type: application/json" \
  -d '{
    "prompt_id": "query_enhancement",
    "strategy": "auto"
  }'

# Get system statistics
curl http://localhost:8080/api/v1/prompt_engineering/stats

# Submit feedback
curl -X POST http://localhost:8080/api/v1/prompt_engineering/feedback \
  -H "Content-Type: application/json" \
  -d '{
    "prompt_id": "query_enhancement",
    "query": "test query",
    "response": "test response",
    "type": "USER_POSITIVE",
    "severity": 0.9
  }'

# View Prometheus metrics
curl http://localhost:8080/metrics | grep themis_prompt_engineering
```

### 7. Using the gRPC API

Access via high-performance binary protocol:

```cpp
#include "proto/prompt_engineering_service.grpc.pb.h"

// Connect to gRPC server
auto channel = grpc::CreateChannel(
    "localhost:18765",
    grpc::InsecureChannelCredentials()
);

auto stub = prompt_engineering::PromptEngineeringService::NewStub(channel);

// Trigger optimization
prompt_engineering::OptimizeRequest request;
request.set_prompt_id("query_enhancement");
request.set_strategy("auto");

prompt_engineering::OptimizeResponse response;
grpc::ClientContext context;

auto status = stub->Optimize(&context, request, &response);
if (status.ok()) {
    std::cout << "Improvement: " << response.improvement() << std::endl;
}

// Submit feedback
prompt_engineering::FeedbackRequest fb_request;
fb_request.set_prompt_id("query_enhancement");
fb_request.set_query("test query");
fb_request.set_response("test response");
fb_request.set_type(prompt_engineering::USER_POSITIVE);
fb_request.set_severity(0.9);

prompt_engineering::FeedbackResponse fb_response;
grpc::ClientContext fb_context;

auto fb_status = stub->SubmitFeedback(&fb_context, fb_request, &fb_response);

// Get statistics
prompt_engineering::StatsRequest stats_request;
prompt_engineering::StatsResponse stats_response;
grpc::ClientContext stats_context;

auto stats_status = stub->GetStats(&stats_context, stats_request, &stats_response);
if (stats_status.ok()) {
    std::cout << "Total executions: " 
              << stats_response.integration().total_executions() << std::endl;
    std::cout << "Success rate: " 
              << stats_response.performance().avg_success_rate() << std::endl;
}
```

### Python gRPC Client Example

```python
import grpc
import prompt_engineering_service_pb2
import prompt_engineering_service_pb2_grpc

# Connect to server
channel = grpc.insecure_channel('localhost:18765')
stub = prompt_engineering_service_pb2_grpc.PromptEngineeringServiceStub(channel)

# Trigger optimization
request = prompt_engineering_service_pb2.OptimizeRequest(
    prompt_id="query_enhancement",
    strategy="auto"
)

response = stub.Optimize(request)
print(f"Improvement: {response.improvement}")

# Submit feedback
feedback_request = prompt_engineering_service_pb2.FeedbackRequest(
    prompt_id="query_enhancement",
    query="test query",
    response="test response",
    type=prompt_engineering_service_pb2.USER_POSITIVE,
    severity=0.9
)

feedback_response = stub.SubmitFeedback(feedback_request)
print(f"Feedback ID: {feedback_response.feedback_id}")
```

## Additional Resources

- **Implementation Summaries**:
  - `IMPLEMENTATION_SUMMARY_PROMPT_ENGINEERING.md`: Complete implementation details
  - `PHASE3_COMPLETE_SUMMARY.md`: Phase 3 (Self-Improvement) details
  - `PHASE4_COMPLETE_SUMMARY.md`: Phase 4 (Feedback Collection) details
  - `PHASE5_COMPLETE_SUMMARY.md`: Phase 5 (Version Control) details
  - `PHASE6_COMPLETE_SUMMARY.md`: Phase 6 (Integration Layer) details

- **Architecture**:
  - This document: Complete system architecture
  - `ARCHITECTURE.md`: Overall ThemisDB architecture
  - `LLAMA_CPP_INTEGRATION_SUMMARY.md`: LLM integration details

- **API Reference**:
  - Header files in `include/prompt_engineering/`
  - Inline documentation in source code

## Support and Contributing

- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Documentation**: https://makr-code.github.io/ThemisDB/
- **Contributing**: See `CONTRIBUTING.md`

---

**Last Updated**: 2026-04-06  
**Status**: All 6 Phases Complete ✅  
**Version**: 2.0.0 (Production Ready)
