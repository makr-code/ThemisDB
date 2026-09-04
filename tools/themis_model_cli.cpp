/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_model_cli.cpp                               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     367                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file themis_model_cli.cpp
 * @brief CLI tool for managing LLM models in ThemisDB (similar to Ollama)
 * 
 * Commands:
 *   themis-model pull <model>    Download a model
 *   themis-model list            List downloaded models
 *   themis-model rm <model>      Remove a model
 *   themis-model show <model>    Show model information
 * 
 * Example:
 *   themis-model pull phi3:mini-4k
 */

#include "llm/model_downloader.h"
#include "utils/cli_parser_utils.h"
#include "utils/logger.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <optional>
#include <string_view>
#include <yaml-cpp/yaml.h>
#include <sstream>

namespace fs = std::filesystem;
using namespace themis::llm;

namespace {

struct ModelCliOptions {
    std::string model_dir = "models/default";
    std::string command;
    std::vector<std::string> positional_args;
    bool show_help = false;
};

using themis::cli::is_help_flag;

bool parse_model_cli_options(int argc,
                             char** argv,
                             ModelCliOptions& options,
                             std::string& error_message) {
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];

        if (options.command.empty() && is_help_flag(arg)) {
            options.show_help = true;
            continue;
        }

        if (options.command.empty() && arg == "--model-dir") {
            if (index + 1 >= argc) {
                error_message = "Missing value for option --model-dir";
                return false;
            }
            options.model_dir = argv[++index];
            continue;
        }

        if (options.command.empty() && arg.starts_with("--")) {
            error_message = "Unknown option: " + arg;
            return false;
        }

        if (options.command.empty()) {
            options.command = arg;
            continue;
        }

        options.positional_args.push_back(arg);
    }

    return true;
}

} // namespace

// ANSI color codes for terminal output
namespace Color {
    const std::string Reset = "\033[0m";
    const std::string Bold = "\033[1m";
    const std::string Green = "\033[32m";
    const std::string Yellow = "\033[33m";
    const std::string Blue = "\033[34m";
    const std::string Red = "\033[31m";
    const std::string Cyan = "\033[36m";
}

// Format bytes to human-readable format
std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss = {};
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    return oss.str();
}

// Format time duration
std::string formatDuration(double seconds) {
    if (seconds < 60) {
        return std::to_string(static_cast<int>(seconds)) + "s";
    } else if (seconds < 3600) {
        int mins = static_cast<int>(seconds / 60);
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(mins) + "m " + std::to_string(secs) + "s";
    } else {
        int hours = static_cast<int>(seconds / 3600);
        int mins = static_cast<int>(seconds / 60) % 60;
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
}

// Draw a progress bar (Ollama-style)
void drawProgressBar(float percentage, size_t downloaded, size_t total, double speed_mbps) {
    const int bar_width = 40;
    int filled = static_cast<int>(bar_width * percentage / 100.0f);
    
    std::cout << "\r";  // Return to start of line
    std::cout << Color::Cyan << "▐";
    
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            std::cout << "█";
        } else {
            std::cout << "░";
        }
    }
    
    std::cout << "▌" << Color::Reset << " ";
    std::cout << std::fixed << std::setprecision(1) << percentage << "% ";
    std::cout << "(" << formatBytes(downloaded) << " / " << formatBytes(total) << ") ";
    std::cout << Color::Yellow << speed_mbps << " MB/s" << Color::Reset;
    std::cout << std::flush;
}

