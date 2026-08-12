#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sstream>
// Generate a reasonably unique id per process without OS-specific headers
static inline unsigned long long make_unique_id() {
    return static_cast<unsigned long long>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );
}

#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "server/http_server.h"

// Global in-process HTTP server for integration tests using Google Test Environment
// This avoids spawning external processes and stabilizes tests.

namespace {
class ThemisServerEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        try {
            // Prepare per-process test DB directory to avoid RocksDB lock contention under parallel CTest
            std::ostringstream path_suffix;
            path_suffix << "themis_gtest_env_" << make_unique_id();
            base_path_ = std::filesystem::path("./data") / path_suffix.str();
            {
                std::error_code ec;
                if (std::filesystem::exists(base_path_)) {
                    std::filesystem::remove_all(base_path_, ec);
                }
                // Ensure directory exists
                std::filesystem::create_directories(base_path_, ec);
            }

            // RocksDB configuration
            themis::RocksDBWrapper::Config cfg;
            cfg.db_path = base_path_.string();
            cfg.memtable_size_mb = 64;
            cfg.block_cache_size_mb = 256;
            cfg.enable_wal = true;

            storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
            if (!storage_->open()) {
                // Try again after a short delay in case of lingering locks
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!storage_->open()) {
                    throw std::runtime_error("Failed to open RocksDB for test env");
                }
            }

            secondary_index_ = std::make_shared<themis::SecondaryIndexManager>(*storage_);
            graph_index_ = std::make_shared<themis::GraphIndexManager>(*storage_);
            vector_index_ = std::make_shared<themis::VectorIndexManager>(*storage_);
            tx_manager_ = std::make_shared<themis::TransactionManager>(
                *storage_, *secondary_index_, *graph_index_, *vector_index_
            );

            // HTTP server on available test port (retry range)
            themis::server::HttpServer::Config scfg;
            scfg.host = "127.0.0.1";
            scfg.port = 8765;
            scfg.num_threads = 2;
            // Enable features as safe defaults for integration tests
            scfg.feature_semantic_cache = false;
            scfg.feature_llm_store = false;
            scfg.feature_cdc = false;
            scfg.feature_timeseries = false; // timeseries tests bring their own server

            int attempts = 0;
            const int maxAttempts = 16;
            while (attempts < maxAttempts) {
                try {
                    server_ = std::make_unique<themis::server::HttpServer>(
                        scfg, storage_, secondary_index_, graph_index_, vector_index_, tx_manager_
                    );
                    server_->start();
                    break; // started successfully
                } catch (const std::exception&) {
                    // try next port
                    ++attempts;
                    ++scfg.port;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
            if (!server_) {
                throw std::runtime_error("Failed to start HTTP server for test env (ports in use) ");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } catch (const std::exception& e) {
            // Non-fatal: log a warning so tests can proceed; some tests may not require the HTTP server or persistent RocksDB.
            GTEST_LOG_(WARNING) << "ThemisServerEnvironment setup warning: " << e.what();
        }
    }

    void TearDown() override {
        try {
            if (server_) server_->stop();
            if (storage_) storage_->close();
            // Cleanup test DB directory
            if (!base_path_.empty() && std::filesystem::exists(base_path_)) {
                std::error_code ec; std::filesystem::remove_all(base_path_, ec);
            }
        } catch (...) {
            // best effort
        }
        server_.reset();
        tx_manager_.reset();
        vector_index_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        storage_.reset();
    }

private:
    std::filesystem::path base_path_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index_;
    std::shared_ptr<themis::GraphIndexManager> graph_index_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::TransactionManager> tx_manager_;
    std::unique_ptr<themis::server::HttpServer> server_;
};

// Register environment via static initializer (works with gtest_main)
struct EnvRegistrar {
    EnvRegistrar() {
        ::testing::AddGlobalTestEnvironment(new ThemisServerEnvironment());
    }
} g_env_registrar;

} // namespace
