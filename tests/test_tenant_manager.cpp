/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_tenant_manager.cpp                            ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:59:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     504                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "server/tenant_manager.h"
#include <thread>
#include <vector>

using namespace themis;

class TenantManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset tenant manager to default state
        auto& tm = TenantManager::instance();
        
        // Enable default tenant for tests (backward compatibility mode)
        TenantManager::Config config = tm.getConfig();
        config.allow_default_tenant = true;
        tm.configure(config);
        
        // Remove any non-default tenants
        auto tenants = tm.listTenants();
        for (const auto& t : tenants) {
            if (t.tenant_id != "default") {
                tm.deleteTenant(t.tenant_id);
            }
        }
    }
    
    void TearDown() override {
        // Cleanup
        auto& tm = TenantManager::instance();
        auto tenants = tm.listTenants();
        for (const auto& t : tenants) {
            if (t.tenant_id != "default") {
                tm.deleteTenant(t.tenant_id);
            }
        }
    }
};

// ===== Basic Tenant Lifecycle Tests =====

TEST_F(TenantManagerTest, DefaultTenantExists) {
    auto& tm = TenantManager::instance();
    
    auto defaultTenant = tm.getTenant("default");
    ASSERT_TRUE(defaultTenant.has_value());
    EXPECT_EQ(defaultTenant->tenant_id, "default");
    EXPECT_TRUE(defaultTenant->enabled);
}

TEST_F(TenantManagerTest, CreateTenant) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "acme-corp";
    config.display_name = "ACME Corporation";
    config.max_storage_bytes = 10ULL * 1024 * 1024 * 1024;  // 10 GB
    config.max_documents = 1000000;
    
    auto result = tm.createTenant(config);
    EXPECT_EQ(result, TenantManager::CreateResult::Success);
    
    auto tenant = tm.getTenant("acme-corp");
    ASSERT_TRUE(tenant.has_value());
    EXPECT_EQ(tenant->tenant_id, "acme-corp");
    EXPECT_EQ(tenant->display_name, "ACME Corporation");
    EXPECT_EQ(tenant->max_storage_bytes, 10ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(tenant->max_documents, 1000000ULL);
}

TEST_F(TenantManagerTest, CreateDuplicateTenant) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "test-tenant";
    config.display_name = "Test Tenant";
    
    auto result1 = tm.createTenant(config);
    EXPECT_EQ(result1, TenantManager::CreateResult::Success);
    
    auto result2 = tm.createTenant(config);
    EXPECT_EQ(result2, TenantManager::CreateResult::AlreadyExists);
}

TEST_F(TenantManagerTest, CreateTenantWithEmptyId) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "";
    config.display_name = "Empty ID Tenant";
    
    auto result = tm.createTenant(config);
    EXPECT_EQ(result, TenantManager::CreateResult::InvalidConfig);
}

TEST_F(TenantManagerTest, UpdateTenant) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "update-test";
    config.display_name = "Original Name";
    config.max_storage_bytes = 1024;
    
    EXPECT_EQ(tm.createTenant(config), TenantManager::CreateResult::Success);
    
    // Update the tenant
    config.display_name = "Updated Name";
    config.max_storage_bytes = 2048;
    
    EXPECT_TRUE(tm.updateTenant(config));
    
    auto tenant = tm.getTenant("update-test");
    ASSERT_TRUE(tenant.has_value());
    EXPECT_EQ(tenant->display_name, "Updated Name");
    EXPECT_EQ(tenant->max_storage_bytes, 2048ULL);
}

TEST_F(TenantManagerTest, DeleteTenant) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "delete-test";
    config.display_name = "Delete Test";
    
    tm.createTenant(config);
    EXPECT_TRUE(tm.tenantExists("delete-test"));
    
    EXPECT_TRUE(tm.deleteTenant("delete-test"));
    EXPECT_FALSE(tm.tenantExists("delete-test"));
}

TEST_F(TenantManagerTest, CannotDeleteDefaultTenant) {
    auto& tm = TenantManager::instance();
    
    EXPECT_FALSE(tm.deleteTenant("default"));
    EXPECT_TRUE(tm.tenantExists("default"));
}

