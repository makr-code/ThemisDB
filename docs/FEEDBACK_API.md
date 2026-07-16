# Feedback API Documentation

## Overview

The Feedback API allows users to provide feedback on LLM model responses for continuous learning and LoRA fine-tuning. This feedback is collected, validated, and stored for later use in training iterations.

## Base URL

All feedback endpoints are under: `/api/v1/llm/feedback`

## Authentication

All endpoints require Bearer Token (JWT) authentication via the `Authorization` header:

```
Authorization: Bearer <your-jwt-token>
```

## Endpoints

### 1. Submit Feedback

Submit user feedback for a model response.

**Endpoint:** `POST /api/v1/llm/feedback`

**Request Body:**
```json
{
  "type": "positive|negative",
  "question": "User's original question",
  "answer": "Model's answer",
  "user_id": "optional_user_identifier",
  "interaction_id": "optional_llm_interaction_id",
  "correction": "For negative feedback: corrected answer",
  "comment": "Optional user comment",
  "model_version": "Model version that generated the answer",
  "adapter_id": "LoRA adapter ID (if used)",
  "adapter_version": "LoRA adapter version"
}
```

**Required Fields:**
- `type`: Must be either "positive" or "negative"
- `question`: The original question asked
- `answer`: The model's response

**Optional Fields:**
- `user_id`: Identifier for the user providing feedback
- `interaction_id`: Reference to the LLM interaction (from LLMInteractionStore)
- `correction`: Required for negative feedback to provide the correct answer
- `comment`: Additional user comments
- `model_version`: Model version that generated the response
- `adapter_id`: LoRA adapter identifier (e.g., "themis_help_lora")
- `adapter_version`: LoRA adapter version (e.g., "v1.0")

**Response (201 Created):**
```json
{
  "id": "feedback-unique-id",
  "type": "positive",
  "question": "How do I enable sharding?",
  "answer": "To enable sharding...",
  "validation_status": "approved|pending|rejected|flagged",
  "created_at": 1234567890000,
  "message": "Feedback recorded successfully"
}
```

**Validation:**
- Feedback is automatically validated for spam and quality
- Validation statuses:
  - `approved`: Feedback passed validation checks
  - `pending`: Awaiting validation
  - `rejected`: Failed validation (spam, too short, etc.)
  - `flagged`: Requires manual review

**Example - Positive Feedback:**
```bash
curl -X POST https://api.themisdb.com/api/v1/llm/feedback \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "type": "positive",
    "question": "How do I enable sharding in ThemisDB?",
    "answer": "To enable sharding, use SHARD BY clause in CREATE COLLECTION.",
    "user_id": "user123",
    "model_version": "llama-2-7b",
    "adapter_id": "themis_help_lora",
    "adapter_version": "v1.0"
  }'
```

**Example - Negative Feedback:**
```bash
curl -X POST https://api.themisdb.com/api/v1/llm/feedback \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "type": "negative",
    "question": "What is the default replication factor?",
    "answer": "The default replication factor is 1.",
    "correction": "The default replication factor is 3 for production.",
    "user_id": "user456",
    "model_version": "llama-2-7b"
  }'
```

### 2. Get Specific Feedback

Retrieve a specific feedback entry by ID.

**Endpoint:** `GET /api/v1/llm/feedback/{id}`

**Path Parameters:**
- `id`: Feedback entry ID

**Response (200 OK):**
```json
{
  "id": "feedback-12345",
  "type": "positive",
  "question": "How do I enable sharding?",
  "answer": "To enable sharding...",
  "user_id": "user123",
  "validation_status": "approved",
  "model_version": "llama-2-7b",
  "adapter_id": "themis_help_lora",
  "adapter_version": "v1.0",
  "used_for_training": false,
  "training_batch_id": 0,
  "created_at": 1234567890000
}
```

**Example:**
```bash
curl -X GET https://api.themisdb.com/api/v1/llm/feedback/feedback-12345 \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"
```

### 3. List Feedback

List feedback entries with optional filters.

**Endpoint:** `GET /api/v1/llm/feedback`

