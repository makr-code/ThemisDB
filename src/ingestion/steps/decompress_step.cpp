/**
 * @file decompress_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.4.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/ingestion_step.h"
#include "ingestion/builtin_step_factories.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <chrono>

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace themis {
namespace ingestion {
namespace builtin {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helper: fork/execvp without shell
// ─────────────────────────────────────────────────────────────────────────────

static bool runProcess(const std::vector<const char*>& argv_vec) {
#if defined(_WIN32)
    (void)argv_vec;
    return false;
#else
    std::vector<const char*> argv = argv_vec;
    argv.push_back(nullptr);

    pid_t pid = ::fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        // child
        ::execvp(argv[0], const_cast<char* const*>(argv.data()));
        ::_exit(127);
    }
    // parent: wait
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Walk a directory tree and collect all regular file paths
// ─────────────────────────────────────────────────────────────────────────────

static void collectPaths(const std::string& dir,
                          std::vector<std::string>& out) {
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(dir,
            fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_regular_file(ec)) {
            out.push_back(entry.path().string());
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DecompressStep
// ─────────────────────────────────────────────────────────────────────────────

/** @brief DecompressStep. */
class DecompressStep final : public IIngestionStep {
public:
    // IThemisPlugin boilerplate
    const char* getName()    const override { return "builtin.decompress"; }
    const char* getVersion() const override { return "1.4.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override {
        return {
            "application/zip",
            "application/gzip",
            "application/x-gzip",
            "application/x-tar",
            "application/x-bzip2",
            "application/x-xz",
            "application/x-compressed-tar"
        };
    }

    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        const std::string& source = ctx.manifest.original_path;
        if (source.empty()) {
            ctx.warnings.push_back("decompress: manifest.original_path is empty — skipping");
            return {};
        }

        // ── Determine archive type ─────────────────────────────────────────
        const std::string& mime = ctx.manifest.detected_mime;
        const std::string  ext  = fs::path(source).extension().string();

        enum class ArchiveKind { ZIP, TGZ, TAR, TBZ2, TXZ, UNKNOWN };
        ArchiveKind kind = ArchiveKind::UNKNOWN;

        if (mime == "application/zip" || ext == ".zip") {
            kind = ArchiveKind::ZIP;
        } else if (mime == "application/gzip" || mime == "application/x-gzip"
                   || ext == ".gz" || ext == ".tgz") {
            kind = ArchiveKind::TGZ;
        } else if (mime == "application/x-tar" || ext == ".tar") {
            kind = ArchiveKind::TAR;
        } else if (mime == "application/x-bzip2" || ext == ".bz2" || ext == ".tbz2") {
            kind = ArchiveKind::TBZ2;
        } else if (mime == "application/x-xz" || ext == ".xz") {
            kind = ArchiveKind::TXZ;
        }

        if (kind == ArchiveKind::UNKNOWN) {
            ctx.warnings.push_back(
                "decompress: unrecognised archive type mime='" + mime +
                "' ext='" + ext + "' — skipping");
            return {};
        }

        // ── Resolve output directory ───────────────────────────────────────
        std::string output_dir;
        if (cfg.config.contains("output_dir") &&
            cfg.config["output_dir"].is_string()) {
            output_dir = cfg.config["output_dir"].get<std::string>();
            std::error_code ec;
            fs::create_directories(output_dir, ec);
        } else {
            const auto base = fs::temp_directory_path();
            const auto unique = "themis_decompress_"
                + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
            output_dir = (base / unique).string();
            std::error_code ec;
            fs::create_directories(output_dir, ec);
            if (ec) {
                return ErrVoid(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                    "decompress: create_directories failed: " + ec.message());
            }
        }

        // ── Store depth hint in extra ──────────────────────────────────────
        int depth = 1;
        if (cfg.config.contains("max_depth") && cfg.config["max_depth"].is_number()) {
            depth = cfg.config["max_depth"].get<int>();
        }
        ctx.extra["decompress.depth"] = std::to_string(depth);

        // ── Invoke platform tool ───────────────────────────────────────────
        bool ok = false;
        switch (kind) {
        case ArchiveKind::ZIP:
            ok = runProcess({"unzip", "-o", source.c_str(), "-d", output_dir.c_str()});
            break;
        case ArchiveKind::TGZ:
            ok = runProcess({"tar", "-xzf", source.c_str(), "-C", output_dir.c_str()});
            break;
        case ArchiveKind::TAR:
            ok = runProcess({"tar", "-xf",  source.c_str(), "-C", output_dir.c_str()});
            break;
        case ArchiveKind::TBZ2:
            ok = runProcess({"tar", "-xjf", source.c_str(), "-C", output_dir.c_str()});
            break;
        case ArchiveKind::TXZ:
            ok = runProcess({"tar", "-xJf", source.c_str(), "-C", output_dir.c_str()});
            break;
        default:
            break;
        }

        if (!ok) {
            ctx.warnings.push_back(
                "decompress: extraction tool exited with non-zero status for '" + source + "'");
            // Non-fatal: still collect any partial output
        }

        // ── Collect extracted paths ────────────────────────────────────────
        collectPaths(output_dir, ctx.extracted_file_paths);

        ctx.extra["decompress.output_dir"]     = output_dir;
        ctx.extra["decompress.extracted_count"] =
            std::to_string(ctx.extracted_file_paths.size());

        return {};
    }
};

std::shared_ptr<IIngestionStep> createDecompressStep() {
    return std::make_shared<DecompressStep>();
}

} // namespace builtin
} // namespace ingestion
} // namespace themis

