# LLM Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** LLM

---

## Übersicht

Das LLM-Modul bietet Speicherung und Verwaltung von LLM-Interaktionen, Prompt-Templates und Chain-of-Thought (CoT) Reasoning.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| LLMInteractionStore | `llm_interaction_store.h` | `llm_interaction_store.cpp` | Interaction Storage |
| PromptManager | `prompt_manager.h` | `prompt_manager.cpp` | Prompt Templates |

**Gesamt:** 2 Header, 2 Source-Dateien, ~700 LOC

## Implementierte Klassen

### LLMInteractionStore

```cpp
class LLMInteractionStore {
    // Key format: "llm_interaction:{interaction_id}"
    
    struct Interaction {
        std::string id;                        // UUID
        std::string prompt_template_id;        // Referenz auf Template
        std::string prompt;                    // Gesendeter Prompt
        std::vector<std::string> reasoning_chain; // Chain-of-Thought Steps
        std::string response;                  // LLM Response
        std::string model_version;             // e.g., "gpt-4o-mini"
        int64_t timestamp_ms;
        int latency_ms;
        int token_count;
        nlohmann::json metadata;               // Feedback, user_id, etc.
    };
    
    struct ListOptions {
        size_t limit = 100;
        std::optional<std::string> start_after_id;  // Pagination
        std::optional<std::string> filter_model;
        std::optional<int64_t> since_timestamp_ms;
    };
    
    struct Stats {
        size_t total_interactions;
        int64_t total_tokens;
        double avg_latency_ms;
        size_t total_size_bytes;
    };
    
    // API
    Interaction createInteraction(Interaction);
    std::optional<Interaction> getInteraction(id);
    std::vector<Interaction> listInteractions(ListOptions);
    bool deleteInteraction(id);
    Stats getStats();
};
```

### PromptManager

```cpp
class PromptManager {
    struct PromptTemplate {
        std::string id;
        std::string name;
        std::string template_text;
        std::string version;
        nlohmann::json variables;     // {name, description, required}
        int64_t created_at;
        int64_t updated_at;
    };
    
    // API
    PromptTemplate createTemplate(PromptTemplate);
    std::optional<PromptTemplate> getTemplate(id);
    std::vector<PromptTemplate> listTemplates();
    PromptTemplate updateTemplate(id, updates);
    bool deleteTemplate(id);
    
    // Rendering
    std::string render(template_id, variables);
};
```

## Features

### Chain-of-Thought Storage

```cpp
Interaction interaction;
interaction.reasoning_chain = {
    "Step 1: Parse the user query",
    "Step 2: Identify relevant documents",
    "Step 3: Generate response based on context"
};
store.createInteraction(interaction);
```

### Token & Latency Tracking

```cpp
interaction.token_count = 150;
interaction.latency_ms = 450;

auto stats = store.getStats();
// stats.avg_latency_ms = 380.5
// stats.total_tokens = 125000
```

### Prompt Versioning

```cpp
auto template = manager.createTemplate({
    .name = "qa_template",
    .template_text = "Question: {{question}}\nContext: {{context}}\nAnswer:",
    .version = "1.0.0",
    .variables = {{"question", {.required = true}}, {"context", {.required = true}}}
});

auto prompt = manager.render(template.id, {
    {"question", "What is ThemisDB?"},
    {"context", "ThemisDB is a multi-model database..."}
});
```

## HTTP API

### POST /api/llm/interactions
```json
{
  "prompt": "What is the capital of France?",
  "response": "The capital of France is Paris.",
  "model_version": "gpt-4o-mini",
  "reasoning_chain": ["Parse query", "Lookup knowledge", "Generate response"],
  "token_count": 25,
  "latency_ms": 200
}
```

### GET /api/llm/interactions?limit=10&filter_model=gpt-4o-mini
### GET /api/llm/stats

### PATCH /llm/interaction/{id} - Update Metadata (Enterprise)
```json
{
  "feedback": {
    "rating": 5,
    "feedback_text": "Excellent response",
    "user_id": "user123",
    "flagged_for_training": true,
    "training_category": "positive"
  }
}
```

### POST /query/enhanced - Enhanced Query with LLM Context (Enterprise)
```json
{
  "aql": "FOR doc IN products FILTER doc.category == 'electronics' RETURN doc",
  "llm_context": {
    "limit": 5,
    "model": "gpt-4o-mini"
  }
}
```

## Enterprise Features

### Feedback-System
Das Feedback-System ist als Enterprise Add-on implementiert und nutzt das flexible `metadata`-Feld. Es erfordert **keinen separaten Layer**.

**Use Cases:**
- User-Feedback für LLM-Antworten sammeln
- Trainingsdaten für LoRa Fine-Tuning markieren
- Qualitätsmetriken tracking

**Siehe:** [LLM Feedback Enterprise](./LLM_FEEDBACK_ENTERPRISE.md)

### Query Enhancement
Kombiniert DB-Abfragen mit LLM-Kontext für KI-gestützte Anwendungen.

**Vorteile:**
- 49% Kostenreduktion
- 38% Latenz-Verbesserung
- +25-40% Qualitätsverbesserung
- Real-time Feedback-Loop

**Siehe:** [LLM Integration Benefits Analysis](../enterprise/LLM_INTEGRATION_BENEFITS_ANALYSIS.md)

## Verwandte Dokumentation

- [Features: Semantic Cache](../features/features_semantic_cache.md) - LLM Response Caching
- [Projects: RAG LLM](../projects/RAG_LLM_PROGRAMMIERHILFE.md) - RAG Integration

## Verwandte Dokumentation

- [AQL Prompt Engineering](../AQL_PROMPT_ENGINEERING.md)
- [Projects: RAG LLM Programmierhilfe](../projects/RAG_LLM_PROGRAMMIERHILFE.md)
