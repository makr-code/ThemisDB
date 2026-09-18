/**
 * @file i_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_logger.h"  // for TraceContext
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace core {
namespace concerns {

class IContext;
using IContextPtr = std::shared_ptr<IContext>;

class IContext {
public:
    /**
     * @brief IContext.
     * @return Return value.
     */
    virtual ~IContext() = default;

    // -----------------------------------------------------------------------
    // Attribute access
    // -----------------------------------------------------------------------

    /**
     * @brief Set.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     */
    virtual void set(std::string_view key, std::string_view value) = 0;

    [[nodiscard]] virtual std::optional<std::string> get(std::string_view key) const = 0;

    [[nodiscard]] virtual bool has(std::string_view key) const = 0;

    // -----------------------------------------------------------------------
    // Child context creation
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual IContextPtr createChild() const = 0;

    // -----------------------------------------------------------------------
    // Bridge to existing logging API
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual TraceContext toTraceContext() const = 0;
};

// ---------------------------------------------------------------------------
// Well-known attribute-key constants
// ---------------------------------------------------------------------------

namespace context_keys {

inline constexpr std::string_view kTraceId   = "trace_id";
inline constexpr std::string_view kSpanId    = "span_id";
inline constexpr std::string_view kRequestId = "request_id";
inline constexpr std::string_view kUserId    = "user_id";
inline constexpr std::string_view kTenantId  = "tenant_id";
inline constexpr std::string_view kOperation = "operation";
inline constexpr std::string_view kService   = "service";
inline constexpr std::string_view kSessionId = "session_id";

} // namespace context_keys

// ---------------------------------------------------------------------------
// SimpleContext -- default concrete implementation
// ---------------------------------------------------------------------------

class SimpleContext
    : public IContext
    , public std::enable_shared_from_this<SimpleContext>
{
public:
    /**
     * @brief Create.
     * @return Return value.
     * @details Calls: SimpleContext().
     */
    static std::shared_ptr<SimpleContext> create() {
        return std::shared_ptr<SimpleContext>(new SimpleContext(nullptr));
    }

    /**
     * @brief Create.
     * @param[in] trace_id Identifier of the trace.
     * @param[in] request_id Identifier of the request.
     * @return Return value.
     */
    static std::shared_ptr<SimpleContext> create(
        std::string_view trace_id, std::string_view request_id)
    {
        auto ctx = create();
        if (!trace_id.empty()) {
          ctx->set(context_keys::kTraceId,   trace_id);
        }
        if (!request_id.empty()) {
          ctx->set(context_keys::kRequestId, request_id);
        }
        return ctx;
    }

    // IContext implementation

    void set(std::string_view key, std::string_view value) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mutex_);
        attrs_[std::string(key)] = std::string(value);
    }

    std::optional<std::string> get(std::string_view key) const override {
        {
            /**
             * @brief Lk.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lk(mutex_);
            auto it = attrs_.find(std::string(key));
            if (it != attrs_.end()) {
              return it->second;
            }
        }
        // Walk up the parent chain without holding our lock.
        if (parent_) {
          return parent_->get(key);
        }
        return std::nullopt;
    }

    bool has(std::string_view key) const override {
        return get(key).has_value();
    }

    IContextPtr createChild() const override {
        // shared_from_this() is safe here because SimpleContext must always
        // be managed by a shared_ptr (created via create()).
        return IContextPtr(new SimpleContext(
            std::const_pointer_cast<SimpleContext>(shared_from_this())));
    }

    TraceContext toTraceContext() const override {
        TraceContext tc = {};
        if (auto v = get(context_keys::kTraceId)) {
          tc.trace_id   = *v;
        }
        if (auto v = get(context_keys::kSpanId)) {
          tc.span_id    = *v;
        }
        if (auto v = get(context_keys::kRequestId)) {
          tc.request_id = *v;
        }
        return tc;
    }

private:
    /**
     * @brief Simple Context.
     * @param[in] parent Input parameter.
     * @return Return value.
     */
    explicit SimpleContext(std::shared_ptr<SimpleContext> parent)
        : parent_(std::move(parent)) {}

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> attrs_;
    std::shared_ptr<SimpleContext> parent_;
};

} // namespace concerns
} // namespace core
} // namespace themis
