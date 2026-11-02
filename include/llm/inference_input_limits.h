/**
 * @file inference_input_limits.h
 * @brief Conservative byte bounds for managed inference inputs.
 */

#pragma once

#include "llm/llm_api_contract.h"
#include "llm/llm_plugin_interface.h"

#include <cstddef>

namespace themis::llm {
namespace detail {

/// @brief Add byte counts without allowing the running total to exceed a limit.
/// @param total Running total, updated only when the addition fits.
/// @param bytes Number of bytes to add.
/// @param limit Maximum accepted total.
/// @return True when the addition fits; false otherwise.
inline bool addWithinLimit(std::size_t& total, std::size_t bytes,
                           std::size_t limit) noexcept {
    if (total > limit || bytes > limit - total) {
        return false;
    }
    total += bytes;
    return true;
}

/// @brief Count JSON structure and string bytes within a conservative bound.
/// @param value JSON value to measure.
/// @param total Running byte total.
/// @param limit Maximum accepted total.
/// @param depth Current nesting depth; values deeper than 64 are rejected.
/// @return True when the value fits within the remaining limit.
inline bool addJsonBytes(const json& value, std::size_t& total,
                         std::size_t limit, std::size_t depth = 0) {
    if (depth > 64) {
        return false;
    }
    if (value.is_string()) {
        return addWithinLimit(total, 2, limit) &&
            addWithinLimit(total, value.get_ref<const std::string&>().size(), limit);
    }
    if (value.is_array()) {
        if (!addWithinLimit(total, 2, limit)) {
            return false;
        }
        for (const auto& item : value) {
            if (!addWithinLimit(total, 1, limit) ||
                !addJsonBytes(item, total, limit, depth + 1)) {
                return false;
            }
        }
        return true;
    }
    if (value.is_object()) {
        if (!addWithinLimit(total, 2, limit)) {
            return false;
        }
        for (const auto& item : value.items()) {
            if (!addWithinLimit(total, 4, limit) ||
                !addWithinLimit(total, item.key().size(), limit) ||
                !addJsonBytes(item.value(), total, limit, depth + 1)) {
                return false;
            }
        }
        return true;
    }

    // Reserve a conservative fixed allowance for JSON scalar syntax/value.
    return addWithinLimit(total, 32, limit);
}

/// @brief Add prompt and optional system-prompt bytes to a shared budget.
/// @param request Managed inference request to measure.
/// @param total Running byte total, updated only when both fields fit.
/// @param limit Maximum accepted total.
/// @return True when the prompt fields fit within the remaining limit.
inline bool addPromptBytes(const InferenceRequest& request, std::size_t& total,
                           std::size_t limit) noexcept {
    if (!addWithinLimit(total, request.prompt.size(), limit)) {
        return false;
    }
    return !request.system_prompt ||
        addWithinLimit(total, request.system_prompt->size(), limit);
}

/// @brief Add RAG query, template, document text, source, and metadata bytes.
/// @param context RAG context to measure.
/// @param total Running byte total.
/// @param limit Maximum accepted total across all managed inputs.
/// @return True when all fields fit; false when any field exceeds the limit.
inline bool addRagContextBytes(const RAGContext& context, std::size_t& total,
                              std::size_t limit) {
    if (!addWithinLimit(total, context.query.size(), limit) ||
        !addWithinLimit(total, context.context_template.size(), limit)) {
        return false;
    }
    for (const auto& document : context.documents) {
        if (!addWithinLimit(total, document.content.size(), limit) ||
            !addWithinLimit(total, document.source.size(), limit) ||
            !addJsonBytes(document.metadata, total, limit)) {
            return false;
        }
    }
    return true;
}

/// @brief Check prompt and system-prompt bytes against the managed input cap.
/// @param request Managed inference request to validate.
/// @return True when the combined fields fit within the byte limit.
inline bool promptWithinByteLimit(const InferenceRequest& request) noexcept {
    std::size_t total = 0;
    return addPromptBytes(request, total, kMaxPromptBytes);
}

/// @brief Check all request and RAG input fields against one byte budget.
/// @param request Managed inference request to validate.
/// @param context RAG context to validate.
/// @return True when all fields fit; this byte bound is not a token estimate.
inline bool inferenceInputsWithinByteLimit(const InferenceRequest& request,
                                           const RAGContext& context) {
    std::size_t total = 0;
    return addPromptBytes(request, total, kMaxPromptBytes) &&
        addRagContextBytes(context, total, kMaxPromptBytes);
}

} // namespace detail
} // namespace themis::llm
