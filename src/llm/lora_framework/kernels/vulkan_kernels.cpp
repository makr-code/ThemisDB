#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
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

// Helper function to get shader path
static std::string get_shader_path(const std::string& shader_name) {
    // Look for pre-compiled SPIR-V shaders
    // Priority: 1) Binary directory, 2) Install directory, 3) Source directory
    const char* binary_dir = std::getenv("THEMIS_BINARY_DIR");
    const char* install_dir = std::getenv("THEMIS_INSTALL_DIR");
    
    std::vector<std::string> search_paths = {
        "./shaders/lora/",  // Current directory
        "../shaders/lora/", // Parent directory
        "../../shaders/lora/", // Grandparent directory
    };
    
    if (binary_dir) {
        search_paths.insert(search_paths.begin(), std::string(binary_dir) + "/shaders/lora/");
    }
    
    if (install_dir) {
        search_paths.insert(search_paths.begin(), std::string(install_dir) + "/shaders/lora/");
    }
    
    // Also check source directory location
    search_paths.push_back("src/acceleration/vulkan/shaders/lora/");
    
    for (const auto& path : search_paths) {
        std::string full_path = path + shader_name + ".spv";
        std::ifstream file(full_path);
        if (file.good()) {
            return full_path;
        }
    }
    
    // If .spv not found, return .comp path (will need runtime compilation)
    return "src/acceleration/vulkan/shaders/lora/" + shader_name + ".comp";
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

void launch_matmul_shader(
    const float* A, const float* B, float* C,
    int M, int N, int K, float alpha) {
    
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized");
    }
    
    // Push constants structure matching shader
    struct PushConstants {
        uint32_t M, N, K;
        uint32_t alpha_bits; // float stored as uint for alignment
    } pc;
    
    pc.M = static_cast<uint32_t>(M);
    pc.N = static_cast<uint32_t>(N);
    pc.K = static_cast<uint32_t>(K);
    pc.alpha_bits = *reinterpret_cast<const uint32_t*>(&alpha);
    
    // Get or create pipeline
    VulkanComputePipeline* pipeline = get_pipeline("matmul", sizeof(PushConstants));
    
    // Create buffers
    size_t size_A = M * K * sizeof(float);
    size_t size_B = K * N * sizeof(float);
    size_t size_C = M * N * sizeof(float);
    
    VulkanBuffer buf_A(g_vulkan_state.context.get(), size_A, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), size_B, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_C(g_vulkan_state.context.get(), size_C, VulkanBuffer::Usage::DeviceLocal);
    
    // Upload data
    buf_A.upload(A, size_A);
    buf_B.upload(B, size_B);
    
    // Bind buffers
    pipeline->bind_buffer(0, buf_A);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_C);
    
    // Set push constants
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch (workgroup size 16x16 from shader)
    uint32_t groups_x = (N + 15) / 16;
    uint32_t groups_y = (M + 15) / 16;
    pipeline->dispatch(groups_x, groups_y, 1);
    
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

} // namespace vulkan
} // namespace lora
} // namespace themis
