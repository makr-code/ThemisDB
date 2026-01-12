# Plugin Developer Guide - LoRA Feedback System

## Overview

The LoRA Feedback System provides a plugin architecture for extending and customizing feedback validation, processing, and training triggers. This guide explains how to create custom plugins.

## Plugin Interface

### Base Interface: `FeedbackPlugin`

Located in: `include/llm/lora_framework/feedback_plugin.h`

```cpp
class FeedbackPlugin {
public:
    virtual ~FeedbackPlugin() = default;
    
    // Validate feedback before storage
    virtual bool validate(const Feedback& feedback) const = 0;
    
    // Process feedback after validation, before storage
    virtual void process(Feedback& feedback) = 0;
    
    // Determine if training should be triggered
    virtual bool onTrainingTrigger(const std::vector<Feedback>& batch) const = 0;
    
    // Get plugin name for logging
    virtual std::string getName() const = 0;
};
```

## Plugin Lifecycle

1. **Registration**: Plugin is registered with `FeedbackStorageService`
2. **Validation**: Called for each new feedback entry
3. **Processing**: Called if validation passes
4. **Storage**: Feedback is stored in database
5. **Training Check**: Periodically called to check if training should trigger

```
User submits feedback
        ↓
    Validation (all plugins)
        ↓ (pass)
    Processing (all plugins)
        ↓
    Storage in database
        ↓
    Training check (periodic)
```

## Built-in Plugins

### 1. BaseFeedbackPlugin

Provides sensible defaults for all interface methods.

```cpp
class BaseFeedbackPlugin : public FeedbackPlugin {
public:
    bool validate(const Feedback& feedback) const override {
        // Basic validation
        if (feedback.adapter_id.empty()) return false;
        if (feedback.user_id.empty()) return false;
        if (feedback.rating < 1 || feedback.rating > 5) return false;
        return true;
    }
    
    void process(Feedback& feedback) override {
        // Default: no-op
    }
    
    bool onTrainingTrigger(const std::vector<Feedback>& batch) const override {
        // Default: trigger at 100 items
        return batch.size() >= 100;
    }
    
    std::string getName() const override {
        return "BaseFeedbackPlugin";
    }
};
```

### 2. PrivacyFilterPlugin

Removes PII from feedback text.

```cpp
class PrivacyFilterPlugin : public BaseFeedbackPlugin {
public:
    void process(Feedback& feedback) override {
        // Remove emails
        feedback.feedback_text = std::regex_replace(
            feedback.feedback_text,
            std::regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"),
            "[EMAIL]"
        );
        
        // Remove phone numbers
        feedback.feedback_text = std::regex_replace(
            feedback.feedback_text,
            std::regex(R"(\b\d{3}[-.]?\d{3}[-.]?\d{4}\b)"),
            "[PHONE]"
        );
    }
    
    std::string getName() const override {
        return "PrivacyFilterPlugin";
    }
};
```

### 3. ContentValidationPlugin

Validates feedback content for spam and quality.

```cpp
class ContentValidationPlugin : public BaseFeedbackPlugin {
public:
    bool validate(const Feedback& feedback) const override {
        if (!BaseFeedbackPlugin::validate(feedback)) {
            return false;
        }
        
        // Check for spam
        if (containsSpam(feedback.feedback_text)) {
            return false;
        }
        
        // Check length
        if (!feedback.feedback_text.empty() && 
            feedback.feedback_text.length() < 3) {
            return false;
        }
        
        return true;
    }
    
    std::string getName() const override {
        return "ContentValidationPlugin";
    }
};
```

### 4. TrainingTriggerPlugin

Advanced training trigger logic with configurable thresholds.

