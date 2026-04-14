/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_shader_utils.h                             ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:39:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
