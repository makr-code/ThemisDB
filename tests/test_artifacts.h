#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>

namespace themis::test {

inline std::filesystem::path repo_root() {
    if (const char* env_source = std::getenv("CMAKE_SOURCE_DIR"); env_source && env_source[0] != '\0') {
        return std::filesystem::path(env_source);
    }
    if (const char* env_workspace = std::getenv("GITHUB_WORKSPACE"); env_workspace && env_workspace[0] != '\0') {
        return std::filesystem::path(env_workspace);
    }
    return std::filesystem::current_path();
}

inline std::filesystem::path central_logs_dir() {
    auto path = repo_root() / "logs";
    std::filesystem::create_directories(path);
    return path;
}

inline std::filesystem::path central_assets_dir() {
    auto path = repo_root() / "assets";
    std::filesystem::create_directories(path);
    return path;
}

inline std::filesystem::path central_log_path(const std::string& relative_name) {
    auto path = central_logs_dir() / relative_name;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    return path;
}

inline std::filesystem::path central_asset_path(const std::string& relative_name) {
    auto path = central_assets_dir() / relative_name;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    return path;
}

}  // namespace themis::test
