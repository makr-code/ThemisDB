#pragma once
#include <functional>

namespace themis::llm {

/// @brief RAII wrapper for database connections acquired from a pool.
///        Releases the connection on destruction, even if an exception is thrown.
/// @tparam DbType  Type of the database connection object.
/// @tparam PoolType  Type of the connection pool or manager.
class ScopedDbConnection {
public:
    using ReleaseFunc = std::function<void()>;

    /// @brief Acquire a connection; @p release_fn is called on destruction.
    explicit ScopedDbConnection(ReleaseFunc release_fn) noexcept
        : release_fn_(std::move(release_fn)), released_(false) {}

    /// @brief Release the connection on destruction.
    ~ScopedDbConnection() noexcept { release(); }

    ScopedDbConnection(const ScopedDbConnection&) = delete;
    ScopedDbConnection& operator=(const ScopedDbConnection&) = delete;
    ScopedDbConnection(ScopedDbConnection&& other) noexcept
        : release_fn_(std::move(other.release_fn_)), released_(other.released_) {
        other.released_ = true;
    }

    /// @brief Explicitly release the connection before destructor.
    void release() noexcept {
        if (!released_ && release_fn_) {
            released_ = true;
            try { release_fn_(); } catch (...) {}
        }
    }

    /// @return True if the connection has been released.
    [[nodiscard]] bool isReleased() const noexcept { return released_; }

private:
    ReleaseFunc release_fn_;
    bool released_;
};

} // namespace themis::llm
