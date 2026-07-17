#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "server/wal_api_handler.h"

namespace http = boost::beast::http;

namespace themis::server {

namespace {

std::string computeHmacSha256Hex(const std::string& key, const std::string& data) {
    unsigned int len = 0;
    unsigned char* result = HMAC(EVP_sha256(),
                                 key.data(),
                                 static_cast<int>(key.size()),
                                 reinterpret_cast<const unsigned char*>(data.data()),
                                 data.size(),
                                 nullptr,
                                 &len);
    if (!result || len == 0) {
        return {};
    }

    static constexpr char kHexDigits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(len * 2);
    for (unsigned int i = 0; i < len; ++i) {
        hex.push_back(kHexDigits[(result[i] >> 4) & 0x0F]);
        hex.push_back(kHexDigits[result[i] & 0x0F]);
    }
    return hex;
}

http::request<http::string_body> makeApplyRequest(const std::string& body = R"({"entries":[]})") {
    http::request<http::string_body> req{http::verb::post, "/api/v1/wal/apply", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

} // namespace

TEST(WALApiHandlerSecurityTest, SharedSecretMismatchReturnsUnauthorized) {
    WALApiHandler handler{
        nullptr, nullptr, nullptr, nullptr, nullptr, "server-shared-secret", ""};

    auto req = makeApplyRequest();
    req.set("X-WAL-Auth", "wrong-secret");

    auto res = handler.handleApply(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST(WALApiHandlerSecurityTest, SharedSecretMatchContinuesToBusinessChecks) {
    WALApiHandler handler{
        nullptr, nullptr, nullptr, nullptr, nullptr, "server-shared-secret", ""};

    auto req = makeApplyRequest();
    req.set("X-WAL-Auth", "server-shared-secret");

    auto res = handler.handleApply(req);
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}

TEST(WALApiHandlerSecurityTest, MissingHmacHeaderReturnsUnauthorized) {
    WALApiHandler handler{
        nullptr, nullptr, nullptr, nullptr, nullptr, "", "hmac-secret"};

    auto req = makeApplyRequest();

    auto res = handler.handleApply(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST(WALApiHandlerSecurityTest, WrongHmacReturnsUnauthorized) {
    WALApiHandler handler{
        nullptr, nullptr, nullptr, nullptr, nullptr, "", "hmac-secret"};

    auto req = makeApplyRequest();
    req.set("X-WAL-HMAC", "bad-hmac");

    auto res = handler.handleApply(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST(WALApiHandlerSecurityTest, CorrectHmacContinuesToBusinessChecks) {
    WALApiHandler handler{
        nullptr, nullptr, nullptr, nullptr, nullptr, "", "hmac-secret"};

    auto req = makeApplyRequest();
    req.set("X-WAL-HMAC", computeHmacSha256Hex("hmac-secret", req.body()));

    auto res = handler.handleApply(req);
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}

} // namespace themis::server
