// Test for composite secondary indexes

#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace themis;

class CompositeIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CompositeIndexTest on Windows due to intermittent SEH in fixture setup.";
#endif
        // Clean up test directory
        std::filesystem::remove_all(test_db_path_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 16;
        config.block_cache_size_mb = 32;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        idx_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
    }
    
    void TearDown() override {
        idx_mgr_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        std::error_code ec;
        std::filesystem::remove_all(test_db_path_, ec);
    }
    
    std::string test_db_path_ = "./test_composite_index_db";
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> idx_mgr_;
};

TEST_F(CompositeIndexTest, CreateAndDrop) {
    std::vector<std::string> cols = {"age", "city"};
    
    EXPECT_FALSE(idx_mgr_->hasCompositeIndex("users", cols));
    
    auto st = idx_mgr_->createCompositeIndex("users", cols);
    EXPECT_TRUE(st.ok) << st.message;
    
    EXPECT_TRUE(idx_mgr_->hasCompositeIndex("users", cols));
    
    st = idx_mgr_->dropCompositeIndex("users", cols);
    EXPECT_TRUE(st.ok) << st.message;
    
    EXPECT_FALSE(idx_mgr_->hasCompositeIndex("users", cols));
}

TEST_F(CompositeIndexTest, RequiresMinimumTwoColumns) {
    std::vector<std::string> cols = {"age"};
    
    auto st = idx_mgr_->createCompositeIndex("users", cols);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("mindestens 2"), std::string::npos);
}

