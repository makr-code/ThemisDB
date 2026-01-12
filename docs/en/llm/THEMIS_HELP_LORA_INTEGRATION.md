# ThemisHelp LoRA Integration Guide

## Overview

This guide demonstrates how to integrate the LoRA Feedback System with the ThemisHelp LoRA adapter for continuous learning from user feedback.

## Architecture

```
┌─────────────────┐
│  User Interface │
└────────┬────────┘
         │
    ┌────▼────────────────────┐
    │  ThemisHelp LoRA App    │
    │  - Query Processing     │
    │  - Response Generation  │
    └────┬────────────────┬───┘
         │                │
    ┌────▼────┐      ┌────▼─────────┐
    │  LoRA   │      │   Feedback   │
    │ Adapter │      │   System     │
    └────┬────┘      └────┬─────────┘
         │                │
    ┌────▼────────────────▼───┐
    │   Training Service      │
    │   - Collect Feedback    │
    │   - Trigger Training    │
    │   - Update Adapter      │
    └─────────────────────────┘
```

## Complete Example

### 1. Setup and Initialization

```cpp
#include "llm/applications/themis_help_lora.h"
#include "llm/lora_framework/lora_feedback_storage.h"
#include "llm/lora_framework/feedback_plugin.h"

using namespace themis::llm;
using namespace themis::llm::lora;

// Initialize components
auto db = std::make_shared<RocksDBWrapper>("data/themisdb");
auto graph_index = std::make_shared<GraphIndex>(db);

// Create feedback storage
FeedbackStorageService::Config feedback_config;
feedback_config.db = db;
feedback_config.graph_index = graph_index;
feedback_config.collection_name = "help_feedback";
feedback_config.enable_graph_links = true;

auto feedback_storage = std::make_shared<FeedbackStorageService>(feedback_config);

// Register plugins
feedback_storage->registerPlugin(std::make_shared<BaseFeedbackPlugin>());
feedback_storage->registerPlugin(std::make_shared<PrivacyFilterPlugin>());
feedback_storage->registerPlugin(std::make_shared<ContentValidationPlugin>());

auto training_config = TrainingTriggerPlugin::Config{};
training_config.min_batch_size = 50;
training_config.max_batch_size = 200;
training_config.min_avg_rating = 3.5f;
training_config.max_wait_time = std::chrono::hours{24};
feedback_storage->registerPlugin(
    std::make_shared<TrainingTriggerPlugin>(training_config)
);

// Initialize ThemisHelp LoRA
applications::ThemisHelpLoRA::Config lora_config;
lora_config.adapter_id = "themis_help_lora";
lora_config.base_model = "llama-2-7b";
lora_config.docs_database_path = "data/docs_database.json";
lora_config.feedback_batch_size = 100;

auto help_lora = std::make_shared<applications::ThemisHelpLoRA>(lora_config);
```

### 2. Query with Feedback Collection

