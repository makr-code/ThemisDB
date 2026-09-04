/**
 * @file i_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Abstract interface for request/call-chain context propagation.
 *
 * `IContext` carries arbitrary key/value metadata across call boundaries so
 * that a single originating request is traceable through every layer of the
 * system without polluting function signatures.
 *
 * ### Well-known attribute keys (use `context_keys::k*` constants)
 *
 * | Key constant   | Meaning                               |
 * |----------------|---------------------------------------|
 * | `kTraceId`     | OpenTelemetry trace-id (hex string)   |
 * | `kRequestId`   | Per-request / per-RPC correlation id  |
 * | `kUserId`      | Authenticated user identifier         |
 * | `kTenantId`    | Multi-tenant partition identifier     |
 * | `kOperation`   | Logical operation name                |
 * | `kService`     | Originating service name              |
 * | `kSessionId`   | Session identifier                    |
 *
 * ### Lifecycle
 *
 * - A root context is created at the entry point of a request
 *   (e.g. HTTP handler, gRPC stub).
 * - Child contexts are created for sub-operations via `createChild()`;
 *   children inherit all parent attributes and may override them locally.
 * - Contexts are immutable with respect to inherited attributes; a child
 *   can only *add* or *shadow* keys, never modify the parent.
 * - createChild() preserves the visible attribute set at lookup time, but the
 *   parent chain remains live so later parent updates are visible unless a key
 *   is shadowed locally.
 *
 * ### Integration with `ILogger`
 *
 * Use `toTraceContext()` to extract a `TraceContext` suitable for
 * `ILogger::logWithContext()`:
 * @code
 *   auto ctx = SimpleContext::create();
 *   ctx->set(context_keys::kTraceId,   "abc123");
 *   ctx->set(context_keys::kRequestId, "req-42");
 *
 *   logger.logWithContext(ILogger::Level::INFO, "Processing request",
 *                         ctx->toTraceContext(), {{"db.table", "users"}});
 * @endcode
 *
 * ### Thread Safety
 *
 * Implementations MUST be thread-safe: `get()` and `set()` may be called
 * from multiple threads concurrently.
 */
class IContext {
public:
    virtual ~IContext() = default;

    // -----------------------------------------------------------------------
    // Attribute access
    // -----------------------------------------------------------------------

    /**
     * @brief Set (or replace) an attribute on this context.
     *
     * The change is local to this context and does not propagate back to
        * the parent. Keys are compared by string value, not by pointer identity.
     *
     * @param key   Attribute name (use a `context_keys::k*` constant where
     *              applicable to prevent typos).
     * @param value Attribute value.
     */
    virtual void set(std::string_view key, std::string_view value) = 0;

    /**
     * @brief Retrieve an attribute by key.
     *
     * Lookup walks up the parent chain: if the key is not found in this
        * context, the parent is queried recursively. A returned std::string is a
        * copy so callers may keep it independently of the context lifetime.
     *
     * @param key Attribute name.
     * @return The attribute value if found, `std::nullopt` otherwise.
     */
    [[nodiscard]] virtual std::optional<std::string> get(std::string_view key) const = 0;

    /**
     * @brief Return true if the attribute exists in this context or any parent.
     * @param key Attribute name.
     * @return true when `get(key).has_value()`.
     */
    [[nodiscard]] virtual bool has(std::string_view key) const = 0;

    // -----------------------------------------------------------------------
    // Child context creation
    // -----------------------------------------------------------------------

    /**
     * @brief Create a child context that inherits this context's attributes.
     *
     * The child shares read-access to all parent attributes via the lookup
     * chain but writes to its own attribute store so the parent is never
    * mutated. Child creation must preserve thread safety and must not expose
    * partially constructed state to other threads.
     *
     * @return A new `IContext` whose parent is *this.
     */
    [[nodiscard]] virtual IContextPtr createChild() const = 0;

    // -----------------------------------------------------------------------
    // Bridge to existing logging API
    // -----------------------------------------------------------------------

    /**
     * @brief Extract a `TraceContext` for use with `ILogger::logWithContext()`.
     *
     * Reads `context_keys::kTraceId` and `context_keys::kRequestId` from
    * this context (including inherited values) and populates a `TraceContext`
    * that can be passed directly to `ILogger::logWithContext()`. Span-id is
    * also copied when present so nested span logging can retain correlation.
     *
     * @return A `TraceContext` with trace_id and request_id populated
     *         (empty strings if the corresponding attributes are absent).
     */
    [[nodiscard]] virtual TraceContext toTraceContext() const = 0;
};

