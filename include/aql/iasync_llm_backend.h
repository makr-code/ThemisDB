/**
 * @file iasync_llm_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llm_plugin_interface.h"
#include "utils/expected.h"
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// IAsyncLLMBackend
// ============================================================================

/**
 * @brief Non-blocking async interface for LLM inference backends.
 *
 * All methods return a `std::future<Result<T>>` so that callers can issue
 * requests without blocking the calling thread.  Concrete implementations
 * are expected to hand the work off to a thread pool or I/O loop immediately,
 * keeping dispatch overhead ≤ 50 µs (excluding actual model generation time).
 *
 * Error handling:
 *  - The future resolves to a `Result<T>` (tl::expected) on both success
 *    and expected-failure paths (model not loaded, invalid request, etc.).
 *  - Unexpected failures (internal bugs, OOM) may surface as future exceptions
 *    propagated through `std::promise::set_exception()`.
 *
 * ABI note:
 *  New virtual methods may only be appended at the end of the vtable.
 *  Breaking changes must be versioned as `IAsyncLLMBackendV2`.
 *
 * Usage:
 * @code
 *   std::shared_ptr<IAsyncLLMBackend> backend = makeMyBackend();
 *
 *   llm::InferenceRequest req;
 *   req.prompt   = "Translate to French: Hello, world!";
 *   req.model_id = "llama-3-8b";
 *
 *   auto fut = backend->inferAsync(req);
 *   // ... do other work ...
 *   auto result = fut.get();
 *   if (result) {
 *       std::cout << result.value() << '\n';
 *   } else {
 *       std::cerr << result.error().message() << '\n';
 *   }
 * @endcode
 */
class IAsyncLLMBackend {
public:
    virtual ~IAsyncLLMBackend() = default;

    // -------------------------------------------------------------------------
    // Core async inference
    // -------------------------------------------------------------------------

    /**
     * @brief Asynchronously run text-generation inference.
     *
     * @param req  Inference request (prompt, model_id, generation parameters, …).
     * @return     Future that resolves to the generated text on success, or an
     *             `Error` describing the failure.
     *
     * @note Dispatch overhead (call → thread hand-off) must be ≤ 50 µs.
     */
    virtual std::future<Result<std::string>>
    inferAsync(const llm::InferenceRequest& req) = 0;

    /**
     * @brief Asynchronously compute a text embedding.
     *
     * @param text  Input text to embed.
     * @return      Future that resolves to the embedding vector on success, or
     *              an `Error` describing the failure.
     *
     * @note Dispatch overhead (call → thread hand-off) must be ≤ 50 µs.
     */
    virtual std::future<Result<std::vector<float>>>
    embedAsync(const std::string& text) = 0;

    // -------------------------------------------------------------------------
    // Optional capability query
    // -------------------------------------------------------------------------

    /**
     * @brief Return true if this backend supports multi-modal inference.
     *
     * When false, `inferAsync()` must return an error for any request that
     * carries multi-modal inputs.
     *
     * The default implementation returns false; concrete backends override
     * this when they support vision or audio models.
     */
    virtual bool supportsMultiModal() const { return false; }
};

// ============================================================================
// ThreadPoolAsyncLLMBackend
// ============================================================================

/**
 * @brief Adapter that wraps a synchronous `llm::ILLMPlugin` and exposes the
 *        non-blocking `IAsyncLLMBackend` interface via `std::async`.
 *
 * Each call to `inferAsync()` or `embedAsync()` launches a detached task on
 * `std::launch::async` (a separate OS thread).  This is suitable for
 * low-to-medium concurrency scenarios.  High-throughput applications should
 * provide a custom implementation backed by a bounded thread pool.
 *
 * Thread-safety: `inferAsync()` and `embedAsync()` are thread-safe and may be
 * called concurrently from multiple threads.  The underlying plugin must also
 * be thread-safe for concurrent `generate()` and `embed()` calls.
 *
 * Usage:
 * @code
 *   auto plugin = std::make_shared<MyLLMPlugin>();
 *   auto backend = std::make_shared<ThreadPoolAsyncLLMBackend>(plugin);
 *
 *   auto fut = backend->inferAsync(req);
 *   auto result = fut.get();
 * @endcode
 */
class ThreadPoolAsyncLLMBackend : public IAsyncLLMBackend {
public:
    /**
     * @param plugin  Synchronous LLM plugin whose generate() and embed() methods
     *                will be called from worker threads.  Must not be null.
     */
    explicit ThreadPoolAsyncLLMBackend(std::shared_ptr<llm::ILLMPlugin> plugin)
        : plugin_(std::move(plugin))
    {
        if (!plugin_) {
            throw std::invalid_argument(
                "ThreadPoolAsyncLLMBackend: plugin must not be null"
            );
        }
    }

    /**
     * @brief Dispatch `plugin_->generate(req)` on a separate thread.
     *
     * On success, the future resolves to `response.text`.
     * On any exception from the plugin, the future resolves to an Error.
     */
    std::future<Result<std::string>>
    inferAsync(const llm::InferenceRequest& req) override {
        // Copy req to avoid dangling reference in the async lambda.
        llm::InferenceRequest req_copy = req;
        auto plugin = plugin_;

        return std::async(std::launch::async, [plugin, req_copy]() -> Result<std::string> {
            try {
                auto response = plugin->generate(req_copy);
                return response.text;
            } catch (const std::exception& e) {
                return Err<std::string>(errors::ErrorCode::ERR_UNKNOWN, e.what());
            } catch (...) {
                return Err<std::string>(errors::ErrorCode::ERR_UNKNOWN,
                                        "unknown error in inferAsync");
            }
        });
    }

    /**
     * @brief Dispatch `plugin_->embed(text)` on a separate thread.
     *
     * On success, the future resolves to the embedding vector.
     * On any exception from the plugin, the future resolves to an Error.
     */
    std::future<Result<std::vector<float>>>
    embedAsync(const std::string& text) override {
        auto plugin = plugin_;

        return std::async(std::launch::async, [plugin, text]() -> Result<std::vector<float>> {
            try {
                return plugin->embed(text);
            } catch (const std::exception& e) {
                return Err<std::vector<float>>(errors::ErrorCode::ERR_UNKNOWN, e.what());
            } catch (...) {
                return Err<std::vector<float>>(errors::ErrorCode::ERR_UNKNOWN,
                                               "unknown error in embedAsync");
            }
        });
    }

    /**
     * @brief Delegates to the underlying plugin's LLMCapabilities.
     */
    bool supportsMultiModal() const override {
        auto caps = plugin_->getCapabilities();
        return caps.supports_multimodal;
    }

private:
    std::shared_ptr<llm::ILLMPlugin> plugin_;
};

} // namespace aql
} // namespace themis