**Query Parameters:**
- `limit`: Maximum number of entries to return (default: 100, max: 1000)
- `type`: Filter by type (`positive` or `negative`)
- `status`: Filter by validation status (`pending`, `approved`, `rejected`, `flagged`)
- `unused`: Set to `true` to list only feedback not yet used for training
- `model`: Filter by model version
- `adapter`: Filter by adapter ID

**Response (200 OK):**
```json
{
  "feedback": [
    {
      "id": "feedback-1",
      "type": "positive",
      "question": "...",
      "answer": "...",
      "validation_status": "approved",
      "created_at": 1234567890000
    },
    {
      "id": "feedback-2",
      "type": "negative",
      "question": "...",
      "answer": "...",
      "correction": "...",
      "validation_status": "approved",
      "created_at": 1234567891000
    }
  ],
  "count": 2,
  "limit": 100
}
```

**Example - List all positive feedback:**
```bash
curl -X GET "https://api.themisdb.com/api/v1/llm/feedback?type=positive&limit=50" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"
```

**Example - List unused feedback for training:**
```bash
curl -X GET "https://api.themisdb.com/api/v1/llm/feedback?unused=true" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"
```

### 4. Get Feedback Statistics

Get aggregate statistics about feedback.

**Endpoint:** `GET /api/v1/llm/feedback/stats`

**Response (200 OK):**
```json
{
  "total_feedback": 1000,
  "positive_count": 650,
  "negative_count": 350,
  "pending_validation": 10,
  "approved_count": 900,
  "rejected_count": 80,
  "flagged_count": 10,
  "unused_for_training": 750,
  "used_for_training": 250,
  "positive_ratio": 0.65
}
```

**Example:**
```bash
curl -X GET https://api.themisdb.com/api/v1/llm/feedback/stats \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"
```

## Validation Rules

The system automatically validates feedback using the following rules:

### Spam Detection
- Rejects entries with less than 3 characters
- Rejects entries with excessive character repetition (10+ consecutive identical characters)
- Rejects entries containing common spam keywords

### Quality Checks
- Question and answer must be at least 5 characters long
- Negative feedback should include either a correction or comment (otherwise flagged)
- Entries that don't meet quality thresholds are flagged for manual review

## Integration with LoRA Training

Feedback entries are designed to be consumed by LoRA training pipelines:

1. **Collection**: Users provide feedback through the API
2. **Validation**: System validates and filters feedback
3. **Storage**: Approved feedback is stored in the `help_feedback` collection
4. **Training**: Training service queries unused feedback entries
5. **Marking**: After training, feedback is marked as used with a batch ID

Example workflow for training service:
```bash
# Get unused feedback for training
curl -X GET "https://api.themisdb.com/api/v1/llm/feedback?unused=true&status=approved&limit=100" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"

# After training, mark feedback as used (requires admin API)
# This would be done through internal service APIs
```

## Error Responses

All endpoints may return these error responses:

**401 Unauthorized:**
```json
{
  "error": "Unauthorized",
  "details": "Valid Bearer Token required"
}
```

**400 Bad Request:**
```json
{
  "error": "Invalid request",
  "details": "Missing 'type' field (positive or negative)"
}
```

**404 Not Found:**
```json
{
  "error": "Not Found",
  "details": "Feedback entry not found"
}
```

**500 Internal Server Error:**
```json
{
  "error": "Internal server error",
  "details": "Failed to store feedback"
}
```

## Best Practices

1. **Always provide context**: Include model_version and adapter_id when known
2. **Meaningful corrections**: For negative feedback, provide clear corrections
3. **Link interactions**: Use interaction_id to link feedback to specific LLM interactions
4. **Batch feedback**: Collect feedback over time and process in batches for training
5. **Monitor statistics**: Regularly check feedback stats to gauge model performance

## Rate Limiting

Feedback submission is subject to standard API rate limits:
- 100 requests per minute per user
- 10,000 requests per hour per organization

## Storage

