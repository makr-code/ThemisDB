/*
 * ThemisDB | File: voice_assistant_llm.cpp | Version: 0.0.47 | Last Modified: 2026-05-22 06:56:08
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 170
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=6, L=0
 * PR History (last 5): #204 Complete llama.cpp implemen... (2026-03-11)
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
#include "rag/prompt_injection_detector.h"
#include <sstream>

namespace themis {
namespace voice {

namespace {

std::string sanitizePromptFragment(const std::string& text, bool* changed = nullptr) {
    thread_local rag::security::PromptInjectionSanitizer sanitizer;
    const std::string safe = sanitizer.sanitize(text);
    if (changed) {
        *changed = (safe != text);
    }
    return safe;
}

} // namespace

// Replace generateLLMResponse to use EmbeddedLLM instead of inference engine
std::string VoiceAssistant::generateLLMResponse(
    const std::string& user_input,
    const VoiceSession& session
) {
    bool user_input_sanitized = false;
    const std::string safe_user_input = sanitizePromptFragment(user_input, &user_input_sanitized);

    // Build prompt with conversation history
    std::stringstream prompt;
    prompt << "You are a helpful voice assistant integrated into ThemisDB. ";
    prompt << "You help users with database queries, data analysis, and general tasks.\n\n";
    
    // Add conversation history (last 5 exchanges)
    size_t history_start = session.history.size() > 10 ? session.history.size() - 10 : 0;
    size_t sanitized_history_entries = 0;
    for (size_t i = history_start; i < session.history.size(); ++i) {
        bool history_line_sanitized = false;
        const std::string safe_history_line = sanitizePromptFragment(session.history[i], &history_line_sanitized);
        if (history_line_sanitized) {
            ++sanitized_history_entries;
        }
        prompt << safe_history_line << "\n";
    }
    
    prompt << "User: " << safe_user_input << "\n";
    prompt << "Assistant: ";

    if (user_input_sanitized || sanitized_history_entries > 0) {
        VoiceAuditEntry entry;
        entry.event_type = "voice_prompt_sanitization";
        entry.session_id = session.session_id;
        entry.user_id = session.user_id;
        entry.action = "generate_llm_response";
        entry.resource = "voice_assistant_llm";
        entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        entry.success = true;
        entry.details = "Prompt input sanitized before LLM dispatch";
        entry.metadata = {
            {"user_input_sanitized", user_input_sanitized},
            {"sanitized_history_entries", sanitized_history_entries}
        };
        voice_security_manager_.logEvent(entry);
    }
    
    // Use EmbeddedLLM instead of inference engine
    try {
        std::string response = THEMIS_LLM_GENERATE(prompt.str());
        
        if (!response.empty()) {
            return response;
        }
    } catch (const std::exception& e) {
        static_cast<void>(e);
        // Log error in production
    }
    
    return "I'm sorry, I encountered an error processing your request.";
}

// Replace generateSummary to use EmbeddedLLM
json VoiceAssistant::generateSummary(const std::string& transcript) {
    if (transcript.empty()) {
        return "No summary available";
    }

    const std::string safe_transcript = sanitizePromptFragment(transcript);
    
    // Build prompt for summary generation
    std::stringstream prompt;
    prompt << "Please provide a concise summary of the following transcript:\n\n";
    prompt << safe_transcript.substr(0, std::min(safe_transcript.size(), size_t(4000))) << "\n\n";
    prompt << "Summary: ";
    
    try {
        std::string summary = THEMIS_LLM_GENERATE(prompt.str());
        
        if (!summary.empty()) {
            return summary;
        }
    } catch (const std::exception& e) {
        static_cast<void>(e);
        // Log error
    }
    
    return "Summary generation failed";
}

// Replace extractKeyPoints to use EmbeddedLLM
json VoiceAssistant::extractKeyPoints(const std::string& transcript) {
    if (transcript.empty()) {
        return json::array();
    }

    const std::string safe_transcript = sanitizePromptFragment(transcript);
    
    // Build prompt for key points extraction
    std::stringstream prompt;
    prompt << "Extract the key points from the following transcript as a bullet list:\n\n";
    prompt << safe_transcript.substr(0, std::min(safe_transcript.size(), size_t(4000))) << "\n\n";
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
        static_cast<void>(e);
        // Log error
    }
    
    return json::array();
}

// Replace extractActionItems to use EmbeddedLLM
json VoiceAssistant::extractActionItems(const std::string& transcript) {
    if (transcript.empty()) {
        return json::array();
    }

    const std::string safe_transcript = sanitizePromptFragment(transcript);
    
    // Build prompt for action items extraction
    std::stringstream prompt;
    prompt << "Extract action items and tasks from the following transcript:\n\n";
    prompt << safe_transcript.substr(0, std::min(safe_transcript.size(), size_t(4000))) << "\n\n";
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
        static_cast<void>(e);
        // Log error
    }
    
    return json::array();
}

} // namespace voice
} // namespace themis
