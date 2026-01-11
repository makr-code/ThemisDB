# LoRA Integration with HELP() Function

## Overview

This document describes the integration of the `themis_help_lora` adapter with the `HELP()` function in ThemisDB's AQL documentation assistant.

## Motivation

The HELP() function previously used only the base LLM for generating documentation assistance. By integrating the LoRA framework, we can:

1. **Improve Accuracy** - Fine-tuned adapter specifically trained on ThemisDB documentation
2. **Reduce Hallucinations** - Context-specific knowledge reduces incorrect information
3. **Enable Learning** - System can learn from user corrections and feedback
4. **Maintain Performance** - Graceful fallback to base LLM when adapter unavailable

## Architecture

### Components

```
┌─────────────────────────────────────────────────────────────┐
│                    HELP() Function                          │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Intent Detection (3-tier)                           │  │
│  │  1. Native NLP (CLASSIFY)                            │  │
│  │  2. LLM-based classification                         │  │
│  │  3. Regex fallback                                   │  │
│  └──────────────────────────────────────────────────────┘  │
│                          ↓                                  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  LoRA Integration Layer                              │  │
│  │                                                       │  │
│  │  ┌─────────────────┐      ┌────────────────────┐    │  │
│  │  │ ThemisHelpLoRA  │──┐   │ DocsAssistant      │    │  │
│  │  │ (if available)  │  │   │ (base fallback)    │    │  │
│  │  └─────────────────┘  │   └────────────────────┘    │  │
│  │         ↓              │            ↓                │  │
│  │    LoRA Adapter        └──→  Fallback if needed     │  │
│  └──────────────────────────────────────────────────────┘  │
│                          ↓                                  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Response Generation                                 │  │
│  │  - Performance tracking                              │  │
│  │  - Logging (LoRA vs base)                            │  │
│  │  - Metrics collection                                │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Integration Points

#### 1. DocsAssistantFunctions Class

**Location**: `include/aql/docs_assistant_functions.h`, `src/aql/docs_assistant_functions.cpp`

**Changes**:
- Added `ThemisHelpLoRA` member to implementation class
- Modified `help()` to try LoRA first, then fallback to base
- Added `isLoRAActive()` to check adapter availability
- Added `getPerformanceMetrics()` to track LoRA vs base performance

**Key Methods**:

```cpp
// Updated signature with user_id for tracking
std::string help(const std::string& query, const std::string& user_id = "anonymous");

// New methods
bool isLoRAActive() const;
json getPerformanceMetrics() const;
```

#### 2. ThemisHelpLoRA Class

**Location**: `include/llm/applications/themis_help_lora.h`, `src/llm/applications/themis_help_lora.cpp`

**Changes**:
- Fixed header to match implementation signatures
- Added missing type definitions (FeedbackItem, PerformanceMetrics, FeedbackStats)
- Updated namespace usage for `lora::` framework components
- Corrected return types and method signatures

**Key Features**:
- Adapter loading/unloading
- Feedback collection (positive/negative)
- Training from feedback
- Performance metrics tracking
- Version management

## Usage

### Basic Usage

```sql
-- Automatic LoRA usage (when available)
SELECT HELP('How do I enable sharding?') AS answer;

-- With user ID for personalization and tracking
SELECT HELP('Configure security settings', 'user123') AS guide;

-- Check if LoRA is active
SELECT IS_LORA_ACTIVE() AS lora_available;

-- Get performance metrics
SELECT GET_PERFORMANCE_METRICS() AS metrics;
```

### AQL Examples

```sql
-- Configuration help (intent detected automatically)
SELECT HELP('Configure replication settings') AS config;
-- Uses LoRA if available, falls back to base LLM

-- Troubleshooting (intent detected automatically)
SELECT HELP('Server hangs at startup') AS solution;

-- Search (intent detected automatically)  
SELECT HELP('Search for RAID documentation') AS results;

-- General query (default)
SELECT HELP('What is MVCC?') AS explanation;
```

### Performance Tracking

```sql
-- Get detailed metrics
LET metrics = GET_PERFORMANCE_METRICS()
RETURN {
    lora_active: metrics.lora_active,
    lora_queries: metrics.lora.total_queries,
    lora_success_rate: metrics.lora.success_rate,
    lora_avg_latency: metrics.lora.average_latency_ms,
    feedback_stats: metrics.lora_feedback
}
```

## Behavior

### LoRA Available

When the `themis_help_lora` adapter is available and loaded:

1. **Query Processing**:
   - Query is sent to ThemisHelpLoRA
   - Adapter applies fine-tuned weights
   - Response generated with ThemisDB-specific knowledge
   - Performance metrics logged

2. **Benefits**:
   - Higher accuracy on ThemisDB-specific questions
   - Reduced hallucinations
   - Better understanding of ThemisDB terminology
   - Faster responses (cached adapter)

### LoRA Unavailable (Fallback)

When the adapter is not available or fails:

1. **Graceful Degradation**:
   - System logs fallback event
   - Switches to base DocsAssistant
   - Uses standard RAG pipeline
   - No user-visible errors

2. **Fallback Triggers**:
   - Adapter not installed
   - Adapter loading failed
   - Query processing error
   - Adapter cache miss

## Performance

### Metrics Collected

The integration tracks:

1. **Query Metrics**:
   - Total queries (LoRA vs base)
   - Success/failure rates
   - Average latency
   - Cache hit rates

2. **Feedback Metrics**:
   - Total feedback items
   - Positive vs negative ratio
   - Feedback used for training

3. **Comparison Metrics**:
   - LoRA vs base performance
   - Accuracy improvements
   - Latency differences

### Expected Improvements

Based on initial design goals:

| Metric | Base LLM | With LoRA | Improvement |
|--------|----------|-----------|-------------|
| Accuracy | 70-75% | 85-90% | +15-20% |
| Hallucination Rate | 15-20% | 5-8% | -10-15% |
| ThemisDB Terminology | Fair | Excellent | Significant |
| Response Time | Baseline | Similar | Marginal overhead |

## Error Handling

### Graceful Degradation

```cpp
// Pseudocode
if (lora_available) {
    try {
        return lora->query(question, user_id);
    } catch (LoRAError& e) {
        log_warning("LoRA failed, using base: {}", e.what());
        // Fall through to base implementation
    }
}

