// Copyright (c) 2025 ThemisDB Contributors
// Licensed under the MIT License

/**
 * @file themis-license.cpp
 * @brief Command-line tool for ThemisDB license management
 * 
 * Usage:
 *   themis-license validate <license-file>
 *   themis-license info <license-file>
 *   themis-license activate <license-key>
 *   themis-license status
 *   themis-license deactivate
 *   themis-license check-online
 */

#include "enterprise/plugin_loader.h"
#include "enterprise/license_validation_client.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;
using namespace themis::enterprise;

// Color output helpers
#ifdef _WIN32
    #include <windows.h>
    void setColor(int color) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
    }
    #define COLOR_GREEN 10
    #define COLOR_RED 12
    #define COLOR_YELLOW 14
    #define COLOR_RESET 7
#else
    #define COLOR_GREEN "\033[32m"
    #define COLOR_RED "\033[31m"
    #define COLOR_YELLOW "\033[33m"
    #define COLOR_RESET "\033[0m"
    void setColor(const char* color) {
        std::cout << color;
    }
#endif

class LicenseTool {
public:
    int run(int argc, char* argv[]) {
        if (argc < 2) {
            printUsage();
            return 1;
        }
        
        std::string command = argv[1];
        
        if (command == "validate") {
            return cmdValidate(argc, argv);
        } else if (command == "info") {
            return cmdInfo(argc, argv);
        } else if (command == "activate") {
            return cmdActivate(argc, argv);
        } else if (command == "status") {
            return cmdStatus(argc, argv);
        } else if (command == "deactivate") {
            return cmdDeactivate(argc, argv);
        } else if (command == "check-online") {
            return cmdCheckOnline(argc, argv);
        } else if (command == "--help" || command == "-h") {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            printUsage();
            return 1;
        }
    }
    
private:
    void printUsage();
    int cmdValidate(int argc, char* argv[]);
    int cmdInfo(int argc, char* argv[]);
    int cmdActivate(int argc, char* argv[]);
    int cmdStatus(int argc, char* argv[]);
    int cmdCheckOnline(int argc, char* argv[]);
    int cmdDeactivate(int argc, char* argv[]);
    void printLicenseInfo(const LicenseInfo& info);
};

void LicenseTool::printUsage() {
    std::cout << "ThemisDB License Management Tool v" << THEMIS_VERSION_STRING << "\n\n";
    std::cout << "Usage:\n";
    std::cout << "  themis-license validate <license-file>     Validate a license file\n";
    std::cout << "  themis-license info <license-file>         Show license information\n";
    std::cout << "  themis-license activate <license-key>      Activate license from server\n";
    std::cout << "  themis-license status                      Show current license status\n";
    std::cout << "  themis-license check-online                Validate license with server\n";
    std::cout << "  themis-license deactivate                  Deactivate current license\n";
    std::cout << "  themis-license --help                      Show this help\n";
    std::cout << "\nExamples:\n";
    std::cout << "  themis-license validate /etc/themis/license.json\n";
    std::cout << "  themis-license activate THEMIS-ENT-XXXX-XXXX-XXXX\n";
    std::cout << "  themis-license status\n";
    std::cout << "  themis-license check-online\n";
}

int LicenseTool::cmdValidate(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing license file path\n";
        std::cerr << "Usage: themis-license validate <license-file>\n";
        return 1;
    }
    
    std::string license_path = argv[2];
    
    std::cout << "Validating license file: " << license_path << "\n\n";
    
    EnterprisePluginLoader loader;
    bool valid = loader.loadLicense(license_path);
    
    if (valid) {
        setColor(COLOR_GREEN);
        std::cout << "✓ License is valid\n";
        setColor(COLOR_RESET);
        
        auto license_info = loader.getLicenseInfo();
        printLicenseInfo(license_info);
        return 0;
    } else {
        setColor(COLOR_RED);
        std::cout << "✗ License validation failed\n";
        setColor(COLOR_RESET);
        return 1;
    }
}

