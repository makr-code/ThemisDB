/*
 * ThemisDB | File: kv_prefix_transfer_manager.h | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 257
 * Gap Summary: total=7; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2026 ThemisDB — Licensed under MIT License
#pragma once

/**
 * @file kv_prefix_transfer_manager.h
 * @brief Phase 5 — KV-Prefix Cross-Shard Transfer (LLM+RAID v2.0.0)
 *
 * Enables one ThemisDB shard (the "source") to pre-compute the KV cache
 * for a shared system-prompt prefix and send it to one or more peer shards
 * ("targets") so they can warm their KV caches before the user-turn is
 * processed.  This cuts TTFT (time-to-first-token) by >= 30 % for requests
 * that share long system prompts (>= 256 tokens).
 *
 * Design constraints
 * ------------------
 * - Only applicable when source and target run the *same* base model +
 *   quantisation (verified via model_fingerprint).
 * - The serialised KV state is sent via RemoteExecutor::postBinary() to
 *   the path POST /api/v1/kv-prefix/ingest on the target shard.
 * - The transfer is best-effort: a failure is logged but never propagates
 *   to the inference path (inference falls back to a cold start).
 * - Thread-safety: all public methods are safe to call concurrently.
 *
 * STUB/SIMULATION NOTE (llama_state_seq_save_file path):
 * Purpose: In the current implementation the KV state is serialised via a
 *          thin abstraction (IKVStateSerializer) whose production backend
 *          wraps llama_state_seq_save_file / llama_state_seq_load_file from
 *          llama.cpp.  The default (no-op) backend produces a zero-byte
 *          payload, which means warm-up on the receiver has no effect.
 * Activation: Production: link against KVStateSerializerLlama (future PR).
 *             Default:    NullKVStateSerializer (current, for CI builds).
 * Production Delta: NullKVStateSerializer serialises nothing; a real
 *                   TTFT improvement requires the llama.cpp backend.
 * Removal Plan: Remove NullKVStateSerializer as default once the llama.cpp
 *               backend is available and validated (Target: Q2 2027).
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

/**
 * @brief Serialise the computed KV state for a prompt prefix into a byte
 *        buffer that can be transferred to another shard.
 *
 * The production implementation wraps @c llama_state_seq_save_file /
 * @c llama_state_seq_load_file from llama.cpp.  Tests and CI builds use
 * @c NullKVStateSerializer (see below).
 */
struct IKVStateSerializer {
    virtual ~IKVStateSerializer() = default;

    /**
     * Serialise the KV state for the given prefix text.
     *
     * @param prefix_text  System-prompt text already evaluated on this shard.
     * @param model_id     Model identifier used for evaluation.
     * @return Serialised bytes, or empty if serialisation fails / not supported.
     */
    virtual std::vector<std::uint8_t> serialise(const std::string& prefix_text,
                                                 const std::string& model_id) = 0;

    /**
     * Return an opaque fingerprint that identifies the model + quantisation.
     * Two shards must have the same fingerprint for a transfer to make sense.
     */
    virtual std::string modelFingerprint(const std::string& model_id) const = 0;
};

/**
 * @brief No-op serialiser used in CI / builds without a live llama.cpp model.
 *
 * STUB/SIMULATION NOTE:
 * Purpose:          Allow KVPrefixTransferManager to compile and run without
 *                   a linked llama.cpp model.
 * Activation:       Default when no IKVStateSerializer is injected.
 * Production Delta: Returns an empty buffer; no actual KV state is sent.
 * Removal Plan:     Replace with KVStateSerializerLlama once available (Q2 2027).
 */
class NullKVStateSerializer final : public IKVStateSerializer {
public:
    ~NullKVStateSerializer() override = default;

    using SerialiseFn =
        std::function<std::vector<std::uint8_t>(const std::string& prefix_text,
                                                const std::string& model_id)>;
    using ModelFingerprintFn =
        std::function<std::string(const std::string& model_id)>;

