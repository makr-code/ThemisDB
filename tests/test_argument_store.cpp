#include <gtest/gtest.h>
#include "ethics_ai/argument_store.h"
#include "storage/rocksdb_wrapper.h"
#include "query/query_engine.h"
#include <thread>
#include <chrono>

using namespace themis::plugins::ethics;

class ArgumentStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize RocksDB wrapper
        themis::RocksDBWrapper::Config db_config;
        db_config.db_path = "/tmp/test_argument_store";
        db_wrapper_ = std::make_shared<themis::RocksDBWrapper>(db_config);
        if (!db_wrapper_->open()) {
            GTEST_SKIP() << "Failed to open database";
        }
        
        store_ = std::make_unique<ArgumentStore>();
        // Initialize with optional query engine (nullptr = standalone mode)
        auto status = store_->initialize(db_wrapper_, nullptr);
        if (!status.isOK()) {
            GTEST_SKIP() << "Failed to initialize ArgumentStore: " << status.message;
        }
    }
    
    void TearDown() override {
        if (store_) {
            store_->shutdown();
        }
        if (db_wrapper_) {
            db_wrapper_->close();
        }
    }
    
    EthicalArgument createTestArgument(const std::string& id, const std::string& school) {
        EthicalArgument arg;
        arg.id = id;
        arg.philosophy_school = school;
        arg.argument_type = ArgumentType::PRO;
        arg.content = "Test argument content for " + school;
        arg.principle_basis = {"principle1", "principle2"};
        arg.strength = ArgumentStrength::MODERATE;
        return arg;
    }
    
    std::shared_ptr<themis::RocksDBWrapper> db_wrapper_;
    std::unique_ptr<ArgumentStore> store_;
};

TEST_F(ArgumentStoreTest, StoreAndRetrieveArgument) {
    auto arg = createTestArgument("arg_001", "kant");
    
    auto status = store_->storeArgument(arg, false);
    ASSERT_TRUE(status.isOK()) << status.message;
    
    auto result = store_->getArgument("arg_001");
    ASSERT_TRUE(std::holds_alternative<EthicalArgument>(result));
    
    auto retrieved = std::get<EthicalArgument>(result);
    EXPECT_EQ("arg_001", retrieved.id);
    EXPECT_EQ("kant", retrieved.philosophy_school);
    EXPECT_EQ(ArgumentType::PRO, retrieved.argument_type);
}

TEST_F(ArgumentStoreTest, GetNonExistentArgument) {
    auto result = store_->getArgument("nonexistent");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    
    auto status = std::get<Status>(result);
    EXPECT_FALSE(status.isOK());
    EXPECT_NE(status.message.find("not found"), std::string::npos);
}

TEST_F(ArgumentStoreTest, StoreArgumentWithEmptyId) {
    EthicalArgument arg;
    arg.id = "";
    arg.philosophy_school = "kant";
    
    auto status = store_->storeArgument(arg, false);
    EXPECT_FALSE(status.isOK());
    EXPECT_NE(status.message.find("cannot be empty"), std::string::npos);
}

TEST_F(ArgumentStoreTest, GetArgumentsByPhilosophy) {
    // Store multiple arguments
    store_->storeArgument(createTestArgument("kant_1", "kant"), false);
    store_->storeArgument(createTestArgument("kant_2", "kant"), false);
    store_->storeArgument(createTestArgument("util_1", "utilitarianism"), false);
    
    // Retrieve Kant arguments
    auto result = store_->getArgumentsByPhilosophy("kant", {}, 10);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    
    auto args = std::get<std::vector<EthicalArgument>>(result);
    EXPECT_EQ(2u, args.size());
    
    for (const auto& arg : args) {
        EXPECT_EQ("kant", arg.philosophy_school);
    }
}

