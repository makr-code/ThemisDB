#pragma once

/**
 * @file kv_prefix_transfer_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "sharding/remote_executor.h"
#include "sharding/shard_topology.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis::llm {

// ─────────────────────────────────────────────────────────────────────────────
// IKVStateSerializer — abstraction over llama_state_seq_save/load_file
// ─────────────────────────────────────────────────────────────────────────────

struct IKVStateSerializer {
    /**
     * @brief IKVState Serializer.
     * @return Return value.
     */
    virtual ~IKVStateSerializer() = default;

    /**
     * @brief Serialise.
     * @param[in] prefix_text Input parameter.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    virtual std::vector<std::uint8_t> serialise(const std::string& prefix_text,
                                                 const std::string& model_id) = 0;

    /**
     * @brief Model Fingerprint.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    virtual std::string modelFingerprint(const std::string& model_id) const = 0;
};

class NullKVStateSerializer final : public IKVStateSerializer {
public:
    ~NullKVStateSerializer() override = default;

    using SerialiseFn =
        std::function<std::vector<std::uint8_t>(const std::string& prefix_text,
                                                const std::string& model_id)>;
    using ModelFingerprintFn =
        std::function<std::string(const std::string& model_id)>;

    /**
     * @brief Set Serialise Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), serialiseFnMutex(), serialiseFnStorage(), std::move().
     */
    static void setSerialiseFn(SerialiseFn fn) {
        std::lock_guard<std::mutex> lk(serialiseFnMutex());
        serialiseFnStorage() = std::move(fn);
    }
    /**
     * @brief Set Model Fingerprint Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), modelFingerprintFnMutex(), modelFingerprintFnStorage(), std::move().
     */
    static void setModelFingerprintFn(ModelFingerprintFn fn) {
        std::lock_guard<std::mutex> lk(modelFingerprintFnMutex());
        modelFingerprintFnStorage() = std::move(fn);
    }

    std::vector<std::uint8_t> serialise(const std::string& prefix_text,
                                        const std::string& model_id) override {
        SerialiseFn fn;
        {
            std::lock_guard<std::mutex> lk(serialiseFnMutex());
            fn = serialiseFnStorage();
        }
        if (fn) {
            try {
                return fn(prefix_text, model_id);
            } catch (...) {
                return std::vector<std::uint8_t>(prefix_text.begin(), prefix_text.end());
            }
        }
        return std::vector<std::uint8_t>(prefix_text.begin(), prefix_text.end());
    }

    std::string modelFingerprint(const std::string& model_id) const override {
        ModelFingerprintFn fn;
        {
            std::lock_guard<std::mutex> lk(modelFingerprintFnMutex());
            fn = modelFingerprintFnStorage();
        }
        if (fn) {
            try {
                return fn(model_id);
            } catch (...) {
                return "null:" + model_id;
            }
        }
        return "null:" + model_id;
    }

private:
    /**
     * @brief Serialise Fn Mutex.
     * @return Return value.
     * @details Implements serialiseFnMutex without additional internal calls.
     */
    static std::mutex& serialiseFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief Serialise Fn Storage.
     * @return Return value.
     * @details Implements serialiseFnStorage without additional internal calls.
     */
    static SerialiseFn& serialiseFnStorage() {
        static SerialiseFn fn;
        return fn;
    }
    /**
     * @brief Model Fingerprint Fn Mutex.
     * @return Return value.
     * @details Implements modelFingerprintFnMutex without additional internal calls.
     */
    static std::mutex& modelFingerprintFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief Model Fingerprint Fn Storage.
     * @return Return value.
     * @details Implements modelFingerprintFnStorage without additional internal calls.
     */
    static ModelFingerprintFn& modelFingerprintFnStorage() {
        static ModelFingerprintFn fn;
        return fn;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// KVPrefixTransferManager
// ─────────────────────────────────────────────────────────────────────────────

class KVPrefixTransferManager {
public:
    using SerializerFactoryFn = std::function<std::unique_ptr<IKVStateSerializer>()>;

    /**
     * @brief Set Default Serializer Factory.
     * @param[in] fn Input parameter.
     */
    static void setDefaultSerializerFactory(SerializerFactoryFn fn);

    explicit KVPrefixTransferManager(
        ::themis::sharding::RemoteExecutor& remote_executor,
        std::unique_ptr<IKVStateSerializer> serializer = nullptr,
        std::size_t min_prefix_tokens = 256);

    ~KVPrefixTransferManager();
    KVPrefixTransferManager(const KVPrefixTransferManager&) = delete;
    KVPrefixTransferManager& operator=(const KVPrefixTransferManager&) = delete;

    bool transferIfBeneficial(const ::themis::sharding::ShardInfo& target_shard,
                               const std::string& prefix_text,
                               const std::string& model_id,
                               std::size_t estimated_tokens = 0);

    /**
     * @brief Transfer Attempt Count.
     * @return Return value.
     */
    std::size_t transferAttemptCount() const;

    /**
     * @brief Transfer Success Count.
     * @return Return value.
     */
    std::size_t transferSuccessCount() const;

private:
    ::themis::sharding::RemoteExecutor& remote_executor_;
    std::unique_ptr<IKVStateSerializer> serializer_;
    const std::size_t min_prefix_tokens_;

    mutable std::atomic<std::size_t> attempt_count_{0};
    mutable std::atomic<std::size_t> success_count_{0};

    static constexpr std::size_t kCharsPerToken = 4;

    static constexpr const char* kIngestPath = "/api/v1/kv-prefix/ingest";

    /**
     * @brief Serializer Factory Mutex.
     * @return Return value.
     */
    static std::mutex& serializerFactoryMutex();
    /**
     * @brief Serializer Factory Storage.
     * @return Return value.
     */
    static SerializerFactoryFn& serializerFactoryStorage();
};

} // namespace themis::llm