TEST_F(TenantManagerTest, EnableDisableTenant) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "enable-test";
    config.display_name = "Enable Test";
    
    tm.createTenant(config);
    
    EXPECT_TRUE(tm.setTenantEnabled("enable-test", false));
    
    auto tenant = tm.getTenant("enable-test");
    ASSERT_TRUE(tenant.has_value());
    EXPECT_FALSE(tenant->enabled);
    
    EXPECT_TRUE(tm.setTenantEnabled("enable-test", true));
    
    tenant = tm.getTenant("enable-test");
    EXPECT_TRUE(tenant->enabled);
}

// ===== Tenant Resolution Tests =====

TEST_F(TenantManagerTest, ResolveTenantFromHeader) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "header-test";
    config.display_name = "Header Test";
    tm.createTenant(config);
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Tenant-ID"] = "header-test";
    
    auto ctx = tm.resolveContext(headers, "/api/documents", "user123");
    
    ASSERT_TRUE(ctx.has_value());
    EXPECT_EQ(ctx->tenant_id, "header-test");
    EXPECT_EQ(ctx->user_id, "user123");
}

TEST_F(TenantManagerTest, ResolveTenantFromPath) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "path-test";
    config.display_name = "Path Test";
    tm.createTenant(config);
    
    std::unordered_map<std::string, std::string> headers;  // No header
    
    auto ctx = tm.resolveContext(headers, "/tenants/path-test/documents", "user456");
    
    ASSERT_TRUE(ctx.has_value());
    EXPECT_EQ(ctx->tenant_id, "path-test");
}

TEST_F(TenantManagerTest, ResolveDefaultTenant) {
    auto& tm = TenantManager::instance();
    
    std::unordered_map<std::string, std::string> headers;
    
    auto ctx = tm.resolveContext(headers, "/api/documents", "user789");
    
    ASSERT_TRUE(ctx.has_value());
    EXPECT_EQ(ctx->tenant_id, "default");
}

TEST_F(TenantManagerTest, ResolveDisabledTenantFails) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "disabled-test";
    config.display_name = "Disabled Test";
    tm.createTenant(config);
    tm.setTenantEnabled("disabled-test", false);
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Tenant-ID"] = "disabled-test";
    
    auto ctx = tm.resolveContext(headers, "/api/documents", "user");
    
    EXPECT_FALSE(ctx.has_value());
}

// ===== Quota Tests =====

TEST_F(TenantManagerTest, CheckQuotaStorage) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "quota-test";
    config.display_name = "Quota Test";
    config.max_storage_bytes = 1000;
    tm.createTenant(config);
    
    // Initially should allow
    auto result1 = tm.checkQuota("quota-test", "storage", 500);
    EXPECT_TRUE(result1.allowed);
    
    // Simulate usage
    tm.incrementStorage("quota-test", 800);
    
    // Now should reject exceeding request
    auto result2 = tm.checkQuota("quota-test", "storage", 300);
    EXPECT_FALSE(result2.allowed);
    EXPECT_EQ(result2.reason, "Storage quota exceeded");
    
    // But should allow smaller request
    auto result3 = tm.checkQuota("quota-test", "storage", 100);
    EXPECT_TRUE(result3.allowed);
}

TEST_F(TenantManagerTest, CheckQuotaConnections) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "conn-quota-test";
    config.display_name = "Connection Quota Test";
    config.max_connections = 2;
    tm.createTenant(config);
    
    // Acquire connections
    EXPECT_TRUE(tm.acquireConnection("conn-quota-test"));
    EXPECT_TRUE(tm.acquireConnection("conn-quota-test"));
    
    // Third connection should fail
    EXPECT_FALSE(tm.acquireConnection("conn-quota-test"));
    
    // Release one
    tm.releaseConnection("conn-quota-test");
    
    // Now should work
    EXPECT_TRUE(tm.acquireConnection("conn-quota-test"));
}

// ===== Usage Tracking Tests =====

TEST_F(TenantManagerTest, TrackRequests) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "tracking-test";
    config.display_name = "Tracking Test";
    tm.createTenant(config);
    
    tm.recordRequest("tracking-test");
    tm.recordRequest("tracking-test");
    tm.recordRequest("tracking-test");
    
    auto* usage = tm.getUsage("tracking-test");
    ASSERT_NE(usage, nullptr);
    EXPECT_EQ(usage->total_requests.load(), 3ULL);
}

