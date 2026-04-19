# Feedback Plugin Interface Documentation

## Overview

The Feedback Plugin Interface enables **optional** custom validation, preprocessing, and filtering logic for user feedback in the LoRA continuous learning system.

This simplified design:
- Stores all feedback in the relational collection (`help_feedback`)
- Links feedback to LoRA adapters via graph edges (`FEEDBACK_FOR`)
- Makes validation optional through plugins
- Separates concerns between storage and validation

## Key Concepts

### 1. Storage is Primary

All feedback is stored in RocksDB with the key prefix `help_feedback:{id}`. The storage layer is simple and does not enforce complex validation.

### 2. Validation is Optional

Validation can be added via plugins without modifying core code. Plugins can:
- Accept/reject/flag feedback
- Modify metadata or comments (e.g., redact PII)
- Add plugin-specific analytics data
- Trigger workflows or notifications

### 3. Graph Links Enable Queries

Feedback entries are linked to LoRA adapters via `FEEDBACK_FOR` graph edges, enabling:
- Query feedback by adapter: "Get all feedback for adapter X"
- Track adapter lineage: "Which adapters were trained on this feedback?"
- Multi-model analysis: "Which feedback applies to multiple adapters?"

## Plugin Interface

### Core Interface: `IFeedbackPlugin`

```cpp
class IFeedbackPlugin {
public:
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    
    virtual bool initialize(const json& config) = 0;
    virtual ValidationResponse validate(const FeedbackData& feedback) = 0;
    virtual void onFeedbackStored(const std::string& feedback_id, 
                                   const FeedbackData& feedback);
    virtual void shutdown() = 0;
    virtual json getStatistics() const;
};
```

### Validation Results

```cpp
enum class FeedbackValidationResult {
    ACCEPT,     // Accept feedback as-is
    REJECT,     // Reject feedback (spam, invalid)
    FLAG,       // Flag for manual review
    MODIFY      // Accept but with modifications
};
```

## Built-in Plugins

### 1. NoOpFeedbackPlugin

Accepts all feedback without validation. Use when:
- You don't need validation
- Testing feedback storage
- You have external validation

```cpp
auto plugin = std::make_shared<NoOpFeedbackPlugin>();
feedback_store.setValidationPlugin(plugin);
```

### 2. BasicSpamDetectionPlugin

Simple keyword-based spam detection. Use for:
- Basic spam filtering
- Low-complexity environments
- Template for custom plugins

**Configuration:**
```json
{
  "spam_keywords": [
    "buy now",
    "click here",
    "casino",
    "lottery"
  ]
}
```

**Usage:**
```cpp
auto plugin = std::make_shared<BasicSpamDetectionPlugin>();
json config = {
    {"spam_keywords", {"spam1", "spam2", "spam3"}}
};
plugin->initialize(config);
feedback_store.setValidationPlugin(plugin);
```

## Creating Custom Plugins

### Example 1: PII Detection Plugin

```cpp
class PIIDetectionPlugin : public IFeedbackPlugin {
public:
    std::string getName() const override {
        return "pii_detection";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "Detects and redacts PII in feedback";
    }
    
    bool initialize(const json& config) override {
        // Load PII patterns (emails, phone numbers, SSN, etc.)
        email_pattern_ = std::regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        phone_pattern_ = std::regex(R"(\d{3}-\d{3}-\d{4})");
        return true;
    }
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        ValidationResponse response;
        response.result = FeedbackValidationResult::ACCEPT;
        
        // Check for PII
        bool has_pii = false;
        if (std::regex_search(feedback.question, email_pattern_) ||
            std::regex_search(feedback.question, phone_pattern_)) {
            has_pii = true;
        }
        
        if (has_pii) {
            // Option 1: Reject
            response.result = FeedbackValidationResult::REJECT;
            response.reason = "Contains PII";
            
            // Option 2: Redact (MODIFY)
            // response.result = FeedbackValidationResult::MODIFY;
            // response.modified_comment = redactPII(feedback.comment);
        }
        
        return response;
    }
    
    void shutdown() override {
        // Cleanup
    }
    
private:
    std::regex email_pattern_;
    std::regex phone_pattern_;
};
```

### Example 2: Quality Scoring Plugin

