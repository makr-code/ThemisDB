/**
 * @file default_model_path.h
 * @brief Resolves the local GGUF model used when a request does not select a model.
 */

#pragma once

#include <array>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

namespace themis::llm::detail {

/**
 * @brief Resolves an explicitly configured local model path.
 * @param base_directory Directory used to resolve relative overrides; the process
 *        current directory is used when this value is empty.
 * @param configured_model_path Optional configured filename to preserve when a model
 *        directory override is provided.
 * @return Existing regular model file, or `std::nullopt` when no override resolves.
 * @note Path precedence is THEMIS_LLM_DEFAULT_MODEL_PATH, THEMIS_DEMO_LLM_MODEL_PATH,
 *       then a file or matching model inside THEMIS_MODEL_DIR/THEMIS_LLM_MODEL_PATH.
 */
inline std::optional<std::filesystem::path> resolveModelPathOverride(
     std::filesystem::path base_directory = {},
     std::filesystem::path configured_model_path = {}) {
    std::error_code error = {};
    if (base_directory.empty()) {
        base_directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
    }

    const auto existing_file = [](const std::filesystem::path& candidate)
        -> std::optional<std::filesystem::path> {
        std::error_code candidate_error = {};
        if (!std::filesystem::is_regular_file(candidate, candidate_error) || candidate_error) {
            return std::nullopt;
        }
        return candidate.lexically_normal();
    };

    constexpr std::array<const char*, 2> kModelPathOverrides = {
        "THEMIS_DEMO_LLM_MODEL_PATH",
        "THEMIS_LLM_DEFAULT_MODEL_PATH",
    };
    for (const char* name : kModelPathOverrides) {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0') {
            continue;
        }
        std::filesystem::path candidate(value);
        if (candidate.is_relative()) {
            candidate = base_directory / candidate;
        }
        if (auto resolved = existing_file(candidate)) {
            return resolved;
        }
    }

    constexpr std::array<const char*, 2> kModelDirectoryOverrides = {
        "THEMIS_MODEL_DIR",
        "THEMIS_LLM_MODEL_PATH",
    };
    for (const char* name : kModelDirectoryOverrides) {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0') {
            continue;
        }
        std::filesystem::path candidate(value);
        if (candidate.is_relative()) {
            candidate = base_directory / candidate;
        }
        if (auto resolved = existing_file(candidate)) {
            return resolved;
        }
        std::error_code directory_error = {};
        if (std::filesystem::is_directory(candidate, directory_error) && !directory_error) {
            if (!configured_model_path.empty()) {
                if (auto resolved = existing_file(candidate / configured_model_path.filename())) {
                    return resolved;
                }
            }
            if (auto resolved = existing_file(candidate / "default.gguf")) {
                return resolved;
            }
        }
    }

    return std::nullopt;
}

/**
 * @brief Resolves the default local model when no explicit path is configured.
 * @param base_directory Directory used for relative candidates; the process current
 *        directory is used when this value is empty.
 * @return Existing regular GGUF model file, or `std::nullopt` when unavailable.
 * @note Explicit environment overrides are resolved before `models/default.gguf`.
 */
inline std::optional<std::filesystem::path> resolveDefaultModelPath(
    std::filesystem::path base_directory = {}) {
    if (auto override_path = resolveModelPathOverride(base_directory)) {
        return override_path;
    }

    std::error_code error = {};
    if (base_directory.empty()) {
        base_directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
    }

    constexpr std::array<const char*, 4> kDefaultModelCandidates = {
        "models/default.gguf",
        "default.gguf",
        "../models/default.gguf",
        "../../models/default.gguf",
    };
    for (const char* candidate_name : kDefaultModelCandidates) {
        const auto candidate = (base_directory / candidate_name).lexically_normal();
        std::error_code candidate_error = {};
        if (std::filesystem::is_regular_file(candidate, candidate_error) && !candidate_error) {
            return candidate;
        }
    }

    return std::nullopt;
}

} // namespace themis::llm::detail