```cpp
class ThemisHelpWithFeedback {
public:
    ThemisHelpWithFeedback(
        std::shared_ptr<applications::ThemisHelpLoRA> lora,
        std::shared_ptr<FeedbackStorageService> feedback_storage
    ) : lora_(lora), feedback_storage_(feedback_storage) {}
    
    struct QueryResult {
        std::string response;
        std::string response_id;
        std::chrono::milliseconds latency;
    };
    
    QueryResult query(const std::string& question, const std::string& user_id) {
        auto start = std::chrono::steady_clock::now();
        
        // Get response from LoRA adapter
        std::string response = lora_->query(question);
        
        auto end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        );
        
        // Generate response ID for tracking
        std::string response_id = generateResponseId();
        
        // Store interaction for future feedback
        pending_responses_[response_id] = {
            question,
            response,
            user_id,
            std::chrono::system_clock::now()
        };
        
        return {response, response_id, latency};
    }
    
    bool submitFeedback(
        const std::string& response_id,
        int rating,
        const std::string& feedback_text = ""
    ) {
        // Find pending response
        auto it = pending_responses_.find(response_id);
        if (it == pending_responses_.end()) {
            return false;
        }
        
        const auto& pending = it->second;
        
        // Create feedback
        Feedback feedback;
        feedback.adapter_id = "themis_help_lora";
        feedback.user_id = pending.user_id;
        feedback.rating = rating;
        feedback.feedback_text = feedback_text;
        feedback.prompt = pending.question;
        feedback.response = pending.response;
        feedback.model_response_id = response_id;
        
        // Categorize feedback
        if (rating >= 4) {
            feedback.flagged_for_training = true;
            feedback.training_category = "positive";
        } else if (rating <= 2) {
            feedback.flagged_for_training = true;
            feedback.training_category = "negative";
        } else {
            feedback.flagged_for_training = false;
            feedback.training_category = "neutral";
        }
        
        // Store feedback
        auto stored = feedback_storage_->createFeedback(feedback);
        
        if (stored) {
            // Clean up pending response
            pending_responses_.erase(it);
            
            // Check if training should be triggered
            if (feedback_storage_->shouldTriggerTraining("themis_help_lora")) {
                triggerTraining();
            }
            
            return true;
        }
        
        return false;
    }
    
private:
    struct PendingResponse {
        std::string question;
        std::string response;
        std::string user_id;
        std::chrono::system_clock::time_point timestamp;
    };
    
    void triggerTraining() {
        spdlog::info("Training triggered for themis_help_lora");
        
        // Get training feedback
        auto training_feedback = feedback_storage_->getTrainingFeedback(
            "themis_help_lora",
            1000
        );
        
        if (training_feedback.empty()) {
            spdlog::warn("No training feedback available");
            return;
        }
        
        // Trigger training asynchronously
        std::thread([this, training_feedback]() {
            try {
                lora_->trainFromFeedback();
                spdlog::info("Training completed successfully");
            } catch (const std::exception& e) {
                spdlog::error("Training failed: {}", e.what());
            }
        }).detach();
    }
    
    std::string generateResponseId() {
        uuid_t uuid;
        uuid_generate(uuid);
        char uuid_str[37];
        uuid_unparse_lower(uuid, uuid_str);
        return "resp_" + std::string(uuid_str);
    }
    
    std::shared_ptr<applications::ThemisHelpLoRA> lora_;
    std::shared_ptr<FeedbackStorageService> feedback_storage_;
    std::unordered_map<std::string, PendingResponse> pending_responses_;
    std::mutex mutex_;
};
```

### 3. Web API Integration

```cpp
#include "server/feedback_api_handler.h"

class ThemisHelpAPIHandler {
public:
    ThemisHelpAPIHandler(
        std::shared_ptr<ThemisHelpWithFeedback> help_with_feedback,
        std::shared_ptr<FeedbackAPIHandler> feedback_handler
    ) : help_with_feedback_(help_with_feedback),
        feedback_handler_(feedback_handler) {}
    
    // Handle query request
    http::response<http::string_body> handleQuery(
        const http::request<http::string_body>& req
    ) {
        try {
            auto body = json::parse(req.body());
            
            std::string question = body["question"];
            std::string user_id = body.value("user_id", "anonymous");
            
            // Query ThemisHelp
            auto result = help_with_feedback_->query(question, user_id);
            
            // Build response
            json response = {
                {"response", result.response},
                {"response_id", result.response_id},
                {"latency_ms", result.latency.count()},
                {"adapter_id", "themis_help_lora"}
            };
            
            return makeJsonResponse(http::status::ok, response, req);
            
        } catch (const std::exception& e) {
            return makeErrorResponse(
                http::status::internal_server_error,
                e.what(),
                req
            );
        }
    }
    
    // Handle feedback submission
    http::response<http::string_body> handleFeedback(
        const http::request<http::string_body>& req
    ) {
        return feedback_handler_->handleCreateFeedback(req);
    }

private:
    std::shared_ptr<ThemisHelpWithFeedback> help_with_feedback_;
    std::shared_ptr<FeedbackAPIHandler> feedback_handler_;
};
```

