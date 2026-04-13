/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_data_generator.h                              ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:33:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     116                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_data_generator.h
 * @brief Utilities for generating test data
 * 
 * Provides reusable test data generation for integration tests.
 */

#pragma once

#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace test {

/**
 * @brief Generates test data for integration tests
 */
class TestDataGenerator {
public:
    TestDataGenerator() : gen_(std::random_device{}()) {}
    
    /**
     * @brief Generate random string of specified length
     */
    std::string GenerateRandomString(size_t length) {
        static const char charset[] = 
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
        
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += charset[dist(gen_)];
        }
        return result;
    }
    
    /**
     * @brief Generate random integer in range [min, max]
     */
    int GenerateRandomInt(int min, int max) {
        std::uniform_int_distribution<> dist(min, max);
        return dist(gen_);
    }
    
    /**
     * @brief Generate test JSON document
     */
    nlohmann::json GenerateTestDocument(const std::string& id_prefix = "doc") {
        return {
            {"id", id_prefix + "_" + std::to_string(GenerateRandomInt(1000, 9999))},
            {"title", "Test Document " + GenerateRandomString(10)},
            {"content", GenerateRandomString(100)},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"value", GenerateRandomInt(1, 1000)}
        };
    }
    
    /**
     * @brief Generate multiple test documents
     */
    std::vector<nlohmann::json> GenerateTestDocuments(size_t count, const std::string& id_prefix = "doc") {
        std::vector<nlohmann::json> docs;
        docs.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            docs.push_back(GenerateTestDocument(id_prefix));
        }
        return docs;
    }
    
    /**
     * @brief Generate test encryption key
     */
    std::vector<uint8_t> GenerateEncryptionKey(size_t key_size = 32) {
        std::vector<uint8_t> key(key_size);
        std::uniform_int_distribution<uint16_t> dist(0, 255);
        for (auto& byte : key) {
            byte = static_cast<uint8_t>(dist(gen_));
        }
        return key;
    }

private:
    std::mt19937 gen_;
};

} // namespace test
} // namespace themis
