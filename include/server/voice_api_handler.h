/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_api_handler.h                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     267                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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

// Forward-declare AuthMiddleware so callers can pass it without pulling in the
// full header in most translation units.
namespace themis {
class AuthMiddleware;
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
 * - POST /api/v1/voice/command/stream - Process voice command with streaming STT (segments + TTS)
 * - POST /api/v1/voice/wake-word/detect - Scan audio chunk for registered wake words
 * - POST /api/v1/voice/call/record - Record and transcribe phone call
 * - POST /api/v1/voice/meeting/protocol - Generate meeting protocol
 * - GET  /api/v1/voice/sessions/{id} - Get session information
 * - POST /api/v1/voice/sessions/{id}/context - Update session context
 * - DELETE /api/v1/voice/sessions/{id} - Delete session
 * - GET  /api/v1/voice/stats - Get voice assistant statistics
 * - GET  /api/v1/voice/health - Health check
 * - GET  /api/v1/voice/voices - List available TTS voices
 * - GET  /api/v1/voice/languages - List supported languages
 * - POST /api/v1/voice/macros - Create voice command macro
 * - GET  /api/v1/voice/macros - List voice command macros
 * - GET  /api/v1/voice/macros/{id} - Get a specific macro
 * - PUT  /api/v1/voice/macros/{id} - Update a macro
 * - DELETE /api/v1/voice/macros/{id} - Delete a macro
 * - GET  /api/v1/voice/recordings - List stored recordings (playback index)
 * - GET  /api/v1/voice/recordings/search?q=<query> - Full-text search in stored transcripts
 * - GET  /api/v1/voice/recordings/{id} - Get a specific recording for playback
 * - POST /api/v1/voice/auth/enroll - Enroll a speaker's voice profile
 * - POST /api/v1/voice/auth/verify - 1:1 speaker verification against a profile
 * - POST /api/v1/voice/auth/authenticate - Full biometric authentication (liveness + verification)
 * - POST /api/v1/voice/auth/identify - 1:N speaker identification among candidate profiles
 * - GET  /api/v1/voice/auth/profiles - List enrolled voice profiles
 * - DELETE /api/v1/voice/auth/profiles/{id} - Delete a voice profile
 * - WS  /ws/voice/stream - WebSocket for real-time voice interaction
 * 
 * All endpoints require Bearer Token (JWT) authentication via Authorization header.
 * Audio data should be sent as multipart/form-data or base64-encoded in JSON.
 */
class VoiceApiHandler {
public:
    /**
     * @brief Construct Voice API handler.
     *
     * @param voice_assistant  Voice assistant instance (required).
     * @param auth             Optional authentication middleware.  When non-null
     *                         and enabled, every request is validated via the
     *                         repository-wide JWT/OIDC stack.  When null the
     *                         handler operates in open mode (non-empty bearer
     *                         token check only) for backward compatibility.
     */
    explicit VoiceApiHandler(
        std::shared_ptr<voice::VoiceAssistant> voice_assistant,
        std::shared_ptr<::themis::AuthMiddleware> auth = nullptr);
    
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
    
    http::response<http::string_body> handleStreamCommand(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleWakeWordDetect(
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
    
    // Voice macro CRUD endpoints
    http::response<http::string_body> handleCreateMacro(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleListMacros(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleGetMacro(
        const http::request<http::string_body>& req,
        const std::string& macro_id);
    
    http::response<http::string_body> handleUpdateMacro(
        const http::request<http::string_body>& req,
        const std::string& macro_id);
    
    http::response<http::string_body> handleDeleteMacro(
        const http::request<http::string_body>& req,
        const std::string& macro_id);

    // Recording playback and transcript search endpoints
    http::response<http::string_body> handleListRecordings(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleGetRecording(
        const http::request<http::string_body>& req,
        const std::string& record_id);

    http::response<http::string_body> handleSearchTranscripts(
        const http::request<http::string_body>& req);

    // Voice biometric authentication endpoints
    http::response<http::string_body> handleAuthEnroll(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleAuthVerify(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleAuthAuthenticate(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleAuthIdentify(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleAuthListProfiles(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleAuthDeleteProfile(
        const http::request<http::string_body>& req,
        const std::string& profile_id);

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

    /**
     * @brief Parse the value of a single query parameter from a request target.
     *
     * @param target    Full request target (path + optional "?key=value&...").
     * @param key       Parameter name to look up.
     * @return Parameter value, or empty string if not found.
     *
     * @note Percent-encoded characters are not decoded; tag values should
     *       use plain ASCII identifiers to avoid encoding issues.
     */
    static std::string parseQueryParam(const std::string& target, const std::string& key);

    std::shared_ptr<voice::VoiceAssistant> voice_assistant_;
    std::shared_ptr<utils::HTTPClientPool> http_client_pool_;
    std::shared_ptr<::themis::AuthMiddleware> auth_; ///< JWT/OIDC auth middleware; null = open mode
};

} // namespace themis::server
