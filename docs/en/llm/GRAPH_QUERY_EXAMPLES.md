# Graph Query Examples for LoRA Feedback

## Overview

The LoRA Feedback System integrates with ThemisDB's graph capabilities, creating relationships between feedback entries and LoRA adapters. This document provides examples of querying feedback using graph traversal and AQL (Adaptive Query Language).

## Graph Schema

### Entities (Nodes)

1. **LoRA Adapters** (`lora_adapters` collection)
   - `adapter_id`: Unique adapter identifier
   - `version`: Adapter version
   - `base_model`: Base model name
   - `description`: Adapter description

2. **Feedback** (`help_feedback` collection)
   - `id`: Unique feedback identifier
   - `adapter_id`: Associated adapter
   - `user_id`: User who provided feedback
   - `rating`: Rating (1-5)
   - `feedback_text`: Feedback content
   - `timestamp`: Creation timestamp

### Relationships (Edges)

- **belongs_to_adapter**: Feedback → LoRA Adapter
  - Links each feedback entry to its associated adapter
  - Direction: Feedback (from) → Adapter (to)
  - Type: Many-to-One

## AQL Query Examples

### 1. Get All Feedback for an Adapter

Find all feedback entries connected to a specific LoRA adapter.

```aql
FOR feedback IN help_feedback
  FOR adapter IN lora_adapters
    FILTER feedback.adapter_id == adapter.adapter_id
    FILTER adapter.adapter_id == 'themis_help_lora'
    RETURN {
      feedback_id: feedback.id,
      rating: feedback.rating,
      feedback_text: feedback.feedback_text,
      user_id: feedback.user_id,
      timestamp: feedback.timestamp,
      adapter: {
        id: adapter.adapter_id,
        version: adapter.version
      }
    }
```

### 2. Get Adapters with High-Quality Feedback

Find adapters that have received mostly positive feedback (rating >= 4).

```aql
FOR adapter IN lora_adapters
  LET feedback_list = (
    FOR feedback IN help_feedback
      FILTER feedback.adapter_id == adapter.adapter_id
      FILTER feedback.rating >= 4
      RETURN feedback
  )
  FILTER LENGTH(feedback_list) > 0
  RETURN {
    adapter_id: adapter.adapter_id,
    positive_feedback_count: LENGTH(feedback_list),
    avg_rating: AVG(feedback_list[*].rating)
  }
```

### 3. Get Feedback Statistics by Adapter

Calculate aggregated statistics for each adapter.

```aql
FOR adapter IN lora_adapters
  LET feedback_list = (
    FOR feedback IN help_feedback
      FILTER feedback.adapter_id == adapter.adapter_id
      RETURN feedback
  )
  FILTER LENGTH(feedback_list) > 0
  RETURN {
    adapter_id: adapter.adapter_id,
    total_feedback: LENGTH(feedback_list),
    avg_rating: AVG(feedback_list[*].rating),
    rating_distribution: {
      "5_star": LENGTH(FOR f IN feedback_list FILTER f.rating == 5 RETURN 1),
      "4_star": LENGTH(FOR f IN feedback_list FILTER f.rating == 4 RETURN 1),
      "3_star": LENGTH(FOR f IN feedback_list FILTER f.rating == 3 RETURN 1),
      "2_star": LENGTH(FOR f IN feedback_list FILTER f.rating == 2 RETURN 1),
      "1_star": LENGTH(FOR f IN feedback_list FILTER f.rating == 1 RETURN 1)
    },
    flagged_for_training: LENGTH(FOR f IN feedback_list FILTER f.flagged_for_training RETURN 1)
  }
```

### 4. Find Adapters Needing Training

Identify adapters with sufficient negative feedback that might benefit from retraining.

```aql
FOR adapter IN lora_adapters
  LET negative_feedback = (
    FOR feedback IN help_feedback
      FILTER feedback.adapter_id == adapter.adapter_id
      FILTER feedback.rating <= 2
      FILTER feedback.flagged_for_training == true
      RETURN feedback
  )
  FILTER LENGTH(negative_feedback) >= 10
  RETURN {
    adapter_id: adapter.adapter_id,
    negative_feedback_count: LENGTH(negative_feedback),
    needs_retraining: true,
    avg_negative_rating: AVG(negative_feedback[*].rating)
  }
```

