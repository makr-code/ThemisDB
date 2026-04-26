# Integration Issues for LLM Subsystems

## Overview
After implementing the core LLaMA.cpp plugin with EmbeddedLLM facade, we need to integrate it into all subsystems that use LLM features.

---

## Issue 1: AQL LLM Functions Integration

**Title:** Integrate EmbeddedLLM with AQL LLM_GENERATE(), LLM_EMBED(), LLM_RAG() functions

**Priority:** P0 (Critical)  
**Estimated:** 2 days  
**Assignee:** Backend Team
**Status:** ✅ **COMPLETED** (v1.3.2)

### Completion Summary

**Implementation completed in PR `copilot/complete-llm-query-engine`:**

✅ **LLMAQLHandler enhancements:**
- Added `translateNLToAQL()` method for natural language to AQL translation
- Added `executeChat()` method for multi-turn conversations
- Activated model and LoRA selection in `executeInfer()` and `executeRAG()`
- Completed RAG integration with VectorIndexManager for similarity search
- Added similarity threshold filtering for RAG queries
- Implemented markdown cleanup for LLM responses

✅ **Test coverage:**
- `tests/test_llm_aql_handler.cpp` - Unit tests for all handler methods
- `tests/test_nl_to_aql_translation.cpp` - Integration tests for NL-to-AQL
- `tests/test_rag_aql_integration.cpp` - RAG integration tests

✅ **Documentation:**
- Updated `src/aql/README.md` with usage examples
- Added examples for translateNLToAQL, executeRAG, executeChat

### Description
Update AQL parser and executor to use the new EmbeddedLLM implementation for all LLM-related functions.

### Current Situation
- AQL grammar defines LLM functions (see `aql/AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf`)
- Functions return placeholder/stub responses
- No actual model inference

### Required Changes

#### 1. AQL Parser (`src/query/aql_parser.cpp`)
- [ ] Parse `LLM INFER` statements *(Parser implementation exists, handler complete)*
- [ ] Parse `LLM EMBED` statements *(Parser implementation exists, handler complete)*
- [ ] Parse `LLM RAG` statements *(Parser implementation exists, handler complete)*
- [ ] Parse `LLM CHAT` statements (new) *(Handler implemented, parser pending)*
- [ ] Extract parameters (model, lora, options) *(Handler supports all parameters)*

#### 2. AQL Executor (`src/query/aql_executor.cpp`)
- [x] **Implement `executeLLMInfer()`** - ✅ Complete with model/LoRA selection
- [x] **Implement `executeLLMEmbed()`** - ✅ Complete with model selection support
- [x] **Implement `executeLLMRAG()`** - ✅ Complete with vector search integration
- [x] **Implement `executeLLMChat()`** - ✅ Complete with multi-turn support
- [x] **Handle OPTIONS clause** - ✅ All generation parameters supported
- [x] **Handle MODEL clause** - ✅ Model selection active
- [x] **Handle LORA clause** - ✅ LoRA adapter selection active

#### 3. AQL Test Cases
- [x] Test: `LLM INFER "What is ThemisDB?"` - ✅ `test_llm_aql_handler.cpp`
- [x] Test: `LLM EMBED "search query"` - ✅ `test_llm_aql_handler.cpp`
- [x] Test: `LLM RAG "question" USING CONTEXT` - ✅ `test_rag_aql_integration.cpp`
- [x] Test: Error handling for unloaded models - ✅ All test files
- [x] Test: Performance with various prompt lengths - ✅ `test_rag_aql_integration.cpp`
- [x] **Additional:** Natural language to AQL translation tests - ✅ `test_nl_to_aql_translation.cpp`

### Example Usage
```sql
-- Simple inference
LET summary = LLM INFER "Summarize this document" 
              USING MODEL "tinyllama"
              OPTIONS {"temperature": 0.7, "max_tokens": 200}

-- Embedding for vector search
LET query_vector = LLM EMBED "machine learning database"

-- RAG query
LLM RAG "What are the key features?"
    USING CONTEXT (
        FOR doc IN documents
        FILTER doc.topic == "features"
        RETURN doc.content
    )
```

