/**
 * @file directx_backend_full.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// DirectX 12 Compute Shaders Backend (Windows only)
// Provides GPU acceleration using DirectX 12 Compute Shaders
// Native Windows GPU acceleration for NVIDIA, AMD, Intel GPUs

#include "acceleration/graphics_backends.h"

#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
// Include the central directx context header which provides D3D includes and a ComPtr fallback
#include "llm/lora_framework/directx_context.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <string>
#include <cstring>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace themis {
namespace acceleration {

// ============================================================================
// DirectX 12 Helper Macros
// ============================================================================

#define DX_CHECK(call) \
    do { \
        HRESULT _hr = (call); \
        if (FAILED(_hr)) { \
            std::cerr << "[DirectX] Error in " << __FILE__ << ":" << __LINE__ \
                      << " - HRESULT: 0x" << std::hex << _hr << std::dec << std::endl; \
            return false; \
        } \
    } while(0)

#define DX_CHECK_THROW(call) \
    do { \
        HRESULT _hr = (call); \
        if (FAILED(_hr)) { \
            char _buf[64]; \
            snprintf(_buf, sizeof(_buf), "DirectX error: HRESULT 0x%08X", \
                     static_cast<unsigned int>(_hr)); \
            throw std::runtime_error(_buf); \
        } \
    } while(0)

// ============================================================================
// Shader file-loading utilities
// ============================================================================

namespace {

/**
 * @brief Probe candidate directories for a shader file with the given stem.
 *
 * Search order for each root:
 *   1. <root>/shaders/vector_index/<stem>.cso   (pre-compiled, preferred)
 *   2. <root>/shaders/vector_index/<stem>.hlsl
 *   3. <root>/bin/shaders/vector_index/<stem>.cso
 *   4. <root>/src/acceleration/directx/shaders/<stem>.cso
 *   5. <root>/src/acceleration/directx/shaders/<stem>.hlsl
 *
 * @param stem  Base file name without extension (e.g. "l2_distance").
 * @return Absolute path to the first matching file, or empty string if none found.
 */
static std::string find_ann_shader_path(const std::string& stem)
{
    namespace fs = std::filesystem;

    // Build candidate roots: cwd + up to 4 parent directories
    std::vector<fs::path> roots;
    fs::path cur = fs::current_path();
    for (int i = 0; i < 5 && cur.has_parent_path(); ++i) {
        roots.push_back(cur);
        cur = cur.parent_path();
    }
    // Also probe the directory containing the repository root (look for CMakeLists.txt)
    fs::path probe = fs::current_path();
    for (int i = 0; i < 8 && probe.has_parent_path(); ++i) {
        if (fs::exists(probe / "CMakeLists.txt")) {
            roots.push_back(probe);
            break;
        }
        probe = probe.parent_path();
    }

    for (const auto& root : roots) {
        std::vector<fs::path> candidates = {
            root / "shaders" / "vector_index" / (stem + ".cso"),
            root / "shaders" / "vector_index" / (stem + ".hlsl"),
            root / "bin" / "shaders" / "vector_index" / (stem + ".cso"),
            root / "src" / "acceleration" / "directx" / "shaders" / (stem + ".cso"),
            root / "src" / "acceleration" / "directx" / "shaders" / (stem + ".hlsl"),
        };
        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                return c.string();
            }
        }
    }
    return {};
}

/**
 * @brief Read a binary file into a byte vector.
 *
 * @param path  Absolute path to the file.
 * @return File contents, or empty vector on failure.
 */
static std::vector<uint8_t> read_binary_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    return buf;
}

/**
 * @brief Read a text file into a string.
 *
 * @param path  Absolute path to the file.
 * @return File contents, or empty string on failure.
 */
static std::string read_text_file(const std::string& path)
{
    std::ifstream f(path);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), {}};
}

} // anonymous namespace

// ============================================================================
// DirectXVectorBackend::DirectXVectorBackendImpl
// (nested class registered in the DirectXVectorBackend interface)
// ============================================================================

class DirectXVectorBackend::DirectXVectorBackendImpl {
public:
    // DX12 core objects
    ComPtr<ID3D12Device>              device_;
    ComPtr<ID3D12CommandQueue>        commandQueue_;
    ComPtr<ID3D12CommandAllocator>    commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence>               fence_;
    HANDLE                            fenceEvent_ = nullptr;
    UINT64                            fenceValue_ = 0;

    // Compute pipeline objects
    ComPtr<ID3D12RootSignature>       rootSignature_;
    ComPtr<ID3D12PipelineState>       l2Pipeline_;
    ComPtr<ID3D12PipelineState>       cosinePipeline_;

