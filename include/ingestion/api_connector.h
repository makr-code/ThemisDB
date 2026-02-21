/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_connector.h                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     122                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a6af0c040  2026-02-20  feat(ingestion): Production-ready ingestion module – stru... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ingestion_manager.h"
#include <string>
#include <memory>

namespace themis {
namespace ingestion {

/**
 * @brief Generic REST API source connector
 *
 * Fetches documents from a paginated REST/JSON API endpoint and ingests them
 * into ThemisDB.  Pagination is driven by a configurable cursor/offset
 * parameter; retries use the shared `RetryConfig` back-off logic.
 *
 * Supported `SourceConfig::options` keys:
 * | Key             | Description                                           | Default  |
 * |-----------------|-------------------------------------------------------|----------|
 * | `api_key`       | Bearer token sent in `Authorization: Bearer <token>`  | (none)   |
 * | `page_size`     | Items requested per page                              | `100`    |
 * | `cursor_param`  | Query-parameter name for the page cursor              | `offset` |
 * | `text_field`    | JSON key whose value is treated as the document text  | `text`   |
 * | `max_pages`     | Maximum pages to fetch (0 = unlimited)                | `0`      |
 *
 * Example usage:
 * @code
 * SourceConfig cfg{
 *     .source_id = "my_api",
 *     .type      = SourceType::API,
 *     .location  = "https://api.example.com/v1/documents",
 *     .options   = {{"api_key","secret"},{"page_size","50"},{"text_field","content"}}
 * };
 * GenericApiConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 */
class GenericApiConnector : public ISourceConnector {
public:
    GenericApiConnector();
    ~GenericApiConnector() override;

    // Non-copyable
    GenericApiConnector(const GenericApiConnector&) = delete;
    GenericApiConnector& operator=(const GenericApiConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration
     * @param config  Must have `type == SourceType::API`; `location` is the
     *                base endpoint URL.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /** @brief Poll the endpoint to check availability (HTTP 200) */
    bool isAvailable() const override;

    /**
     * @brief Estimate the total document count
     *
     * Returns the value from a `total` or `count` field in the first-page
     * response, or 0 when the API does not expose a total count.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief Ingest documents from the API endpoint
     *
     * Pages through the endpoint using the configured cursor parameter until
     * no more documents are returned or `max_pages` is reached.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Override the API key (alternative to `options["api_key"]`)
     */
    void setApiKey(const std::string& key);

    /**
     * @brief Override the page size (alternative to `options["page_size"]`)
     */
    void setPageSize(size_t page_size);

    /**
     * @brief Configure retry and timeout behaviour
     */
    void setRetryConfig(const RetryConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
