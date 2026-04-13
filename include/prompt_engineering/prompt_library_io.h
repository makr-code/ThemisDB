/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_library_io.h                                ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 20:24:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     277                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3a592c29a2  2026-03-23  feat(prompt_engineering): Prompt Library Import/Export — ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_library_io.h
 * @brief Import/export of prompt template libraries to JSON and YAML (Phase 5 / v2.0.0).
 *
 * Enables cross-environment portability for prompt template collections.
 * A prompt library is serialised as a **`PromptLibraryBundle`**: a self-contained
 * document that carries the collection of `PromptManager::PromptTemplate` objects
 * together with bundle metadata and an FNV-1a integrity checksum.
 *
 * Key types
 * ---------
 *
 * - **`PromptLibraryBundle`** — name, description, version, format_version
 *   ("1.0"), created_at, checksum, templates.  `toJson()` / `fromJson()`.
 * - **`ExportFormat`** — `JSON` or `YAML`.
 * - **`ImportResult`** — success flag, templates_loaded count, error_message,
 *   checksum_valid flag.
 * - **`ExportResult`** — success flag, templates_written count, error_message.
 * - **`PromptLibraryIO`** — all-static utility class:
 *     * `exportToJson(bundle)` — pretty-printed JSON; embeds checksum.
 *     * `exportToYaml(bundle)` — YAML via yaml-cpp emitter; embeds checksum.
 *     * `exportToFile(bundle, path, fmt)` — format derived from extension when
 *       fmt == JSON and extension is `.yaml`/`.yml`.
 *     * `importFromJson(json_str)` → `optional<PromptLibraryBundle>`.
 *     * `importFromYaml(yaml_str)` → `optional<PromptLibraryBundle>`.
 *     * `importFromFile(path, out_bundle)` → `ImportResult`; auto-detects format.
 *     * `computeChecksum(bundle)` — FNV-1a 64-bit over sorted template JSON.
 *     * `verifyChecksum(bundle)` — `bundle.checksum == computeChecksum(bundle)`.
 *
 * Checksum algorithm
 * ------------------
 *
 * Each template is serialised to its canonical JSON representation (via
 * `PromptTemplate::toJson()`), the resulting strings are sorted by template id
 * for determinism, then concatenated and hashed with FNV-1a 64.  The hash is
 * stored as a zero-padded 16-character lowercase hex string.
 *
 * @see PromptManager::PromptTemplate
 */

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "prompt_engineering/prompt_manager.h"

namespace themis {
namespace prompt_engineering {

// ============================================================================
// ExportFormat
// ============================================================================

/** @brief Target serialisation format for `PromptLibraryIO::exportToFile()`. */
enum class ExportFormat { JSON, YAML };

// ============================================================================
// PromptLibraryBundle
// ============================================================================

/**
 * @brief A self-contained snapshot of a prompt template library.
 *
 * The bundle is the unit of cross-environment transfer.  It carries all
 * `PromptTemplate` objects together with enough metadata to detect version
 * mismatches and data corruption.
 */
struct PromptLibraryBundle {
    std::string name;                         ///< Human-readable library name.
    std::string description;                  ///< Short description.
    std::string version;                      ///< Semver-style snapshot version (e.g. "1.2.0").
    std::string format_version = "1.0";       ///< Bundle schema version for forward-compat.
    std::chrono::system_clock::time_point created_at; ///< Bundle creation timestamp.
    std::string checksum;                     ///< FNV-1a hex; computed by `PromptLibraryIO`.
    std::vector<PromptManager::PromptTemplate> templates;

    /** @brief Serialise to JSON (does NOT recompute checksum). */
    nlohmann::json toJson() const;

    /** @brief Deserialise from JSON. */
    static PromptLibraryBundle fromJson(const nlohmann::json& j);
};

// ============================================================================
// ImportResult
// ============================================================================

/** @brief Outcome of a `PromptLibraryIO::importFrom*()` operation. */
struct ImportResult {
    bool        success         = false;
    std::size_t templates_loaded = 0;   ///< Number of templates successfully parsed.
    std::string error_message;          ///< Non-empty on failure.
    bool        checksum_valid  = true; ///< `false` when stored checksum ≠ computed.
};

// ============================================================================
// ExportResult
// ============================================================================

/** @brief Outcome of a `PromptLibraryIO::exportToFile()` operation. */
struct ExportResult {
    bool        success          = false;
    std::size_t templates_written = 0;  ///< Number of templates included in the file.
    std::string error_message;          ///< Non-empty on failure.
};

// ============================================================================
// PromptLibraryIO
// ============================================================================

/**
 * @brief All-static import/export utilities for prompt library bundles.
 *
 * All methods are thread-safe; they operate on value types and hold no shared
 * state.  Callers are responsible for creating the `PromptLibraryBundle` and
 * populating `bundle.templates`; the IO methods handle serialisation only.
 *
 * Typical write path:
 * @code
 * PromptLibraryBundle bundle;
 * bundle.name        = "contracts";
 * bundle.version     = "2.0.0";
 * bundle.templates   = manager.listTemplates();
 * // checksum is computed automatically:
 * ExportResult r = PromptLibraryIO::exportToFile(bundle, "/tmp/contracts.yaml");
 * assert(r.success);
 * @endcode
 *
 * Typical read path:
 * @code
 * PromptLibraryBundle loaded;
 * ImportResult r = PromptLibraryIO::importFromFile("/tmp/contracts.yaml", loaded);
 * assert(r.success && r.checksum_valid);
 * for (auto& tpl : loaded.templates) { manager.createTemplate(tpl); }
 * @endcode
 */
class PromptLibraryIO {
public:
    // -------------------------------------------------------------------------
    // Export
    // -------------------------------------------------------------------------