// Pull (download) a model
int cmdPull(const std::string& model_name, const std::string& model_dir) {
    std::cout << Color::Bold << "Pulling model: " << Color::Green << model_name << Color::Reset << "\n";
    std::cout << "Destination: " << model_dir << "\n\n";
    
    // Create model directory if needed
    if (!fs::exists(model_dir)) {
        fs::create_directories(model_dir);
    }
    
    // Configure download
    ModelDownloadConfig config;
    config.model_name = model_name;
    config.ollama_url = "http://localhost:11434";
    config.download_dir = model_dir;
    config.use_cache = true;
    config.timeout_seconds = 600;  // 10 minutes
    
    // Track progress
    auto start_time = std::chrono::steady_clock::now();
    size_t last_downloaded = 0;
    auto last_time = start_time;
    
    config.progress_callback = [&](size_t downloaded, size_t total, const std::string&) {
        if (total > 0) {
            auto now = std::chrono::steady_clock::now();
            auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
            
            // Calculate speed (update every 500ms)
            double speed_mbps = 0.0;
            if (time_diff > 500) {
                size_t bytes_diff = downloaded - last_downloaded;
                speed_mbps = (bytes_diff / 1024.0 / 1024.0) / (time_diff / 1000.0);
                last_downloaded = downloaded;
                last_time = now;
            }
            
            float percentage = 100.0f * downloaded / total;
            drawProgressBar(percentage, downloaded, total, speed_mbps);
            
            if (downloaded == total) {
                std::cout << "\n";
            }
        }
    };
    
    // Download the model
    ModelDownloader downloader;
    auto result = downloader.downloadFromOllama(config);
    
    if (result.success) {
        auto end_time = std::chrono::steady_clock::now();
        [[maybe_unused]] auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        std::cout << Color::Green << "✓ " << Color::Bold << "Success!" << Color::Reset << "\n";
        std::cout << "Model: " << result.model_path << "\n";
        std::cout << "Size: " << formatBytes(result.file_size_bytes) << "\n";
        std::cout << "Time: " << formatDuration(result.download_time_seconds) << "\n";
        return 0;
    } else {
        std::cout << "\n" << Color::Red << "✗ Error: " << result.error_message << Color::Reset << "\n";
        return 1;
    }
}

// List downloaded models
int cmdList(const std::string& model_dir) {
    std::cout << Color::Bold << "Downloaded models:" << Color::Reset << "\n\n";
    
    if (!fs::exists(model_dir)) {
        std::cout << Color::Yellow << "No models found. Model directory doesn't exist." << Color::Reset << "\n";
        std::cout << "Use 'themis-model pull <model>' to download a model.\n";
        return 0;
    }
    
    bool found_any = false;
    
    for (const auto& entry : fs::directory_iterator(model_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
            found_any = true;
            auto size = fs::file_size(entry.path());
            auto modified = fs::last_write_time(entry.path());
            
            std::cout << Color::Cyan << "▐ " << Color::Bold << entry.path().filename().string() << Color::Reset << "\n";
            std::cout << "  Size: " << formatBytes(size) << "\n";
            std::cout << "  Path: " << entry.path().string() << "\n";
            std::cout << "\n";
        }
    }
    
    if (!found_any) {
        std::cout << Color::Yellow << "No models found." << Color::Reset << "\n";
        std::cout << "Use 'themis-model pull <model>' to download a model.\n";
    }
    
    return 0;
}

// Remove a model
int cmdRemove(const std::string& model_name, const std::string& model_dir) {
    std::string model_path = model_dir + "/" + model_name;
    if (!model_path.ends_with(".gguf")) {
        model_path += ".gguf";
    }
    
    if (!fs::exists(model_path)) {
        std::cout << Color::Red << "Error: Model not found: " << model_path << Color::Reset << "\n";
        return 1;
    }
    
    std::cout << "Remove model: " << model_path << "\n";
    std::cout << "Are you sure? (y/N): ";
    
    std::string response = {};
    std::getline(std::cin, response);
    
    if (response == "y" || response == "Y" || response == "yes") {
        try {
            fs::remove(model_path);
            std::cout << Color::Green << "✓ Model removed successfully." << Color::Reset << "\n";
            return 0;
        } catch (const std::exception& e) {
            std::cout << Color::Red << "Error removing model: " << e.what() << Color::Reset << "\n";
            return 1;
        }
    } else {
        std::cout << "Cancelled.\n";
        return 0;
    }
}

