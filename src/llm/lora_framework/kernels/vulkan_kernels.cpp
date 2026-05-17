/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vulkan_kernels.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   73.0/100                                       ║
    • Total Lines:     843                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <limits>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

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
    std::mutex mutex;
};

static VulkanState g_vulkan_state;

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

static uint32_t float_to_bits(float value) {
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

bool initialize_vulkan_lora(int device_id) {
    std::lock_guard<std::mutex> lock(g_vulkan_state.mutex);
    
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
    std::lock_guard<std::mutex> lock(g_vulkan_state.mutex);
    
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
    std::lock_guard<std::mutex> lock(g_vulkan_state.mutex);
    
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
    
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
    
    // Get or create pipeline
    VulkanComputePipeline* pipeline = get_pipeline("matmul", sizeof(MatmulPushConstants));
    
    // Create buffers
    size_t size_A = static_cast<size_t>(M) * static_cast<size_t>(K) * sizeof(float);
    size_t size_B = static_cast<size_t>(K) * static_cast<size_t>(N) * sizeof(float);
    size_t size_C = static_cast<size_t>(M) * static_cast<size_t>(N) * sizeof(float);
    
    VulkanBuffer buf_A(g_vulkan_state.context.get(), size_A, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), size_B, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(g_vulkan_state.context.get(), size_C, VulkanBuffer::Usage::DeviceLocal);
    
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
    pipeline->wait();
    
    // Download result
    buf_C.download(C, size_C);
}

void launch_add_shader(const float* A, const float* B, float* C, size_t size) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
    
    // Push constants for elementwise operation
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 0 = add
        uint32_t rows;    // unused for add
        uint32_t cols;    // unused for add
        uint32_t scalar;  // unused for add
    } pc;
    
    pc.size = static_cast<uint32_t>(size);
    pc.op = 0; // add operation
    pc.rows = 0;
    pc.cols = 0;
    pc.scalar = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = size * sizeof(float);
    VulkanBuffer buf_A(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_A.upload(A, byte_size);
    buf_B.upload(B, byte_size);
    
    pipeline->bind_buffer(0, buf_A);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_C);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch with workgroup size 256
    uint32_t groups = (static_cast<uint32_t>(size) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);
    
    pipeline->wait();
    buf_C.download(C, byte_size);
}

void launch_multiply_shader(const float* A, const float* B, float* C, size_t size) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
    
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 2 = multiply
        uint32_t rows;
        uint32_t cols;
        uint32_t scalar;
    } pc;
    
    pc.size = static_cast<uint32_t>(size);
    pc.op = 2; // multiply operation
    pc.rows = 0;
    pc.cols = 0;
    pc.scalar = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = size * sizeof(float);
    VulkanBuffer buf_A(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_A.upload(A, byte_size);
    buf_B.upload(B, byte_size);
    
    pipeline->bind_buffer(0, buf_A);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_C);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    uint32_t groups = (static_cast<uint32_t>(size) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);
    
    pipeline->wait();
    buf_C.download(C, byte_size);
}

void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
    
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 4 = scalar multiply
        uint32_t rows;
        uint32_t cols;
        uint32_t scalar_bits;
    } pc;
    
    pc.size = static_cast<uint32_t>(size);
    pc.op = 4; // scalar multiply operation
    pc.rows = 0;
    pc.cols = 0;
    pc.scalar_bits = *reinterpret_cast<const uint32_t*>(&scalar);
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = size * sizeof(float);
    VulkanBuffer buf_A(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_A.upload(A, byte_size);
    
    pipeline->bind_buffer(0, buf_A);
    // Bind dummy buffer to binding 1 (required by layout)
    pipeline->bind_buffer(1, buf_A);
    pipeline->bind_buffer(2, buf_B);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    uint32_t groups = (static_cast<uint32_t>(size) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);
    
    pipeline->wait();
    buf_B.download(B, byte_size);
}

