/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_shader.h                                   ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     95                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
