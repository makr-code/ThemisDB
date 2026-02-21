/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chat_formatting_example.cpp                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:35:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     79                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f3787300e  2026-01-04  Add comprehensive LLM integration: streaming, chat, MCP, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Chat Formatting Examples for LlamaWrapper
 * 
 * This demonstrates how to use the new chat formatting functionality
 * added to support multi-turn conversations.
 */

#include <iostream>
#include <vector>
#include "llm/llama_wrapper.h"

using namespace themis::llm;

int main() {
    // Example 1: ChatML format (default)
    std::vector<ChatMessage> messages = {
        {"system", "You are a helpful AI assistant."},
        {"user", "What is the capital of France?"},
        {"assistant", "The capital of France is Paris."},
        {"user", "What is its population?"}
    };
    
    LlamaWrapper wrapper(LlamaWrapper::Config{});
    
    std::cout << "=== ChatML Format ===" << std::endl;
    std::string chatML = wrapper.formatChatMessages(messages, ChatFormat::ChatML);
    std::cout << chatML << std::endl << std::endl;
    
    std::cout << "=== Llama-2 Format ===" << std::endl;
    std::string llama2 = wrapper.formatChatMessages(messages, ChatFormat::Llama2);
    std::cout << llama2 << std::endl << std::endl;
    
    std::cout << "=== Vicuna Format ===" << std::endl;
    std::string vicuna = wrapper.formatChatMessages(messages, ChatFormat::Vicuna);
    std::cout << vicuna << std::endl << std::endl;
    
    std::cout << "=== Alpaca Format ===" << std::endl;
    std::string alpaca = wrapper.formatChatMessages(messages, ChatFormat::Alpaca);
    std::cout << alpaca << std::endl << std::endl;
    
    // Example 2: Using with generate()
    // This shows how to integrate chat formatting with inference
    /*
    InferenceRequest request;
    request.prompt = wrapper.formatChatMessages(messages, ChatFormat::ChatML);
    request.max_tokens = 100;
    request.temperature = 0.7f;
    
    // This would call the real llama.cpp inference
    // auto response = wrapper.generate(request);
    // std::cout << "Response: " << response.text << std::endl;
    */
    
    return 0;
}
