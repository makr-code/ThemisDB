/**
 * @file huggingface_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <memory>

namespace themis {
namespace governance { class ModelGovernancePolicy; }
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

    /**
     * @brief Set the governance policy for data classification checks.
     *
     * When set, `initialize()` calls `policy->checkExportPermission()` with
     * purpose="DATA_INGESTION" before accepting the configuration.  If the
     * decision is DENY, `initialize()` returns false and logs the denial reason.
     * When @p policy is nullptr, the classification gate is bypassed and a
     * WARN is logged (Gap 8 degraded mode — AI_ML_IMPACT_ASSESSMENT.md §7).
     *
     * @param policy  Governance policy to consult; may be nullptr.
     */
    void setIngestionPolicy(
        std::shared_ptr<governance::ModelGovernancePolicy> policy);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
