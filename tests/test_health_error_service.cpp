#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

#include "server/health_error_service.h"
#include "utils/error_registry.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HealthErrorServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure health service on test port
        themis::server::HealthErrorService::Config config;
        config.bind_address = "127.0.0.1";
        config.port = 19090; // Use different port for testing
        config.enabled = true;
        
        service_ = std::make_unique<themis::server::HealthErrorService>(config);
        service_->start();
        
        // Wait for service to be ready (poll with timeout)
        int attempts = 0;
        while (!service_->isRunning() && attempts < 20) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            attempts++;
        }
        
        // Give additional time for socket to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    void TearDown() override {
        if (service_) {
            service_->stop();
        }
    }
    
    static http::response<http::string_body> http_get(
        const std::string& host, 
        const std::string& port, 
        const std::string& target) 
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        
        auto const results = resolver.resolve(host, port);
        stream.connect(results);
        
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "HealthErrorServiceTest");
        
        http::write(stream, req);
        
        beast::flat_buffer buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);
        
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        return res;
    }
    
    std::unique_ptr<themis::server::HealthErrorService> service_;
};

TEST_F(HealthErrorServiceTest, ServiceStartsSuccessfully) {
    ASSERT_TRUE(service_->isRunning());
}

TEST_F(HealthErrorServiceTest, HealthEndpointReturnsSuccess) {
    auto res = http_get("127.0.0.1", "19090", "/health");
    
    ASSERT_EQ(res.result(), http::status::ok) << "Status: " << res.result_int();
    EXPECT_EQ(res[http::field::content_type], "application/json");
    
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "healthy");
    ASSERT_TRUE(body.contains("uptime_seconds"));
    ASSERT_TRUE(body.contains("timestamp"));
}

TEST_F(HealthErrorServiceTest, HealthComponentsEndpointReturnsDetails) {
    auto res = http_get("127.0.0.1", "19090", "/health/components");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("health_error_service"));
    ASSERT_TRUE(body.contains("error_registry"));
    
    auto health_svc = body["health_error_service"];
    EXPECT_EQ(health_svc["status"], "healthy");
    EXPECT_EQ(health_svc["port"], 19090);
}

TEST_F(HealthErrorServiceTest, ErrorsEndpointReturnsErrorList) {
    auto res = http_get("127.0.0.1", "19090", "/errors");
    
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("errors"));
    EXPECT_TRUE(body["errors"].is_array());
    EXPECT_GT(body["errors"].size(), 0) << "Should have registered errors";
}

TEST_F(HealthErrorServiceTest, SpecificErrorCodeReturnsDetails) {
    // Test with known error code (2000 - LLM model not found)
    auto res = http_get("127.0.0.1", "19090", "/errors/2000");
    
    ASSERT_EQ(res.result(), http::status::ok) << res.body();
    
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("status"));
    EXPECT_EQ(body["status"], "success");
    ASSERT_TRUE(body.contains("error"));
    
    auto error = body["error"];
    EXPECT_EQ(error["code"], 2000);
    EXPECT_TRUE(error.contains("message_template"));
    EXPECT_TRUE(error.contains("category"));
    EXPECT_EQ(error["category"], "LLM");
}

TEST_F(HealthErrorServiceTest, InvalidErrorCodeReturns404) {
    auto res = http_get("127.0.0.1", "19090", "/errors/99999");
    
    ASSERT_EQ(res.result(), http::status::not_found);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_EQ(body["status"], "not_found");
}

TEST_F(HealthErrorServiceTest, CategoriesEndpointReturnsCategories) {
    auto res = http_get("127.0.0.1", "19090", "/errors/categories");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    ASSERT_TRUE(body.contains("categories"));
    EXPECT_TRUE(body["categories"].is_array());
    EXPECT_GT(body["categories"].size(), 0);
    
    // Check for known categories
    auto categories = body["categories"];
    bool has_storage = false;
    bool has_llm = false;
    for (const auto& cat : categories) {
        std::string cat_str = cat.get<std::string>();
        if (cat_str == "Storage") {
          has_storage = true;
        }
        if (cat_str == "LLM") {
          has_llm = true;
        }
    }
    EXPECT_TRUE(has_storage) << "Should have Storage category";
    EXPECT_TRUE(has_llm) << "Should have LLM category";
}

TEST_F(HealthErrorServiceTest, SearchEndpointFindsErrors) {
    auto res = http_get("127.0.0.1", "19090", "/errors/search?q=model");
    
    ASSERT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_EQ(body["status"], "success");
    EXPECT_EQ(body["query"], "model");
    ASSERT_TRUE(body.contains("errors"));
    EXPECT_GT(body["errors"].size(), 0) << "Should find errors with 'model' keyword";
}

TEST_F(HealthErrorServiceTest, SearchWithoutQueryReturns400) {
    auto res = http_get("127.0.0.1", "19090", "/errors/search");
    
    ASSERT_EQ(res.result(), http::status::bad_request);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_EQ(body["status"], "error");
    EXPECT_TRUE(body.contains("message"));
}

TEST_F(HealthErrorServiceTest, InvalidEndpointReturns404) {
    auto res = http_get("127.0.0.1", "19090", "/invalid");
    
    ASSERT_EQ(res.result(), http::status::not_found);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_EQ(body["status"], "error");
    ASSERT_TRUE(body.contains("available_endpoints"));
}

TEST_F(HealthErrorServiceTest, UptimeIncreasesOverTime) {
    auto res1 = http_get("127.0.0.1", "19090", "/health");
    auto body1 = nlohmann::json::parse(res1.body());
    int64_t uptime1 = body1["uptime_seconds"];
    
    // Sleep for just over 1 second to ensure uptime increases
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    auto res2 = http_get("127.0.0.1", "19090", "/health");
    auto body2 = nlohmann::json::parse(res2.body());
    int64_t uptime2 = body2["uptime_seconds"];
    
    EXPECT_GE(uptime2, uptime1 + 1) << "Uptime should increase over time";
}

TEST_F(HealthErrorServiceTest, ServiceStopsGracefully) {
    ASSERT_TRUE(service_->isRunning());
    
    service_->stop();
    
    EXPECT_FALSE(service_->isRunning());
    
    // Verify service is no longer accepting connections
    // (Connection should fail after stop)
    EXPECT_THROW({
        try {
            http_get("127.0.0.1", "19090", "/health");
        } catch (const boost::system::system_error& e) {
            // Expected: connection refused or similar
            throw;
        }
    }, boost::system::system_error);
}

TEST_F(HealthErrorServiceTest, MultipleRequestsHandledCorrectly) {
    // Send multiple requests in quick succession
    for (int i = 0; i < 5; ++i) {
        auto res = http_get("127.0.0.1", "19090", "/health");
        ASSERT_EQ(res.result(), http::status::ok) << "Request " << i << " failed";
    }
}
