/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chat_formatting_example.cpp                        ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     77                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
