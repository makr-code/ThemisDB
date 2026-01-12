# LoRA Feedback System API Documentation

## Overview

The LoRA Feedback System provides RESTful API endpoints for managing user feedback on LoRA adapter responses. This feedback can be used for continuous learning and model improvement.

## Base URL

```
http://localhost:8765/api/feedback
```

## Authentication

All API endpoints support JWT authentication via the `Authorization` header:

```
Authorization: Bearer <your-jwt-token>
```

## Endpoints

### 1. Create Feedback

Create a new feedback entry for a LoRA adapter response.

**Endpoint:** `POST /api/feedback`

**Request Body:**
```json
{
  "adapter_id": "themis_help_lora",
  "user_id": "user123",
  "rating": 5,
  "feedback_text": "Excellent response, very accurate!",
  "prompt": "What is ThemisDB?",
  "response": "ThemisDB is a multi-model database with native AI integration...",
  "model_response_id": "resp_abc123",
  "flagged_for_training": true,
  "training_category": "positive",
  "custom_metadata": {
    "source": "web_interface",
    "session_id": "sess_xyz789"
  }
}
```

**Response (201 Created):**
```json
{
  "id": "fb_550e8400-e29b-41d4-a716-446655440000",
  "adapter_id": "themis_help_lora",
  "user_id": "user123",
  "rating": 5,
  "feedback_text": "Excellent response, very accurate!",
  "prompt": "What is ThemisDB?",
  "response": "ThemisDB is a multi-model database with native AI integration...",
  "model_response_id": "resp_abc123",
  "timestamp": 1704067200,
  "flagged_for_training": true,
  "training_category": "positive",
  "custom_metadata": {
    "source": "web_interface",
    "session_id": "sess_xyz789"
  }
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8765/api/feedback \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "adapter_id": "themis_help_lora",
    "user_id": "user123",
    "rating": 5,
    "feedback_text": "Excellent response!",
    "prompt": "What is ThemisDB?",
    "response": "ThemisDB is a multi-model database..."
  }'
```

---

### 2. List Feedback

Retrieve a list of feedback entries with optional filters.

**Endpoint:** `GET /api/feedback`

**Query Parameters:**
- `adapter_id` (string, optional): Filter by adapter ID
- `user_id` (string, optional): Filter by user ID
- `min_rating` (integer, optional): Minimum rating (1-5)
- `flagged_for_training` (boolean, optional): Filter by training flag
- `training_category` (string, optional): Filter by category (positive, negative, neutral)
- `limit` (integer, optional, default: 100): Maximum results to return
- `offset` (integer, optional, default: 0): Pagination offset

**Response (200 OK):**
```json
{
  "count": 2,
  "feedback": [
    {
      "id": "fb_550e8400-e29b-41d4-a716-446655440000",
      "adapter_id": "themis_help_lora",
      "user_id": "user123",
      "rating": 5,
      "feedback_text": "Excellent response!",
      "timestamp": 1704067200,
      "flagged_for_training": true,
      "training_category": "positive"
    },
    {
      "id": "fb_660e8400-e29b-41d4-a716-446655440001",
      "adapter_id": "themis_help_lora",
      "user_id": "user456",
      "rating": 4,
      "feedback_text": "Good, but could be better",
      "timestamp": 1704067300,
      "flagged_for_training": false,
      "training_category": "neutral"
    }
  ]
}
```

**cURL Examples:**

List all feedback:
```bash
curl -X GET http://localhost:8765/api/feedback \
  -H "Authorization: Bearer YOUR_TOKEN"
```

Filter by adapter:
```bash
curl -X GET "http://localhost:8765/api/feedback?adapter_id=themis_help_lora&limit=50" \
  -H "Authorization: Bearer YOUR_TOKEN"
```

Filter by rating:
```bash
curl -X GET "http://localhost:8765/api/feedback?min_rating=4&flagged_for_training=true" \
  -H "Authorization: Bearer YOUR_TOKEN"
```

---

### 3. Get Feedback by ID

Retrieve a specific feedback entry by its ID.

**Endpoint:** `GET /api/feedback/{id}`

**Response (200 OK):**
```json
{
  "id": "fb_550e8400-e29b-41d4-a716-446655440000",
  "adapter_id": "themis_help_lora",
  "user_id": "user123",
  "rating": 5,
  "feedback_text": "Excellent response!",
  "prompt": "What is ThemisDB?",
  "response": "ThemisDB is a multi-model database...",
  "timestamp": 1704067200,
  "flagged_for_training": true,
  "training_category": "positive"
}
```

**cURL Example:**
```bash
curl -X GET http://localhost:8765/api/feedback/fb_550e8400-e29b-41d4-a716-446655440000 \
  -H "Authorization: Bearer YOUR_TOKEN"
```

---

### 4. Update Feedback

Update an existing feedback entry.

**Endpoint:** `PUT /api/feedback/{id}`

**Request Body:**
```json
{
  "adapter_id": "themis_help_lora",
  "user_id": "user123",
  "rating": 4,
  "feedback_text": "Updated: Good but not perfect",
  "flagged_for_training": true,
  "training_category": "neutral"
}
```

