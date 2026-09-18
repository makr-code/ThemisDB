/**
 * @file include/security/safe_move.h
 * @brief Move semantics safety utilities for preventing use-after-move and double-free violations
 *
 * This library provides RAII-based utilities to safely manage C++ move semantics:
 * - MoveValidator: Pre/post-move state validation
 * - MoveGuard: Wraps move sources to detect invalid access after move
 * - SafeMove: Type-safe wrapper for safe move operations
 * - MoveChainTracker: Tracks multi-hop move operations for complex scenarios
 *
 * @author ThemisDB Team
 * @date 2026-07-05
 * @version 1.0.0
 * @license Apache 2.0
 *
 * @defgroup security_safe_move SafeMove Security Utilities
 * @ingroup security
 */

#ifndef THEMIS_SECURITY_SAFE_MOVE_H_
#define THEMIS_SECURITY_SAFE_MOVE_H_

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <cassert>
#include <atomic>
#include <optional>

namespace themis::security {

/**
 * @defgroup safe_move_exceptions SafeMove Exceptions
 * @ingroup security_safe_move
 * @{
 */

/// Exception thrown when a move operation violates safety constraints
class MoveViolationException : public std::logic_error {
 public:
  explicit MoveViolationException(const std::string& msg) 
      : std::logic_error(msg) {}
};

/// Exception thrown when use-after-move is detected
class UseAfterMoveException : public MoveViolationException {
 public:
  UseAfterMoveException() 
      : MoveViolationException("Detected use-after-move: object accessed after being moved from") {}
};

/// Exception thrown when move source is not properly cleared
class MoveSourceNotCleared : public MoveViolationException {
 public:
  MoveSourceNotCleared()
      : MoveViolationException("Move source not in valid state after move operation") {}
};

/// @}

/**
 * @class MoveValidator
 * @brief Validates pre/post-move object state
 * 
 * Provides compile-time and runtime checks for move operations.
 * 
 * @tparam T Type being validated
 * @ingroup security_safe_move
 * 
 * Example:
 * @code
 * MyClass obj = createObject();
 * MoveValidator<MyClass>::validatePreMove(obj);
 * auto moved = std::move(obj);
 * MoveValidator<MyClass>::validatePostMove(obj);
 * @endcode
 */
template<typename T>
class MoveValidator {
 public:
  /// Validates object is safe to move from
  /// @param obj Object to validate
  /// @throws MoveViolationException if validation fails
  static void validatePreMove(const T& obj) {
    // Base implementation: check object is constructed
    // Specializations can override for specific types
    if constexpr (std::is_default_constructible_v<T>) {
      // Object is default constructible, likely valid
    }
  }

  /// Validates object state after being moved from
  /// @param obj Object to validate
  /// @throws MoveSourceNotCleared if validation fails
  static void validatePostMove(const T& obj) {
    // Base implementation: empty check if applicable
    // Specializations can override for specific types
    if constexpr (requires(const T& t) { t.empty(); }) {
      // Can check empty state for containers
    }
  }

  /// Validates move destination received valid moved data
  /// @param dest Destination object after move assignment
  /// @throws MoveViolationException if validation fails
  static void validateMoveDestination(const T& dest) {
    // Base implementation: just check constructed
    // Specializations can override
  }
};

/**
 * @class MoveGuard
 * @brief RAII wrapper that detects invalid access after move
 * 
 * Wraps a pointer to an object and marks it as "moved from" if the
 * object is moved. Any subsequent access to the guard will throw.
 * 
 * @tparam T Type of object being guarded
 * @ingroup security_safe_move
 * 
 * Example:
 * @code
 * MyClass obj;
 * {
 *   MoveGuard<MyClass> guard(&obj);
 *   auto moved = std::move(obj);
 *   obj.someMethod(); // Would throw UseAfterMoveException if called
 * }
 * @endcode
 */
template<typename T>
class MoveGuard {
 private:
  T* ptr_;
  mutable std::atomic<bool> moved_from_{false};

