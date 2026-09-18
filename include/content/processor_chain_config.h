/**
 * @file processor_chain_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct StageConfig {
    bool enabled = true;            ///< Whether the stage is active. Defaults to true.
    int max_retries = 0;            ///< Maximum retry attempts on failure (0 = no retry).
    int retry_delay_ms = 100;       ///< Milliseconds to wait between retry attempts.
    bool continue_on_error = false; ///< If true, skip this stage on failure instead of aborting ingestion.
};

struct ContentTypePipelineConfig {
    StageConfig extraction;    ///< Text / metadata extraction stage.
    StageConfig chunking;      ///< Content chunking stage.
    StageConfig embedding;     ///< Embedding generation stage.
    StageConfig deduplication; ///< Near-duplicate detection stage.
    StageConfig storage;       ///< Storage (importContent) retry stage.
};

class ProcessorChainConfig {
public:
    ContentTypePipelineConfig default_config;

    std::unordered_map<std::string, ContentTypePipelineConfig> mime_type_configs;

    std::unordered_map<ContentCategory, ContentTypePipelineConfig> category_configs;

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
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: is_boolean(), is_object(), contains(), is_number_integer(), load_stage(), load_stage_cfg(), begin(), end().
     */
    static ProcessorChainConfig fromJson(const json& j) {
        ProcessorChainConfig cfg;

        // Parse a single StageConfig from a JSON value (bool or object).
        auto load_stage = [](const json& v) -> StageConfig {
            StageConfig s = {};
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
            ContentTypePipelineConfig c = {};
            if (obj.contains("extraction")) {
              c.extraction    = load_stage(obj["extraction"]);
            }
            if (obj.contains("chunking")) {
              c.chunking      = load_stage(obj["chunking"]);
            }
            if (obj.contains("embedding")) {
              c.embedding     = load_stage(obj["embedding"]);
            }
            if (obj.contains("deduplication")) {
              c.deduplication = load_stage(obj["deduplication"]);
            }
            if (obj.contains("storage")) {
              c.storage       = load_stage(obj["storage"]);
            }
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
                if (!it.value().is_object()) {
                  continue;
                }
                const auto cat_it = categoryNames().find(it.key());
                if (cat_it == categoryNames().end()) continue;  // unknown – skip silently
                cfg.category_configs[cat_it->second] = load_stage_cfg(it.value());
            }
        }

        return cfg;
    }

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

