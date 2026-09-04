/**
 * @file request_validation_middleware.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/request_validation_middleware.h"
#include "utils/input_validator.h"

#include <algorithm>
#include <cctype>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

/*static*/
std::string RequestValidationMiddleware::normalizeMethod(const std::string& method) {
    std::string upper(method);
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upper;
}

// ---------------------------------------------------------------------------
// schema registry
// ---------------------------------------------------------------------------

void RequestValidationMiddleware::registerSchema(const std::string& method,
                                                  const std::string& path,
                                                  nlohmann::json schema) {
    std::lock_guard<std::mutex> lock(mutex_);
    EndpointKey key{normalizeMethod(method), path};
    schemas_[std::move(key)] = std::move(schema);
}

bool RequestValidationMiddleware::removeSchema(const std::string& method,
                                                const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    EndpointKey key{normalizeMethod(method), path};
    return schemas_.erase(key) > 0;
}

void RequestValidationMiddleware::clearSchemas() {
    std::lock_guard<std::mutex> lock(mutex_);
    schemas_.clear();
}

size_t RequestValidationMiddleware::schemaCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(schemas_.size());
}

// ---------------------------------------------------------------------------
// lookup
// ---------------------------------------------------------------------------

const nlohmann::json* RequestValidationMiddleware::findSchemaLocked(
    const std::string& method,
    const std::string& path) const
{
    // Helper: find the best (longest-prefix) schema for a given method string.
    auto findForMethod = [&]([[maybe_unused]] const std::string& m) -> const nlohmann::json* {
        // 1. Exact match
        auto it = schemas_.find(EndpointKey{m, path});
        if (it != schemas_.end()) {
          return &it->second;
        }

        // 2. Longest prefix match
        const nlohmann::json* best = nullptr;
        size_t best_len = 0;
        for (const auto& [key, schema] : schemas_) {
            if (key.method != m) {
              continue;
            }
            const std::string& registered_path = key.path;
            if (registered_path.empty()) {
              continue;
            }
            // path must start with registered_path
            if (static_cast<int>(path.size()) >= registered_path.size() &&
                path.compare(0,static_cast<int>(registered_path.size()), registered_path) == 0) {
                // Ensure it's a proper prefix boundary:
                //   - exact match, OR
                //   - next char in request path is '/', OR
                //   - registered path ends with '/' (already encodes the separator)
                bool boundary = (static_cast<int>(path.size()) == static_cast<int>(registered_path.size())) ||
                                (path[static_cast<int>(registered_path.size())] == '/') ||
                                (registered_path.back() == '/');
                if (boundary && static_cast<int>(registered_path.size()) > best_len) {
                    best_len = registered_path.size();
                    best = &schema;
                }
            }
        }
        return best;
    };

    // Try specific method first, then wildcard
    if (auto* s = findForMethod(method)) {
      return s;
    }
    if (method != "*") {
        if (auto* s = findForMethod("*")) {
          return s;
        }
    }
    return nullptr;
}

bool RequestValidationMiddleware::hasSchema(const std::string& method,
                                             const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findSchemaLocked(normalizeMethod(method), path) != nullptr;
}

// ---------------------------------------------------------------------------
// core validation
// ---------------------------------------------------------------------------

/*static*/
RequestValidationMiddleware::ValidationResult
RequestValidationMiddleware::applySchema(const nlohmann::json& body,
                                          const nlohmann::json& schema) {
    auto err = utils::InputValidator::validateJson(body, schema);
    if (err.has_value()) {
        return ValidationResult::Error(std::move(*err));
    }
    return ValidationResult::OK();
}

RequestValidationMiddleware::ValidationResult
RequestValidationMiddleware::validate(const std::string& method,
                                       const std::string& path,
                                       const nlohmann::json& body) const {
    std::unique_lock<std::mutex> lock(mutex_);
    const nlohmann::json* schema = findSchemaLocked(normalizeMethod(method), path);
    if (!schema) {
        lock.unlock();
        metrics_.validation_skip_total.fetch_add(1, std::memory_order_relaxed);
        return ValidationResult::OK();
    }
    // Copy schema to release lock during validation (avoid holding mutex during CPU work)
    nlohmann::json schema_copy = *schema;
    lock.unlock();

    auto result = applySchema(body, schema_copy);
    if (result.valid) {
        metrics_.validation_pass_total.fetch_add(1, std::memory_order_relaxed);
    } else {
        metrics_.validation_fail_total.fetch_add(1, std::memory_order_relaxed);
    }
    return result;
}

RequestValidationMiddleware::ValidationResult
RequestValidationMiddleware::validate(const std::string& method,
                                       const std::string& path,
                                       const std::string& body) const {
    // Empty body is treated as an empty JSON object for required-field checking.
    nlohmann::json parsed;
    if (body.empty()) {
        // Only proceed if a schema is registered (avoid parse on skip path)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!findSchemaLocked(normalizeMethod(method), path)) {
                metrics_.validation_skip_total.fetch_add(1, std::memory_order_relaxed);
                return ValidationResult::OK();
            }
        }
        parsed = nlohmann::json::object();
    } else {
        try {
            parsed = nlohmann::json::parse(body);
        } catch (const nlohmann::json::exception&) {
            // Check if a schema is registered before counting parse error
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!findSchemaLocked(normalizeMethod(method), path)) {
                    metrics_.validation_skip_total.fetch_add(1, std::memory_order_relaxed);
                    return ValidationResult::OK();
                }
            }
            metrics_.parse_error_total.fetch_add(1, std::memory_order_relaxed);
            metrics_.validation_fail_total.fetch_add(1, std::memory_order_relaxed);
            return ValidationResult::Error("request body is not valid JSON");
        }
    }
    return validate(method, path, parsed);
}

} // namespace server
} // namespace themis
