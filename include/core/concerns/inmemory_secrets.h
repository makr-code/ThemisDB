/**
 * @file inmemory_secrets.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Thread-safe in-memory secrets provider.
 *
 * Suitable for unit tests, single-process deployments, and loading
 * credentials from configuration files at startup.
 *
 * The secret store is mutable at runtime: call setSecret() to add or
 * update a value, and removeSecret() to revoke one. All values are stored
 * as plain strings; callers are responsible for protecting the process
 * address space (e.g. via OS-level memory locking when needed).
 *
 * Thread-safety: all public methods are guarded by an internal mutex.
 */
class InMemorySecrets : public ISecrets {
public:
    InMemorySecrets() = default;

    /**
     * @brief Construct with a pre-populated set of secrets.
     *
     * @param initial  Map of secret name → secret value.
     */
    explicit InMemorySecrets(std::map<std::string, std::string> initial)
        : secrets_(std::move(initial)) {}

    // -----------------------------------------------------------------------
    // ISecrets interface
    // -----------------------------------------------------------------------

    std::optional<std::string> getSecret(std::string_view name) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = secrets_.find(std::string(name));
        if (it == secrets_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool hasSecret(std::string_view name) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.count(std::string(name)) > 0;
    }

    std::vector<std::string> listSecretNames() const override {
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

    // -----------------------------------------------------------------------
    // Mutable operations (not part of ISecrets)
    // -----------------------------------------------------------------------

    /**
     * @brief Add or replace a secret.
     *
     * @param name   Secret name.
     * @param value  Secret value.
     */
    void setSecret(std::string_view name, std::string_view value) {
        std::lock_guard<std::mutex> lock(mutex_);
        secrets_[std::string(name)] = std::string(value);
    }

    /**
     * @brief Remove a secret from the store.
     *
     * No-op if the secret does not exist.
     *
     * @param name  Secret name to remove.
     * @return true if the secret was found and removed, false otherwise.
     */
    bool removeSecret(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.erase(std::string(name)) > 0;
    }

    /**
     * @brief Return the number of secrets currently stored.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.size();
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::string> secrets_;
};

// ---------------------------------------------------------------------------

/**
 * @brief Secrets provider that reads credentials from environment variables.
 *
 * Each secret name is mapped to an environment variable by:
 *   1. Prepending the configured prefix (default: "THEMIS_SECRET_").
 *   2. Converting the secret name to upper-case.
 *   3. Replacing every dot ('.') and dash ('-') with an underscore ('_').
 *
 * Examples (prefix = "THEMIS_SECRET_"):
 *   getSecret("db.password")   → reads THEMIS_SECRET_DB_PASSWORD
 *   getSecret("api.key.stripe")→ reads THEMIS_SECRET_API_KEY_STRIPE
 *   getSecret("redis-url")     → reads THEMIS_SECRET_REDIS_URL
 *
 * listSecretNames() returns the names that were registered via
 * registerName(). Since POSIX does not provide a portable enumeration API
 * for environment variables, enumeration is opt-in.
 *
 * Thread-safety: getSecret() and hasSecret() call ::getenv() which is
 * not thread-safe on all platforms if the environment is mutated concurrently.
 * In typical server applications the environment is read-only after startup,
 * making this safe in practice. The registered-names list is mutex-protected.
 */
class EnvSecretsProvider : public ISecrets {
public:
    /**
     * @brief Construct with a custom env-var prefix.
     *
    * @param prefix Prefix prepended to every secret name before looking up
    *               the environment variable. Default: "THEMIS_SECRET_".
     */
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

    /**
     * @brief Return the registered secret names that are currently available.
     *
    * Only names explicitly registered via registerName() are returned.
    * Names without a matching environment variable are silently excluded.
    * Duplicate registrations are preserved and may therefore appear more
    * than once in the returned vector.
     */
    std::vector<std::string> listSecretNames() const override {
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

    // -----------------------------------------------------------------------
    // Registration (not part of ISecrets)
    // -----------------------------------------------------------------------

    /**
     * @brief Register a secret name so it appears in listSecretNames().
     *
     * Duplicate names are allowed and are returned as duplicates by
     * listSecretNames() when the corresponding environment variable exists.
     *
     * @param name The logical secret name (e.g. "db.password").
     */
    void registerName(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex_);
        registered_names_.push_back(std::string(name));
    }

    /**
     * @brief Return the env-var key for a given secret name (for diagnostics).
     */
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