TEST_F(TenantManagerTest, TrackBytesReadWritten) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "bytes-test";
    config.display_name = "Bytes Test";
    tm.createTenant(config);
    
    tm.recordBytesRead("bytes-test", 1000);
    tm.recordBytesWritten("bytes-test", 500);
    tm.recordBytesRead("bytes-test", 2000);
    
    auto* usage = tm.getUsage("bytes-test");
    ASSERT_NE(usage, nullptr);
    EXPECT_EQ(usage->total_bytes_read.load(), 3000ULL);
    EXPECT_EQ(usage->total_bytes_written.load(), 500ULL);
}

// ===== Concurrency Tests =====

TEST_F(TenantManagerTest, ConcurrentConnectionAcquisition) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "concurrent-test";
    config.display_name = "Concurrent Test";
    config.max_connections = 10;
    tm.createTenant(config);
    
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&tm, &successCount]() {
            if (tm.acquireConnection("concurrent-test")) {
                ++successCount;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                tm.releaseConnection("concurrent-test");
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // At most 10 should have succeeded at any one time
    // But all should have eventually gotten a chance
    auto* usage = tm.getUsage("concurrent-test");
    EXPECT_EQ(usage->active_connections.load(), 0U);  // All released
}

// ===== Tenant Context Guard Tests =====

TEST_F(TenantManagerTest, TenantContextGuard) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "guard-test";
    config.display_name = "Guard Test";
    config.max_connections = 5;
    tm.createTenant(config);
    
    {
        TenantContext ctx = TenantContext::fromConfig(config, "user1");
        TenantContextGuard guard(ctx);
        
        EXPECT_TRUE(guard.hasConnection());
        EXPECT_EQ(guard.context().tenant_id, "guard-test");
        
        // Check connection is acquired
        auto* usage = tm.getUsage("guard-test");
        EXPECT_EQ(usage->active_connections.load(), 1U);
        EXPECT_EQ(usage->total_requests.load(), 1ULL);
    }
    
    // Connection should be released after guard goes out of scope
    auto* usage = tm.getUsage("guard-test");
    EXPECT_EQ(usage->active_connections.load(), 0U);
}

// ===== Metrics Tests =====

TEST_F(TenantManagerTest, GetMetrics) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "metrics-test";
    config.display_name = "Metrics Test";
    tm.createTenant(config);
    
    tm.recordRequest("metrics-test");
    tm.incrementStorage("metrics-test", 1000);
    tm.incrementDocuments("metrics-test", 5);
    
    std::string metrics = tm.getMetrics();
    
    EXPECT_TRUE(metrics.find("themis_tenant_count") != std::string::npos);
    EXPECT_TRUE(metrics.find("themis_tenant_storage_bytes") != std::string::npos);
    EXPECT_TRUE(metrics.find("metrics-test") != std::string::npos);
}

// ===== Feature Flags Tests =====

TEST_F(TenantManagerTest, TenantFeatureFlags) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "features-test";
    config.display_name = "Features Test";
    config.allow_gpu_acceleration = false;
    config.allow_vector_search = true;
    tm.createTenant(config);
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Tenant-ID"] = "features-test";
    
    auto ctx = tm.resolveContext(headers, "/api/test", "user");
    
    ASSERT_TRUE(ctx.has_value());
    EXPECT_FALSE(ctx->gpu_allowed);
    EXPECT_TRUE(ctx->vector_search_allowed);
}

// ===== Encryption Key Tests =====

TEST_F(TenantManagerTest, TenantEncryptionKey) {
    auto& tm = TenantManager::instance();
    
    TenantConfig config;
    config.tenant_id = "encryption-test";
    config.display_name = "Encryption Test";
    config.encryption_key_id = "custom-key-123";
    tm.createTenant(config);
    
    std::string keyId = tm.getTenantKeyId("encryption-test");
    EXPECT_EQ(keyId, "custom-key-123");
    
    // Default key derivation for tenant without custom key
    std::string defaultKeyId = tm.getTenantKeyId("default");
    EXPECT_EQ(defaultKeyId, "tenant:default:master");
}

// ===== Security Tests =====

