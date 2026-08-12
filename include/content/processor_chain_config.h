/**
 * @file processor_chain_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "content/content_type.h"

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Configuration for a single processing stage in the ingestion pipeline.
 *
 * Each stage can be independently enabled or disabled and optionally configured
 * with retry logic and graceful degradation (continue_on_error).
 */
struct StageConfig {
    bool enabled = true;            ///< Whether the stage is active. Defaults to true.
    int max_retries = 0;            ///< Maximum retry attempts on failure (0 = no retry).
    int retry_delay_ms = 100;       ///< Milliseconds to wait between retry attempts.
    bool continue_on_error = false; ///< If true, skip this stage on failure instead of aborting ingestion.
};

/**
 * @brief Per-content-type pipeline configuration specifying which stages to run.
 *
 * Controls the five main stages of the content ingestion pipeline:
 *  - extraction:    Text / metadata extraction (e.g. HTML boilerplate removal,
 *                   Markdown front-matter parsing, EXIF data, etc.)
 *  - chunking:      Splitting extracted text into index-ready chunks.
 *  - embedding:     Vector embedding generation (requires EmbeddingPipeline).
 *  - deduplication: Near-duplicate detection via pHash / MinHash
 *                   (requires DeduplicationChecker).
 *  - storage:       Final persistence via importContent(); retried independently.
 *
 * All stages default to enabled with no retries, preserving backward-compatible
 * behaviour.  Set `max_retries > 0` on a stage to enable retry on transient
 * failures.  Set `continue_on_error = true` (extraction) to continue with
 * degraded ingestion (no text chunks) when extraction fails.
 */
struct ContentTypePipelineConfig {
    StageConfig extraction;    ///< Text / metadata extraction stage.
    StageConfig chunking;      ///< Content chunking stage.
    StageConfig embedding;     ///< Embedding generation stage.
    StageConfig deduplication; ///< Near-duplicate detection stage.
    StageConfig storage;       ///< Storage (importContent) retry stage.
};

/**
 * @brief Configurable processor chain for the content ingestion pipeline.
 *
 * Allows fine-grained control over which processing stages run per content
 * type.  Look-up priority (highest to lowest):
 *  1. Per-MIME-type override   (`mime_type_configs`)
 *  2. Per-category override    (`category_configs`)
 *  3. Global default           (`default_config`)
 *
 * All stages are enabled by default so that existing behaviour is preserved
 * when no explicit configuration is set.
 *
 * Example – disable embedding for all image content:
 * @code
 *   ProcessorChainConfig cfg;
 *   ContentTypePipelineConfig img_cfg;
 *   img_cfg.embedding.enabled = false;
 *   cfg.category_configs[ContentCategory::IMAGE] = img_cfg;
 *   content_manager.setProcessorChainConfig(cfg);
 * @endcode
 *
 * Example – disable deduplication for a specific MIME type:
 * @code
 *   ProcessorChainConfig cfg;
 *   ContentTypePipelineConfig pdf_cfg;
 *   pdf_cfg.deduplication.enabled = false;
 *   cfg.mime_type_configs["application/pdf"] = pdf_cfg;
 *   content_manager.setProcessorChainConfig(cfg);
 * @endcode
 */
class ProcessorChainConfig {
public:
    /// Global default applied to all content types not matched by a more
    /// specific override.  All stages are enabled by default.
    ContentTypePipelineConfig default_config;

    /// Per-MIME-type overrides (highest priority).
    std::unordered_map<std::string, ContentTypePipelineConfig> mime_type_configs;

    /// Per-category overrides (lower priority than MIME-type overrides).
    std::unordered_map<ContentCategory, ContentTypePipelineConfig> category_configs;

    /**
     * @brief Return the effective pipeline config for the given MIME type and category.
     *
     * Priority: mime_type_configs > category_configs > default_config.
     *
     * @param mime_type  Detected MIME type (e.g. "text/html").
     * @param category   Detected content category enum value.
     * @return           Effective ContentTypePipelineConfig.
     */
    ContentTypePipelineConfig getEffectiveConfig(
        const std::string& mime_type,
        ContentCategory category
    ) const {
        // 1. Exact MIME type match (highest priority)
        auto mime_it = mime_type_configs.find(mime_type);
        if (mime_it != mime_type_configs.end()) {
            return mime_it->second;
        }

        // 2. Category match
        auto cat_it = category_configs.find(category);
        if (cat_it != category_configs.end()) {
            return cat_it->second;
        }

        // 3. Fall back to global default
        return default_config;
    }