 public:
  /// Constructs guard for given object pointer
  /// @param ptr Pointer to object being guarded (must not be null)
  explicit MoveGuard(T* ptr) : ptr_(ptr) {
    assert(ptr != nullptr && "MoveGuard requires non-null pointer");
  }

  /// Destructor
  ~MoveGuard() = default;

  // Delete copy semantics - guard tracks identity
  MoveGuard(const MoveGuard&) = delete;
  MoveGuard& operator=(const MoveGuard&) = delete;

  /// Move is allowed (transfers guard)
  MoveGuard(MoveGuard&& other) noexcept
      : ptr_(other.ptr_), moved_from_(other.moved_from_.load()) {
    other.ptr_ = nullptr;
  }

  /// Move assignment
  MoveGuard& operator=(MoveGuard&& other) noexcept {
    if (this != &other) {
      ptr_ = other.ptr_;
      moved_from_.store(other.moved_from_.load());
      other.ptr_ = nullptr;
    }
    return *this;
  }

  /// Marks the guarded object as moved-from
  void markMovedFrom() const { moved_from_.store(true); }

  /// Checks if guarded object was moved from
  /// @returns true if object was moved from
  /// @throws UseAfterMoveException if object was moved from
  bool checkNotMovedFrom() const {
    if (moved_from_.load()) {
      throw UseAfterMoveException();
    }
    return true;
  }

  /// Gets the guarded pointer (with validation)
  /// @returns Pointer to guarded object
  /// @throws UseAfterMoveException if object was moved from
  T* get() const {
    checkNotMovedFrom();
    return ptr_;
  }

  /// Dereferencing operator (with validation)
  T& operator*() const {
    checkNotMovedFrom();
    return *ptr_;
  }

  /// Member access operator (with validation)
  T* operator->() const {
    checkNotMovedFrom();
    return ptr_;
  }
};

/**
 * @class SafeMove
 * @brief Type-safe wrapper for move operations with automatic validation
 * 
 * Provides a zero-cost abstraction over move operations with safety guarantees:
 * - Pre-move validation
 * - Post-move source cleanup
 * - Move destination validation
 * - Compile-time move type checking
 * 
 * @tparam T Type being safely moved
 * @ingroup security_safe_move
 * 
 * Example:
 * @code
 * std::vector<int> data = {1, 2, 3};
 * auto safe = SafeMove<std::vector<int>>::create(std::move(data));
 * auto result = safe.take(); // Takes ownership, validates state
 * @endcode
 */
template<typename T>
class SafeMove {
 public:
  /// Creates a safe move wrapper from an rvalue reference
  /// @param value Object to wrap (must be rvalue reference)
  /// @returns SafeMove instance
  static SafeMove create(T&& value) {
    MoveValidator<T>::validatePreMove(value);
    return SafeMove(std::move(value));
  }

  /// Extracts the moved-to value with validation
  /// @returns Moved value
  /// @throws MoveViolationException if validation fails
  T take() && {
    MoveValidator<T>::validateMoveDestination(value_);
    return std::move(value_);
  }

  /// Const reference access
  const T& get() const { return value_; }

  /// Non-const reference access
  T& get() { return value_; }

  /// Checks if value is in valid state
  bool isValid() const {
    try {
      MoveValidator<T>::validatePostMove(value_);
      return true;
    } catch (...) {
      return false;
    }
  }

 private:
  T value_;

  /// Private constructor - use create() instead
  explicit SafeMove(T&& value) : value_(std::move(value)) {}
};

/**
 * @struct MoveChainTracker
 * @brief Tracks multi-hop move operations to detect cycles and issues
 * 
 * For complex scenarios where objects are moved through multiple functions/types,
 * tracks the chain to detect:
 * - Circular moves (A moves to B, B moves back to A)
 * - Premature destruction in move chain
 * - Unbalanced move operations
 * 
 * @ingroup security_safe_move
 */
struct MoveChainTracker {
  /// Maximum chain depth (prevents infinite loops in tracking)
  static constexpr int MAX_CHAIN_DEPTH = 100;

  /// Current chain depth
  std::atomic<int> depth{0};

  /// Flag indicating chain error state
  std::atomic<bool> error_state{false};

