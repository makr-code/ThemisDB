/*
 * ThemisDB | File: voice_assistant_llm.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 165
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=54 | delta=51 | status=divergent
 * External Severity (v3): C=2, H=29, M=23
 * PR: #204 Complete llama.cpp implementation with full subsystem integration, ... (2026-03-11T21:22:39Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file voice_assistant_llm.cpp
 * @brief Voice Assistant LLM Integration Implementation (Issue #4)
 * 
 * Uses LlamaWrapper for unified llama.cpp integration.
 * Provides voice command processing with natural language understanding.
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include "voice/voice_assistant.h"
#include "llm/embedded_llm.h"
#include <sstream>

namespace themis {
namespace voice {

// Replace generateLLMResponse to use EmbeddedLLM instead of inference engine
std::string VoiceAssistant::generateLLMResponse(
    const std::string& user_input,
    const VoiceSession& session
) {
    // Build prompt with conversation history
    std::stringstream prompt;
    prompt << "You are a helpful voice assistant integrated into ThemisDB. ";
    prompt << "You help users with database queries, data analysis, and general tasks.\n\n";
    
    // Add conversation history (last 5 exchanges)
    size_t history_start = session.history.size() > 10 ? session.history.size() - 10 : 0;
    for (size_t i = history_start; i < session.history.size(); ++i) {
        prompt << session.history[i] << "\n";
    }
    
    prompt << "User: " << user_input << "\n";
    prompt << "Assistant: ";
    
    // Use EmbeddedLLM instead of inference engine
    try {
        std::string response = THEMIS_LLM_GENERATE(prompt.str());
        
        if (!response.empty()) {
            return response;
        }
    } catch (const std::exception& e) {
        // Log error in production
    }
    
    return "I'm sorry, I encountered an error processing your request.";
}

// Replace generateSummary to use EmbeddedLLM
json VoiceAssistant::generateSummary(const std::string& transcript) {
    if (transcript.empty()) {
        return "No summary available";
    }
    
    // Build prompt for summary generation
    std::stringstream prompt;
    prompt << "Please provide a concise summary of the following transcript:\n\n";
    prompt << transcript.substr(0, std::min(transcript.size(), size_t(4000))) << "\n\n";
    prompt << "Summary: ";
    
    try {
        std::string summary = THEMIS_LLM_GENERATE(prompt.str());
        
        if (!summary.empty()) {
            return summary;
        }
    } catch (const std::exception& e) {
        // Log error
    }
    
    return "Summary generation failed";
}

// Replace extractKeyPoints to use EmbeddedLLM
json VoiceAssistant::extractKeyPoints(const std::string& transcript) {
    if (transcript.empty()) {
        return json::array();
    }
    
    // Build prompt for key points extraction
    std::stringstream prompt;
    prompt << "Extract the key points from the following transcript as a bullet list:\n\n";
    prompt << transcript.substr(0, std::min(transcript.size(), size_t(4000))) << "\n\n";
    prompt << "Key Points:\n";
    
    try {
        std::string response = THEMIS_LLM_GENERATE(prompt.str());
        
        if (!response.empty()) {
            // Parse bullet points from response
            json key_points = json::array();
            std::istringstream iss(response);
            std::string line;
            while (std::getline(iss, line)) {
                // Remove bullet point markers
                if (line.find("- ") == 0 || line.find("* ") == 0) {
                    line = line.substr(2);
                }
                if (!line.empty()) {
                    key_points.push_back(line);
                }
            }
            return key_points;
        }
    } catch (const std::exception& e) {
        // Log error
    }
    
    return json::array();
}

// Replace extractActionItems to use EmbeddedLLM
json VoiceAssistant::extractActionItems(const std::string& transcript) {
    if (transcript.empty()) {
        return json::array();
    }
    
    // Build prompt for action items extraction
    std::stringstream prompt;
    prompt << "Extract action items and tasks from the following transcript:\n\n";
    prompt << transcript.substr(0, std::min(transcript.size(), size_t(4000))) << "\n\n";
    prompt << "Action Items:\n";
    
    try {
        std::string response = THEMIS_LLM_GENERATE(prompt.str());
        
        if (!response.empty()) {
            // Parse action items from response
            json action_items = json::array();
            std::istringstream iss(response);
            std::string line;
            while (std::getline(iss, line)) {
                // Remove bullet point markers
                if (line.find("- ") == 0 || line.find("* ") == 0) {
                    line = line.substr(2);
                }
                if (!line.empty()) {
                    json item;
                    item["description"] = line;
                    item["status"] = "pending";
                    action_items.push_back(item);
                }
            }
            return action_items;
        }
    } catch (const std::exception& e) {
        // Log error
    }
    
    return json::array();
}

} // namespace voice
} // namespace themis
