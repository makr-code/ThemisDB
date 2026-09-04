/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_cli.cpp                                     ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:58:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     352                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e841bec3a2  2026-03-11  feat(exporters): complete validate_template - add CLI fla... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// export_cli.cpp
// ThemisDB Export CLI
//
// Usage:
//   themisdb-export --collection <name> --output <file> [options]
//
// Options:
//   --collection <name>       Source collection name (required)
//   --output <path>           Output file (or directory for huggingface format)
//   --format <fmt>            Output format: jsonl (default), parquet, arrow,
//                             arrow_stream, huggingface, streaming, incremental
//   --incremental             Alias for --format incremental (EXP-004)
//   --watermark <file>        Watermark file path for incremental exports
//   --filter <aql>            AQL predicate to filter exported records
//   --include-field <name>    Include only this field (repeatable)
//   --exclude-field <name>    Exclude this field (repeatable)
//   --compress                Enable GZIP compression on output
//   --compress-level <n>      Compression level 1-9 (default: 6)
//   --user <id>               Requesting user ID (for audit / policy checks)
//   --progress                Print progress to stderr
//   --output-json             Print final ExportStats as JSON to stdout
//   --validate-template <tpl> Dry-run: validate that all entities in the
//                             collection have the required fields for the given
//                             template (alpaca|sharegpt|chatml|openai).
//                             Prints missing fields to stderr and exits with
//                             code 1 when any are absent; exits 0 on success.
//                             No output file is written.
//   --template-instruction <f> Override instruction_field name for --validate-template
//   --template-input <f>      Override input_field name
//   --template-output <f>     Override output_field name
//   --template-system <f>     Override system_field name
//   --template-user <f>       Override user_field name
//   --template-assistant <f>  Override assistant_field name
//   --help                    Show this help message
//
// Exit codes:
//   0  Export completed successfully  (or --validate-template: all fields present)
//   1  Export failed (fatal error)    (or --validate-template: missing fields found)
//   2  Export completed with non-fatal errors
//   3  Invalid arguments

#include "exporters/export_format_registry.h"
#include "exporters/exporter_interface.h"
#include "exporters/format_template.h"
#include "exporters/incremental_exporter.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/streaming_exporter.h"
#include "storage/base_entity.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace themis::exporters;

// ── Usage ────────────────────────────────────────────────────────────────────

static void printUsage(const char* prog) {
    std::cerr <<
        "Usage: " << prog << " --collection <name> --output <path> [options]\n"
        "\n"
        "Options:\n"
        "  --collection <name>       Source collection name (required)\n"
        "  --output <path>           Output file or directory (required, unless --validate-template)\n"
        "  --format <fmt>            Output format: jsonl (default), parquet, arrow,\n"
        "                            arrow_stream, huggingface, streaming, incremental\n"
        "  --incremental             Use incremental/delta exporter (alias for --format incremental)\n"
        "  --watermark <file>        Watermark file for incremental exports (default: <output>.watermark.json)\n"
        "  --filter <aql>            AQL predicate to filter exported records\n"
        "  --include-field <name>    Include only this field (repeatable)\n"
        "  --exclude-field <name>    Exclude this field (repeatable)\n"
        "  --compress                Enable GZIP compression\n"
        "  --compress-level <n>      GZIP compression level 1-9 (default: 6)\n"
        "  --user <id>               Requesting user ID (for audit logs)\n"
        "  --progress                Print export progress to stderr\n"
        "  --output-json             Print ExportStats JSON to stdout on completion\n"
        "  --validate-template <tpl> Dry-run: check that collection fields satisfy\n"
        "                            the template (alpaca|sharegpt|chatml|openai).\n"
        "                            Exits 0 when all fields are present; 1 if any\n"
        "                            required field is missing. No file is written.\n"
        "  --template-instruction <f> Field name for 'instruction' (default: question)\n"
        "  --template-input <f>      Field name for 'input'        (default: context)\n"
        "  --template-output <f>     Field name for 'output'       (default: answer)\n"
        "  --template-system <f>     Field name for 'system'       (default: system_prompt)\n"
        "  --template-user <f>       Field name for 'user'         (default: user_message)\n"
        "  --template-assistant <f>  Field name for 'assistant'    (default: assistant_response)\n"
        "  --help                    Show this message\n"
        "\n"
        "Exit codes:\n"
        "  0  Success (or --validate-template: all required fields present)\n"
        "  1  Fatal error (or --validate-template: missing required fields)\n"
        "  2  Completed with non-fatal errors\n"
        "  3  Invalid arguments\n";
}

