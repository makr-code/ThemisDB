#include <gtest/gtest.h>

#include "server/hot_reload_api_handler.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;

namespace themis { namespace server { 
namespace {

http::request<http::string_body> makeRequest(http::verb method, const std::string& target) {
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    req.prepare_payload();
    return req;
}

json parseBody(const http::response<http::string_body>& response) {
    return json::parse(response.body());
}

class HotReloadApiHandlerValidationTest : public ::testing::Test {
protected:
    HotReloadApiHandler handler{nullptr, nullptr};
};

TEST_F(HotReloadApiHandlerValidationTest, ManifestRejectsInvalidVersionPath) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::get, "/api/updates/manifests/../bad"));

    EXPECT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["error"], "invalid version");
}

TEST_F(HotReloadApiHandlerValidationTest, DownloadRejectsInvalidVersionPath) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::post, "/api/updates/download/../bad"));

    EXPECT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["error"], "invalid version");
}

TEST_F(HotReloadApiHandlerValidationTest, ApplyRejectsInvalidVersionPath) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::post, "/api/updates/apply/../bad"));

    EXPECT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["error"], "invalid version");
}

TEST_F(HotReloadApiHandlerValidationTest, RollbackRejectsInvalidRollbackIdPath) {
    const auto response = handler.handleRequest(
        makeRequest(http::verb::post, "/api/updates/rollback/../bad"));

    EXPECT_EQ(response.result(), http::status::bad_request);
    EXPECT_EQ(parseBody(response)["error"], "invalid rollback id");
}

} // namespace (anonymous)
} // namespace server
} // namespace themis