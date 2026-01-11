# HTTP Server Integration Guide for Feedback API

## Overview

This document describes how to integrate the Feedback API Handler into the ThemisDB HTTP server.

## Files to Modify

### 1. `src/server/http_server.cpp`

#### Step 1: Add Include

Add at the top with other API handler includes:

```cpp
#include "server/feedback_api_handler.h"
```

#### Step 2: Add Routes to Route Enum

Add to the `Route` enum (around line 1034):

```cpp
enum class Route {
    // ... existing routes ...
    
    // Feedback API routes
    FeedbackPost,              // POST /api/feedback
    FeedbackGet,               // GET /api/feedback
    FeedbackGetById,           // GET /api/feedback/{id}
    FeedbackPut,               // PUT /api/feedback/{id}
    FeedbackDelete,            // DELETE /api/feedback/{id}
    FeedbackAdapterGet,        // GET /api/feedback/adapter/{adapter_id}
    FeedbackStatsGet,          // GET /api/feedback/stats
    
    // ... rest of routes ...
};
```

#### Step 3: Add Route Detection

Add to the `matchRoute` function (around line 1173):

```cpp
// Feedback API routes
if (path_only == "/api/feedback" && method == http::verb::post) return Route::FeedbackPost;
if (path_only == "/api/feedback" && method == http::verb::get) return Route::FeedbackGet;

// Pattern: /api/feedback/{id}
if (path_only.rfind("/api/feedback/", 0) == 0 && path_only != "/api/feedback/stats") {
    std::string suffix = path_only.substr(14); // Length of "/api/feedback/"
    
    if (suffix.rfind("adapter/", 0) == 0) {
        // /api/feedback/adapter/{adapter_id}
        if (method == http::verb::get) return Route::FeedbackAdapterGet;
    } else if (!suffix.empty() && suffix.find('/') == std::string::npos) {
        // /api/feedback/{id}
        if (method == http::verb::get) return Route::FeedbackGetById;
        if (method == http::verb::put) return Route::FeedbackPut;
        if (method == http::verb::delete_) return Route::FeedbackDelete;
    }
}

if (path_only == "/api/feedback/stats" && method == http::verb::get) return Route::FeedbackStatsGet;
```

#### Step 4: Add Member Variable

Add to the `HttpServer::Impl` class (around line 1000):

```cpp
class HttpServer::Impl {
    // ... existing members ...
    
    std::unique_ptr<server::FeedbackAPIHandler> feedback_api_handler_;
    
    // ... rest of members ...
};
```

#### Step 5: Initialize Handler in Constructor

Add to the `HttpServer::Impl` constructor (around line 800):

```cpp
// Initialize feedback storage service
if (db_) {
    auto feedback_storage_config = llm::lora::FeedbackStorageService::Config{};
    feedback_storage_config.db = std::shared_ptr<RocksDBWrapper>(
        db_, [](RocksDBWrapper*){}
    );
    if (graph_index_) {
        feedback_storage_config.graph_index = graph_index_;
    }
    
    auto feedback_storage = std::make_shared<llm::lora::FeedbackStorageService>(
        feedback_storage_config
    );
    
    // Register default plugins
    feedback_storage->registerPlugin(
        std::make_shared<llm::lora::BaseFeedbackPlugin>()
    );
    feedback_storage->registerPlugin(
        std::make_shared<llm::lora::PrivacyFilterPlugin>()
    );
    feedback_storage->registerPlugin(
        std::make_shared<llm::lora::ContentValidationPlugin>()
    );
    
    feedback_api_handler_ = std::make_unique<server::FeedbackAPIHandler>(
        feedback_storage
    );
}
```

#### Step 6: Add Route Handlers

Add to the switch statement in the request handler (around line 1408):

