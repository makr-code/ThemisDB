#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace themis::importers {

using json = nlohmann::json;

/**
 * @brief Durable checkpoint state for restart-safe full and incremental imports.
 */

/**
 * @file wikipedia_checkpoint.hpp
 * @brief Checkpoint support for the Wikipedia import pipeline.
 *
 * Defines serialisable checkpoint state that allows an interrupted
 * Wikipedia XML dump ingestion to resume from the last committed offset.
 */
struct WikipediaCheckpointState {
    std::string source_path;
    std::string source_id;
    size_t processed_pages = 0;
    size_t processed_revisions = 0;
    uint64_t last_page_id = 0;
    std::string last_page_title;
    size_t imported_pages = 0;
    size_t failed_pages = 0;
    std::string updated_at;

    [[nodiscard]] json toJson() const {
        return json{
            {"source_path", source_path},
            {"source_id", source_id},
            {"processed_pages", processed_pages},
            {"processed_revisions", processed_revisions},
            {"last_page_id", last_page_id},
            {"last_page_title", last_page_title},
            {"imported_pages", imported_pages},
            {"failed_pages", failed_pages},
            {"updated_at", updated_at}
        };
    }

    static WikipediaCheckpointState fromJson(const json& j) {
        WikipediaCheckpointState state;
        state.source_path = j.value("source_path", std::string{});
        state.source_id = j.value("source_id", std::string{});
        state.processed_pages = j.value("processed_pages", static_cast<size_t>(0));
        state.processed_revisions = j.value("processed_revisions", static_cast<size_t>(0));
        state.last_page_id = j.value("last_page_id", static_cast<uint64_t>(0));
        state.last_page_title = j.value("last_page_title", std::string{});
        state.imported_pages = j.value("imported_pages", static_cast<size_t>(0));
        state.failed_pages = j.value("failed_pages", static_cast<size_t>(0));
        state.updated_at = j.value("updated_at", std::string{});
        return state;
    }
};

/**
 * @brief JSON-backed checkpoint store used by the MVP importer.
 */
class WikipediaCheckpointStore {
public:
    explicit WikipediaCheckpointStore(std::string path = {});

    void setPath(std::string path);
    [[nodiscard]] const std::string& path() const;
    [[nodiscard]] bool hasPath() const;
    [[nodiscard]] bool exists() const;
    [[nodiscard]] WikipediaCheckpointState load() const;
    [[nodiscard]] bool save(const WikipediaCheckpointState& state) const;
    [[nodiscard]] bool clear() const;

private:
    std::string path_;
};

} // namespace themis::importers
