/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            huggingface_connector.h                            ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:39:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 25f69a572f  2026-03-09  feat(ingestion): replace simulated HttpClient in HuggingF... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • eda6e27def  2026-02-28  fix(ingestion): reject_invalid=false mode, schema_violati... ║
    • b40bbc1612  2026-02-26  feat(ingestion): OAuth 2.0 token refresh handling in Gene... ║
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
 * @brief HuggingFace dataset connector
 * 
 * Downloads datasets from HuggingFace Hub via REST API and ingests them
 * into ThemisDB. Supports streaming for large datasets.
 * 
 * Example usage:
 * @code
 * HuggingFaceConnector connector;
 * SourceConfig config{
 *     .source_id = "hf_legal",
 *     .type = SourceType::HUGGINGFACE,
 *     .location = "lexlms/ger_legal_data",
 *     .options = {{"split", "train"}, {"streaming", "true"}}
 * };
 * connector.initialize(config);
 * auto stats = connector.ingest("legal_documents", nullptr);
 * @endcode
 */
class HuggingFaceConnector : public ISourceConnector {
public:
    /**
     * @brief Construct HuggingFace connector
     */
    HuggingFaceConnector();
    
    ~HuggingFaceConnector() override;
    
    // Delete copy
    HuggingFaceConnector(const HuggingFaceConnector&) = delete;
    HuggingFaceConnector& operator=(const HuggingFaceConnector&) = delete;
    
    /**
     * @brief Initialize connector with configuration
     * @param config Source configuration with:
     *        - location: dataset name (e.g., "lexlms/ger_legal_data")
     *        - options["split"]: dataset split (train/test/validation)
     *        - options["streaming"]: "true" for streaming mode
     *        - options["token"]: HuggingFace API token (optional)
     *        - options["revision"]: dataset revision/branch (optional)
     * @return true if initialization successful
     */
    bool initialize(const SourceConfig& config) override;
    
    /**
     * @brief Check if HuggingFace Hub is accessible
     * @return true if dataset can be accessed
     */
    bool isAvailable() const override;
    
    /**
     * @brief Get total number of documents in dataset
     * @return Document count (may require pre-loading metadata)
     */
    size_t getDocumentCount() const override;
    
    /**
     * @brief Ingest documents from HuggingFace dataset
     * @param target_collection Target collection in ThemisDB
     * @param progress_callback Optional progress callback
     * @return Ingestion statistics
     */
    IngestionStats ingest(const std::string& target_collection,
                         ProgressCallback progress_callback) override;
    
    /**
     * @brief Set API token for authenticated access
     * @param token HuggingFace API token
     */
    void setApiToken(const std::string& token);
    
    /**
     * @brief Set batch size for ingestion
     * @param batch_size Number of documents to process per batch
     */
    void setBatchSize(size_t batch_size);
    
    /**
     * @brief Enable/disable streaming mode
     * @param enabled Whether to use streaming (recommended for large datasets)
     */
    void setStreamingMode(bool enabled);

    /**
     * @brief Configure retry and timeout behaviour
     * @param config Retry settings (attempts, back-off, timeout)
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Configure OAuth 2.0 token refresh for this connector
     *
     * When set, the connector automatically attempts a token refresh
     * (RFC 6749 §6) upon receiving HTTP 401 from the HuggingFace Hub API.
     * The refreshed access token is cached inside the connector and used for
     * all subsequent requests in the same ingestion run.
     *
     * `config.access_token` is used immediately as the Bearer token; if it is
     * empty, the static API token (set via `setApiToken()` or
     * `options["token"]`) is used until the first refresh.
     *
     * @param config  OAuth configuration including token endpoint, client
     *                credentials, and the refresh token.
     */
    void setOAuthConfig(const OAuthConfig& config);

    /**
     * @brief Inject a mock HTTP GET function for dataset API calls (unit testing only)
     *
     * When set, every HTTP GET that would normally be performed via libcurl
     * is replaced by a call to @p fn.  The function receives the URL and
     * the Bearer token, and returns `{status_code, response_body}`.
     *
     * Pass an empty `ApiHttpGetFn{}` to restore the real libcurl path.
     */
    void setHttpGetForTesting(ApiHttpGetFn fn);

    /**
     * @brief Inject a mock HTTP POST function for OAuth token refresh (unit testing only)
     *
     * When set, every token-refresh POST that would normally be performed
     * via libcurl is replaced by a call to @p fn.  The function receives the
     * token endpoint URL and the URL-encoded form body, and returns
     * `{status_code, response_body}`.
     *
     * Pass an empty `ApiHttpPostFn{}` to restore the real libcurl path.
     */
    void setHttpPostForTesting(ApiHttpPostFn fn);

    /**
     * @brief Inject a per-document validator called before each write.
     *
     * When set, the validator is called for every document counted in a
     * streaming chunk.  Documents that fail validation are counted as
     * failed (not processed).
     * Pass an empty `DocumentValidatorFn` to remove a previously set validator.
     *
     * @param validator Validator callback; empty = disable
     */
    void setDocumentValidator(DocumentValidatorFn validator) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