```cpp
case Route::FeedbackPost:
    if (feedback_api_handler_) {
        return feedback_api_handler_->handleCreateFeedback(req);
    }
    break;

case Route::FeedbackGet:
    if (feedback_api_handler_) {
        return feedback_api_handler_->handleListFeedback(req);
    }
    break;

case Route::FeedbackGetById: {
    if (feedback_api_handler_) {
        std::string path(req.target());
        std::string id = extractPathParam(path, "/api/feedback/");
        return feedback_api_handler_->handleGetFeedback(req, id);
    }
    break;
}

case Route::FeedbackPut: {
    if (feedback_api_handler_) {
        std::string path(req.target());
        std::string id = extractPathParam(path, "/api/feedback/");
        return feedback_api_handler_->handleUpdateFeedback(req, id);
    }
    break;
}

case Route::FeedbackDelete: {
    if (feedback_api_handler_) {
        std::string path(req.target());
        std::string id = extractPathParam(path, "/api/feedback/");
        return feedback_api_handler_->handleDeleteFeedback(req, id);
    }
    break;
}

case Route::FeedbackAdapterGet: {
    if (feedback_api_handler_) {
        std::string path(req.target());
        std::string adapter_id = extractPathParam(path, "/api/feedback/adapter/");
        return feedback_api_handler_->handleGetAdapterFeedback(req, adapter_id);
    }
    break;
}

case Route::FeedbackStatsGet:
    if (feedback_api_handler_) {
        return feedback_api_handler_->handleGetStatistics(req);
    }
    break;
```

## Helper Function

The `extractPathParam` helper function (if not already present):

```cpp
std::string extractPathParam(const std::string& path, const std::string& prefix) {
    if (path.rfind(prefix, 0) == 0) {
        std::string param = path.substr(prefix.length());
        // Remove query string if present
        size_t query_pos = param.find('?');
        if (query_pos != std::string::npos) {
            param = param.substr(0, query_pos);
        }
        return param;
    }
    return "";
}
```

## Testing the Integration

### 1. Build the Project

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
```

### 2. Run Tests

```bash
cd build
ctest -R feedback -V
```

### 3. Manual API Testing

Start the server:
```bash
./build/themis_server --config config/server.yaml
```

Test the endpoints:
```bash
# Create feedback
curl -X POST http://localhost:8765/api/feedback \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "test_adapter",
    "user_id": "user123",
    "rating": 5,
    "feedback_text": "Great response!"
  }'

# List feedback
curl -X GET http://localhost:8765/api/feedback

# Get statistics
curl -X GET http://localhost:8765/api/feedback/stats
```

## Configuration

Add to server configuration file (`config/server.yaml`):

```yaml
feedback:
  enabled: true
  collection_name: help_feedback
  enable_graph_links: true
  plugins:
    - type: base
    - type: privacy
    - type: content_validation
    - type: training_trigger
      config:
        min_batch_size: 50
        max_batch_size: 200
        min_avg_rating: 3.5
        max_wait_time_hours: 24
```

## Security Considerations

1. **Authentication**: Ensure JWT authentication is enabled
2. **Rate Limiting**: Apply rate limits to feedback endpoints
3. **Input Validation**: All input is validated through plugins
4. **PII Protection**: PrivacyFilterPlugin automatically removes PII

## Next Steps

1. Review and apply these changes to `http_server.cpp`
2. Build and test the integration
3. Enable in production configuration
4. Monitor feedback collection metrics
5. Configure automated training pipeline

## Troubleshooting

### Build Errors

If you encounter build errors:

1. Check that all header files are included
2. Verify that `llm/lora_framework` headers are in include path
3. Ensure RocksDB and other dependencies are properly linked

### Runtime Errors

If the API endpoints return errors:

1. Check that `feedback_api_handler_` is properly initialized
2. Verify database connection is established
3. Review logs for detailed error messages
4. Ensure collection permissions are correct

### Performance Issues

If feedback operations are slow:

1. Create indexes on `adapter_id` and `timestamp` fields
2. Adjust batch sizes in configuration
3. Enable caching for frequently accessed feedback
4. Monitor database query performance
