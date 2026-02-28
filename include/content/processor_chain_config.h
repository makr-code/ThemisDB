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
 * Each stage can be independently enabled or disabled.
 */
struct StageConfig {
    bool enabled = true;  ///< Whether the stage is active. Defaults to true.
};

/**
 * @brief Per-content-type pipeline configuration specifying which stages to run.
 *
 * Controls the four main stages of the content ingestion pipeline:
 *  - extraction:    Text / metadata extraction (e.g. HTML boilerplate removal,
 *                   Markdown front-matter parsing, EXIF data, etc.)
 *  - chunking:      Splitting extracted text into index-ready chunks.
 *  - embedding:     Vector embedding generation (requires EmbeddingPipeline).
 *  - deduplication: Near-duplicate detection via pHash / MinHash
 *                   (requires DeduplicationChecker).
 *
 * All stages default to enabled, preserving backward-compatible behaviour.
 */
struct ContentTypePipelineConfig {
    StageConfig extraction;    ///< Text / metadata extraction stage.
    StageConfig chunking;      ///< Content chunking stage.
    StageConfig embedding;     ///< Embedding generation stage.
    StageConfig deduplication; ///< Near-duplicate detection stage.
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
     * Expected JSON structure:
     * @code
     * {
     *   "default": { "extraction": true, "chunking": true, "embedding": true, "deduplication": true },
     *   "mime_types": {
     *     "application/pdf": { "embedding": false }
     *   },
     *   "categories": {
     *     "IMAGE": { "deduplication": false }
     *   }
     * }
     * @endcode
     *
     * Omitted keys retain the default value of `true`.
     */
    static ProcessorChainConfig fromJson(const json& j) {
        ProcessorChainConfig cfg;

        auto load_stage_cfg = [](const json& obj) -> ContentTypePipelineConfig {
            ContentTypePipelineConfig c;
            if (obj.contains("extraction") && obj["extraction"].is_boolean())
                c.extraction.enabled = obj["extraction"].get<bool>();
            if (obj.contains("chunking") && obj["chunking"].is_boolean())
                c.chunking.enabled = obj["chunking"].get<bool>();
            if (obj.contains("embedding") && obj["embedding"].is_boolean())
                c.embedding.enabled = obj["embedding"].get<bool>();
            if (obj.contains("deduplication") && obj["deduplication"].is_boolean())
                c.deduplication.enabled = obj["deduplication"].get<bool>();
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
     */
    json toJson() const {
        auto dump_stage_cfg = [](const ContentTypePipelineConfig& c) -> json {
            return json{
                {"extraction",    c.extraction.enabled},
                {"chunking",      c.chunking.enabled},
                {"embedding",     c.embedding.enabled},
                {"deduplication", c.deduplication.enabled}
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