```cpp
class QualityScoringPlugin : public IFeedbackPlugin {
public:
    std::string getName() const override {
        return "quality_scoring";
    }
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        ValidationResponse response;
        float quality_score = calculateQualityScore(feedback);
        
        if (quality_score < 0.3f) {
            response.result = FeedbackValidationResult::REJECT;
            response.reason = "Low quality score";
        } else if (quality_score < 0.6f) {
            response.result = FeedbackValidationResult::FLAG;
            response.reason = "Medium quality - needs review";
        } else {
            response.result = FeedbackValidationResult::ACCEPT;
        }
        
        // Add quality score to metadata
        response.plugin_data["quality_score"] = quality_score;
        response.confidence_score = quality_score;
        
        return response;
    }
    
private:
    float calculateQualityScore(const FeedbackData& feedback) {
        float score = 1.0f;
        
        // Length checks
        if (feedback.question.length() < 10) score -= 0.2f;
        if (feedback.answer.length() < 10) score -= 0.2f;
        
        // Completeness
        if (!feedback.is_positive && feedback.correction.empty()) {
            score -= 0.3f;
        }
        
        // TODO: Add ML-based quality scoring
        
        return std::max(0.0f, score);
    }
};
```

### Example 3: ML-Based Spam Detection Plugin

```cpp
class MLSpamDetectionPlugin : public IFeedbackPlugin {
public:
    bool initialize(const json& config) override {
        // Load ML model (e.g., TensorFlow, PyTorch, ONNX)
        model_path_ = config.value("model_path", "models/spam_detector.onnx");
        // Load model...
        return true;
    }
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        // Run ML inference
        float spam_probability = runModel(feedback);
        
        ValidationResponse response;
        if (spam_probability > 0.9f) {
            response.result = FeedbackValidationResult::REJECT;
            response.confidence_score = spam_probability;
        } else if (spam_probability > 0.7f) {
            response.result = FeedbackValidationResult::FLAG;
            response.confidence_score = spam_probability;
        } else {
            response.result = FeedbackValidationResult::ACCEPT;
            response.confidence_score = 1.0f - spam_probability;
        }
        
        response.plugin_data["spam_probability"] = spam_probability;
        return response;
    }
    
private:
    std::string model_path_;
    // ML model handle...
    
    float runModel(const FeedbackData& feedback) {
        // Preprocess text
        // Run inference
        // Return probability
        return 0.0f; // Placeholder
    }
};
```

## Graph Links

### Creating Links

When feedback is submitted, create a graph link to the adapter:

```cpp
// Submit feedback
FeedbackStore::FeedbackEntry feedback;
feedback.type = FeedbackType::POSITIVE;
feedback.question = "How do I use sharding?";
feedback.answer = "Use SHARD BY clause...";
feedback.adapter_id = "themis_help_lora_v2";

auto stored = feedback_store.createFeedback(feedback);

// Create graph link
json link_metadata = {
    {"created_by", "user123"},
    {"session_id", "abc-123"},
    {"confidence", 0.95}
};
feedback_store.createAdapterLink(stored.id, "themis_help_lora_v2", link_metadata);
```

### Querying Feedback by Adapter

```cpp
// Get all feedback for a specific adapter
auto feedback_list = feedback_store.getFeedbackForAdapter(
    "themis_help_lora_v2",
    FeedbackStore::ListOptions{
        .limit = 100,
        .filter_status = ValidationStatus::APPROVED,
        .unused_for_training = true
    }
);

// Get adapters linked to feedback
auto adapters = feedback_store.getLinkedAdapters(feedback_id);

// Check if feedback is linked to adapter
bool is_linked = feedback_store.isLinkedToAdapter(feedback_id, adapter_id);
```

## AQL Query Examples

### Query 1: Get Feedback for Adapter

```aql
// Get all approved feedback for an adapter
MATCH (f:Feedback)-[r:FEEDBACK_FOR]->(a:Adapter {id: 'themis_help_lora_v2'})
WHERE f.validation_status = 'approved' 
  AND f.used_for_training = false
RETURN f
LIMIT 100
```

### Query 2: Adapter Feedback Stats

```aql
// Get feedback statistics by adapter
MATCH (f:Feedback)-[r:FEEDBACK_FOR]->(a:Adapter)
RETURN a.id, 
       COUNT(f) AS total_feedback,
       SUM(CASE WHEN f.type = 'positive' THEN 1 ELSE 0 END) AS positive_count,
       SUM(CASE WHEN f.type = 'negative' THEN 1 ELSE 0 END) AS negative_count
GROUP BY a.id
```

