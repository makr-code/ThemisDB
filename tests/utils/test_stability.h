#pragma once

/**
 * @file test_stability.h
 * @brief Determinism and stability helpers for ThemisDB test suites.
 *
 * Provides lightweight utilities that eliminate blind sleeps (flaky timing
 * races) from test code:
 *
 *   - themis::test::wait_for_clock_advance_ms()
 *       Spins with yield() until the system_clock millisecond value has
 *       advanced by at least one unit.  Replaces blind sleep_for() calls
 *       that exist solely to ensure wall-clock timestamps differ between
 *       consecutive operations.  No artificial delay is added; the spin
 *       terminates as soon as the OS scheduler reports a new millisecond.
 *
 *   - themis::test::poll_until()
 *       Deadline-bounded busy-poll over an arbitrary predicate.  Replaces
 *       fixed-duration sleeps that guard asynchronous side-effects (e.g.
 *       waiting for background file writes to appear on disk).  The caller
 *       provides the predicate and a timeout; the function returns true if
 *       the predicate became true before the deadline.
 *
 * ## Design Notes
 *
 * Both helpers use `std::this_thread::yield()` rather than
 * `std::this_thread::sleep_for()` during the spin.  `yield()` relinquishes
 * the current time-slice but does not insert a hard lower-bound delay,
 * keeping test runtimes fast on lightly loaded machines while remaining
 * correct on heavily loaded CI agents.
 *
 * Neither helper introduces new sleeps as synchronisation.  They are purely
 * condition-based waits that happen to use the wall clock as the condition.
 *
 * ## Usage
 *
 * @code
 * #include "utils/test_stability.h"
 *
 * // Ensure successive snapshot IDs (millisecond-based) are distinct:
 * themis::test::wait_for_clock_advance_ms();
 * auto id = snapshot_mgr.createSnapshot(...);
 *
 * // Wait for an async background write (bounded at 5 s):
 * bool appeared = themis::test::poll_until(
 *     []{ return std::filesystem::exists("/tmp/result.dat"); },
 *     std::chrono::seconds(5));
 * EXPECT_TRUE(appeared) << "Background write did not complete within 5 s";
 * @endcode
 *
 * @note Thread Safety: These are free functions with no shared state.
 *       They are safe to call from multiple threads simultaneously.
 */

#include <chrono>
#include <functional>
#include <thread>

namespace themis {
namespace test {

/**
 * @brief Spin-wait until the system_clock millisecond counter advances by ≥1.
 *
 * Call this function when the code under test derives a unique identifier
 * or ordering guarantee from the current wall-clock millisecond (e.g. a
 * snapshot ID generated as `duration_cast<milliseconds>(now()).count()`).
 * Using this helper instead of `sleep_for(N ms)` eliminates the blind delay:
 * on fast machines the spin completes in < 1 ms; on slow CI agents it waits
 * exactly as long as needed.
 *
 * @note The function has a built-in safety timeout of 500 ms.  If the OS
 *       clock fails to advance within that window (highly unusual), the
 *       function returns without a guarantee — the calling test will then
 *       naturally fail or pass on its own assertions.
 */
inline void wait_for_clock_advance_ms() {
    using Clock = std::chrono::system_clock;
    using Ms = std::chrono::milliseconds;

    const auto t0_ms = std::chrono::duration_cast<Ms>(
        Clock::now().time_since_epoch()).count();
    const auto safety_deadline = std::chrono::steady_clock::now()
                                 + std::chrono::milliseconds(500);

    while (std::chrono::duration_cast<Ms>(
               Clock::now().time_since_epoch()).count() == t0_ms
           && std::chrono::steady_clock::now() < safety_deadline) {
        std::this_thread::yield();
    }
}

/**
 * @brief Deadline-bounded poll for an arbitrary predicate.
 *
 * Calls @p predicate repeatedly (with yield between calls) until either the
 * predicate returns true or the @p timeout elapses.  Replaces fixed-duration
 * `sleep_for()` calls that guard asynchronous side-effects.
 *
 * @param predicate  Callable returning bool; must be side-effect-free in
 *                   terms of test state (can be called multiple times).
 * @param timeout    Maximum time to wait.  A value of zero means a single
 *                   evaluation with no waiting — the predicate is called
 *                   exactly once and the result is returned immediately.
 * @return true  if @p predicate returned true before the deadline.
 * @return false if the deadline elapsed without the predicate becoming true.
 */
inline bool poll_until(
    const std::function<bool()>& predicate,
    std::chrono::steady_clock::duration timeout = std::chrono::seconds(5))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        if (predicate()) {
          return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
          return false;
        }
        std::this_thread::yield();
    }
}

} // namespace test
} // namespace themis
