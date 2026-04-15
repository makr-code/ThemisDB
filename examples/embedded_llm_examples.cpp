/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            embedded_llm_examples.cpp                          ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     283                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * EmbeddedLLM Integration Examples
 * 
 * Demonstrates how to use the embedded LLM interface throughout ThemisDB:
 * - AQL LLM_GENERATE() function implementation
 * - MCP protocol integration
 * - SSE streaming endpoints
 * - Content analysis
 * - Voice assistant integration
 */

#include "llm/embedded_llm.h"
#include <iostream>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Example 1: AQL LLM_GENERATE() Function Implementation
// ═══════════════════════════════════════════════════════════

/**
 * AQL Query: LLM INFER "Summarize the document"
 * 
 * This shows how AQL parser would call the embedded LLM
 */
std::string aql_llm_infer(const std::string& prompt, int max_tokens = 512) {
    try {
        // Direct access to embedded LLM
        return THEMIS_LLM_GENERATE(prompt);
        
        // Or with explicit parameters
        // return THEMIS_LLM().generateWithParams(prompt, 0.7f, 0.9f, max_tokens);
    } catch (const std::exception& e) {
        return std::string("[LLM Error: ") + e.what() + "]";
    }
}

/**
 * AQL Query: LLM EMBED "semantic search query"
 * 
 * Returns embedding vector for AQL vector operations
 */
std::vector<float> aql_llm_embed(const std::string& text) {
    return THEMIS_LLM_EMBED(text);
}

// ═══════════════════════════════════════════════════════════
// Example 2: MCP Protocol Handler
// ═══════════════════════════════════════════════════════════

/**
 * MCP tool handler: llm_complete
 * 
 * Handles MCP requests from Claude Desktop, Cline, etc.
 */
json mcp_tool_llm_complete(const json& args) {
    std::string prompt = args.value("prompt", "");
    int max_tokens = args.value("max_tokens", 512);
    
    try {
        // Generate and format as MCP response
        return THEMIS_LLM().generateAsMCP(prompt, max_tokens);
    } catch (const std::exception& e) {
        return json{
            {"error", e.what()},
            {"type", "llm_error"}
        };
    }
}

/**
 * MCP resource handler: database_schema
 * 
 * Uses LLM to explain database schema
 */
json mcp_resource_explain_schema(const std::string& schema_json) {
    std::string prompt = "Explain this database schema in simple terms:\n\n" + schema_json;
    
    return THEMIS_LLM().generateAsJsonMarkdown(prompt);
}

// ═══════════════════════════════════════════════════════════
// Example 3: SSE Streaming Endpoint
// ═══════════════════════════════════════════════════════════

/**
 * HTTP endpoint: POST /api/llm/stream
 * 
 * Streams LLM responses as Server-Sent Events
 */
void http_llm_stream_handler(
    const std::string& prompt,
    const std::string& request_id,
    std::function<void(const std::string&)> send_sse
) {
    // Set SSE headers
    send_sse("event: start\n");
    send_sse("data: {\"request_id\":\"" + request_id + "\"}\n\n");
    
    // Stream tokens as SSE events
    std::string full_text = THEMIS_LLM().generateStreamingSSE(
        prompt,
        send_sse,  // Callback receives SSE-formatted strings
        request_id
    );
    
    // Send completion event
    json done_event = {
        {"type", "done"},
        {"request_id", request_id},
        {"text", full_text}
    };
    send_sse("event: done\n");
    send_sse("data: " + done_event.dump() + "\n\n");
}

// ═══════════════════════════════════════════════════════════
// Example 4: Content Analysis
// ═══════════════════════════════════════════════════════════

/**
 * Analyze document content with LLM
 * 
 * Used by content processors for automatic summarization
 */