### Query 3: Feedback Lineage

```aql
// Get feedback that influenced an adapter through its lineage
MATCH path = (f:Feedback)-[:FEEDBACK_FOR]->(a1:Adapter)-[:DERIVED_FROM*]->(a2:Adapter)
WHERE a2.id = 'base_model'
RETURN path
```

## Integration Workflow

### 1. Basic Setup (No Validation)

```cpp
// Create feedback store without plugin
auto feedback_store = std::make_shared<FeedbackStore>(db, cf);

// Submit feedback - no validation
FeedbackStore::FeedbackEntry feedback;
// ... set fields ...
auto stored = feedback_store->createFeedback(feedback);
```

### 2. With Built-in Plugin

```cpp
// Create feedback store
auto feedback_store = std::make_shared<FeedbackStore>(db, cf);

// Add spam detection plugin
auto plugin = std::make_shared<BasicSpamDetectionPlugin>();
json config = {{"spam_keywords", {"spam1", "spam2"}}};
plugin->initialize(config);
feedback_store->setValidationPlugin(plugin);

// Submit feedback - validated by plugin
auto stored = feedback_store->createFeedback(feedback);
```

### 3. With Custom Plugin

```cpp
// Create custom plugin
auto plugin = std::make_shared<MyCustomPlugin>();
plugin->initialize(my_config);
feedback_store->setValidationPlugin(plugin);

// Plugin is now active for all feedback
```

## Best Practices

### 1. Plugin Design

- **Keep plugins focused**: One plugin, one responsibility
- **Handle errors gracefully**: Don't crash on invalid input
- **Log important events**: Use THEMIS_DEBUG, THEMIS_INFO, etc.
- **Return meaningful reasons**: Help users understand rejections
- **Be fast**: Validation runs synchronously

### 2. Validation Logic

- **Accept by default**: Only reject/flag when confident
- **Use confidence scores**: Express uncertainty
- **Provide reasons**: Always explain rejections
- **Consider false positives**: Flagging is safer than rejecting

### 3. Graph Links

- **Create links immediately**: After feedback is stored
- **Add metadata**: Include context (user, session, confidence)
- **Use for training**: Query by adapter to get training data
- **Track usage**: Mark feedback as used after training

### 4. Performance

- **Lazy validation**: Only validate on create, not on read
- **Cache models**: Load ML models once in initialize()
- **Use batch processing**: For post-storage analytics
- **Index efficiently**: RocksDB prefix scans are fast

## Plugin Lifecycle

```
1. Initialize
   └─> Load config, models, resources
   
2. Validate (per feedback)
   └─> Check feedback, return result
   
3. OnFeedbackStored (per feedback)
   └─> Optional post-storage hook
   
4. GetStatistics (on demand)
   └─> Return plugin metrics
   
5. Shutdown
   └─> Cleanup resources
```

## Testing Plugins

### Unit Test Template

```cpp
TEST(MyPluginTest, BasicValidation) {
    auto plugin = std::make_shared<MyPlugin>();
    json config = {/* ... */};
    ASSERT_TRUE(plugin->initialize(config));
    
    FeedbackData feedback;
    feedback.question = "test";
    feedback.answer = "test";
    
    auto result = plugin->validate(feedback);
    EXPECT_EQ(result.result, FeedbackValidationResult::ACCEPT);
    
    plugin->shutdown();
}
```

## Troubleshooting

### Plugin not active?

```cpp
// Check if plugin is set
auto current_plugin = feedback_store->getValidationPlugin();
if (!current_plugin) {
    THEMIS_WARN("No validation plugin set");
}
```

### Plugin rejecting too much?

- Lower thresholds in configuration
- Add logging to see rejection reasons
- Use FLAG instead of REJECT for borderline cases

### Performance issues?

- Profile plugin validation time
- Consider async validation for expensive checks
- Cache expensive computations

## See Also

- [Feedback API Documentation](FEEDBACK_API.md)
- [LoRA Graph Structure](../include/llm/lora_framework/lora_graph.h)
- [Plugin Interface](../include/llm/i_feedback_plugin.h)