int LicenseTool::cmdInfo(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing license file path\n";
        std::cerr << "Usage: themis-license info <license-file>\n";
        return 1;
    }
    
    std::string license_path = argv[2];
    
    try {
        std::ifstream file(license_path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open license file: " << license_path << "\n";
            return 1;
        }
        
        json license_json;
        file >> license_json;
        
        std::cout << "License Information:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Organization:  " << license_json.value("organization", "N/A") << "\n";
        std::cout << "Edition:       " << license_json.value("edition", "N/A") << "\n";
        std::cout << "Issued:        " << license_json.value("issued_date", "N/A") << "\n";
        std::cout << "Expires:       " << license_json.value("expiry_date", "N/A") << "\n";
        
        if (license_json.contains("modules") && license_json["modules"].is_array()) {
            std::cout << "Modules:       ";
            bool first = true;
            for (const auto& module : license_json["modules"]) {
                if (!first) std::cout << ", ";
                std::cout << module.get<std::string>();
                first = false;
            }
            std::cout << "\n";
        }
        
        if (license_json.contains("limits") && license_json["limits"].is_object()) {
            std::cout << "\nLimits:\n";
            for (auto it = license_json["limits"].begin(); 
                 it != license_json["limits"].end(); ++it) {
                std::cout << "  " << it.key() << ": ";
                int64_t value = it.value().get<int64_t>();
                if (value < 0) {
                    std::cout << "Unlimited\n";
                } else {
                    std::cout << value << "\n";
                }
            }
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error reading license file: " << e.what() << "\n";
        return 1;
    }
}

int LicenseTool::cmdActivate(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing license key\n";
        std::cerr << "Usage: themis-license activate <license-key>\n";
        return 1;
    }
    
    std::string license_key = argv[2];
    
    std::cout << "Activating license: " << license_key << "\n\n";
    
    setColor(COLOR_YELLOW);
    std::cout << "Note: License activation requires network connectivity\n";
    std::cout << "      and a valid license key from ThemisDB.\n";
    setColor(COLOR_RESET);
    
    // TODO: Implement actual license download
    std::cout << "\nLicense activation will be implemented in Phase 2.\n";
    std::cout << "For now, please use a license file provided by ThemisDB support.\n";
    
    return 1;
}

int LicenseTool::cmdStatus(int argc, char* argv[]) {
    std::string default_paths[] = {
        "/etc/themis/license.json",
        "./config/enterprise_license.json",
        "./license.json"
    };
    
    std::cout << "Checking license status...\n\n";
    
    for (const auto& path : default_paths) {
        if (std::filesystem::exists(path)) {
            std::cout << "Found license file: " << path << "\n\n";
            
            EnterprisePluginLoader loader;
            bool valid = loader.loadLicense(path);
            
            if (valid) {
                auto license_info = loader.getLicenseInfo();
                
                setColor(COLOR_GREEN);
                std::cout << "✓ License Status: ACTIVE\n";
                setColor(COLOR_RESET);
                
                printLicenseInfo(license_info);
                return 0;
            } else {
                setColor(COLOR_RED);
                std::cout << "✗ License Status: INVALID\n";
                setColor(COLOR_RESET);
                return 1;
            }
        }
    }
    
    setColor(COLOR_YELLOW);
    std::cout << "No license file found\n";
    std::cout << "Checked locations:\n";
    for (const auto& path : default_paths) {
        std::cout << "  - " << path << "\n";
    }
    setColor(COLOR_RESET);
    
    return 1;
}

int LicenseTool::cmdCheckOnline(int argc, char* argv[]) {
    // Find license file first
    std::string license_path;
    std::string default_paths[] = {
        "/etc/themis/license.json",
        "./config/enterprise_license.json",
        "./license.json"
    };
    
    for (const auto& path : default_paths) {
        if (std::filesystem::exists(path)) {
            license_path = path;
            break;
        }
    }
    
    if (license_path.empty()) {
        std::cerr << "Error: No license file found\n";
        return 1;
    }
    
    // Load license info
    EnterprisePluginLoader loader;
    if (!loader.loadLicense(license_path)) {
        std::cerr << "Error: Failed to load license file\n";
        return 1;
    }
    
    auto license_info = loader.getLicenseInfo();
    
    std::cout << "Validating license with server...\n\n";
    
    // Contact validation server
    LicenseValidationClient::Config config;
    config.server_url = "https://license.themisdb.io/api/v1";
    
    LicenseValidationClient client(config);
    
    auto result = client.validateLicense(
        license_info.license_key,
        license_info.edition,
        1  // TODO: Get actual node count
    );
    
    if (result && result->is_valid) {
        setColor(COLOR_GREEN);
        std::cout << "✓ Online validation successful\n";
        setColor(COLOR_RESET);
        std::cout << "Message: " << result->message << "\n";
        std::cout << "Server version: " << result->server_version << "\n";
        return 0;
    } else {
        setColor(COLOR_RED);
        std::cout << "✗ Online validation failed\n";
        setColor(COLOR_RESET);
        if (result) {
            std::cout << "Message: " << result->message << "\n";
        }
        return 1;
    }
}