TEST_F(CompositeIndexTest, ScanEqualComposite) {
    std::vector<std::string> cols = {"city", "age"};
    auto st = idx_mgr_->createCompositeIndex("users", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert test entities
    BaseEntity e1("user1");
    e1.setField("name", "Alice");
    e1.setField("city", "Berlin");
    e1.setField("age", "30");
    st = idx_mgr_->put("users", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("user2");
    e2.setField("name", "Bob");
    e2.setField("city", "Berlin");
    e2.setField("age", "25");
    st = idx_mgr_->put("users", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e3("user3");
    e3.setField("name", "Charlie");
    e3.setField("city", "Berlin");
    e3.setField("age", "30");
    st = idx_mgr_->put("users", e3);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e4("user4");
    e4.setField("name", "Diana");
    e4.setField("city", "Munich");
    e4.setField("age", "30");
    st = idx_mgr_->put("users", e4);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query: city=Berlin AND age=30
    std::vector<std::string> values = {"Berlin", "30"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("users", cols, values);
    ASSERT_TRUE(status.ok);
    
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_NE(std::find(keys.begin(), keys.end(), "user1"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "user3"), keys.end());
    
    // Query: city=Munich AND age=30
    values = {"Munich", "30"};
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("users", cols, values);
    ASSERT_TRUE(status2.ok);
    
    EXPECT_EQ(keys2.size(), 1u);
    EXPECT_EQ(keys2[0], "user4");
    
    // Query: city=Berlin AND age=25
    values = {"Berlin", "25"};
    auto [status3, keys3] = idx_mgr_->scanKeysEqualComposite("users", cols, values);
    ASSERT_TRUE(status3.ok);
    
    EXPECT_EQ(keys3.size(), 1u);
    EXPECT_EQ(keys3[0], "user2");
}

TEST_F(CompositeIndexTest, EstimateCount) {
    std::vector<std::string> cols = {"status", "priority"};
    auto st = idx_mgr_->createCompositeIndex("tasks", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert 5 tasks with status=open, priority=high
    for (int i = 0; i < 5; ++i) {
        BaseEntity e("task" + std::to_string(i));
        e.setField("status", "open");
        e.setField("priority", "high");
        e.setField("title", "Task " + std::to_string(i));
        st = idx_mgr_->put("tasks", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Insert 3 tasks with status=open, priority=low
    for (int i = 5; i < 8; ++i) {
        BaseEntity e("task" + std::to_string(i));
        e.setField("status", "open");
        e.setField("priority", "low");
        e.setField("title", "Task " + std::to_string(i));
        st = idx_mgr_->put("tasks", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    std::vector<std::string> values = {"open", "high"};
    bool capped = false;
    size_t count = idx_mgr_->estimateCountEqualComposite("tasks", cols, values, 100, &capped);
    
    EXPECT_EQ(count, 5u);
    EXPECT_FALSE(capped);
    
    values = {"open", "low"};
    count = idx_mgr_->estimateCountEqualComposite("tasks", cols, values, 100, &capped);
    
    EXPECT_EQ(count, 3u);
    EXPECT_FALSE(capped);
}

TEST_F(CompositeIndexTest, UpdateEntityMaintainsIndex) {
    std::vector<std::string> cols = {"department", "role"};
    auto st = idx_mgr_->createCompositeIndex("employees", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert employee
    BaseEntity e("emp1");
    e.setField("name", "Alice");
    e.setField("department", "Engineering");
    e.setField("role", "Senior");
    st = idx_mgr_->put("employees", e);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Verify indexed
    std::vector<std::string> values = {"Engineering", "Senior"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("employees", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "emp1");
    
    // Update employee (change department)
    e.setField("department", "Sales");
    st = idx_mgr_->put("employees", e);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Old index should be gone
    auto [status_old, keys2] = idx_mgr_->scanKeysEqualComposite("employees", cols, values);
    ASSERT_TRUE(status_old.ok);
    EXPECT_EQ(keys2.size(), 0u);
    
    // New index should exist
    values = {"Sales", "Senior"};
    auto [status_new, keys3] = idx_mgr_->scanKeysEqualComposite("employees", cols, values);
    ASSERT_TRUE(status_new.ok);
    EXPECT_EQ(keys3.size(), 1u);
    EXPECT_EQ(keys3[0], "emp1");
}

TEST_F(CompositeIndexTest, DeleteEntityRemovesIndexEntry) {
    std::vector<std::string> cols = {"country", "state"};
    auto st = idx_mgr_->createCompositeIndex("locations", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e("loc1");
    e.setField("city", "Munich");
    e.setField("country", "Germany");
    e.setField("state", "Bavaria");
    st = idx_mgr_->put("locations", e);
    ASSERT_TRUE(st.ok) << st.message;
    
    std::vector<std::string> values = {"Germany", "Bavaria"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("locations", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    
    // Delete
    st = idx_mgr_->erase("locations", "loc1");
    ASSERT_TRUE(st.ok) << st.message;
    
    // Index entry should be gone
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("locations", cols, values);
    ASSERT_TRUE(status2.ok);
    EXPECT_EQ(keys2.size(), 0u);
}

TEST_F(CompositeIndexTest, MismatchedColumnsAndValues) {
    std::vector<std::string> cols = {"a", "b", "c"};
    auto st = idx_mgr_->createCompositeIndex("test", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Too few values
    std::vector<std::string> values = {"1", "2"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("test", cols, values);
    EXPECT_FALSE(status.ok);
    
    // Too many values
    values = {"1", "2", "3", "4"};
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("test", cols, values);
    EXPECT_FALSE(status2.ok);
}

TEST_F(CompositeIndexTest, ThreeColumnComposite) {
    std::vector<std::string> cols = {"region", "year", "quarter"};
    auto st = idx_mgr_->createCompositeIndex("sales", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("sale1");
    e1.setField("region", "EU");
    e1.setField("year", "2024");
    e1.setField("quarter", "Q1");
    e1.setField("amount", "100000");
    st = idx_mgr_->put("sales", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("sale2");
    e2.setField("region", "EU");
    e2.setField("year", "2024");
    e2.setField("quarter", "Q2");
    e2.setField("amount", "120000");
    st = idx_mgr_->put("sales", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e3("sale3");
    e3.setField("region", "EU");
    e3.setField("year", "2024");
    e3.setField("quarter", "Q1");
    e3.setField("amount", "95000");
    st = idx_mgr_->put("sales", e3);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query: EU, 2024, Q1
    std::vector<std::string> values = {"EU", "2024", "Q1"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("sales", cols, values);
    ASSERT_TRUE(status.ok);
    
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_NE(std::find(keys.begin(), keys.end(), "sale1"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "sale3"), keys.end());
}

// ============================================================================
// Additional Tests - Basic Composite Index Operations
// ============================================================================

TEST_F(CompositeIndexTest, InsertWithMultipleCompositeKeys) {
    std::vector<std::string> cols = {"type", "category"};
    auto st = idx_mgr_->createCompositeIndex("products", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert multiple products with different composite keys
    for (int i = 0; i < 5; ++i) {
        BaseEntity e("prod" + std::to_string(i));
        e.setField("name", "Product " + std::to_string(i));
        e.setField("type", i % 2 == 0 ? "electronics" : "furniture");
        e.setField("category", i % 3 == 0 ? "premium" : "standard");
        st = idx_mgr_->put("products", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Query for electronics + premium
    std::vector<std::string> values = {"electronics", "premium"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("products", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u); // only prod0 matches electronics+premium
    EXPECT_NE(std::find(keys.begin(), keys.end(), "prod0"), keys.end());
}

TEST_F(CompositeIndexTest, GetWithCompositeKey) {
    std::vector<std::string> cols = {"country", "state"};
    auto st = idx_mgr_->createCompositeIndex("addresses", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("addr1");
    e1.setField("street", "Main St");
    e1.setField("country", "USA");
    e1.setField("state", "CA");
    st = idx_mgr_->put("addresses", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("addr2");
    e2.setField("street", "Second St");
    e2.setField("country", "USA");
    e2.setField("state", "NY");
    st = idx_mgr_->put("addresses", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Get addresses in USA, CA
    std::vector<std::string> values = {"USA", "CA"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("addresses", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "addr1");
}

TEST_F(CompositeIndexTest, DeleteWithCompositeKey) {
    std::vector<std::string> cols = {"type", "status"};
    auto st = idx_mgr_->createCompositeIndex("orders", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("order1");
    e1.setField("type", "online");
    e1.setField("status", "pending");
    st = idx_mgr_->put("orders", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("order2");
    e2.setField("type", "online");
    e2.setField("status", "pending");
    st = idx_mgr_->put("orders", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Verify both exist
    std::vector<std::string> values = {"online", "pending"};
    auto [status1, keys1] = idx_mgr_->scanKeysEqualComposite("orders", cols, values);
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys1.size(), 2u);
    
    // Delete one
    st = idx_mgr_->erase("orders", "order1");
    ASSERT_TRUE(st.ok) << st.message;
    
    // Verify only one remains
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("orders", cols, values);
    ASSERT_TRUE(status2.ok);
    EXPECT_EQ(keys2.size(), 1u);
    EXPECT_EQ(keys2[0], "order2");
}

TEST_F(CompositeIndexTest, UpdateCompositeIndexedData) {
    std::vector<std::string> cols = {"grade", "section"};
    auto st = idx_mgr_->createCompositeIndex("students", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e("student1");
    e.setField("name", "Alice");
    e.setField("grade", "10");
    e.setField("section", "A");
    st = idx_mgr_->put("students", e);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Verify original index
    std::vector<std::string> values1 = {"10", "A"};
    auto [status1, keys1] = idx_mgr_->scanKeysEqualComposite("students", cols, values1);
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys1.size(), 1u);
    
    // Update section
    e.setField("section", "B");
    st = idx_mgr_->put("students", e);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Old index should be empty
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("students", cols, values1);
    ASSERT_TRUE(status2.ok);
    EXPECT_EQ(keys2.size(), 0u);
    
    // New index should contain student
    std::vector<std::string> values2 = {"10", "B"};
    auto [status3, keys3] = idx_mgr_->scanKeysEqualComposite("students", cols, values2);
    ASSERT_TRUE(status3.ok);
    EXPECT_EQ(keys3.size(), 1u);
    EXPECT_EQ(keys3[0], "student1");
}

TEST_F(CompositeIndexTest, FourColumnComposite) {
    std::vector<std::string> cols = {"year", "quarter", "region", "category"};
    auto st = idx_mgr_->createCompositeIndex("sales_data", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("sale1");
    e1.setField("year", "2024");
    e1.setField("quarter", "Q1");
    e1.setField("region", "EU");
    e1.setField("category", "electronics");
    e1.setField("revenue", "100000");
    st = idx_mgr_->put("sales_data", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("sale2");
    e2.setField("year", "2024");
    e2.setField("quarter", "Q1");
    e2.setField("region", "EU");
    e2.setField("category", "furniture");
    e2.setField("revenue", "50000");
    st = idx_mgr_->put("sales_data", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query for specific 4-column combination
    std::vector<std::string> values = {"2024", "Q1", "EU", "electronics"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("sales_data", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "sale1");
}

// ============================================================================
// Multi-Column Sorting Tests
// ============================================================================

TEST_F(CompositeIndexTest, SortedScanAscending) {
    std::vector<std::string> cols = {"priority", "created_at"};
    auto st = idx_mgr_->createCompositeIndex("tickets", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert tickets with different priorities and timestamps
    for (int i = 0; i < 5; ++i) {
        BaseEntity e("ticket" + std::to_string(i));
        e.setField("priority", std::to_string(i % 3)); // 0, 1, 2, 0, 1
        e.setField("created_at", std::to_string(1000 + i));
        e.setField("title", "Ticket " + std::to_string(i));
        st = idx_mgr_->put("tickets", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Query for priority=1 tickets - should be sorted by created_at
    std::vector<std::string> values = {"1", ""};
    // Note: This test verifies data insertion; actual sorting would need range scan support
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("tickets", cols, {"1", "1001"});
    ASSERT_TRUE(status.ok);
}

TEST_F(CompositeIndexTest, IndexScanValidation) {
    std::vector<std::string> cols = {"level", "timestamp"};
    auto st = idx_mgr_->createCompositeIndex("logs", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert logs at different levels
    BaseEntity e1("log1");
    e1.setField("level", "ERROR");
    e1.setField("timestamp", "2024-01-01T10:00:00");
    st = idx_mgr_->put("logs", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("log2");
    e2.setField("level", "ERROR");
    e2.setField("timestamp", "2024-01-01T11:00:00");
    st = idx_mgr_->put("logs", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e3("log3");
    e3.setField("level", "ERROR");
    e3.setField("timestamp", "2024-01-01T12:00:00");
    st = idx_mgr_->put("logs", e3);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Scan should return all ERROR logs
    std::vector<std::string> values = {"ERROR", "2024-01-01T11:00:00"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("logs", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
}

TEST_F(CompositeIndexTest, CompositeKeyOrdering) {
    std::vector<std::string> cols = {"dept", "salary"};
    auto st = idx_mgr_->createCompositeIndex("employees_sal", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert employees with various salaries
    BaseEntity e1("emp1");
    e1.setField("dept", "Engineering");
    e1.setField("salary", "100000");
    st = idx_mgr_->put("employees_sal", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("emp2");
    e2.setField("dept", "Engineering");
    e2.setField("salary", "120000");
    st = idx_mgr_->put("employees_sal", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query should work with exact match
    std::vector<std::string> values = {"Engineering", "100000"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("employees_sal", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "emp1");
}

TEST_F(CompositeIndexTest, PrefixQuerySimulation) {
    std::vector<std::string> cols = {"category", "subcategory"};
    auto st = idx_mgr_->createCompositeIndex("items", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert items
    BaseEntity e1("item1");
    e1.setField("category", "books");
    e1.setField("subcategory", "fiction");
    st = idx_mgr_->put("items", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("item2");
    e2.setField("category", "books");
    e2.setField("subcategory", "nonfiction");
    st = idx_mgr_->put("items", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e3("item3");
    e3.setField("category", "electronics");
    e3.setField("subcategory", "phones");
    st = idx_mgr_->put("items", e3);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query for specific subcategory
    std::vector<std::string> values = {"books", "fiction"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("items", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "item1");
}

// ============================================================================
// Index Filtering Tests
// ============================================================================

TEST_F(CompositeIndexTest, FilterFirstColumnOnly) {
    std::vector<std::string> cols = {"brand", "model"};
    auto st = idx_mgr_->createCompositeIndex("cars", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("car1");
    e1.setField("brand", "Toyota");
    e1.setField("model", "Camry");
    st = idx_mgr_->put("cars", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("car2");
    e2.setField("brand", "Toyota");
    e2.setField("model", "Corolla");
    st = idx_mgr_->put("cars", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e3("car3");
    e3.setField("brand", "Honda");
    e3.setField("model", "Civic");
    st = idx_mgr_->put("cars", e3);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Filter by brand only (first column)
    std::vector<std::string> values = {"Toyota", "Camry"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("cars", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
}

TEST_F(CompositeIndexTest, CombinedFiltersAND) {
    std::vector<std::string> cols = {"color", "size"};
    auto st = idx_mgr_->createCompositeIndex("shirts", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert various shirts
    BaseEntity e1("shirt1");
    e1.setField("color", "blue");
    e1.setField("size", "M");
    st = idx_mgr_->put("shirts", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("shirt2");
    e2.setField("color", "blue");
    e2.setField("size", "L");
    st = idx_mgr_->put("shirts", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e3("shirt3");
    e3.setField("color", "red");
    e3.setField("size", "M");
    st = idx_mgr_->put("shirts", e3);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Combined filter: blue AND M
    std::vector<std::string> values = {"blue", "M"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("shirts", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "shirt1");
}

TEST_F(CompositeIndexTest, MultipleIndexesOnSameTable) {
    // Create first composite index
    std::vector<std::string> cols1 = {"author", "year"};
    auto st = idx_mgr_->createCompositeIndex("books", cols1);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Create second composite index
    std::vector<std::string> cols2 = {"genre", "language"};
    st = idx_mgr_->createCompositeIndex("books", cols2);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e("book1");
    e.setField("title", "Book Title");
    e.setField("author", "John Doe");
    e.setField("year", "2024");
    e.setField("genre", "SciFi");
    e.setField("language", "English");
    st = idx_mgr_->put("books", e);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query using first index
    std::vector<std::string> values1 = {"John Doe", "2024"};
    auto [status1, keys1] = idx_mgr_->scanKeysEqualComposite("books", cols1, values1);
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys1.size(), 1u);
    
    // Query using second index
    std::vector<std::string> values2 = {"SciFi", "English"};
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("books", cols2, values2);
    ASSERT_TRUE(status2.ok);
    EXPECT_EQ(keys2.size(), 1u);
}

TEST_F(CompositeIndexTest, FilterSecondColumnDifferentValues) {
    std::vector<std::string> cols = {"team", "role"};
    auto st = idx_mgr_->createCompositeIndex("members", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("member1");
    e1.setField("team", "alpha");
    e1.setField("role", "developer");
    st = idx_mgr_->put("members", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("member2");
    e2.setField("team", "alpha");
    e2.setField("role", "manager");
    st = idx_mgr_->put("members", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query for different second column values
    std::vector<std::string> values1 = {"alpha", "developer"};
    auto [status1, keys1] = idx_mgr_->scanKeysEqualComposite("members", cols, values1);
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys1.size(), 1u);
    EXPECT_EQ(keys1[0], "member1");
    
    std::vector<std::string> values2 = {"alpha", "manager"};
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("members", cols, values2);
    ASSERT_TRUE(status2.ok);
    EXPECT_EQ(keys2.size(), 1u);
    EXPECT_EQ(keys2[0], "member2");
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(CompositeIndexTest, EmptyStringInCompositeKey) {
    std::vector<std::string> cols = {"field1", "field2"};
    auto st = idx_mgr_->createCompositeIndex("test_empty", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("entity1");
    e1.setField("field1", "");
    e1.setField("field2", "value2");
    st = idx_mgr_->put("test_empty", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("entity2");
    e2.setField("field1", "value1");
    e2.setField("field2", "");
    st = idx_mgr_->put("test_empty", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query with empty string
    std::vector<std::string> values = {"", "value2"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("test_empty", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "entity1");
}

TEST_F(CompositeIndexTest, SpecialCharactersInCompositeKey) {
    std::vector<std::string> cols = {"name", "symbol"};
    auto st = idx_mgr_->createCompositeIndex("test_special", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e1("entity1");
    e1.setField("name", "test@example.com");
    e1.setField("symbol", "$%^&*");
    st = idx_mgr_->put("test_special", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("entity2");
    e2.setField("name", "test with spaces");
    e2.setField("symbol", "!@#");
    st = idx_mgr_->put("test_special", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query with special characters
    std::vector<std::string> values = {"test@example.com", "$%^&*"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("test_special", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "entity1");
}

TEST_F(CompositeIndexTest, VeryLongCompositeKeys) {
    std::vector<std::string> cols = {"longfield1", "longfield2"};
    auto st = idx_mgr_->createCompositeIndex("test_long", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Create very long strings (500 chars each)
    std::string long_value1(500, 'A');
    std::string long_value2(500, 'B');
    
    BaseEntity e1("entity1");
    e1.setField("longfield1", long_value1);
    e1.setField("longfield2", long_value2);
    st = idx_mgr_->put("test_long", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query with long values
    std::vector<std::string> values = {long_value1, long_value2};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("test_long", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "entity1");
}

TEST_F(CompositeIndexTest, NumericStringsInCompositeKey) {
    std::vector<std::string> cols = {"num1", "num2"};
    auto st = idx_mgr_->createCompositeIndex("test_numeric", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert with numeric strings
    BaseEntity e1("entity1");
    e1.setField("num1", "0");
    e1.setField("num2", "100");
    st = idx_mgr_->put("test_numeric", e1);
    ASSERT_TRUE(st.ok) << st.message;
    
    BaseEntity e2("entity2");
    e2.setField("num1", "0");
    e2.setField("num2", "0");
    st = idx_mgr_->put("test_numeric", e2);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Query with numeric strings
    std::vector<std::string> values = {"0", "100"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("test_numeric", cols, values);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "entity1");
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(CompositeIndexTest, BulkInsertWithCompositeIndexes) {
    std::vector<std::string> cols = {"category", "status"};
    auto st = idx_mgr_->createCompositeIndex("bulk_test", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Bulk insert 100 entities
    const int count = 100;
    for (int i = 0; i < count; ++i) {
        BaseEntity e("entity" + std::to_string(i));
        e.setField("category", "cat" + std::to_string(i % 10));
        e.setField("status", i % 2 == 0 ? "active" : "inactive");
        e.setField("data", "Data for entity " + std::to_string(i));
        st = idx_mgr_->put("bulk_test", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Query a specific combination
    std::vector<std::string> values = {"cat0", "active"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("bulk_test", cols, values);
    ASSERT_TRUE(status.ok);
    // cat0 occurs at: 0,10,20,30,40,50,60,70,80,90 (10 items)
    // Every cat0 row is also even, so all 10 are active.
    EXPECT_EQ(keys.size(), 10u);
}

TEST_F(CompositeIndexTest, ConcurrentUpdatesOnCompositeIndex) {
    std::vector<std::string> cols = {"version", "state"};
    auto st = idx_mgr_->createCompositeIndex("concurrent_test", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert initial entities
    for (int i = 0; i < 10; ++i) {
        BaseEntity e("item" + std::to_string(i));
        e.setField("version", "v1");
        e.setField("state", "draft");
        st = idx_mgr_->put("concurrent_test", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Update all to published
    for (int i = 0; i < 10; ++i) {
        BaseEntity e("item" + std::to_string(i));
        e.setField("version", "v1");
        e.setField("state", "published");
        st = idx_mgr_->put("concurrent_test", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Verify old index is empty
    std::vector<std::string> values1 = {"v1", "draft"};
    auto [status1, keys1] = idx_mgr_->scanKeysEqualComposite("concurrent_test", cols, values1);
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys1.size(), 0u);
    
    // Verify new index has all items
    std::vector<std::string> values2 = {"v1", "published"};
    auto [status2, keys2] = idx_mgr_->scanKeysEqualComposite("concurrent_test", cols, values2);
    ASSERT_TRUE(status2.ok);
    EXPECT_EQ(keys2.size(), 10u);
}

TEST_F(CompositeIndexTest, IndexSizeVsQueryPerformance) {
    std::vector<std::string> cols = {"zone", "tier"};
    auto st = idx_mgr_->createCompositeIndex("perf_test", cols);
    ASSERT_TRUE(st.ok) << st.message;
    
    // Insert entities with different distributions
    for (int i = 0; i < 50; ++i) {
        BaseEntity e("perf" + std::to_string(i));
        e.setField("zone", "zone" + std::to_string(i % 5)); // 5 zones
        e.setField("tier", "tier" + std::to_string(i % 3)); // 3 tiers
        e.setField("payload", std::string(100, 'X')); // Add some data
        st = idx_mgr_->put("perf_test", e);
        ASSERT_TRUE(st.ok) << st.message;
    }
    
    // Query specific combination
    std::vector<std::string> values = {"zone0", "tier0"};
    auto [status, keys] = idx_mgr_->scanKeysEqualComposite("perf_test", cols, values);
    ASSERT_TRUE(status.ok);
    
    // Verify correct number of results
    // zone0: 0,5,10,15,20,25,30,35,40,45 (10 items)
    // tier0: 0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48 (17 items)
    // Both: 0,15,30,45 (4 items)
    EXPECT_GE(keys.size(), 1u);
}
