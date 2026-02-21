/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chat_formatting_example.cpp                        ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
