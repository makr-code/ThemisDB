/**
 * @file context_propagation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_context.h"
#include <future>
#include <memory>
#include <utility>

namespace themis {
namespace core {
namespace concerns {

// Forward declaration so ContextPropagation can declare ContextScope as friend.
class ContextScope;

class ContextPropagation {
public:
    static IContextPtr current() noexcept {
        return current_;
    }

    template <typename Fn>
    /**
     * @brief Propagate.
     * @param[in] fn Input parameter.
     * @return Return value.
     */
    static auto propagate(Fn&& fn) -> std::future<std::invoke_result_t<Fn>>;

    // Non-instantiable static utility class.
    ContextPropagation() = delete;

private:
    friend class ContextScope;

    static thread_local IContextPtr current_;
};

// ---------------------------------------------------------------------------
// ContextScope — defined before propagate() body so it can be used in the
// template implementation below.
// ---------------------------------------------------------------------------

class ContextScope {
public:
    explicit ContextScope(IContextPtr ctx) noexcept
        : previous_(ContextPropagation::current_)
    {
        ContextPropagation::current_ = std::move(ctx);
    }

    ~ContextScope() noexcept {
        ContextPropagation::current_ = std::move(previous_);
    }

    // Not copyable or movable — a scope guard must stay on one thread's stack.
    ContextScope(const ContextScope&)            = delete;
    ContextScope& operator=(const ContextScope&) = delete;
    ContextScope(ContextScope&&)                 = delete;
    ContextScope& operator=(ContextScope&&)      = delete;

private:
    IContextPtr previous_;
};

// ---------------------------------------------------------------------------
// ContextPropagation::propagate() — out-of-line template body
// (must appear after ContextScope is fully defined)
// ---------------------------------------------------------------------------

template <typename Fn>
/**
 * @brief Propagate.
 * @param[in] fn Input parameter.
 * @return Return value.
 */
auto ContextPropagation::propagate(Fn&& fn)
    -> std::future<std::invoke_result_t<Fn>>
{
    // Capture the caller's context; create a child so the async task can
    // add its own attributes without modifying the caller's context.
    IContextPtr parent = current_;
    IContextPtr child  = parent ? parent->createChild() : nullptr;

    return std::async(std::launch::async,
        [ctx = std::move(child), f = std::forward<Fn>(fn)]() mutable
            -> std::invoke_result_t<Fn>
        {
            /**
             * @brief Install the propagated context for the duration of this task.
             * @param[in] ctx Input parameter.
             * @return Return value.
             */
            ContextScope scope(ctx);
            return f();
        });
}

} // namespace concerns
} // namespace core
} // namespace themis
