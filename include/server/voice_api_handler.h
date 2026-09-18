/**
 * @file voice_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "server/auth_middleware.h"
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <nlohmann/json.hpp>
#include "auth/jwt_validator.h"

// Forward declarations
namespace themis {
namespace voice {
class VoiceAssistant;
}
namespace utils {
class HTTPClientPool;
}
class AuthMiddleware;
}

namespace themis::server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

class VoiceApiHandler {
public:
    explicit VoiceApiHandler(
        std::shared_ptr<voice::VoiceAssistant> voice_assistant,
        std::shared_ptr<themis::AuthMiddleware> auth = nullptr);
    
    /**
     * @brief Handle Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

    using TokenValidatorFn = std::function<bool(std::string_view)>;

    /**
     * @brief Set Token Validator Fn.
     * @param[in] fn Input parameter.
     */
    static void setTokenValidatorFn(TokenValidatorFn fn);

private:
    // Core endpoints
    /**
     * @brief Handle Transcribe.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTranscribe(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Synthesize.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSynthesize(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Voice Command.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleVoiceCommand(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Stream Command.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStreamCommand(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Wake Word Detect.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleWakeWordDetect(
        const http::request<http::string_body>& req);
    
    // Phone call endpoints
    /**
     * @brief Handle Record Call.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRecordCall(
        const http::request<http::string_body>& req);
    
    // Meeting endpoints
    /**
     * @brief Handle Generate Protocol.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGenerateProtocol(
        const http::request<http::string_body>& req);
    
    // Session management endpoints
    /**
     * @brief Handle Get Session.
     * @param[in] req Input parameter.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    http::response<http::string_body> handleGetSession(
        const http::request<http::string_body>& req,
        const std::string& session_id);
    
    /**
     * @brief Handle Update Session Context.
     * @param[in] req Input parameter.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateSessionContext(
        const http::request<http::string_body>& req,
        const std::string& session_id);
    
    /**
     * @brief Handle Delete Session.
     * @param[in] req Input parameter.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteSession(
        const http::request<http::string_body>& req,
        const std::string& session_id);
    
    // Information endpoints
    /**
     * @brief Handle Get Voices.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetVoices(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Get Languages.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetLanguages(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Create Macro.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateMacro(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle List Macros.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListMacros(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Get Macro.
     * @param[in] req Input parameter.
     * @param[in] macro_id Identifier of the macro.
     * @return Return value.
     */
    http::response<http::string_body> handleGetMacro(
        const http::request<http::string_body>& req,
        const std::string& macro_id);
    
    /**
     * @brief Handle Update Macro.
     * @param[in] req Input parameter.
     * @param[in] macro_id Identifier of the macro.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateMacro(
        const http::request<http::string_body>& req,
        const std::string& macro_id);
    
    /**
     * @brief Handle Delete Macro.
     * @param[in] req Input parameter.
     * @param[in] macro_id Identifier of the macro.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteMacro(
        const http::request<http::string_body>& req,
        const std::string& macro_id);

    /**
     * @brief Handle List Recordings.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListRecordings(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Recording.
     * @param[in] req Input parameter.
     * @param[in] record_id Identifier of the record.
     * @return Return value.
     */
    http::response<http::string_body> handleGetRecording(
        const http::request<http::string_body>& req,
        const std::string& record_id);

    /**
     * @brief Handle Search Transcripts.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSearchTranscripts(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Auth Enroll.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuthEnroll(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Auth Verify.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuthVerify(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Auth Authenticate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuthAuthenticate(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Auth Identify.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuthIdentify(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Auth List Profiles.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuthListProfiles(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Auth Delete Profile.
     * @param[in] req Input parameter.
     * @param[in] profile_id Identifier of the profile.
     * @return Return value.
     */
    http::response<http::string_body> handleAuthDeleteProfile(
        const http::request<http::string_body>& req,
        const std::string& profile_id);

    // Statistics and health
    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Health.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealth(
        const http::request<http::string_body>& req);
    
    // Helper methods
    /**
     * @brief Validate Bearer Token.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateBearerToken(const http::request<http::string_body>& req);
    
    http::response<http::string_body> createErrorResponse(
        http::status status,
        std::string_view error,
        std::string_view details = "");
    
    http::response<http::string_body> createJsonResponse(
        const json& data,
        http::status status = http::status::ok);
    
    /**
     * @brief Create Audio Response.
     * @param[in] audio_data Input parameter.
     * @param[in] mime_type Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> createAudioResponse(
        const std::vector<uint8_t>& audio_data,
        const std::string& mime_type);
    
    /**
     * @brief Parse Request Body.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    std::optional<json> parseRequestBody(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Extract Audio Data.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> extractAudioData(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Decode Base64.
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decodeBase64(const std::string& encoded);
    
    /**
     * @brief Encode Base64.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string encodeBase64(const std::vector<uint8_t>& data);
    
    /**
     * @brief Download Audio From Url.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> downloadAudioFromUrl(const std::string& url);

    /**
     * @brief Parse Query Param.
     * @param[in] target Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    static std::string parseQueryParam(const std::string& target, const std::string& key);

    std::shared_ptr<voice::VoiceAssistant> voice_assistant_;
    std::shared_ptr<utils::HTTPClientPool> http_client_pool_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
};

} // namespace themis::server
