/**
 * @file safe_iterator.h
 * @brief Bounds-safe iterator wrapper to prevent out-of-range memory accesses.
 *
 * Wraps standard iterators with runtime bounds checks and clear
 * diagnostics, guarding against buffer-overrun vulnerabilities in
 * security-sensitive code paths.
 */

#ifndef THEMIS_SECURITY_SAFE_ITERATOR_H
#define THEMIS_SECURITY_SAFE_ITERATOR_H

#include <iterator>
#include <stdexcept>
#include <memory>
#include <type_traits>
#include <atomic>

// Conditional spdlog inclusion - only if available
#ifdef __has_include
  #if __has_include(<spdlog/spdlog.h>)
    #define THEMIS_HAS_SPDLOG 1
    #include <spdlog/spdlog.h>
  #endif
#endif

// Fallback: provide no-op logging if spdlog not available
#ifndef THEMIS_HAS_SPDLOG
namespace spdlog {
    template<typename... Args>
    inline void debug(const char*, Args&&...) {}
    
    template<typename... Args>
    inline void warn(const char*, Args&&...) {}
}
#endif

namespace themis::security {

namespace SafeIterator {

class InvalidationDetector {
public:
    template<typename Container>
    explicit InvalidationDetector(const Container& container, bool strict = false)
        : container_ptr_(&container),
          initial_size_(container.size()),
          strict_mode_(strict),
          modification_detected_(false) {
        spdlog::debug("InvalidationDetector initialized: container_ptr={}, size={}",
                      static_cast<const void*>(container_ptr_), initial_size_);
    }

    ~InvalidationDetector() = default;

    /**
     * @brief Check.
     * @return True when the operation succeeds.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: load().
     */
    bool check() {
        if (!container_ptr_) {
            throw std::runtime_error("InvalidationDetector: container pointer is null");
        }
        
        // The base detector relies on subclass-provided container tracking to
        // determine whether a modification was observed.
        return modification_detected_.load(std::memory_order_acquire);
    }

    /**
     * @brief Explicitly mark modification detection state.
     * @param[in] detected True to mark the container as modified.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: store(), spdlog::warn().
     */
    void set_modification_detected(bool detected) {
        modification_detected_.store(detected, std::memory_order_release);
        
        if (detected) {
            if (strict_mode_) {
                throw std::runtime_error("Container modification detected during iteration");
            } else {
                spdlog::warn("Container modification detected during iteration");
            }
        }
    }

    /**
     * @brief Reset the modification detection flag.
     * @details Calls: store().
     */
    void reset() {
        modification_detected_.store(false, std::memory_order_release);
    }

    size_t initial_size() const { return initial_size_; }

private:
    const void* container_ptr_;
    size_t initial_size_;
    bool strict_mode_;
    std::atomic<bool> modification_detected_;
};

class AdvanceSafe {
public:
    template<typename Iterator>
    /**
     * @brief Advance an iterator within the validated range.
     * @param[in,out] it Iterator to advance in place.
     * @param[in] distance Number of steps to advance.
     * @param[in] begin Beginning of the valid range.
     * @param[in] end End of the valid range.
     * @throws std::out_of_range if an error occurs.
     * @details Calls: constexpr(), std::distance(), std::to_string(), spdlog::debug().
     */
    static void advance(Iterator& it, typename std::iterator_traits<Iterator>::difference_type distance,
                       const Iterator& begin, const Iterator& end) {
        if (distance == 0) {
            return;  // No-op for zero distance
        }

        // For random-access iterators, compute distance and validate
        if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                     std::random_access_iterator_tag>) {
            auto total_distance = std::distance(begin, end);
            auto current_offset = std::distance(begin, it);

            if (current_offset < 0 || current_offset > total_distance) {
                throw std::out_of_range(
                    "AdvanceSafe: Iterator not in valid range [begin, end)");
            }

            auto new_offset = current_offset + distance;
            if (new_offset < 0 || new_offset > total_distance) {
                throw std::out_of_range(
                    "AdvanceSafe: Advance would move iterator outside [begin, end); "
                    "current_offset=" + std::to_string(current_offset) + ", "
                    "distance=" + std::to_string(distance) + ", "
                    "total_distance=" + std::to_string(total_distance));
            }

            std::advance(it, distance);
        } else {
            // For non-random-access iterators, we must advance step-by-step and check bounds
            if (distance > 0) {
                for (auto i = 0; i < distance; ++i) {
                    if (it == end) {
                        throw std::out_of_range(
                            "AdvanceSafe: Advance would move iterator past end()");
                    }
                    ++it;
                }
            } else {
                // Negative advance (only for bidirectional iterators)
                for (auto i = 0; i < -distance; ++i) {
                    if (it == begin) {
                        throw std::out_of_range(
                            "AdvanceSafe: Advance would move iterator before begin()");
                    }
                    --it;
                }
            }
        }

        spdlog::debug("AdvanceSafe: Iterator advanced by {} within bounds", distance);
    }

