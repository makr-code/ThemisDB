// Test: Tenant Buffer Manager and Isolation
// Tests for multi-tenant CDC with per-tenant buffers, metrics, and quotas

#include <gtest/gtest.h>
#include "cdc/tenant_buffer_manager.h"
#include "cdc/changefeed.h"
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>
#include <thread>

using namespace themis;
using namespace themis::cdc;

class TenantBufferManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for RocksDB
        test_db_path_ = "/tmp/test_tenant_cdc_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_db_path_);
        
        // Open RocksDB as a TransactionDB (required by Changefeed)
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_options;
        rocksdb::TransactionDB* db_ptr;
        auto status = rocksdb::TransactionDB::Open(options, txn_options, test_db_path_, &db_ptr);
        ASSERT_TRUE(status.ok()) << "Failed to open DB: " << status.ToString();
        db_.reset(db_ptr);
        
        // Create changefeed
        changefeed_ = std::make_unique<Changefeed>(db_.get());
    }
    
    void TearDown() override {
        changefeed_.reset();
        db_.reset();
        std::filesystem::remove_all(test_db_path_);
    }
    
    std::string test_db_path_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::unique_ptr<Changefeed> changefeed_;
};

// ===== Basic Tenant Operations =====

TEST_F(TenantBufferManagerTest, CreateManagerSuccessfully) {
    ChangefeedBufferConfig config;
    TenantBufferManager manager(changefeed_.get(), config);
    
    // Should start successfully
    manager.start();
    EXPECT_TRUE(manager.getActiveTenants().empty());
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, RecordEventCreatesNewTenant) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "test_key";
    event.value = "test_value";
    
    // Record event for tenant1 (auto-creates tenant)
    auto recorded = manager.recordEvent("tenant1", event);
    
    EXPECT_TRUE(manager.hasTenant("tenant1"));
    EXPECT_EQ(manager.getActiveTenants().size(), 1);
    EXPECT_EQ(manager.getActiveTenants()[0], "tenant1");
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, MultiTenantEventRecording) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Record events for 3 different tenants
    for (int tenant = 1; tenant <= 3; tenant++) {
        for (int i = 0; i < 10; i++) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "key_" + std::to_string(i);
            event.value = "value_" + std::to_string(i);
            
            manager.recordEvent("tenant" + std::to_string(tenant), event);
        }
    }
    
    // Verify all tenants created
    EXPECT_EQ(manager.getActiveTenants().size(), 3);
    EXPECT_TRUE(manager.hasTenant("tenant1"));
    EXPECT_TRUE(manager.hasTenant("tenant2"));
    EXPECT_TRUE(manager.hasTenant("tenant3"));
    
    // Verify stats
    auto stats1 = manager.getTenantStats("tenant1");
    ASSERT_TRUE(stats1.has_value());
    EXPECT_EQ(stats1->events_recorded, 10);
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, TenantIsolation) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Record 100 events for tenant1
    for (int i = 0; i < 100; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        manager.recordEvent("tenant1", event);
    }
    
    // Record 50 events for tenant2
    for (int i = 0; i < 50; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        manager.recordEvent("tenant2", event);
    }
    
    // Verify isolation
    auto stats1 = manager.getTenantStats("tenant1");
    auto stats2 = manager.getTenantStats("tenant2");
    
    ASSERT_TRUE(stats1.has_value());
    ASSERT_TRUE(stats2.has_value());
    
    EXPECT_EQ(stats1->events_recorded, 100);
    EXPECT_EQ(stats2->events_recorded, 50);
    
    // Stats should be independent
    EXPECT_NE(stats1->events_recorded, stats2->events_recorded);
    
    manager.stop();
}

// ===== Tenant Configuration =====

TEST_F(TenantBufferManagerTest, ConfigureTenant) {
    TenantBufferManager manager(changefeed_.get());
    
    TenantConfig config;
    config.tenant_id = "tenant1";
    config.enable_quotas = true;
    config.max_events_per_second = 5000;
    config.max_memory_bytes = 10 * 1024 * 1024;  // 10MB
    
    manager.configureTenant(config);
    
    auto retrieved_config = manager.getTenantConfig("tenant1");
    ASSERT_TRUE(retrieved_config.has_value());
    EXPECT_EQ(retrieved_config->tenant_id, "tenant1");
    EXPECT_TRUE(retrieved_config->enable_quotas);
    EXPECT_EQ(retrieved_config->max_events_per_second, 5000);
}

TEST_F(TenantBufferManagerTest, DisableAndEnableTenant) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Create tenant with event
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "key1";
    event.value = "value1";
    manager.recordEvent("tenant1", event);
    
    // Disable tenant
    manager.disableTenant("tenant1");
    
    // Try to record event (should throw)
    EXPECT_THROW({
        manager.recordEvent("tenant1", event);
    }, CDCException);
    
    // Enable tenant
    manager.enableTenant("tenant1");
    
    // Should work now
    EXPECT_NO_THROW({
        manager.recordEvent("tenant1", event);
    });
    
    manager.stop();
}

// ===== Quota Enforcement =====

TEST_F(TenantBufferManagerTest, QuotaEnforcement) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Configure tenant with very low quota
    TenantConfig config;
    config.tenant_id = "tenant1";
    config.enable_quotas = true;
    config.max_buffered_events = 10;  // Very low limit
    config.buffer_config.async_flush = false;  // Disable async to control flush
    
    manager.configureTenant(config);
    
    // Record events up to quota
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        
        EXPECT_NO_THROW({
            manager.recordEvent("tenant1", event);
        });
    }
    
    // Next event should fail (quota exceeded)
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "key_overflow";
    event.value = "value";
    
    EXPECT_THROW({
        manager.recordEvent("tenant1", event);
    }, CDCException);
    
    // Check quota violation stat
    auto stats = manager.getTenantStats("tenant1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_GT(stats->quota_violations, 0);
    
    manager.stop();
}

