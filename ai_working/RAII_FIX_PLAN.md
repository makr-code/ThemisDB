# RAII and Resource Management Fix Plan for LLM Module

## Summary
Fixing critical resource management gaps in 4 key LLM files focusing on RAII patterns, exception safety, and GPU memory management.

## Critical Files to Fix

### 1. gguf_loader.cpp (GPU Memory & File Handling)
**Issues:**
- Raw file descriptor and mmap without RAII wrapper
- Potential resource leak if parseFile() fails mid-operation
- Database pointer is raw (dependency injection)

**Fixes:**
- Create RAII wrapper for file descriptor (fd_)
- Create RAII wrapper for mmap region
- Add exception-safe transaction guard around parseFile()
- Add proper nullptr checks in all operations
- Document resource ownership contracts

### 2. llm_plugin_manager.cpp (Resource Cleanup)
**Issues:**
- Already using std::unique_ptr (good), but needs exception guarantees
- State store cleanup could leak on exception
- Add noexcept specifications to destructor and key methods

**Fixes:**
- Ensure exception-safe cleanup in destructor
- Add noexcept(true) to destructor
- Validate state_store lifecycle management
- Add proper nullptr checks in all operations

### 3. adaptive_vram_allocator.cpp (VRAM Management)
**Issues:**
- Impl_ uses std::make_unique (good)
- Need to ensure all calculations don't throw
- Add exception-safety guarantees

**Fixes:**
- Mark methods with noexcept where appropriate
- Add bounds checking for all allocations
- Ensure exception-safe state transitions

### 4. model_loader.cpp (Model Lifecycle)
**Issues:**
- CachedModel destructor needs exception-safety
- Async loading could leak on exception
- Futures need proper cleanup on cancellation

**Fixes:**
- Make destructor noexcept(true)
- Add exception-safe async cleanup
- Document cancellation token handling
- Add proper nullptr checks

## Implementation Strategy

1. Add RAII helper wrappers where needed
2. Convert all manual cleanup to exception-safe patterns
3. Add comprehensive nullptr checks
4. Document resource ownership in Doxygen comments
5. Add noexcept specifications based on exception-safety guarantees
6. Test exception paths

## Expected Impact

- Eliminate 108 resource_leaked_in_exception issues
- Eliminate 192 db_connection_leak issues
- Eliminate 10 gpu_memory_leak issues
- Eliminate 44 manual_cleanup issues
- Eliminate 12 delete_without_nullptr issues
- Eliminate 59 null_dereference issues
- Eliminate 7 memory_order issues

## Testing Approach

- Verify all resource cleanup paths
- Test exception throwing scenarios
- Validate no resource leaks under stress
- Check thread-safety with sanitizers