    template<typename Iterator>
    static bool can_advance(const Iterator& it, typename std::iterator_traits<Iterator>::difference_type distance,
                           const Iterator& begin, const Iterator& end) noexcept {
        if (distance == 0) {
            return true;
        }

        if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                     std::random_access_iterator_tag>) {
            try {
                auto total_distance = std::distance(begin, end);
                auto current_offset = std::distance(begin, it);

                if (current_offset < 0 || current_offset > total_distance) {
                    return false;
                }

                auto new_offset = current_offset + distance;
                return (new_offset >= 0 && new_offset <= total_distance);
            } catch (...) {
                return false;
            }
        }

        // For non-random-access iterators, we cannot validate without advancing
        return true;  // Optimistic default
    }
};

template<typename Iterator>
class RangeValidator {
public:
    /**
     * @brief Validate and store a safe iterator range.
     * @param[in] begin Beginning of the iterator range.
     * @param[in] end End of the iterator range.
     * @return Validated range object.
     */
    explicit RangeValidator(const Iterator& begin, const Iterator& end)
        : begin_(begin), end_(end), validated_(false) {
        
        // For random-access iterators, validate immediately
        if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                     std::random_access_iterator_tag>) {
            if (std::distance(begin, end) < 0) {
                throw std::invalid_argument(
                    "RangeValidator: begin > end (invalid range)");
            }
            validated_ = true;
        }
    }

    Iterator begin() const { return begin_; }

    Iterator end() const { return end_; }

    bool empty() const { return begin_ == end_; }

    size_t size() const {
        if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                     std::random_access_iterator_tag>) {
            auto dist = std::distance(begin_, end_);
            if (dist < 0) {
                throw std::logic_error(
                    "RangeValidator::size(): distance computation failed; "
                    "range may be invalid");
            }
            return static_cast<size_t>(dist);
        } else {
            throw std::runtime_error(
                "RangeValidator::size(): size() not available for non-random-access iterators");
        }
    }

private:
    Iterator begin_;
    Iterator end_;
    bool validated_;
};

class BoundsChecker {
public:
    template<typename Iterator>
    /**
     * @brief Check dereference.
     * @param[in] it Input parameter.
     * @param[in] begin Input parameter.
     * @param[in] end Input parameter.
     * @throws std::out_of_range if an error occurs.
     * @details Calls: constexpr(), std::distance(), spdlog::debug().
     */
    static void check_dereference(const Iterator& it, const Iterator& begin, const Iterator& end) {
        // For random-access iterators, we can do O(1) bounds checking
        if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                     std::random_access_iterator_tag>) {
            if (std::distance(begin, it) < 0 || std::distance(it, end) <= 0) {
                throw std::out_of_range(
                    "BoundsChecker: Iterator is out of bounds [begin, end)");
            }
        } else {
            // For non-random-access iterators, we can only check equality with end
            if (it == end) {
                throw std::out_of_range(
                    "BoundsChecker: Iterator is at end(); cannot dereference");
            }
        }

        spdlog::debug("BoundsChecker: Iterator dereference validated");
    }

    template<typename Iterator>
    /**
     * @brief Check access.
     * @param[in] it Input parameter.
     * @param[in] begin Input parameter.
     * @param[in] end Input parameter.
     * @throws std::out_of_range if an error occurs.
     * @details Calls: constexpr(), std::distance(), spdlog::debug().
     */
    static void check_access(const Iterator& it, const Iterator& begin, const Iterator& end) {
        if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                     std::random_access_iterator_tag>) {
            if (std::distance(begin, it) < 0 || std::distance(it, end) < 0) {
                throw std::out_of_range(
                    "BoundsChecker: Iterator is outside valid range [begin, end]");
            }
        } else {
            // For non-random-access iterators, only check if we can increment
            // This is a best-effort check
            (void)it;  // Iterator position unverifiable for non-random-access
        }

        spdlog::debug("BoundsChecker: Iterator access validated");
    }

    template<typename Iterator>
    static bool is_valid_for_dereference(const Iterator& it, const Iterator& begin,
                                        const Iterator& end) noexcept {
        try {
            if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                         std::random_access_iterator_tag>) {
                return std::distance(begin, it) >= 0 && std::distance(it, end) > 0;
            } else {
                return it != end;
            }
        } catch (...) {
            return false;
        }
    }

    template<typename Iterator>
    static bool is_valid_for_access(const Iterator& it, const Iterator& begin,
                                   const Iterator& end) noexcept {
        try {
            if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                                         std::random_access_iterator_tag>) {
                return std::distance(begin, it) >= 0 && std::distance(it, end) >= 0;
            } else {
                return true;  // No reliable check for non-random-access
            }
        } catch (...) {
            return false;
        }
    }
};

}  // namespace SafeIterator

}  // namespace themis::security

#endif  // THEMIS_SECURITY_SAFE_ITERATOR_H
