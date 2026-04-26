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
    // Priority: 1) CMake binary dir, 2) Install directory, 3) Relative paths, 4) Source directory
    const char* binary_dir = std::getenv("THEMIS_BINARY_DIR");
    const char* install_dir = std::getenv("THEMIS_INSTALL_DIR");
    
    std::vector<std::string> search_paths;
    
    // Add CMake binary directory paths
    search_paths.push_back("./shaders/lora/");  // Current directory
    search_paths.push_back("../shaders/lora/"); // Parent directory
    search_paths.push_back("../../shaders/lora/"); // Grandparent directory
    search_paths.push_back("shaders/lora/"); // Relative to binary root
    
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

void launch_embedding_lookup_shader(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size) {
    
// ============================================================================
// Fused LoRA Kernels (Phase 4: Vulkan Backend)
// ============================================================================

/**
 * @brief Vulkan fused LoRA forward pass: Y = (X @ B^T @ A^T) * scaling
 * 
 * Implements the complete LoRA forward path in a single compute shader dispatch:
 * - Computes intermediate h = X @ B^T
 * - Keeps intermediate in shared memory (workgroup local memory)
 * - Computes output = h @ A^T * scaling
 * 
 * Expected performance: Similar to CUDA/HIP fused kernels
 * Memory savings: 33-75% reduction in global memory traffic
 */
void launch_fused_lora_forward(
    const float* input,     // [batch_size, in_dim]
    const float* B,         // [in_dim, rank]
    const float* A,         // [rank, out_dim]
    float* output,          // [batch_size, out_dim]
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized. Call initialize_vulkan_lora() first.");
    }
    
    std::lock_guard<std::mutex> lock(g_vulkan_state.mutex);
    
    // Calculate sizes
    size_t total_tokens = batch_size * seq_len;
    size_t output_size = total_tokens * hidden_dim;
    size_t embedding_matrix_size = vocab_size * hidden_dim;
    
    // Push constants structure
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
    
    VulkanComputePipeline* pipeline = get_pipeline("embedding_lookup", sizeof(PushConstants));
    
    // Create buffers
    VulkanBuffer buf_token_ids(g_vulkan_state.context.get(), total_tokens * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_embedding_weights(g_vulkan_state.context.get(), embedding_matrix_size * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(g_vulkan_state.context.get(), output_size * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    
    // Upload data
    buf_token_ids.upload(token_ids, total_tokens * sizeof(float));
    buf_embedding_weights.upload(embedding_weights, embedding_matrix_size * sizeof(float));
    
    // Bind buffers
    pipeline->bind_buffer(0, buf_token_ids);
    pipeline->bind_buffer(1, buf_embedding_weights);
    pipeline->bind_buffer(2, buf_output);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch: each thread handles one token
    uint32_t groups = (static_cast<uint32_t>(total_tokens) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);
    
    pipeline->wait();
    buf_output.download(output, output_size * sizeof(float));
}

void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim) {
    
    // Push constants for fused LoRA forward
    struct PushConstants {
        uint32_t batch_size;
        uint32_t in_dim;
        uint32_t rank;
        uint32_t out_dim;
        uint32_t scaling_bits; // float as uint for alignment
    } pc;
    
    pc.batch_size = static_cast<uint32_t>(batch_size);
    pc.in_dim = static_cast<uint32_t>(in_dim);
    pc.rank = static_cast<uint32_t>(rank);
    pc.out_dim = static_cast<uint32_t>(out_dim);
    pc.scaling_bits = *reinterpret_cast<const uint32_t*>(&scaling);
    
    // Get or create pipeline for fused LoRA forward
    VulkanComputePipeline* pipeline = get_pipeline("fused_lora_forward", sizeof(PushConstants));
    
    // Create buffers
    size_t size_input = batch_size * in_dim * sizeof(float);
    size_t size_B = in_dim * rank * sizeof(float);
    size_t size_A = rank * out_dim * sizeof(float);
    size_t size_output = batch_size * out_dim * sizeof(float);
    
    VulkanBuffer buf_input(g_vulkan_state.context.get(), size_input, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), size_B, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_A(g_vulkan_state.context.get(), size_A, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(g_vulkan_state.context.get(), size_output, VulkanBuffer::Usage::DeviceLocal);
    
    // Upload data
    buf_input.upload(input, size_input);
    buf_B.upload(B, size_B);
    buf_A.upload(A, size_A);
    
    // Bind buffers (matches shader layout)
    pipeline->bind_buffer(0, buf_input);   // binding 0: input
    pipeline->bind_buffer(1, buf_B);       // binding 1: B
    pipeline->bind_buffer(2, buf_A);       // binding 2: A
    pipeline->bind_buffer(3, buf_output);  // binding 3: output
    
    // Set push constants
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch compute shader
    // Workgroup size: 16x1x1 (defined in shader)
    // Each workgroup processes one batch element
    uint32_t groups_x = (out_dim + 15) / 16;
    uint32_t groups_y = batch_size;
    pipeline->dispatch(groups_x, groups_y, 1);
    
    // Wait for completion
    pipeline->wait();
    
    // Download result
    buf_output.download(output, size_output);
}

/**
 * @brief Vulkan fused LoRA backward pass
 * 
 * Computes all gradients in a single compute shader:
 * - grad_A = intermediate^T @ grad_output * scaling
 * - grad_B = input^T @ (grad_output @ A) * scaling
 * - grad_input = (grad_output @ A) @ B * scaling
 */
void launch_fused_lora_backward(
    const float* input,       // [batch_size, in_dim]
    const float* B,           // [in_dim, rank]
    const float* A,           // [rank, out_dim]
    const float* grad_output, // [batch_size, out_dim]
    float* grad_A,            // [rank, out_dim]
    float* grad_B,            // [in_dim, rank]
    float* grad_input,        // [batch_size, in_dim]
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
) {
    if (!g_vulkan_state.initialized) {
        throw std::runtime_error("Vulkan not initialized. Call initialize_vulkan_lora() first.");
    }
    
    std::lock_guard<std::mutex> lock(g_vulkan_state.mutex);
    
    // Calculate sizes
    size_t input_size = batch_size * seq_len * hidden_dim;
    size_t output_size = batch_size * hidden_dim;
    
    // Push constants structure
    struct PushConstants {
        uint32_t batch_size;
        uint32_t seq_len;
        uint32_t hidden_dim;
        uint32_t reserved;  // Padding for alignment
    } pc;
    
    pc.batch_size = static_cast<uint32_t>(batch_size);
    pc.seq_len = static_cast<uint32_t>(seq_len);
    pc.hidden_dim = static_cast<uint32_t>(hidden_dim);
    pc.reserved = 0;
    
    VulkanComputePipeline* pipeline = get_pipeline("sequence_mean", sizeof(PushConstants));
    
    // Create buffers
    VulkanBuffer buf_input(g_vulkan_state.context.get(), input_size * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_output(g_vulkan_state.context.get(), output_size * sizeof(float), VulkanBuffer::Usage::DeviceLocal);
    
    // Upload data
    buf_input.upload(input, input_size * sizeof(float));
    
    // Bind buffers
    pipeline->bind_buffer(0, buf_input);
    pipeline->bind_buffer(1, buf_output);
    
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch: each thread handles one output element
    uint32_t groups = (static_cast<uint32_t>(output_size) + 255) / 256;
    pipeline->dispatch(groups, 1, 1);
    
    pipeline->wait();
    buf_output.download(output, output_size * sizeof(float));
    // Push constants for fused LoRA backward
    struct PushConstants {
        uint32_t batch_size;
        uint32_t in_dim;
        uint32_t rank;
        uint32_t out_dim;
        uint32_t scaling_bits;
    } pc;
    
    pc.batch_size = static_cast<uint32_t>(batch_size);
    pc.in_dim = static_cast<uint32_t>(in_dim);
    pc.rank = static_cast<uint32_t>(rank);
    pc.out_dim = static_cast<uint32_t>(out_dim);
    pc.scaling_bits = *reinterpret_cast<const uint32_t*>(&scaling);
    
    // Get or create pipeline
    VulkanComputePipeline* pipeline = get_pipeline("fused_lora_backward", sizeof(PushConstants));
    
    // Create buffers
    size_t size_input = batch_size * in_dim * sizeof(float);
    size_t size_B = in_dim * rank * sizeof(float);
    size_t size_A = rank * out_dim * sizeof(float);
    size_t size_grad_output = batch_size * out_dim * sizeof(float);
    size_t size_grad_A = rank * out_dim * sizeof(float);
    size_t size_grad_B = in_dim * rank * sizeof(float);
    size_t size_grad_input = batch_size * in_dim * sizeof(float);
    
    VulkanBuffer buf_input(g_vulkan_state.context.get(), size_input, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_B(g_vulkan_state.context.get(), size_B, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_A(g_vulkan_state.context.get(), size_A, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_output(g_vulkan_state.context.get(), size_grad_output, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_A(g_vulkan_state.context.get(), size_grad_A, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_B(g_vulkan_state.context.get(), size_grad_B, VulkanBuffer::Usage::DeviceLocal);
    VulkanBuffer buf_grad_input(g_vulkan_state.context.get(), size_grad_input, VulkanBuffer::Usage::DeviceLocal);
    
    // Upload data
    buf_input.upload(input, size_input);
    buf_B.upload(B, size_B);
    buf_A.upload(A, size_A);
    buf_grad_output.upload(grad_output, size_grad_output);
    
    // Bind buffers
    pipeline->bind_buffer(0, buf_input);
    pipeline->bind_buffer(1, buf_B);
    pipeline->bind_buffer(2, buf_A);
    pipeline->bind_buffer(3, buf_grad_output);
    pipeline->bind_buffer(4, buf_grad_A);
    pipeline->bind_buffer(5, buf_grad_B);
    pipeline->bind_buffer(6, buf_grad_input);
    
    // Set push constants
    pipeline->set_push_constants(&pc, sizeof(pc));
    
    // Dispatch (workgroup size 16x16 from shader)
    // Note: Backward pass is more complex, may need multiple dispatches
    // For simplicity, we dispatch once with appropriate workgroup configuration
    uint32_t groups_x = (std::max({out_dim, rank, in_dim}) + 15) / 16;
    uint32_t groups_y = (std::max({rank, in_dim, batch_size}) + 15) / 16;
    pipeline->dispatch(groups_x, groups_y, 1);
    
    // Wait for completion
    pipeline->wait();
    
    // Download results
    buf_grad_A.download(grad_A, size_grad_A);
    buf_grad_B.download(grad_B, size_grad_B);
    buf_grad_input.download(grad_input, size_grad_input);
}

} // namespace vulkan
} // namespace lora
} // namespace themis