```cpp
class TrainingTriggerPlugin : public BaseFeedbackPlugin {
public:
    struct Config {
        size_t min_batch_size = 50;
        size_t max_batch_size = 200;
        float min_avg_rating = 3.0f;
        std::chrono::hours max_wait_time{24};
    };
    
    explicit TrainingTriggerPlugin(const Config& config = Config{})
        : config_(config) {}
    
    bool onTrainingTrigger(const std::vector<Feedback>& batch) const override {
        if (batch.size() < config_.min_batch_size) return false;
        if (batch.size() >= config_.max_batch_size) return true;
        
        // Check average rating
        float avg_rating = calculateAverageRating(batch);
        if (avg_rating < config_.min_avg_rating) {
            return batch.size() >= config_.min_batch_size;
        }
        
        // Check time
        if (!batch.empty()) {
            auto age = std::chrono::system_clock::now() - batch.front().timestamp;
            if (age >= config_.max_wait_time) return true;
        }
        
        return false;
    }
};
```

## Creating Custom Plugins

### Example 1: Sentiment Analysis Plugin

Add sentiment scores to feedback metadata.

```cpp
#include "llm/lora_framework/feedback_plugin.h"
#include "nlp/sentiment_analyzer.h"

class SentimentAnalysisPlugin : public BaseFeedbackPlugin {
public:
    explicit SentimentAnalysisPlugin(
        std::shared_ptr<SentimentAnalyzer> analyzer
    ) : analyzer_(analyzer) {}
    
    void process(Feedback& feedback) override {
        if (feedback.feedback_text.empty()) {
            return;
        }
        
        // Analyze sentiment
        auto sentiment = analyzer_->analyze(feedback.feedback_text);
        
        // Add to metadata
        feedback.custom_metadata["sentiment"] = {
            {"score", sentiment.score},      // -1.0 to 1.0
            {"label", sentiment.label},      // "positive", "negative", "neutral"
            {"confidence", sentiment.confidence}
        };
        
        // Auto-categorize for training
        if (sentiment.score > 0.5 && sentiment.confidence > 0.8) {
            feedback.training_category = "positive";
            feedback.flagged_for_training = true;
        } else if (sentiment.score < -0.5 && sentiment.confidence > 0.8) {
            feedback.training_category = "negative";
            feedback.flagged_for_training = true;
        }
    }
    
    std::string getName() const override {
        return "SentimentAnalysisPlugin";
    }

private:
    std::shared_ptr<SentimentAnalyzer> analyzer_;
};
```

### Example 2: Language Detection Plugin

Categorize feedback by language.

```cpp
#include "llm/lora_framework/feedback_plugin.h"
#include "nlp/language_detector.h"

class LanguageDetectionPlugin : public BaseFeedbackPlugin {
public:
    void process(Feedback& feedback) override {
        if (feedback.feedback_text.empty()) {
            return;
        }
        
        auto lang = detectLanguage(feedback.feedback_text);
        
        feedback.custom_metadata["language"] = lang.code;  // "en", "de", "fr", etc.
        feedback.custom_metadata["language_confidence"] = lang.confidence;
        
        // Filter non-English feedback for English-only adapters
        if (requiresEnglish(feedback.adapter_id) && lang.code != "en") {
            // Mark as low priority for training
            feedback.flagged_for_training = false;
        }
    }
    
    std::string getName() const override {
        return "LanguageDetectionPlugin";
    }

private:
    struct LanguageResult {
        std::string code;
        float confidence;
    };
    
    LanguageResult detectLanguage(const std::string& text) {
        // Implement or use library like langdetect
        // Simplified example:
        return {"en", 0.95f};
    }
    
    bool requiresEnglish(const std::string& adapter_id) {
        // Check adapter requirements
        return adapter_id.find("_en") != std::string::npos;
    }
};
```

### Example 3: Quality Score Plugin

Calculate quality scores based on multiple factors.