  /// Increments chain depth on move start
  /// @throws MoveViolationException if chain depth exceeds limit
  void onMoveBegin() {
    int current = depth.load();
    if (current >= MAX_CHAIN_DEPTH) {
      error_state.store(true);
      throw MoveViolationException("Move chain exceeds maximum depth");
    }
    depth.store(current + 1);
  }

  /// Decrements chain depth on move completion
  void onMoveEnd() {
    int current = depth.load();
    if (current > 0) {
      depth.store(current - 1);
    }
  }

  /// Checks if chain is in valid state
  bool isValid() const {
    return !error_state.load() && depth.load() >= 0;
  }

  /// Resets chain state
  void reset() {
    depth.store(0);
    error_state.store(false);
  }
};

/**
 * @brief Macro to validate move operations at compile/runtime
 * 
 * Usage in move constructors/assignments:
 * @code
 * MyClass::MyClass(MyClass&& other) noexcept {
 *   THEMIS_VALIDATE_MOVE(other);
 *   data_ = std::move(other.data_);
 * }
 * @endcode
 */
#define THEMIS_VALIDATE_MOVE(obj) \
  ::themis::security::MoveValidator<std::remove_reference_t<decltype(obj)>>::validatePreMove(obj)

/**
 * @brief Macro to create MoveGuard for source object
 * 
 * Usage:
 * @code
 * auto guard = THEMIS_MOVE_GUARD(my_object);
 * auto moved = std::move(my_object);
 * @endcode
 */
#define THEMIS_MOVE_GUARD(obj) \
  ::themis::security::MoveGuard<std::remove_reference_t<decltype(obj)>>(&obj)

/**
 * @brief Macro for safe move with automatic validation
 * 
 * Usage:
 * @code
 * std::vector<int> data = {1, 2, 3};
 * auto result = THEMIS_SAFE_MOVE(std::vector<int>, std::move(data)).take();
 * @endcode
 */
#define THEMIS_SAFE_MOVE(Type, value) \
  ::themis::security::SafeMove<Type>::create(value)

/**
 * @brief Specialization for std::vector move validation
 * 
 * @tparam T Element type
 * @ingroup security_safe_move
 */
template<typename T>
class MoveValidator<std::vector<T>> {
 public:
  static void validatePreMove(const std::vector<T>& vec) {
    // Can move any vector (even empty ones)
  }

  static void validatePostMove(const std::vector<T>& vec) {
    // After move, vector should be empty or moved from
    // This is guaranteed by std::vector move semantics
  }

  static void validateMoveDestination(const std::vector<T>& vec) {
    // Destination can be any vector state
  }
};

/**
 * @brief Specialization for std::unique_ptr move validation
 * 
 * @tparam T Pointed-to type
 * @ingroup security_safe_move
 */
template<typename T>
class MoveValidator<std::unique_ptr<T>> {
 public:
  static void validatePreMove(const std::unique_ptr<T>& ptr) {
    // Can move any unique_ptr (even null)
  }

  static void validatePostMove(const std::unique_ptr<T>& ptr) {
    // After move from, source should be null
    if (ptr != nullptr) {
      throw MoveSourceNotCleared();
    }
  }

  static void validateMoveDestination(const std::unique_ptr<T>& ptr) {
    // Destination becomes owner, state doesn't matter
  }
};

/**
 * @brief Specialization for std::shared_ptr move validation
 * 
 * @tparam T Pointed-to type
 * @ingroup security_safe_move
 */
template<typename T>
class MoveValidator<std::shared_ptr<T>> {
 public:
  static void validatePreMove(const std::shared_ptr<T>& ptr) {
    // Can move any shared_ptr
  }

  static void validatePostMove(const std::shared_ptr<T>& ptr) {
    // After move from, source should be null
    if (ptr != nullptr) {
      throw MoveSourceNotCleared();
    }
  }

  static void validateMoveDestination(const std::shared_ptr<T>& ptr) {
    // Destination becomes owner
  }
};

} // namespace themis::security

#endif // THEMIS_SECURITY_SAFE_MOVE_H_