// ── Argument parsing ─────────────────────────────────────────────────────────

struct ExportCliConfig {
    std::string collection_name;
    std::string output_path;
    std::string format       = "jsonl";
    bool incremental         = false;
    std::string watermark_path;
    std::string filter_expression;
    std::vector<std::string> include_fields;
    std::vector<std::string> exclude_fields;
    bool compress            = false;
    int compress_level       = 6;
    std::string user_id;
    bool show_progress       = false;
    bool output_json         = false;

    // --validate-template mode
    std::string validate_template;   // empty = disabled; else template name
    FormatTemplateFieldMapping template_mapping;  // field-name overrides
};

static bool parseArgs(int argc, char** argv, ExportCliConfig& cfg) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "error: " << arg << " requires an argument\n";
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--help") { printUsage(argv[0]); std::exit(0); }
        else if (arg == "--collection")    { const char* v = next(); if (!v) return false; cfg.collection_name = v; }
        else if (arg == "--output")        { const char* v = next(); if (!v) return false; cfg.output_path     = v; }
        else if (arg == "--format")        { const char* v = next(); if (!v) return false; cfg.format          = v; }
        else if (arg == "--incremental")   { cfg.incremental = true; cfg.format = "incremental"; }
        else if (arg == "--watermark")     { const char* v = next(); if (!v) return false; cfg.watermark_path  = v; }
        else if (arg == "--filter")        { const char* v = next(); if (!v) return false; cfg.filter_expression = v; }
        else if (arg == "--include-field") { const char* v = next(); if (!v) return false; cfg.include_fields.emplace_back(v); }
        else if (arg == "--exclude-field") { const char* v = next(); if (!v) return false; cfg.exclude_fields.emplace_back(v); }
        else if (arg == "--compress")      { cfg.compress = true; }
        else if (arg == "--compress-level"){ const char* v = next(); if (!v) return false; cfg.compress_level = std::stoi(v); }
        else if (arg == "--user")          { const char* v = next(); if (!v) return false; cfg.user_id         = v; }
        else if (arg == "--progress")      { cfg.show_progress = true; }
        else if (arg == "--output-json")   { cfg.output_json   = true; }
        else if (arg == "--validate-template")    { const char* v = next(); if (!v) return false; cfg.validate_template = v; }
        else if (arg == "--template-instruction") { const char* v = next(); if (!v) return false; cfg.template_mapping.instruction_field = v; }
        else if (arg == "--template-input")       { const char* v = next(); if (!v) return false; cfg.template_mapping.input_field       = v; }
        else if (arg == "--template-output")      { const char* v = next(); if (!v) return false; cfg.template_mapping.output_field      = v; }
        else if (arg == "--template-system")      { const char* v = next(); if (!v) return false; cfg.template_mapping.system_field      = v; }
        else if (arg == "--template-user")        { const char* v = next(); if (!v) return false; cfg.template_mapping.user_field        = v; }
        else if (arg == "--template-assistant")   { const char* v = next(); if (!v) return false; cfg.template_mapping.assistant_field   = v; }
        else {
            std::cerr << "error: unknown argument '" << arg << "'\n";
            return false;
        }
    }
    return true;
}

// ── Minimal stub collection loader ───────────────────────────────────────────
// In production this would load from RocksDB; here we surface the interface
// so that external callers (server, tests) can inject entities.