    /**
     * @brief Serialise `bundle` to a pretty-printed JSON string (indent 2).
     *
     * If `bundle.checksum` is empty, the checksum is computed automatically.
     *
     * @param bundle Library bundle to serialise.
     * @return JSON string; never empty on success.
     */
    static std::string exportToJson(PromptLibraryBundle bundle);

    /**
     * @brief Serialise `bundle` to a YAML string (via yaml-cpp emitter).
     *
     * If `bundle.checksum` is empty, the checksum is computed automatically.
     * Template `metadata` is serialised as a JSON sub-string within the YAML.
     *
     * @param bundle Library bundle to serialise.
     * @return YAML string; never empty on success.
     */
    static std::string exportToYaml(PromptLibraryBundle bundle);

    /**
     * @brief Write `bundle` to the file at `path`.
     *
     * Format selection:
     *  - `fmt == ExportFormat::YAML`, **or** `path` ends with `.yaml`/`.yml`
     *    → YAML output.
     *  - Otherwise → JSON output.
     *
     * Parent directories must already exist.
     *
     * @param bundle Library bundle.
     * @param path   Destination file path.
     * @param fmt    Explicit format override (default: JSON; see above).
     * @return `ExportResult`; `success` is `false` if the file cannot be written.
     */
    static ExportResult exportToFile(PromptLibraryBundle bundle,
                                     const std::string&  path,
                                     ExportFormat        fmt = ExportFormat::JSON);

    // -------------------------------------------------------------------------
    // Import
    // -------------------------------------------------------------------------

    /**
     * @brief Parse a bundle from a JSON string.
     * @return `nullopt` if parsing fails (empty string, malformed JSON, missing
     *         required fields).
     */
    static std::optional<PromptLibraryBundle> importFromJson(
        const std::string& json_str);

    /**
     * @brief Parse a bundle from a YAML string (via yaml-cpp).
     * @return `nullopt` if parsing fails.
     */
    static std::optional<PromptLibraryBundle> importFromYaml(
        const std::string& yaml_str);

    /**
     * @brief Read and parse a bundle from the file at `path`.
     *
     * Format auto-detected from extension (`.yaml`/`.yml` → YAML; otherwise JSON).
     * Checksum is validated after parsing; `result.checksum_valid` is `false` on
     * mismatch but the bundle is still returned.
     *
     * @param path       Source file path.
     * @param out_bundle Populated on success.
     * @return `ImportResult`; `success` is `false` if the file cannot be read or
     *         parsed.
     */
    static ImportResult importFromFile(const std::string&  path,
                                        PromptLibraryBundle& out_bundle);

    // -------------------------------------------------------------------------
    // Checksum
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the FNV-1a 64-bit checksum of `bundle.templates`.
     *
     * Algorithm:
     *  1. For each template, call `toJson().dump()` to get a canonical string.
     *  2. Sort the strings by the `id` field for determinism.
     *  3. Concatenate all strings and compute FNV-1a 64-bit hash.
     *  4. Return as a zero-padded 16-character lowercase hex string.
     *
     * @return 16-character lowercase hex string.
     */
    static std::string computeChecksum(const PromptLibraryBundle& bundle);

    /**
     * @brief Verify that `bundle.checksum == computeChecksum(bundle)`.
     * @return `true` iff the stored checksum matches the freshly computed one.
     */
    static bool verifyChecksum(const PromptLibraryBundle& bundle);

private:
    /** @brief Detect YAML format from file extension. */
    static bool isYamlPath(const std::string& path) noexcept;

    /** @brief Parse a single PromptTemplate from a YAML::Node. */
    static PromptManager::PromptTemplate templateFromYamlNode(
        const void* node_ptr);
};

} // namespace prompt_engineering
} // namespace themis