int LicenseTool::cmdDeactivate(int argc, char* argv[]) {
    std::cout << "Deactivating license...\n\n";
    
    setColor(COLOR_YELLOW);
    std::cout << "Warning: This will remove the license file.\n";
    std::cout << "         Enterprise features will be disabled.\n";
    std::cout << "\nAre you sure? (yes/no): ";
    setColor(COLOR_RESET);
    
    std::string confirmation;
    std::cin >> confirmation;
    
    if (confirmation != "yes") {
        std::cout << "Deactivation cancelled.\n";
        return 0;
    }
    
    std::string default_paths[] = {
        "/etc/themis/license.json",
        "./config/enterprise_license.json",
        "./license.json"
    };
    
    bool removed = false;
    for (const auto& path : default_paths) {
        if (std::filesystem::exists(path)) {
            try {
                std::filesystem::remove(path);
                std::cout << "Removed: " << path << "\n";
                removed = true;
            } catch (const std::exception& e) {
                std::cerr << "Failed to remove " << path << ": " << e.what() << "\n";
            }
        }
    }
    
    if (removed) {
        setColor(COLOR_GREEN);
        std::cout << "\n✓ License deactivated successfully\n";
        setColor(COLOR_RESET);
        return 0;
    } else {
        std::cout << "\nNo license file found to deactivate\n";
        return 1;
    }
}

void LicenseTool::printLicenseInfo(const LicenseInfo& info) {
    std::cout << "\nLicense Details:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Organization:  " << info.organization << "\n";
    std::cout << "Edition:       " << info.edition << "\n";
    
    // Format dates
    auto issued_time = std::chrono::system_clock::to_time_t(info.issued_date);
    auto expiry_time = std::chrono::system_clock::to_time_t(info.expiry_date);
    
    std::tm issued_tm = {};
    std::tm expiry_tm = {};
#ifdef _WIN32
    localtime_s(&issued_tm, &issued_time);
    localtime_s(&expiry_tm, &expiry_time);
#else
    localtime_r(&issued_time, &issued_tm);
    localtime_r(&expiry_time, &expiry_tm);
#endif
    
    char issued_buf[100], expiry_buf[100];
    std::strftime(issued_buf, sizeof(issued_buf), "%Y-%m-%d", &issued_tm);
    std::strftime(expiry_buf, sizeof(expiry_buf), "%Y-%m-%d", &expiry_tm);
    
    std::cout << "Issued:        " << issued_buf << "\n";
    std::cout << "Expires:       " << expiry_buf << "\n";
    
    // Check expiry
    auto now = std::chrono::system_clock::now();
    auto days_until_expiry = std::chrono::duration_cast<std::chrono::hours>(
        info.expiry_date - now
    ).count() / 24;
    
    if (days_until_expiry < 0) {
        setColor(COLOR_RED);
        std::cout << "Status:        EXPIRED (" << -days_until_expiry << " days ago)\n";
        setColor(COLOR_RESET);
    } else if (days_until_expiry < 30) {
        setColor(COLOR_YELLOW);
        std::cout << "Status:        EXPIRING SOON (" << days_until_expiry << " days left)\n";
        setColor(COLOR_RESET);
    } else {
        setColor(COLOR_GREEN);
        std::cout << "Status:        ACTIVE (" << days_until_expiry << " days left)\n";
        setColor(COLOR_RESET);
    }
    
    std::cout << "\nEnabled Modules:\n";
    for (const auto& module : info.enabled_modules) {
        std::cout << "  ✓ " << module << "\n";
    }
    
    if (!info.limits.empty()) {
        std::cout << "\nLimits:\n";
        for (const auto& [key, value] : info.limits) {
            std::cout << "  " << key << ": ";
            if (value < 0) {
                std::cout << "Unlimited\n";
            } else {
                std::cout << value << "\n";
            }
        }
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
}

int main(int argc, char* argv[]) {
    // Setup logging
    spdlog::set_level(spdlog::level::warn);  // Only show warnings/errors
    
    LicenseTool tool;
    return tool.run(argc, argv);
}
