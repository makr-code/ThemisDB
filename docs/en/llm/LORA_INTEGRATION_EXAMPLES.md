# LoRA Framework Integration Examples

**Version:** 1.0  
**Date:** 2026-01-11  
**Status:** Complete

---

## Table of Contents

1. [C++ Integration Examples](#cpp-integration-examples)
2. [REST API Examples](#rest-api-examples)
3. [AQL Function Examples](#aql-function-examples)
4. [Python Client Examples](#python-client-examples)
5. [Complete Application Examples](#complete-application-examples)

---

## C++ Integration Examples

### Basic Adapter Creation

```cpp
#include "llm/lora_framework/lora_orchestrator.h"
#include <iostream>

int main() {
    // Initialize orchestrator
    LoRAOrchestrator::Config config;
    config.db = database_wrapper;
    config.blob_manager = blob_manager;
    config.enable_encryption = true;
    
    auto orchestrator = std::make_shared<LoRAOrchestrator>(config);
    
    // Prepare training data
    TrainingData data;
    data.base_model = "llama-2-7b";
    data.task_type = "documentation_qa";
    data.samples = {
        {"What is ThemisDB?", "ThemisDB is a distributed database..."},
        {"How to enable sharding?", "Use CREATE COLLECTION ... SHARD BY ..."}
    };
    
    // Configure hyperparameters
    LoRAHyperparameters params;
    params.rank = 8;
    params.alpha = 16;
    params.learning_rate = 0.0003;
    params.epochs = 3;
    
    // Create adapter
    try {
        std::string job_id = orchestrator->createAdapter(
            "my_first_adapter",
            "llama-2-7b",
            data,
            params
        );
        
        std::cout << "Training started! Job ID: " << job_id << "\n";
        
        // Wait for completion
        while (true) {
            auto job = orchestrator->getJobStatus(job_id);
            if (job.status == JobStatus::Completed) {
                std::cout << "Training completed successfully!\n";
                break;
            } else if (job.status == JobStatus::Failed) {
                std::cerr << "Training failed: " << job.error_message << "\n";
                return 1;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### Complete themis_help_lora Integration

```cpp
#include "llm/applications/themis_help_lora.h"
#include <iostream>
#include <string>

class DocumentationAssistantApp {
private:
    std::shared_ptr<ThemisHelpLoRA> assistant_;
    
public:
    void initialize() {
        ThemisHelpLoRA::Config config;
        config.adapter_id = "themis_help_lora";
        config.base_model_id = "llama-2-7b";
        config.db = database_wrapper_;
        config.blob_manager = blob_manager_;
        
        assistant_ = std::make_shared<ThemisHelpLoRA>(config);
        
        // Check if adapter is available
        if (!assistant_->isTrained()) {
            std::cout << "Training adapter for the first time...\n";
            bool success = assistant_->trainFromDocumentation();
            if (!success) {
                throw std::runtime_error("Initial training failed");
            }
        }
        
        std::cout << "Documentation assistant ready!\n";
        std::cout << "Version: " << assistant_->getVersion() << "\n";
    }
    
    void interactiveMode() {
        std::string question;
        std::string user_id = "default_user";
        
        while (true) {
            std::cout << "\nEnter your question (or 'quit' to exit): ";
            std::getline(std::cin, question);
            
            if (question == "quit") break;
            if (question.empty()) continue;
            
            // Query
            auto start = std::chrono::steady_clock::now();
            std::string answer = assistant_->query(question, user_id);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // Display answer
            std::cout << "\nAnswer:\n" << answer << "\n";
            std::cout << "(answered in " << duration.count() << "ms)\n";
            
            // Collect feedback
            std::cout << "\nWas this helpful? (y/n/c for correction): ";
            std::string feedback;
            std::getline(std::cin, feedback);
            
            if (feedback == "y") {
                assistant_->addPositiveFeedback(question, answer, user_id);
                std::cout << "Thank you for the positive feedback!\n";
            } else if (feedback == "n") {
                std::cout << "Please provide the correct answer: ";
                std::string correction;
                std::getline(std::cin, correction);
                assistant_->addNegativeFeedback(question, answer, correction, user_id);
                std::cout << "Thank you for the correction!\n";
            }
        }
        
        // Show statistics
        auto metrics = assistant_->getMetrics();
        auto feedback_stats = assistant_->getFeedbackStats();
        
        std::cout << "\n=== Session Statistics ===\n";
        std::cout << "Total queries: " << metrics.total_queries << "\n";
        std::cout << "Success rate: " << (metrics.success_rate * 100) << "%\n";
        std::cout << "Feedback collected: " << feedback_stats.total_feedback << "\n";
        std::cout << "Positive feedback: " << feedback_stats.positive_feedback << "\n";
    }
    
    void batchProcessing(const std::vector<std::string>& questions) {
        std::string user_id = "batch_processor";
        
        for (const auto& question : questions) {
            std::cout << "\nQ: " << question << "\n";
            std::string answer = assistant_->query(question, user_id);
            std::cout << "A: " << answer << "\n";
            std::cout << "---\n";
        }
    }
};

int main() {
    try {
        DocumentationAssistantApp app;
        app.initialize();
        
        // Interactive mode
        app.interactiveMode();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### Feedback-Driven Training

```cpp
#include "llm/applications/themis_help_lora.h"
#include <thread>
#include <chrono>

class FeedbackTrainingService {
private:
    std::shared_ptr<ThemisHelpLoRA> assistant_;
    std::thread training_thread_;
    std::atomic<bool> running_{true};
    
public:
    void start() {
        training_thread_ = std::thread(&FeedbackTrainingService::trainingLoop, this);
    }
    
    void stop() {
        running_ = false;
        if (training_thread_.joinable()) {
            training_thread_.join();
        }
    }
    
private:
    void trainingLoop() {
        while (running_) {
            // Check feedback count every hour
            std::this_thread::sleep_for(std::chrono::hours(1));
            
            auto feedback_stats = assistant_->getFeedbackStats();
            
            // Trigger retraining if threshold reached
            if (feedback_stats.total_feedback >= 100) {
                spdlog::info("Feedback threshold reached, starting retraining...");
                
                std::string old_version = assistant_->getVersion();
                bool success = assistant_->trainFromFeedback();
                
                if (success) {
                    std::string new_version = assistant_->getVersion();
                    spdlog::info("Retraining successful! {} -> {}", 
                               old_version, new_version);
                    
                    // Validate new version
                    if (!validateNewVersion(new_version)) {
                        spdlog::warn("New version failed validation, rolling back");
                        assistant_->rollbackToPreviousVersion();
                    }
                } else {
                    spdlog::error("Retraining failed");
                }
            }
        }
    }
    
    bool validateNewVersion(const std::string& version) {
        // Test with known questions
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"How to enable sharding?", "SHARD BY"},
            {"What is replication?", "REPLICATION"},
            {"Configure backups", "backup"}
        };
        
        int passed = 0;
        for (const auto& [question, expected_keyword] : test_cases) {
            std::string answer = assistant_->query(question, "validator");
            if (answer.find(expected_keyword) != std::string::npos) {
                passed++;
            }
        }
        
        float accuracy = static_cast<float>(passed) / test_cases.size();
        return accuracy >= 0.8f; // 80% threshold
    }
};

int main() {
    ThemisHelpLoRA::Config config;
    config.adapter_id = "themis_help_lora";
    config.base_model_id = "llama-2-7b";
    
    auto assistant = std::make_shared<ThemisHelpLoRA>(config);
    
    FeedbackTrainingService service;
    service.start();
    
    // Run your application...
    
    service.stop();
    return 0;
}
```

---

## REST API Examples

### Create Adapter

```bash
#!/bin/bash

# Set authentication token
export THEMIS_TOKEN="your_jwt_token_here"
export THEMIS_URL="https://your-themisdb-server.com"

# Create adapter
curl -X POST "$THEMIS_URL/api/v1/llm/lora/create" \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "my_documentation_assistant",
    "base_model": "llama-2-7b",
    "training_data": {
      "task_type": "documentation_qa",
      "samples": [
        {
          "input": "How do I enable sharding?",
          "output": "To enable sharding in ThemisDB:\n1. Use CREATE COLLECTION\n2. Add SHARD BY clause\n3. Specify number of shards\n\nExample: CREATE COLLECTION mydata SHARD BY user_id SHARDS 8;"
        },
        {
          "input": "What is replication?",
          "output": "Replication in ThemisDB creates multiple copies of data across nodes for fault tolerance and high availability. Configure with REPLICATION factor."
        }
      ]
    },
    "hyperparameters": {
      "rank": 8,
      "alpha": 16,
      "learning_rate": 0.0003,
      "epochs": 3,
      "batch_size": 32
    }
  }'

# Response:
# {
#   "job_id": "job_abc123",
#   "adapter_id": "my_documentation_assistant",
#   "status": "pending",
#   "estimated_completion": "2026-01-11T16:30:00Z"
# }
```

### Get Adapter Information

```bash
# Get adapter details
curl -X GET "$THEMIS_URL/api/v1/llm/lora/my_documentation_assistant" \
  -H "Authorization: Bearer $THEMIS_TOKEN"

# Response:
# {
#   "adapter_id": "my_documentation_assistant",
#   "base_model": "llama-2-7b",
#   "version": "v1.0",
#   "status": "ready",
#   "training_samples": 150,
#   "validation_accuracy": 0.87,
#   "created_at": "2026-01-11T15:00:00Z",
#   "updated_at": "2026-01-11T15:30:00Z"
# }
```

### List Adapters

```bash
# List all adapters
curl -X GET "$THEMIS_URL/api/v1/llm/lora/list" \
  -H "Authorization: Bearer $THEMIS_TOKEN"

# List adapters for specific base model
curl -X GET "$THEMIS_URL/api/v1/llm/lora/list?base_model=llama-2-7b" \
  -H "Authorization: Bearer $THEMIS_TOKEN"

# Response:
# {
#   "adapters": [
#     {
#       "adapter_id": "my_documentation_assistant",
#       "base_model": "llama-2-7b",
#       "version": "v1.0",
#       "status": "ready"
#     },
#     {
#       "adapter_id": "sql_translator",
#       "base_model": "llama-2-7b",
#       "version": "v2.1",
#       "status": "ready"
#     }
#   ],
#   "total": 2
# }
```

### Query with Adapter

```bash
# Query documentation assistant
curl -X POST "$THEMIS_URL/api/v1/llm/docs/query" \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I configure replication in ThemisDB?"
  }'

# Response:
# {
#   "question": "How do I configure replication in ThemisDB?",
#   "answer": "To configure replication in ThemisDB:\n\n1. Set replicationFactor in collection definition\n2. Recommended: Use 3 replicas for production\n3. Monitor replica health using ADMIN_HEALTH()\n4. Configure failover policies\n\nExample: CREATE COLLECTION mydata REPLICATION 3;",
#   "adapter_used": "themis_help_lora",
#   "adapter_version": "v1.0",
#   "response_time_ms": 234
# }
```

### Provide Feedback

```bash
# Positive feedback
curl -X POST "$THEMIS_URL/api/v1/llm/docs/feedback" \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I configure replication?",
    "answer": "To configure replication...",
    "feedback_type": "positive"
  }'

# Negative feedback with correction
curl -X POST "$THEMIS_URL/api/v1/llm/docs/feedback" \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I configure replication?",
    "answer": "Incorrect answer here...",
    "feedback_type": "negative",
    "correction": "The correct answer is: CREATE COLLECTION mydata REPLICATION 3;"
  }'
```

### Update Adapter (Retrain)

```bash
# Retrain with additional data
curl -X PUT "$THEMIS_URL/api/v1/llm/lora/my_documentation_assistant" \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "additional_training_data": {
      "samples": [
        {
          "input": "How to configure backups?",
          "output": "ThemisDB backup strategies..."
        }
      ]
    }
  }'

# Response:
# {
#   "job_id": "job_xyz789",
#   "adapter_id": "my_documentation_assistant",
#   "old_version": "v1.0",
#   "new_version": "v1.1",
#   "status": "training"
# }
```

### Delete Adapter

```bash
# Delete adapter
curl -X DELETE "$THEMIS_URL/api/v1/llm/lora/my_documentation_assistant" \
  -H "Authorization: Bearer $THEMIS_TOKEN"

# Response:
# {
#   "adapter_id": "my_documentation_assistant",
#   "deleted": true
# }
```

---

## AQL Function Examples

### Basic Training

```aql
// Train a new adapter from dataset
LET result = LORA_TRAIN(
  "documentation_assistant",
  "llama-2-7b",
  {
    "task": "documentation_qa",
    "samples": [
      {
        "input": "How do I enable sharding?",
        "output": "To enable sharding in ThemisDB..."
      },
      {
        "input": "What is replication?",
        "output": "Replication in ThemisDB..."
      }
    ]
  },
  {
    "rank": 8,
    "alpha": 16,
    "learning_rate": 0.0003,
    "epochs": 3
  }
)

RETURN result
```

### Batch Training from Collection

```aql
// Train adapters from stored datasets
FOR dataset IN training_datasets
  FILTER dataset.status == "ready"
  FILTER dataset.sample_count >= 100
  
  LET adapter_id = CONCAT(dataset.task_type, "_lora_", DATE_FORMAT(DATE_NOW(), "%Y%m%d"))
  
  LET job = LORA_TRAIN(
    adapter_id,
    dataset.base_model,
    dataset,
    {
      "rank": dataset.config.rank || 8,
      "alpha": dataset.config.alpha || 16,
      "learning_rate": 0.0003,
      "epochs": 3
    }
  )
  
  RETURN {
    adapter_id: adapter_id,
    job_id: job.job_id,
    status: job.status,
    estimated_completion: job.estimated_completion
  }
```

### Query with Adapter

```aql
// Single query
LET answer = LORA_QUERY(
  "llama-2-7b",
  "documentation_assistant",
  "How do I configure backups in ThemisDB?",
  {
    "max_tokens": 500,
    "temperature": 0.7
  }
)

RETURN {
  question: "How do I configure backups in ThemisDB?",
  answer: answer,
  timestamp: DATE_NOW()
}
```

### Batch Query Processing

```aql
// Process multiple questions
FOR question IN user_questions
  FILTER question.status == "pending"
  FILTER question.category == "documentation"
  
  LET answer = LORA_QUERY(
    "llama-2-7b",
    "documentation_assistant",
    question.text,
    {"max_tokens": 500}
  )
  
  UPDATE question WITH {
    answer: answer,
    status: "answered",
    answered_at: DATE_NOW()
  } IN user_questions
  
  RETURN {
    question_id: question._key,
    question: question.text,
    answer: answer
  }
```

### Find Similar Adapters

```aql
// Find adapters similar to a specific adapter
LET similar = LORA_SIMILAR("documentation_assistant", 5, 0.85)

FOR adapter IN similar
  LET stats = LORA_STATS(adapter.adapter_id, ["validation_accuracy", "inference_count"])
  RETURN {
    adapter_id: adapter.adapter_id,
    similarity_score: adapter.score,
    task: adapter.task,
    accuracy: stats.validation_accuracy,
    usage_count: stats.inference_count
  }
```

### Adapter Performance Analysis

```aql
// Analyze and rank adapters by performance
FOR adapter IN lora_adapters
  FILTER adapter.base_model == "llama-2-7b"
  FILTER adapter.status == "ready"
  
  LET stats = LORA_STATS(
    adapter.adapter_id,
    ["validation_accuracy", "inference_count", "avg_latency", "cache_hit_rate"]
  )
  
  // Calculate composite performance score
  LET performance_score = (
    stats.validation_accuracy * 0.5 +
    MIN([stats.cache_hit_rate, 1.0]) * 0.3 +
    (1.0 - MIN([stats.avg_latency_ms / 1000, 1.0])) * 0.2
  )
  
  SORT performance_score DESC
  LIMIT 10
  
  RETURN {
    rank: ROW_NUMBER(),
    adapter_id: adapter.adapter_id,
    task: adapter.task,
    performance_score: ROUND(performance_score, 3),
    accuracy: ROUND(stats.validation_accuracy, 3),
    latency_ms: stats.avg_latency_ms,
    usage: stats.inference_count
  }
```

### Adaptive Query Routing

```aql
// Automatically select best adapter for query
FOR query IN user_queries
  // Get recommendation for best adapter
  LET recommendation = LORA_RECOMMEND(
    query.text,
    query.preferred_model || "llama-2-7b",
    query.category,
    {
      "min_accuracy": 0.85,
      "max_latency_ms": 100
    }
  )
  
  // Execute query with recommended adapter
  LET answer = recommendation.adapter_id != null 
    ? LORA_QUERY(
        query.preferred_model || "llama-2-7b",
        recommendation.adapter_id,
        query.text,
        {}
      )
    : "No suitable adapter found"
  
  RETURN {
    query_id: query._key,
    query: query.text,
    answer: answer,
    adapter_used: recommendation.adapter_id,
    confidence: recommendation.confidence,
    reason: recommendation.reason
  }
```

---

## Python Client Examples

### Basic Usage

```python
from themisdb import Client
from themisdb.lora import LoRAManager

# Connect to ThemisDB
client = Client(
    url="https://your-themisdb-server.com",
    token="your_jwt_token"
)

# Initialize LoRA manager
lora = LoRAManager(client)

# Create adapter
job_id = lora.create_adapter(
    adapter_id="my_adapter",
    base_model="llama-2-7b",
    training_data={
        "task_type": "documentation_qa",
        "samples": [
            {
                "input": "How do I enable sharding?",
                "output": "To enable sharding..."
            }
        ]
    },
    hyperparameters={
        "rank": 8,
        "alpha": 16,
        "learning_rate": 0.0003,
        "epochs": 3
    }
)

print(f"Training job started: {job_id}")

# Wait for completion
status = lora.wait_for_job(job_id)
if status == "completed":
    print("Training completed successfully!")
else:
    print(f"Training failed: {status}")
```

### Documentation Assistant

```python
class DocumentationAssistant:
    def __init__(self, client, adapter_id="themis_help_lora"):
        self.client = client
        self.adapter_id = adapter_id
        self.lora = LoRAManager(client)
    
    def query(self, question):
        """Ask a documentation question"""
        response = self.client.post("/api/v1/llm/docs/query", {
            "question": question
        })
        return response["answer"]
    
    def feedback(self, question, answer, is_positive, correction=None):
        """Provide feedback on an answer"""
        data = {
            "question": question,
            "answer": answer,
            "feedback_type": "positive" if is_positive else "negative"
        }
        if correction:
            data["correction"] = correction
        
        self.client.post("/api/v1/llm/docs/feedback", data)
    
    def interactive(self):
        """Interactive Q&A session"""
        print("ThemisDB Documentation Assistant")
        print("Type 'quit' to exit\n")
        
        while True:
            question = input("Your question: ")
            if question.lower() == "quit":
                break
            
            if not question.strip():
                continue
            
            # Get answer
            answer = self.query(question)
            print(f"\nAnswer:\n{answer}\n")
            
            # Collect feedback
            feedback = input("Was this helpful? (y/n/c for correction): ")
            if feedback.lower() == "y":
                self.feedback(question, answer, True)
                print("Thank you for the feedback!\n")
            elif feedback.lower() == "n":
                correction = input("Please provide the correct answer: ")
                self.feedback(question, answer, False, correction)
                print("Thank you for the correction!\n")

# Usage
client = Client(url="https://themisdb.example.com", token="...")
assistant = DocumentationAssistant(client)
assistant.interactive()
```

### Batch Processing

```python
import pandas as pd

# Load questions from CSV
questions_df = pd.read_csv("questions.csv")

# Process batch
results = []
for _, row in questions_df.iterrows():
    question = row["question"]
    
    # Query
    answer = assistant.query(question)
    
    # Store result
    results.append({
        "question_id": row["id"],
        "question": question,
        "answer": answer
    })

# Save results
results_df = pd.DataFrame(results)
results_df.to_csv("answers.csv", index=False)
```

### Training from Feedback

```python
class FeedbackTrainer:
    def __init__(self, client, adapter_id):
        self.client = client
        self.adapter_id = adapter_id
        self.lora = LoRAManager(client)
    
    def collect_feedback(self):
        """Get feedback from database"""
        # Query feedback collection
        aql = """
        FOR feedback IN feedback_collection
            FILTER feedback.adapter_id == @adapter_id
            FILTER feedback.used_for_training == false
            RETURN feedback
        """
        
        result = self.client.aql(aql, {"adapter_id": self.adapter_id})
        return result["result"]
    
    def retrain(self):
        """Retrain adapter from feedback"""
        feedback = self.collect_feedback()
        
        if len(feedback) < 100:
            print(f"Not enough feedback: {len(feedback)}/100")
            return False
        
        # Convert feedback to training data
        training_data = {
            "samples": []
        }
        
        for item in feedback:
            if not item["is_positive"] and item["correction"]:
                training_data["samples"].append({
                    "input": item["question"],
                    "output": item["correction"]
                })
            elif item["is_positive"]:
                training_data["samples"].append({
                    "input": item["question"],
                    "output": item["answer"]
                })
        
        # Retrain
        job_id = self.lora.update_adapter(
            self.adapter_id,
            training_data
        )
        
        # Wait for completion
        status = self.lora.wait_for_job(job_id)
        
        if status == "completed":
            # Mark feedback as used
            self.mark_feedback_used(feedback)
            print("Retraining completed successfully!")
            return True
        else:
            print(f"Retraining failed: {status}")
            return False
    
    def mark_feedback_used(self, feedback):
        """Mark feedback as used for training"""
        for item in feedback:
            aql = """
            UPDATE @key WITH {used_for_training: true} IN feedback_collection
            """
            self.client.aql(aql, {"key": item["_key"]})

# Usage
trainer = FeedbackTrainer(client, "themis_help_lora")
trainer.retrain()
```

---

## Complete Application Examples

### CLI Tool

```bash
#!/bin/bash
# themis-docs - CLI tool for ThemisDB documentation assistant

THEMIS_URL="https://your-server.com"
THEMIS_TOKEN="your_token"

case "$1" in
  ask)
    # Ask a question
    curl -s -X POST "$THEMIS_URL/api/v1/llm/docs/query" \
      -H "Authorization: Bearer $THEMIS_TOKEN" \
      -H "Content-Type: application/json" \
      -d "{\"question\": \"$2\"}" | jq -r '.answer'
    ;;
  
  feedback)
    # Provide feedback
    curl -s -X POST "$THEMIS_URL/api/v1/llm/docs/feedback" \
      -H "Authorization: Bearer $THEMIS_TOKEN" \
      -H "Content-Type: application/json" \
      -d "{\"question\": \"$2\", \"answer\": \"$3\", \"feedback_type\": \"$4\"}"
    echo "Feedback submitted"
    ;;
  
  stats)
    # Show statistics
    curl -s -X GET "$THEMIS_URL/api/v1/llm/docs/stats" \
      -H "Authorization: Bearer $THEMIS_TOKEN" | jq
    ;;
  
  *)
    echo "Usage: themis-docs {ask|feedback|stats}"
    ;;
esac
```

### Web Application (Node.js/Express)

```javascript
const express = require('express');
const axios = require('axios');

const app = express();
app.use(express.json());

const THEMIS_URL = process.env.THEMIS_URL;
const THEMIS_TOKEN = process.env.THEMIS_TOKEN;

// Query endpoint
app.post('/api/query', async (req, res) => {
  try {
    const { question } = req.body;
    
    const response = await axios.post(
      `${THEMIS_URL}/api/v1/llm/docs/query`,
      { question },
      {
        headers: {
          'Authorization': `Bearer ${THEMIS_TOKEN}`,
          'Content-Type': 'application/json'
        }
      }
    );
    
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Feedback endpoint
app.post('/api/feedback', async (req, res) => {
  try {
    const { question, answer, feedback_type, correction } = req.body;
    
    await axios.post(
      `${THEMIS_URL}/api/v1/llm/docs/feedback`,
      { question, answer, feedback_type, correction },
      {
        headers: {
          'Authorization': `Bearer ${THEMIS_TOKEN}`,
          'Content-Type': 'application/json'
        }
      }
    );
    
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.listen(3000, () => {
  console.log('Documentation assistant running on port 3000');
});
```

### Telegram Bot

```python
import os
from telegram import Update
from telegram.ext import Application, CommandHandler, MessageHandler, filters
import requests

THEMIS_URL = os.getenv("THEMIS_URL")
THEMIS_TOKEN = os.getenv("THEMIS_TOKEN")
BOT_TOKEN = os.getenv("BOT_TOKEN")

def query_themisdb(question):
    """Query ThemisDB documentation assistant"""
    response = requests.post(
        f"{THEMIS_URL}/api/v1/llm/docs/query",
        headers={
            "Authorization": f"Bearer {THEMIS_TOKEN}",
            "Content-Type": "application/json"
        },
        json={"question": question}
    )
    return response.json()["answer"]

async def start(update: Update, context):
    """Start command handler"""
    await update.message.reply_text(
        "Welcome to ThemisDB Documentation Assistant!\n"
        "Ask me anything about ThemisDB."
    )

async def handle_message(update: Update, context):
    """Handle user messages"""
    question = update.message.text
    
    # Show typing indicator
    await update.message.chat.send_action("typing")
    
    # Query
    answer = query_themisdb(question)
    
    # Send answer
    await update.message.reply_text(answer)

def main():
    """Run the bot"""
    application = Application.builder().token(BOT_TOKEN).build()
    
    application.add_handler(CommandHandler("start", start))
    application.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, handle_message))
    
    application.run_polling()

if __name__ == "__main__":
    main()
```

---

## Summary

These examples demonstrate:
- ✅ Complete C++ integration with LoRA framework
- ✅ REST API usage for all operations
- ✅ AQL functions for query-based access
- ✅ Python client library examples
- ✅ Complete application examples (CLI, web, bot)

**Next Steps:**
- Review [Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md)
- See [Training Guide](LORA_TRAINING_GUIDE.md)
- Check [AQL Reference](../../../LORA_AQL_REFERENCE.md)

---

**Last Updated**: 2026-04-06  
**Version**: 1.0
