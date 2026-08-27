/**
 * @file llm_wiki_status.h
 * @brief Lightweight `Status` result type for LLM Wiki plugin operations.
 *
 * Extracted from `llm_wiki_plugin_interface.h` so that low-level
 * implementation headers (e.g., `rocksdb_wiki_store.h`) can include this
 * single lightweight header without pulling in the full plugin interface
 * (which transitively includes `llm/wiki_index_store.h` → TBB → CUDA).
 *
 * `llm_wiki_plugin_interface.h` includes this header in place of its
 * previous inline Status definition; callers who included
 * `llm_wiki_plugin_interface.h` continue to get `Status` unchanged.
 *
 * @version 0.1.0
 * @date    2026-08-26
 */

#pragma once

#include <string>

namespace themis {
namespace plugins {
namespace llm_wiki {

/**
 * @brief Status result for ILLMWikiPlugin lifecycle operations.
 *
 * Lightweight value type returned by `initialize()`, `wikiInit()`, and
 * similar methods to signal success or failure with a human-readable message.
 */
struct Status {
    /// @brief Status code categories.
    enum class Code {
        Ok,               ///< Operation succeeded.
        Error,            ///< Generic failure.
        PermissionDenied, ///< Sub-feature or edition gate blocked the call.
        InvalidArgument,  ///< Malformed or out-of-range input.
        NotInitialized,   ///< Plugin is not yet initialized.
    };

    Code        code    = Code::Ok;
    std::string message;

    [[nodiscard]] bool ok() const noexcept { return code == Code::Ok; }

    [[nodiscard]] static Status Ok() { return {Code::Ok, {}}; }
    [[nodiscard]] static Status Error(std::string msg)
        { return {Code::Error, std::move(msg)}; }
    [[nodiscard]] static Status PermissionDenied(std::string msg)
        { return {Code::PermissionDenied, std::move(msg)}; }
    [[nodiscard]] static Status InvalidArgument(std::string msg)
        { return {Code::InvalidArgument, std::move(msg)}; }
    [[nodiscard]] static Status NotInitialized()
        { return {Code::NotInitialized,
                  "plugin not initialized; call initialize() first"}; }
};

}  // namespace llm_wiki
}  // namespace plugins
}  // namespace themis
