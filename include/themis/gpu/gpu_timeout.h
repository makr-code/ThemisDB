#ifndef THEMIS_GPU_TIMEOUT_H_
#define THEMIS_GPU_TIMEOUT_H_

// gpu_timeout.h
// ASCII-sanitized Kernel SLA enforcement header
// Restored minimal documentation and the KernelSLAGuard API.

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace gpu {

// Enforces a maximum kernel execution time and provides diagnostic helpers.
// Default SLA: 5 seconds.
class KernelSLAGuard {
 public:
  static constexpr std::chrono::seconds DEFAULT_SLA_DURATION{5};

  explicit KernelSLAGuard(
      std::chrono::steady_clock::duration timeout_duration = DEFAULT_SLA_DURATION) noexcept
      : start_time_(std::chrono::steady_clock::now()),
        deadline_(start_time_ + timeout_duration),
        timeout_duration_(timeout_duration) {}

  KernelSLAGuard(const KernelSLAGuard&) = delete;
  KernelSLAGuard& operator=(const KernelSLAGuard&) = delete;

  KernelSLAGuard(KernelSLAGuard&& other) noexcept
      : start_time_(other.start_time_),
        deadline_(other.deadline_),
        timeout_duration_(other.timeout_duration_) {}

  KernelSLAGuard& operator=(KernelSLAGuard&& other) noexcept {
    if (this != &other) {
      start_time_ = other.start_time_;
      deadline_ = other.deadline_;
      timeout_duration_ = other.timeout_duration_;
    }
    return *this;
  }

  ~KernelSLAGuard() noexcept = default;

  bool checkTimeoutDeadline() const noexcept {
    auto now = std::chrono::steady_clock::now();
    return now >= deadline_;
  }

  std::chrono::steady_clock::duration getElapsedTime() const noexcept {
    return std::chrono::steady_clock::now() - start_time_;
  }

  std::chrono::steady_clock::duration getRemainingTime() const noexcept {
    return deadline_ - std::chrono::steady_clock::now();
  }

  std::chrono::steady_clock::duration getSLADuration() const noexcept {
    return timeout_duration_;
  }

  std::chrono::steady_clock::time_point getDeadline() const noexcept { return deadline_; }
  std::chrono::steady_clock::time_point getStartTime() const noexcept { return start_time_; }

 private:
  std::chrono::steady_clock::time_point start_time_;
  std::chrono::steady_clock::time_point deadline_;
  std::chrono::steady_clock::duration timeout_duration_;
};

}  // namespace gpu
}  // namespace themis

#endif  // THEMIS_GPU_TIMEOUT_H_