// Always have base as fallback
return base_assistant->query(question);
```

### Error Scenarios

1. **Adapter Not Found**: Falls back to base, logs info
2. **Adapter Load Failure**: Falls back to base, logs warning
3. **Query Processing Error**: Retries with base, logs error
4. **Out of Memory**: Unloads adapter, uses base, logs critical

## Logging

### Log Levels

**INFO**: Normal operations
```
[INFO] ThemisHelpLoRA initialized successfully
[INFO] HELP() query completed in 45ms using LoRA
```

**DEBUG**: Detailed execution flow
```
[DEBUG] Using ThemisHelpLoRA for query: How do I enable sharding?
[DEBUG] LoRA adapter loaded: themis_help_lora v1.2
```

**WARN**: Fallback scenarios
```
[WARN] LoRA query failed, falling back to base: adapter cache miss
[WARN] ThemisHelpLoRA initialization failed, using base LLM: ...
```

**ERROR**: Critical issues
```
[ERROR] LoRA adapter loading failed: file not found
```

## Testing

### Test Coverage

The integration includes comprehensive tests:

1. **LoRA Availability Tests**:
   - Check if adapter is active
   - Verify graceful handling when unavailable

2. **Query Tests**:
   - Test with LoRA adapter
   - Test fallback to base
   - Test multiple queries
   - Test all intent types

3. **Performance Tests**:
   - Metrics retrieval
   - Latency tracking
   - Cache behavior

4. **Error Handling Tests**:
   - Empty queries
   - Invalid input
   - Adapter failures

5. **Integration Tests**:
   - End-to-end workflows
   - Cache clearing
   - Performance comparison

### Running Tests

```bash
# Build tests
cmake -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build --target test_docs_assistant_aql

# Run tests
cd build
./tests/test_docs_assistant_aql

# Run specific test
./tests/test_docs_assistant_aql --gtest_filter="*LoRA*"
```

## Future Enhancements

### Short-term

1. **Feedback Integration**: UI for collecting user feedback
2. **Training Pipeline**: Automated training from feedback
3. **A/B Testing**: Compare LoRA vs base in production
4. **Metrics Dashboard**: Visualize performance metrics

### Medium-term

1. **Multi-Adapter Support**: Switch between adapters
2. **Personalization**: User-specific adapters
3. **Auto-Scaling**: Load/unload based on demand
4. **Quality Gates**: Automatic rollback on quality drop

### Long-term

1. **Continuous Learning**: Real-time adapter updates
2. **Ensemble Models**: Combine multiple adapters
3. **Cross-Language**: Adapters for different languages
4. **Domain-Specific**: Specialized adapters per domain

## Configuration

### Environment Variables

```bash
# Enable LoRA support
export THEMIS_ENABLE_LORA=true

# Specify adapter path
export THEMIS_LORA_ADAPTER_PATH=/path/to/adapters

# Set adapter ID
export THEMIS_HELP_LORA_ID=themis_help_lora

# Configure caching
export THEMIS_LORA_CACHE_SIZE=1GB
```

### Config File

```yaml
# themis.yaml
llm:
  enable_lora: true
  help_assistant:
    adapter_id: themis_help_lora
    base_model: llama-2-7b
    fallback_to_base: true
    enable_feedback: true
    cache:
      enabled: true
      size_mb: 1024
```

## Troubleshooting

### LoRA Not Loading

**Symptom**: Queries always use base model

**Solutions**:
1. Check adapter files exist: `ls /path/to/adapters/themis_help_lora`
2. Verify permissions: `chmod 644 adapter_files`
3. Check logs: `grep LoRA /var/log/themisdb/themis.log`
4. Validate configuration: `themisdb-admin config validate`

### Performance Degradation

**Symptom**: Slower responses with LoRA

**Solutions**:
1. Check cache configuration
2. Monitor memory usage
3. Consider adapter size optimization
4. Review concurrent query load

### Accuracy Issues

**Symptom**: Poor quality responses

**Solutions**:
1. Verify adapter version
2. Check training data quality
3. Collect user feedback
4. Retrain adapter
5. Consider base model fallback

## References

- [LoRA Framework Documentation](../../LORA_FRAMEWORK_ANALYSIS.md)
- [HELP() Function Design](HELP_FUNCTION_DESIGN.md)
- [ThemisDB LLM Integration](../../LLM_LORA_UNIFIED_ARCHITECTURE.md)
- [Feedback Store API](../../FEEDBACK_API.md)

---

**Date**: 2026-01-11  
**Author**: GitHub Copilot  
**Status**: Implemented  
**Version**: 1.0