```cpp
class QualityScorePlugin : public BaseFeedbackPlugin {
public:
    struct QualityMetrics {
        float completeness;     // Has all fields filled
        float specificity;      // Detailed vs vague
        float constructiveness; // Actionable feedback
        float overall;          // Combined score
    };
    
    void process(Feedback& feedback) override {
        auto metrics = calculateQuality(feedback);
        
        feedback.custom_metadata["quality_score"] = {
            {"completeness", metrics.completeness},
            {"specificity", metrics.specificity},
            {"constructiveness", metrics.constructiveness},
            {"overall", metrics.overall}
        };
        
        // Only flag high-quality feedback for training
        if (metrics.overall >= 0.7f) {
            feedback.flagged_for_training = true;
        }
    }
    
    std::string getName() const override {
        return "QualityScorePlugin";
    }

private:
    QualityMetrics calculateQuality(const Feedback& feedback) {
        QualityMetrics metrics;
        
        // Completeness: Check if fields are filled
        int filled = 0;
        if (!feedback.feedback_text.empty()) filled++;
        if (!feedback.prompt.empty()) filled++;
        if (!feedback.response.empty()) filled++;
        if (feedback.rating > 0) filled++;
        metrics.completeness = filled / 4.0f;
        
        // Specificity: Length and detail
        float text_length = feedback.feedback_text.length();
        metrics.specificity = std::min(text_length / 200.0f, 1.0f);
        
        // Constructiveness: Contains actionable words
        std::vector<std::string> constructive_words = {
            "should", "could", "improve", "better", "suggest",
            "recommend", "consider", "would", "help"
        };
        int constructive_count = 0;
        for (const auto& word : constructive_words) {
            if (feedback.feedback_text.find(word) != std::string::npos) {
                constructive_count++;
            }
        }
        metrics.constructiveness = std::min(constructive_count / 3.0f, 1.0f);
        
        // Overall: Weighted average
        metrics.overall = (metrics.completeness * 0.3f +
                          metrics.specificity * 0.3f +
                          metrics.constructiveness * 0.4f);
        
        return metrics;
    }
};
```

### Example 4: Anomaly Detection Plugin

Detect and filter suspicious feedback patterns.

```cpp
class AnomalyDetectionPlugin : public BaseFeedbackPlugin {
public:
    bool validate(const Feedback& feedback) const override {
        if (!BaseFeedbackPlugin::validate(feedback)) {
            return false;
        }
        
        // Check for duplicate submissions
        if (isDuplicate(feedback)) {
            spdlog::warn("Duplicate feedback detected from user: {}", 
                        feedback.user_id);
            return false;
        }
        
        // Check for bot-like behavior
        if (isBotLike(feedback)) {
            spdlog::warn("Bot-like behavior detected from user: {}", 
                        feedback.user_id);
            return false;
        }
        
        // Check for rating manipulation
        if (isRatingManipulation(feedback)) {
            spdlog::warn("Rating manipulation detected from user: {}", 
                        feedback.user_id);
            return false;
        }
        
        return true;
    }
    
    std::string getName() const override {
        return "AnomalyDetectionPlugin";
    }

private:
    bool isDuplicate(const Feedback& feedback) const {
        // Check recent submissions from same user
        // Implementation depends on access to storage
        return false;
    }
    
    bool isBotLike(const Feedback& feedback) const {
        // Check submission rate
        // Check for template-like responses
        // Simple heuristic: very short submission time
        return false;
    }
    
    bool isRatingManipulation(const Feedback& feedback) const {
        // Check for patterns of extreme ratings
        // Check for coordinated reviews
        return false;
    }
};
```

## Plugin Registration

### Method 1: Direct Registration

```cpp
// Create storage service
FeedbackStorageService::Config config;
config.db = db;
auto storage = std::make_shared<FeedbackStorageService>(config);

// Register plugins
storage->registerPlugin(std::make_shared<BaseFeedbackPlugin>());
storage->registerPlugin(std::make_shared<PrivacyFilterPlugin>());
storage->registerPlugin(std::make_shared<ContentValidationPlugin>());

// Register custom plugin
auto sentiment_analyzer = std::make_shared<SentimentAnalyzer>();
storage->registerPlugin(
    std::make_shared<SentimentAnalysisPlugin>(sentiment_analyzer)
);
```

### Method 2: Configuration-Based Registration