### 5. Get Recent Feedback Across All Adapters

Find the most recent feedback entries across all adapters.

```aql
FOR feedback IN help_feedback
  SORT feedback.timestamp DESC
  LIMIT 100
  FOR adapter IN lora_adapters
    FILTER feedback.adapter_id == adapter.adapter_id
    RETURN {
      feedback_id: feedback.id,
      adapter_id: adapter.adapter_id,
      adapter_version: adapter.version,
      rating: feedback.rating,
      feedback_text: feedback.feedback_text,
      timestamp: feedback.timestamp,
      user_id: feedback.user_id
    }
```

### 6. Compare Adapter Performance

Compare multiple adapters based on their feedback.

```aql
FOR adapter IN lora_adapters
  FILTER adapter.adapter_id IN ['themis_help_lora', 'general_qa_lora', 'code_assist_lora']
  LET feedback_list = (
    FOR feedback IN help_feedback
      FILTER feedback.adapter_id == adapter.adapter_id
      RETURN feedback
  )
  RETURN {
    adapter_id: adapter.adapter_id,
    total_feedback: LENGTH(feedback_list),
    avg_rating: AVG(feedback_list[*].rating),
    positive_ratio: LENGTH(FOR f IN feedback_list FILTER f.rating >= 4 RETURN 1) / LENGTH(feedback_list),
    training_ready: LENGTH(FOR f IN feedback_list FILTER f.flagged_for_training RETURN 1) >= 100
  }
```

### 7. Find Training-Ready Batches

Get feedback batches that are ready for training, grouped by adapter and category.

```aql
FOR adapter IN lora_adapters
  LET training_feedback = (
    FOR feedback IN help_feedback
      FILTER feedback.adapter_id == adapter.adapter_id
      FILTER feedback.flagged_for_training == true
      RETURN feedback
  )
  FILTER LENGTH(training_feedback) >= 50
  RETURN {
    adapter_id: adapter.adapter_id,
    batch_size: LENGTH(training_feedback),
    by_category: {
      positive: (FOR f IN training_feedback FILTER f.training_category == 'positive' RETURN f),
      negative: (FOR f IN training_feedback FILTER f.training_category == 'negative' RETURN f),
      neutral: (FOR f IN training_feedback FILTER f.training_category == 'neutral' RETURN f)
    }
  }
```

### 8. Track User Engagement

Find users who provide the most feedback and their satisfaction levels.

```aql
FOR feedback IN help_feedback
  COLLECT user = feedback.user_id INTO user_feedback = feedback
  LET avg_rating = AVG(user_feedback[*].rating)
  LET feedback_count = LENGTH(user_feedback)
  FILTER feedback_count >= 5
  SORT feedback_count DESC
  LIMIT 20
  RETURN {
    user_id: user,
    feedback_count: feedback_count,
    avg_rating: avg_rating,
    engagement_level: feedback_count > 20 ? 'high' : (feedback_count > 10 ? 'medium' : 'low')
  }
```

### 9. Time-Series Analysis

Analyze feedback trends over time for an adapter.

```aql
FOR feedback IN help_feedback
  FILTER feedback.adapter_id == 'themis_help_lora'
  FILTER feedback.timestamp >= DATE_TIMESTAMP('2024-01-01')
  LET date = DATE_FORMAT(feedback.timestamp, '%Y-%m-%d')
  COLLECT day = date INTO daily_feedback = feedback
  RETURN {
    date: day,
    count: LENGTH(daily_feedback),
    avg_rating: AVG(daily_feedback[*].rating),
    positive_count: LENGTH(FOR f IN daily_feedback FILTER f.rating >= 4 RETURN 1),
    negative_count: LENGTH(FOR f IN daily_feedback FILTER f.rating <= 2 RETURN 1)
  }
```

### 10. Cross-Adapter Feedback Patterns

Find users who have provided feedback for multiple adapters.