**Response (200 OK):**
```json
{
  "id": "fb_550e8400-e29b-41d4-a716-446655440000",
  "adapter_id": "themis_help_lora",
  "user_id": "user123",
  "rating": 4,
  "feedback_text": "Updated: Good but not perfect",
  "timestamp": 1704067200,
  "flagged_for_training": true,
  "training_category": "neutral"
}
```

**cURL Example:**
```bash
curl -X PUT http://localhost:8765/api/feedback/fb_550e8400-e29b-41d4-a716-446655440000 \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "rating": 4,
    "feedback_text": "Updated feedback",
    "flagged_for_training": true
  }'
```

---

### 5. Delete Feedback

Delete a feedback entry.

**Endpoint:** `DELETE /api/feedback/{id}`

**Response (200 OK):**
```json
{
  "success": true,
  "message": "Feedback deleted successfully"
}
```

**cURL Example:**
```bash
curl -X DELETE http://localhost:8765/api/feedback/fb_550e8400-e29b-41d4-a716-446655440000 \
  -H "Authorization: Bearer YOUR_TOKEN"
```

---

### 6. Get Adapter Feedback

Get all feedback for a specific LoRA adapter.

**Endpoint:** `GET /api/feedback/adapter/{adapter_id}`

**Query Parameters:**
- `limit` (integer, optional, default: 100): Maximum results

**Response (200 OK):**
```json
{
  "adapter_id": "themis_help_lora",
  "count": 150,
  "feedback": [
    {
      "id": "fb_550e8400-e29b-41d4-a716-446655440000",
      "rating": 5,
      "feedback_text": "Excellent!",
      "timestamp": 1704067200
    }
  ]
}
```

**cURL Example:**
```bash
curl -X GET "http://localhost:8765/api/feedback/adapter/themis_help_lora?limit=100" \
  -H "Authorization: Bearer YOUR_TOKEN"
```

---

### 7. Get Feedback Statistics

Get aggregated statistics about feedback.

**Endpoint:** `GET /api/feedback/stats`

**Query Parameters:**
- `adapter_id` (string, optional): Filter by adapter ID

**Response (200 OK):**
```json
{
  "total_count": 500,
  "avg_rating": 4.2,
  "flagged_for_training": 125,
  "by_rating": {
    "1": 10,
    "2": 25,
    "3": 100,
    "4": 200,
    "5": 165
  },
  "by_category": {
    "positive": 300,
    "neutral": 150,
    "negative": 50
  }
}
```

**cURL Example:**
```bash
curl -X GET "http://localhost:8765/api/feedback/stats?adapter_id=themis_help_lora" \
  -H "Authorization: Bearer YOUR_TOKEN"
```

---

## Error Responses

### 400 Bad Request
```json
{
  "error": "Invalid JSON: unexpected token",
  "status": 400
}
```

### 404 Not Found
```json
{
  "error": "Feedback not found",
  "status": 404
}
```

### 500 Internal Server Error
```json
{
  "error": "Internal server error",
  "status": 500
}
```

---

## Best Practices

### 1. Rating Scale
Use a consistent 1-5 rating scale:
- **5**: Excellent, perfect response
- **4**: Good, minor issues
- **3**: Acceptable, some problems
- **2**: Poor, major issues
- **1**: Unacceptable, completely wrong

### 2. Training Categories
Use standard categories for consistency:
- `positive`: High-quality responses for reinforcement
- `negative`: Incorrect responses for correction
- `neutral`: Adequate responses, no strong signal

### 3. Metadata
Use `custom_metadata` for application-specific data:
```json
{
  "custom_metadata": {
    "source": "web|mobile|api",
    "session_id": "unique_session_id",
    "model_version": "v1.2.3",
    "response_time_ms": 250
  }
}
```

### 4. Privacy
- Never include PII in feedback text
- Use the PrivacyFilterPlugin to automatically scrub sensitive data
- Store only necessary user identifiers

---

## Integration Example

### Complete Workflow

```javascript
// 1. User submits feedback via web interface
async function submitFeedback(adapterId, rating, text) {
  const response = await fetch('http://localhost:8765/api/feedback', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`
    },
    body: JSON.stringify({
      adapter_id: adapterId,
      user_id: getCurrentUserId(),
      rating: rating,
      feedback_text: text,
      prompt: getLastPrompt(),
      response: getLastResponse(),
      flagged_for_training: rating >= 4 || rating <= 2
    })
  });
  
  return await response.json();
}

// 2. Periodically check if training should be triggered
async function checkTrainingTrigger(adapterId) {
  const stats = await fetch(
    `http://localhost:8765/api/feedback/stats?adapter_id=${adapterId}`,
    {
      headers: { 'Authorization': `Bearer ${token}` }
    }
  ).then(r => r.json());
  
  if (stats.flagged_for_training >= 100) {
    // Trigger training via LoRA Training API
    triggerLoRATraining(adapterId);
  }
}
```

---

## Next Steps

- See [Graph Query Examples](./GRAPH_QUERY_EXAMPLES.md) for querying feedback via graph relationships
- See [Plugin Developer Guide](./PLUGIN_DEVELOPER_GUIDE.md) for creating custom feedback plugins
- See [Integration with themis_help_lora](./THEMIS_HELP_LORA_INTEGRATION.md) for usage examples
