/**
 * @file vulkan_kernels.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=14; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=11, Debt=0, C=66, H=87, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include <bit>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <limits>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace themis {
namespace lora {
namespace vulkan {

// Global state for Vulkan compute pipeline
struct VulkanState {
    bool initialized = false;
    int device_id = -1;
    
    // Vulkan context
    std::unique_ptr<VulkanContext> context;
    
    // Compute pipelines (cached)
    std::unordered_map<std::string, std::unique_ptr<VulkanComputePipeline>> pipelines;
    
    // Mutex for thread safety
    std::recursive_timed_mutex mutex;
};

static VulkanState g_vulkan_state;
constexpr auto kVulkanStateLockTimeout = std::chrono::seconds(30);
constexpr uint64_t kVulkanKernelWaitTimeoutNs = 30000000000ULL;

static std::unique_lock<std::recursive_timed_mutex> lock_vulkan_state_or_throw() {
    std::unique_lock<std::recursive_timed_mutex> lock(g_vulkan_state.mutex, std::defer_lock);
    if (!lock.try_lock_for(kVulkanStateLockTimeout)) {
        throw std::runtime_error("Timeout while waiting for Vulkan kernel state lock");
    }
    return lock;
}

static VulkanContext& get_context_or_throw() {
    if (!g_vulkan_state.initialized || !g_vulkan_state.context) {
        throw std::runtime_error("Vulkan not initialized");
    }
    return *g_vulkan_state.context;
}

static size_t checked_mul_size(size_t lhs, size_t rhs, const char* context) {
    if (lhs != 0 && rhs > (std::numeric_limits<size_t>::max() / lhs)) {
        throw std::overflow_error(std::string(context) + ": size overflow");
    }
    return lhs * rhs;
}

static size_t checked_float_bytes_2d(size_t rows, size_t cols, const char* context) {
    const size_t elems = checked_mul_size(rows, cols, context);
    return checked_mul_size(elems, sizeof(float), context);
}

static uint32_t checked_u32_size(size_t value, const char* context) {
    if (value > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        throw std::overflow_error(std::string(context) + ": value exceeds uint32 range");
    }
    return static_cast<uint32_t>(value);
}

static void wait_for_pipeline_or_throw(VulkanComputePipeline* pipeline, const char* context) {
    if (pipeline == nullptr) {
        throw std::invalid_argument(std::string(context) + ": pipeline is null");
    }
    if (!pipeline->wait(kVulkanKernelWaitTimeoutNs)) {
        throw std::runtime_error(std::string(context) + ": timed out waiting for Vulkan kernel execution");
    }
}

struct FusedForwardBufferCache {
    VulkanContext* context = nullptr;
    size_t size_input = 0;
    size_t size_B = 0;
    size_t size_A = 0;
    size_t size_h = 0;
    size_t size_output = 0;

    std::unique_ptr<VulkanBuffer> buf_input;
    std::unique_ptr<VulkanBuffer> buf_B;
    std::unique_ptr<VulkanBuffer> buf_A;
    std::unique_ptr<VulkanBuffer> buf_h;
    std::unique_ptr<VulkanBuffer> buf_output;

    void ensure(
        VulkanContext* ctx,
        size_t input_size,
        size_t b_size,
        size_t a_size,
        size_t h_size,
        size_t output_size) {

        const bool same_context = (context == ctx);
        const bool same_sizes =
            (size_input == input_size) &&
            (size_B == b_size) &&
            (size_A == a_size) &&
            (size_h == h_size) &&
            (size_output == output_size);

        if (same_context && same_sizes && buf_input && buf_B && buf_A && buf_h && buf_output) {
            return;
        }

        context = ctx;
        size_input = input_size;
        size_B = b_size;
        size_A = a_size;
        size_h = h_size;
        size_output = output_size;

        buf_input = std::make_unique<VulkanBuffer>(ctx, size_input, VulkanBuffer::Usage::DeviceLocal);
        buf_B = std::make_unique<VulkanBuffer>(ctx, size_B, VulkanBuffer::Usage::DeviceLocal);
        buf_A = std::make_unique<VulkanBuffer>(ctx, size_A, VulkanBuffer::Usage::DeviceLocal);
        buf_h = std::make_unique<VulkanBuffer>(ctx, size_h, VulkanBuffer::Usage::DeviceLocal);
        buf_output = std::make_unique<VulkanBuffer>(ctx, size_output, VulkanBuffer::Usage::DeviceLocal);
    }
};

struct FusedBackwardBufferCache {
    VulkanContext* context = nullptr;

    size_t size_input = 0;
    size_t size_B = 0;
    size_t size_A = 0;
    size_t size_grad_output = 0;
    size_t size_h = 0;
    size_t size_grad_h = 0;
    size_t size_a_t = 0;
    size_t size_b_t = 0;
    size_t size_input_t = 0;
    size_t size_h_t = 0;
    size_t size_grad_A = 0;
    size_t size_grad_B = 0;
    size_t size_grad_input = 0;

    std::unique_ptr<VulkanBuffer> buf_input;
    std::unique_ptr<VulkanBuffer> buf_B;
    std::unique_ptr<VulkanBuffer> buf_A;
    std::unique_ptr<VulkanBuffer> buf_grad_output;
    std::unique_ptr<VulkanBuffer> buf_h;
    std::unique_ptr<VulkanBuffer> buf_grad_h;
    std::unique_ptr<VulkanBuffer> buf_a_t;
    std::unique_ptr<VulkanBuffer> buf_b_t;
    std::unique_ptr<VulkanBuffer> buf_input_t;
    std::unique_ptr<VulkanBuffer> buf_h_t;
    std::unique_ptr<VulkanBuffer> buf_grad_A;
    std::unique_ptr<VulkanBuffer> buf_grad_B;
    std::unique_ptr<VulkanBuffer> buf_grad_input;

    void ensure(
        VulkanContext* ctx,
        size_t input_size,
        size_t b_size,
        size_t a_size,
        size_t grad_output_size,
        size_t h_size,
        size_t grad_h_size,
        size_t a_t_size,
        size_t b_t_size,
        size_t input_t_size,
        size_t h_t_size,
        size_t grad_a_size,
        size_t grad_b_size,
        size_t grad_input_size) {

        const bool same_context = (context == ctx);
        const bool same_sizes =
            (size_input == input_size) &&
            (size_B == b_size) &&
            (size_A == a_size) &&
            (size_grad_output == grad_output_size) &&
            (size_h == h_size) &&
            (size_grad_h == grad_h_size) &&
            (size_a_t == a_t_size) &&
            (size_b_t == b_t_size) &&
            (size_input_t == input_t_size) &&
            (size_h_t == h_t_size) &&
            (size_grad_A == grad_a_size) &&
            (size_grad_B == grad_b_size) &&
            (size_grad_input == grad_input_size);

        if (same_context && same_sizes &&
            buf_input && buf_B && buf_A && buf_grad_output &&
            buf_h && buf_grad_h && buf_a_t && buf_b_t && buf_input_t && buf_h_t &&
            buf_grad_A && buf_grad_B && buf_grad_input) {
            return;
        }

        context = ctx;
        size_input = input_size;
        size_B = b_size;
        size_A = a_size;
        size_grad_output = grad_output_size;
        size_h = h_size;
        size_grad_h = grad_h_size;
        size_a_t = a_t_size;
        size_b_t = b_t_size;
        size_input_t = input_t_size;
        size_h_t = h_t_size;
        size_grad_A = grad_a_size;
        size_grad_B = grad_b_size;
        size_grad_input = grad_input_size;

        buf_input = std::make_unique<VulkanBuffer>(ctx, size_input, VulkanBuffer::Usage::DeviceLocal);
        buf_B = std::make_unique<VulkanBuffer>(ctx, size_B, VulkanBuffer::Usage::DeviceLocal);
        buf_A = std::make_unique<VulkanBuffer>(ctx, size_A, VulkanBuffer::Usage::DeviceLocal);
        buf_grad_output = std::make_unique<VulkanBuffer>(ctx, size_grad_output, VulkanBuffer::Usage::DeviceLocal);
        buf_h = std::make_unique<VulkanBuffer>(ctx, size_h, VulkanBuffer::Usage::DeviceLocal);
        buf_grad_h = std::make_unique<VulkanBuffer>(ctx, size_grad_h, VulkanBuffer::Usage::DeviceLocal);
        buf_a_t = std::make_unique<VulkanBuffer>(ctx, size_a_t, VulkanBuffer::Usage::DeviceLocal);
        buf_b_t = std::make_unique<VulkanBuffer>(ctx, size_b_t, VulkanBuffer::Usage::DeviceLocal);
        buf_input_t = std::make_unique<VulkanBuffer>(ctx, size_input_t, VulkanBuffer::Usage::DeviceLocal);
        buf_h_t = std::make_unique<VulkanBuffer>(ctx, size_h_t, VulkanBuffer::Usage::DeviceLocal);
        buf_grad_A = std::make_unique<VulkanBuffer>(ctx, size_grad_A, VulkanBuffer::Usage::DeviceLocal);
        buf_grad_B = std::make_unique<VulkanBuffer>(ctx, size_grad_B, VulkanBuffer::Usage::DeviceLocal);
        buf_grad_input = std::make_unique<VulkanBuffer>(ctx, size_grad_input, VulkanBuffer::Usage::DeviceLocal);
    }
};

struct MatmulPushConstants {
    uint32_t M;
    uint32_t N;
    uint32_t K;
    uint32_t alpha_bits;
};

struct ElementwisePushConstants {
    uint32_t size;
    uint32_t op;
    uint32_t rows;
    uint32_t cols;
    uint32_t scalar_bits;
};

static uint32_t float_to_bits([[maybe_unused]] float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

// Helper function to get shader path
static std::string get_shader_path(const std::string& shader_name) {
    // Look for pre-compiled SPIR-V shaders
    // Priority: 1) CMake binary dir, 2) Install directory, 3) Relative paths, 4) Source directory
    const char* binary_dir = std::getenv("THEMIS_BINARY_DIR");
    const char* install_dir = std::getenv("THEMIS_INSTALL_DIR");
    
    std::vector<std::string> search_paths;
    
    // Add CMake binary directory paths
    search_paths.push_back("./shaders/lora/");  // Current directory
    search_paths.push_back("../shaders/lora/"); // Parent directory
    search_paths.push_back("../../shaders/lora/"); // Grandparent directory
    search_paths.push_back("shaders/lora/"); // Relative to binary root
    // Common invocation path when running benchmarks from repository root.
    search_paths.push_back("build/windows-bench-release/shaders/lora/");
    search_paths.push_back("build/windows-release/shaders/lora/");
    
    if (binary_dir) {
        search_paths.insert(search_paths.begin(), std::string(binary_dir) + "/shaders/lora/");
    }
    
    if (install_dir) {
        search_paths.insert(search_paths.begin(), std::string(install_dir) + "/shaders/lora/");
        search_paths.insert(search_paths.begin(), std::string(install_dir) + "/share/themis/shaders/lora/");
    }
    
    // Also check source directory location
    search_paths.push_back("src/acceleration/vulkan/shaders/lora/");
    search_paths.push_back("../src/acceleration/vulkan/shaders/lora/");
    search_paths.push_back("../../src/acceleration/vulkan/shaders/lora/");
    
    // Try with .spv extension first (pre-compiled SPIR-V)
    for (const auto& path : search_paths) {
        std::string full_path = path + shader_name + ".comp.spv";
        std::ifstream file(full_path);
        if (file.good()) {
            std::cout << "Found shader: " << full_path << std::endl;
            return full_path;
        }
    }
    
    // If .comp.spv not found, try .spv
    for (const auto& path : search_paths) {
        std::string full_path = path + shader_name + ".spv";
        std::ifstream file(full_path);
        if (file.good()) {
            std::cout << "Found shader: " << full_path << std::endl;
            return full_path;
        }
    }
    
    // If .spv not found, return .comp path (will fail but with clear error message)
    std::string comp_path = "src/acceleration/vulkan/shaders/lora/" + shader_name + ".comp";
    std::cerr << "WARNING: Could not find pre-compiled shader " << shader_name 
              << ". Shaders must be compiled to SPIR-V format (.spv)" << std::endl;
    std::cerr << "Searched paths:" << std::endl;
    for (const auto& path : search_paths) {
        std::cerr << "  " << path << shader_name << ".comp.spv" << std::endl;
    }
    
    throw std::runtime_error("Shader not found: " + shader_name + 
                           ". Please compile shaders or provide pre-compiled SPIR-V files.");
}

bool initialize_vulkan_lora([[maybe_unused]] int device_id) {
    auto lock = lock_vulkan_state_or_throw();
    
    if (g_vulkan_state.initialized) {
        return true;
    }
    
    try {
        // Create Vulkan context
        g_vulkan_state.context = std::make_unique<VulkanContext>();
        
        // Initialize context with validation layers in debug builds
        #ifdef NDEBUG
        bool enable_validation = false;
        #else
        bool enable_validation = true;
        #endif
        
        if (!g_vulkan_state.context->initialize(device_id, enable_validation)) {
            std::cerr << "Failed to initialize Vulkan context" << std::endl;
            g_vulkan_state.context.reset();
            return false;
        }
        
        std::cout << "Vulkan LoRA backend initialized successfully" << std::endl;
        std::cout << "Device: " << g_vulkan_state.context->device_properties().deviceName << std::endl;
        
        g_vulkan_state.device_id = device_id;
        g_vulkan_state.initialized = true;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Vulkan LoRA: " << e.what() << std::endl;
        g_vulkan_state.context.reset();
        return false;
    }
}

void cleanup_vulkan_lora() {
    auto lock = lock_vulkan_state_or_throw();
    
    if (!g_vulkan_state.initialized) {
        return;
    }
    
    // Clear pipelines
    g_vulkan_state.pipelines.clear();
    
    // Cleanup context
    if (g_vulkan_state.context) {
        g_vulkan_state.context->cleanup();
        g_vulkan_state.context.reset();
    }
    
    g_vulkan_state.initialized = false;
    g_vulkan_state.device_id = -1;
}

bool is_vulkan_available() {
    return VulkanContext::is_available();
}

// Helper to get or create pipeline
static VulkanComputePipeline* get_pipeline(const std::string& name, size_t push_constant_size) {
    auto lock = lock_vulkan_state_or_throw();
    
    if (!g_vulkan_state.initialized || !g_vulkan_state.context) {
        throw std::runtime_error("Vulkan not initialized. Call initialize_vulkan_lora() first.");
    }
    
    auto it = g_vulkan_state.pipelines.find(name);
    if (it != g_vulkan_state.pipelines.end()) {
        return it->second.get();
    }
    
    // Create new pipeline
    try {
        std::string shader_path = get_shader_path(name);
        auto pipeline = std::make_unique<VulkanComputePipeline>(
            g_vulkan_state.context.get(), shader_path);
        
        if (!pipeline->create(push_constant_size)) {
            throw std::runtime_error("Failed to create pipeline for " + name);
        }
        
        VulkanComputePipeline* pipeline_ptr = pipeline.get();
        g_vulkan_state.pipelines[name] = std::move(pipeline);
        
        return pipeline_ptr;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create pipeline " << name << ": " << e.what() << std::endl;
        throw;
    }
}

static void dispatch_matmul_device(
    VulkanComputePipeline* pipeline,
    const VulkanBuffer& buf_A,
    const VulkanBuffer& buf_B,
    const VulkanBuffer& buf_C,
    uint32_t M,
    uint32_t N,
    uint32_t K,
    float alpha) {

    MatmulPushConstants pc;
    pc.M = M;
    pc.N = N;
    pc.K = K;
    pc.alpha_bits = float_to_bits(alpha);

    pipeline->bind_buffer(0, buf_A);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_C);
    pipeline->set_push_constants(&pc, sizeof(pc));

    const uint32_t groups_x = (N + 15u) / 16u;
    const uint32_t groups_y = (M + 15u) / 16u;
    pipeline->dispatch(groups_x, groups_y, 1);
}

static void dispatch_transpose_device(
    VulkanComputePipeline* pipeline,
    const VulkanBuffer& buf_input,
    const VulkanBuffer& buf_output,
    uint32_t rows,
    uint32_t cols) {

    ElementwisePushConstants pc;
    pc.size = rows * cols;
    pc.op = 5u; // transpose
    pc.rows = rows;
    pc.cols = cols;
    pc.scalar_bits = 0u;

    pipeline->bind_buffer(0, buf_input);
    // Layout requires binding 1 for elementwise pipeline; reuse input as dummy.
    pipeline->bind_buffer(1, buf_input);
    pipeline->bind_buffer(2, buf_output);
    pipeline->set_push_constants(&pc, sizeof(pc));

    const uint32_t groups = (pc.size + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);
}

void launch_matmul_shader(
    const float* A, const float* B, float* C,
    int M, int N, int K, float alpha) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!A || !B || !C) {
        throw std::invalid_argument("launch_matmul_shader received null pointer");
    }
    if (M <= 0 || N <= 0 || K <= 0) {
        throw std::invalid_argument("launch_matmul_shader received invalid dimensions");
    }
    
    // Get or create pipeline
    VulkanComputePipeline* pipeline = get_pipeline("matmul", sizeof(MatmulPushConstants));
    
    // Create buffers
    size_t size_A = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(K), "launch_matmul_shader");
    size_t size_B = checked_float_bytes_2d(static_cast<size_t>(K), static_cast<size_t>(N), "launch_matmul_shader");
    size_t size_C = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(N), "launch_matmul_shader");
    
    VulkanBuffer buf_A(&context, size_A, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(&context, size_B, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(&context, size_C, VulkanBuffer::Usage::DeviceLocal);
    
    // Upload data
    buf_A.upload(A, size_A);
    buf_B.upload(B, size_B);
    
    dispatch_matmul_device(
        pipeline,
        buf_A,
        buf_B,
        buf_C,
        static_cast<uint32_t>(M),
        static_cast<uint32_t>(N),
        static_cast<uint32_t>(K),
        alpha);
    
    // Wait for completion
    wait_for_pipeline_or_throw(pipeline, "launch_matmul_shader");
    
    // Download result
    buf_C.download(C, size_C);
}

void launch_add_shader(const float* A, const float* B, float* C, size_t size) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!A || !B || !C) {
        throw std::invalid_argument("launch_add_shader received null pointer");
    }
    if (size == 0) {
        throw std::invalid_argument("launch_add_shader received invalid size");
    }
    
    // Push constants for elementwise operation
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 0 = add
        uint32_t rows;    // unused for add
        uint32_t cols;    // unused for add
        uint32_t scalar;  // unused for add
    } pc;
    
    const uint32_t size_u32 = checked_u32_size(size, "launch_add_shader");
    pc.size = size_u32;
    pc.op = 0; // add operation
    pc.rows = 0;
    pc.cols = 0;
    pc.scalar = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = checked_mul_size(size, sizeof(float), "launch_add_shader");
    VulkanBuffer buf_A(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_A.upload(A, byte_size);
    buf_B.upload(B, byte_size);
    
    pipeline->bind_buffer(0, buf_A);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_C);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch with workgroup size 256
    uint32_t groups = (size_u32 + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);
    
    wait_for_pipeline_or_throw(pipeline, "launch_add_shader");
    buf_C.download(C, byte_size);
}

void launch_multiply_shader(const float* A, const float* B, float* C, size_t size) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!A || !B || !C) {
        throw std::invalid_argument("launch_multiply_shader received null pointer");
    }
    if (size == 0) {
        throw std::invalid_argument("launch_multiply_shader received invalid size");
    }
    
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 2 = multiply
        uint32_t rows;
        uint32_t cols;
        uint32_t scalar;
    } pc;
    
    const uint32_t size_u32 = checked_u32_size(size, "launch_multiply_shader");
    pc.size = size_u32;
    pc.op = 2; // multiply operation
    pc.rows = 0;
    pc.cols = 0;
    pc.scalar = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = checked_mul_size(size, sizeof(float), "launch_multiply_shader");
    VulkanBuffer buf_A(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_A.upload(A, byte_size);
    buf_B.upload(B, byte_size);
    
    pipeline->bind_buffer(0, buf_A);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_C);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    uint32_t groups = (size_u32 + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);
    
    wait_for_pipeline_or_throw(pipeline, "launch_multiply_shader");
    buf_C.download(C, byte_size);
}

void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!A || !B) {
        throw std::invalid_argument("launch_scalar_multiply_shader received null pointer");
    }
    if (size == 0) {
        throw std::invalid_argument("launch_scalar_multiply_shader received invalid size");
    }
    if (!std::isfinite(scalar)) {
        throw std::invalid_argument("launch_scalar_multiply_shader received non-finite scalar");
    }
    
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 4 = scalar multiply
        uint32_t rows;
        uint32_t cols;
        uint32_t scalar_bits;
    } pc;
    
    const uint32_t size_u32 = checked_u32_size(size, "launch_scalar_multiply_shader");
    pc.size = size_u32;
    pc.op = 4; // scalar multiply operation
    pc.rows = 0;
    pc.cols = 0;
    pc.scalar_bits = std::bit_cast<uint32_t>(scalar);
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = checked_mul_size(size, sizeof(float), "launch_scalar_multiply_shader");
    VulkanBuffer buf_A(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_A.upload(A, byte_size);
    
    pipeline->bind_buffer(0, buf_A);
    // Bind dummy buffer to binding 1 (required by layout)
    pipeline->bind_buffer(1, buf_A);
    pipeline->bind_buffer(2, buf_B);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    uint32_t groups = (size_u32 + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);
    
    wait_for_pipeline_or_throw(pipeline, "launch_relu_shader");
    buf_B.download(B, byte_size);
}

void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!input || !output) {
        throw std::invalid_argument("launch_transpose_shader received null pointer");
    }
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("launch_transpose_shader received invalid dimensions");
    }
    
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 5 = transpose
        uint32_t rows;
        uint32_t cols;
        uint32_t scalar;
    } pc;
    
    size_t total_size = checked_mul_size(static_cast<size_t>(rows), static_cast<size_t>(cols), "launch_transpose_shader");
    const uint32_t total_size_u32 = checked_u32_size(total_size, "launch_transpose_shader");
    pc.size = total_size_u32;
    pc.op = 5; // transpose operation
    pc.rows = static_cast<uint32_t>(rows);
    pc.cols = static_cast<uint32_t>(cols);
    pc.scalar = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = checked_mul_size(total_size, sizeof(float), "launch_transpose_shader");
    VulkanBuffer buf_input(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_input.upload(input, byte_size);
    
    pipeline->bind_buffer(0, buf_input);
    // Bind dummy buffer to binding 1 (required by layout)
    pipeline->bind_buffer(1, buf_input);
    pipeline->bind_buffer(2, buf_output);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    uint32_t groups = (total_size_u32 + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);
    
    wait_for_pipeline_or_throw(pipeline, "launch_gelu_shader");
    buf_output.download(output, byte_size);
}

void launch_lora_grad_A_shader(
    const float* h, const float* grad_output, float* grad_A,
    int M, int K, int N, float scaling) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!h || !grad_output || !grad_A) {
        throw std::invalid_argument("launch_lora_grad_A_shader received null pointer");
    }
    if (M <= 0 || K <= 0 || N <= 0) {
        throw std::invalid_argument("launch_lora_grad_A_shader received invalid dimensions");
    }
    if (!std::isfinite(scaling)) {
        throw std::invalid_argument("launch_lora_grad_A_shader received non-finite scaling");
    }
    
    // Push constants for gradient computation
    struct PushConstants {
        uint32_t batch_size;
        uint32_t in_dim;
        uint32_t rank;
        uint32_t out_dim;
        uint32_t scaling_bits;
        uint32_t compute_mode; // 0 = grad_A
    } pc;
    
    pc.batch_size = static_cast<uint32_t>(M);
    pc.in_dim = 0; // Not used for grad_A
    pc.rank = static_cast<uint32_t>(K);
    pc.out_dim = static_cast<uint32_t>(N);
    pc.scaling_bits = std::bit_cast<uint32_t>(scaling);
    pc.compute_mode = 0; // grad_A computation
    
    VulkanComputePipeline* pipeline = get_pipeline("gradient", sizeof(PushConstants));
    
    size_t size_h = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(K), "launch_lora_grad_A_shader");
    size_t size_grad_output = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(N), "launch_lora_grad_A_shader");
    size_t size_grad_A = checked_float_bytes_2d(static_cast<size_t>(K), static_cast<size_t>(N), "launch_lora_grad_A_shader");
    
    VulkanBuffer buf_h(&context, size_h, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_output(&context, size_grad_output, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_A(&context, size_grad_A, VulkanBuffer::Usage::DeviceLocal);
    
    buf_h.upload(h, size_h);
    buf_grad_output.upload(grad_output, size_grad_output);
    
    // Gradient shader bindings: input(0), B(1), A(2), grad_output(3), grad_A(4), grad_B(5), grad_input(6)
    // For grad_A computation we need: input(h), grad_output, grad_A
    pipeline->bind_buffer(0, buf_h); // h serves as input
    pipeline->bind_buffer(1, buf_h); // dummy B
    pipeline->bind_buffer(2, buf_h); // dummy A
    pipeline->bind_buffer(3, buf_grad_output);
    pipeline->bind_buffer(4, buf_grad_A);
    pipeline->bind_buffer(5, buf_grad_A); // dummy grad_B
    pipeline->bind_buffer(6, buf_grad_A); // dummy grad_input
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch with workgroup size 16x16
    uint32_t groups_x = (N + 15) / 16;
    uint32_t groups_y = (K + 15) / 16;
    pipeline->dispatch(groups_x, groups_y, 1);
    
    wait_for_pipeline_or_throw(pipeline, "launch_softmax_shader");
    buf_grad_A.download(grad_A, size_grad_A);
}

void launch_lora_grad_B_shader(
    const float* input, const float* grad_h, float* grad_B,
    int M, int D, int K) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!input || !grad_h || !grad_B) {
        throw std::invalid_argument("launch_lora_grad_B_shader received null pointer");
    }
    if (M <= 0 || D <= 0 || K <= 0) {
        throw std::invalid_argument("launch_lora_grad_B_shader received invalid dimensions");
    }
    
    struct PushConstants {
        uint32_t batch_size;
        uint32_t in_dim;
        uint32_t rank;
        uint32_t out_dim;
        uint32_t scaling_bits;
        uint32_t compute_mode; // 1 = grad_B
    } pc;
    
    pc.batch_size = static_cast<uint32_t>(M);
    pc.in_dim = static_cast<uint32_t>(D);
    pc.rank = static_cast<uint32_t>(K);
    pc.out_dim = 0; // Not used for grad_B
    float scaling = 1.0f;
    pc.scaling_bits = std::bit_cast<uint32_t>(scaling);
    pc.compute_mode = 1; // grad_B computation
    
    VulkanComputePipeline* pipeline = get_pipeline("gradient", sizeof(PushConstants));
    
    size_t size_input = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
    size_t size_grad_h = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(K), "launch_lora_grad_B_shader");
    size_t size_grad_B = checked_float_bytes_2d(static_cast<size_t>(D), static_cast<size_t>(K), "launch_lora_grad_B_shader");
    
    VulkanBuffer buf_input(&context, size_input, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_h(&context, size_grad_h, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_B(&context, size_grad_B, VulkanBuffer::Usage::DeviceLocal);
    
    buf_input.upload(input, size_input);
    buf_grad_h.upload(grad_h, size_grad_h);
    
    // For grad_B computation we need: input, grad_h (as grad_output), grad_B
    pipeline->bind_buffer(0, buf_input);
    pipeline->bind_buffer(1, buf_grad_B); // dummy B
    pipeline->bind_buffer(2, buf_grad_B); // dummy A
    pipeline->bind_buffer(3, buf_grad_h); // grad_h as grad_output
    pipeline->bind_buffer(4, buf_grad_B); // dummy grad_A
    pipeline->bind_buffer(5, buf_grad_B); // grad_B output
    pipeline->bind_buffer(6, buf_grad_B); // dummy grad_input
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch with workgroup size 16x16
    uint32_t groups_x = (K + 15) / 16;
    uint32_t groups_y = (D + 15) / 16;
    pipeline->dispatch(groups_x, groups_y, 1);
    
    wait_for_pipeline_or_throw(pipeline, "launch_lora_forward_shader");
    buf_grad_B.download(grad_B, size_grad_B);
}

void launch_embedding_lookup_shader(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!output || !token_ids || !embedding_weights) {
        throw std::invalid_argument("launch_embedding_lookup_shader received null pointer");
    }
    if (batch_size <= 0 || seq_len <= 0 || hidden_dim <= 0 || vocab_size <= 0) {
        throw std::invalid_argument("launch_embedding_lookup_shader received invalid dimensions");
    }

    struct PushConstants {
        uint32_t batch_size;
        uint32_t seq_len;
        uint32_t hidden_dim;
        uint32_t vocab_size;
    } pc;

    pc.batch_size = static_cast<uint32_t>(batch_size);
    pc.seq_len = static_cast<uint32_t>(seq_len);
    pc.hidden_dim = static_cast<uint32_t>(hidden_dim);
    pc.vocab_size = static_cast<uint32_t>(vocab_size);

    const size_t total_tokens = checked_mul_size(static_cast<size_t>(batch_size), static_cast<size_t>(seq_len), "launch_embedding_lookup_shader");
    const size_t output_elems = checked_mul_size(total_tokens, static_cast<size_t>(hidden_dim), "launch_embedding_lookup_shader");
    const size_t embedding_elems = checked_mul_size(static_cast<size_t>(vocab_size), static_cast<size_t>(hidden_dim), "launch_embedding_lookup_shader");
    const uint32_t total_tokens_u32 = checked_u32_size(total_tokens, "launch_embedding_lookup_shader");

    VulkanComputePipeline* pipeline = get_pipeline("embedding_lookup", sizeof(PushConstants));

    VulkanBuffer buf_token_ids(&context, checked_mul_size(total_tokens, sizeof(float), "launch_embedding_lookup_shader"), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_embedding_weights(&context, checked_mul_size(embedding_elems, sizeof(float), "launch_embedding_lookup_shader"), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(&context, checked_mul_size(output_elems, sizeof(float), "launch_embedding_lookup_shader"), VulkanBuffer::Usage::DeviceLocal);

    buf_token_ids.upload(token_ids, checked_mul_size(total_tokens, sizeof(float), "launch_embedding_lookup_shader"));
    buf_embedding_weights.upload(embedding_weights, checked_mul_size(embedding_elems, sizeof(float), "launch_embedding_lookup_shader"));

    pipeline->bind_buffer(0, buf_token_ids);
    pipeline->bind_buffer(1, buf_embedding_weights);
    pipeline->bind_buffer(2, buf_output);
    pipeline->set_push_constants(&pc, sizeof(pc));

    const uint32_t groups = (total_tokens_u32 + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);

    wait_for_pipeline_or_throw(pipeline, "launch_lora_backward_shader");
    buf_output.download(output, checked_mul_size(output_elems, sizeof(float), "launch_embedding_lookup_shader"));
}

void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!output || !input) {
        throw std::invalid_argument("launch_sequence_mean_shader received null pointer");
    }
    if (batch_size <= 0 || seq_len <= 0 || hidden_dim <= 0) {
        throw std::invalid_argument("launch_sequence_mean_shader received invalid dimensions");
    }

    struct PushConstants {
        uint32_t batch_size;
        uint32_t seq_len;
        uint32_t hidden_dim;
        uint32_t reserved;
    } pc;

    pc.batch_size = static_cast<uint32_t>(batch_size);
    pc.seq_len = static_cast<uint32_t>(seq_len);
    pc.hidden_dim = static_cast<uint32_t>(hidden_dim);
    pc.reserved = 0;

    const size_t input_elems = checked_mul_size(
        checked_mul_size(static_cast<size_t>(batch_size), static_cast<size_t>(seq_len), "launch_sequence_mean_shader"),
        static_cast<size_t>(hidden_dim),
        "launch_sequence_mean_shader");
    const size_t output_elems = checked_mul_size(static_cast<size_t>(batch_size), static_cast<size_t>(hidden_dim), "launch_sequence_mean_shader");
    const uint32_t output_elems_u32 = checked_u32_size(output_elems, "launch_sequence_mean_shader");

    VulkanComputePipeline* pipeline = get_pipeline("sequence_mean", sizeof(PushConstants));

    VulkanBuffer buf_input(&context, checked_mul_size(input_elems, sizeof(float), "launch_sequence_mean_shader"), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(&context, checked_mul_size(output_elems, sizeof(float), "launch_sequence_mean_shader"), VulkanBuffer::Usage::DeviceLocal);

    buf_input.upload(input, checked_mul_size(input_elems, sizeof(float), "launch_sequence_mean_shader"));
    pipeline->bind_buffer(0, buf_input);
    pipeline->bind_buffer(1, buf_output);
    pipeline->set_push_constants(&pc, sizeof(pc));

    const uint32_t groups = (output_elems_u32 + 255u) / 256u;
    pipeline->dispatch(groups, 1, 1);

    wait_for_pipeline_or_throw(pipeline, "launch_lora_grad_A_shader");
    buf_output.download(output, checked_mul_size(output_elems, sizeof(float), "launch_sequence_mean_shader"));
}

void launch_fused_lora_forward(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!input || !B || !A || !output) {
        throw std::invalid_argument("launch_fused_lora_forward received null pointer");
    }
    if (batch_size == 0 || in_dim == 0 || rank == 0 || out_dim == 0) {
        throw std::invalid_argument("launch_fused_lora_forward received invalid dimensions");
    }
    if (!std::isfinite(scaling)) {
        throw std::invalid_argument("launch_fused_lora_forward received non-finite scaling");
    }
    if (batch_size > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        in_dim > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        rank > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        out_dim > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("launch_fused_lora_forward dimensions exceed int range");
    }

    const uint32_t batch_u = static_cast<uint32_t>(batch_size);
    const uint32_t in_u = static_cast<uint32_t>(in_dim);
    const uint32_t rank_u = static_cast<uint32_t>(rank);
    const uint32_t out_u = static_cast<uint32_t>(out_dim);

    const size_t size_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_forward");
    const size_t size_B = checked_float_bytes_2d(in_dim, rank, "launch_fused_lora_forward");
    const size_t size_A = checked_float_bytes_2d(rank, out_dim, "launch_fused_lora_forward");
    const size_t size_h = checked_float_bytes_2d(batch_size, rank, "launch_fused_lora_forward");
    const size_t size_output = checked_float_bytes_2d(batch_size, out_dim, "launch_fused_lora_forward");

    VulkanComputePipeline* pipeline = get_pipeline("matmul", sizeof(MatmulPushConstants));

    FusedForwardBufferCache cache;
    cache.ensure(
        &context,
        size_input,
        size_B,
        size_A,
        size_h,
        size_output);

    // Hardware-near path: keep intermediate h on device and avoid host roundtrip between the two GEMMs.
    cache.buf_input->upload(input, size_input);
    cache.buf_B->upload(B, size_B);
    cache.buf_A->upload(A, size_A);

    dispatch_matmul_device(pipeline, *cache.buf_input, *cache.buf_B, *cache.buf_h, batch_u, rank_u, in_u, 1.0f);
    dispatch_matmul_device(pipeline, *cache.buf_h, *cache.buf_A, *cache.buf_output, batch_u, out_u, rank_u, scaling);

    wait_for_pipeline_or_throw(pipeline, "launch_lora_grad_B_shader");
    cache.buf_output->download(output, size_output);
}

void launch_fused_lora_backward(
    const float* input,
    const float* B,
    const float* A,
    const float* grad_output,
    float* grad_A,
    float* grad_B,
    float* grad_input,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling) {
    auto lock = lock_vulkan_state_or_throw();
    VulkanContext& context = get_context_or_throw();
    if (!input || !B || !A || !grad_output || !grad_A || !grad_B || !grad_input) {
        throw std::invalid_argument("launch_fused_lora_backward received null pointer");
    }
    if (batch_size == 0 || in_dim == 0 || rank == 0 || out_dim == 0) {
        throw std::invalid_argument("launch_fused_lora_backward received invalid dimensions");
    }
    if (!std::isfinite(scaling)) {
        throw std::invalid_argument("launch_fused_lora_backward received non-finite scaling");
    }
    if (batch_size > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        in_dim > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        rank > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        out_dim > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("launch_fused_lora_backward dimensions exceed int range");
    }

    const uint32_t batch_u = static_cast<uint32_t>(batch_size);
    const uint32_t in_u = static_cast<uint32_t>(in_dim);
    const uint32_t rank_u = static_cast<uint32_t>(rank);
    const uint32_t out_u = static_cast<uint32_t>(out_dim);

    const size_t size_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_backward");
    const size_t size_B = checked_float_bytes_2d(in_dim, rank, "launch_fused_lora_backward");
    const size_t size_A = checked_float_bytes_2d(rank, out_dim, "launch_fused_lora_backward");
    const size_t size_grad_output = checked_float_bytes_2d(batch_size, out_dim, "launch_fused_lora_backward");
    const size_t size_h = checked_float_bytes_2d(batch_size, rank, "launch_fused_lora_backward");
    const size_t size_grad_h = checked_float_bytes_2d(batch_size, rank, "launch_fused_lora_backward");
    const size_t size_a_t = checked_float_bytes_2d(out_dim, rank, "launch_fused_lora_backward");
    const size_t size_b_t = checked_float_bytes_2d(rank, in_dim, "launch_fused_lora_backward");
    const size_t size_input_t = checked_float_bytes_2d(in_dim, batch_size, "launch_fused_lora_backward");
    const size_t size_h_t = checked_float_bytes_2d(rank, batch_size, "launch_fused_lora_backward");
    const size_t size_grad_A = checked_float_bytes_2d(rank, out_dim, "launch_fused_lora_backward");
    const size_t size_grad_B = checked_float_bytes_2d(in_dim, rank, "launch_fused_lora_backward");
    const size_t size_grad_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_backward");

    VulkanComputePipeline* matmul_pipeline = get_pipeline("matmul", sizeof(MatmulPushConstants));
    VulkanComputePipeline* elementwise_pipeline = get_pipeline("elementwise", sizeof(ElementwisePushConstants));

    FusedBackwardBufferCache cache;
    cache.ensure(
        &context,
        size_input,
        size_B,
        size_A,
        size_grad_output,
        size_h,
        size_grad_h,
        size_a_t,
        size_b_t,
        size_input_t,
        size_h_t,
        size_grad_A,
        size_grad_B,
        size_grad_input);

    // Upload once, keep intermediates on device.
    cache.buf_input->upload(input, size_input);
    cache.buf_B->upload(B, size_B);
    cache.buf_A->upload(A, size_A);
    cache.buf_grad_output->upload(grad_output, size_grad_output);

    // h = input @ B
    dispatch_matmul_device(matmul_pipeline, *cache.buf_input, *cache.buf_B, *cache.buf_h, batch_u, rank_u, in_u, 1.0f);
    wait_for_pipeline_or_throw(matmul_pipeline, "launch_fused_lora_backward: matmul(h)");

    // A^T and h^T
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_A, *cache.buf_a_t, rank_u, out_u);
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_h, *cache.buf_h_t, batch_u, rank_u);
    // input^T and B^T
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_input, *cache.buf_input_t, batch_u, in_u);
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_B, *cache.buf_b_t, in_u, rank_u);
    wait_for_pipeline_or_throw(elementwise_pipeline, "launch_fused_lora_backward: elementwise(transpose)");

    // grad_h = grad_output @ A^T * scaling
    dispatch_matmul_device(matmul_pipeline, *cache.buf_grad_output, *cache.buf_a_t, *cache.buf_grad_h, batch_u, rank_u, out_u, scaling);
    // grad_A = h^T @ grad_output * scaling
    dispatch_matmul_device(matmul_pipeline, *cache.buf_h_t, *cache.buf_grad_output, *cache.buf_grad_A, rank_u, out_u, batch_u, scaling);
    // grad_B = input^T @ grad_h
    dispatch_matmul_device(matmul_pipeline, *cache.buf_input_t, *cache.buf_grad_h, *cache.buf_grad_B, in_u, rank_u, batch_u, 1.0f);
    // grad_input = grad_h @ B^T
    dispatch_matmul_device(matmul_pipeline, *cache.buf_grad_h, *cache.buf_b_t, *cache.buf_grad_input, batch_u, in_u, rank_u, 1.0f);
    wait_for_pipeline_or_throw(matmul_pipeline, "launch_fused_lora_backward: matmul(grads)");

    cache.buf_grad_A->download(grad_A, size_grad_A);
    cache.buf_grad_B->download(grad_B, size_grad_B);
    cache.buf_grad_input->download(grad_input, size_grad_input);
}

} // namespace vulkan
} // namespace lora
} // namespace themis