### 4. Usage Examples

#### Simple Query-Feedback Flow

```cpp
// Initialize
auto help = std::make_shared<ThemisHelpWithFeedback>(lora, feedback_storage);

// 1. User asks a question
auto result = help->query(
    "How do I create a collection in ThemisDB?",
    "user123"
);

std::cout << "Response: " << result.response << std::endl;
std::cout << "Response ID: " << result.response_id << std::endl;

// 2. User provides feedback
bool success = help->submitFeedback(
    result.response_id,
    5,  // 5-star rating
    "Very helpful and accurate!"
);
```

#### Web Interface Integration

HTML/JavaScript:

```html
<!DOCTYPE html>
<html>
<head>
    <title>ThemisHelp Assistant</title>
</head>
<body>
    <div id="chat-container">
        <div id="messages"></div>
        <input type="text" id="question" placeholder="Ask a question...">
        <button onclick="submitQuestion()">Ask</button>
    </div>
    
    <script>
        let currentResponseId = null;
        
        async function submitQuestion() {
            const question = document.getElementById('question').value;
            
            const response = await fetch('/api/themishelp/query', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${token}`
                },
                body: JSON.stringify({
                    question: question,
                    user_id: 'user123'
                })
            });
            
            const data = await response.json();
            currentResponseId = data.response_id;
            
            // Display response
            displayMessage(question, data.response, data.response_id);
        }
        
        function displayMessage(question, response, responseId) {
            const messagesDiv = document.getElementById('messages');
            
            messagesDiv.innerHTML += `
                <div class="message">
                    <div class="question">${question}</div>
                    <div class="response">${response}</div>
                    <div class="feedback">
                        <span>Was this helpful?</span>
                        ${[1, 2, 3, 4, 5].map(rating => 
                            `<button onclick="submitFeedback('${responseId}', ${rating})">
                                ${rating} ⭐
                            </button>`
                        ).join('')}
                    </div>
                </div>
            `;
        }
        
        async function submitFeedback(responseId, rating) {
            await fetch('/api/feedback', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${token}`
                },
                body: JSON.stringify({
                    adapter_id: 'themis_help_lora',
                    user_id: 'user123',
                    rating: rating,
                    model_response_id: responseId
                })
            });
            
            alert('Thank you for your feedback!');
        }
    </script>
</body>
</html>
```

### 5. Monitoring and Analytics

```cpp
class FeedbackAnalytics {
public:
    explicit FeedbackAnalytics(
        std::shared_ptr<FeedbackStorageService> storage
    ) : storage_(storage) {}
    
    struct AdapterMetrics {
        size_t total_feedback;
        double avg_rating;
        size_t positive_count;
        size_t negative_count;
        size_t training_ready;
        double quality_trend;  // Trend over last 7 days
    };
    
    AdapterMetrics getMetrics(const std::string& adapter_id) {
        auto stats = storage_->getStatistics(adapter_id);
        
        AdapterMetrics metrics;
        metrics.total_feedback = stats["total_count"];
        metrics.avg_rating = stats["avg_rating"];
        metrics.training_ready = stats["flagged_for_training"];
        
        // Calculate positive/negative counts
        if (stats.contains("by_rating")) {
            for (int rating = 4; rating <= 5; rating++) {
                if (stats["by_rating"].contains(std::to_string(rating))) {
                    metrics.positive_count += 
                        stats["by_rating"][std::to_string(rating)].get<size_t>();
                }
            }
            for (int rating = 1; rating <= 2; rating++) {
                if (stats["by_rating"].contains(std::to_string(rating))) {
                    metrics.negative_count += 
                        stats["by_rating"][std::to_string(rating)].get<size_t>();
                }
            }
        }
        
        // Calculate trend
        metrics.quality_trend = calculateTrend(adapter_id);
        
        return metrics;
    }
    
    void printDashboard(const std::string& adapter_id) {
        auto metrics = getMetrics(adapter_id);
        
        std::cout << "=== ThemisHelp LoRA Dashboard ===" << std::endl;
        std::cout << "Adapter: " << adapter_id << std::endl;
        std::cout << "Total Feedback: " << metrics.total_feedback << std::endl;
        std::cout << "Average Rating: " << std::fixed << std::setprecision(2) 
                  << metrics.avg_rating << " / 5.0" << std::endl;
        std::cout << "Positive: " << metrics.positive_count << std::endl;
        std::cout << "Negative: " << metrics.negative_count << std::endl;
        std::cout << "Training Ready: " << metrics.training_ready << std::endl;
        std::cout << "Quality Trend: " << (metrics.quality_trend > 0 ? "↑" : "↓") 
                  << " " << std::abs(metrics.quality_trend) << "%" << std::endl;
    }

private:
    double calculateTrend(const std::string& adapter_id) {
        // Get feedback from last 7 days
        auto now = std::chrono::system_clock::now();
        auto week_ago = now - std::chrono::hours{7 * 24};
        
        FeedbackFilter recent_filter;
        recent_filter.adapter_id = adapter_id;
        recent_filter.since = week_ago;
        
        auto recent_feedback = storage_->listFeedback(recent_filter);
        
        if (recent_feedback.size() < 10) {
            return 0.0;  // Not enough data
        }
        
        // Calculate average rating for first and second half
        size_t half = recent_feedback.size() / 2;
        
        double first_half_avg = 0;
        double second_half_avg = 0;
        
        for (size_t i = 0; i < half; i++) {
            first_half_avg += recent_feedback[i].rating;
        }
        first_half_avg /= half;
        
        for (size_t i = half; i < recent_feedback.size(); i++) {
            second_half_avg += recent_feedback[i].rating;
        }
        second_half_avg /= (recent_feedback.size() - half);
        
        // Calculate percentage change
        return ((second_half_avg - first_half_avg) / first_half_avg) * 100.0;
    }
    
    std::shared_ptr<FeedbackStorageService> storage_;
};
```

### 6. Automated Training Pipeline

```cpp
class AutomatedTrainingPipeline {
public:
    AutomatedTrainingPipeline(
        std::shared_ptr<applications::ThemisHelpLoRA> lora,
        std::shared_ptr<FeedbackStorageService> feedback_storage
    ) : lora_(lora), 
        feedback_storage_(feedback_storage),
        running_(false) {}
    
    void start() {
        running_ = true;
        
        // Start monitoring thread
        monitor_thread_ = std::thread([this]() {
            while (running_) {
                checkAndTrain();
                std::this_thread::sleep_for(std::chrono::hours{1});
            }
        });
    }
    
    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

private:
    void checkAndTrain() {
        // Check if training should be triggered
        if (!feedback_storage_->shouldTriggerTraining("themis_help_lora")) {
            return;
        }
        
        spdlog::info("Automated training triggered");
        
        try {
            // Backup current adapter
            backupAdapter();
            
            // Train from feedback
            auto result = lora_->trainFromFeedback();
            
            if (result.success) {
                spdlog::info("Training successful - Accuracy: {:.2f}%", 
                            result.validation_accuracy * 100);
                
                // Test new adapter
                if (validateAdapter()) {
                    spdlog::info("New adapter validated successfully");
                } else {
                    spdlog::warn("New adapter validation failed - Rolling back");
                    lora_->rollbackToPreviousVersion();
                }
            } else {
                spdlog::error("Training failed: {}", result.error_message);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Training pipeline error: {}", e.what());
        }
    }
    
    void backupAdapter() {
        // Implementation for backing up current adapter
        spdlog::info("Backing up current adapter version");
    }
    
    bool validateAdapter() {
        // Run validation tests on new adapter
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"What is ThemisDB?", "database"},
            {"How to create a collection?", "CREATE COLLECTION"},
            {"What is AQL?", "Adaptive Query Language"}
        };
        
        int passed = 0;
        for (const auto& [question, expected_keyword] : test_cases) {
            std::string response = lora_->query(question);
            if (response.find(expected_keyword) != std::string::npos) {
                passed++;
            }
        }
        
        // Require at least 80% pass rate
        return (passed >= test_cases.size() * 0.8);
    }
    
    std::shared_ptr<applications::ThemisHelpLoRA> lora_;
    std::shared_ptr<FeedbackStorageService> feedback_storage_;
    std::atomic<bool> running_;
    std::thread monitor_thread_;
};
```

### 7. Complete Integration Example

```cpp
int main() {
    // 1. Initialize components
    auto db = std::make_shared<RocksDBWrapper>("data/themisdb");
    auto graph_index = std::make_shared<GraphIndex>(db);
    
    // 2. Setup feedback storage
    FeedbackStorageService::Config feedback_config;
    feedback_config.db = db;
    feedback_config.graph_index = graph_index;
    auto feedback_storage = std::make_shared<FeedbackStorageService>(feedback_config);
    
    // Register plugins
    feedback_storage->registerPlugin(std::make_shared<BaseFeedbackPlugin>());
    feedback_storage->registerPlugin(std::make_shared<PrivacyFilterPlugin>());
    
    // 3. Initialize ThemisHelp LoRA
    applications::ThemisHelpLoRA::Config lora_config;
    auto help_lora = std::make_shared<applications::ThemisHelpLoRA>(lora_config);
    
    // 4. Create integrated system
    auto help_with_feedback = std::make_shared<ThemisHelpWithFeedback>(
        help_lora,
        feedback_storage
    );
    
    // 5. Start automated training pipeline
    AutomatedTrainingPipeline pipeline(help_lora, feedback_storage);
    pipeline.start();
    
    // 6. Start monitoring
    FeedbackAnalytics analytics(feedback_storage);
    
    // 7. Example usage
    auto result = help_with_feedback->query(
        "How do I use vector search in ThemisDB?",
        "user123"
    );
    
    std::cout << "Response: " << result.response << std::endl;
    
    // User provides feedback
    help_with_feedback->submitFeedback(result.response_id, 5, "Perfect!");
    
    // Print dashboard
    analytics.printDashboard("themis_help_lora");
    
    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds{60});
    
    // Cleanup
    pipeline.stop();
    
    return 0;
}
```

## Best Practices

### 1. Feedback Collection Timing

- **Immediate**: Collect feedback right after response
- **Delayed**: Allow users to try the solution first
- **Contextual**: Show feedback prompt after successful task completion

### 2. Feedback Quality

- Use rating scales (1-5 stars)
- Encourage textual feedback for context
- Make feedback optional but encourage it
- Reward users for providing feedback

### 3. Training Strategy

- **Batch Training**: Accumulate 50-200 feedback items
- **Incremental**: Train regularly (daily/weekly)
- **Validation**: Always validate new adapters
- **Rollback**: Keep previous versions for fallback

### 4. Monitoring

- Track average ratings over time
- Monitor training frequency
- Alert on quality degradation
- Dashboard for key metrics

## Troubleshooting

### Training Not Triggering

Check:
1. Sufficient feedback accumulated?
2. Training trigger plugin configured?
3. Feedback flagged for training?

### Low Quality Responses

Solutions:
1. Review recent negative feedback
2. Check training data quality
3. Adjust training hyperparameters
4. Rollback to previous version

### High Latency

Optimize:
1. Use response caching
2. Load adapter to GPU
3. Reduce batch size
4. Use quantized models

## Next Steps

- See [API Documentation](./LORA_FEEDBACK_API.md) for REST API details
- See [Graph Query Examples](./GRAPH_QUERY_EXAMPLES.md) for advanced queries
- See [Plugin Developer Guide](./PLUGIN_DEVELOPER_GUIDE.md) for custom plugins
