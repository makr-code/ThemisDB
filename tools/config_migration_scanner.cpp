/*
 * config_migration_scanner – CLI tool that scans a deployment directory tree
 * for files referencing legacy config paths and outputs a migration report.
 *
 * Usage:
 *   config_migration_scanner [--root <dir>] [--output {text|json|csv}]
 *                            [--dry-run] [--fix]
 *
 * Flags:
 *   --root <dir>          Directory to scan (default: current working directory)
 *   --output text|json|csv  Output format (default: text)
 *   --dry-run             Show what would be renamed without modifying files
 *   --fix                 Rewrite files replacing legacy path strings with new
 *                         paths; creates .bak backups before any modification
 *
 * Exit codes:
 *   0  – No legacy paths found (or only paths within their deprecation window)
 *   1  – At least one path whose removal_date has already passed was found
 *   2  – Usage / argument error
 */

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

namespace fs = std::filesystem;
using namespace themis::config;

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

struct ScanMatch {
    fs::path   file;
    int        line_number;
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

static const std::set<std::string> kScanExtensions = {
    ".yaml", ".yml", ".json", ".toml", ".ini", ".env"
};

static bool shouldScanFile(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return kScanExtensions.count(ext) > 0;
}

static std::string formatTimePoint(
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
static std::vector<ScanMatch> scanFile(const fs::path& file) {
    std::vector<ScanMatch> matches;

    std::ifstream ifs(file);
    if (!ifs.is_open()) return matches;

    std::string line;
    int line_num = 0;
    while (std::getline(ifs, line)) {
        ++line_num;

        // Check every key in the legacyPathMappings against this line.
        for (const auto& [legacy, new_path] : ConfigPathResolver::legacyPathMappings()) {
            if (line.find(legacy) != std::string::npos) {
                auto meta = ConfigPathResolver::getMetadata(legacy);

                ScanMatch m;
                m.file        = file;
                m.line_number = line_num;
                m.legacy_path = legacy;
                m.new_path    = new_path;
                m.removal_due = meta && meta->isRemovalDue();

                if (meta) {
                    m.category           = meta->category;
                    m.deprecated_date    = formatTimePoint(meta->deprecated_date);
                    m.removal_date       = formatTimePoint(meta->removal_date);
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
static bool fixFile(const fs::path& file,
                    const std::vector<ScanMatch>& matches,
                    bool dry_run) {
    // Collect unique (legacy → new) replacements affecting this file
    std::vector<std::pair<std::string,std::string>> replacements;
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

static void printText(const std::vector<ScanMatch>& matches) {
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

static void printJson(const std::vector<ScanMatch>& matches) {
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

static void printCsv(const std::vector<ScanMatch>& matches) {
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
        std::cout << q(m.file.string())       << ','
                  << m.line_number            << ','
                  << q(m.legacy_path)         << ','
                  << q(m.new_path)            << ','
                  << q(m.category)            << ','
                  << m.deprecated_date        << ','
                  << m.removal_date           << ','
                  << (m.removal_due ? "1":"0")<< ','
                  << q(m.migration_guide_url) << '\n';
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

static void printUsage(const char* prog) {
    std::cerr
        << "Usage: " << prog
        << " [--root <dir>] [--output text|json|csv] [--dry-run] [--fix]\n"
        << "\n"
        << "Scans a deployment directory tree for files referencing legacy config\n"
        << "paths and outputs a migration report.\n"
        << "\n"
        << "Options:\n"
        << "  --root <dir>           Directory to scan (default: .)\n"
        << "  --output text|json|csv Output format (default: text)\n"
        << "  --dry-run              Show what --fix would change, without modifying files\n"
        << "  --fix                  Rewrite files in-place (creates .bak backups)\n"
        << "\n"
        << "Exit codes:\n"
        << "  0  No overdue legacy paths found\n"
        << "  1  At least one path past its removal_date was found\n"
        << "  2  Argument error\n";
}

int main(int argc, char* argv[]) {
    fs::path root = ".";
    std::string output_format = "text";
    bool do_fix   = false;
    bool dry_run  = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--root") {
            if (++i >= argc) {
                std::cerr << "[ERROR] --root requires an argument\n";
                printUsage(argv[0]);
                return 2;
            }
            root = argv[i];
        } else if (arg == "--output") {
            if (++i >= argc) {
                std::cerr << "[ERROR] --output requires an argument\n";
                printUsage(argv[0]);
                return 2;
            }
            output_format = argv[i];
            if (output_format != "text" && output_format != "json" && output_format != "csv") {
                std::cerr << "[ERROR] --output must be one of: text, json, csv\n";
                return 2;
            }
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "--fix") {
            do_fix = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "[ERROR] Unknown argument: " << arg << '\n';
            printUsage(argv[0]);
            return 2;
        }
    }

    std::error_code ec;
    if (!fs::is_directory(root, ec) || ec) {
        std::cerr << "[ERROR] Root directory not found: " << root << '\n';
        return 2;
    }

    // Scan
    std::vector<ScanMatch> all_matches;
    for (const auto& entry : fs::recursive_directory_iterator(root,
            fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;
        if (!shouldScanFile(entry.path())) continue;
        auto file_matches = scanFile(entry.path());
        all_matches.insert(all_matches.end(),
                           std::make_move_iterator(file_matches.begin()),
                           std::make_move_iterator(file_matches.end()));
    }

    // Apply --fix if requested
    if (do_fix || dry_run) {
        // Collect affected files
        std::set<fs::path> files_to_fix;
        for (const auto& m : all_matches) files_to_fix.insert(m.file);
        for (const auto& f : files_to_fix) {
            fixFile(f, all_matches, dry_run);
        }
    }

    // Output report
    if (output_format == "json") {
        printJson(all_matches);
    } else if (output_format == "csv") {
        printCsv(all_matches);
    } else {
        printText(all_matches);
    }

    // Exit code 1 if any removal-overdue path was found
    bool any_overdue = std::any_of(all_matches.begin(), all_matches.end(),
                                   [](const ScanMatch& m){ return m.removal_due; });
    return any_overdue ? 1 : 0;
}
