/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_provenance_cli.cpp                            ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-15 18:58:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     499                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file lora_provenance_cli.cpp
 * @brief CLI tool for LoRA Adapter Provenance management in ThemisDB.
 *
 * Commands:
 *   lora-provenance show-provenance <adapter_id> [--json]
 *       Print the cryptographic provenance record for a locally trained adapter.
 *
 *   lora-provenance list-snapshots <adapter_id>
 *       List all MVCC snapshots for an adapter (oldest first).
 *
 *   lora-provenance verify <adapter_id>
 *       Verify the integrity of the Merkle-chained inference audit log.
 *
 *   lora-provenance export-audit <adapter_id> [--output <file>]
 *       Export the full audit log as JSON Lines (JSONL) to stdout or a file.
 *
 *   lora-provenance import-external <adapter_id> <provenance_json_file>
 *             [--trusted-ca <ca_pem_file>] [--allow-unsigned]
 *       Import an external adapter and validate its provenance.
 *
 *   lora-provenance hash-file <path>
 *       Compute and print the SHA-256 hash of a file.
 *
 * Exit codes:
 *   0 – success
 *   1 – verification / validation failure
 *   2 – usage error
 *   3 – I/O or internal error
 *
 * Example usage:
 *   lora-provenance verify legal-lora-v2
 *   lora-provenance show-provenance legal-lora-v2 --json
 *   lora-provenance export-audit legal-lora-v2 --output /tmp/audit.jsonl
 *   lora-provenance import-external ext-adapter provenance.json \
 *       --trusted-ca /etc/ssl/certs/ca-certificates.crt
 *   lora-provenance hash-file /models/mistral-7b.gguf
 */

#include "llm/lora_framework/lora_provenance.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::llm::lora;

// ============================================================================
// ANSI colour helpers
// ============================================================================

namespace Color {
    const std::string Reset  = "\033[0m";
    const std::string Bold   = "\033[1m";
    const std::string Green  = "\033[32m";
    const std::string Yellow = "\033[33m";
    const std::string Red    = "\033[31m";
    const std::string Cyan   = "\033[36m";
}

static bool use_color = true;

static std::string ok()   { return use_color ? Color::Green  + "OK"   + Color::Reset : "OK";   }
static std::string fail() { return use_color ? Color::Red    + "FAIL" + Color::Reset : "FAIL"; }
static std::string warn() { return use_color ? Color::Yellow + "WARN" + Color::Reset : "WARN"; }

// ============================================================================
// Argument parsing helpers
// ============================================================================

static bool flag(const std::vector<std::string>& args, const std::string& name) {
    for (const auto& a : args)
        if (a == name) {
          return true;
        }
    return false;
}

static std::string opt(const std::vector<std::string>& args, const std::string& name,
                        const std::string& default_val = "") {
    for (size_t i = 0; i + 1 < args.size(); ++i)
        if (args[i] == name) {
          return args[i + 1];
        }
    return default_val;
}

// ============================================================================
// File helpers
// ============================================================================

