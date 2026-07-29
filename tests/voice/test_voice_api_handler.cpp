/*
 * ThemisDB | File: test_voice_api_handler.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>

#include "server/voice_api_handler.h"
#include "voice/voice_assistant.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace http = boost::beast::http;
using json = nlohmann::json;

namespace themis { namespace server { 
namespace {

http::request<http::string_body> makeRequest(http::verb method, const std::string& target) {
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.set(http::field::content_type, "application/json");
    req.prepare_payload();
    return req;
}

http::request<http::string_body> makeJsonRequest(
    http::verb method,
    const std::string& target,
    const json& body) {
    auto req = makeRequest(method, target);
    req.body() = body.dump();
    req.prepare_payload();
    return req;
}

json parseBody(const http::response<http::string_body>& response) {
    return json::parse(response.body());
}

class VoiceApiHandlerPathValidationTest : public ::testing::Test {
protected:
    VoiceApiHandlerPathValidationTest()
        : handler{createVoiceAssistant(), createAuth()} {}

    static std::shared_ptr<voice::VoiceAssistant> createVoiceAssistant() {
        return std::make_shared<voice::VoiceAssistant>(voice::VoiceAssistant::Config{});
    }

    static std::shared_ptr<themis::AuthMiddleware> createAuth() {
        auto auth = std::make_shared<themis::AuthMiddleware>();
        themis::AuthMiddleware::TokenConfig cfg;
        cfg.token = "test-token";
        cfg.user_id = "voice-test-user";
        cfg.scopes = {"admin", "data:read", "data:write", "metrics:read"};
        auth->addToken(cfg);
        return auth;
    }

    VoiceApiHandler handler;
};

TEST(VoiceApiHandlerConstructionTest, RejectsNullVoiceAssistant) {
    EXPECT_THROW((VoiceApiHandler{nullptr}), std::invalid_argument);
}

TEST_F(VoiceApiHandlerPathValidationTest, MacroRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/macros/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid macro ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, MacroRejectsMissingId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/macros/"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Missing macro ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, SessionRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/sessions/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid session ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, SessionRejectsMissingId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/sessions/"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Missing session ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, RecordingRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid recording ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, RecordingRejectsMissingId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings/"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Missing recording ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, ProfileDeleteRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::delete_, "/api/v1/voice/auth/profiles/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid profile ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, ProfileDeleteRejectsMissingId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::delete_, "/api/v1/voice/auth/profiles/"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Missing profile ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthVerifyRejectsNonStringProfileId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/verify",
        json{{"profile_id", 42}, {"audio", "QUJD"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "profile_id must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthVerifyRejectsInvalidProfileId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/verify",
        json{{"profile_id", "../bad"}, {"audio", "QUJD"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid profile_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthVerifyRejectsEmptyAudio) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/verify",
        json{{"profile_id", "profile-1"}, {"audio", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthAuthenticateRejectsNonStringAudio) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/authenticate",
        json{{"user_id", "user-1"}, {"audio", 42}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio must be a base64 string");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthAuthenticateRejectsInvalidUserId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/authenticate",
        json{{"user_id", "../bad"}, {"audio", "QUJD"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid user_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthAuthenticateRejectsEmptyAudio) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/authenticate",
        json{{"user_id", "user-1"}, {"audio", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthIdentifyRejectsNonStringAudio) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/identify",
        json{{"candidate_profiles", json::array({"profile-1"})}, {"audio", false}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio must be a base64 string");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthIdentifyRejectsEmptyCandidateProfiles) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/identify",
        json{{"candidate_profiles", json::array()}, {"audio", "QUJD"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "candidate_profiles must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthIdentifyRejectsInvalidCandidateProfile) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/identify",
        json{{"candidate_profiles", json::array({"../bad"})}, {"audio", "QUJD"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each candidate_profiles element must be a valid identifier");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthIdentifyRejectsEmptyAudio) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/identify",
        json{{"candidate_profiles", json::array({"profile-1"})}, {"audio", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthEnrollRejectsNonBooleanRequireLiveness) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/enroll",
        json{{"user_id", "user-1"},
             {"audio_samples", json::array({"QUJD"})},
             {"require_liveness", "yes"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "require_liveness must be a boolean");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthEnrollRejectsInvalidUserId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/enroll",
        json{{"user_id", "../bad"},
             {"audio_samples", json::array({"QUJD"})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid user_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, AuthEnrollRejectsEmptyAudioSample) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/auth/enroll",
        json{{"user_id", "user-1"},
             {"audio_samples", json::array({""})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each element in audio_samples must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, TranscribeRejectsNonStringAudioBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/transcribe",
        json{{"audio_base64", 42}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_base64 must be a base64 string");
}

TEST_F(VoiceApiHandlerPathValidationTest, TranscribeRejectsEmptyAudioBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/transcribe",
        json{{"audio_base64", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_base64 must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, TranscribeRejectsEmptyAudioUrl) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/transcribe",
        json{{"audio_url", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_url must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, TranscribeRejectsNonBooleanTimestamps) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/transcribe",
        json{{"audio_base64", "QUJD"}, {"timestamps", "yes"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "timestamps must be a boolean");
}

TEST_F(VoiceApiHandlerPathValidationTest, SynthesizeRejectsNonStringText) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/synthesize",
        json{{"text", 42}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "text must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, SynthesizeRejectsEmptyText) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/synthesize",
        json{{"text", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "text must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, SynthesizeRejectsNonBooleanReturnBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/synthesize",
        json{{"text", "hello"}, {"return_base64", "yes"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "return_base64 must be a boolean");
}

TEST_F(VoiceApiHandlerPathValidationTest, SynthesizeRejectsUnsupportedFormat) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/synthesize",
        json{{"text", "hello"}, {"format", "flac"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "format must be one of: wav, mp3, ogg");
}

TEST_F(VoiceApiHandlerPathValidationTest, VoiceCommandRejectsInvalidSessionId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/command",
        json{{"session_id", "../bad"}, {"text", "hello"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid session_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, VoiceCommandRejectsNonStringAudioBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/command",
        json{{"audio_base64", 42}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_base64 must be a base64 string");
}

TEST_F(VoiceApiHandlerPathValidationTest, VoiceCommandRejectsEmptyText) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/command",
        json{{"text", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "text must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, VoiceCommandRejectsEmptyAudioBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/command",
        json{{"audio_base64", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_base64 must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, StreamCommandRejectsInvalidSessionId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/command/stream",
        json{{"session_id", "../bad"}, {"audio_base64", "QUJD"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid session_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, StreamCommandRejectsEmptyAudioBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/command/stream",
        json{{"audio_base64", ""}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_base64 must not be empty");
}

TEST_F(VoiceApiHandlerPathValidationTest, CreateMacroRejectsNonStringTriggerPhrase) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/macros",
        json{{"trigger_phrase", 42}, {"steps", json::array()}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "trigger_phrase must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, CreateMacroRejectsNonObjectOptions) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/macros",
        json{{"trigger_phrase", "hello"}, {"steps", json::array()}, {"options", true}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "options must be an object");
}

TEST_F(VoiceApiHandlerPathValidationTest, CreateMacroRejectsNonPositiveExecutionTime) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/macros",
        json{{"trigger_phrase", "hello"},
             {"steps", json::array()},
             {"options", json{{"max_execution_time_ms", 0}}}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "options.max_execution_time_ms must be positive");
}

TEST_F(VoiceApiHandlerPathValidationTest, CreateMacroRejectsNonStringTagElement) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/macros",
        json{{"trigger_phrase", "hello"},
             {"steps", json::array()},
             {"tags", json::array({"ok", 1})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each tags element must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, UpdateMacroRejectsNonObjectStep) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::put,
        "/api/v1/voice/macros/macro-1",
        json{{"steps", json::array({"bad"})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each step must be an object");
}

TEST_F(VoiceApiHandlerPathValidationTest, CreateMacroRejectsNonStringStepAction) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/macros",
        json{{"trigger_phrase", "hello"},
             {"steps", json::array({json{{"action", 42}}})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each step action must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, UpdateMacroRejectsNonStringParameterValue) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::put,
        "/api/v1/voice/macros/macro-1",
        json{{"steps", json::array({json{{"parameters", json{{"k", 1}}}}})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each step parameter value must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, UpdateMacroRejectsNonStringTagElement) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::put,
        "/api/v1/voice/macros/macro-1",
        json{{"steps", json::array()},
             {"tags", json::array({"ok", false})}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Each tags element must be a string");
}

TEST_F(VoiceApiHandlerPathValidationTest, UpdateMacroRejectsNonPositiveExecutionTime) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::put,
        "/api/v1/voice/macros/macro-1",
        json{{"steps", json::array()},
             {"options", json{{"max_execution_time_ms", -1}}}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "options.max_execution_time_ms must be positive");
}

TEST_F(VoiceApiHandlerPathValidationTest, RecordCallRejectsNonStringAudioBase64) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/call/record",
        json{{"audio_base64", 42}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "audio_base64 must be a base64 string");
}

TEST_F(VoiceApiHandlerPathValidationTest, RecordCallRejectsInvalidCallId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/call/record",
        json{{"audio_base64", "QUJD"}, {"call_id", "../bad"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid call_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, RecordCallRejectsEndTimeBeforeStartTime) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/call/record",
        json{{"audio_base64", "QUJD"}, {"start_time", 200}, {"end_time", 100}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "end_time must be greater than or equal to start_time");
}

TEST_F(VoiceApiHandlerPathValidationTest, GenerateProtocolRejectsNonArrayParticipants) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/meeting/protocol",
        json{{"audio_base64", "QUJD"}, {"participants", "alice"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "participants must be an array");
}

TEST_F(VoiceApiHandlerPathValidationTest, GenerateProtocolRejectsInvalidMeetingId) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/meeting/protocol",
        json{{"audio_base64", "QUJD"}, {"meeting_id", "../bad"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid meeting_id");
}

TEST_F(VoiceApiHandlerPathValidationTest, GenerateProtocolRejectsEndTimeBeforeStartTime) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/meeting/protocol",
        json{{"audio_base64", "QUJD"}, {"start_time", 200}, {"end_time", 100}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "end_time must be greater than or equal to start_time");
}

TEST_F(VoiceApiHandlerPathValidationTest, SessionContextRejectsNonObjectContext) {
    const auto response = handler.handleRequest(makeJsonRequest(
        http::verb::post,
        "/api/v1/voice/sessions/session-1/context",
        json{{"context", "bad"}}));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "context must be an object");
}

TEST_F(VoiceApiHandlerPathValidationTest, ListRecordingsRejectsInvalidTier) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings?tier=archive"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "tier must be one of: hot, warm, cold");
}

TEST_F(VoiceApiHandlerPathValidationTest, ListRecordingsRejectsInvalidLimit) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings?limit=abc"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "limit must be an integer between 1 and 1000");
}

TEST_F(VoiceApiHandlerPathValidationTest, SearchTranscriptsRejectsInvalidLimit) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings/search?q=hello&limit=abc"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "limit must be an integer between 1 and 1000");
}

TEST_F(VoiceApiHandlerPathValidationTest, GetRecordingRejectsInvalidFormat) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings/record-1?format=xml"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "format must be one of: metadata, audio");
}

} // namespace
} // namespace themis::server