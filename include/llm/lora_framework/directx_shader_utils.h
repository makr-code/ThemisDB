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
 * 1. CMAKE_BINARY_DIR/shaders/lora (build directory)
 * 2. Executable directory/shaders/lora (installed location)
 * 3. ../shaders/lora (relative to executable)
 */
inline std::string get_shader_path(const std::string& shader_name) {
    namespace fs = std::filesystem;
    
    // Try build directory first (during development)
    std::vector<fs::path> search_paths = {
        fs::path(CMAKE_BINARY_DIR) / "shaders" / "lora" / shader_name,
        fs::current_path() / "shaders" / "lora" / shader_name,
        fs::current_path() / ".." / "shaders" / "lora" / shader_name,
        fs::current_path() / shader_name,
    };
    
    for (const auto& path : search_paths) {
        if (fs::exists(path)) {
            return path.string();
        }
    }
    
    // Return default path (will fail at shader load time with clear error)
    return (fs::current_path() / "shaders" / "lora" / shader_name).string();
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
