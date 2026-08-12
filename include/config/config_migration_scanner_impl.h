/**
 * @file config_migration_scanner_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * config_migration_scanner_impl.h – Testable implementation of the
 * config_migration_scanner logic.
 *
 * This header is included by tools/config_migration_scanner.cpp (which
 * provides main()) and by the unit-test target so that the scanning and
 * fix logic can be exercised directly without spawning a subprocess.
 *
 * All functions are declared inline so that the header can be included
 * from multiple translation units without linker errors.
 */

#pragma once

#include "config/config_path_resolver.h"
#include "config/path_mapping_metadata.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace cms {   // config_migration_scanner

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

struct ScanMatch {
    fs::path    file;
    int         line_number;
    std::string legacy_path;
    std::string new_path;
    std::string category;
    bool        removal_due;
    std::string deprecated_date;
    std::string removal_date;
    std::string migration_guide_url;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

inline const std::set<std::string>& scanExtensions() {
    static const std::set<std::string> kExts = {
        ".yaml", ".yml", ".json", ".toml", ".ini", ".env"
    };
    return kExts;
}

inline bool shouldScanFile(const fs::path& p) {
    std::string filename = p.filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    if (filename == ".env") {
        return true;
    }

    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return scanExtensions().count(ext) > 0;
}

inline std::string formatTimePoint(
        const std::optional<std::chrono::system_clock::time_point>& timestamp) {
    if (!timestamp.has_value()) return "";
    auto days = std::chrono::floor<std::chrono::days>(*timestamp);
    std::chrono::year_month_day ymd{days};
    auto y = static_cast<int>(ymd.year());
    auto m = static_cast<unsigned>(ymd.month());
    auto d = static_cast<unsigned>(ymd.day());
    std::ostringstream oss;
    oss << y
        << '-' << (m < 10 ? "0" : "") << m
        << '-' << (d < 10 ? "0" : "") << d;
    return oss.str();
}

// Scan a single file for any legacy path references.
inline std::vector<ScanMatch> scanFile(const fs::path& file) {
    std::vector<ScanMatch> matches;

    std::ifstream ifs(file);
    if (!ifs.is_open()) return matches;

    std::string line;
    int line_num = 0;
    while (std::getline(ifs, line)) {
        ++line_num;

        for (const auto& [legacy, new_path] : themis::config::ConfigPathResolver::legacyPathMappings()) {
            if (line.find(legacy) != std::string::npos) {
                auto meta = themis::config::ConfigPathResolver::getMetadata(legacy);

                ScanMatch m;
                m.file        = file;
                m.line_number = line_num;
                m.legacy_path = legacy;
                m.new_path    = new_path;
                m.removal_due = meta && meta->isRemovalDue();

                if (meta) {
                    m.category            = meta->category;
                    m.deprecated_date     = formatTimePoint(meta->deprecated_date);
                    m.removal_date        = formatTimePoint(meta->removal_date);
                    m.migration_guide_url = meta->migration_guide_url.value_or("");
                } else {
                    m.category = "unknown";
                }

                matches.push_back(std::move(m));
            }
        }
    }
    return matches;
}

// Apply --fix: rewrite the file replacing legacy strings with new paths.
// Creates a .bak backup before modifying.
// Returns true on success (or when no change is needed), false on I/O error.
inline bool fixFile(const fs::path& file,
                    const std::vector<ScanMatch>& matches,
                    bool dry_run) {
    // Collect unique (legacy → new) replacements affecting this file
    std::vector<std::pair<std::string, std::string>> replacements;
    for (const auto& m : matches) {
        if (m.file == file) {
            auto it = std::find_if(replacements.begin(), replacements.end(),
                [&](const auto& p){ return p.first == m.legacy_path; });
            if (it == replacements.end()) {
                replacements.emplace_back(m.legacy_path, m.new_path);
            }
        }
    }
    if (replacements.empty()) return true;

    // Read original content
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "[ERROR] Cannot read: " << file << '\n';
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    ifs.close();

    // Build replacement content
    std::string updated = content;
    for (const auto& [legacy, new_p] : replacements) {
        std::string::size_type pos = 0;
        while ((pos = updated.find(legacy, pos)) != std::string::npos) {
            updated.replace(pos, legacy.size(), new_p);
            pos += new_p.size();
        }
    }

    if (updated == content) return true;  // No change needed

    if (dry_run) {
        std::cout << "[dry-run] Would update: " << file.string() << '\n';
        return true;
    }

    // Backup
    fs::path backup = fs::path(file.string() + ".bak");
    std::error_code ec;
    fs::copy_file(file, backup, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "[ERROR] Cannot create backup " << backup << ": " << ec.message() << '\n';
        return false;
    }

    // Write updated content
    std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "[ERROR] Cannot write: " << file << '\n';
        return false;
    }
    ofs << updated;
    ofs.close();
    std::cout << "[fixed] " << file.string() << " (backup: " << backup.string() << ")\n";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Output formatters
// ─────────────────────────────────────────────────────────────────────────────

inline void printText(const std::vector<ScanMatch>& matches) {
    for (const auto& m : matches) {
        std::cout << m.file.string() << ':' << m.line_number
                  << ": [" << m.category << "] "
                  << m.legacy_path << " -> " << m.new_path;
        if (!m.removal_date.empty()) {
            std::cout << " (removal: " << m.removal_date << ')';
            if (m.removal_due) std::cout << " [OVERDUE]";
        }
        if (!m.migration_guide_url.empty()) {
            std::cout << " guide: " << m.migration_guide_url;
        }
        std::cout << '\n';
    }
}

inline void printJson(const std::vector<ScanMatch>& matches) {
    std::cout << "[\n";
    for (std::size_t i = 0; i < matches.size(); ++i) {
        const auto& m = matches[i];
        auto escape = [](const std::string& s) -> std::string {
            std::string out;
            for (char c : s) {
                if (c == '"')  out += "\\\"";
                else if (c == '\\') out += "\\\\";
                else           out += c;
            }
            return out;
        };
        std::cout << "  {\n"
                  << "    \"file\": \""            << escape(m.file.string()) << "\",\n"
                  << "    \"line\": "              << m.line_number           << ",\n"
                  << "    \"legacy_path\": \""     << escape(m.legacy_path)   << "\",\n"
                  << "    \"new_path\": \""        << escape(m.new_path)      << "\",\n"
                  << "    \"category\": \""        << escape(m.category)      << "\",\n"
                  << "    \"deprecated_date\": \"" << m.deprecated_date       << "\",\n"
                  << "    \"removal_date\": \""    << m.removal_date          << "\",\n"
                  << "    \"removal_overdue\": "   << (m.removal_due ? "true" : "false") << ",\n"
                  << "    \"migration_guide\": \"" << escape(m.migration_guide_url) << "\"\n"
                  << "  }" << (i + 1 < matches.size() ? "," : "") << "\n";
    }
    std::cout << "]\n";
}

inline void printCsv(const std::vector<ScanMatch>& matches) {
    std::cout << "file,line,legacy_path,new_path,category,"
                 "deprecated_date,removal_date,removal_overdue,migration_guide\n";
    for (const auto& m : matches) {
        auto q = [](const std::string& s) -> std::string {
            if (s.find(',') == std::string::npos && s.find('"') == std::string::npos)
                return s;
            std::string out = "\"";
            for (char c : s) { if (c == '"') out += '"'; out += c; }
            out += '"';
            return out;
        };
        std::cout << q(m.file.string())        << ','
                  << m.line_number             << ','
                  << q(m.legacy_path)          << ','
                  << q(m.new_path)             << ','
                  << q(m.category)             << ','
                  << m.deprecated_date         << ','
                  << m.removal_date            << ','
                  << (m.removal_due ? "1":"0") << ','
                  << q(m.migration_guide_url)  << '\n';
    }
}

} // namespace cms