void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
    
    struct PushConstants {
        uint32_t size;
        uint32_t op;      // 5 = transpose
        uint32_t rows;
        uint32_t cols;
        uint32_t scalar;
    } pc;
    
    size_t total_size = rows * cols;
    pc.size = static_cast<uint32_t>(total_size);
    pc.op = 5; // transpose operation
    pc.rows = static_cast<uint32_t>(rows);
    pc.cols = static_cast<uint32_t>(cols);
    pc.scalar = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("elementwise", sizeof(PushConstants));
    
    size_t byte_size = total_size * sizeof(float);
    VulkanBuffer buf_input(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(g_vulkan_state.context.get(), byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    buf_input.upload(input, byte_size);
    
    pipeline->bind_buffer(0, buf_input);
    // Bind dummy buffer to binding 1 (required by layout)
    pipeline->bind_buffer(1, buf_input);
    pipeline->bind_buffer(2, buf_output);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    uint32_t groups = (static_cast<uint32_t>(total_size) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);
    
    pipeline->wait();
    buf_output.download(output, byte_size);
}

void launch_lora_grad_A_shader(
    const float* h, const float* grad_output, float* grad_A,
    int M, int K, int N, float scaling) {
    
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
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
    pc.scaling_bits = *reinterpret_cast<const uint32_t*>(&scaling);
    pc.compute_mode = 0; // grad_A computation
    
    VulkanComputePipeline* pipeline = get_pipeline("gradient", sizeof(PushConstants));
    
    size_t size_h = M * K * sizeof(float);
    size_t size_grad_output = M * N * sizeof(float);
    size_t size_grad_A = K * N * sizeof(float);
    
    VulkanBuffer buf_h(g_vulkan_state.context.get(), size_h, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_output(g_vulkan_state.context.get(), size_grad_output, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_A(g_vulkan_state.context.get(), size_grad_A, VulkanBuffer::Usage::DeviceLocal);
    
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
    
    pipeline->wait();
    buf_grad_A.download(grad_A, size_grad_A);
}

void launch_lora_grad_B_shader(
    const float* input, const float* grad_h, float* grad_B,
    int M, int D, int K) {
    
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
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
    pc.scaling_bits = *reinterpret_cast<const uint32_t*>(&scaling);
    pc.compute_mode = 1; // grad_B computation
    
    VulkanComputePipeline* pipeline = get_pipeline("gradient", sizeof(PushConstants));
    
    size_t size_input = M * D * sizeof(float);
    size_t size_grad_h = M * K * sizeof(float);
    size_t size_grad_B = D * K * sizeof(float);
    
    VulkanBuffer buf_input(g_vulkan_state.context.get(), size_input, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_h(g_vulkan_state.context.get(), size_grad_h, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_B(g_vulkan_state.context.get(), size_grad_B, VulkanBuffer::Usage::DeviceLocal);
    
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
    
    pipeline->wait();
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
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
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

    const size_t total_tokens = static_cast<size_t>(batch_size) * static_cast<size_t>(seq_len);
    const size_t output_elems = total_tokens * static_cast<size_t>(hidden_dim);
    const size_t embedding_elems = static_cast<size_t>(vocab_size) * static_cast<size_t>(hidden_dim);

    VulkanComputePipeline* pipeline = get_pipeline("embedding_lookup", sizeof(PushConstants));

    VulkanBuffer buf_token_ids(g_vulkan_state.context.get(), total_tokens * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_embedding_weights(g_vulkan_state.context.get(), embedding_elems * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(g_vulkan_state.context.get(), output_elems * sizeof(float), VulkanBuffer::Usage::DeviceLocal);

    buf_token_ids.upload(token_ids, total_tokens * sizeof(float));
    buf_embedding_weights.upload(embedding_weights, embedding_elems * sizeof(float));

    pipeline->bind_buffer(0, buf_token_ids);
    pipeline->bind_buffer(1, buf_embedding_weights);
    pipeline->bind_buffer(2, buf_output);
    pipeline->set_push_constants(&pc, sizeof(pc));

    const uint32_t groups = (static_cast<uint32_t>(total_tokens) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);

    pipeline->wait();
    buf_output.download(output, output_elems * sizeof(float));
}

void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
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

    const size_t input_elems = static_cast<size_t>(batch_size) * static_cast<size_t>(seq_len) * static_cast<size_t>(hidden_dim);
    const size_t output_elems = static_cast<size_t>(batch_size) * static_cast<size_t>(hidden_dim);

    VulkanComputePipeline* pipeline = get_pipeline("sequence_mean", sizeof(PushConstants));

    VulkanBuffer buf_input(g_vulkan_state.context.get(), input_elems * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(g_vulkan_state.context.get(), output_elems * sizeof(float), VulkanBuffer::Usage::DeviceLocal);

    buf_input.upload(input, input_elems * sizeof(float));
    pipeline->bind_buffer(0, buf_input);
    pipeline->bind_buffer(1, buf_output);
    pipeline->set_push_constants(&pc, sizeof(pc));

    const uint32_t groups = (static_cast<uint32_t>(output_elems) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);

    pipeline->wait();
    buf_output.download(output, output_elems * sizeof(float));
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
    if (!input || !B || !A || !output) {
        throw std::invalid_argument("launch_fused_lora_forward received null pointer");
    }
    if (batch_size == 0 || in_dim == 0 || rank == 0 || out_dim == 0) {
        throw std::invalid_argument("launch_fused_lora_forward received invalid dimensions");
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

    const size_t size_input = batch_size * in_dim * sizeof(float);
    const size_t size_B = in_dim * rank * sizeof(float);
    const size_t size_A = rank * out_dim * sizeof(float);
    const size_t size_h = batch_size * rank * sizeof(float);
    const size_t size_output = batch_size * out_dim * sizeof(float);

    VulkanComputePipeline* pipeline = get_pipeline("matmul", sizeof(MatmulPushConstants));

    static thread_local FusedForwardBufferCache cache;
    cache.ensure(
        g_vulkan_state.context.get(),
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

    pipeline->wait();
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
    if (!input || !B || !A || !grad_output || !grad_A || !grad_B || !grad_input) {
        throw std::invalid_argument("launch_fused_lora_backward received null pointer");
    }
    if (batch_size == 0 || in_dim == 0 || rank == 0 || out_dim == 0) {
        throw std::invalid_argument("launch_fused_lora_backward received invalid dimensions");
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

    const size_t size_input = batch_size * in_dim * sizeof(float);
    const size_t size_B = in_dim * rank * sizeof(float);
    const size_t size_A = rank * out_dim * sizeof(float);
    const size_t size_grad_output = batch_size * out_dim * sizeof(float);
    const size_t size_h = batch_size * rank * sizeof(float);
    const size_t size_grad_h = batch_size * rank * sizeof(float);
    const size_t size_a_t = out_dim * rank * sizeof(float);
    const size_t size_b_t = rank * in_dim * sizeof(float);
    const size_t size_input_t = in_dim * batch_size * sizeof(float);
    const size_t size_h_t = rank * batch_size * sizeof(float);
    const size_t size_grad_A = rank * out_dim * sizeof(float);
    const size_t size_grad_B = in_dim * rank * sizeof(float);
    const size_t size_grad_input = batch_size * in_dim * sizeof(float);

    VulkanComputePipeline* matmul_pipeline = get_pipeline("matmul", sizeof(MatmulPushConstants));
    VulkanComputePipeline* elementwise_pipeline = get_pipeline("elementwise", sizeof(ElementwisePushConstants));

    static thread_local FusedBackwardBufferCache cache;
    cache.ensure(
        g_vulkan_state.context.get(),
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
    matmul_pipeline->wait();

    // A^T and h^T
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_A, *cache.buf_a_t, rank_u, out_u);
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_h, *cache.buf_h_t, batch_u, rank_u);
    // input^T and B^T
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_input, *cache.buf_input_t, batch_u, in_u);
    dispatch_transpose_device(elementwise_pipeline, *cache.buf_B, *cache.buf_b_t, in_u, rank_u);
    elementwise_pipeline->wait();

    // grad_h = grad_output @ A^T * scaling
    dispatch_matmul_device(matmul_pipeline, *cache.buf_grad_output, *cache.buf_a_t, *cache.buf_grad_h, batch_u, rank_u, out_u, scaling);
    // grad_A = h^T @ grad_output * scaling
    dispatch_matmul_device(matmul_pipeline, *cache.buf_h_t, *cache.buf_grad_output, *cache.buf_grad_A, rank_u, out_u, batch_u, scaling);
    // grad_B = input^T @ grad_h
    dispatch_matmul_device(matmul_pipeline, *cache.buf_input_t, *cache.buf_grad_h, *cache.buf_grad_B, in_u, rank_u, batch_u, 1.0f);
    // grad_input = grad_h @ B^T
    dispatch_matmul_device(matmul_pipeline, *cache.buf_grad_h, *cache.buf_b_t, *cache.buf_grad_input, batch_u, in_u, rank_u, 1.0f);
    matmul_pipeline->wait();

    cache.buf_grad_A->download(grad_A, size_grad_A);
    cache.buf_grad_B->download(grad_B, size_grad_B);
    cache.buf_grad_input->download(grad_input, size_grad_input);
}

} // namespace vulkan
} // namespace lora
} // namespace themis
