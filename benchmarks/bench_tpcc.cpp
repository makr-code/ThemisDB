#if 1
#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <optional>
#include <random>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>

/**
 * TPC-C Lite Benchmark for ThemisDB
 *
 * Based on TPC-C Specification 5.11
 * http://www.tpc.org/tpcc/
 *
 * Implements the 5 TPC-C transactions:
 * 1. New Order (45%) - Most critical, creates new customer order
 * 2. Payment (43%) - Updates customer and warehouse statistics
 * 3. Order Status (4%) - Read-only query of order status
 * 4. Delivery (4%) - Batch processing of orders
 * 5. Stock Level (4%) - Inventory query
 *
 * Dataset / Warmup parameters:
 * - range(0): number of warehouses  (default Arg: 1)
 * - range(1): customers per district (default Arg: 3000; TPC-C spec §4.3.3)
 * - Warmup: data is loaded in SetUp(); measurement starts after full load.
 * - Items table: ITEMS_COUNT = 100,000 (shared across warehouses, per spec)
 *
 * Performance targets (8-core, 32GB RAM, NVMe):
 * - PostgreSQL baseline: ~200,000 tpmC
 * - ThemisDB target: 150,000-200,000 tpmC (80-100% of PostgreSQL)
 *
 * CI artifacts: exported via --benchmark_out=<file> --benchmark_out_format=json
 * Baseline reference: artifacts/perf_nv/targeted_validation/bench_tpcc_targeted_v2.json
 */

namespace {
    // TPC-C Constants
    constexpr int ITEMS_COUNT = 100000;
    constexpr int DISTRICTS_PER_WAREHOUSE = 10;
    constexpr int CUSTOMERS_PER_DISTRICT = 3000;
    
    // Random generators
    std::mt19937 rng{std::random_device{}()};
    
    // NURand function from TPC-C spec (4.3.2.3)
    int NURand(int a, int x, int y) {
        static const int c_load = 0; // For data generation
        std::uniform_int_distribution<int> dist1(0, a);
        std::uniform_int_distribution<int> dist2(x, y);
        return (((dist1(rng) | dist2(rng)) + c_load) % (y - x + 1)) + x;
    }
    
    std::string makeRandomString(size_t min_len, size_t max_len) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<size_t> char_dist(0, sizeof(charset) - 2);
        
        size_t len = len_dist(rng);
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            s += charset[char_dist(rng)];
        }
        return s;
    }
    
    std::string makeRandomNumberString(size_t len) {
        static const char digits[] = "0123456789";
        std::uniform_int_distribution<size_t> dist(0, 9);
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            s += digits[dist(rng)];
        }
        return s;
    }
    
    std::string makeZipCode() {
        return makeRandomNumberString(4) + "11111";
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    // Generate customer last name using syllables (TPC-C 4.3.2.3)
    std::string makeLastName(int num) {
        static const std::vector<std::string> syllables = {
            "BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE", "ANTI", "CALLY", "ATION", "EING"
        };
        int n1 = (num / 100) % 10;
        int n2 = (num / 10) % 10;
        int n3 = num % 10;
        return syllables[n1] + syllables[n2] + syllables[n3];
    }
    
    void cleanupTestDB(const std::string& path) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
}

/**
 * TPC-C Lite Benchmark Fixture
 *
 * Sets up the database with TPC-C schema:
 * - WAREHOUSE: Warehouse information
 * - DISTRICT: Sales districts (10 per warehouse)
 * - CUSTOMER: Customer records (customers_per_district_ per district)
 * - HISTORY: Payment history
 * - NEW_ORDER: Pending orders
 * - ORDERS: Order headers
 * - ORDER_LINE: Order line items
 * - ITEM: Item catalog (100,000 items)
 * - STOCK: Inventory (100,000 per warehouse)
 *
 * Benchmark parameters:
 *   state.range(0) = num_warehouses      (e.g. 1)
 *   state.range(1) = customers_per_dist  (e.g. 3000 = TPC-C spec full scale)
 *                    If only one Arg is supplied, defaults to 100 (fast CI mode).
 */
class TPCCLiteFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = std::string("tmp/bench_tpcc_") +
                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        cleanupTestDB(db_path_);
        
        // Configure database for TPC-C workload
        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        config.block_cache_size_mb = 512; // Larger cache for TPC-C
        config.db_write_buffer_size_mb = 64;
        config.max_write_buffer_number = 3;
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        
        // Number of warehouses from benchmark parameter (default: 1)
        num_warehouses_ = state.range(0);
        // Customers per district: range(1); 100 = fast CI mode, 3000 = full TPC-C spec
        customers_per_district_ = static_cast<int>(state.range(1));
        
        // Create indexes for TPC-C tables
        createTPCCIndexes();
        
        // Load initial data
        loadTPCCData();
    }
    
    void TearDown(const ::benchmark::State&) override {
        secondary_.reset();
        db_.reset();
        cleanupTestDB(db_path_);
    }
    
protected:
    void createTPCCIndexes() {
        // WAREHOUSE indexes
        secondary_->createIndex("WAREHOUSE", "W_ID", true);
        
        // DISTRICT indexes
        secondary_->createIndex("DISTRICT", "D_W_ID_D_ID", true); // Composite key
        secondary_->createIndex("DISTRICT", "D_W_ID", false);
        
        // CUSTOMER indexes
        secondary_->createIndex("CUSTOMER", "C_W_ID_C_D_ID_C_ID", true); // Composite key
        secondary_->createIndex("CUSTOMER", "C_W_ID_C_D_ID_C_LAST", false); // For lookup by name
        
        // ORDERS indexes
        secondary_->createIndex("ORDERS", "O_W_ID_O_D_ID_O_ID", true);
        secondary_->createIndex("ORDERS", "O_W_ID_O_D_ID_O_C_ID", false);
        
        // NEW_ORDER indexes
        secondary_->createIndex("NEW_ORDER", "NO_W_ID_NO_D_ID_NO_O_ID", true);
        
        // ORDER_LINE indexes
        secondary_->createIndex("ORDER_LINE", "OL_W_ID_OL_D_ID_OL_O_ID_OL_NUMBER", true);
        
        // ITEM indexes
        secondary_->createIndex("ITEM", "I_ID", true);
        
        // STOCK indexes
        secondary_->createIndex("STOCK", "S_W_ID_S_I_ID", true);
    }
    
    void loadTPCCData() {
        // Load ITEM table (shared across all warehouses)
        loadItems();
        
        // Load per-warehouse data
        for (int w_id = 1; w_id <= num_warehouses_; ++w_id) {
            loadWarehouse(w_id);
            loadStock(w_id);
            
            for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; ++d_id) {
                loadDistrict(w_id, d_id);
                loadCustomers(w_id, d_id);
                loadOrders(w_id, d_id);
            }
        }
    }
    
    void loadItems() {
        for (int i_id = 1; i_id <= ITEMS_COUNT; ++i_id) {
            themis::BaseEntity item("item_" + std::to_string(i_id));
            item.setField("I_ID", static_cast<int64_t>(i_id));
            item.setField("I_IM_ID", static_cast<int64_t>(std::uniform_int_distribution<int>(1, 10000)(rng)));
            item.setField("I_NAME", makeRandomString(14, 24));
            item.setField("I_PRICE", static_cast<double>(std::uniform_int_distribution<int>(100, 10000)(rng)) / 100.0);
            item.setField("I_DATA", makeRandomString(26, 50));
            secondary_->put("ITEM", item);
        }
    }
    
    void loadWarehouse(int w_id) {
        themis::BaseEntity warehouse("warehouse_" + std::to_string(w_id));
        warehouse.setField("W_ID", static_cast<int64_t>(w_id));
        warehouse.setField("W_NAME", makeRandomString(6, 10));
        warehouse.setField("W_STREET_1", makeRandomString(10, 20));
        warehouse.setField("W_STREET_2", makeRandomString(10, 20));
        warehouse.setField("W_CITY", makeRandomString(10, 20));
        warehouse.setField("W_STATE", makeRandomString(2, 2));
        warehouse.setField("W_ZIP", makeZipCode());
        warehouse.setField("W_TAX", static_cast<double>(std::uniform_int_distribution<int>(0, 2000)(rng)) / 10000.0);
        warehouse.setField("W_YTD", 300000.0);
        secondary_->put("WAREHOUSE", warehouse);
    }
    
    void loadStock(int w_id) {
        for (int i_id = 1; i_id <= ITEMS_COUNT; ++i_id) {
            themis::BaseEntity stock("stock_" + std::to_string(w_id) + "_" + std::to_string(i_id));
            stock.setField("S_I_ID", static_cast<int64_t>(i_id));
            stock.setField("S_W_ID", static_cast<int64_t>(w_id));
            stock.setField("S_QUANTITY", static_cast<int64_t>(std::uniform_int_distribution<int>(10, 100)(rng)));
            stock.setField("S_YTD", static_cast<int64_t>(0));
            stock.setField("S_ORDER_CNT", static_cast<int64_t>(0));
            stock.setField("S_REMOTE_CNT", static_cast<int64_t>(0));
            stock.setField("S_DATA", makeRandomString(26, 50));
            // Simplified: Not storing all S_DIST_XX fields for this benchmark
            secondary_->put("STOCK", stock);
        }
    }
    
    void loadDistrict(int w_id, int d_id) {
        themis::BaseEntity district("district_" + std::to_string(w_id) + "_" + std::to_string(d_id));
        district.setField("D_ID", static_cast<int64_t>(d_id));
        district.setField("D_W_ID", static_cast<int64_t>(w_id));
        district.setField("D_NAME", makeRandomString(6, 10));
        district.setField("D_STREET_1", makeRandomString(10, 20));
        district.setField("D_STREET_2", makeRandomString(10, 20));
        district.setField("D_CITY", makeRandomString(10, 20));
        district.setField("D_STATE", makeRandomString(2, 2));
        district.setField("D_ZIP", makeZipCode());
        district.setField("D_TAX", static_cast<double>(std::uniform_int_distribution<int>(0, 2000)(rng)) / 10000.0);
        district.setField("D_YTD", 30000.0);
        district.setField("D_NEXT_O_ID", static_cast<int64_t>(3001));
        secondary_->put("DISTRICT", district);
    }
    
    void loadCustomers(int w_id, int d_id) {
        for (int c_id = 1; c_id <= customers_per_district_; ++c_id) {
            themis::BaseEntity customer("customer_" + std::to_string(w_id) + "_" + 
                                       std::to_string(d_id) + "_" + std::to_string(c_id));
            customer.setField("C_ID", static_cast<int64_t>(c_id));
            customer.setField("C_D_ID", static_cast<int64_t>(d_id));
            customer.setField("C_W_ID", static_cast<int64_t>(w_id));
            customer.setField("C_FIRST", makeRandomString(8, 16));
            customer.setField("C_MIDDLE", std::string("OE"));
            customer.setField("C_LAST", makeLastName(c_id <= 1000 ? c_id - 1 : NURand(255, 0, 999)));
            customer.setField("C_STREET_1", makeRandomString(10, 20));
            customer.setField("C_STREET_2", makeRandomString(10, 20));
            customer.setField("C_CITY", makeRandomString(10, 20));
            customer.setField("C_STATE", makeRandomString(2, 2));
            customer.setField("C_ZIP", makeZipCode());
            customer.setField("C_PHONE", makeRandomNumberString(16));
            customer.setField("C_SINCE", getCurrentTimestamp());
            customer.setField("C_CREDIT", std::uniform_real_distribution<double>(0, 1)(rng) < 0.1 ? "BC" : "GC");
            customer.setField("C_CREDIT_LIM", 50000.0);
            customer.setField("C_DISCOUNT", static_cast<double>(std::uniform_int_distribution<int>(0, 5000)(rng)) / 10000.0);
            customer.setField("C_BALANCE", -10.0);
            customer.setField("C_YTD_PAYMENT", 10.0);
            customer.setField("C_PAYMENT_CNT", static_cast<int64_t>(1));
            customer.setField("C_DELIVERY_CNT", static_cast<int64_t>(0));
            customer.setField("C_DATA", makeRandomString(300, 500));
            secondary_->put("CUSTOMER", customer);
        }
    }
    
    void loadOrders(int w_id, int d_id) {
        // Load orders proportional to customer count (TPC-C: same count as customers)
        int orders_to_load = std::min(customers_per_district_, 3000);
        int new_order_threshold = static_cast<int>(orders_to_load * 0.9); // last 10% go to NEW_ORDER
        for (int o_id = 1; o_id <= orders_to_load; ++o_id) {
            int c_id = NURand(1023, 1, customers_per_district_);
            
            themis::BaseEntity order("order_" + std::to_string(w_id) + "_" + 
                                    std::to_string(d_id) + "_" + std::to_string(o_id));
            order.setField("O_ID", static_cast<int64_t>(o_id));
            order.setField("O_D_ID", static_cast<int64_t>(d_id));
            order.setField("O_W_ID", static_cast<int64_t>(w_id));
            order.setField("O_C_ID", static_cast<int64_t>(c_id));
            order.setField("O_ENTRY_D", getCurrentTimestamp());
            order.setField("O_CARRIER_ID", o_id > new_order_threshold ? static_cast<int64_t>(0) : static_cast<int64_t>(std::uniform_int_distribution<int>(1, 10)(rng)));
            order.setField("O_OL_CNT", static_cast<int64_t>(std::uniform_int_distribution<int>(5, 15)(rng)));
            order.setField("O_ALL_LOCAL", static_cast<int64_t>(1));
            secondary_->put("ORDERS", order);
            
            // Add to NEW_ORDER if recent (last 10% per TPC-C spec §4.3.3.1)
            if (o_id > new_order_threshold) {
                themis::BaseEntity new_order("new_order_" + std::to_string(w_id) + "_" + 
                                            std::to_string(d_id) + "_" + std::to_string(o_id));
                new_order.setField("NO_O_ID", static_cast<int64_t>(o_id));
                new_order.setField("NO_D_ID", static_cast<int64_t>(d_id));
                new_order.setField("NO_W_ID", static_cast<int64_t>(w_id));
                secondary_->put("NEW_ORDER", new_order);
            }
        }
    }

    std::optional<themis::BaseEntity> loadEntity(const std::string& table, const std::string& pk) {
        const auto entity_key = themis::KeySchema::makeRelationalKey(table, pk);
        auto blob = db_->get(entity_key);
        if (!blob) {
            return std::nullopt;
        }
        return themis::BaseEntity::deserialize(pk, *blob);
    }
    
    // -------------------------------------------------------------------------
    // Transaction helpers – called directly by BENCHMARK_DEFINE_F bodies and
    // MixedWorkload.  Each helper performs exactly one TPC-C transaction cycle.
    // -------------------------------------------------------------------------

    void runNewOrder() {
        int w_id = 1;
        int d_id = std::uniform_int_distribution<int>(1, DISTRICTS_PER_WAREHOUSE)(rng);
        int c_id = NURand(1023, 1, customers_per_district_);
        int ol_cnt = std::uniform_int_distribution<int>(5, 15)(rng);

        std::string district_key = "district_" + std::to_string(w_id) + "_" + std::to_string(d_id);
        auto district_opt = loadEntity("DISTRICT", district_key);

        if (district_opt) {
            int64_t o_id = district_opt->getFieldAsInt("D_NEXT_O_ID").value_or(3001);
            district_opt->setField("D_NEXT_O_ID", o_id + 1);
            secondary_->put("DISTRICT", *district_opt);

            themis::BaseEntity new_order("new_order_" + std::to_string(w_id) + "_" +
                                        std::to_string(d_id) + "_" + std::to_string(o_id));
            new_order.setField("NO_O_ID", o_id);
            new_order.setField("NO_D_ID", static_cast<int64_t>(d_id));
            new_order.setField("NO_W_ID", static_cast<int64_t>(w_id));
            secondary_->put("NEW_ORDER", new_order);

            themis::BaseEntity order("order_" + std::to_string(w_id) + "_" +
                                    std::to_string(d_id) + "_" + std::to_string(o_id));
            order.setField("O_ID", o_id);
            order.setField("O_D_ID", static_cast<int64_t>(d_id));
            order.setField("O_W_ID", static_cast<int64_t>(w_id));
            order.setField("O_C_ID", static_cast<int64_t>(c_id));
            order.setField("O_ENTRY_D", getCurrentTimestamp());
            order.setField("O_CARRIER_ID", static_cast<int64_t>(0));
            order.setField("O_OL_CNT", static_cast<int64_t>(ol_cnt));
            order.setField("O_ALL_LOCAL", static_cast<int64_t>(1));
            secondary_->put("ORDERS", order);

            for (int ol_number = 1; ol_number <= ol_cnt; ++ol_number) {
                int i_id = NURand(8191, 1, ITEMS_COUNT);
                std::string stock_key = "stock_" + std::to_string(w_id) + "_" + std::to_string(i_id);
                auto stock_opt = loadEntity("STOCK", stock_key);
                if (stock_opt) {
                    int64_t quantity = stock_opt->getFieldAsInt("S_QUANTITY").value_or(50);
                    quantity -= 5;
                    if (quantity < 10) quantity += 91;
                    stock_opt->setField("S_QUANTITY", quantity);
                    secondary_->put("STOCK", *stock_opt);
                }
                benchmark::DoNotOptimize(ol_number);
            }
        }
    }

    void runPayment() {
        int w_id = 1;
        int d_id = std::uniform_int_distribution<int>(1, DISTRICTS_PER_WAREHOUSE)(rng);
        int c_id = NURand(1023, 1, customers_per_district_);
        double h_amount = 500.0;

        std::string warehouse_key = "warehouse_" + std::to_string(w_id);
        auto warehouse_opt = loadEntity("WAREHOUSE", warehouse_key);
        if (warehouse_opt) {
            double ytd = warehouse_opt->getFieldAsDouble("W_YTD").value_or(0.0);
            warehouse_opt->setField("W_YTD", ytd + h_amount);
            secondary_->put("WAREHOUSE", *warehouse_opt);
        }

        std::string district_key = "district_" + std::to_string(w_id) + "_" + std::to_string(d_id);
        auto district_opt = loadEntity("DISTRICT", district_key);
        if (district_opt) {
            double ytd = district_opt->getFieldAsDouble("D_YTD").value_or(0.0);
            district_opt->setField("D_YTD", ytd + h_amount);
            secondary_->put("DISTRICT", *district_opt);
        }

        std::string customer_key = "customer_" + std::to_string(w_id) + "_" +
                                   std::to_string(d_id) + "_" + std::to_string(c_id);
        auto customer_opt = loadEntity("CUSTOMER", customer_key);
        if (customer_opt) {
            double balance = customer_opt->getFieldAsDouble("C_BALANCE").value_or(0.0);
            double ytd_payment = customer_opt->getFieldAsDouble("C_YTD_PAYMENT").value_or(0.0);
            int64_t payment_cnt = customer_opt->getFieldAsInt("C_PAYMENT_CNT").value_or(0);
            customer_opt->setField("C_BALANCE", balance - h_amount);
            customer_opt->setField("C_YTD_PAYMENT", ytd_payment + h_amount);
            customer_opt->setField("C_PAYMENT_CNT", payment_cnt + 1);
            secondary_->put("CUSTOMER", *customer_opt);
        }

        themis::BaseEntity history("history_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()));
        history.setField("H_C_ID", static_cast<int64_t>(c_id));
        history.setField("H_C_D_ID", static_cast<int64_t>(d_id));
        history.setField("H_C_W_ID", static_cast<int64_t>(w_id));
        history.setField("H_D_ID", static_cast<int64_t>(d_id));
        history.setField("H_W_ID", static_cast<int64_t>(w_id));
        history.setField("H_DATE", getCurrentTimestamp());
        history.setField("H_AMOUNT", h_amount);
        secondary_->put("HISTORY", history);
    }

    void runOrderStatus() {
        int w_id = 1;
        int d_id = std::uniform_int_distribution<int>(1, DISTRICTS_PER_WAREHOUSE)(rng);
        int c_id = NURand(1023, 1, customers_per_district_);

        std::string customer_key = "customer_" + std::to_string(w_id) + "_" +
                                   std::to_string(d_id) + "_" + std::to_string(c_id);
        auto customer_opt = loadEntity("CUSTOMER", customer_key);
        benchmark::DoNotOptimize(customer_opt);

        int orders_loaded = std::min(customers_per_district_, 3000);
        for (int o_id = orders_loaded; o_id >= 1; --o_id) {
            std::string order_key = "order_" + std::to_string(w_id) + "_" +
                                   std::to_string(d_id) + "_" + std::to_string(o_id);
            auto order_opt = loadEntity("ORDERS", order_key);
            if (order_opt) {
                int64_t order_c_id = order_opt->getFieldAsInt("O_C_ID").value_or(0);
                if (order_c_id == c_id) {
                    benchmark::DoNotOptimize(order_opt);
                    break;
                }
            }
        }
    }

    void runStockLevel() {
        int threshold = 15;
        int w_id = 1;
        int low_stock_count = 0;
        for (int i_id = 1; i_id <= 100; ++i_id) {
            std::string stock_key = "stock_" + std::to_string(w_id) + "_" + std::to_string(i_id);
            auto stock_opt = loadEntity("STOCK", stock_key);
            if (stock_opt) {
                int64_t quantity = stock_opt->getFieldAsInt("S_QUANTITY").value_or(50);
                if (quantity < threshold) {
                    ++low_stock_count;
                }
            }
        }
        benchmark::DoNotOptimize(low_stock_count);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
    int num_warehouses_;
    int customers_per_district_{100};
};

/**
 * TPC-C New Order Transaction
 *
 * The most critical transaction (45% of workload mix).
 * Creates a new customer order with 5-15 order lines.
 *
 * Performance target: < 5 s mean response time (TPC-C §5.4.1)
 */
BENCHMARK_DEFINE_F(TPCCLiteFixture, NewOrderTransaction)(benchmark::State& state) {
    for (auto _ : state) {
        runNewOrder();
    }
    state.SetItemsProcessed(state.iterations());
}

/**
 * TPC-C Payment Transaction
 *
 * Second most frequent transaction (43% of workload mix).
 * Updates customer payment and warehouse/district year-to-date statistics.
 */
BENCHMARK_DEFINE_F(TPCCLiteFixture, PaymentTransaction)(benchmark::State& state) {
    for (auto _ : state) {
        runPayment();
    }
    state.SetItemsProcessed(state.iterations());
}

/**
 * TPC-C Order Status Transaction
 *
 * Read-only query (4% of workload mix).
 * Retrieves order status for a customer.
 */
BENCHMARK_DEFINE_F(TPCCLiteFixture, OrderStatusTransaction)(benchmark::State& state) {
    for (auto _ : state) {
        runOrderStatus();
    }
    state.SetItemsProcessed(state.iterations());
}

/**
 * TPC-C Stock Level Transaction
 *
 * Inventory query (4% of workload mix).
 * Counts items with stock below threshold.
 */
BENCHMARK_DEFINE_F(TPCCLiteFixture, StockLevelTransaction)(benchmark::State& state) {
    for (auto _ : state) {
        runStockLevel();
    }
    state.SetItemsProcessed(state.iterations());
}

/**
 * TPC-C New Order Lite
 *
 * Canonical CI benchmark: warehouses=range(0), customers_per_district=range(1).
 * Matches the reference entry "TPCCLiteFixture/NewOrderLite/1/3000" in
 * PERFORMANCE_EXPECTATIONS.md §5.8.
 *
 * Args: {warehouses, customers_per_district}
 *   -> Args({1, 3000}) produces name "TPCCLiteFixture/NewOrderLite/1/3000"
 */
BENCHMARK_DEFINE_F(TPCCLiteFixture, NewOrderLite)(benchmark::State& state) {
    for (auto _ : state) {
        runNewOrder();
    }
    state.SetItemsProcessed(state.iterations() * state.range(1));
}

/**
 * TPC-C Mixed Workload
 *
 * Approximates the canonical TPC-C transaction mix:
 *   45% New Order, 43% Payment, 4% Order Status, 4% Stock Level, 4% Delivery.
 * (Delivery is elided; its slot is covered by an additional Payment.)
 */
BENCHMARK_DEFINE_F(TPCCLiteFixture, MixedWorkload)(benchmark::State& state) {
    std::uniform_int_distribution<int> mix_dist(1, 100);

    for (auto _ : state) {
        int t = mix_dist(rng);
        if (t <= 45) {
            runNewOrder();
        } else if (t <= 88) {
            runPayment();
        } else if (t <= 92) {
            runOrderStatus();
        } else {
            runStockLevel();
        }
    }
    state.SetItemsProcessed(state.iterations());
}

// ─────────────────────────── Registration ────────────────────────────────────
// Args({warehouses, customers_per_district})
//   {1, 100}  = fast CI mode (100 customers/district)
//   {1, 3000} = full TPC-C spec scale (3000 customers/district, §4.3.3)

BENCHMARK_REGISTER_F(TPCCLiteFixture, NewOrderTransaction)->Args({1, 100})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(TPCCLiteFixture, PaymentTransaction)->Args({1, 100})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(TPCCLiteFixture, OrderStatusTransaction)->Args({1, 100})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(TPCCLiteFixture, StockLevelTransaction)->Args({1, 100})->Unit(benchmark::kMillisecond);

// Canonical CI reference point (matches PERFORMANCE_EXPECTATIONS.md §5.8):
BENCHMARK_REGISTER_F(TPCCLiteFixture, NewOrderLite)->Args({1, 3000})->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(TPCCLiteFixture, MixedWorkload)->Args({1, 100})->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
#endif