static std::string readFile(const std::string& path, bool& ok_out) {
    std::ifstream f(path);
    if (!f.is_open()) { ok_out = false; return {}; }
    ok_out = true;
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

// ============================================================================
// Command implementations
// ============================================================================

// lora-provenance show-provenance <adapter_id> [--json]
static int cmdShowProvenance(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: lora-provenance show-provenance <adapter_id> [--json]\n";
        return 2;
    }
    const std::string adapter_id = args[0];
    const bool as_json = flag(args, "--json");

    LoRAProvenanceManager mgr;
    // NOTE: In a production deployment the manager would be initialised from a
    // persistent store (RocksDB / blob storage).  This CLI acts as a standalone
    // tool that reads provenance data from a JSONL file produced by export-audit
    // or populated programmatically.  For demonstration purposes we attempt to
    // read a sidecar provenance file "<adapter_id>.provenance.json" from the
    // current working directory.
    const std::string prov_path = adapter_id + ".provenance.json";
    bool file_ok = false;
    const std::string prov_raw = readFile(prov_path, file_ok);

    if (!file_ok) {
        std::cerr << "[" << warn() << "] Provenance file not found: " << prov_path << "\n";
        std::cerr << "       Create it with: lora-provenance export-audit <adapter_id>\n";
        return 3;
    }

    try {
        const auto j        = json::parse(prov_raw);
        const auto record   = LoRAProvenanceRecord::fromJSON(j);

        if (as_json) {
            std::cout << record.toJSON().dump(2) << "\n";
        } else {
            std::cout << Color::Bold << "Provenance for adapter: " << adapter_id
                      << Color::Reset << "\n";
            std::cout << std::left;
            auto row = [&](const char* label, const std::string& val) {
                std::cout << "  " << std::setw(26) << label << val << "\n";
            };
            row("dataset_hash:",         record.dataset_hash.empty()
                                             ? "(not set)" : record.dataset_hash);
            row("base_model_hash:",      record.base_model_hash.empty()
                                             ? "(not set)" : record.base_model_hash);
            row("hyperparameter_hash:",  record.hyperparameter_hash.empty()
                                             ? "(not set)" : record.hyperparameter_hash);
            row("adapter_weights_hash:", record.adapter_weights_hash.empty()
                                             ? "(not set)" : record.adapter_weights_hash);
            row("trainer_id:",           record.trainer_id.empty()
                                             ? "(not set)" : record.trainer_id);
            row("created_at:",           record.created_at.empty()
                                             ? "(not set)" : record.created_at);
            row("rfc3161_timestamp:",    record.rfc3161_timestamp.empty()
                                             ? "(not set)"
                                             : "[present, " +
                                               std::to_string(record.rfc3161_timestamp.size()) +
                                               " bytes base64]");
            row("ca_chain:",             record.ca_chain.empty()
                                             ? "(not set)"
                                             : "[present, " +
                                               std::to_string(record.ca_chain.size()) +
                                               " bytes PEM]");
            row("training_duration:",    std::to_string(record.training_duration_secs) + "s");
            if (!record.hardware_info.empty())
                row("hardware_info:",    record.hardware_info.dump());
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << fail() << "] Failed to parse provenance file: " << e.what() << "\n";
        return 3;
    }

    return 0;
}

// lora-provenance list-snapshots <adapter_id>
static int cmdListSnapshots(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: lora-provenance list-snapshots <adapter_id>\n";
        return 2;
    }
    const std::string adapter_id = args[0];

    // Load snapshots from sidecar file "<adapter_id>.snapshots.jsonl"
    const std::string snap_path = adapter_id + ".snapshots.jsonl";
    bool file_ok = false;
    const std::string raw = readFile(snap_path, file_ok);

    if (!file_ok) {
        std::cerr << "[" << warn() << "] Snapshots file not found: " << snap_path << "\n";
        return 3;
    }

    std::vector<AdapterSnapshot> snaps;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
          continue;
        }
        try {
            snaps.push_back(AdapterSnapshot::fromJSON(json::parse(line)));
        } catch (...) { std::cerr << "warning: skipping malformed snapshot entry for adapter '" << adapter_id << "'\n"; }
    }

    if (snaps.empty()) {
        std::cout << "No snapshots found for adapter: " << adapter_id << "\n";
        return 0;
    }

    std::cout << Color::Bold << "Snapshots for adapter: " << adapter_id
              << Color::Reset << " (" << snaps.size() << " total)\n\n";
    std::cout << std::left
              << std::setw(34) << "Snapshot ID"
              << std::setw(12) << "Version"
              << std::setw(30) << "Timestamp"
              << "Parent\n";
    std::cout << std::string(90, '-') << "\n";

    for (const auto& s : snaps) {
        std::cout << std::setw(34) << s.snapshot_id
                  << std::setw(12) << s.version
                  << std::setw(30) << s.timestamp
                  << (s.parent_snapshot_id.empty() ? "(root)" : s.parent_snapshot_id)
                  << "\n";
    }

    return 0;
}

