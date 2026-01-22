/**
 * @file voice_api_handler.cpp
 * @brief Voice Assistant API Handler Implementation
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#include "server/voice_api_handler.h"
#include "voice/voice_assistant.h"
#include "utils/http_client_pool.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace themis::server {

VoiceApiHandler::VoiceApiHandler(std::shared_ptr<voice::VoiceAssistant> voice_assistant)
    : voice_assistant_(voice_assistant) {
    // Initialize HTTP client pool for downloading audio from URLs
    utils::HTTPClientPool::Config http_config;
    http_config.max_connections = 10;
    http_config.connect_timeout = std::chrono::seconds(10);
    http_config.request_timeout = std::chrono::seconds(60); // Audio files may be large
    http_client_pool_ = std::make_shared<utils::HTTPClientPool>(http_config);
}

http::response<http::string_body> VoiceApiHandler::handleRequest(
    const http::request<http::string_body>& req
) {
    // Validate authentication
    if (!validateBearerToken(req)) {
        return createErrorResponse(
            http::status::unauthorized,
            "Unauthorized",
            "Invalid or missing Bearer token"
        );
    }
    
    // Extract path and method
    std::string path = std::string(req.target());
    auto method = req.method();
    
    // Route to appropriate handler
    if (path == "/api/v1/voice/transcribe" && method == http::verb::post) {
        return handleTranscribe(req);
    }
    else if (path == "/api/v1/voice/synthesize" && method == http::verb::post) {
        return handleSynthesize(req);
    }
    else if (path == "/api/v1/voice/command" && method == http::verb::post) {
        return handleVoiceCommand(req);
    }
    else if (path == "/api/v1/voice/call/record" && method == http::verb::post) {
        return handleRecordCall(req);
    }
    else if (path == "/api/v1/voice/meeting/protocol" && method == http::verb::post) {
        return handleGenerateProtocol(req);
    }
    else if (path == "/api/v1/voice/voices" && method == http::verb::get) {
        return handleGetVoices(req);
    }
    else if (path == "/api/v1/voice/languages" && method == http::verb::get) {
        return handleGetLanguages(req);
    }
    else if (path == "/api/v1/voice/stats" && method == http::verb::get) {
        return handleStats(req);
    }
    else if (path == "/api/v1/voice/health" && method == http::verb::get) {
        return handleHealth(req);
    }
    else if (path.find("/api/v1/voice/sessions/") == 0) {
        // Extract session ID
        std::string session_id = path.substr(24);  // Length of "/api/v1/voice/sessions/"
        
        // Remove trailing segments
        auto slash_pos = session_id.find('/');
        if (slash_pos != std::string::npos) {
            std::string action = session_id.substr(slash_pos + 1);
            session_id = session_id.substr(0, slash_pos);
            
            if (action == "context" && method == http::verb::post) {
                return handleUpdateSessionContext(req, session_id);
            }
        }
        
        if (method == http::verb::get) {
            return handleGetSession(req, session_id);
        }
        else if (method == http::verb::delete_) {
            return handleDeleteSession(req, session_id);
        }
    }
    
    return createErrorResponse(
        http::status::not_found,
        "Not Found",
        "Voice API endpoint not found"
    );
}

http::response<http::string_body> VoiceApiHandler::handleTranscribe(
    const http::request<http::string_body>& req
) {
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Invalid JSON body"
        );
    }
    
    // Extract audio data
    std::vector<uint8_t> audio_data;
    
    if (body->contains("audio_base64")) {
        audio_data = decodeBase64((*body)["audio_base64"]);
    } else if (body->contains("audio_url")) {
        // Download audio from URL
        try {
            std::string audio_url = (*body)["audio_url"];
            audio_data = downloadAudioFromUrl(audio_url);
        } catch (const std::exception& e) {
            return createErrorResponse(
                http::status::bad_request,
                "Bad Request",
                std::string("Failed to download audio from URL: ") + e.what()
            );
        }
    } else {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Missing audio_base64 or audio_url field"
        );
    }
    
    // Transcribe
    json options;
    if (body->contains("language")) {
        options["language"] = (*body)["language"];
    }
    if (body->contains("timestamps")) {
        options["timestamps"] = (*body)["timestamps"];
    }
    if (body->contains("speaker_diarization")) {
        options["speaker_diarization"] = (*body)["speaker_diarization"];
    }
    
    // Use STT processor directly
    // (In real implementation, would access through voice_assistant)
    json result;
    result["success"] = true;
    result["text"] = "[Transcription result would appear here]";
    result["language"] = "en";
    result["confidence"] = 0.95;
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleSynthesize(
    const http::request<http::string_body>& req
) {
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Invalid JSON body"
        );
    }
    
    if (!body->contains("text")) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Missing text field"
        );
    }
    
    std::string text = (*body)["text"];
    
    // Extract options
    std::string voice = body->value("voice", "default");
    float speed = body->value("speed", 1.0f);
    float pitch = body->value("pitch", 1.0f);
    std::string format = body->value("format", "wav");
    bool return_base64 = body->value("return_base64", false);
    
    // Synthesize audio (placeholder)
    std::vector<uint8_t> audio_data;  // Would contain actual audio
    
    if (return_base64) {
        json result;
        result["success"] = true;
        result["audio_base64"] = encodeBase64(audio_data);
        result["mime_type"] = "audio/wav";
        result["duration_ms"] = 3000;
        return createJsonResponse(result);
    } else {
        return createAudioResponse(audio_data, "audio/wav");
    }
}

http::response<http::string_body> VoiceApiHandler::handleVoiceCommand(
    const http::request<http::string_body>& req
) {
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Invalid JSON body"
        );
    }
    
    std::string session_id = body->value("session_id", "default");
    
    // Process text command or audio command
    if (body->contains("text")) {
        std::string text = (*body)["text"];
        std::string response = voice_assistant_->processTextCommand(text, session_id);
        
        json result;
        result["success"] = true;
        result["response"] = response;
        result["session_id"] = session_id;
        
        return createJsonResponse(result);
    }
    else if (body->contains("audio_base64")) {
        auto audio_data = decodeBase64((*body)["audio_base64"]);
        auto audio_response = voice_assistant_->processVoiceCommand(audio_data, session_id);
        
        json result;
        result["success"] = true;
        result["audio_base64"] = encodeBase64(audio_response);
        result["mime_type"] = "audio/wav";
        result["session_id"] = session_id;
        
        return createJsonResponse(result);
    }
    
    return createErrorResponse(
        http::status::bad_request,
        "Bad Request",
        "Missing text or audio_base64 field"
    );
}

http::response<http::string_body> VoiceApiHandler::handleRecordCall(
    const http::request<http::string_body>& req
) {
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Invalid JSON body"
        );
    }
    
    // Extract audio data
    std::vector<uint8_t> audio_data;
    if (body->contains("audio_base64")) {
        audio_data = decodeBase64((*body)["audio_base64"]);
    } else {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Missing audio_base64 field"
        );
    }
    
    // Extract metadata
    voice::PhoneCallMetadata metadata;
    metadata.call_id = body->value("call_id", "");
    metadata.caller_number = body->value("caller", "");
    metadata.callee_number = body->value("callee", "");
    metadata.start_time = body->value("start_time", 0LL);
    metadata.end_time = body->value("end_time", 0LL);
    metadata.call_type = body->value("call_type", "inbound");
    
    if (body->contains("custom_fields")) {
        metadata.custom_fields = (*body)["custom_fields"];
    }
    
    // Record call
    auto result = voice_assistant_->recordPhoneCall(audio_data, metadata);
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleGenerateProtocol(
    const http::request<http::string_body>& req
) {
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Invalid JSON body"
        );
    }
    
    // Extract audio data
    std::vector<uint8_t> audio_data;
    if (body->contains("audio_base64")) {
        audio_data = decodeBase64((*body)["audio_base64"]);
    } else {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Missing audio_base64 field"
        );
    }
    
    // Extract metadata
    voice::MeetingMetadata metadata;
    metadata.meeting_id = body->value("meeting_id", "");
    metadata.title = body->value("title", "");
    metadata.start_time = body->value("start_time", 0LL);
    metadata.end_time = body->value("end_time", 0LL);
    metadata.organizer = body->value("organizer", "");
    
    if (body->contains("participants") && (*body)["participants"].is_array()) {
        for (const auto& p : (*body)["participants"]) {
            metadata.participants.push_back(p);
        }
    }
    
    if (body->contains("custom_fields")) {
        metadata.custom_fields = (*body)["custom_fields"];
    }
    
    // Generate protocol
    auto result = voice_assistant_->generateMeetingProtocol(audio_data, metadata);
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleGetSession(
    const http::request<http::string_body>& req,
    const std::string& session_id
) {
    auto session = voice_assistant_->getSession(session_id);
    
    json result;
    result["session_id"] = session.session_id;
    result["user_id"] = session.user_id;
    result["created_at"] = session.created_at;
    result["last_activity"] = session.last_activity;
    result["context"] = session.context;
    result["history"] = session.history;
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleUpdateSessionContext(
    const http::request<http::string_body>& req,
    const std::string& session_id
) {
    auto body = parseRequestBody(req);
    if (!body || !body->contains("context")) {
        return createErrorResponse(
            http::status::bad_request,
            "Bad Request",
            "Missing context field"
        );
    }
    
    voice_assistant_->updateSession(session_id, (*body)["context"]);
    
    json result;
    result["success"] = true;
    result["session_id"] = session_id;
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleDeleteSession(
    const http::request<http::string_body>& req,
    const std::string& session_id
) {
    // Delete session (not yet implemented in voice_assistant)
    
    json result;
    result["success"] = true;
    result["session_id"] = session_id;
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleGetVoices(
    const http::request<http::string_body>& req
) {
    // Get available voices (placeholder)
    json result;
    result["voices"] = json::array({
        {{"id", "default"}, {"name", "Default Voice"}, {"language", "en"}},
        {{"id", "female_en"}, {"name", "Female English"}, {"language", "en"}},
        {{"id", "male_en"}, {"name", "Male English"}, {"language", "en"}},
        {{"id", "female_de"}, {"name", "Female German"}, {"language", "de"}}
    });
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleGetLanguages(
    const http::request<http::string_body>& req
) {
    json result;
    result["languages"] = json::array({
        "en", "de", "es", "fr", "it", "pt", "ru", "zh", "ja", "ko"
    });
    
    return createJsonResponse(result);
}

http::response<http::string_body> VoiceApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    auto stats = voice_assistant_->getStatistics();
    return createJsonResponse(stats);
}

http::response<http::string_body> VoiceApiHandler::handleHealth(
    const http::request<http::string_body>& req
) {
    json result;
    result["status"] = "healthy";
    result["voice_assistant"] = "available";
    result["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    
    return createJsonResponse(result);
}

// Helper methods

bool VoiceApiHandler::validateBearerToken(
    const http::request<http::string_body>& req
) {
    // Check Authorization header
    auto it = req.find(http::field::authorization);
    if (it == req.end()) {
        return false;
    }
    
    std::string auth = it->value();
    if (auth.size() < 7 || auth.substr(0, 7) != "Bearer ") {
        return false;
    }
    
    // Extract token
    std::string token = auth.substr(7);
    
    // Validate token (placeholder - real implementation would verify JWT)
    return !token.empty();
}

http::response<http::string_body> VoiceApiHandler::createErrorResponse(
    http::status status,
    std::string_view error,
    std::string_view details
) {
    json body;
    body["error"] = error;
    if (!details.empty()) {
        body["details"] = details;
    }
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> VoiceApiHandler::createJsonResponse(
    const json& data,
    http::status status
) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.body() = data.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> VoiceApiHandler::createAudioResponse(
    const std::vector<uint8_t>& audio_data,
    const std::string& mime_type
) {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, mime_type);
    res.body() = std::string(audio_data.begin(), audio_data.end());
    res.prepare_payload();
    return res;
}

std::optional<json> VoiceApiHandler::parseRequestBody(
    const http::request<http::string_body>& req
) {
    try {
        return json::parse(req.body());
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<uint8_t> VoiceApiHandler::decodeBase64(const std::string& encoded) {
    // TODO: Implement base64 decoding
    // For now, return empty vector to avoid crashes
    // Real implementation should use a base64 library (e.g., Boost.Beast base64)
    return std::vector<uint8_t>();
}

std::string VoiceApiHandler::encodeBase64(const std::vector<uint8_t>& data) {
    // TODO: Implement base64 encoding
    // For now, return empty string to avoid crashes
    // Real implementation should use a base64 library (e.g., Boost.Beast base64)
    return "";
}

std::vector<uint8_t> VoiceApiHandler::downloadAudioFromUrl(const std::string& url) {
    // Validate URL format
    if (url.empty()) {
        throw std::invalid_argument("URL cannot be empty");
    }
    
    // Check if URL starts with http:// or https://
    if (url.find("http://") != 0 && url.find("https://") != 0) {
        throw std::invalid_argument("URL must start with http:// or https://");
    }
    
    // SSRF Protection: Parse URL and validate host to prevent access to internal resources
    try {
        auto components = utils::parseURL(url);
        std::string host_lower = components.host;
        std::transform(host_lower.begin(), host_lower.end(), host_lower.begin(), ::tolower);
        
        // Block localhost and loopback addresses
        if (host_lower == "localhost" || 
            host_lower == "127.0.0.1" ||
            host_lower.find("127.") == 0 ||
            host_lower == "::1" ||
            host_lower.find("[::1]") == 0) {
            throw std::invalid_argument("Access to localhost is not allowed");
        }
        
        // Block private IP ranges (RFC 1918)
        if (host_lower.find("10.") == 0 ||
            host_lower.find("192.168.") == 0 ||
            (host_lower.find("172.") == 0 && 
             std::stoi(host_lower.substr(4, host_lower.find('.', 4) - 4)) >= 16 &&
             std::stoi(host_lower.substr(4, host_lower.find('.', 4) - 4)) <= 31)) {
            throw std::invalid_argument("Access to private IP addresses is not allowed");
        }
        
        // Block link-local addresses (169.254.0.0/16)
        if (host_lower.find("169.254.") == 0) {
            throw std::invalid_argument("Access to link-local addresses is not allowed");
        }
        
        // Block metadata endpoints commonly used in cloud environments
        if (host_lower == "169.254.169.254" || host_lower == "metadata.google.internal") {
            throw std::invalid_argument("Access to metadata endpoints is not allowed");
        }
        
    } catch (const std::invalid_argument& e) {
        throw; // Re-throw validation errors
    } catch (const std::exception& e) {
        throw std::invalid_argument("Invalid URL format");
    }
    
    // Download audio using HTTP client pool
    // Note: The HTTP client pool has built-in timeouts (connect_timeout and request_timeout)
    // which prevent indefinite blocking
    auto response_future = http_client_pool_->get(url);
    auto response = response_future.get();
    
    // Check if download was successful
    if (!response.isSuccess()) {
        throw std::runtime_error(
            "Failed to download audio: HTTP " + std::to_string(response.status_code)
        );
    }
    
    // Check if response body is not empty
    if (response.body.empty()) {
        throw std::runtime_error("Downloaded audio is empty");
    }
    
    // Convert response body to vector<uint8_t>
    std::vector<uint8_t> audio_data(response.body.begin(), response.body.end());
    
    return audio_data;
}

} // namespace themis::server
