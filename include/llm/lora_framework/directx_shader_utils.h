/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_shader_utils.h                             ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:36:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     73                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2bed9f378  2026-01-16  Complete DirectX 12 Compute Pipeline Integration for GPU-... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef _WIN32

#include <string>
#include <filesystem>

namespace themis {
namespace lora {
namespace directx {

/**
 * @brief Get the path to compiled shader files
 * 
 * Searches for shader files in:
 * 1. Executable directory/shaders/lora (installed location)
 * 2. ../shaders/lora (relative to executable)
 * 3. ./shaders/lora (current directory)
 * 4. ../../build/shaders/lora (development build directory)
 */
inline std::string get_shader_path(const std::string& shader_name) {
    namespace fs = std::filesystem;
    
    // Get executable directory
    fs::path exe_dir = fs::current_path();
    
    // Try various paths relative to executable
    std::vector<fs::path> search_paths = {
        exe_dir / "shaders" / "lora" / shader_name,
        exe_dir / ".." / "shaders" / "lora" / shader_name,
        exe_dir / "bin" / "shaders" / "lora" / shader_name,
        exe_dir / ".." / ".." / "build" / "shaders" / "lora" / shader_name,
    };
    
    for (const auto& path : search_paths) {
        if (fs::exists(path)) {
            return path.string();
        }
    }
    
    // Return default path (will fail at shader load time with clear error)
    return (exe_dir / "shaders" / "lora" / shader_name).string();
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
