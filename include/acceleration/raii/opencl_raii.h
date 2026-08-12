/**
 * @file opencl_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// RAII wrappers for OpenCL resources
// Provides automatic resource cleanup and exception safety
// Header-only implementation for ease of use

#ifdef THEMIS_ENABLE_OPENCL

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <stdexcept>
#include <string>

namespace themis {
namespace acceleration {
namespace raii {

// ============================================================================
// OpenCL Context RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for OpenCL context (cl_context).
///
/// Manages the lifetime of an OpenCL compute context. Automatically releases
/// the context on scope exit (exception-safe RAII).
///
/// Features:
/// - Ownership semantics: can own or wrap existing contexts.
/// - Reference counting: respects OpenCL's retain/release mechanism.
/// - Move semantics: efficient transfer of context ownership.
/// - Non-copyable: prevents accidental context duplication.
/// - Exception-safe: context is released even during unwinding.
///
/// Example usage:
/// ```cpp
/// cl_context raw_ctx = clCreateContext(...);
/// OpenCLContext ctx(raw_ctx, true);  // Takes ownership
/// // Context automatically released on scope exit
/// ```
///
/// @see OpenCLQueue, OpenCLProgram for related OpenCL resource wrappers.
class OpenCLContext {
public:
    /// @brief Default constructor; does not own a context.
    OpenCLContext() : context_(nullptr) {}
    
    /// @brief Construct from an existing OpenCL context.
    /// @param context The OpenCL context handle.
    /// @param owned If true, retains and will release the context; if false, wraps it without ownership.
    /// @throws std::runtime_error if context is invalid.
    explicit OpenCLContext(cl_context context, bool owned = true) 
        : context_(context), owned_(owned) {
        if (context_ && owned_) {
            clRetainContext(context_);
        }
    }
    
    // Non-copyable
    OpenCLContext(const OpenCLContext&) = delete;
    OpenCLContext& operator=(const OpenCLContext&) = delete;
    
    /// @brief Move constructor; transfers context ownership.
    OpenCLContext(OpenCLContext&& other) noexcept 
        : context_(other.context_), owned_(other.owned_) {
        other.context_ = nullptr;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers context ownership.
    OpenCLContext& operator=(OpenCLContext&& other) noexcept {
        if (this != &other) {
            destroy();
            context_ = other.context_;
            owned_ = other.owned_;
            other.context_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    /// @brief Destructor; releases the context if owned.
    ~OpenCLContext() {
        destroy();
    }
    
    /// @brief Create a new OpenCL context from devices.
    /// @param properties Optional context properties (platform, etc.); nullptr for defaults.
    /// @param numDevices Number of devices in the @p devices array.
    /// @param devices Array of device IDs to include in the context.
    /// @throws std::runtime_error if context creation fails.
    void create(const cl_context_properties* properties,
                cl_uint numDevices,
                const cl_device_id* devices) {
        if (context_) {
            destroy();
        }
        
        cl_int err;
        context_ = clCreateContext(properties, numDevices, devices, nullptr, nullptr, &err);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                std::string("Failed to create OpenCL context: error code ") + 
                std::to_string(err)
            );
        }
        owned_ = true;
    }
    
    /// @brief Check if the context is valid and ready for use.
    /// @return true if a valid context is owned; false otherwise.
    bool valid() const { return context_ != nullptr; }
    
    /// @brief Get the underlying OpenCL context handle.
    /// @return The cl_context handle; nullptr if not initialized.
    cl_context get() const { return context_; }
    
    /// @brief Release ownership of the context without releasing it.
    /// @return The OpenCL context handle.
    /// @note After calling release(), the caller is responsible for calling clReleaseContext().
    cl_context release() {
        owned_ = false;
        cl_context tmp = context_;
        context_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (context_ && owned_) {
            clReleaseContext(context_);
        }
        context_ = nullptr;
        owned_ = false;
    }
    
    cl_context context_;
    bool owned_ = false;
};

// ============================================================================
// OpenCL Command Queue RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for OpenCL command queue (cl_command_queue).
///
/// Manages the lifetime of an OpenCL command queue. Automatically releases
/// the queue on scope exit (exception-safe RAII).
///
/// Features:
/// - Ownership semantics: can own or wrap existing queues.
/// - Reference counting: respects OpenCL's retain/release mechanism.
/// - Move semantics: efficient transfer of queue ownership.
/// - Non-copyable: prevents accidental queue duplication.
/// - Queue synchronization: finish() waits for all pending commands.
///
/// Example usage:
/// ```cpp
/// OpenCLContext ctx = ...;
/// cl_device_id device = ...;
/// OpenCLQueue queue;
/// queue.create(ctx.get(), device);
/// queue.finish();  // Wait for all commands
/// // Queue automatically released on scope exit
/// ```
///
/// @see OpenCLContext, OpenCLProgram for related OpenCL resource wrappers.
class OpenCLQueue {
public:
    /// @brief Default constructor; does not own a queue.
    OpenCLQueue() : queue_(nullptr), owned_(false) {}
    
    /// @brief Construct from an existing OpenCL queue.
    /// @param queue The OpenCL command queue handle.
    /// @param owned If true, retains and will release the queue; if false, wraps it without ownership.
    explicit OpenCLQueue(cl_command_queue queue, bool owned = true) 
        : queue_(queue), owned_(owned) {
        if (queue_ && owned_) {
            clRetainCommandQueue(queue_);
        }
    }
    
    // Non-copyable
    OpenCLQueue(const OpenCLQueue&) = delete;
    OpenCLQueue& operator=(const OpenCLQueue&) = delete;
    
    /// @brief Move constructor; transfers queue ownership.
    OpenCLQueue(OpenCLQueue&& other) noexcept 
        : queue_(other.queue_), owned_(other.owned_) {
        other.queue_ = nullptr;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers queue ownership.
    OpenCLQueue& operator=(OpenCLQueue&& other) noexcept {
        if (this != &other) {
            destroy();
            queue_ = other.queue_;
            owned_ = other.owned_;
            other.queue_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    /// @brief Destructor; releases the queue if owned.
    ~OpenCLQueue() {
        destroy();
    }
    
    /// @brief Create a new command queue for a device and context.
    /// @param context The OpenCL context.
    /// @param device The device to create the queue for.
    /// @param properties Optional command queue properties (e.g., profiling).
    /// @throws std::runtime_error if queue creation fails.
    void create(cl_context context, cl_device_id device, cl_command_queue_properties properties = 0) {
        if (queue_) {
            destroy();
        }
        
        cl_int err;
        queue_ = clCreateCommandQueue(context, device, properties, &err);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                std::string("Failed to create OpenCL command queue: error code ") + 
                std::to_string(err)
            );
        }
        owned_ = true;
    }
    
    /// @brief Block until all queued commands complete.
    /// @throws std::runtime_error if synchronization fails.
    /// @note This is a blocking call; it waits for all pending operations.
    void finish() {
        if (queue_) {
            cl_int err = clFinish(queue_);
            if (err != CL_SUCCESS) {
                throw std::runtime_error(
                    std::string("OpenCL queue finish failed: error code ") + 
                    std::to_string(err)
                );
            }
        }
    }
    
    /// @brief Check if the queue is valid and ready for use.
    /// @return true if a valid queue is owned; false otherwise.
    bool valid() const { return queue_ != nullptr; }
    
    /// @brief Get the underlying OpenCL queue handle.
    /// @return The cl_command_queue handle; nullptr if not initialized.
    cl_command_queue get() const { return queue_; }
    
    /// @brief Release ownership of the queue without releasing it.
    /// @return The OpenCL queue handle.
    /// @note After calling release(), the caller is responsible for calling clReleaseCommandQueue().
    cl_command_queue release() {
        owned_ = false;
        cl_command_queue tmp = queue_;
        queue_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (queue_ && owned_) {
            clReleaseCommandQueue(queue_);
        }
        queue_ = nullptr;
        owned_ = false;
    }
    
    cl_command_queue queue_;
    bool owned_;
};

// ============================================================================
// OpenCL Program RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for OpenCL program (cl_program).
///
/// Manages the lifetime of an OpenCL compute program (compiled or compiled+linked).
/// Automatically releases the program on scope exit (exception-safe RAII).
///
/// Features:
/// - Ownership semantics: can own or wrap existing programs.
/// - Reference counting: respects OpenCL's retain/release mechanism.
/// - Move semantics: efficient transfer of program ownership.
/// - Non-copyable: prevents accidental program duplication.
/// - Program building: build() compiles the program for one or more devices.
///
/// Example usage:
/// ```cpp
/// OpenCLContext ctx = ...;
/// OpenCLProgram prog;
/// prog.createWithSource(ctx.get(), kernel_source);
/// prog.build(1, &device);
/// // Program automatically released on scope exit
/// ```
///
/// @see OpenCLKernel for extracting kernels from compiled programs.
class OpenCLProgram {
public:
    /// @brief Default constructor; does not own a program.
    OpenCLProgram() : program_(nullptr), owned_(false) {}
    
    /// @brief Construct from an existing OpenCL program.
    /// @param program The OpenCL program handle.
    /// @param owned If true, retains and will release the program; if false, wraps it without ownership.
    explicit OpenCLProgram(cl_program program, bool owned = true) 
        : program_(program), owned_(owned) {
        if (program_ && owned_) {
            clRetainProgram(program_);
        }
    }
    
    // Non-copyable
    OpenCLProgram(const OpenCLProgram&) = delete;
    OpenCLProgram& operator=(const OpenCLProgram&) = delete;
    
    /// @brief Move constructor; transfers program ownership.
    OpenCLProgram(OpenCLProgram&& other) noexcept 
        : program_(other.program_), owned_(other.owned_) {
        other.program_ = nullptr;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers program ownership.
    OpenCLProgram& operator=(OpenCLProgram&& other) noexcept {
        if (this != &other) {
            destroy();
            program_ = other.program_;
            owned_ = other.owned_;
            other.program_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    /// @brief Destructor; releases the program if owned.
    ~OpenCLProgram() {
        destroy();
    }
    
    /// @brief Create a program from source code.
    /// @param context The OpenCL context.
    /// @param source Null-terminated C string containing the kernel source.
    /// @throws std::runtime_error if program creation fails.
    void createWithSource(cl_context context, const char* source) {
        if (program_) {
            destroy();
        }
        
        size_t sourceSize = strlen(source);
        cl_int err;
        program_ = clCreateProgramWithSource(context, 1, &source, &sourceSize, &err);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                std::string("Failed to create OpenCL program: error code ") + 
                std::to_string(err)
            );
        }
        owned_ = true;
    }
    
    /// @brief Compile and link the program for target devices.
    /// @param numDevices Number of devices in the @p devices array.
    /// @param devices Array of device IDs to compile for.
    /// @param options Optional compiler options (e.g., "-cl-mad-enable").
    /// @throws std::runtime_error if compilation or linking fails.
    void build(cl_uint numDevices, const cl_device_id* devices, const char* options = nullptr) {
        if (!program_) {
            throw std::runtime_error("Cannot build uninitialized OpenCL program");
        }
        
        cl_int err = clBuildProgram(program_, numDevices, devices, options, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                std::string("Failed to build OpenCL program: error code ") + 
                std::to_string(err)
            );
        }
    }
    
    /// @brief Check if the program is valid and ready for use.
    /// @return true if a valid program is owned; false otherwise.
    bool valid() const { return program_ != nullptr; }
    
    /// @brief Get the underlying OpenCL program handle.
    /// @return The cl_program handle; nullptr if not initialized.
    cl_program get() const { return program_; }
    
    /// @brief Release ownership of the program without releasing it.
    /// @return The OpenCL program handle.
    /// @note After calling release(), the caller is responsible for calling clReleaseProgram().
    cl_program release() {
        owned_ = false;
        cl_program tmp = program_;
        program_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (program_ && owned_) {
            clReleaseProgram(program_);
        }
        program_ = nullptr;
        owned_ = false;
    }
    
    cl_program program_;
    bool owned_;
};

// ============================================================================
// OpenCL Kernel RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for OpenCL kernel (cl_kernel).
///
/// Manages the lifetime of an OpenCL kernel function extracted from a compiled
/// program. Automatically releases the kernel on scope exit (exception-safe RAII).
///
/// Features:
/// - Ownership semantics: can own or wrap existing kernels.
/// - Reference counting: respects OpenCL's retain/release mechanism.
/// - Move semantics: efficient transfer of kernel ownership.
/// - Non-copyable: prevents accidental kernel duplication.
///
/// Example usage:
/// ```cpp
/// OpenCLProgram prog = ...;
/// OpenCLKernel kern;
/// kern.create(prog.get(), "my_kernel_name");
/// // Kernel automatically released on scope exit
/// ```
///
/// @see OpenCLProgram for creating kernels from programs.
class OpenCLKernel {
public:
    /// @brief Default constructor; does not own a kernel.
    OpenCLKernel() : kernel_(nullptr), owned_(false) {}
    
    /// @brief Construct from an existing OpenCL kernel.
    /// @param kernel The OpenCL kernel handle.
    /// @param owned If true, retains and will release the kernel; if false, wraps it without ownership.
    explicit OpenCLKernel(cl_kernel kernel, bool owned = true) 
        : kernel_(kernel), owned_(owned) {
        if (kernel_ && owned_) {
            clRetainKernel(kernel_);
        }
    }
    
    // Non-copyable
    OpenCLKernel(const OpenCLKernel&) = delete;
    OpenCLKernel& operator=(const OpenCLKernel&) = delete;
    
    /// @brief Move constructor; transfers kernel ownership.
    OpenCLKernel(OpenCLKernel&& other) noexcept 
        : kernel_(other.kernel_), owned_(other.owned_) {
        other.kernel_ = nullptr;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers kernel ownership.
    OpenCLKernel& operator=(OpenCLKernel&& other) noexcept {
        if (this != &other) {
            destroy();
            kernel_ = other.kernel_;
            owned_ = other.owned_;
            other.kernel_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    /// @brief Destructor; releases the kernel if owned.
    ~OpenCLKernel() {
        destroy();
    }
    
    /// @brief Create a kernel from a compiled program.
    /// @param program The OpenCL program (must be compiled/linked).
    /// @param kernelName The name of the kernel function (null-terminated).
    /// @throws std::runtime_error if the kernel is not found or creation fails.
    void create(cl_program program, const char* kernelName) {
        if (kernel_) {
            destroy();
        }
        
        cl_int err;
        kernel_ = clCreateKernel(program, kernelName, &err);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                std::string("Failed to create OpenCL kernel '") + kernelName + 
                "': error code " + std::to_string(err)
            );
        }
        owned_ = true;
    }
    
    /// @brief Check if the kernel is valid and ready for use.
    /// @return true if a valid kernel is owned; false otherwise.
    bool valid() const { return kernel_ != nullptr; }
    
    /// @brief Get the underlying OpenCL kernel handle.
    /// @return The cl_kernel handle; nullptr if not initialized.
    cl_kernel get() const { return kernel_; }
    
    /// @brief Release ownership of the kernel without releasing it.
    /// @return The OpenCL kernel handle.
    /// @note After calling release(), the caller is responsible for calling clReleaseKernel().
    cl_kernel release() {
        owned_ = false;
        cl_kernel tmp = kernel_;
        kernel_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (kernel_ && owned_) {
            clReleaseKernel(kernel_);
        }
        kernel_ = nullptr;
        owned_ = false;
    }
    
    cl_kernel kernel_;
    bool owned_;
};

// ============================================================================
// OpenCL Buffer RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for OpenCL buffer memory (cl_mem).
///
/// Manages the lifetime of an OpenCL device memory buffer. Automatically
/// releases the buffer on scope exit (exception-safe RAII).
///
/// Features:
/// - Ownership semantics: can own or wrap existing buffers.
/// - Reference counting: respects OpenCL's retain/release mechanism.
/// - Move semantics: efficient transfer of buffer ownership.
/// - Non-copyable: prevents accidental buffer duplication.
/// - Size tracking: remembers the allocated size for validation.
///
/// Example usage:
/// ```cpp
/// OpenCLContext ctx = ...;
/// OpenCLBuffer buf;
/// buf.create(ctx.get(), CL_MEM_READ_WRITE, 4096);
/// // Buffer automatically released on scope exit
/// ```
///
/// @see OpenCLContext, OpenCLQueue for buffer usage context.
class OpenCLBuffer {
public:
    /// @brief Default constructor; does not own a buffer.
    OpenCLBuffer() : buffer_(nullptr), size_(0), owned_(false) {}
    
    /// @brief Construct from an existing OpenCL buffer.
    /// @param buffer The OpenCL buffer handle.
    /// @param size The allocated size in bytes.
    /// @param owned If true, retains and will release the buffer; if false, wraps it without ownership.
    explicit OpenCLBuffer(cl_mem buffer, size_t size, bool owned = true) 
        : buffer_(buffer), size_(size), owned_(owned) {
        if (buffer_ && owned_) {
            clRetainMemObject(buffer_);
        }
    }
    
    // Non-copyable
    OpenCLBuffer(const OpenCLBuffer&) = delete;
    OpenCLBuffer& operator=(const OpenCLBuffer&) = delete;
    
    /// @brief Move constructor; transfers buffer ownership.
    OpenCLBuffer(OpenCLBuffer&& other) noexcept 
        : buffer_(other.buffer_), size_(other.size_), owned_(other.owned_) {
        other.buffer_ = nullptr;
        other.size_ = 0;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers buffer ownership.
    OpenCLBuffer& operator=(OpenCLBuffer&& other) noexcept {
        if (this != &other) {
            destroy();
            buffer_ = other.buffer_;
            size_ = other.size_;
            owned_ = other.owned_;
            other.buffer_ = nullptr;
            other.size_ = 0;
            other.owned_ = false;
        }
        return *this;
    }
    
    /// @brief Destructor; releases the buffer if owned.
    ~OpenCLBuffer() {
        destroy();
    }
    
    /// @brief Create a new device buffer with specified flags and optional host data.
    /// @param context The OpenCL context.
    /// @param flags Memory allocation flags (e.g., CL_MEM_READ_WRITE, CL_MEM_COPY_HOST_PTR).
    /// @param size The size in bytes to allocate.
    /// @param hostPtr Optional host pointer for CL_MEM_COPY_HOST_PTR (default: nullptr).
    /// @throws std::runtime_error if buffer creation fails.
    void create(cl_context context, cl_mem_flags flags, size_t size, void* hostPtr = nullptr) {
        if (buffer_) {
            destroy();
        }
        
        if (size == 0) {
            return;
        }
        
        cl_int err;
        buffer_ = clCreateBuffer(context, flags, size, hostPtr, &err);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                std::string("Failed to create OpenCL buffer (") + 
                std::to_string(size) + " bytes): error code " + 
                std::to_string(err)
            );
        }
        size_ = size;
        owned_ = true;
    }
    
    /// @brief Check if the buffer is valid and ready for use.
    /// @return true if a valid buffer is allocated; false otherwise.
    bool valid() const { return buffer_ != nullptr; }
    
    /// @brief Get the underlying OpenCL buffer handle.
    /// @return The cl_mem buffer handle; nullptr if not initialized.
    cl_mem get() const { return buffer_; }
    
    /// @brief Get the allocated buffer size in bytes.
    /// @return The size of the buffer; 0 if unallocated.
    size_t size() const { return size_; }
    
    /// @brief Release ownership of the buffer without releasing it.
    /// @return The OpenCL buffer handle.
    /// @note After calling release(), the caller is responsible for calling clReleaseMemObject().
    cl_mem release() {
        owned_ = false;
        cl_mem tmp = buffer_;
        buffer_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
private:
    void destroy() {
        if (buffer_ && owned_) {
            clReleaseMemObject(buffer_);
        }
        buffer_ = nullptr;
        size_ = 0;
        owned_ = false;
    }
    
    cl_mem buffer_;
    size_t size_;
    bool owned_;
};

} // namespace raii
} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_OPENCL
