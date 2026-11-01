#include <gtest/gtest.h>

#include "metadata/imetadata_security_provider.h"
#include "server/auth_middleware.h"
#include "server/schema_api_handler.h"

namespace {

constexpr std::string_view kTestToken = "metadata-security-test-token";
using themis::AuthMiddleware;
using themis::metadata::InMemoryRbacMetadataSecurityProvider;
using themis::metadata::MetadataOperation;
using themis::server::SchemaApiHandler;
namespace http = boost::beast::http;

std::shared_ptr<AuthMiddleware> makeAuthenticatedUser() {
    auto auth = std::make_shared<AuthMiddleware>();
    AuthMiddleware::TokenConfig token;
    token.token = std::string{kTestToken};
    token.user_id = "metadata-test-user";
    auth->addToken(token);
    return auth;
}

http::request<http::string_body> makeRequest(
    http::verb method,
    std::string target,
    bool include_token = true) {
    http::request<http::string_body> req{method, std::move(target), 11};
    if (include_token) {
        req.set(http::field::authorization,
                std::string{"Bearer "} + std::string{kTestToken});
    }
    return req;
}

struct MetadataSecurityHandlerFixture : testing::Test {
    std::shared_ptr<InMemoryRbacMetadataSecurityProvider> provider =
        std::make_shared<InMemoryRbacMetadataSecurityProvider>();
    std::shared_ptr<AuthMiddleware> auth = makeAuthenticatedUser();
    SchemaApiHandler handler{nullptr, nullptr, nullptr};

    MetadataSecurityHandlerFixture() {
        handler.setMetadataSecurityProvider(provider);
        handler.setAuthMiddleware(auth);
    }
};

TEST_F(MetadataSecurityHandlerFixture, MissingBearerIdentityIsRejected) {
    const auto response = handler.handleGetSchema(
        makeRequest(http::verb::get, "/api/v1/schema", false));

    EXPECT_EQ(response.result(), http::status::unauthorized);
}

TEST_F(MetadataSecurityHandlerFixture, SchemaReadChecksTheRequestedTableGrant) {
    provider->grant("metadata-test-user", MetadataOperation::READ_SCHEMA, "orders");

    const auto denied = handler.handleGetTable(
        makeRequest(http::verb::get, "/api/v1/schema/tables/users"));
    EXPECT_EQ(denied.result(), http::status::forbidden);

    const auto allowed = handler.handleGetTable(
        makeRequest(http::verb::get, "/api/v1/schema/tables/orders"));
    EXPECT_EQ(allowed.result(), http::status::service_unavailable);
}

TEST_F(MetadataSecurityHandlerFixture, TableResourceIsNotTruncatedAtPathSeparator) {
    provider->grant("metadata-test-user", MetadataOperation::READ_SCHEMA, "orders");

    const auto response = handler.handleGetTable(
        makeRequest(http::verb::get, "/api/v1/schema/tables/orders/private"));

    EXPECT_EQ(response.result(), http::status::forbidden);
}

TEST_F(MetadataSecurityHandlerFixture, InformationSchemaReadChecksTableGrant) {
    provider->grant("metadata-test-user", MetadataOperation::READ_SCHEMA, "orders");

    const auto denied = handler.handleGetInformationSchema(
        makeRequest(http::verb::get, "/api/v1/information_schema/columns/users"));
    EXPECT_EQ(denied.result(), http::status::forbidden);

    const auto allowed = handler.handleGetInformationSchema(
        makeRequest(http::verb::get, "/api/v1/information_schema/columns/orders"));
    EXPECT_EQ(allowed.result(), http::status::service_unavailable);
}

TEST_F(MetadataSecurityHandlerFixture, StatisticsCollectionRequiresWriteGrant) {
    provider->grant("metadata-test-user", MetadataOperation::READ_STATISTICS, "orders");
    auto request = makeRequest(http::verb::post, "/api/v1/metadata/stats/orders");

    const auto denied = handler.handleCollectStats(request);
    EXPECT_EQ(denied.result(), http::status::forbidden);

    provider->grant("metadata-test-user", MetadataOperation::WRITE_STATISTICS, "orders");
    const auto allowed = handler.handleCollectStats(request);
    EXPECT_EQ(allowed.result(), http::status::service_unavailable);
}

TEST(SchemaApiMetadataSecurityTest, MissingProviderPreservesExistingHandlerBehavior) {
    SchemaApiHandler handler{nullptr, nullptr, nullptr};
    const auto response = handler.handleGetSchema(
        makeRequest(http::verb::get, "/api/v1/schema", false));

    EXPECT_EQ(response.result(), http::status::service_unavailable);
}

} // namespace
