/**
 * @file directx_shader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: directx_shader.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:49:01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 78
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #572 Complete DirectX 12 Compute... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#ifdef _WIN32

#include "directx_context.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

/**
 * @brief DirectX 12 shader manager
 * 
 * Handles shader compilation, loading, and root signature creation.
 */
class DirectXShader {
public:
    /**
     * @brief Load compiled shader from file
     * @param shader_path Path to .cso or .dxil file
     */
    explicit DirectXShader(const std::string& shader_path);
    
    ~DirectXShader();
    
    // Disable copy, allow move
    DirectXShader(const DirectXShader&) = delete;
    DirectXShader& operator=(const DirectXShader&) = delete;
    DirectXShader(DirectXShader&&) noexcept;
    DirectXShader& operator=(DirectXShader&&) noexcept;
    
    /**
     * @brief Load shader bytecode from file
     */
    bool load();
    
    /**
     * @brief Get shader bytecode
     */
    D3D12_SHADER_BYTECODE get_bytecode() const {
        return D3D12_SHADER_BYTECODE{bytecode_.data(), bytecode_.size()};
    }
    
    /**
     * @brief Check if shader is loaded
     */
    bool is_loaded() const { return !bytecode_.empty(); }
    
    /**
     * @brief Get shader path
     */
    const std::string& path() const { return shader_path_; }

private:
    std::string shader_path_;
    std::vector<uint8_t> bytecode_;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