// lora-provenance verify <adapter_id>
static int cmdVerify(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: lora-provenance verify <adapter_id>\n";
        return 2;
    }
    const std::string adapter_id = args[0];

    // Load audit log from sidecar file "<adapter_id>.audit.jsonl"
    const std::string audit_path = adapter_id + ".audit.jsonl";
    bool file_ok = false;
    const std::string raw = readFile(audit_path, file_ok);

    if (!file_ok) {
        std::cerr << "[" << warn() << "] Audit log file not found: " << audit_path << "\n";
        std::cerr << "       Export it first with: lora-provenance export-audit " << adapter_id << "\n";
        return 3;
    }

    LoRAProvenanceManager mgr;
    std::vector<InferenceAuditEntry> entries;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
          continue;
        }
        try {
            auto e = InferenceAuditEntry::fromJSON(json::parse(line));
            entries.push_back(std::move(e));
        } catch (...) { std::cerr << "warning: skipping malformed audit entry for adapter '" << adapter_id << "'\n"; }
    }

    if (entries.empty()) {
        std::cout << "[" << ok() << "] Empty audit log — trivially valid.\n";
        return 0;
    }

    // Replay entries through the manager so verifyAuditChain() can check them
    for (auto& e : entries) {
        // Re-append: use the stored entry_id and timestamp to preserve the chain
        mgr.appendAuditEntry(adapter_id, e);
    }

    const bool valid = mgr.verifyAuditChain(adapter_id);

    if (valid) {
        std::cout << "[" << ok() << "] Merkle audit chain is intact ("
                  << entries.size() << " entries).\n";
        return 0;
    } else {
        std::cerr << "[" << fail() << "] Merkle audit chain verification FAILED for: "
                  << adapter_id << "\n";
        return 1;
    }
}

// lora-provenance export-audit <adapter_id> [--output <file>]
static int cmdExportAudit(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: lora-provenance export-audit <adapter_id> [--output <file>]\n";
        return 2;
    }
    const std::string adapter_id = args[0];
    const std::string out_path   = opt(args, "--output");

    // Load audit log from sidecar file
    const std::string audit_path = adapter_id + ".audit.jsonl";
    bool file_ok = false;
    const std::string raw = readFile(audit_path, file_ok);

    if (!file_ok) {
        std::cerr << "[" << warn() << "] Audit log file not found: " << audit_path << "\n";
        return 3;
    }

    // Determine output destination
    std::ostream* out = &std::cout;
    std::ofstream file_out;
    if (!out_path.empty()) {
        file_out.open(out_path);
        if (!file_out.is_open()) {
            std::cerr << "[" << fail() << "] Cannot open output file: " << out_path << "\n";
            return 3;
        }
        out = &file_out;
    }

    // Stream JSONL lines with additional metadata
    size_t count = 0;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
          continue;
        }
        *out << line << "\n";
        ++count;
    }

    if (!out_path.empty()) {
        std::cout << "[" << ok() << "] Exported " << count
                  << " audit entries to: " << out_path << "\n";
    }

    return 0;
}

// lora-provenance import-external <adapter_id> <provenance_json_file>
//     [--trusted-ca <pem_file>] [--allow-unsigned]
static int cmdImportExternal(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: lora-provenance import-external <adapter_id> <provenance_json_file>\n"
                  << "           [--trusted-ca <pem_file>] [--allow-unsigned]\n";
        return 2;
    }
    const std::string adapter_id  = args[0];
    const std::string prov_file   = args[1];
    const std::string ca_file     = opt(args, "--trusted-ca");
    const bool allow_unsigned     = flag(args, "--allow-unsigned");

    // Read provenance JSON
    bool file_ok = false;
    const std::string prov_raw = readFile(prov_file, file_ok);
    if (!file_ok) {
        std::cerr << "[" << fail() << "] Cannot read provenance file: " << prov_file << "\n";
        return 3;
    }

    // Read trusted CA bundle (optional)
    std::string trusted_ca;
    if (!ca_file.empty()) {
        bool ca_ok = false;
        trusted_ca = readFile(ca_file, ca_ok);
        if (!ca_ok) {
            std::cerr << "[" << fail() << "] Cannot read CA file: " << ca_file << "\n";
            return 3;
        }
    }

    // Parse provenance
    ExternalAdapterProvenance prov;
    try {
        prov = ExternalAdapterProvenance::fromJSON(json::parse(prov_raw));
    } catch (const std::exception& e) {
        std::cerr << "[" << fail() << "] Invalid provenance JSON: " << e.what() << "\n";
        return 3;
    }

    // Run import/validation
    LoRAProvenanceManager mgr;
    const auto result = mgr.importExternalAdapter(adapter_id, prov, trusted_ca, allow_unsigned);

    if (!result.validation_errors.empty()) {
        std::cerr << "[" << fail() << "] Provenance validation failed for: " << adapter_id << "\n";
        for (const auto& err : result.validation_errors) {
            std::cerr << "    - " << err << "\n";
        }
        return 1;
    }

    std::cout << "[" << ok() << "] External adapter provenance validated and imported.\n";
    std::cout << "  adapter_id:       " << adapter_id << "\n";
    std::cout << "  source_url:       " << result.source_url << "\n";
    std::cout << "  adapter_hash:     " << result.adapter_hash << "\n";
    std::cout << "  signature_valid:  " << (result.signature_valid  ? "yes" : "no") << "\n";
    std::cout << "  cert_chain_valid: " << (result.cert_chain_valid ? "yes" : "no") << "\n";
    std::cout << "  import_timestamp: " << result.import_timestamp << "\n";

    return 0;
}