static std::vector<themis::BaseEntity> loadCollection(const std::string& collection_name) {
    // The export CLI delegates loading to the caller via stdin (JSON-Lines) or
    // an embedded store.  This implementation reads from stdin when the collection
    // name starts with "@" (pipe mode) or returns an empty vector (dry-run friendly).
    std::vector<themis::BaseEntity> entities;

    if (collection_name.size() > 1 && collection_name[0] == '@') {
        // Pipe mode: "@" prefix means read JSONL from stdin
        const std::string actual_name = collection_name.substr(1);
        std::string line = {};
        size_t idx = 0;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
              continue;
            }
            try {
                entities.push_back(
                    themis::BaseEntity::fromJson("doc_" + std::to_string(idx++), line));
            } catch (...) {
                // Skip malformed lines
            }
        }
        return entities;
    }

    // No embedded store in this standalone tool; the caller must supply data
    // via the "@collection_name" pipe convention or extend this function.
    std::cerr << "info: collection '" << collection_name << "' not loaded (no embedded store in export_cli).\n"
              << "      Use '@" << collection_name << "' to pipe JSONL from stdin.\n";
    return entities;
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 3;
    }

    ExportCliConfig cfg = {};
    if (!parseArgs(argc, argv, cfg)) {
        printUsage(argv[0]);
        return 3;
    }

    if (cfg.collection_name.empty()) {
        std::cerr << "error: --collection is required\n";
        return 3;
    }

    // ── validate-template dry-run mode ───────────────────────────────────────
    if (!cfg.validate_template.empty()) {
        // Map name → FormatTemplateType
        FormatTemplateType tpl_type = FormatTemplateType::NONE;
        const std::string& tname = cfg.validate_template;
        if      (tname == "alpaca") {
          tpl_type = FormatTemplateType::ALPACA;
        }
        else if (tname == "sharegpt")           tpl_type = FormatTemplateType::SHAREGPT;
        else if (tname == "chatml")             tpl_type = FormatTemplateType::CHATML;
        else if (tname == "openai" ||
                 tname == "openai_finetuning")  tpl_type = FormatTemplateType::OPENAI_FINETUNING;
        else {
            std::cerr << "error: unknown template '" << tname << "'\n"
                      << "       supported: alpaca, sharegpt, chatml, openai\n";
            return 3;
        }

        const auto entities = loadCollection(cfg.collection_name);
        const auto result   = validateTemplate(tpl_type, cfg.template_mapping, entities);

        if (result.valid) {
            std::cout << "validate-template: OK ("
                      << result.entities_checked << " entities checked, template '"
                      << tname << "')\n";
            return 0;
        }

        std::cerr << "validate-template: FAILED — " << result.entities_failed
                  << " / " << result.entities_checked
                  << " entities missing required fields (template '" << tname << "'):\n";
        for (const auto& f : result.missing_fields) {
            std::cerr << "  missing field: " << f << '\n';
        }
        return 1;
    }

    if (cfg.output_path.empty()) {
        std::cerr << "error: --output is required\n";
        return 3;
    }

    // Load entities
    const auto entities = loadCollection(cfg.collection_name);

    // Build ExportOptions
    ExportOptions opts;
    opts.output_path        = cfg.output_path;
    opts.collection_name    = cfg.collection_name;
    opts.requesting_user    = cfg.user_id;
    opts.filter_expression  = cfg.filter_expression;
    opts.include_fields     = cfg.include_fields;
    opts.exclude_fields     = cfg.exclude_fields;
    opts.compress           = cfg.compress;
    opts.compression_level  = cfg.compress_level;
    if (cfg.show_progress) {
        opts.progress_callback = [](const ExportStats& s) {
            std::cerr << "\r  Exported: " << s.exported_entities
                      << "  Bytes: " << s.bytes_written
                      << std::flush;
        };
        opts.progress_interval = 500;
    }

    // Build exporter
    std::unique_ptr<IExporter> exporter;

    if (cfg.format == "incremental" || cfg.incremental) {
        IncrementalExportConfig icfg;
        icfg.watermark_path = cfg.watermark_path.empty()
            ? cfg.output_path + ".watermark.json"
            : cfg.watermark_path;
        exporter = std::make_unique<IncrementalExporter>(icfg);
    } else {
        ExportFormatRegistry::instance().registerBuiltins();
        if (!ExportFormatRegistry::instance().hasFormat(cfg.format)) {
            std::cerr << "error: unknown format '" << cfg.format << "'\n"
                      << "Known formats:";
            for (const auto& f : ExportFormatRegistry::instance().registeredFormats()) {
                std::cerr << " " << f;
            }
            std::cerr << '\n';
            return 3;
        }
        exporter = ExportFormatRegistry::instance().createExporter(cfg.format);
    }

    // Run
    ExportStats stats;
    try {
        stats = exporter->exportEntities(entities, opts);
    } catch (const std::exception& e) {
        std::cerr << "error: export failed: " << e.what() << '\n';
        return 1;
    }

    if (cfg.show_progress) {
        std::cerr << '\n';
    }

    if (cfg.output_json) {
        std::cout << stats.toJson() << '\n';
    } else {
        std::cout << "Exported " << stats.exported_entities
                  << " / " << stats.total_entities << " entities"
                  << " (" << stats.bytes_written << " bytes)\n";
        if (!stats.errors.empty()) {
            std::cerr << stats.errors.size() << " non-fatal error(s):\n";
            for (const auto& e : stats.errors) {
                std::cerr << "  " << e << '\n';
            }
        }
    }

    return stats.errors.empty() ? 0 : 2;
}
