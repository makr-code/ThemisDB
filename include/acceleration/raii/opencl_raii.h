/**
 * @file opencl_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

class OpenCLContext {
public:
    OpenCLContext() : context_(nullptr) {}
    
    explicit OpenCLContext(cl_context context, bool owned = true) 
        : context_(context), owned_(owned) {
        if (context_ && owned_) {
            clRetainContext(context_);
        }
    }
    
    // Non-copyable
    OpenCLContext(const OpenCLContext&) = delete;
    OpenCLContext& operator=(const OpenCLContext&) = delete;
    
    OpenCLContext(OpenCLContext&& other) noexcept 
        : context_(other.context_), owned_(other.owned_) {
        other.context_ = nullptr;
        other.owned_ = false;
    }
    
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
    
    ~OpenCLContext() {
        destroy();
    }
    
    /**
     * @brief Create.
     * @param[in] properties Input parameter.
     * @param[in] numDevices Input parameter.
     * @param[in] devices Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: destroy(), clCreateContext(), std::string(), std::to_string().
     */
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
    
    bool valid() const { return context_ != nullptr; }
    
    cl_context get() const { return context_; }
    
    /**
     * @brief Release.
     * @return Return value.
     * @details Implements release without additional internal calls.
     */
    cl_context release() {
        owned_ = false;
        cl_context tmp = context_;
        context_ = nullptr;
        return tmp;
    }
    
private:
    /**
     * @brief Destroy.
     * @details Calls: clReleaseContext().
     */
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

class OpenCLQueue {
public:
    OpenCLQueue() : queue_(nullptr), owned_(false) {}
    
    explicit OpenCLQueue(cl_command_queue queue, bool owned = true) 
        : queue_(queue), owned_(owned) {
        if (queue_ && owned_) {
            clRetainCommandQueue(queue_);
        }
    }
    
    // Non-copyable
    OpenCLQueue(const OpenCLQueue&) = delete;
    OpenCLQueue& operator=(const OpenCLQueue&) = delete;
    
    OpenCLQueue(OpenCLQueue&& other) noexcept 
        : queue_(other.queue_), owned_(other.owned_) {
        other.queue_ = nullptr;
        other.owned_ = false;
    }
    
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
    
    ~OpenCLQueue() {
        destroy();
    }
    
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
    
    /**
     * @brief Finish.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: clFinish(), std::string(), std::to_string().
     */
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
    
    bool valid() const { return queue_ != nullptr; }
    
    cl_command_queue get() const { return queue_; }
    
    /**
     * @brief Release.
     * @return Return value.
     * @details Implements release without additional internal calls.
     */
    cl_command_queue release() {
        owned_ = false;
        cl_command_queue tmp = queue_;
        queue_ = nullptr;
        return tmp;
    }
    
private:
    /**
     * @brief Destroy.
     * @details Calls: clReleaseCommandQueue().
     */
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

class OpenCLProgram {
public:
    OpenCLProgram() : program_(nullptr), owned_(false) {}
    
    explicit OpenCLProgram(cl_program program, bool owned = true) 
        : program_(program), owned_(owned) {
        if (program_ && owned_) {
            clRetainProgram(program_);
        }
    }
    
    // Non-copyable
    OpenCLProgram(const OpenCLProgram&) = delete;
    OpenCLProgram& operator=(const OpenCLProgram&) = delete;
    
    OpenCLProgram(OpenCLProgram&& other) noexcept 
        : program_(other.program_), owned_(other.owned_) {
        other.program_ = nullptr;
        other.owned_ = false;
    }
    
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
    
    ~OpenCLProgram() {
        destroy();
    }
    
    /**
     * @brief Create With Source.
     * @param[in] context Input parameter.
     * @param[in] source Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: destroy(), strlen(), clCreateProgramWithSource(), std::string(), std::to_string().
     */
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
    
    bool valid() const { return program_ != nullptr; }
    
    cl_program get() const { return program_; }
    
    /**
     * @brief Release.
     * @return Return value.
     * @details Implements release without additional internal calls.
     */
    cl_program release() {
        owned_ = false;
        cl_program tmp = program_;
        program_ = nullptr;
        return tmp;
    }
    
private:
    /**
     * @brief Destroy.
     * @details Calls: clReleaseProgram().
     */
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

class OpenCLKernel {
public:
    OpenCLKernel() : kernel_(nullptr), owned_(false) {}
    
    explicit OpenCLKernel(cl_kernel kernel, bool owned = true) 
        : kernel_(kernel), owned_(owned) {
        if (kernel_ && owned_) {
            clRetainKernel(kernel_);
        }
    }
    
    // Non-copyable
    OpenCLKernel(const OpenCLKernel&) = delete;
    OpenCLKernel& operator=(const OpenCLKernel&) = delete;
    
    OpenCLKernel(OpenCLKernel&& other) noexcept 
        : kernel_(other.kernel_), owned_(other.owned_) {
        other.kernel_ = nullptr;
        other.owned_ = false;
    }
    
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
    
    ~OpenCLKernel() {
        destroy();
    }
    
    /**
     * @brief Create.
     * @param[in] program Input parameter.
     * @param[in] kernelName Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: destroy(), clCreateKernel(), std::string(), std::to_string().
     */
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
    
    bool valid() const { return kernel_ != nullptr; }
    
    cl_kernel get() const { return kernel_; }
    
    /**
     * @brief Release.
     * @return Return value.
     * @details Implements release without additional internal calls.
     */
    cl_kernel release() {
        owned_ = false;
        cl_kernel tmp = kernel_;
        kernel_ = nullptr;
        return tmp;
    }
    
private:
    /**
     * @brief Destroy.
     * @details Calls: clReleaseKernel().
     */
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

class OpenCLBuffer {
public:
    OpenCLBuffer() : buffer_(nullptr), size_(0), owned_(false) {}
    
    explicit OpenCLBuffer(cl_mem buffer, size_t size, bool owned = true) 
        : buffer_(buffer), size_(size), owned_(owned) {
        if (buffer_ && owned_) {
            clRetainMemObject(buffer_);
        }
    }
    
    // Non-copyable
    OpenCLBuffer(const OpenCLBuffer&) = delete;
    OpenCLBuffer& operator=(const OpenCLBuffer&) = delete;
    
    OpenCLBuffer(OpenCLBuffer&& other) noexcept 
        : buffer_(other.buffer_), size_(other.size_), owned_(other.owned_) {
        other.buffer_ = nullptr;
        other.size_ = 0;
        other.owned_ = false;
    }
    
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
    
    ~OpenCLBuffer() {
        destroy();
    }
    
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
    
    bool valid() const { return buffer_ != nullptr; }
    
    cl_mem get() const { return buffer_; }
    
    size_t size() const { return size_; }
    
    /**
     * @brief Release.
     * @return Return value.
     * @details Implements release without additional internal calls.
     */
    cl_mem release() {
        owned_ = false;
        cl_mem tmp = buffer_;
        buffer_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
private:
    /**
     * @brief Destroy.
     * @details Calls: clReleaseMemObject().
     */
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
