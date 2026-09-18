/**
 * @file iasync_llm_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class IAsyncLLMBackend {
public:
    /**
     * @brief IAsync LLMBackend.
     * @return Return value.
     */
    virtual ~IAsyncLLMBackend() = default;

    // -------------------------------------------------------------------------
    // Core async inference
    // -------------------------------------------------------------------------

    virtual std::future<Result<std::string>>
    inferAsync(const llm::InferenceRequest& req) = 0;

    virtual std::future<Result<std::vector<float>>>
    embedAsync(const std::string& text) = 0;

    // -------------------------------------------------------------------------
    // Optional capability query
    // -------------------------------------------------------------------------

    virtual bool supportsMultiModal() const { return false; }
};

// ============================================================================
// ThreadPoolAsyncLLMBackend
// ============================================================================

class ThreadPoolAsyncLLMBackend : public IAsyncLLMBackend {
public:
    /**
     * @brief Thread Pool Async LLMBackend.
     * @param[in] plugin Input parameter.
     * @return Return value.
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

    bool supportsMultiModal() const override {
        auto caps = plugin_->getCapabilities();
        return caps.supports_multimodal;
    }

private:
    std::shared_ptr<llm::ILLMPlugin> plugin_;
};

} // namespace aql
} // namespace themis
