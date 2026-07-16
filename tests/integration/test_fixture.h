/*
 * ThemisDB | File: test_fixture.h | Version: 0.0.48
 * Maturity: 🟢 PRODUCTION-READY | Score: 88/100
 * Gap Summary: total=9; TODO=1, Stub=2, Unimpl=0, Mock=5, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_fixture.h
 * @brief Base fixture and reusable pipeline mocks for integration tests.
 */

#pragma once

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace themis {
namespace test {

/**
 * @brief Result of a mock authorization check.
 */
struct MockAuthResult {
    bool authorized{false};
    std::string reason{"unauthorized"};
};

/**
 * @brief Audit event used by pipeline integration tests.
 */
struct PipelineAuditEvent {
    std::string module;
    std::string action;
    std::string detail;
};

/**
 * @brief Thread-safe in-memory audit log for pipeline tests.
 */
class PipelineAuditLog {
public:
    void Record(PipelineAuditEvent event) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(std::move(event));
    }

    [[nodiscard]] size_t Count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

    [[nodiscard]] bool Contains(const std::string& module, const std::string& action) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& event : events_) {
            if (event.module == module && event.action == action) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::vector<PipelineAuditEvent> Snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_;
    }

private:
    mutable std::mutex mutex_;
    std::vector<PipelineAuditEvent> events_;
};

/**
 * @brief Thread-safe in-memory key/value storage for pipeline tests.
 */
class InMemoryPipelineStorage {
public:
    [[nodiscard]] bool Write(const std::string& key, std::string value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = std::move(value);
        return true;
    }

    [[nodiscard]] std::optional<std::string> Read(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = data_.find(key);
        if (it == data_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    [[nodiscard]] bool Erase(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.erase(key) > 0;
    }

    [[nodiscard]] bool Contains(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.find(key) != data_.end();
    }

    [[nodiscard]] size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

/**
 * @brief Thread-safe inverted index used by pipeline tests.
 */
class MockPipelineIndex {
public:
    void IndexDocument(const std::string& document_id, const std::vector<std::string>& terms) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& term : terms) {
            term_to_docs_[term].push_back(document_id);
        }
    }

    /**
     * @brief Removes a document ID from every indexed term.
     * @param document_id Stable document identifier to erase from the inverted index.
     * @return true if at least one posting-list entry was removed; false if the ID was absent.
     */
    [[nodiscard]] bool RemoveDocument(const std::string& document_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        bool removed = false;
        for (auto it = term_to_docs_.begin(); it != term_to_docs_.end();) {
            auto& docs = it->second;
            const auto new_end = std::remove(docs.begin(), docs.end(), document_id);
            if (new_end != docs.end()) {
                docs.erase(new_end, docs.end());
                removed = true;
            }
            if (docs.empty()) {
                it = term_to_docs_.erase(it);
            } else {
                ++it;
            }
        }
        return removed;
    }

    [[nodiscard]] std::vector<std::string> Search(const std::string& term) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = term_to_docs_.find(term);
        if (it == term_to_docs_.end()) {
            return {};
        }
        return it->second;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::string>> term_to_docs_;
};

/**
 * @brief Configurable auth mock for pipeline tests.
 */
class MockPipelineAuth {
public:
    void AllowToken(std::string token) {
        std::lock_guard<std::mutex> lock(mutex_);
        allowed_tokens_.insert(std::move(token));
    }

    void DenyToken(std::string token) {
        std::lock_guard<std::mutex> lock(mutex_);
        denied_tokens_.insert(std::move(token));
    }

    [[nodiscard]] MockAuthResult Authorize(const std::string& token) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (denied_tokens_.count(token) > 0) {
            return {false, "token-denied"};
        }
        if (allowed_tokens_.count(token) > 0) {
            return {true, "authorized"};
        }
        return {false, "token-unknown"};
    }

private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> allowed_tokens_;
    std::unordered_set<std::string> denied_tokens_;
};

/**
 * @brief Configurable local LLM mock with embedding and inference counters.
 */
class MockPipelineLlmBackend {
public:
    // STUB/SIMULATION NOTE:
    // Purpose: Deterministic offline LLM behavior for integration pipeline tests.
    // Activation: Test-only via tests/integration fixtures.
    // Production Delta: No external model loading or runtime inference backends.
    // Removal Plan: Keep for deterministic CI coverage of module interfaces.
    void SetEmbeddingFailure(bool fail) {
        std::lock_guard<std::mutex> lock(mutex_);
        fail_embedding_ = fail;
    }

    void SetInferenceFailure(bool fail) {
        std::lock_guard<std::mutex> lock(mutex_);
        fail_inference_ = fail;
    }

    [[nodiscard]] std::optional<std::vector<float>> GenerateEmbedding(const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++embedding_calls_;
        if (fail_embedding_) {
            return std::nullopt;
        }

        std::vector<float> embedding(8, 0.0F);
        for (size_t i = 0; i < text.size(); ++i) {
            embedding[i % embedding.size()] += static_cast<float>((text[i] % 13) + 1);
        }
        return embedding;
    }

    [[nodiscard]] std::optional<std::string> Infer(const std::string& prompt,
                                                   const std::string& context) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++inference_calls_;
        if (fail_inference_) {
            return std::nullopt;
        }
        return std::string{"answer("} + prompt + ")::" + context;
    }

    [[nodiscard]] size_t EmbeddingCalls() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return embedding_calls_;
    }

    [[nodiscard]] size_t InferenceCalls() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return inference_calls_;
    }

private:
    mutable std::mutex mutex_;
    bool fail_embedding_{false};
    bool fail_inference_{false};
    size_t embedding_calls_{0};
    size_t inference_calls_{0};
};

/**
 * @brief Base fixture for all integration tests.
 */
class IntegrationTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() /
                    ("themis_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(temp_dir_)) {
            std::filesystem::remove_all(temp_dir_);
        }
    }

    [[nodiscard]] bool WaitForCondition(
        const std::function<bool()>& condition,
        std::chrono::seconds timeout = std::chrono::seconds(10),
        std::chrono::milliseconds check_interval = std::chrono::milliseconds(100)) {
        const auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (condition()) {
                return true;
            }
            std::this_thread::sleep_for(check_interval);
        }
        return false;
    }

    [[nodiscard]] const std::filesystem::path& GetTempDir() const {
        return temp_dir_;
    }

    [[nodiscard]] std::filesystem::path CreateTestDbPath(const std::string& name = "test_db") const {
        return temp_dir_ / name;
    }

    [[nodiscard]] std::shared_ptr<InMemoryPipelineStorage> CreateInMemoryStorage() const {
        return std::make_shared<InMemoryPipelineStorage>();
    }

    [[nodiscard]] std::shared_ptr<MockPipelineIndex> CreateMockIndex() const {
        return std::make_shared<MockPipelineIndex>();
    }

    [[nodiscard]] std::shared_ptr<MockPipelineAuth> CreateMockAuth() const {
        return std::make_shared<MockPipelineAuth>();
    }

    [[nodiscard]] std::shared_ptr<MockPipelineLlmBackend> CreateMockLlmBackend() const {
        return std::make_shared<MockPipelineLlmBackend>();
    }

    [[nodiscard]] std::shared_ptr<PipelineAuditLog> CreateAuditLog() const {
        return std::make_shared<PipelineAuditLog>();
    }

protected:
    std::filesystem::path temp_dir_;
};

} // namespace test
} // namespace themis
