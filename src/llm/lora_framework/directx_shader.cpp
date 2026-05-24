/*
 * ThemisDB | File: directx_shader.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 76
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=18 | delta=15 | status=divergent
 * External Severity (v3): C=2, H=11, M=5
 * PR: #572 Complete DirectX 12 Compute Pipeline Integration for GPU-Accelerate... (2026-03-11T21:38:25Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