// lora-provenance hash-file <path>
static int cmdHashFile(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: lora-provenance hash-file <path>\n";
        return 2;
    }
    const std::string path = args[0];
    const std::string hash = LoRAProvenanceManager::sha256File(path);
    if (hash.empty()) {
        std::cerr << "[" << fail() << "] Cannot hash file (not found or unreadable): "
                  << path << "\n";
        return 3;
    }
    std::cout << hash << "  " << path << "\n";
    return 0;
}

// ============================================================================
// Help
// ============================================================================

static void printHelp(const char* argv0) {
    std::cout << Color::Bold << "lora-provenance" << Color::Reset
              << " — ThemisDB LoRA Adapter Provenance CLI\n\n";
    std::cout << "Usage: " << argv0 << " <command> [args] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  show-provenance <adapter_id> [--json]\n"
              << "      Print provenance record (reads <adapter_id>.provenance.json)\n\n";
    std::cout << "  list-snapshots <adapter_id>\n"
              << "      List MVCC snapshots (reads <adapter_id>.snapshots.jsonl)\n\n";
    std::cout << "  verify <adapter_id>\n"
              << "      Verify Merkle audit chain (reads <adapter_id>.audit.jsonl)\n"
              << "      Exit code 0 = intact, 1 = tampered/corrupt\n\n";
    std::cout << "  export-audit <adapter_id> [--output <file>]\n"
              << "      Export audit log as JSONL (reads <adapter_id>.audit.jsonl)\n\n";
    std::cout << "  import-external <adapter_id> <provenance_json_file>\n"
              << "               [--trusted-ca <pem_file>] [--allow-unsigned]\n"
              << "      Import external adapter and validate its provenance\n\n";
    std::cout << "  hash-file <path>\n"
              << "      Compute SHA-256 hash of a file (compatible with sha256sum)\n\n";
    std::cout << "Options:\n";
    std::cout << "  --no-color    Disable ANSI color output\n";
    std::cout << "  --help, -h    Print this help\n";
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[]) {
    std::vector<std::string> all_args(argv + 1, argv + argc);

    // Global flags
    if (flag(all_args, "--no-color")) {
      use_color = false;
    }

    if (all_args.empty() || flag(all_args, "--help") || flag(all_args, "-h")) {
        printHelp(argv[0]);
        return all_args.empty() ? 2 : 0;
    }

    const std::string command = all_args[0];
    const std::vector<std::string> cmd_args(all_args.begin() + 1, all_args.end());

    static const std::unordered_map<std::string,
        int(*)(const std::vector<std::string>&)> dispatch = {
        {"show-provenance", cmdShowProvenance},
        {"list-snapshots",  cmdListSnapshots},
        {"verify",          cmdVerify},
        {"export-audit",    cmdExportAudit},
        {"import-external", cmdImportExternal},
        {"hash-file",       cmdHashFile},
    };

    auto it = dispatch.find(command);
    if (it == dispatch.end()) {
        std::cerr << "Unknown command: " << command << "\n"
                  << "Run '" << argv[0] << " --help' for usage.\n";
        return 2;
    }

    return it->second(cmd_args);
}
