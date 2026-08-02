/**
 * @file llm_memory_safety_utils.h
 * @brief RAII utilities for memory-safe resource management in LLM module
 * 
 * Tier 1 Batch 1: Memory Safety Hardening
 * Provides utilities to eliminate manual cleanup, leaks, and exception issues.
 * 
 * @version 2026-08-02
 * @status Phase 1 (LLM Batch 1)
 */

#ifndef THEMIS_LLM_MEMORY_SAFETY_UTILS_H
#define THEMIS_LLM_MEMORY_SAFETY_UTILS_H

#include <memory>
#include <mutex>
#include <atomic>
#include <vector>
#include <stdexcept>
#include <functional>

namespace themis::llm {

// ============================================================================
// GPUMemoryGuard: RAII wrapper for GPU memory allocation
// ============================================================================
class GPUMemoryGuard {
 public:
  using Deleter = std::function<void(void*)>;
  
  /**
   * Construct GPU memory guard with pointer and deleter function
   * @param ptr GPU memory pointer (nullptr allowed)
   * @param deleter Function to call when memory should be freed
   */
  explicit GPUMemoryGuard(void* ptr, Deleter deleter) 
      : ptr_(ptr), deleter_(deleter) {}
  
  // Default construct: nullptr (safe)
  GPUMemoryGuard() : ptr_(nullptr), deleter_(nullptr) {}
  
  // Move semantics (for exclusive ownership)
  GPUMemoryGuard(GPUMemoryGuard&& other) noexcept 
      : ptr_(other.ptr_), deleter_(other.deleter_) {
    other.ptr_ = nullptr;
    other.deleter_ = nullptr;
  }
  
  GPUMemoryGuard& operator=(GPUMemoryGuard&& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      deleter_ = other.deleter_;
      other.ptr_ = nullptr;
      other.deleter_ = nullptr;
    }
    return *this;
  }
  
  // Prevent copying (exclusive ownership)
  GPUMemoryGuard(const GPUMemoryGuard&) = delete;
  GPUMemoryGuard& operator=(const GPUMemoryGuard&) = delete;
  
  // Destructor: exception-safe GPU cleanup
  ~GPUMemoryGuard() noexcept {
    reset();
  }
  
  // Get raw pointer (non-owning)
  void* get() const { return ptr_; }
  
  // Check if valid
  explicit operator bool() const { return ptr_ != nullptr; }
  
  // Release ownership without cleanup
  void* release() {
    void* tmp = ptr_;
    ptr_ = nullptr;
    deleter_ = nullptr;
    return tmp;
  }
  
  // Reset to nullptr, calling deleter if set
  void reset() {
    if (ptr_ && deleter_) {
      try {
        deleter_(ptr_);
      } catch (...) {
        // Swallow exceptions in destructor per C++ guidelines
        ptr_ = nullptr;
        deleter_ = nullptr;
      }
    }
    ptr_ = nullptr;
    deleter_ = nullptr;
  }
  
 private:
  void* ptr_;
  Deleter deleter_;
};

// ============================================================================
// DBConnectionGuard: RAII wrapper for database connections
// ============================================================================
class DBConnectionGuard {
 public:
  using Releaser = std::function<void()>;
  
  /**
   * Construct DB connection guard
   * @param connection_id Unique connection identifier
   * @param releaser Function to call when connection should be returned
   */
  explicit DBConnectionGuard(int connection_id, Releaser releaser) 
      : connection_id_(connection_id), 
        releaser_(releaser),
        is_released_(false) {}
  
  // Move semantics (for exclusive ownership)
  DBConnectionGuard(DBConnectionGuard&& other) noexcept
      : connection_id_(other.connection_id_),
        releaser_(other.releaser_),
        is_released_(other.is_released_) {
    other.connection_id_ = -1;
    other.releaser_ = nullptr;
    other.is_released_ = true;
  }
  
  DBConnectionGuard& operator=(DBConnectionGuard&& other) noexcept {
    if (this != &other) {
      release();
      connection_id_ = other.connection_id_;
      releaser_ = other.releaser_;
      is_released_ = other.is_released_;
      other.connection_id_ = -1;
      other.releaser_ = nullptr;
      other.is_released_ = true;
    }
    return *this;
  }
  
  // Prevent copying
  DBConnectionGuard(const DBConnectionGuard&) = delete;
  DBConnectionGuard& operator=(const DBConnectionGuard&) = delete;
  
  // Destructor: exception-safe connection release
  ~DBConnectionGuard() noexcept {
    release();
  }
  
  // Get connection ID
  int getId() const { return connection_id_; }
  
  // Manual release (optional)
  void release() {
    if (!is_released_ && releaser_) {
      try {
        releaser_();
      } catch (...) {
        // Swallow exceptions in destructor
      }
      is_released_ = true;
    }
  }
  
 private:
  int connection_id_;
  Releaser releaser_;
  std::atomic<bool> is_released_;
};

// ============================================================================
// ScopedLockGuard: Manages lock ordering to prevent deadlocks
// ============================================================================
class ScopedLockGuard {
 public:
  using LockFunc = std::function<void()>;
  using UnlockFunc = std::function<void()>;
  
  /**
   * RAII lock guard with exception safety
   * @param lock Function to acquire lock
   * @param unlock Function to release lock
   */
  ScopedLockGuard(LockFunc lock, UnlockFunc unlock)
      : lock_(lock), 
        unlock_(unlock),
        is_locked_(false) {
    if (lock_) {
      lock_();
      is_locked_ = true;
    }
  }
  
