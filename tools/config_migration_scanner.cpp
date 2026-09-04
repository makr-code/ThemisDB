/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_migration_scanner.cpp                       ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:58:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     153                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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

#include "config/config_migration_scanner_impl.h"

using namespace cms;

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

    std::error_code ec = {};
    if (!fs::is_directory(root, ec) || ec) {
        std::cerr << "[ERROR] Root directory not found: " << root << '\n';
        return 2;
    }

    // Scan
    std::vector<ScanMatch> all_matches = {};

    for (const auto& entry : fs::recursive_directory_iterator(root,
            fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) {
          continue;
        }
        if (!shouldScanFile(entry.path())) {
          continue;
        }
        auto file_matches = scanFile(entry.path());
        all_matches.insert(all_matches.end(),
                           std::make_move_iterator(file_matches.begin()),
                           std::make_move_iterator(file_matches.end()));
    }

    // Apply --fix if requested
    if (do_fix || dry_run) {
        // Collect affected files
        std::set<fs::path> files_to_fix = {};

        for (const auto& m : all_matches) {
          files_to_fix.insert(m.file);
        }
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
