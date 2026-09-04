/**
 * @file kv_prefix_transfer_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/kv_prefix_transfer_manager.h"
#include <stdexcept>

#include <spdlog/spdlog.h>
#include "utils/logger.h"

namespace themis::llm {

std::mutex& KVPrefixTransferManager::serializerFactoryMutex() {
    static std::mutex m;
    return m;
}

KVPrefixTransferManager::SerializerFactoryFn&
KVPrefixTransferManager::serializerFactoryStorage() {
    static SerializerFactoryFn fn;
    return fn;
}

void KVPrefixTransferManager::setDefaultSerializerFactory(SerializerFactoryFn fn) {
    std::lock_guard<std::mutex> lk(serializerFactoryMutex());
    serializerFactoryStorage() = std::move(fn);
}

KVPrefixTransferManager::KVPrefixTransferManager(
    ::themis::sharding::RemoteExecutor& remote_executor,
    std::unique_ptr<IKVStateSerializer> serializer,
    std::size_t min_prefix_tokens)
    : remote_executor_(remote_executor)
    , serializer_([&]() -> std::unique_ptr<IKVStateSerializer> {
        if (serializer) {
            return std::move(serializer);
        }
        SerializerFactoryFn factory;
        {
            std::lock_guard<std::mutex> lk(serializerFactoryMutex());
            factory = serializerFactoryStorage();
        }
        if (factory) {
            try {
                auto custom = factory();
                if (custom) {
                    return custom;
                }
            } catch (...) {
                THEMIS_WARN("kv_prefix_transfer_manager::remote_executor_: unhandled exception caught");
                // fail-closed: default back to NullKVStateSerializer
            }
        }
        return std::make_unique<NullKVStateSerializer>();
    }())
    , min_prefix_tokens_(min_prefix_tokens)
{}

KVPrefixTransferManager::~KVPrefixTransferManager() = default;

bool KVPrefixTransferManager::transferIfBeneficial(
    const ::themis::sharding::ShardInfo& target_shard,
    const std::string& prefix_text,
    const std::string& model_id,
    std::size_t estimated_tokens)
{
    // Estimate token count if not provided.
    const std::size_t token_estimate =
        (estimated_tokens > 0) ? estimated_tokens
                               : (prefix_text.size() / kCharsPerToken);

    if (token_estimate < min_prefix_tokens_) {
        spdlog::debug(
            "[KVPrefix] Skipping transfer to {}: estimated tokens {} < threshold {}",
            target_shard.shard_id, token_estimate, min_prefix_tokens_);
        return false;
    }

    // Serialise KV state (may return empty for NullKVStateSerializer).
    auto payload = serializer_->serialise(prefix_text, model_id);

    if (payload.empty()) {
        spdlog::debug(
            "[KVPrefix] Serialiser returned empty payload for model={} shard={}; "
            "skipping binary transfer (NullKVStateSerializer active)",
            model_id, target_shard.shard_id);
        // Still count as an attempt so callers can detect the no-op path.
        attempt_count_.fetch_add(1, std::memory_order_relaxed);
        return true; // preconditions met; transfer attempted (no-op)
    }

    attempt_count_.fetch_add(1, std::memory_order_relaxed);

    const auto result = remote_executor_.postBinary(
        target_shard, kIngestPath, payload.data(),static_cast<int>(payload.size()));

    if (result.success) {
        success_count_.fetch_add(1, std::memory_order_relaxed);
        spdlog::debug(
            "[KVPrefix] KV state transferred to shard={}: {} bytes, model={}",
            target_shard.shard_id,static_cast<int>(payload.size()), model_id);
    } else {
        spdlog::warn(
            "[KVPrefix] Transfer to shard={} failed: {}; inference continues cold",
            target_shard.shard_id, result.error);
    }

    return true;
}

std::size_t KVPrefixTransferManager::transferAttemptCount() const
{
    return attempt_count_.load(std::memory_order_relaxed);
}

std::size_t KVPrefixTransferManager::transferSuccessCount() const
{
    return success_count_.load(std::memory_order_relaxed);
}

} // namespace themis::llm