Feedback is stored in RocksDB under the key prefix `help_feedback:` with:
- Automatic versioning
- Full audit trail
- Efficient querying and filtering
- Integration with existing ThemisDB storage infrastructure

## Graph Links

### Overview

Feedback entries are linked to LoRA adapters via **FEEDBACK_FOR** graph edges. This enables:
- Querying feedback by adapter
- Tracking adapter lineage
- Multi-model analysis
- Training data provenance

### Creating Graph Links

Graph links are created automatically when `adapter_id` is specified in feedback submission:

```bash
curl -X POST https://api.themisdb.com/api/v1/llm/feedback \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "type": "positive",
    "question": "How do I use vector search?",
    "answer": "Use the VECTOR SEARCH command...",
    "adapter_id": "themis_help_lora_v2",
    "user_id": "user123"
  }'
```

Alternatively, create links manually:

```bash
curl -X POST https://api.themisdb.com/api/v1/llm/feedback/{feedback_id}/link \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "themis_help_lora_v2",
    "metadata": {
      "confidence": 0.95,
      "session_id": "abc-123"
    }
  }'
```

### Query Feedback by Adapter

Get all feedback for a specific adapter:

```bash
curl -X GET "https://api.themisdb.com/api/v1/llm/feedback/by-adapter/themis_help_lora_v2?limit=100&unused=true" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"
```

**Response:**
```json
{
  "adapter_id": "themis_help_lora_v2",
  "feedback": [
    {
      "id": "feedback-1",
      "type": "positive",
      "question": "...",
      "answer": "...",
      "validation_status": "approved",
      "used_for_training": false
    }
  ],
  "count": 42,
  "total_linked": 150
}
```

### AQL Query Examples

**Get all feedback for an adapter:**
```aql
MATCH (f:Feedback)-[r:FEEDBACK_FOR]->(a:Adapter {id: 'themis_help_lora_v2'})
WHERE f.validation_status = 'approved' 
  AND f.used_for_training = false
RETURN f
LIMIT 100
```

**Get feedback statistics by adapter:**
```aql
MATCH (f:Feedback)-[r:FEEDBACK_FOR]->(a:Adapter)
RETURN a.id AS adapter_id,
       COUNT(f) AS total_feedback,
       SUM(CASE WHEN f.type = 'positive' THEN 1 ELSE 0 END) AS positive_count,
       SUM(CASE WHEN f.type = 'negative' THEN 1 ELSE 0 END) AS negative_count,
       AVG(CASE WHEN f.type = 'positive' THEN 1.0 ELSE 0.0 END) AS positive_ratio
GROUP BY a.id
```

**Get feedback lineage (adapter inheritance):**
```aql
MATCH path = (f:Feedback)-[:FEEDBACK_FOR]->(a1:Adapter)-[:DERIVED_FROM*]->(a2:Adapter)
WHERE a2.id = 'base_model'
RETURN path, f.id, a1.id
```

**Find adapters trained on similar feedback:**
```aql
MATCH (f:Feedback)-[:FEEDBACK_FOR]->(a1:Adapter),
      (f:Feedback)-[:FEEDBACK_FOR]->(a2:Adapter)
WHERE a1.id <> a2.id
RETURN a1.id, a2.id, COUNT(f) AS shared_feedback_count
GROUP BY a1.id, a2.id
HAVING shared_feedback_count > 10
ORDER BY shared_feedback_count DESC
```

## Plugin-Based Validation

The feedback system supports **optional** plugin-based validation. By default, basic validation is performed, but you can customize validation logic using plugins.

### Available Plugins

1. **NoOp Plugin** - No validation (accept all)
2. **Basic Spam Detection** - Keyword-based spam filtering
3. **Custom Plugins** - Implement your own validation logic

### Example: Custom Validation

```json
{
  "validation_plugin": {
    "name": "custom_spam_detector",
    "config": {
      "ml_model_path": "/models/spam_detector.onnx",
      "threshold": 0.8
    }
  }
}
```

For details, see [Feedback Plugin Interface Documentation](ARCHIVED/implementation-summaries/FEEDBACK_PLUGIN_INTERFACE.md).