    std::string adapterName_;

    ~DirectXVectorBackendImpl() {
        if (fenceEvent_) {
            CloseHandle(fenceEvent_);
            fenceEvent_ = nullptr;
        }
    }

    // ------------------------------------------------------------------
    // Public API used by DirectXVectorBackend
    // ------------------------------------------------------------------

    std::string deviceName() const { return adapterName_; }

    bool initialize() {
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debugCtrl;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugCtrl)))) {
            debugCtrl->EnableDebugLayer();
        }
#endif
        // Find best hardware adapter (largest dedicated VRAM, skip software)
        ComPtr<IDXGIFactory6> factory;
        DX_CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> bestAdapter;
        SIZE_T bestMem = 0;

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
              continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                            __uuidof(ID3D12Device), nullptr))) {
                if (desc.DedicatedVideoMemory >= bestMem) {
                    bestMem     = desc.DedicatedVideoMemory;
                    bestAdapter = adapter;
                    // Store adapter name
                    char narrow[128] = {};
                    int converted = WideCharToMultiByte(
                        CP_UTF8, 0, desc.Description, -1,
                        narrow, sizeof(narrow) - 1, nullptr, nullptr);
                    adapterName_ = (converted > 0) ? narrow : "Unknown DirectX 12 Adapter";
                }
            }
        }

        if (!bestAdapter) {
            std::cerr << "[DirectX] No D3D12-capable hardware adapter found" << std::endl;
            return false;
        }

        DX_CHECK(D3D12CreateDevice(bestAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                   IID_PPV_ARGS(&device_)));

        // Compute command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        DX_CHECK(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));

        // Command allocator + command list
        DX_CHECK(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                  IID_PPV_ARGS(&commandAllocator_)));
        DX_CHECK(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                             commandAllocator_.Get(), nullptr,
                                             IID_PPV_ARGS(&commandList_)));
        commandList_->Close();

        // Fence for CPU/GPU synchronisation
        DX_CHECK(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
        fenceValue_ = 1;
        fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_) {
          return false;
        }

        if (!createRootSignature()) {
          return false;
        }
        if (!createComputePipelines()) {
          return false;
        }

        return true;
    }

    void shutdown() {
        waitForGPU();  // flush any in-flight work before releasing resources
        l2Pipeline_.Reset();
        cosinePipeline_.Reset();
        rootSignature_.Reset();
        if (fenceEvent_) { CloseHandle(fenceEvent_); fenceEvent_ = nullptr; }
        fence_.Reset();
        commandList_.Reset();
        commandAllocator_.Reset();
        commandQueue_.Reset();
        device_.Reset();
    }

    // ------------------------------------------------------------------
    // computeDistances: dispatch the appropriate HLSL kernel and readback
    // ------------------------------------------------------------------
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2)
    {
        const SIZE_T queryBytes   = numQueries * dim * sizeof(float);
        const SIZE_T vectorBytes  = numVectors * dim * sizeof(float);
        const SIZE_T distBytes    = numQueries * numVectors * sizeof(float);

        // GPU default-heap UAV buffers
        auto queryBuf    = createUAVBuffer(queryBytes);
        auto vectorBuf   = createUAVBuffer(vectorBytes);
        auto distanceBuf = createUAVBuffer(distBytes);

        // CPU-writable upload buffers
        auto queryUpload  = createUploadBuffer(queryBytes);
        auto vectorUpload = createUploadBuffer(vectorBytes);

        // CPU-readable readback buffer
        auto distReadback = createReadbackBuffer(distBytes);

        // Copy host data into upload buffers
        uploadData(queryUpload.Get(),  queries,  queryBytes);
        uploadData(vectorUpload.Get(), vectors, vectorBytes);

        // Record commands
        DX_CHECK_THROW(commandAllocator_->Reset());
        DX_CHECK_THROW(commandList_->Reset(commandAllocator_.Get(), nullptr));

        // Copy upload → UAV buffers via resource-state transitions
        transitionBarrier(queryBuf.Get(),
                          D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_STATE_COPY_DEST);
        transitionBarrier(vectorBuf.Get(),
                          D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_STATE_COPY_DEST);
        commandList_->CopyResource(queryBuf.Get(),  queryUpload.Get());
        commandList_->CopyResource(vectorBuf.Get(), vectorUpload.Get());

        // Transition UAV buffers to UAV state for compute
        transitionBarrier(queryBuf.Get(),
                          D3D12_RESOURCE_STATE_COPY_DEST,
                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        transitionBarrier(vectorBuf.Get(),
                          D3D12_RESOURCE_STATE_COPY_DEST,
                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        transitionBarrier(distanceBuf.Get(),
                          D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Bind pipeline and dispatch
        commandList_->SetComputeRootSignature(rootSignature_.Get());
        commandList_->SetPipelineState(useL2 ? l2Pipeline_.Get() : cosinePipeline_.Get());
        commandList_->SetComputeRootUnorderedAccessView(
            0, queryBuf->GetGPUVirtualAddress());
        commandList_->SetComputeRootUnorderedAccessView(
            1, vectorBuf->GetGPUVirtualAddress());
        commandList_->SetComputeRootUnorderedAccessView(
            2, distanceBuf->GetGPUVirtualAddress());

        UINT constants[4] = {
            static_cast<UINT>(numQueries),
            static_cast<UINT>(numVectors),
            static_cast<UINT>(dim),
            0u
        };
        commandList_->SetComputeRoot32BitConstants(3, 4, constants, 0);

        UINT groupsX = static_cast<UINT>((numVectors + 15) / 16);
        UINT groupsY = static_cast<UINT>((numQueries  + 15) / 16);
        commandList_->Dispatch(groupsX, groupsY, 1);

        // Transition distance buffer for copy to readback
        transitionBarrier(distanceBuf.Get(),
                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                          D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList_->CopyResource(distReadback.Get(), distanceBuf.Get());

        DX_CHECK_THROW(commandList_->Close());

        // Execute and wait
        ID3D12CommandList* lists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(1, lists);
        waitForGPU();

        // Readback results
        std::vector<float> result(numQueries * numVectors);
        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, distBytes };
        DX_CHECK_THROW(distReadback->Map(0, &readRange, &mapped));
        std::memcpy(result.data(), mapped, distBytes);
        D3D12_RANGE writeRange = { 0, 0 };
        distReadback->Unmap(0, &writeRange);

        return result;
    }

    // ------------------------------------------------------------------
    // batchKnnSearch: GPU distance computation + CPU top-k selection
    // ------------------------------------------------------------------
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2)
    {
        auto distances = computeDistances(
            queries, numQueries, dim, vectors, numVectors, useL2);

        // Guard against an unexpectedly sized result from computeDistances
        if (distances.size() != numQueries * numVectors) {
            throw std::runtime_error(
                "[DirectX] batchKnnSearch: computeDistances returned unexpected size "
                "(expected " + std::to_string(numQueries * numVectors) +
                ", got " + std::to_string(distances.size()) + ")");
        }

        const size_t actualK = std::min(k, numVectors);
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);

        for (size_t q = 0; q < numQueries; ++q) {
            const float* row = distances.data() + q * numVectors;

            // Max-heap of size k: (distance, index)
            using Pair = std::pair<float, uint32_t>;
            std::priority_queue<Pair> heap;
            for (size_t v = 0; v < numVectors; ++v) {
                heap.push({ row[v], static_cast<uint32_t>(v) });
                if (heap.size() > actualK) {
                  heap.pop();
                }
            }

            // Drain heap in ascending distance order
            results[q].resize(heap.size());
            for (size_t i = heap.size(); i > 0; --i) {
                results[q][i - 1] = { heap.top().second, heap.top().first };
                heap.pop();
            }
        }
        return results;
    }

private:
    // ------------------------------------------------------------------
    // Root signature: 3 UAV root descriptors + 4 inline 32-bit constants
    // ------------------------------------------------------------------
    bool createRootSignature() {
        D3D12_ROOT_PARAMETER params[4] = {};

        for (UINT i = 0; i < 3; ++i) {
            params[i].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
            params[i].Descriptor.ShaderRegister = i;
            params[i].Descriptor.RegisterSpace  = 0;
            params[i].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
        }
        params[3].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[3].Constants.ShaderRegister = 0;
        params[3].Constants.RegisterSpace  = 0;
        params[3].Constants.Num32BitValues = 4;
        params[3].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = 4;
        desc.pParameters   = params;
        desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> blob, error;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                                  &blob, &error);
        if (FAILED(hr)) {
            if (error) std::cerr << "[DirectX] Root signature error: "
                                 << static_cast<char*>(error->GetBufferPointer()) << std::endl;
            return false;
        }
        DX_CHECK(device_->CreateRootSignature(0, blob->GetBufferPointer(),
                                               blob->GetBufferSize(),
                                               IID_PPV_ARGS(&rootSignature_)));
        return true;
    }

    // ------------------------------------------------------------------
    // Compile HLSL shaders at runtime and create compute pipeline states
    // ------------------------------------------------------------------
    bool createComputePipelines() {
        // Helper: create a pipeline state from an HLSL source string.
        auto compilePipelineFromSource = [&](const std::string& hlsl,
                                              const char*       debugName,
                                              ComPtr<ID3D12PipelineState>& pso) -> bool {
            ComPtr<ID3DBlob> shaderBlob, errorBlob;
            UINT flags = 0;
#ifdef _DEBUG
            flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
            HRESULT hr = D3DCompile(hlsl.c_str(), hlsl.size(), debugName,
                                    nullptr, nullptr, "CSMain", "cs_5_0",
                                    flags, 0, &shaderBlob, &errorBlob);
            if (FAILED(hr)) {
                if (errorBlob)
                    std::cerr << "[DirectX] Shader compile error (" << debugName << "): "
                              << static_cast<char*>(errorBlob->GetBufferPointer()) << std::endl;
                return false;
            }
            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature     = rootSignature_.Get();
            psoDesc.CS.pShaderBytecode = shaderBlob->GetBufferPointer();
            psoDesc.CS.BytecodeLength  = shaderBlob->GetBufferSize();
            DX_CHECK(device_->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
            return true;
        };

        // Helper: create a pipeline state from a pre-compiled .cso bytecode blob.
        auto compilePipelineFromCSO = [&](const std::vector<uint8_t>& cso,
                                           ComPtr<ID3D12PipelineState>& pso) -> bool {
            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature     = rootSignature_.Get();
            psoDesc.CS.pShaderBytecode = cso.data();
            psoDesc.CS.BytecodeLength  = cso.size();
            DX_CHECK(device_->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
            return true;
        };

        // Helper: load a pipeline from external files, trying .cso then .hlsl.
        // The shader stem (e.g. "l2_distance") is resolved via find_ann_shader_path().
        auto loadPipeline = [&](const char* stem,
                                 ComPtr<ID3D12PipelineState>& pso) -> bool {
            std::string path = find_ann_shader_path(stem);
            if (path.empty()) {
                std::cerr << "[DirectX] Shader file not found: " << stem
                          << " (.cso or .hlsl). "
                          << "Build the directx_vector_shaders target or place the shader "
                          << "files in shaders/vector_index/." << std::endl;
                return false;
            }

            namespace fs = std::filesystem;
            if (fs::path(path).extension() == ".cso") {
                auto bytes = read_binary_file(path);
                if (bytes.empty()) {
                    std::cerr << "[DirectX] Failed to read .cso: " << path << std::endl;
                    return false;
                }
                return compilePipelineFromCSO(bytes, pso);
            }

            // .hlsl path: load and compile at runtime with d3dcompiler
            auto src = read_text_file(path);
            if (src.empty()) {
                std::cerr << "[DirectX] Failed to read .hlsl: " << path << std::endl;
                return false;
            }
            return compilePipelineFromSource(src, stem, pso);
        };

        if (!loadPipeline("l2_distance",     l2Pipeline_)) {
          return false;
        }
        if (!loadPipeline("cosine_distance", cosinePipeline_)) {
          return false;
        }
        return true;
    }

    // ------------------------------------------------------------------
    // Buffer helpers
    // ------------------------------------------------------------------

    // Default-heap UAV buffer (GPU-side, written/read by compute shaders)
    ComPtr<ID3D12Resource> createUAVBuffer(SIZE_T size) {
        D3D12_HEAP_PROPERTIES hp = {};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC rd = {};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width            = size;
        rd.Height           = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels        = 1;
        rd.Format           = DXGI_FORMAT_UNKNOWN;
        rd.SampleDesc.Count = 1;
        rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ComPtr<ID3D12Resource> buf;
        DX_CHECK_THROW(device_->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buf)));
        return buf;
    }

    // Upload heap buffer (CPU-writable, GPU-readable) — no UAV flag
    ComPtr<ID3D12Resource> createUploadBuffer(SIZE_T size) {
        D3D12_HEAP_PROPERTIES hp = {};
        hp.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC rd = {};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width            = size;
        rd.Height           = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels        = 1;
        rd.Format           = DXGI_FORMAT_UNKNOWN;
        rd.SampleDesc.Count = 1;
        rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags            = D3D12_RESOURCE_FLAG_NONE;

        ComPtr<ID3D12Resource> buf;
        DX_CHECK_THROW(device_->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buf)));
        return buf;
    }

    // Readback heap buffer (GPU-writable, CPU-readable) — no UAV flag
    ComPtr<ID3D12Resource> createReadbackBuffer(SIZE_T size) {
        D3D12_HEAP_PROPERTIES hp = {};
        hp.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC rd = {};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width            = size;
        rd.Height           = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels        = 1;
        rd.Format           = DXGI_FORMAT_UNKNOWN;
        rd.SampleDesc.Count = 1;
        rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags            = D3D12_RESOURCE_FLAG_NONE;

        ComPtr<ID3D12Resource> buf;
        DX_CHECK_THROW(device_->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buf)));
        return buf;
    }

    // Map an upload buffer, write data, and unmap
    static void uploadData(ID3D12Resource* uploadBuf, const void* src, SIZE_T bytes) {
        void* mapped = nullptr;
        D3D12_RANGE emptyRange = { 0, 0 };
        DX_CHECK_THROW(uploadBuf->Map(0, &emptyRange, &mapped));
        std::memcpy(mapped, src, bytes);
        D3D12_RANGE writtenRange = { 0, bytes };
        uploadBuf->Unmap(0, &writtenRange);
    }

    // Insert a resource barrier into the open command list
    void transitionBarrier(ID3D12Resource* res,
                           D3D12_RESOURCE_STATES before,
                           D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = res;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter  = after;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList_->ResourceBarrier(1, &barrier);
    }

    // Signal the fence and wait until the GPU reaches it
    void waitForGPU() {
        if (!commandQueue_ || !fence_ || !fenceEvent_) {
          return;
        }
        const UINT64 val = fenceValue_++;
        commandQueue_->Signal(fence_.Get(), val);
        if (fence_->GetCompletedValue() < val) {
            fence_->SetEventOnCompletion(val, fenceEvent_);
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
    }
};

