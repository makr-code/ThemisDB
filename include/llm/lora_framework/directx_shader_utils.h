/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_shader_utils.h                             ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     75                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