// ===== Flush Operations =====

TEST_F(TenantBufferManagerTest, FlushTenant) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Record events for tenant1
    for (int i = 0; i < 20; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        manager.recordEvent("tenant1", event);
    }
    
    // Flush tenant
    size_t flushed = manager.flushTenant("tenant1");
    EXPECT_GT(flushed, 0);
    
    // Check flush stats
    auto stats = manager.getTenantStats("tenant1");
    ASSERT_TRUE(stats.has_value());
    EXPECT_GT(stats->events_flushed, 0);
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, FlushAll) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Record events for multiple tenants
    for (int tenant = 1; tenant <= 3; tenant++) {
        for (int i = 0; i < 10; i++) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "key_" + std::to_string(i);
            event.value = "value";
            manager.recordEvent("tenant" + std::to_string(tenant), event);
        }
    }
    
    // Flush all tenants
    size_t total_flushed = manager.flushAll();
    EXPECT_GT(total_flushed, 0);
    
    manager.stop();
}

// ===== Metrics =====

TEST_F(TenantBufferManagerTest, TenantMetrics) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Record events
    for (int i = 0; i < 50; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "value_" + std::to_string(i);
        manager.recordEvent("tenant1", event);
    }
    
    // Get tenant metrics
    auto metrics = manager.getTenantMetrics("tenant1");
    ASSERT_TRUE(metrics.has_value());
    EXPECT_EQ(metrics->get().events_recorded.load(), 50);
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, GlobalMetricsAggregation) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Record events for multiple tenants
    for (int tenant = 1; tenant <= 3; tenant++) {
        for (int i = 0; i < 20; i++) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "key_" + std::to_string(i);
            event.value = "value";
            manager.recordEvent("tenant" + std::to_string(tenant), event);
        }
    }
    
    // Get global metrics
    auto global_metrics = manager.getGlobalMetrics();
    
    // Should aggregate across all tenants (3 * 20 = 60)
    EXPECT_EQ(global_metrics["events_recorded"].get<uint64_t>(), 60);
    
    manager.stop();
}

// ===== Tenant Management =====

TEST_F(TenantBufferManagerTest, RemoveTenant) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Create tenant
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "key1";
    event.value = "value1";
    manager.recordEvent("tenant1", event);
    
    EXPECT_TRUE(manager.hasTenant("tenant1"));
    
    // Remove tenant
    manager.removeTenant("tenant1");
    
    EXPECT_FALSE(manager.hasTenant("tenant1"));
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, GetAllTenantStats) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    // Create multiple tenants
    for (int tenant = 1; tenant <= 5; tenant++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key1";
        event.value = "value1";
        manager.recordEvent("tenant" + std::to_string(tenant), event);
    }
    
    // Get all stats
    auto all_stats = manager.getAllTenantStats();
    
    EXPECT_EQ(all_stats.size(), 5);
    EXPECT_TRUE(all_stats.contains("tenant1"));
    EXPECT_TRUE(all_stats.contains("tenant5"));
    
    manager.stop();
}

// ===== Concurrent Access =====

TEST_F(TenantBufferManagerTest, ConcurrentMultiTenantAccess) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    const int num_tenants = 5;
    const int events_per_tenant = 100;
    
    std::vector<std::thread> threads;
    
    // Spawn threads for each tenant
    for (int tenant = 0; tenant < num_tenants; tenant++) {
        threads.emplace_back([&manager, tenant, events_per_tenant]() {
            std::string tenant_id = "tenant" + std::to_string(tenant);
            
            for (int i = 0; i < events_per_tenant; i++) {
                Changefeed::ChangeEvent event;
                event.type = Changefeed::ChangeEventType::EVENT_PUT;
                event.key = "key_" + std::to_string(i);
                event.value = "value_" + std::to_string(i);
                
                try {
                    manager.recordEvent(tenant_id, event);
                } catch (const std::exception& e) {
                    // Ignore errors in concurrent test
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all tenants created
    EXPECT_EQ(manager.getActiveTenants().size(), num_tenants);
    
    // Verify each tenant has recorded events
    for (int tenant = 0; tenant < num_tenants; tenant++) {
        std::string tenant_id = "tenant" + std::to_string(tenant);
        auto stats = manager.getTenantStats(tenant_id);
        ASSERT_TRUE(stats.has_value());
        EXPECT_GT(stats->events_recorded, 0);
    }
    
    manager.stop();
}

// ===== Error Handling =====

TEST_F(TenantBufferManagerTest, EmptyTenantIdThrows) {
    TenantBufferManager manager(changefeed_.get());
    manager.start();
    
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "key1";
    event.value = "value1";
    
    // Empty tenant ID should throw
    EXPECT_THROW({
        manager.recordEvent("", event);
    }, CDCException);
    
    manager.stop();
}

TEST_F(TenantBufferManagerTest, RecordWhenNotRunningThrows) {
    TenantBufferManager manager(changefeed_.get());
    // Don't start manager
    
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "key1";
    event.value = "value1";
    
    // Should throw when not running
    EXPECT_THROW({
        manager.recordEvent("tenant1", event);
    }, CDCException);
}