    /**
     * @brief Deserialize from JSON.
     *
     * Each stage can be specified as a boolean (backward compatible) or as an
     * object with the full retry configuration:
     * @code
     * {
     *   "default": {
     *     "extraction": { "enabled": true, "max_retries": 2, "retry_delay_ms": 200, "continue_on_error": true },
     *     "chunking": true,
     *     "embedding": true,
     *     "deduplication": { "enabled": true, "max_retries": 1 },
     *     "storage": { "enabled": true, "max_retries": 3, "retry_delay_ms": 500 }
     *   },
     *   "mime_types": {
     *     "application/pdf": { "embedding": false }
     *   },
     *   "categories": {
     *     "IMAGE": { "deduplication": false }
     *   }
     * }
     * @endcode
     *
     * Boolean stage values retain backward compatibility (only `enabled` is set).
     * Omitted keys retain the default values.
     */
    static ProcessorChainConfig fromJson(const json& j) {
        ProcessorChainConfig cfg;

        // Parse a single StageConfig from a JSON value (bool or object).
        auto load_stage = [](const json& v) -> StageConfig {
            StageConfig s;
            if (v.is_boolean()) {
                s.enabled = v.get<bool>();
            } else if (v.is_object()) {
                if (v.contains("enabled") && v["enabled"].is_boolean())
                    s.enabled = v["enabled"].get<bool>();
                if (v.contains("max_retries") && v["max_retries"].is_number_integer())
                    s.max_retries = v["max_retries"].get<int>();
                if (v.contains("retry_delay_ms") && v["retry_delay_ms"].is_number_integer())
                    s.retry_delay_ms = v["retry_delay_ms"].get<int>();
                if (v.contains("continue_on_error") && v["continue_on_error"].is_boolean())
                    s.continue_on_error = v["continue_on_error"].get<bool>();
            }
            return s;
        };

        auto load_stage_cfg = [&load_stage](const json& obj) -> ContentTypePipelineConfig {
            ContentTypePipelineConfig c;
            if (obj.contains("extraction"))    c.extraction    = load_stage(obj["extraction"]);
            if (obj.contains("chunking"))      c.chunking      = load_stage(obj["chunking"]);
            if (obj.contains("embedding"))     c.embedding     = load_stage(obj["embedding"]);
            if (obj.contains("deduplication")) c.deduplication = load_stage(obj["deduplication"]);
            if (obj.contains("storage"))       c.storage       = load_stage(obj["storage"]);
            return c;
        };

        if (j.contains("default") && j["default"].is_object()) {
            cfg.default_config = load_stage_cfg(j["default"]);
        }

        if (j.contains("mime_types") && j["mime_types"].is_object()) {
            for (auto it = j["mime_types"].begin(); it != j["mime_types"].end(); ++it) {
                if (it.value().is_object()) {
                    cfg.mime_type_configs[it.key()] = load_stage_cfg(it.value());
                }
            }
        }

        if (j.contains("categories") && j["categories"].is_object()) {
            for (auto it = j["categories"].begin(); it != j["categories"].end(); ++it) {
                if (!it.value().is_object()) continue;
                const auto cat_it = categoryNames().find(it.key());
                if (cat_it == categoryNames().end()) continue;  // unknown – skip silently
                cfg.category_configs[cat_it->second] = load_stage_cfg(it.value());
            }
        }

        return cfg;
    }

    /**
     * @brief Serialize to JSON.
     *
     * Stages with non-default retry fields are serialized as objects; stages
     * with only `enabled` modified are serialized as booleans (backward compat).
     */
    json toJson() const {
        // Serialize a single StageConfig: boolean when retry fields are at defaults,
        // full object otherwise (preserves backward compatibility with old consumers).
        auto dump_stage = [](const StageConfig& s) -> json {
            if (s.max_retries == 0 && s.retry_delay_ms == 100 && !s.continue_on_error) {
                return s.enabled;  // backward-compatible simple form
            }
            return json{
                {"enabled",          s.enabled},
                {"max_retries",      s.max_retries},
                {"retry_delay_ms",   s.retry_delay_ms},
                {"continue_on_error", s.continue_on_error}
            };
        };

        auto dump_stage_cfg = [&dump_stage](const ContentTypePipelineConfig& c) -> json {
            return json{
                {"extraction",    dump_stage(c.extraction)},
                {"chunking",      dump_stage(c.chunking)},
                {"embedding",     dump_stage(c.embedding)},
                {"deduplication", dump_stage(c.deduplication)},
                {"storage",       dump_stage(c.storage)}
            };
        };

        json j;
        j["default"] = dump_stage_cfg(default_config);

        json mime_obj = json::object();
        for (const auto& [mime, cfg] : mime_type_configs) {
            mime_obj[mime] = dump_stage_cfg(cfg);
        }
        j["mime_types"] = mime_obj;

        json cat_obj = json::object();
        for (const auto& [cat, cfg] : category_configs) {
            const auto& names = categoryNameStrings();
            auto name_it = names.find(cat);
            if (name_it != names.end()) {
                cat_obj[name_it->second] = dump_stage_cfg(cfg);
            }
        }
        j["categories"] = cat_obj;

        return j;
    }

private:
    /// Shared map from category name string → ContentCategory enum (for deserialization).
    static const std::unordered_map<std::string, ContentCategory>& categoryNames() {
        static const std::unordered_map<std::string, ContentCategory> m = {
            {"TEXT",       ContentCategory::TEXT},
            {"IMAGE",      ContentCategory::IMAGE},
            {"GEO",        ContentCategory::GEO},
            {"CAD",        ContentCategory::CAD},
            {"AUDIO",      ContentCategory::AUDIO},
            {"VIDEO",      ContentCategory::VIDEO},
            {"STRUCTURED", ContentCategory::STRUCTURED},
            {"BINARY",     ContentCategory::BINARY},
            {"ARCHIVE",    ContentCategory::ARCHIVE},
            {"UNKNOWN",    ContentCategory::UNKNOWN},
        };
        return m;
    }

    /// Shared map from ContentCategory enum → name string (for serialization).
    static const std::unordered_map<ContentCategory, std::string>& categoryNameStrings() {
        static const std::unordered_map<ContentCategory, std::string> m = {
            {ContentCategory::TEXT,       "TEXT"},
            {ContentCategory::IMAGE,      "IMAGE"},
            {ContentCategory::GEO,        "GEO"},
            {ContentCategory::CAD,        "CAD"},
            {ContentCategory::AUDIO,      "AUDIO"},
            {ContentCategory::VIDEO,      "VIDEO"},
            {ContentCategory::STRUCTURED, "STRUCTURED"},
            {ContentCategory::BINARY,     "BINARY"},
            {ContentCategory::ARCHIVE,    "ARCHIVE"},
            {ContentCategory::UNKNOWN,    "UNKNOWN"},
        };
        return m;
    }
};

} // namespace content
} // namespace themis