// Show model information
int cmdShow(const std::string& model_name, const std::string& model_dir) {
    std::string model_path = model_dir + "/" + model_name;
    if (!model_path.ends_with(".gguf")) {
        model_path += ".gguf";
    }
    
    if (!fs::exists(model_path)) {
        std::cout << Color::Red << "Error: Model not found: " << model_path << Color::Reset << "\n";
        return 1;
    }
    
    auto size = fs::file_size(model_path);
    auto modified = fs::last_write_time(model_path);
    
    std::cout << Color::Bold << "Model Information:" << Color::Reset << "\n\n";
    std::cout << "Name: " << Color::Cyan << model_name << Color::Reset << "\n";
    std::cout << "Path: " << model_path << "\n";
    std::cout << "Size: " << formatBytes(size) << "\n";
    std::cout << "Format: GGUF\n";
    
    return 0;
}

// Print usage
void printUsage(const char* prog_name) {
    std::cout << Color::Bold << "ThemisDB Model Manager" << Color::Reset << "\n";
    std::cout << "Manage LLM models for ThemisDB (similar to Ollama)\n\n";
    std::cout << Color::Bold << "USAGE:" << Color::Reset << "\n";
    std::cout << "  " << prog_name << " <command> [options]\n\n";
    std::cout << Color::Bold << "COMMANDS:" << Color::Reset << "\n";
    std::cout << "  " << Color::Cyan << "pull" << Color::Reset << " <model>    Download a model\n";
    std::cout << "  " << Color::Cyan << "list" << Color::Reset << "           List downloaded models\n";
    std::cout << "  " << Color::Cyan << "rm" << Color::Reset << " <model>      Remove a model\n";
    std::cout << "  " << Color::Cyan << "show" << Color::Reset << " <model>    Show model information\n";
    std::cout << "  " << Color::Cyan << "help" << Color::Reset << "           Show this help message\n";
    std::cout << "\n";
    std::cout << Color::Bold << "OPTIONS:" << Color::Reset << "\n";
    std::cout << "  --model-dir <path>    Model storage directory (default: models/default)\n";
    std::cout << "\n";
    std::cout << Color::Bold << "EXAMPLES:" << Color::Reset << "\n";
    std::cout << "  " << prog_name << " pull phi3:mini-4k\n";
    std::cout << "  " << prog_name << " list\n";
    std::cout << "  " << prog_name << " show phi3-mini-4k-instruct-q4.gguf\n";
    std::cout << "  " << prog_name << " rm phi3-mini-4k-instruct-q4.gguf\n";
    std::cout << "\n";
}

int main(int argc, char** argv) {
    ModelCliOptions options;
    std::string parse_error = {};
    if (!parse_model_cli_options(argc, argv, options, parse_error)) {
        std::cerr << Color::Red << "Error: " << parse_error << Color::Reset << "\n";
        printUsage(argv[0]);
        return 1;
    }

    if (options.show_help || options.command.empty()) {
        printUsage(argv[0]);
        return options.show_help ? 0 : 1;
    }

    const auto& model_dir = options.model_dir;
    const auto& command = options.command;
    
    // Execute command
    if (command == "pull") {
        if (options.positional_args.empty()) {
            std::cerr << Color::Red << "Error: 'pull' requires a model name" << Color::Reset << "\n";
            std::cerr << "Usage: " << argv[0] << " pull <model>\n";
            return 1;
        }
        const auto& model_name = options.positional_args[0];
        return cmdPull(model_name, model_dir);
        
    } else if (command == "list" || command == "ls") {
        return cmdList(model_dir);
        
    } else if (command == "rm" || command == "remove") {
        if (options.positional_args.empty()) {
            std::cerr << Color::Red << "Error: 'rm' requires a model name" << Color::Reset << "\n";
            std::cerr << "Usage: " << argv[0] << " rm <model>\n";
            return 1;
        }
        const auto& model_name = options.positional_args[0];
        return cmdRemove(model_name, model_dir);
        
    } else if (command == "show" || command == "info") {
        if (options.positional_args.empty()) {
            std::cerr << Color::Red << "Error: 'show' requires a model name" << Color::Reset << "\n";
            std::cerr << "Usage: " << argv[0] << " show <model>\n";
            return 1;
        }
        const auto& model_name = options.positional_args[0];
        return cmdShow(model_name, model_dir);
        
    } else if (command == "help" || is_help_flag(command)) {
        printUsage(argv[0]);
        return 0;
        
    } else {
        std::cerr << Color::Red << "Error: Unknown command: " << command << Color::Reset << "\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
