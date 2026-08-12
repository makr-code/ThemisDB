/**
 * @file test_data_generator.h
 * @brief Utilities for generating integration and cross-module pipeline test data.
 *
 * Wave 2 additions (2026-07):
 * - `kCanonicalSeed` constant for reproducible cross-module test runs.
 * - `SeededTestDataGenerator` subclass that uses a fixed seed so all generated
 *   values are deterministic across machines and CI environments.
 * - `GenerateDocumentBatch()` and `GenerateVectorBatch()` helpers for bulk fixture
 *   creation used by the Wave 2 cross-module pipeline suites.
 */

#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <vector>

namespace themis {
namespace test {

/// @brief Canonical RNG seed used by all Wave 2 cross-module fixture tests.
///
/// Using a fixed seed guarantees deterministic RNG-driven values across CI runs
/// and local developer machines, which is required for regression-baseline
/// comparisons.  Note: fields generated from wall-clock time (e.g. timestamps)
/// are not controlled by this seed and may differ between runs.
static constexpr uint32_t kCanonicalSeed = 42U;

/**
 * @brief Generates test data for integration and pipeline tests.
 *
 * The default constructor uses `std::random_device` so values differ per run.
 * Use `SeededTestDataGenerator` (or pass an explicit seed to the protected
 * constructor) for deterministic, reproducible test data.
 */
class TestDataGenerator {
public:
    static constexpr uint32_t kDefaultSeed = 42U;

    // Default constructor uses non-deterministic random device for non-seeded tests.
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

    /**
     * @brief Generates a batch of @p count documents with a common @p prefix and
     *        optional @p terms list attached to each document.
     *
     * All documents have sequential ids (`<prefix>_<index>`) so the batch is
     * deterministic when used together with `SeededTestDataGenerator`.
     *
     * @param count   Number of documents to produce.
     * @param prefix  ID prefix; also used as the collection name hint.
     * @param terms   Search terms to attach to every document.
     * @return Vector of JSON documents ready for ingestion.
     */
    [[nodiscard]] std::vector<nlohmann::json> GenerateDocumentBatch(
            size_t count,
            const std::string& prefix = "doc",
            const std::vector<std::string>& terms = {}) {
        std::vector<nlohmann::json> batch;
        batch.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            auto doc = GenerateTestDocument(prefix + "_" + std::to_string(i));
            // Override id to be deterministic index-based so tests can assert order.
            doc["id"] = prefix + "_" + std::to_string(i);
            doc["seq"] = static_cast<int>(i);
            if (!terms.empty()) {
                doc["terms"] = nlohmann::json(terms);
            }
            batch.push_back(std::move(doc));
        }
        return batch;
    }

    /**
     * @brief Generates a batch of unit-normalized float embeddings.
     *
     * Each vector is L2-normalized so its magnitude equals 1.0, which matches
     * the expectation of cosine-similarity and ANN index tests.  Zero vectors
     * (all components zero) are left unchanged.
     *
     * @param count  Number of embedding vectors.
     * @param dims   Dimensionality of each vector.
     * @return Vector of unit-normalized embedding vectors.
     */
    [[nodiscard]] std::vector<std::vector<float>> GenerateVectorBatch(
            size_t count, size_t dims = 8) {
        std::vector<std::vector<float>> result;
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            auto vec = GenerateEmbedding(dims);
            // L2-normalize the vector.
            float norm = 0.0F;
            for (float v : vec) { norm += v * v; }
            if (norm > 0.0F) {
                norm = std::sqrt(norm);
                for (float& v : vec) { v /= norm; }
            }
            result.push_back(std::move(vec));
        }
        return result;
    }

protected:
    /// @brief Seed-accepting constructor for `SeededTestDataGenerator`.
    explicit TestDataGenerator(uint32_t seed) : gen_(seed) {}

private:
    std::mt19937 gen_;
};

/**
 * @brief Deterministic variant of `TestDataGenerator` seeded with
 *        `kCanonicalSeed` (42).
 *
 * Use this class in Wave 2 cross-module tests wherever reproducibility across
 * CI runs is required.  All RNG-driven values are identical on every invocation
 * with the same call sequence, regardless of host platform.  Fields derived
 * from wall-clock time (e.g. timestamps) are not deterministic.
 *
 * Example:
 * @code
 *   SeededTestDataGenerator gen;
 *   auto docs = gen.GenerateDocumentBatch(10, "cross", {"alpha", "beta"});
 *   // docs[0]["id"] == "cross_0", docs[1]["id"] == "cross_1", etc.
 * @endcode
 */
class SeededTestDataGenerator : public TestDataGenerator {
public:
    SeededTestDataGenerator() : TestDataGenerator(kCanonicalSeed) {}

    /// @brief Construct with an explicit seed; useful for parameterised tests.
    explicit SeededTestDataGenerator(uint32_t seed) : TestDataGenerator(seed) {}
};

} // namespace test
} // namespace themis