TEST_F(ArgumentStoreTest, GetArgumentsByPhilosophyWithTypeFilter) {
    auto pro_arg = createTestArgument("kant_pro", "kant");
    pro_arg.argument_type = ArgumentType::PRO;
    store_->storeArgument(pro_arg, false);
    
    auto contra_arg = createTestArgument("kant_contra", "kant");
    contra_arg.argument_type = ArgumentType::CONTRA;
    store_->storeArgument(contra_arg, false);
    
    auto rebuttal_arg = createTestArgument("kant_rebuttal", "kant");
    rebuttal_arg.argument_type = ArgumentType::REBUTTAL;
    store_->storeArgument(rebuttal_arg, false);
    
    // Filter for PRO and CONTRA only
    std::vector<ArgumentType> filter = {ArgumentType::PRO, ArgumentType::CONTRA};
    auto result = store_->getArgumentsByPhilosophy("kant", filter, 10);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    
    auto args = std::get<std::vector<EthicalArgument>>(result);
    EXPECT_EQ(2u, args.size());
}

TEST_F(ArgumentStoreTest, GetArgumentsByPhilosophyWithLimit) {
    // Store 5 arguments
    for (int i = 0; i < 5; i++) {
        store_->storeArgument(createTestArgument("kant_" + std::to_string(i), "kant"), false);
    }
    
    // Retrieve with limit of 3
    auto result = store_->getArgumentsByPhilosophy("kant", {}, 3);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    
    auto args = std::get<std::vector<EthicalArgument>>(result);
    EXPECT_EQ(3u, args.size());
}

// Note: ArgumentChain storage not yet implemented in ArgumentStore API
// TEST_F(ArgumentStoreTest, StoreAndRetrieveChain) {
//    ...
// }

TEST_F(ArgumentStoreTest, StoreAndRetrieveDecision) {
    EthicalDecision decision;
    decision.decision_id = "dec_001";
    decision.dilemma_id = "dilemma_001";
    decision.decision_text = "Test decision";
    decision.primary_philosophy = "kant";
    decision.supporting_philosophies = {"kant", "utilitarianism"};
    decision.confidence = 0.75;
    decision.consensus_level = 0.80;
    
    auto status = store_->storeDecision(decision);
    ASSERT_TRUE(status.isOK()) << status.message;
    
    auto result = store_->getDecision("dec_001");
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));
    
    auto retrieved = std::get<EthicalDecision>(result);
    EXPECT_EQ("dec_001", retrieved.decision_id);
    EXPECT_EQ("Test decision", retrieved.decision_text);
    EXPECT_EQ("kant", retrieved.primary_philosophy);
    EXPECT_DOUBLE_EQ(0.75, retrieved.confidence);
}

TEST_F(ArgumentStoreTest, ThreadSafety) {
    const int num_threads = 10;
    const int args_per_thread = 10;
    std::vector<std::thread> threads;
    
    // Launch threads that store arguments concurrently
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, args_per_thread]() {
            for (int i = 0; i < args_per_thread; i++) {
                std::string id = "thread_" + std::to_string(t) + "_arg_" + std::to_string(i);
                auto arg = createTestArgument(id, "kant");
                store_->storeArgument(arg, false);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all arguments were stored
    auto result = store_->getArgumentsByPhilosophy("kant", {}, 1000);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    
    auto args = std::get<std::vector<EthicalArgument>>(result);
    EXPECT_EQ(num_threads * args_per_thread, args.size());
}

TEST_F(ArgumentStoreTest, ShutdownClearsData) {
    store_->storeArgument(createTestArgument("arg_001", "kant"), false);
    
    auto result1 = store_->getArgument("arg_001");
    ASSERT_TRUE(std::holds_alternative<EthicalArgument>(result1));
    
    store_->shutdown();
    
    // After shutdown, store should not be usable
    auto result2 = store_->getArgument("arg_001");
    ASSERT_TRUE(std::holds_alternative<Status>(result2));
    EXPECT_FALSE(std::get<Status>(result2).isOK());
}