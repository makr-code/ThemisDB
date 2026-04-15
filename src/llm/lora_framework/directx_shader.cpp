/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_shader.cpp                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:12:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     90                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_framework/directx_shader.h"

#ifdef _WIN32

#include <fstream>
#include <iostream>
#include <stdexcept>

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
    
    // Open file in binary mode
    std::ifstream file(shader_path_, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "DirectXShader: Failed to open shader file: " << shader_path_ << "\n";
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file into bytecode vector
    bytecode_.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(bytecode_.data()), size)) {
        std::cerr << "DirectXShader: Failed to read shader file: " << shader_path_ << "\n";
        bytecode_.clear();
        return false;
    }
    
    std::cout << "DirectXShader: Loaded shader from " << shader_path_ 
              << " (" << bytecode_.size() << " bytes)\n";
    
    return true;
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
