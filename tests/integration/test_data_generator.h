/*
 * ThemisDB | File: test_data_generator.h | Version: 0.0.48
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_data_generator.h
 * @brief Utilities for generating integration and cross-module pipeline test data.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <vector>

namespace themis {
namespace test {

/**
 * @brief Generates test data for integration and pipeline tests.
 */
class TestDataGenerator {
public:
    TestDataGenerator() : gen_(std::random_device{}()) {}

    [[nodiscard]] std::string GenerateRandomString(size_t length) {
        static constexpr char kCharset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::uniform_int_distribution<size_t> dist(0, sizeof(kCharset) - 2);

        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result.push_back(kCharset[dist(gen_)]);
        }
        return result;
    }

    [[nodiscard]] int GenerateRandomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }

    [[nodiscard]] nlohmann::json GenerateTestDocument(const std::string& id_prefix = "doc") {
        return {
            {"id", id_prefix + "_" + std::to_string(GenerateRandomInt(1000, 9999))},
            {"title", "Test Document " + GenerateRandomString(10)},
            {"content", GenerateRandomString(100)},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"value", GenerateRandomInt(1, 1000)}
        };
    }

    [[nodiscard]] std::vector<nlohmann::json> GenerateTestDocuments(size_t count,
                                                                     const std::string& id_prefix = "doc") {
        std::vector<nlohmann::json> docs;
        docs.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            docs.push_back(GenerateTestDocument(id_prefix));
        }
        return docs;
    }

    [[nodiscard]] std::vector<uint8_t> GenerateEncryptionKey(size_t key_size = 32) {
        std::vector<uint8_t> key(key_size);
        std::uniform_int_distribution<uint16_t> dist(0, 255);
        for (auto& byte : key) {
            byte = static_cast<uint8_t>(dist(gen_));
        }
        return key;
    }

    [[nodiscard]] std::string GeneratePipelineToken(bool valid = true) {
        const auto token = GenerateRandomString(16);
        return valid ? "valid_" + token : "invalid_" + token;
    }

    [[nodiscard]] std::string GenerateAqlQuery(const std::string& collection,
                                               const std::string& predicate,
                                               size_t limit = 10) {
        return "FOR d IN " + collection + " FILTER " + predicate + " LIMIT " +
               std::to_string(limit) + " RETURN d";
    }

    [[nodiscard]] std::vector<std::string> GenerateTerms(size_t count,
                                                         const std::string& prefix = "term") {
        std::vector<std::string> terms;
        terms.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            terms.push_back(prefix + "_" + std::to_string(i));
        }
        return terms;
    }

    [[nodiscard]] std::vector<float> GenerateEmbedding(size_t dims = 8) {
        std::uniform_real_distribution<float> dist(0.0F, 1.0F);
        std::vector<float> embedding(dims, 0.0F);
        for (auto& value : embedding) {
            value = dist(gen_);
        }
        return embedding;
    }

    [[nodiscard]] nlohmann::json GenerateCdcEvent(const std::string& id,
                                                  const std::string& operation,
                                                  const nlohmann::json& payload) {
        return {
            {"id", id},
            {"operation", operation},
            {"payload", payload},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        };
    }

private:
    std::mt19937 gen_;
};

} // namespace test
} // namespace themis
