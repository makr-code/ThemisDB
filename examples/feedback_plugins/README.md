> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Feedback Validation Plugin Examples

This directory contains example implementations of feedback validation plugins for ThemisDB's LoRA continuous learning system.

## Overview

The feedback system supports **optional** validation through plugins. Plugins can:
- Accept/reject/flag feedback
- Detect spam, PII, or low-quality content
- Add custom analytics data
- Modify feedback before storage

## Example Implementations

### 1. Python Script Plugin (`feedback_validator.py`)

A standalone Python script that demonstrates:
- Spam keyword detection
- PII detection (email, phone, SSN, credit card)
- Quality scoring
- Plugin statistics

**Usage:**

```bash
# Validate feedback (reads JSON from stdin)
echo '{"question": "test", "answer": "test"}' | python3 feedback_validator.py validate

# Get statistics
python3 feedback_validator.py stats
```

**Configuration** (optional file: `feedback_validator_config.json`):

```json
{
  "spam_keywords": [
    "buy now",
    "click here",
    "casino"
  ]
}
```

### 2. C++ Plugin (built-in)

Built-in C++ plugins:
- `NoOpFeedbackPlugin` - No validation
- `BasicSpamDetectionPlugin` - Simple keyword matching

**Usage in C++:**

```cpp
#include "llm/i_feedback_plugin.h"
#include "llm/feedback_store.h"

// Create plugin
auto plugin = std::make_shared<themis::llm::BasicSpamDetectionPlugin>();

// Configure
nlohmann::json config = {
    {"spam_keywords", {"spam1", "spam2", "spam3"}}
};
plugin->initialize(config);

// Set plugin on feedback store
feedback_store->setValidationPlugin(plugin);

// All feedback will now be validated
auto feedback = /* create feedback */;
auto stored = feedback_store->createFeedback(feedback);
```

## Creating Custom Plugins

### Python/Script-Based Plugins

1. Copy `feedback_validator.py` as a template
2. Modify the `validate()` method with your logic
3. Deploy the script alongside ThemisDB
4. Configure ThemisDB to call your script

**Script Interface:**

**Input (stdin):**
```json
{
  "question": "User question",
  "answer": "Model answer",
  "correction": "User correction (optional)",
  "comment": "User comment (optional)",
  "user_id": "user123",
  "adapter_id": "themis_help_lora_v2",
  "is_positive": true,
  "metadata": {}
}
```

**Output (stdout):**
```json
{
  "result": "accept|reject|flag|modify",
  "reason": "Optional reason",
  "confidence": 0.95,
  "plugin_data": {
    "quality_score": 0.8,
    "custom_field": "value"
  }
}
```

### C++ Plugins

1. Create a class that implements `themis::llm::IFeedbackPlugin`
2. Implement required methods:
   - `getName()`, `getVersion()`, `getDescription()`
   - `initialize(config)`
   - `validate(feedback)`
   - `shutdown()`
3. Compile into a shared library or link statically
4. Register plugin with FeedbackStore

**Example:**

```cpp
class MyCustomPlugin : public themis::llm::IFeedbackPlugin {
public:
    std::string getName() const override {
        return "my_custom_plugin";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "My custom validation logic";
    }
    
    bool initialize(const nlohmann::json& config) override {
        // Load configuration, models, etc.
        return true;
    }
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        ValidationResponse response;
        
        // Your validation logic here
        if (/* some condition */) {
            response.result = FeedbackValidationResult::REJECT;
            response.reason = "Failed validation";
        } else {
            response.result = FeedbackValidationResult::ACCEPT;
        }
        
        response.confidence_score = 0.95f;
        return response;
    }
    
    void shutdown() override {
        // Cleanup
    }
};
```

## Testing Plugins

### Python Script

```bash
# Test with sample feedback
cat > test_feedback.json <<EOF
{
  "question": "How do I enable sharding?",
  "answer": "Use SHARD BY clause",
  "is_positive": true,
  "user_id": "test_user",
  "adapter_id": "test_adapter"
}
EOF

python3 feedback_validator.py validate < test_feedback.json
```

### C++ Plugin

```cpp
TEST(MyPluginTest, BasicValidation) {
    auto plugin = std::make_shared<MyCustomPlugin>();
    
    nlohmann::json config = {/* config */};
    ASSERT_TRUE(plugin->initialize(config));
    
    themis::llm::FeedbackData feedback;
    feedback.question = "test";
    feedback.answer = "test";
    
    auto result = plugin->validate(feedback);
    EXPECT_EQ(result.result, 
              themis::llm::FeedbackValidationResult::ACCEPT);
    
    plugin->shutdown();
}
```

## Use Cases

### 1. Spam Detection

Detect and reject spam feedback using ML models or keyword matching.

### 2. PII Detection

Detect and redact personally identifiable information (PII) such as:
- Email addresses
- Phone numbers
- Social Security Numbers
- Credit card numbers

### 3. Quality Scoring

Assign quality scores to feedback based on:
- Length and completeness
- Grammar and spelling
- Relevance to the question
- User history

### 4. Content Moderation

Filter inappropriate content:
- Profanity
- Hate speech
- Violent content
- NSFW material

### 5. Business Rules

Implement custom business logic:
- Only accept feedback from verified users
- Require minimum quality threshold
- Flag feedback for specific adapters
- Apply different rules by user role

## Performance Considerations

- **Keep validation fast**: Validation runs synchronously during feedback submission
- **Cache ML models**: Load models once in `initialize()`, not per validation
- **Use async for expensive checks**: Consider post-storage hooks for heavy processing
- **Profile your plugin**: Measure validation time and optimize bottlenecks

## Best Practices

1. **Accept by default**: Only reject when confident
2. **Provide clear reasons**: Help users understand rejections
3. **Use confidence scores**: Express uncertainty in your decisions
4. **Log important events**: Use logging for debugging and monitoring
5. **Handle errors gracefully**: Don't crash on unexpected input
6. **Test thoroughly**: Test with edge cases and malformed input

## See Also

- [Feedback Plugin Interface Documentation](../../docs/FEEDBACK_PLUGIN_INTERFACE.md)
- [Feedback API Documentation](../../docs/FEEDBACK_API.md)
- [Plugin Interface Header](../../include/llm/i_feedback_plugin.h)
- [LoRA Graph Documentation](../../docs/LORA_GRAPH.md)