json analyze_document(const std::string& content) {
    std::string prompt = 
        "Analyze the following document and provide:\n"
        "1. A brief summary\n"
        "2. Key topics (max 5)\n"
        "3. Sentiment (positive/neutral/negative)\n\n"
        "Document:\n" + content;
    
    std::string analysis = THEMIS_LLM().generateWithParams(
        prompt,
        0.3f,  // Lower temperature for more focused analysis
        0.9f,
        500
    );
    
    return json{
        {"analysis", analysis},
        {"analyzed_at", std::time(nullptr)}
    };
}

// ═══════════════════════════════════════════════════════════
// Example 5: Voice Assistant Integration
// ═══════════════════════════════════════════════════════════

/**
 * Voice assistant query handler
 * 
 * Processes voice commands with context awareness
 */
std::string voice_assistant_query(
    const std::string& user_speech,
    const std::vector<ChatMessage>& conversation_history
) {
    // Add user message to history
    auto messages = conversation_history;
    messages.push_back({"user", user_speech});
    
    // Generate response with chat context
    return THEMIS_LLM_CHAT(messages);
}

// ═══════════════════════════════════════════════════════════
// Example 6: Batch Embedding for Vector Search
// ═══════════════════════════════════════════════════════════

/**
 * Create embeddings for multiple documents
 * 
 * Used for semantic search indexing
 */
std::vector<std::vector<float>> index_documents(const std::vector<std::string>& documents) {
    return THEMIS_LLM().embedBatch(documents);
}

/**
 * Semantic search query
 * 
 * Finds similar documents using embeddings
 */
std::vector<float> create_search_embedding(const std::string& query) {
    return THEMIS_LLM_EMBED(query);
}

// ═══════════════════════════════════════════════════════════
// Example 7: Initialization
// ═══════════════════════════════════════════════════════════

void initialize_embedded_llm(const std::string& model_path) {
    EmbeddedLLM::Config config;
    config.model_path = model_path;
    config.n_gpu_layers = 32;  // Use GPU if available
    config.n_ctx = 4096;
    config.n_threads = 8;
    config.enable_caching = true;
    
    // Initialize global instance
    EmbeddedLLMManager::instance().initialize(config);
    
    std::cout << "EmbeddedLLM initialized: " 
              << THEMIS_LLM().getModelInfo() << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Main Example
// ═══════════════════════════════════════════════════════════

int main() {
    // Initialize
    initialize_embedded_llm("models/tinyllama-1.1b.gguf");
    
    if (!THEMIS_LLM().isReady()) {
        std::cerr << "LLM not ready" << std::endl;
        return 1;
    }
    
    // Example 1: Simple generation (AQL use case)
    std::cout << "\n=== AQL LLM_GENERATE() ===" << std::endl;
    std::string summary = aql_llm_infer("Explain what ThemisDB is in one sentence.");
    std::cout << summary << std::endl;
    
    // Example 2: Chat (Voice assistant use case)
    std::cout << "\n=== Chat (Voice Assistant) ===" << std::endl;
    std::string response = THEMIS_LLM().chatSimple(
        "You are a helpful database assistant.",
        "How do I create an index in ThemisDB?"
    );
    std::cout << response << std::endl;
    
    // Example 3: MCP response
    std::cout << "\n=== MCP Response ===" << std::endl;
    json mcp_result = THEMIS_LLM().generateAsMCP("What is 2+2?", 50);
    std::cout << mcp_result.dump(2) << std::endl;
    
    // Example 4: Embeddings (Vector search use case)
    std::cout << "\n=== Embeddings ===" << std::endl;
    auto embedding = aql_llm_embed("semantic search query");
    std::cout << "Embedding dimension: " << embedding.size() << std::endl;
    
    // Example 5: Streaming (SSE use case)
    std::cout << "\n=== SSE Streaming ===" << std::endl;
    THEMIS_LLM().generateStreamingSSE(
        "Count from 1 to 5.",
        [](const std::string& sse_event) {
            std::cout << sse_event;  // Already formatted as SSE
        },
        "req-123",
        50
    );
    
    // Stats
    std::cout << "\n=== Performance Stats ===" << std::endl;
    std::cout << THEMIS_LLM().getStats().dump(2) << std::endl;
    
    return 0;
}
