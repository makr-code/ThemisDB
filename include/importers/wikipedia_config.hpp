#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::importers {

using json = nlohmann::json;

/**
 * @brief Embedding hook configuration used by the vector projection.
 */

/**
 * @file wikipedia_config.hpp
 * @brief Configuration types for the Wikipedia import plugin.
 *
 * Declares WikipediaConfig and related options controlling parser behaviour,
 * batch sizes, language filtering, and category inclusion rules.
 */
struct WikipediaEmbeddingHookConfig {
    bool enabled = false;
    std::string provider = {};
    std::string model = "hook:unbound";

    [[nodiscard]] json toJson() const {
        return json{{"enabled", enabled}, {"provider", provider}, {"model", model}};
    }

    static WikipediaEmbeddingHookConfig fromJson(const json& j) {
        WikipediaEmbeddingHookConfig config;
        config.enabled = j.value("enabled", false);
        config.provider = j.value("provider", std::string{});
        config.model = j.value("model", std::string{"hook:unbound"});
        return config;
    }
};

/**
 * @brief Export configuration for the portable wikipedia.db artifact.
 */
struct WikipediaPortableExportConfig {
    std::string database_path = "wikipedia.db";
    std::string manifest_path = "manifest.json";
    bool write_validation_report = true;

    [[nodiscard]] json toJson() const {
        return json{
            {"database_path", database_path},
            {"manifest_path", manifest_path},
            {"write_validation_report", write_validation_report}
        };
    }

    static WikipediaPortableExportConfig fromJson(const json& j) {
        WikipediaPortableExportConfig config;
        config.database_path = j.value("database_path", std::string{"wikipedia.db"});
        config.manifest_path = j.value("manifest_path", std::string{"manifest.json"});
        config.write_validation_report = j.value("write_validation_report", true);
        return config;
    }
};

/**
 * @brief Runtime configuration for the Wikipedia ingestion MVP.
 *
 * Supports strict versus best-effort ingestion, checkpoint cadence, multi-model
 * projection toggles, and vendor-neutral external-tool compatibility hints.
 */
struct WikipediaIngestionConfig {
    bool strict_mode = false;
    bool best_effort = true;
    size_t checkpoint_interval_pages = 100;
    std::string checkpoint_path;
    std::string dead_letter_path;
    std::string importer_version = "0.1.0";
    bool enable_graph_projection = true;
    bool enable_vector_projection = true;
    bool enable_process_projection = true;
    bool enable_timeseries_projection = true;
    WikipediaEmbeddingHookConfig embedding;
    WikipediaPortableExportConfig export_config;
    std::vector<std::string> external_tool_references{
        "mwdumper-compatible XML preprocessing",
        "WikiExtractor-style article text cleanup",
        "Wikimedia dump tooling ecosystem: https://meta.wikimedia.org/wiki/Data_dumps/Other_tools"
    };

    [[nodiscard]] json toJson() const {
        return json{
            {"strict_mode", strict_mode},
            {"best_effort", best_effort},
            {"checkpoint_interval_pages", checkpoint_interval_pages},
            {"checkpoint_path", checkpoint_path},
            {"dead_letter_path", dead_letter_path},
            {"importer_version", importer_version},
            {"enable_graph_projection", enable_graph_projection},
            {"enable_vector_projection", enable_vector_projection},
            {"enable_process_projection", enable_process_projection},
            {"enable_timeseries_projection", enable_timeseries_projection},
            {"embedding", embedding.toJson()},
            {"export_config", export_config.toJson()},
            {"external_tool_references", external_tool_references}
        };
    }

    static WikipediaIngestionConfig fromJson(const json& j) {
        WikipediaIngestionConfig config;
        config.strict_mode = j.value("strict_mode", false);
        config.best_effort = j.value("best_effort", true);
        config.checkpoint_interval_pages = j.value("checkpoint_interval_pages", static_cast<size_t>(100));
        config.checkpoint_path = j.value("checkpoint_path", std::string{});
        config.dead_letter_path = j.value("dead_letter_path", std::string{});
        config.importer_version = j.value("importer_version", std::string{"0.1.0"});
        config.enable_graph_projection = j.value("enable_graph_projection", true);
        config.enable_vector_projection = j.value("enable_vector_projection", true);
        config.enable_process_projection = j.value("enable_process_projection", true);
        config.enable_timeseries_projection = j.value("enable_timeseries_projection", true);
        if (j.contains("embedding") && j["embedding"].is_object()) {
            config.embedding = WikipediaEmbeddingHookConfig::fromJson(j["embedding"]);
        }
        if (j.contains("export_config") && j["export_config"].is_object()) {
            config.export_config = WikipediaPortableExportConfig::fromJson(j["export_config"]);
        }
        if (j.contains("external_tool_references") && j["external_tool_references"].is_array()) {
            config.external_tool_references = j["external_tool_references"].get<std::vector<std::string>>();
        }
        return config;
    }
};

} // namespace themis::importers