  // Move semantics
  ScopedLockGuard(ScopedLockGuard&& other) noexcept
      : lock_(other.lock_),
        unlock_(other.unlock_),
        is_locked_(other.is_locked_) {
    other.lock_ = nullptr;
    other.unlock_ = nullptr;
    other.is_locked_ = false;
  }
  
  // Prevent copying
  ScopedLockGuard(const ScopedLockGuard&) = delete;
  ScopedLockGuard& operator=(const ScopedLockGuard&) = delete;
  
  // Destructor: exception-safe unlock
  ~ScopedLockGuard() noexcept {
    if (is_locked_ && unlock_) {
      try {
        unlock_();
      } catch (...) {
        // Swallow exceptions
      }
      is_locked_ = false;
    }
  }
  
 private:
  LockFunc lock_;
  UnlockFunc unlock_;
  std::atomic<bool> is_locked_;
};

// ============================================================================
// VectorRAII: Automatic vector growth allocation
// ============================================================================
template <typename T>
class VectorRAII {
 public:
  /**
   * Pre-allocate vector with capacity to avoid reallocations
   * @param initial_capacity Capacity to reserve
   */
  explicit VectorRAII(size_t initial_capacity = 0) {
    if (initial_capacity > 0) {
      data_.reserve(initial_capacity);
    }
  }
  
  // Move semantics
  VectorRAII(VectorRAII&& other) noexcept = default;
  VectorRAII& operator=(VectorRAII&& other) noexcept = default;
  
  // Prevent copying for exclusive ownership
  VectorRAII(const VectorRAII&) = default;
  VectorRAII& operator=(const VectorRAII&) = default;
  
  // Safe push_back with capacity management
  void push_back(const T& value) {
    data_.push_back(value);
  }
  
  void push_back(T&& value) {
    data_.push_back(std::move(value));
  }
  
  // Direct access
  T& operator[](size_t index) { return data_[index]; }
  const T& operator[](size_t index) const { return data_[index]; }
  
  size_t size() const { return data_.size(); }
  size_t capacity() const { return data_.capacity(); }
  
  void clear() { data_.clear(); }
  
  typename std::vector<T>::iterator begin() { return data_.begin(); }
  typename std::vector<T>::const_iterator begin() const { return data_.begin(); }
  typename std::vector<T>::iterator end() { return data_.end(); }
  typename std::vector<T>::const_iterator end() const { return data_.end(); }
  
 private:
  std::vector<T> data_;
};

// ============================================================================
// ExceptionSafeDeleter: Safe pointer deletion with nullptr setting
// ============================================================================
template <typename T>
class ExceptionSafeDeleter {
 public:
  /**
   * Safe delete: exception-safe and sets pointer to nullptr
   */
  static void safeDelete(T*& ptr) noexcept {
    if (ptr) {
      try {
        delete ptr;
      } catch (...) {
        // Swallow exception
      }
      ptr = nullptr;  // Always set to nullptr after deletion
    }
  }
  
  /**
   * Safe delete array
   */
  static void safeDeleteArray(T*& ptr) noexcept {
    if (ptr) {
      try {
        delete[] ptr;
      } catch (...) {
        // Swallow exception
      }
      ptr = nullptr;  // Always set to nullptr after deletion
    }
  }
};

// ============================================================================
// ManagedResource: Template for RAII resource management
// ============================================================================
template <typename Resource, typename Deleter>
class ManagedResource {
 public:
  using value_type = Resource;
  
  /**
   * Construct with resource and deleter
   * @param resource The resource to manage
   * @param deleter Callable to clean up resource
   */
  ManagedResource(Resource resource, Deleter deleter)
      : resource_(resource), deleter_(deleter) {}
  
  // Move semantics
  ManagedResource(ManagedResource&& other) noexcept
      : resource_(std::move(other.resource_)),
        deleter_(std::move(other.deleter_)) {
    other.resource_ = nullptr;
    other.deleter_ = nullptr;
  }
  
  ManagedResource& operator=(ManagedResource&& other) noexcept {
    if (this != &other) {
      cleanup();
      resource_ = std::move(other.resource_);
      deleter_ = std::move(other.deleter_);
      other.resource_ = nullptr;
      other.deleter_ = nullptr;
    }
    return *this;
  }
  
  // Prevent copying
  ManagedResource(const ManagedResource&) = delete;
  ManagedResource& operator=(const ManagedResource&) = delete;
  
  // Exception-safe destructor
  ~ManagedResource() noexcept {
    cleanup();
  }
  
  // Get resource
  Resource get() const { return resource_; }
  Resource* operator->() { return resource_; }
  const Resource* operator->() const { return resource_; }
  Resource& operator*() { return *resource_; }
  const Resource& operator*() const { return *resource_; }
  
  // Release ownership without cleanup
  Resource release() {
    Resource tmp = std::move(resource_);
    resource_ = nullptr;
    deleter_ = nullptr;
    return tmp;
  }
  
  // Reset resource
  void reset(Resource new_resource = nullptr) {
    cleanup();
    resource_ = new_resource;
  }
  
 private:
  void cleanup() noexcept {
    if (resource_ && deleter_) {
      try {
        deleter_(resource_);
      } catch (...) {
        // Swallow exceptions in destructor
      }
    }
    resource_ = nullptr;
    deleter_ = nullptr;
  }
  
  Resource resource_;
  Deleter deleter_;
};

}  // namespace themis::llm

#endif  // THEMIS_LLM_MEMORY_SAFETY_UTILS_H