    static void setSerialiseFn(SerialiseFn fn) {
        std::lock_guard<std::mutex> lk(serialiseFnMutex());
        serialiseFnStorage() = std::move(fn);
    }
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
                return {};
            }
        }
        return {};
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
    static std::mutex& serialiseFnMutex() {
        static std::mutex m;
        return m;
    }
    static SerialiseFn& serialiseFnStorage() {
        static SerialiseFn fn;
        return fn;
    }
    static std::mutex& modelFingerprintFnMutex() {
        static std::mutex m;
        return m;
    }
    static ModelFingerprintFn& modelFingerprintFnStorage() {
        static ModelFingerprintFn fn;
        return fn;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// KVPrefixTransferManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates KV-prefix transfer between shards.
 *
 * Usage:
 * @code
 *   KVPrefixTransferManager mgr(remote_executor);
 *
 *   // In executeInfer(), after domain routing selected target_shard:
 *   if (system_prompt_tokens >= 256) {
 *       mgr.transferIfBeneficial(target_shard_info, system_prompt, model_id);
 *   }
 * @endcode
 */
class KVPrefixTransferManager {
public:
    using SerializerFactoryFn = std::function<std::unique_ptr<IKVStateSerializer>()>;

    static void setDefaultSerializerFactory(SerializerFactoryFn fn);

    /**
     * @param remote_executor  Shared RemoteExecutor used to send the KV state.
     * @param serializer       Optional custom serialiser (defaults to NullKVStateSerializer).
     * @param min_prefix_tokens Minimum token count before a transfer is attempted (default: 256).
     */
    explicit KVPrefixTransferManager(
        ::themis::sharding::RemoteExecutor& remote_executor,
        std::unique_ptr<IKVStateSerializer> serializer = nullptr,
        std::size_t min_prefix_tokens = 256);

    ~KVPrefixTransferManager();
    KVPrefixTransferManager(const KVPrefixTransferManager&) = delete;
    KVPrefixTransferManager& operator=(const KVPrefixTransferManager&) = delete;

    /**
     * @brief Transfer KV prefix to @p target_shard if the system prompt is
     *        long enough and the models are compatible.
     *
     * The call is fire-and-best-effort: a network failure is logged but never
     * surfaced to the inference path.
     *
     * @param target_shard    Target shard that will receive the inference request.
     * @param prefix_text     System-prompt / prefix text (already evaluated locally).
     * @param model_id        Model identifier (used for fingerprint compatibility check).
     * @param estimated_tokens Caller-supplied token count estimate (0 = auto-estimate via
     *                         char/4 heuristic).
     * @return true if a transfer was initiated (regardless of its success), false if
     *         the preconditions (length, model compat) were not met.
     */
    bool transferIfBeneficial(const ::themis::sharding::ShardInfo& target_shard,
                               const std::string& prefix_text,
                               const std::string& model_id,
                               std::size_t estimated_tokens = 0);

    /**
     * @return Cumulative number of transfers attempted since construction.
     */
    std::size_t transferAttemptCount() const;

    /**
     * @return Cumulative number of transfers that succeeded (HTTP 2xx).
     */
    std::size_t transferSuccessCount() const;

private:
    ::themis::sharding::RemoteExecutor& remote_executor_;
    std::unique_ptr<IKVStateSerializer> serializer_;
    const std::size_t min_prefix_tokens_;

    mutable std::atomic<std::size_t> attempt_count_{0};
    mutable std::atomic<std::size_t> success_count_{0};

    /// Heuristic: 1 token ≈ 4 chars.
    static constexpr std::size_t kCharsPerToken = 4;

    static constexpr const char* kIngestPath = "/api/v1/kv-prefix/ingest";

    static std::mutex& serializerFactoryMutex();
    static SerializerFactoryFn& serializerFactoryStorage();
};

} // namespace themis::llm