// ============================================================================
// DirectXVectorBackend — full public interface (Windows + THEMIS_ENABLE_DIRECTX)
// ============================================================================

DirectXVectorBackend::DirectXVectorBackend()
    : initialized_(false), impl_(std::make_unique<DirectXVectorBackendImpl>()) {}

DirectXVectorBackend::~DirectXVectorBackend() {
    shutdown();
}

bool DirectXVectorBackend::isAvailable() const noexcept {
    // Probe availability by enumerating DXGI adapters and calling
    // D3D12CreateDevice without actually creating a device (nullptr device).
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
      return false;
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
          continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                        __uuidof(ID3D12Device), nullptr))) {
            return true;
        }
    }
    return false;
}

BackendCapabilities DirectXVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsVectorOps       = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics        = metricBit(DistanceMetric::L2)
                                 | metricBit(DistanceMetric::COSINE);
    caps.deviceName = (initialized_ && impl_) ? impl_->deviceName() : "DirectX 12";
    return caps;
}

bool DirectXVectorBackend::initialize() {
    if (initialized_) {
      return true;
    }
    if (!impl_) {
      impl_ = std::make_unique<DirectXVectorBackendImpl>();
    }
    if (!impl_->initialize()) {
        std::cerr << "[DirectX] Initialization failed" << std::endl;
        return false;
    }
    initialized_ = true;
    std::cout << "[DirectX] Initialized: " << impl_->deviceName() << std::endl;
    return true;
}

