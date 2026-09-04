/**
 * @file directx_shader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/directx_shader.h"

#ifdef _WIN32

#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <d3dcompiler.h>
// Link against D3DCompiler library for runtime HLSL compilation
#pragma comment(lib, "d3dcompiler.lib")

namespace themis {
namespace lora {
namespace directx {

DirectXShader::DirectXShader(const std::string& shader_path)
    : shader_path_(shader_path) {
}

DirectXShader::~DirectXShader() {
    // bytecode_ vector automatically releases
}

DirectXShader::DirectXShader(DirectXShader&& other) noexcept
    : shader_path_(std::move(other.shader_path_))
    , bytecode_(std::move(other.bytecode_)) {
}

DirectXShader& DirectXShader::operator=(DirectXShader&& other) noexcept {
    if (this != &other) {
        shader_path_ = std::move(other.shader_path_);
        bytecode_ = std::move(other.bytecode_);
    }
    return *this;
}

bool DirectXShader::load() {
    if (!bytecode_.empty()) {
        return true;  // Already loaded
    }
    
    namespace fs = std::filesystem;

    fs::path p(shader_path_);

    // If the path points to a compiled bytecode (.cso/.dxil), try to read it.
    auto try_read_bytecode = [&](const fs::path& file_path) -> bool {
        std::ifstream file(file_path.string(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        if (size <= 0) {
          return false;
        }
        bytecode_.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(bytecode_.data()), size)) {
            bytecode_.clear();
            return false;
        }
        std::cout << "DirectXShader: Loaded shader bytecode from " << file_path.string()
                  << " (" <<static_cast<int>(bytecode_.size()) << " bytes)\n";
        return true;
    };

    // 1) If the provided path points to an existing file, try to load it.
    if (fs::exists(p)) {
        // If it's a .hlsl file, compile it; otherwise read raw bytecode
        if (p.extension() == ".hlsl") {
            // Compile HLSL source to bytecode
            UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            ComPtr<ID3DBlob> blob;
            ComPtr<ID3DBlob> error_blob;
            HRESULT hr = D3DCompileFromFile(
                p.c_str(),
                nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                "main",
                "cs_5_0",
                compileFlags,
                0,
                blob.GetAddressOf(),
                error_blob.GetAddressOf()
            );
            if (FAILED(hr)) {
                if (error_blob) {
                    std::cerr << "DirectXShader: HLSL compile error: "
                              << static_cast<const char*>(error_blob->GetBufferPointer()) << "\n";
                } else {
                    std::cerr << "DirectXShader: Failed to compile HLSL: " << shader_path_ << "\n";
                }
                return false;
            }
            bytecode_.assign(
                static_cast<const uint8_t*>(blob->GetBufferPointer()),
                static_cast<const uint8_t*>(blob->GetBufferPointer()) + blob->GetBufferSize()
            );
            std::cout << "DirectXShader: Compiled HLSL source " << shader_path_ << " ("
                      <<static_cast<int>(bytecode_.size()) << " bytes)\n";
            return true;
        }
        // Attempt to read as compiled bytecode
        if (try_read_bytecode(p)) {
          return true;
        }
    }

    // 2) If file not found or not readable, attempt to switch extensions.
    // If requested .cso but .hlsl exists in source tree, compile it.
    if (p.extension() == ".cso" || p.extension() == ".dxil") {
        fs::path alt = p;
        alt.replace_extension(".hlsl");
        if (fs::exists(alt)) {
            // compile alt
            UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            ComPtr<ID3DBlob> blob;
            ComPtr<ID3DBlob> error_blob;
            HRESULT hr = D3DCompileFromFile(
                alt.c_str(),
                nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                "main",
                "cs_5_0",
                compileFlags,
                0,
                blob.GetAddressOf(),
                error_blob.GetAddressOf()
            );
            if (FAILED(hr)) {
                if (error_blob) {
                    std::cerr << "DirectXShader: HLSL compile error: "
                              << static_cast<const char*>(error_blob->GetBufferPointer()) << "\n";
                } else {
                    std::cerr << "DirectXShader: Failed to compile HLSL: " << alt.string() << "\n";
                }
                return false;
            }
            bytecode_.assign(
                static_cast<const uint8_t*>(blob->GetBufferPointer()),
                static_cast<const uint8_t*>(blob->GetBufferPointer()) + blob->GetBufferSize()
            );
            std::cout << "DirectXShader: Compiled HLSL source " << alt.string() << " ("
                      <<static_cast<int>(bytecode_.size()) << " bytes)\n";
            return true;
        }
    }

    std::cerr << "DirectXShader: Failed to open or compile shader file: " << shader_path_ << "\n";
    return false;
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
