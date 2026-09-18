/**
 * @file inmemory_secrets.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_secrets.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

class InMemorySecrets : public ISecrets {
public:
    InMemorySecrets() = default;

    explicit InMemorySecrets(std::map<std::string, std::string> initial)
        : secrets_(std::move(initial)) {}

    // -----------------------------------------------------------------------
    // ISecrets interface
    // -----------------------------------------------------------------------

    std::optional<std::string> getSecret(std::string_view name) const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = secrets_.find(std::string(name));
        if (it == secrets_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool hasSecret(std::string_view name) const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.count(std::string(name)) > 0;
    }

    std::vector<std::string> listSecretNames() const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names = {};

        names.reserve(secrets_.size());
        for (const auto& kv : secrets_) {
            names.push_back(kv.first);
        }
        // std::map is already sorted, so no additional sort needed
        return names;
    }

    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    /**
     * @brief ----------------------------------------------------------------------- Mutable operations (not part of ISecrets) -----------------------------------------------------------------------
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: lock(), std::string().
     */

    void setSecret(std::string_view name, std::string_view value) {
        std::lock_guard<std::mutex> lock(mutex_);
        secrets_[std::string(name)] = std::string(value);
    }

    /**
     * @brief Remove Secret.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lock(), erase(), std::string().
     */
    bool removeSecret(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.erase(std::string(name)) > 0;
    }

    size_t size() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.size();
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::string> secrets_;
};

// ---------------------------------------------------------------------------

class EnvSecretsProvider : public ISecrets {
public:
    explicit EnvSecretsProvider(std::string prefix = "THEMIS_SECRET_")
        : prefix_(std::move(prefix)) {}

    // -----------------------------------------------------------------------
    // ISecrets interface
    // -----------------------------------------------------------------------

    std::optional<std::string> getSecret(std::string_view name) const override {
        const std::string env_key = toEnvKey(name);
        const char* val = std::getenv(env_key.c_str()); // NOLINT(concurrency-mt-unsafe)
        if (!val) {
            return std::nullopt;
        }
        return std::string(val);
    }

    bool hasSecret(std::string_view name) const override {
        const std::string env_key = toEnvKey(name);
        return std::getenv(env_key.c_str()) != nullptr; // NOLINT(concurrency-mt-unsafe)
    }

    std::vector<std::string> listSecretNames() const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> result = {};

        result.reserve(registered_names_.size());
        for (const auto& name : registered_names_) {
            if (hasSecret(name)) {
                result.push_back(name);
            }
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    /**
     * @brief ----------------------------------------------------------------------- Registration (not part of ISecrets) -----------------------------------------------------------------------
     * @param[in] name Input parameter.
     * @details Calls: lock(), push_back(), std::string().
     */

    void registerName(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex_);
        registered_names_.push_back(std::string(name));
    }

    std::string envKeyFor(std::string_view name) const {
        return toEnvKey(name);
    }

private:
    std::string toEnvKey(std::string_view name) const {
        std::string key = prefix_ + std::string(name);
        for (char& c : key) {
            if (c == '.' || c == '-') {
                c = '_';
            } else {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
        }
        return key;
    }

    std::string prefix_;
    mutable std::mutex mutex_;
    std::vector<std::string> registered_names_;
};

} // namespace concerns
} // namespace core
} // namespace themis