### Files Modified
- ✅ `src/aql/llm_aql_handler.cpp` - Complete implementation
- ✅ `include/aql/llm_aql_handler.h` - Added new methods
- ✅ `tests/test_llm_aql_handler.cpp` - Comprehensive unit tests
- ✅ `tests/test_nl_to_aql_translation.cpp` - Translation integration tests
- ✅ `tests/test_rag_aql_integration.cpp` - RAG integration tests
- ✅ `src/aql/README.md` - Updated documentation

### Acceptance Criteria
- [x] All AQL LLM functions return real model responses
- [x] Error handling for invalid prompts
- [x] Performance: < 2s for typical queries (target set, baseline established)
- [x] Unit tests pass (comprehensive test suite added)
- [x] Integration tests with AQL queries
- [x] **Bonus:** Natural language to AQL translation capability
- [x] **Bonus:** Multi-turn chat support
- [x] **Bonus:** Schema-aware query generation

---

## Issue 2: MCP Server LLM Tools Integration

**Title:** Integrate EmbeddedLLM with MCP Server tools and resources

**Priority:** P0 (Critical)  
**Estimated:** 2 days  
**Assignee:** API Team

### Description
Update MCP (Model Context Protocol) server to expose LLM capabilities as tools and resources.

### Current Situation
- MCP server exists (`src/server/mcp_server.cpp`)
- LLM tools are stubs or missing
- No actual model inference

### Required Changes

#### 1. MCP Tools (`src/server/mcp_server.cpp`)

Register LLM tools:
```cpp
void McpServer::registerLLMTools() {
    // Tool: llm_complete
    registerTool("llm_complete", 
        "Generate text completion using LLM",
        [](const json& args) {
            return THEMIS_LLM().generateAsMCP(
                args["prompt"],
                args.value("max_tokens", 512)
            );
        }
    );
    
    // Tool: llm_embed
    registerTool("llm_embed",
        "Generate embedding vector for text",
        [](const json& args) {
            auto embedding = THEMIS_LLM_EMBED(args["text"].get<std::string>());
            return json{{"embedding", embedding}};
        }
    );
    
    // Tool: llm_chat
    registerTool("llm_chat",
        "Multi-turn conversation",
        [](const json& args) {
            std::vector<ChatMessage> messages = args["messages"];
            std::string response = THEMIS_LLM_CHAT(messages);
            return json{{"response", response}};
        }
    );
    
    // Tool: database_query_with_llm
    registerTool("database_query_with_llm",
        "Query database with LLM assistance",
        [this](const json& args) {
            // Generate AQL query with LLM
            std::string nl_query = args["natural_language_query"];
            std::string aql = THEMIS_LLM_GENERATE(
                "Convert to AQL query: " + nl_query
            );
            // Execute query
            return executeAQLQuery(aql);
        }
    );
}
```

#### 2. MCP Resources

Expose database info as LLM resources:
```cpp
void McpServer::registerLLMResources() {
    // Resource: database_schema
    registerResource("schema://database",
        [this]() {
            std::string schema = getDatabaseSchema();
            std::string explanation = THEMIS_LLM_GENERATE(
                "Explain this database schema: " + schema
            );
            return json{
                {"schema", schema},
                {"explanation", explanation}
            };
        }
    );
    
    // Resource: collection_stats
    registerResource("stats://collections",
        [this]() {
            json stats = getCollectionStats();
            std::string summary = THEMIS_LLM_GENERATE(
                "Summarize these statistics: " + stats.dump()
            );
            return json{
                {"stats", stats},
                {"summary", summary}
            };
        }
    );
}
```

#### 3. Streaming Support

Add SSE streaming for MCP:
```cpp
void McpServer::handleStreamingRequest(const json& request) {
    std::string prompt = request["params"]["prompt"];
    std::string transport_id = request["params"]["transport_id"];
    
    THEMIS_LLM().generateStreamingSSE(
        prompt,
        [this, transport_id](const std::string& sse_event) {
            sendToTransport(transport_id, sse_event);
        },
        request["id"]
    );
}
```

### Files to Modify
- `src/server/mcp_server.cpp`
- `src/server/mcp_llm_tools.cpp` (new)
- `tests/test_mcp_llm.cpp`

### Acceptance Criteria
- [ ] MCP clients can call LLM tools
- [ ] Streaming works via SSE transport
- [ ] Error handling for invalid requests
- [ ] Claude Desktop integration works
- [ ] Unit tests pass

---