TEST_F(TenantManagerTest, SecureDefaultBehavior) {
    // Create a new tenant manager instance with default config
    auto& tm = TenantManager::instance();
    
    // Reset to secure defaults
    TenantManager::Config secure_config;
    secure_config.allow_default_tenant = false;  // Secure default
    tm.configure(secure_config);
    
    // Without allow_default_tenant, extracting tenant should fail
    std::unordered_map<std::string, std::string> headers;  // No tenant header
    auto tenant_id = tm.extractTenantId(headers, "/api/documents");
    
    EXPECT_FALSE(tenant_id.has_value());  // Should return nullopt
    
    // Context resolution should also fail
    auto ctx = tm.resolveContext(headers, "/api/documents", "user123");
    EXPECT_FALSE(ctx.has_value());  // Should return nullopt
    
    // Now with explicit tenant header, it should work if tenant exists
    TenantConfig config;
    config.tenant_id = "secure-test";
    config.display_name = "Secure Test";
    tm.createTenant(config);
    
    headers["X-Tenant-ID"] = "secure-test";
    tenant_id = tm.extractTenantId(headers, "/api/documents");
    EXPECT_TRUE(tenant_id.has_value());
    EXPECT_EQ(*tenant_id, "secure-test");
}

TEST_F(TenantManagerTest, BackwardCompatibilityMode) {
    auto& tm = TenantManager::instance();
    
    // Enable backward compatibility mode
    TenantManager::Config compat_config;
    compat_config.allow_default_tenant = true;
    compat_config.default_tenant_id = "default";
    tm.configure(compat_config);
    
    // Now without explicit tenant, should return default
    std::unordered_map<std::string, std::string> headers;
    auto tenant_id = tm.extractTenantId(headers, "/api/documents");
    
    EXPECT_TRUE(tenant_id.has_value());
    EXPECT_EQ(*tenant_id, "default");
}

// ===== Multi-Tenant Namespace Routing (stripTenantPath) Tests =====

TEST_F(TenantManagerTest, StripTenantPath_BasicPath) {
    auto& tm = TenantManager::instance();

    // Standard path-based tenant routing
    EXPECT_EQ(tm.stripTenantPath("/tenants/acme-corp/documents"), "/documents");
    EXPECT_EQ(tm.stripTenantPath("/tenants/acme-corp/documents/123"), "/documents/123");
    EXPECT_EQ(tm.stripTenantPath("/tenants/acme-corp/api/v1/entities/foo"), "/api/v1/entities/foo");
}

TEST_F(TenantManagerTest, StripTenantPath_NoPrefix) {
    auto& tm = TenantManager::instance();

    // Paths without the tenant prefix are returned unchanged
    EXPECT_EQ(tm.stripTenantPath("/documents"), "/documents");
    EXPECT_EQ(tm.stripTenantPath("/api/v1/entities/foo"), "/api/v1/entities/foo");
    EXPECT_EQ(tm.stripTenantPath("/health"), "/health");
    EXPECT_EQ(tm.stripTenantPath("/"), "/");
}

TEST_F(TenantManagerTest, StripTenantPath_WithQueryString) {
    auto& tm = TenantManager::instance();

    // Query strings should be preserved after stripping
    EXPECT_EQ(tm.stripTenantPath("/tenants/acme-corp/search?q=foo"), "/search?q=foo");
    EXPECT_EQ(tm.stripTenantPath("/tenants/t1/vector/search?limit=10&offset=0"),
              "/vector/search?limit=10&offset=0");
}

TEST_F(TenantManagerTest, StripTenantPath_TenantIdOnly) {
    auto& tm = TenantManager::instance();

    // Path with tenant ID but no trailing sub-path should return root
    EXPECT_EQ(tm.stripTenantPath("/tenants/acme-corp"), "/");
}

TEST_F(TenantManagerTest, StripTenantPath_CustomPrefix) {
    auto& tm = TenantManager::instance();

    // Verify behaviour with a non-default tenant path prefix
    TenantManager::Config cfg = tm.getConfig();
    cfg.tenant_path_prefix = "/ns/";
    cfg.allow_default_tenant = true;
    tm.configure(cfg);

    EXPECT_EQ(tm.stripTenantPath("/ns/acme/documents"), "/documents");
    EXPECT_EQ(tm.stripTenantPath("/tenants/acme/documents"), "/tenants/acme/documents");

    // Restore default prefix for other tests
    TenantManager::Config restore;
    restore.allow_default_tenant = true;
    restore.default_tenant_id = "default";
    tm.configure(restore);
}

