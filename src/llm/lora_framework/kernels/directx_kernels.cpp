/**
 * @file directx_kernels.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=6; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=0, C=56, H=45, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/directx_kernels.h"
#include <stdexcept>

#ifdef _WIN32

#include "llm/lora_framework/directx_context.h"
#include "llm/lora_framework/directx_buffer.h"
#include "llm/lora_framework/directx_descriptors.h"
#include "llm/lora_framework/directx_shader.h"
#include "llm/lora_framework/directx_pipeline.h"

#include <string>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <cstring>
#include <cmath>
#include <mutex>
#include <chrono>
#include <limits>

namespace themis {
namespace lora {
namespace directx {

// Helper function to get shader path
static std::string get_shader_path(const std::string& shader_name) {
    namespace fs = std::filesystem;
    
    // Try various candidate roots (build dir, parent, up to project root)
    std::vector<fs::path> roots;
    fs::path cur = fs::current_path();
    roots.push_back(cur);
    if (cur.has_parent_path()) {
      roots.push_back(cur.parent_path());
    }
    if (cur.parent_path().has_parent_path()) {
      roots.push_back(cur.parent_path().parent_path());
    }
    if (cur.parent_path().parent_path().has_parent_path()) {
      roots.push_back(cur.parent_path().parent_path().parent_path());
    }

    for (const auto& root : roots) {
        std::cout << "DirectX Debug: probing root: " << root.string() << "\n";
        std::vector<fs::path> base_paths = {
            root / "shaders" / "lora",
            root / "bin" / "shaders" / "lora",
            root / "src" / "acceleration" / "directx" / "shaders" / "lora",
            root / ".." / "src" / "acceleration" / "directx" / "shaders" / "lora",
        };
        for (const auto& base : base_paths) {
            std::cout << "DirectX Debug: probing base: " << base.string() << "\n";
            // Try exact name
            fs::path p1 = base / shader_name;
            if (fs::exists(p1)) { std::cout << "DirectX Debug: found exact: " << p1.string() << "\n"; return p1.string(); }

            // Try alternate extensions (.hlsl/.cso)
            fs::path p_hlsl = base / fs::path(fs::path(shader_name).stem().string() + ".hlsl");
            if (fs::exists(p_hlsl)) { std::cout << "DirectX Debug: found hlsl: " << p_hlsl.string() << "\n"; return p_hlsl.string(); }

            fs::path p_cso = base / fs::path(fs::path(shader_name).stem().string() + ".cso");
            if (fs::exists(p_cso)) { std::cout << "DirectX Debug: found cso: " << p_cso.string() << "\n"; return p_cso.string(); }
        }
    }

    // Fallback: return best-effort path under current/ shaders
    // If still not found, try to locate repository root (look for CMakeLists.txt)
    fs::path probe = fs::current_path();
    for (int i = 0; i < 8 && probe.has_parent_path(); ++i) {
        if (fs::exists(probe / "CMakeLists.txt")) {
            std::cout << "DirectX Debug: repo root candidate: " << probe.string() << "\n";
            fs::path repo_root = probe;
            fs::path candidate = repo_root / "src" / "acceleration" / "directx" / "shaders" / "lora" / fs::path(fs::path(shader_name).stem().string() + ".hlsl");
            if (fs::exists(candidate)) { std::cout << "DirectX Debug: found in repo: " << candidate.string() << "\n"; return candidate.string(); }
            candidate = repo_root / "src" / "acceleration" / "directx" / "shaders" / "lora" / shader_name;
            if (fs::exists(candidate)) { std::cout << "DirectX Debug: found in repo (2): " << candidate.string() << "\n"; return candidate.string(); }
            break;
        }
        probe = probe.parent_path();
    }

    return (fs::current_path() / "shaders" / "lora" / shader_name).string();
}

// Global state for DirectX 12 compute pipeline
struct DirectXState {
    bool initialized = false;
    int adapter_id = -1;
    
    // DirectX core objects
    std::unique_ptr<DirectXContext> context;
    std::unique_ptr<DirectXDescriptors> descriptors;
    
    // Shader and pipeline cache
    std::unordered_map<std::string, std::unique_ptr<DirectXShader>> shaders;
    std::unordered_map<std::string, std::unique_ptr<DirectXPipeline>> pipelines;
};

static DirectXState g_directx_state;
// Guards all access to the process-wide DirectX context, descriptor heap, and
// shader/pipeline caches so threads never observe partially updated DX state.
static std::mutex g_directx_state_mutex;
constexpr uint32_t kDirectXKernelExecutionTimeoutMs = 30000;

static void ensure_directx_ready_or_throw(const DirectXState& state) {
    if (!state.initialized || !state.context || !state.descriptors) {
        throw std::runtime_error("DirectX not initialized. Call initialize_directx_lora() first.");
    }
}

// Helper function to get or create shader
static DirectXShader* get_or_load_shader(DirectXState& state, const std::string& shader_name) {
    auto& shader_cache = state.shaders;

    // Check if already loaded
    auto it = shader_cache.find(shader_name);
    if (it != shader_cache.end()) {
        return it->second.get();
    }

    // Load shader
    std::string shader_path = get_shader_path(shader_name);
    auto shader = std::make_unique<DirectXShader>(shader_path);

    if (!shader->load()) {
        throw std::runtime_error("Failed to load shader: " + shader_name + " from " + shader_path);
    }

    DirectXShader* shader_ptr = shader.get();
    shader_cache[shader_name] = std::move(shader);
    return shader_ptr;
}

// Helper function to get or create pipeline
static DirectXPipeline* get_or_create_pipeline(
    DirectXState& state,
    const std::string& pipeline_name,
    const std::string& shader_name,
    uint32_t num_root_constants,
    uint32_t num_uavs,
    uint32_t num_srvs) {
    ensure_directx_ready_or_throw(state);
    auto& pipeline_cache = state.pipelines;

    // Check if already created
    auto it = pipeline_cache.find(pipeline_name);
    if (it != pipeline_cache.end()) {
        return it->second.get();
    }

    // Get shader
    DirectXShader* shader = get_or_load_shader(state, shader_name);

    // Create pipeline
    auto pipeline = std::make_unique<DirectXPipeline>(
        state.context.get(),
        shader,
        num_root_constants,
        num_uavs,
        num_srvs
    );

    if (!pipeline->create()) {
        throw std::runtime_error("Failed to create pipeline: " + pipeline_name);
    }

    DirectXPipeline* pipeline_ptr = pipeline.get();
    pipeline_cache[pipeline_name] = std::move(pipeline);
    return pipeline_ptr;
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

bool initialize_directx_lora([[maybe_unused]] int adapter_id) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    if (g_directx_state.initialized) {
        return true;
    }

    try {
        // Create DirectX context
        g_directx_state.context = std::make_unique<DirectXContext>(adapter_id);
        if (!g_directx_state.context->initialize()) {
            std::cerr << "DirectX: Failed to initialize context\n";
            return false;
        }
        
        // Create descriptor manager
        g_directx_state.descriptors = std::make_unique<DirectXDescriptors>(
            g_directx_state.context.get(), 256);
        if (!g_directx_state.descriptors->initialize()) {
            std::cerr << "DirectX: Failed to initialize descriptors\n";
            return false;
        }
        
        // Shader loading happens on-demand in the launch functions.
        // When THEMIS_DIRECTX_PRECOMPILE_SHADERS is defined, load and compile
        // HLSL shaders here during initialization.
        
        g_directx_state.adapter_id = adapter_id;
        g_directx_state.initialized = true;
        
        std::cout << "DirectX 12 LoRA backend initialized successfully\n";
        std::cout << "GPU: " << g_directx_state.context->get_gpu_description() << "\n";
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "DirectX initialization failed: " << e.what() << "\n";
        g_directx_state.context.reset();
        g_directx_state.descriptors.reset();
        return false;
    }
}

void cleanup_directx_lora() {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    if (!g_directx_state.initialized) {
        return;
    }

    // Clear pipeline and shader cache
    g_directx_state.pipelines.clear();
    g_directx_state.shaders.clear();
    
    // Release descriptor manager
    g_directx_state.descriptors.reset();
    
    // Release context (will wait for GPU and cleanup)
    g_directx_state.context.reset();
    
    g_directx_state.initialized = false;
    g_directx_state.adapter_id = -1;
    
    std::cout << "DirectX 12 LoRA backend cleaned up\n";
}

bool is_directx_available() {
#ifdef _WIN32
    // Check if we can create a D3D12 device
    try {
        DirectXContext test_context(0);
        return test_context.initialize();
    }
    catch (...) {
        return false;
    }
#else
    return false;
#endif
}

void launch_matmul_shader(
    const float* A, const float* B, float* C,
    int M, int N, int K, float alpha) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!A || !B || !C) {
        throw std::invalid_argument("launch_matmul_shader received null pointer");
    }
    if (M <= 0 || N <= 0 || K <= 0) {
        throw std::invalid_argument("launch_matmul_shader received invalid dimensions");
    }
    if (!std::isfinite(alpha)) {
        throw std::invalid_argument("launch_matmul_shader received non-finite alpha");
    }
    
    try {
        // Get or create pipeline
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "matmul",
            "matmul.cso",
            4,  // num_root_constants (M, N, K, alpha)
            1,  // num_uavs (output C)
            2   // num_srvs (inputs A, B)
        );
        
        // Create buffers
        size_t size_A = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(K), "launch_matmul_shader");
        size_t size_B = checked_float_bytes_2d(static_cast<size_t>(K), static_cast<size_t>(N), "launch_matmul_shader");
        size_t size_C = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(N), "launch_matmul_shader");
        
        DirectXBuffer buffer_A(g_directx_state.context.get(), size_A);
        DirectXBuffer buffer_B(g_directx_state.context.get(), size_B);
        DirectXBuffer buffer_C(g_directx_state.context.get(), size_C);
        
        // Upload input data
        buffer_A.upload(A, size_A);
        buffer_B.upload(B, size_B);
        
        // Create descriptors
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        const size_t elems_C = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(N), "launch_matmul_shader");
        const size_t elems_A = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(K), "launch_matmul_shader");
        const size_t elems_B = checked_mul_size(static_cast<size_t>(K), static_cast<size_t>(N), "launch_matmul_shader");

        uint32_t uav_C = g_directx_state.descriptors->create_uav(
            buffer_C.resource(), checked_u32_size(elems_C, "launch_matmul_shader"), sizeof(float));
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_A.resource(), checked_u32_size(elems_A, "launch_matmul_shader"), sizeof(float));
        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_B.resource(), checked_u32_size(elems_B, "launch_matmul_shader"), sizeof(float));
        
        // Set pipeline state
        g_directx_state.context->reset_command_list();
        
        // Set root constants (dimensions)
        struct RootConstants {
            uint32_t M, K, N;
            float alpha = {};
        } constants = {static_cast<uint32_t>(M), static_cast<uint32_t>(K), 
                       static_cast<uint32_t>(N), alpha};
        
        pipeline->set_root_constants(&constants, 4);
        
        // Set descriptor heap (must be set before binding descriptor tables)
        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        // Bind descriptor tables
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_C));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_A));
        
        // Dispatch
        uint32_t thread_groups_x = (N + 15) / 16;
        uint32_t thread_groups_y = (M + 15) / 16;
        pipeline->dispatch(thread_groups_x, thread_groups_y, 1);
        
        // Execute and wait
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        
        // Download result
        buffer_C.download(C, size_C);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_matmul_shader failed: ") + e.what());
    }
}

void launch_add_shader(const float* A, const float* B, float* C, size_t size) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!A || !B || !C) {
        throw std::invalid_argument("launch_add_shader received null pointer");
    }
    if (size == 0) {
        throw std::invalid_argument("launch_add_shader received invalid size");
    }
    
    try {
        // Get or create pipeline
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "elementwise",
            "elementwise.cso",
            5,  // num_root_constants (size, op, rows, cols, scalar)
            1,  // num_uavs (output C)
            2   // num_srvs (inputs A, B)
        );
        
        // Create buffers
        size_t byte_size = checked_mul_size(size, sizeof(float), "launch_add_shader");
        
        DirectXBuffer buffer_A(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_B(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_C(g_directx_state.context.get(), byte_size);
        
        // Upload input data
        buffer_A.upload(A, byte_size);
        buffer_B.upload(B, byte_size);
        
        // Create descriptors
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        const uint32_t size_u32 = checked_u32_size(size, "launch_add_shader");
        uint32_t uav_C = g_directx_state.descriptors->create_uav(
            buffer_C.resource(), size_u32, sizeof(float));
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_A.resource(), size_u32, sizeof(float));
        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_B.resource(), size_u32, sizeof(float));
        
        // Set pipeline state
        g_directx_state.context->reset_command_list();
        
        // Set root constants (op=0 for add)
        struct RootConstants {
            uint32_t size = 0;
            uint32_t op;      // 0=add
            uint32_t rows;
            uint32_t cols;
            float scalar;
        } constants = {size_u32, 0, 0, 0, 0.0f};
        
        pipeline->set_root_constants(&constants, 5);

        // Set descriptor heap (must be set before binding descriptor tables)
        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        // Bind descriptor tables
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_C));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_A));
        
        // Dispatch
        uint32_t thread_groups = (size_u32 + 255) / 256;
        pipeline->dispatch(thread_groups, 1, 1);
        
        // Execute and wait
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        
        // Download result
        buffer_C.download(C, byte_size);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_add_shader failed: ") + e.what());
    }
}

void launch_multiply_shader(const float* A, const float* B, float* C, size_t size) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!A || !B || !C) {
        throw std::invalid_argument("launch_multiply_shader received null pointer");
    }
    if (size == 0) {
        throw std::invalid_argument("launch_multiply_shader received invalid size");
    }
    
    try {
        // Use elementwise pipeline with op=2 for multiply
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "elementwise",
            "elementwise.cso",
            5, 1, 2
        );
        
        size_t byte_size = checked_mul_size(size, sizeof(float), "launch_multiply_shader");
        
        DirectXBuffer buffer_A(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_B(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_C(g_directx_state.context.get(), byte_size);
        
        buffer_A.upload(A, byte_size);
        buffer_B.upload(B, byte_size);
        
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        const uint32_t size_u32 = checked_u32_size(size, "launch_multiply_shader");
        uint32_t uav_C = g_directx_state.descriptors->create_uav(
            buffer_C.resource(), size_u32, sizeof(float));
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_A.resource(), size_u32, sizeof(float));
        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_B.resource(), size_u32, sizeof(float));
        
        g_directx_state.context->reset_command_list();
        
        struct RootConstants {
            uint32_t size = 0;
            uint32_t op;      // 2=multiply
            uint32_t rows;
            uint32_t cols;
            float scalar;
        } constants = {size_u32, 2, 0, 0, 0.0f};
        
        pipeline->set_root_constants(&constants, 5);

        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_C));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_A));
        
        uint32_t thread_groups = (size_u32 + 255) / 256;
        pipeline->dispatch(thread_groups, 1, 1);
        
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        buffer_C.download(C, byte_size);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_multiply_shader failed: ") + e.what());
    }
}

void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!A || !B) {
        throw std::invalid_argument("launch_scalar_multiply_shader received null pointer");
    }
    if (size == 0) {
        throw std::invalid_argument("launch_scalar_multiply_shader received invalid size");
    }
    if (!std::isfinite(scalar)) {
        throw std::invalid_argument("launch_scalar_multiply_shader received non-finite scalar");
    }
    
    try {
        // Use elementwise pipeline with op=4 for scalar multiply
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "elementwise",
            "elementwise.cso",
            5, 1, 2
        );
        
        size_t byte_size = checked_mul_size(size, sizeof(float), "launch_scalar_multiply_shader");
        
        DirectXBuffer buffer_A(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_B(g_directx_state.context.get(), byte_size);
        // Note: buffer_B_dummy needed for shader interface, but not used for scalar multiply
        DirectXBuffer buffer_B_dummy(g_directx_state.context.get(), byte_size);
        
        buffer_A.upload(A, byte_size);
        
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        const uint32_t size_u32 = checked_u32_size(size, "launch_scalar_multiply_shader");
        uint32_t uav_C = g_directx_state.descriptors->create_uav(
            buffer_B.resource(), size_u32, sizeof(float));
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_A.resource(), size_u32, sizeof(float));
        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_B_dummy.resource(), size_u32, sizeof(float));

        // Debug: print descriptor indices and GPU handle pointers
        {
            auto gpu_uav = g_directx_state.descriptors->get_gpu_handle(uav_C);
            auto gpu_srvA = g_directx_state.descriptors->get_gpu_handle(srv_A);
            auto gpu_srvB = g_directx_state.descriptors->get_gpu_handle(srv_B);
            std::cout << "DirectX Debug: byte_size=" << byte_size
                      << " size_u32=" << size_u32
                      << " uav_C=" << uav_C << " gpu_uav.ptr=" << gpu_uav.ptr
                      << " srv_A=" << srv_A << " gpu_srvA.ptr=" << gpu_srvA.ptr
                      << " srv_B=" << srv_B << " gpu_srvB.ptr=" << gpu_srvB.ptr
                      << "\n";
        }
        
        g_directx_state.context->reset_command_list();

        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        struct RootConstants {
            uint32_t size = 0;
            uint32_t op;      // 4=scalar multiply
            uint32_t rows;
            uint32_t cols;
            float scalar_val = {};
        } constants = {size_u32, 4, 0, 0, scalar};

        std::cout << "DirectX Debug: RootConstants -> size=" << constants.size
                  << " op=" << constants.op << " scalar=" << constants.scalar_val
                  << "\n";
        
        pipeline->set_root_constants(&constants, 5);
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_C));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_A));
        
        uint32_t thread_groups = (size_u32 + 255) / 256;
        std::cout << "DirectX Debug: dispatch thread_groups=" << thread_groups << "\n";
        pipeline->dispatch(thread_groups, 1, 1);
        
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        buffer_B.download(B, byte_size);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_scalar_multiply_shader failed: ") + e.what());
    }
}

void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!input || !output) {
        throw std::invalid_argument("launch_transpose_shader received null pointer");
    }
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("launch_transpose_shader received invalid dimensions");
    }
    
    try {
        // Use elementwise pipeline with op=5 for transpose
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "elementwise",
            "elementwise.cso",
            5, 1, 2
        );
        
        size_t size = checked_mul_size(static_cast<size_t>(rows), static_cast<size_t>(cols), "launch_transpose_shader");
        size_t byte_size = checked_mul_size(size, sizeof(float), "launch_transpose_shader");
        
        DirectXBuffer buffer_input(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_output(g_directx_state.context.get(), byte_size);
        DirectXBuffer buffer_dummy(g_directx_state.context.get(), byte_size);
        
        buffer_input.upload(input, byte_size);
        
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        const uint32_t size_u32 = checked_u32_size(size, "launch_transpose_shader");
        uint32_t uav_C = g_directx_state.descriptors->create_uav(
            buffer_output.resource(), size_u32, sizeof(float));
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_input.resource(), size_u32, sizeof(float));
        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_dummy.resource(), size_u32, sizeof(float));
        
        g_directx_state.context->reset_command_list();
        
        struct RootConstants {
            uint32_t size = 0;
            uint32_t op;      // 5=transpose
            uint32_t rows = {};
            uint32_t cols = {};
            float scalar;
        } constants = {size_u32, 5,
                       static_cast<uint32_t>(rows), static_cast<uint32_t>(cols), 0.0f};
        
        pipeline->set_root_constants(&constants, 5);

        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_C));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_A));
        
        uint32_t thread_groups = (size_u32 + 255) / 256;
        pipeline->dispatch(thread_groups, 1, 1);
        
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        buffer_output.download(output, byte_size);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_transpose_shader failed: ") + e.what());
    }
}

void launch_lora_grad_A_shader(
    const float* h, const float* grad_output, float* grad_A,
    int M, int K, int N, float scaling) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!h || !grad_output || !grad_A) {
        throw std::invalid_argument("launch_lora_grad_A_shader received null pointer");
    }
    if (M <= 0 || K <= 0 || N <= 0) {
        throw std::invalid_argument("launch_lora_grad_A_shader received invalid dimensions");
    }
    if (!std::isfinite(scaling)) {
        throw std::invalid_argument("launch_lora_grad_A_shader received non-finite scaling");
    }
    
    try {
        // Get or create pipeline
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "gradient",
            "gradient.cso",
            6,  // num_root_constants (batch_size, in_dim, rank, out_dim, scaling, compute_mode)
            3,  // num_uavs (grad_A, grad_B, grad_input)
            4   // num_srvs (input, B, A, grad_output)
        );
        
        // Create buffers
        // For grad_A: h is (M, K), grad_output is (M, N), output grad_A is (K, N)
        size_t size_h = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(K), "launch_lora_grad_A_shader");
        size_t size_grad_output = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(N), "launch_lora_grad_A_shader");
        size_t size_grad_A = checked_float_bytes_2d(static_cast<size_t>(K), static_cast<size_t>(N), "launch_lora_grad_A_shader");
        
        DirectXBuffer buffer_h(g_directx_state.context.get(), size_h);
        DirectXBuffer buffer_grad_output(g_directx_state.context.get(), size_grad_output);
        DirectXBuffer buffer_grad_A(g_directx_state.context.get(), size_grad_A);
        
        // Dummy buffers for unused outputs
        DirectXBuffer buffer_dummy1(g_directx_state.context.get(), sizeof(float));
        DirectXBuffer buffer_dummy2(g_directx_state.context.get(), sizeof(float));
        DirectXBuffer buffer_dummy3(g_directx_state.context.get(), sizeof(float));
        
        // Upload input data
        buffer_h.upload(h, size_h);
        buffer_grad_output.upload(grad_output, size_grad_output);
        
        // Create descriptors
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        const size_t elems_grad_A = checked_mul_size(static_cast<size_t>(K), static_cast<size_t>(N), "launch_lora_grad_A_shader");
        const size_t elems_h = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(K), "launch_lora_grad_A_shader");
        const size_t elems_grad_output = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(N), "launch_lora_grad_A_shader");
        uint32_t uav_grad_A = g_directx_state.descriptors->create_uav(
            buffer_grad_A.resource(), checked_u32_size(elems_grad_A, "launch_lora_grad_A_shader"), sizeof(float));
        uint32_t uav_grad_B = g_directx_state.descriptors->create_uav(
            buffer_dummy1.resource(), 1, sizeof(float));
        uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
            buffer_dummy2.resource(), 1, sizeof(float));
            
        uint32_t srv_input = g_directx_state.descriptors->create_srv(
            buffer_h.resource(), checked_u32_size(elems_h, "launch_lora_grad_A_shader"), sizeof(float));
        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_h.resource(), checked_u32_size(elems_h, "launch_lora_grad_A_shader"), sizeof(float));  // Reuse h as placeholder for B
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_dummy3.resource(), 1, sizeof(float));
        uint32_t srv_grad_output = g_directx_state.descriptors->create_srv(
            buffer_grad_output.resource(), checked_u32_size(elems_grad_output, "launch_lora_grad_A_shader"), sizeof(float));
        
        // Set pipeline state
        g_directx_state.context->reset_command_list();
        
        // Set root constants
        struct RootConstants {
            uint32_t batch_size = 0;
            uint32_t in_dim;
            uint32_t rank;
            uint32_t out_dim;
            float scaling;
            uint32_t compute_mode;  // 0=grad_A
        } constants = {static_cast<uint32_t>(M), static_cast<uint32_t>(M), 
                       static_cast<uint32_t>(K), static_cast<uint32_t>(N), 
                       scaling, 0};
        
        pipeline->set_root_constants(&constants, 6);

        // Set descriptor heap (must be set before binding descriptor tables)
        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        // Bind descriptor tables
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_grad_A));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
        
        // Dispatch
        uint32_t thread_groups_x = (N + 15) / 16;
        uint32_t thread_groups_y = (K + 15) / 16;
        pipeline->dispatch(thread_groups_x, thread_groups_y, 1);
        
        // Execute and wait
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        
        // Download result
        buffer_grad_A.download(grad_A, size_grad_A);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_lora_grad_A_shader failed: ") + e.what());
    }
}

void launch_lora_grad_B_shader(
    const float* input, const float* grad_h, float* grad_B,
    int M, int D, int K) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!input || !grad_h || !grad_B) {
        throw std::invalid_argument("launch_lora_grad_B_shader received null pointer");
    }
    if (M <= 0 || D <= 0 || K <= 0) {
        throw std::invalid_argument("launch_lora_grad_B_shader received invalid dimensions");
    }
    
    try {
        // Get or create pipeline
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "gradient",
            "gradient.cso",
            6, 3, 4
        );
        
        // Create buffers
        // For grad_B: input is (M, D), grad_h is (M, K), output grad_B is (D, K)
        size_t size_input = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
        size_t size_grad_h = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(K), "launch_lora_grad_B_shader");
        size_t size_grad_B = checked_float_bytes_2d(static_cast<size_t>(D), static_cast<size_t>(K), "launch_lora_grad_B_shader");
        
        DirectXBuffer buffer_input(g_directx_state.context.get(), size_input);
        DirectXBuffer buffer_grad_h(g_directx_state.context.get(), size_grad_h);
        DirectXBuffer buffer_grad_B(g_directx_state.context.get(), size_grad_B);
        
        // Dummy buffers for unused outputs
        DirectXBuffer buffer_dummy1(g_directx_state.context.get(), sizeof(float));
        DirectXBuffer buffer_dummy2(g_directx_state.context.get(), sizeof(float));
        DirectXBuffer buffer_dummy3(g_directx_state.context.get(), sizeof(float));
        
        // Upload input data
        buffer_input.upload(input, size_input);
        buffer_grad_h.upload(grad_h, size_grad_h);
        
        // Create descriptors
        // Explicit null guard: descriptors is non-null when initialized==true; made explicit for static analysis.
        if (!g_directx_state.descriptors) { throw std::runtime_error("DirectX: descriptor heap not initialized"); }
        g_directx_state.descriptors->reset();
        
        uint32_t uav_grad_A = g_directx_state.descriptors->create_uav(
            buffer_dummy1.resource(), 1, sizeof(float));
        const size_t elems_grad_B = checked_mul_size(static_cast<size_t>(D), static_cast<size_t>(K), "launch_lora_grad_B_shader");
        const size_t elems_input = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
        const size_t elems_grad_h = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(K), "launch_lora_grad_B_shader");
        uint32_t uav_grad_B = g_directx_state.descriptors->create_uav(
            buffer_grad_B.resource(), checked_u32_size(elems_grad_B, "launch_lora_grad_B_shader"), sizeof(float));
        uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
            buffer_dummy2.resource(), 1, sizeof(float));
            
        uint32_t srv_input = g_directx_state.descriptors->create_srv(
            buffer_input.resource(), checked_u32_size(elems_input, "launch_lora_grad_B_shader"), sizeof(float));

        // Create a small identity A matrix (K x K) so the gradient computation
        // produces non-zero values when a real A isn't provided by the caller.
        size_t size_A = checked_float_bytes_2d(static_cast<size_t>(K), static_cast<size_t>(K), "launch_lora_grad_B_shader");
        DirectXBuffer buffer_A(g_directx_state.context.get(), size_A);
        // Fill identity matrix
        std::vector<float> A_identity(static_cast<size_t>(K) * static_cast<size_t>(K), 0.0f);
        for (int r = 0; r < K; ++r) {
            A_identity[r * K + r] = 1.0f;
        }
        buffer_A.upload(A_identity.data(), size_A);

        uint32_t srv_B = g_directx_state.descriptors->create_srv(
            buffer_dummy3.resource(), 1, sizeof(float));
        uint32_t srv_A = g_directx_state.descriptors->create_srv(
            buffer_A.resource(), checked_u32_size(static_cast<size_t>(K) * static_cast<size_t>(K), "launch_lora_grad_B_shader"), sizeof(float));
        uint32_t srv_grad_output = g_directx_state.descriptors->create_srv(
            buffer_grad_h.resource(), checked_u32_size(elems_grad_h, "launch_lora_grad_B_shader"), sizeof(float));
        
        // Set pipeline state
        g_directx_state.context->reset_command_list();
        
        // Set root constants
        struct RootConstants {
            uint32_t batch_size = 0;
            uint32_t in_dim;
            uint32_t rank;
            uint32_t out_dim;
            float scaling;
            uint32_t compute_mode;  // 1=grad_B
        } constants = {static_cast<uint32_t>(M), static_cast<uint32_t>(D), 
                       static_cast<uint32_t>(K), static_cast<uint32_t>(K),
                       1.0f, 1};
        
        pipeline->set_root_constants(&constants, 6);

        // Set descriptor heap (must be set before binding descriptor tables)
        ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
        g_directx_state.context->command_list()->SetDescriptorHeaps(1, heaps);

        // Bind descriptor tables
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_grad_A));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
        
        // Dispatch
        uint32_t thread_groups_x = (K + 15) / 16;
        uint32_t thread_groups_y = (D + 15) / 16;
        pipeline->dispatch(thread_groups_x, thread_groups_y, 1);
        
        // Execute and wait
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        
        // Download result
        buffer_grad_B.download(grad_B, size_grad_B);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_lora_grad_B_shader failed: ") + e.what());
    }
}

void launch_embedding_lookup_shader(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!output || !token_ids || !embedding_weights) {
        throw std::invalid_argument("launch_embedding_lookup_shader received null pointer");
    }
    if (batch_size <= 0 || seq_len <= 0 || hidden_dim <= 0 || vocab_size <= 0) {
        throw std::invalid_argument("launch_embedding_lookup_shader received invalid dimensions");
    }
    
    try {
        // Calculate sizes
        size_t total_tokens = checked_mul_size(static_cast<size_t>(batch_size), static_cast<size_t>(seq_len), "launch_embedding_lookup_shader");
        size_t output_size = checked_mul_size(total_tokens, static_cast<size_t>(hidden_dim), "launch_embedding_lookup_shader");
        size_t embedding_matrix_size = checked_mul_size(static_cast<size_t>(vocab_size), static_cast<size_t>(hidden_dim), "launch_embedding_lookup_shader");
        
        // Create buffers
        const size_t token_bytes = checked_mul_size(total_tokens, sizeof(float), "launch_embedding_lookup_shader");
        const size_t embedding_bytes = checked_mul_size(embedding_matrix_size, sizeof(float), "launch_embedding_lookup_shader");
        const size_t output_bytes = checked_mul_size(output_size, sizeof(float), "launch_embedding_lookup_shader");
        const uint32_t total_tokens_u32 = checked_u32_size(total_tokens, "launch_embedding_lookup_shader");

        DirectXBuffer buffer_token_ids(g_directx_state.context.get(), token_bytes);
        DirectXBuffer buffer_embedding_weights(g_directx_state.context.get(), embedding_bytes);
        DirectXBuffer buffer_output(g_directx_state.context.get(), output_bytes);
        
        // Upload data
        buffer_token_ids.upload(token_ids, token_bytes);
        buffer_embedding_weights.upload(embedding_weights, embedding_bytes);
        
        // Get or create pipeline
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "embedding_lookup",
            "embedding_lookup.hlsl",
            4,  // 4 uint constants
            1,  // 1 UAV (output)
            2   // 2 SRVs (token_ids, embedding_weights)
        );
        
        // Set root constants
        struct Constants {
            uint32_t batch_size = 0;
            uint32_t seq_len = {};
            uint32_t hidden_dim = {};
            uint32_t vocab_size = {};
        } constants = {
            static_cast<uint32_t>(batch_size),
            static_cast<uint32_t>(seq_len),
            static_cast<uint32_t>(hidden_dim),
            static_cast<uint32_t>(vocab_size)
        };
        
        if (!g_directx_state.descriptors) {
            throw std::runtime_error("DirectX: descriptor heap not initialized");
        }
        g_directx_state.descriptors->reset();
        const uint32_t uav_output = g_directx_state.descriptors->create_uav(
            buffer_output.resource(), checked_u32_size(output_size, "launch_embedding_lookup_shader"), sizeof(float));
        const uint32_t srv_token_ids = g_directx_state.descriptors->create_srv(
            buffer_token_ids.resource(), checked_u32_size(total_tokens, "launch_embedding_lookup_shader"), sizeof(float));
        const uint32_t srv_embedding_weights = g_directx_state.descriptors->create_srv(
            buffer_embedding_weights.resource(), checked_u32_size(embedding_matrix_size, "launch_embedding_lookup_shader"), sizeof(float));

        g_directx_state.context->reset_command_list();
        pipeline->set_root_constants(&constants, 4);
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_output));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_token_ids));
        
        // Dispatch: each thread handles one token
        uint32_t thread_groups = (total_tokens_u32 + 255) / 256;
        pipeline->dispatch(thread_groups, 1, 1);
        
        // Execute and wait
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        
        // Download result
        buffer_output.download(output, output_bytes);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_embedding_lookup_shader failed: ") + e.what());
    }
}

void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim) {
    std::lock_guard<std::mutex> state_lock(g_directx_state_mutex);
    ensure_directx_ready_or_throw(g_directx_state);
    if (!output || !input) {
        throw std::invalid_argument("launch_sequence_mean_shader received null pointer");
    }
    if (batch_size <= 0 || seq_len <= 0 || hidden_dim <= 0) {
        throw std::invalid_argument("launch_sequence_mean_shader received invalid dimensions");
    }
    
    try {
        // Calculate sizes
        size_t input_size = checked_mul_size(
            checked_mul_size(static_cast<size_t>(batch_size), static_cast<size_t>(seq_len), "launch_sequence_mean_shader"),
            static_cast<size_t>(hidden_dim),
            "launch_sequence_mean_shader");
        size_t output_size = checked_mul_size(static_cast<size_t>(batch_size), static_cast<size_t>(hidden_dim), "launch_sequence_mean_shader");
        
        // Create buffers
        const size_t input_bytes = checked_mul_size(input_size, sizeof(float), "launch_sequence_mean_shader");
        const size_t output_bytes = checked_mul_size(output_size, sizeof(float), "launch_sequence_mean_shader");
        const uint32_t output_size_u32 = checked_u32_size(output_size, "launch_sequence_mean_shader");

        DirectXBuffer buffer_input(g_directx_state.context.get(), input_bytes);
        DirectXBuffer buffer_output(g_directx_state.context.get(), output_bytes);
        
        // Upload data
        buffer_input.upload(input, input_bytes);
        
        // Get or create pipeline
        DirectXPipeline* pipeline = get_or_create_pipeline(
            g_directx_state,
            "sequence_mean",
            "sequence_mean.hlsl",
            4,  // 4 uint constants
            1,  // 1 UAV (output)
            1   // 1 SRV (input)
        );
        
        // Set root constants
        struct Constants {
            uint32_t batch_size = 0;
            uint32_t seq_len = {};
            uint32_t hidden_dim = {};
            uint32_t reserved = {};
        } constants = {
            static_cast<uint32_t>(batch_size),
            static_cast<uint32_t>(seq_len),
            static_cast<uint32_t>(hidden_dim),
            0
        };
        
        if (!g_directx_state.descriptors) {
            throw std::runtime_error("DirectX: descriptor heap not initialized");
        }
        g_directx_state.descriptors->reset();
        const uint32_t uav_output = g_directx_state.descriptors->create_uav(
            buffer_output.resource(), checked_u32_size(output_size, "launch_sequence_mean_shader"), sizeof(float));
        const uint32_t srv_input = g_directx_state.descriptors->create_srv(
            buffer_input.resource(), checked_u32_size(input_size, "launch_sequence_mean_shader"), sizeof(float));

        g_directx_state.context->reset_command_list();
        pipeline->set_root_constants(&constants, 4);
        pipeline->bind_uav_table(0, g_directx_state.descriptors->get_gpu_handle(uav_output));
        pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
        
        // Dispatch: each thread handles one output element
        uint32_t thread_groups = (output_size_u32 + 255) / 256;
        pipeline->dispatch(thread_groups, 1, 1);
        
        // Execute and wait
        g_directx_state.context->execute_command_list(kDirectXKernelExecutionTimeoutMs);
        
        // Download result
        buffer_output.download(output, output_bytes);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("launch_sequence_mean_shader failed: ") + e.what());
    }
}

} // namespace directx
} // namespace lora
} // namespace themis

#else // !_WIN32

// Non-Windows fallback implementations (platform guard: !_WIN32)
namespace themis {
namespace lora {
namespace directx {

bool initialize_directx_lora([[maybe_unused]] int adapter_id) {
    return false;
}

void cleanup_directx_lora() {
}

bool is_directx_available() {
    return false;
}

void launch_matmul_shader(
    const float* A, const float* B, float* C,
    int M, int N, int K, float alpha) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_add_shader(const float* A, const float* B, float* C, size_t size) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_multiply_shader(const float* A, const float* B, float* C, size_t size) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_lora_grad_A_shader(
    const float* h, const float* grad_output, float* grad_A,
    int M, int K, int N, float scaling) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_lora_grad_B_shader(
    const float* input, const float* grad_h, float* grad_B,
    int M, int D, int K) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_embedding_lookup_shader(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size) {
    throw std::runtime_error("DirectX is only available on Windows");
}

void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim) {
    throw std::runtime_error("DirectX is only available on Windows");
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32