void DirectXVectorBackend::shutdown() {
    if (initialized_ && impl_) {
        impl_->shutdown();
        initialized_ = false;
    }
}

std::vector<float> DirectXVectorBackend::computeDistances(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    bool useL2)
{
    if (!initialized_ || !impl_) {
        std::cerr << "[DirectX] computeDistances: backend not initialized" << std::endl;
        return {};
    }
    if (!queries || !vectors || numQueries == 0 || numVectors == 0 || dim == 0) {
        return {};
    }
    try {
        return impl_->computeDistances(queries, numQueries, dim, vectors, numVectors, useL2);
    } catch (const std::exception& e) {
        std::cerr << "[DirectX] computeDistances error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<std::vector<std::pair<uint32_t, float>>> DirectXVectorBackend::batchKnnSearch(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2)
{
    if (!initialized_ || !impl_) {
        std::cerr << "[DirectX] batchKnnSearch: backend not initialized" << std::endl;
        return {};
    }
    if (!queries || !vectors || numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        return {};
    }
    try {
        return impl_->batchKnnSearch(queries, numQueries, dim, vectors, numVectors, k, useL2);
    } catch (const std::exception& e) {
        std::cerr << "[DirectX] batchKnnSearch error: " << e.what() << std::endl;
        return {};
    }
}

} // namespace acceleration
} // namespace themis

#endif // _WIN32 && THEMIS_ENABLE_DIRECTX