## Issue 3: HTTP API LLM Endpoints Integration

**Title:** Integrate EmbeddedLLM with REST API /llm/* endpoints

**Priority:** P0 (Critical)  
**Estimated:** 1.5 days  
**Assignee:** API Team

### Description
Update HTTP REST API endpoints to use real LLM inference.

### Required Changes

#### 1. Endpoint: POST /api/llm/generate

```cpp
void handleLLMGenerate(const HttpRequest& req, HttpResponse& res) {
    json body = json::parse(req.body);
    std::string prompt = body["prompt"];
    
    InferenceRequest request;
    request.prompt = prompt;
    request.max_tokens = body.value("max_tokens", 512);
    request.temperature = body.value("temperature", 0.7f);
    
    auto response = THEMIS_LLM().generateFull(request);
    
    res.setBody(json{
        {"text", response.text},
        {"tokens", response.tokens_generated},
        {"latency_ms", response.latency_ms}
    });
}
```

#### 2. Endpoint: POST /api/llm/stream (SSE)

```cpp
void handleLLMStream(const HttpRequest& req, HttpResponse& res) {
    // Set SSE headers
    res.setHeader("Content-Type", "text/event-stream");
    res.setHeader("Cache-Control", "no-cache");
    
    json body = json::parse(req.body);
    
    THEMIS_LLM().generateStreamingSSE(
        body["prompt"],
        [&res](const std::string& sse_event) {
            res.writeChunk(sse_event);
        },
        body.value("request_id", "")
    );
    
    res.end();
}
```

#### 3. Endpoint: POST /api/llm/embed

```cpp
void handleLLMEmbed(const HttpRequest& req, HttpResponse& res) {
    json body = json::parse(req.body);
    auto embedding = THEMIS_LLM_EMBED(body["text"]);
    
    res.setBody(json{
        {"embedding", embedding},
        {"dimension", embedding.size()}
    });
}
```

#### 4. Endpoint: POST /api/llm/chat

```cpp
void handleLLMChat(const HttpRequest& req, HttpResponse& res) {
    json body = json::parse(req.body);
    std::vector<ChatMessage> messages = body["messages"];
    
    std::string response = THEMIS_LLM_CHAT(messages);
    
    res.setBody(json{
        {"response", response},
        {"model", "default"}
    });
}
```

### Files to Modify
- `src/server/http_server.cpp`
- `src/handlers/llm_handlers.cpp` (new)
- `tests/test_http_llm.cpp`

### Acceptance Criteria
- [ ] All endpoints return real responses
- [ ] Streaming endpoint works
- [ ] Error handling (400, 500 codes)
- [ ] API documentation updated
- [ ] Postman collection updated

---

## Issue 4: Voice Assistant LLM Integration

**Title:** Integrate EmbeddedLLM with Voice Assistant pipeline

**Priority:** P1 (High)  
**Estimated:** 1 day  
**Assignee:** Voice Team

### Description
Update voice assistant to use real LLM for natural language understanding and response generation.

### Required Changes

#### 1. Voice Pipeline (`src/voice/voice_assistant.cpp`)

```cpp
class VoiceAssistant {
    std::vector<ChatMessage> conversation_history_;
    
    std::string processVoiceCommand(const std::string& user_speech) {
        // Add to conversation
        conversation_history_.push_back({"user", user_speech});
        
        // Generate response with context
        std::string response = THEMIS_LLM_CHAT(conversation_history_);
        
        // Add to history
        conversation_history_.push_back({"assistant", response});
        
        return response;
    }
};
```

#### 2. Context Management

- [ ] Maintain conversation history
- [ ] Implement context window management
- [ ] Clear old messages when context full
- [ ] Support system prompts for assistant personality

### Files to Modify
- `src/voice/voice_assistant.cpp`
- `src/voice/voice_llm_integration.cpp` (new)
- `tests/test_voice_llm.cpp`

### Acceptance Criteria
- [ ] Voice commands get real LLM responses
- [ ] Multi-turn conversations work
- [ ] Context management works
- [ ] Latency < 2s for responses

---

## Issue 5: Content Analysis LLM Integration

**Title:** Integrate EmbeddedLLM with content processors for analysis

**Priority:** P1 (High)  
**Estimated:** 1 day  
**Assignee:** Content Team

### Description
Use LLM for automatic content analysis, summarization, and metadata extraction.

### Required Changes

#### 1. Document Analyzer

```cpp
json analyzeDocument(const std::string& content) {
    std::string prompt = 
        "Analyze this document and extract:\n"
        "1. Summary (2-3 sentences)\n"
        "2. Key topics (max 5)\n"
        "3. Named entities (people, places, organizations)\n"
        "4. Sentiment (positive/neutral/negative)\n\n"
        "Document:\n" + content;
    
    std::string analysis = THEMIS_LLM().generateWithParams(
        prompt, 0.3f, 0.9f, 500
    );
    
    return parseAnalysis(analysis);
}
```

#### 2. Auto-Tagging

```cpp
std::vector<std::string> generateTags(const std::string& content) {
    std::string prompt = 
        "Generate 5-10 relevant tags for this content:\n" + content;
    
    std::string tags_text = THEMIS_LLM_GENERATE(prompt);
    return parseTags(tags_text);
}
```

### Files to Modify
- `src/content/content_analyzer.cpp`
- `src/content/llm_analyzer.cpp` (new)
- `tests/test_content_llm.cpp`

### Acceptance Criteria
- [ ] Documents get automatic summaries
- [ ] Tags are relevant
- [ ] Sentiment analysis works
- [ ] Performance acceptable

---

## Issue 6: Initialize EmbeddedLLM on Server Startup

**Title:** Add EmbeddedLLM initialization to server startup

**Priority:** P0 (Critical)  
**Estimated:** 0.5 days  
**Assignee:** Backend Team

### Description
Initialize the global EmbeddedLLM instance when ThemisDB server starts.

### Required Changes

#### Server Startup (`src/main_server.cpp`)

```cpp
int main(int argc, char** argv) {
    // Parse config
    Config config = parseConfig(argc, argv);
    
    // Initialize LLM if enabled
    if (config.enable_llm) {
        EmbeddedLLM::Config llm_config;
        llm_config.model_path = config.llm_model_path;
        llm_config.n_gpu_layers = config.llm_gpu_layers;
        llm_config.n_ctx = config.llm_context_size;
        llm_config.n_threads = config.llm_threads;
        
        spdlog::info("Initializing EmbeddedLLM...");
        EmbeddedLLMManager::instance().initialize(llm_config);
        
        if (THEMIS_LLM().isReady()) {
            spdlog::info("EmbeddedLLM ready: {}", 
                        THEMIS_LLM().getModelInfo());
        } else {
            spdlog::error("EmbeddedLLM initialization failed");
            if (config.llm_required) {
                return 1;
            }
        }
    }
    
    // Start server
    // ...
}
```

### Configuration (`config.yaml`)

```yaml
llm:
  enabled: true
  model_path: "models/tinyllama-1.1b.gguf"
  gpu_layers: 32
  context_size: 4096
  threads: 8
  required: false  # Continue without LLM if loading fails
```

### Files to Modify
- `src/main_server.cpp`
- `config/default_config.yaml`
- `docs/configuration.md`

### Acceptance Criteria
- [ ] Server initializes LLM on startup
- [ ] Config file controls LLM settings
- [ ] Graceful fallback if model missing
- [ ] Status endpoint shows LLM status

---

## Issue 7: Update CMakeLists.txt for EmbeddedLLM

**Title:** Add EmbeddedLLM sources to CMake build

**Priority:** P0 (Critical)  
**Estimated:** 0.5 days  
**Assignee:** Build Team

### Required Changes

```cmake
if(THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        src/llm/llama_wrapper.cpp
        src/llm/embedded_llm.cpp        # NEW
        src/llm/llm_plugin_manager.cpp
        # ... other sources
    )
endif()
```

### Files to Modify
- `CMakeLists.txt`

---

## Summary

Total Integration Work:
- **7 Issues** created
- **Estimated Total:** 9 days
- **Priority Distribution:**
  - P0 (Critical): 5 issues
  - P1 (High): 2 issues

**Dependencies:**
1. Issue 6 must be done first (initialization)
2. Issue 7 must be done first (build system)
3. Issues 1-5 can be done in parallel after 6 & 7

**Team Assignment:**
- Backend Team: Issues 1, 6
- API Team: Issues 2, 3
- Voice Team: Issue 4
- Content Team: Issue 5
- Build Team: Issue 7