```aql
FOR feedback IN help_feedback
  COLLECT user = feedback.user_id INTO user_feedback = feedback
  LET adapters = UNIQUE(user_feedback[*].adapter_id)
  FILTER LENGTH(adapters) > 1
  RETURN {
    user_id: user,
    adapters_used: adapters,
    adapter_count: LENGTH(adapters),
    total_feedback: LENGTH(user_feedback),
    avg_rating_by_adapter: (
      FOR adapter_id IN adapters
        LET adapter_feedback = (FOR f IN user_feedback FILTER f.adapter_id == adapter_id RETURN f)
        RETURN {
          adapter_id: adapter_id,
          avg_rating: AVG(adapter_feedback[*].rating),
          count: LENGTH(adapter_feedback)
        }
    )
  }
```

## Graph Traversal Examples

### Using Graph Edges (Future Enhancement)

When graph edges are fully implemented, you can use graph traversal operations:

```aql
// Traverse from feedback to adapter
FOR feedback IN help_feedback
  FILTER feedback.rating >= 4
  FOR adapter IN 1..1 OUTBOUND feedback belongs_to_adapter
    RETURN {
      feedback_id: feedback.id,
      adapter_id: adapter.adapter_id,
      rating: feedback.rating
    }
```

```aql
// Traverse from adapter to all its feedback
FOR adapter IN lora_adapters
  FILTER adapter.adapter_id == 'themis_help_lora'
  FOR feedback IN 1..1 INBOUND adapter belongs_to_adapter
    SORT feedback.timestamp DESC
    LIMIT 100
    RETURN feedback
```

## Performance Tips

### 1. Use Indexes

Create indexes on frequently queried fields:

```javascript
// Create index on adapter_id in feedback collection
db.ensureIndex({
  collection: 'help_feedback',
  fields: ['adapter_id'],
  type: 'hash'
});

// Create index on timestamp for time-based queries
db.ensureIndex({
  collection: 'help_feedback',
  fields: ['timestamp'],
  type: 'skiplist'
});

// Create composite index for filtering
db.ensureIndex({
  collection: 'help_feedback',
  fields: ['adapter_id', 'rating', 'flagged_for_training'],
  type: 'hash'
});
```

### 2. Use Appropriate Limits

Always use `LIMIT` for large result sets to avoid memory issues:

```aql
FOR feedback IN help_feedback
  SORT feedback.timestamp DESC
  LIMIT 1000  // Reasonable limit
  RETURN feedback
```

### 3. Filter Early

Apply filters as early as possible in your query:

```aql
// Good: Filter early
FOR feedback IN help_feedback
  FILTER feedback.adapter_id == 'themis_help_lora'
  FILTER feedback.rating >= 4
  RETURN feedback

// Bad: Filter late
FOR feedback IN help_feedback
  LET result = feedback
  FILTER result.adapter_id == 'themis_help_lora'
  RETURN result
```

## Integration with Application Code

### C++ Example

```cpp
#include "aql/aql_executor.h"

// Execute AQL query for adapter feedback
std::string query = R"(
  FOR feedback IN help_feedback
    FILTER feedback.adapter_id == @adapter_id
    FILTER feedback.rating >= 4
    SORT feedback.timestamp DESC
    LIMIT 100
    RETURN feedback
)";

json bind_vars = {
  {"adapter_id", "themis_help_lora"}
};

auto result = aql_executor->execute(query, bind_vars);

for (const auto& feedback_json : result) {
  auto feedback = Feedback::fromJSON(feedback_json);
  // Process feedback...
}
```

### Python Example

```python
import requests

# Query via HTTP API
query = """
FOR feedback IN help_feedback
  FILTER feedback.adapter_id == 'themis_help_lora'
  FILTER feedback.rating >= 4
  RETURN feedback
"""

response = requests.post(
    'http://localhost:8765/api/aql',
    json={'query': query},
    headers={'Authorization': f'Bearer {token}'}
)

feedback_list = response.json()['result']
```

## Next Steps

- See [API Documentation](./LORA_FEEDBACK_API.md) for REST API examples
- See [Plugin Developer Guide](./PLUGIN_DEVELOPER_GUIDE.md) for extending functionality
- See [ThemisHelp LoRA Integration](./THEMIS_HELP_LORA_INTEGRATION.md) for complete examples