// ---------------------------------------------------------------------------
// Well-known attribute-key constants
// ---------------------------------------------------------------------------

/**
 * @namespace themis::core::concerns::context_keys
 * @brief Predefined attribute-name constants for the `IContext` interface.
 *
 * Using these constants instead of raw string literals prevents typos and
 * makes attribute names refactorable:
 * @code
 *   ctx->set(context_keys::kTraceId, "abc123");
 *   auto tid = ctx->get(context_keys::kTraceId);
 * @endcode
 */
namespace context_keys {

/// OpenTelemetry trace ID (hex string, e.g. "a3b2c1...").
inline constexpr std::string_view kTraceId   = "trace_id";
/// Active span ID (16-character hex string, e.g. "00f067aa0ba902b7").
inline constexpr std::string_view kSpanId    = "span_id";
/// Per-request / per-RPC correlation identifier.
inline constexpr std::string_view kRequestId = "request_id";
/// Authenticated user identifier.
inline constexpr std::string_view kUserId    = "user_id";
/// Multi-tenant partition identifier.
inline constexpr std::string_view kTenantId  = "tenant_id";
/// Logical operation name (e.g. "db.query", "http.handler").
inline constexpr std::string_view kOperation = "operation";
/// Service name that originated the context.
inline constexpr std::string_view kService   = "service";
/// Session identifier (e.g. authenticated session cookie hash).
inline constexpr std::string_view kSessionId = "session_id";

} // namespace context_keys

// ---------------------------------------------------------------------------
// SimpleContext -- default concrete implementation
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe, map-backed implementation of `IContext`.
 *
 * `SimpleContext` uses a `std::unordered_map` for O(1) attribute access and
 * a `shared_ptr` to its parent for the lookup chain.  All methods are
 * thread-safe via an internal mutex.
 *
 * **SimpleContext must always be managed by a `shared_ptr`.**  Use the
 * `SimpleContext::create()` factory instead of constructing directly.
 *
 * ### Creating a root context
 * @code
 *   auto ctx = SimpleContext::create();
 *   ctx->set(context_keys::kTraceId,   "abc123");
 *   ctx->set(context_keys::kRequestId, "req-42");
 * @endcode
 *
 * ### Creating a child context
 * @code
 *   auto child = ctx->createChild();
 *   child->set(context_keys::kOperation, "db.select");
 *   // child->get(context_keys::kTraceId) still returns "abc123" via parent
 * @endcode
 */
class SimpleContext
    : public IContext
    , public std::enable_shared_from_this<SimpleContext>
{
public:
    /**
     * @brief Factory: create an empty root context with no parent.
     * @return A new root `SimpleContext` managed by a `shared_ptr`.
     */
    static std::shared_ptr<SimpleContext> create() {
        return std::shared_ptr<SimpleContext>(new SimpleContext(nullptr));
    }

    /**
    * @brief Factory: create a root context pre-populated with the two most
    *        common correlation attributes.
     *
     * @param trace_id   OpenTelemetry trace-id to set (ignored if empty).
     * @param request_id Request correlation id to set (ignored if empty).
     * @return A new root `SimpleContext` with the supplied attributes set.
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
        std::lock_guard<std::mutex> lk(mutex_);
        attrs_[std::string(key)] = std::string(value);
    }

    std::optional<std::string> get(std::string_view key) const override {
        {
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

    /**
     * @brief Create a child context that inherits this context's attributes.
     *
     * The returned context holds a `shared_ptr` to *this so the parent
     * stays alive for at least as long as any child.
     *
     * @return A new `SimpleContext` child.
     */
    IContextPtr createChild() const override {
        // shared_from_this() is safe here because SimpleContext must always
        // be managed by a shared_ptr (created via create()).
        return IContextPtr(new SimpleContext(
            std::const_pointer_cast<SimpleContext>(shared_from_this())));
    }

    TraceContext toTraceContext() const override {
        TraceContext tc;
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
    explicit SimpleContext(std::shared_ptr<SimpleContext> parent)
        : parent_(std::move(parent)) {}

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> attrs_;
    std::shared_ptr<SimpleContext> parent_;
};

} // namespace concerns
} // namespace core
} // namespace themis
