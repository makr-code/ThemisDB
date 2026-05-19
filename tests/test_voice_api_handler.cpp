#include <gtest/gtest.h>

#include "server/voice_api_handler.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;

namespace themis::server {
namespace {

http::request<http::string_body> makeRequest(http::verb method, const std::string& target) {
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.set(http::field::content_type, "application/json");
    req.prepare_payload();
    return req;
}

json parseBody(const http::response<http::string_body>& response) {
    return json::parse(response.body());
}

class VoiceApiHandlerPathValidationTest : public ::testing::Test {
protected:
    VoiceApiHandler handler{nullptr};
};

TEST_F(VoiceApiHandlerPathValidationTest, MacroRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/macros/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid macro ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, SessionRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/sessions/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid session ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, RecordingRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/v1/voice/recordings/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid recording ID");
}

TEST_F(VoiceApiHandlerPathValidationTest, ProfileDeleteRejectsInvalidId) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::delete_, "/api/v1/voice/auth/profiles/../bad"));

    ASSERT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["details"], "Invalid profile ID");
}

} // namespace
} // namespace themis::server