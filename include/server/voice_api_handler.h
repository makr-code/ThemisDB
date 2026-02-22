/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_api_handler.h                                ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file voice_api_handler.h
 * @brief Voice Assistant API Handler
 * 
 * Implements RESTful endpoints for voice assistant operations:
 * - Speech-to-text transcription
 * - Text-to-speech synthesis
 * - Voice command processing
 * - Phone call recording and transcription
 * - Meeting protocol generation
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <nlohmann/json.hpp>

// Forward declarations
namespace themis {
namespace voice {
class VoiceAssistant;
}
namespace utils {
class HTTPClientPool;
}
}

namespace themis::server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

/**
 * @brief Voice API Handler for ThemisDB HTTP Server
 * 
 * Implements RESTful endpoints for voice operations:
 * - POST /api/v1/voice/transcribe - Transcribe audio to text
 * - POST /api/v1/voice/synthesize - Synthesize text to speech
 * - POST /api/v1/voice/command - Process voice command
 * - POST /api/v1/voice/call/record - Record and transcribe phone call
 * - POST /api/v1/voice/meeting/protocol - Generate meeting protocol
 * - GET  /api/v1/voice/sessions/{id} - Get session information
 * - POST /api/v1/voice/sessions/{id}/context - Update session context
 * - DELETE /api/v1/voice/sessions/{id} - Delete session
 * - GET  /api/v1/voice/stats - Get voice assistant statistics
 * - GET  /api/v1/voice/health - Health check
 * - GET  /api/v1/voice/voices - List available TTS voices
 * - GET  /api/v1/voice/languages - List supported languages
 * - WS  /ws/voice/stream - WebSocket for real-time voice interaction
 * 
 * All endpoints require Bearer Token (JWT) authentication via Authorization header.
 * Audio data should be sent as multipart/form-data or base64-encoded in JSON.
 */
class VoiceApiHandler {
public:
    /**
     * @brief Construct Voice API handler
     * 
     * @param voice_assistant Voice assistant instance
     */
    explicit VoiceApiHandler(std::shared_ptr<voice::VoiceAssistant> voice_assistant);
    
    /**
     * @brief Handle Voice API request
     * 
     * Routes request to appropriate handler based on path and method.
     * Validates JWT Bearer Token authentication.
     * 
     * @param req HTTP request
     * @return HTTP response (JSON or audio data)
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

private:
    // Core endpoints
    http::response<http::string_body> handleTranscribe(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleSynthesize(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleVoiceCommand(
        const http::request<http::string_body>& req);
    
    // Phone call endpoints
    http::response<http::string_body> handleRecordCall(
        const http::request<http::string_body>& req);
    
    // Meeting endpoints
    http::response<http::string_body> handleGenerateProtocol(
        const http::request<http::string_body>& req);
    
    // Session management endpoints
    http::response<http::string_body> handleGetSession(
        const http::request<http::string_body>& req,
        const std::string& session_id);
    
    http::response<http::string_body> handleUpdateSessionContext(
        const http::request<http::string_body>& req,
        const std::string& session_id);
    
    http::response<http::string_body> handleDeleteSession(
        const http::request<http::string_body>& req,
        const std::string& session_id);
    
    // Information endpoints
    http::response<http::string_body> handleGetVoices(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleGetLanguages(
        const http::request<http::string_body>& req);
    
    // Statistics and health
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleHealth(
        const http::request<http::string_body>& req);
    
    // Helper methods
    bool validateBearerToken(const http::request<http::string_body>& req);
    
    http::response<http::string_body> createErrorResponse(
        http::status status,
        std::string_view error,
        std::string_view details = "");
    
    http::response<http::string_body> createJsonResponse(
        const json& data,
        http::status status = http::status::ok);
    
    http::response<http::string_body> createAudioResponse(
        const std::vector<uint8_t>& audio_data,
        const std::string& mime_type);
    
    std::optional<json> parseRequestBody(
        const http::request<http::string_body>& req);
    
    std::vector<uint8_t> extractAudioData(
        const http::request<http::string_body>& req);
    
    std::vector<uint8_t> decodeBase64(const std::string& encoded);
    
    std::string encodeBase64(const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> downloadAudioFromUrl(const std::string& url);
    
    std::shared_ptr<voice::VoiceAssistant> voice_assistant_;
    std::shared_ptr<utils::HTTPClientPool> http_client_pool_;
};

} // namespace themis::server
