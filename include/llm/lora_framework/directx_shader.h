/**
 * @file directx_shader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef _WIN32

#include "directx_context.h"
#include <d3d12.h>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

class DirectXShader {
public:
    /**
     * @brief Direct XShader.
     * @param[in] shader_path Path to the shader.
     * @return Return value.
     */
    explicit DirectXShader(const std::string& shader_path);
    
    ~DirectXShader();
    
    // Disable copy, allow move
    DirectXShader(const DirectXShader&) = delete;
    DirectXShader& operator=(const DirectXShader&) = delete;
    DirectXShader(DirectXShader&&) noexcept;
    DirectXShader& operator=(DirectXShader&&) noexcept;
    
    /**
     * @brief Load.
     * @return True when the operation succeeds.
     */
    bool load();
    
    D3D12_SHADER_BYTECODE get_bytecode() const {
        return D3D12_SHADER_BYTECODE{bytecode_.data(), bytecode_.size()};
    }
    
    bool is_loaded() const { return !bytecode_.empty(); }
    
    const std::string& path() const { return shader_path_; }

private:
    std::string shader_path_;
    std::vector<uint8_t> bytecode_;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