```cpp
class PluginFactory {
public:
    static std::shared_ptr<FeedbackPlugin> create(const json& config) {
        std::string type = config["type"];
        
        if (type == "base") {
            return std::make_shared<BaseFeedbackPlugin>();
        } else if (type == "privacy") {
            return std::make_shared<PrivacyFilterPlugin>();
        } else if (type == "content_validation") {
            return std::make_shared<ContentValidationPlugin>();
        } else if (type == "sentiment") {
            auto analyzer = std::make_shared<SentimentAnalyzer>();
            return std::make_shared<SentimentAnalysisPlugin>(analyzer);
        }
        
        throw std::runtime_error("Unknown plugin type: " + type);
    }
};

// Load from config file
json config = loadConfig("feedback_plugins.json");
for (const auto& plugin_config : config["plugins"]) {
    auto plugin = PluginFactory::create(plugin_config);
    storage->registerPlugin(plugin);
}
```

## Testing Plugins

### Unit Test Template

```cpp
TEST(CustomPluginTest, ValidateFeedback) {
    MyCustomPlugin plugin;
    
    // Valid feedback
    Feedback valid_fb;
    valid_fb.adapter_id = "test";
    valid_fb.user_id = "user123";
    valid_fb.rating = 5;
    EXPECT_TRUE(plugin.validate(valid_fb));
    
    // Invalid feedback
    Feedback invalid_fb;
    EXPECT_FALSE(plugin.validate(invalid_fb));
}

TEST(CustomPluginTest, ProcessFeedback) {
    MyCustomPlugin plugin;
    
    Feedback feedback;
    feedback.feedback_text = "Original text";
    
    plugin.process(feedback);
    
    EXPECT_NE(feedback.feedback_text, "Original text");
    EXPECT_TRUE(feedback.custom_metadata.contains("processed"));
}
```

## Best Practices

### 1. Single Responsibility

Each plugin should have one clear purpose:

```cpp
// Good: Focused on one task
class EmailObfuscationPlugin : public BaseFeedbackPlugin {
    void process(Feedback& feedback) override {
        obfuscateEmails(feedback.feedback_text);
    }
};

// Bad: Too many responsibilities
class AllInOnePlugin : public BaseFeedbackPlugin {
    void process(Feedback& feedback) override {
        obfuscateEmails(feedback.feedback_text);
        detectLanguage(feedback.feedback_text);
        analyzeSentiment(feedback.feedback_text);
        calculateQuality(feedback.feedback_text);
        // ... too much
    }
};
```

### 2. Fail Gracefully

Handle errors without breaking the pipeline:

```cpp
void process(Feedback& feedback) override {
    try {
        // Processing logic
        auto result = expensiveOperation(feedback.feedback_text);
        feedback.custom_metadata["result"] = result;
    } catch (const std::exception& e) {
        // Log error but don't throw
        spdlog::error("{} failed: {}", getName(), e.what());
        // Continue processing
    }
}
```

### 3. Performance Considerations

Avoid expensive operations in the validation phase:

```cpp
// Good: Quick validation
bool validate(const Feedback& feedback) const override {
    return !feedback.feedback_text.empty() &&
           feedback.feedback_text.length() <= MAX_LENGTH;
}

// Bad: Expensive validation
bool validate(const Feedback& feedback) const override {
    // Don't do heavy processing here
    auto sentiment = runComplexNLPModel(feedback.feedback_text);
    return sentiment.score > 0.5;
}
```

### 4. Configuration

Make plugins configurable:

```cpp
class ConfigurablePlugin : public BaseFeedbackPlugin {
public:
    struct Config {
        bool enable_feature_a = true;
        float threshold = 0.5f;
        std::string model_path = "models/default.bin";
    };
    
    explicit ConfigurablePlugin(const Config& config = Config{})
        : config_(config) {}
    
private:
    Config config_;
};
```

## Next Steps

- See [API Documentation](./LORA_FEEDBACK_API.md) for REST API usage
- See [Graph Query Examples](./GRAPH_QUERY_EXAMPLES.md) for querying feedback
- See [ThemisHelp LoRA Integration](./THEMIS_HELP_LORA_INTEGRATION.md) for complete examples
