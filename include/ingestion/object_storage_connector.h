/**
 * @file object_storage_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=1, Unimpl=0, Mock=5, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace themis {
namespace ingestion {

/**
 * @brief Object-storage provider selection
 */
enum class ObjectStorageProvider {
    S3,     ///< AWS S3 or any S3-compatible endpoint (MinIO, GCS interop)
    GCS,    ///< Google Cloud Storage native API
    AZURE   ///< Azure Blob Storage
};

/**
 * @brief Object-storage source connector for S3, GCS, and Azure Blob
 *
 * Lists objects in a bucket/container (optionally filtered by prefix) and
 * ingests them as documents into ThemisDB.  The provider is selected via
 * `SourceConfig::options["provider"]`:
 *   - `"s3"`    → AWS S3 / S3-compatible (requires `THEMIS_ENABLE_S3`)
 *   - `"gcs"`   → Google Cloud Storage (requires `THEMIS_ENABLE_GCS`)
 *   - `"azure"` → Azure Blob Storage (requires `THEMIS_ENABLE_AZURE`)
 *
 * Without the corresponding compile-time flag the connector compiles but
 * always returns `CONNECTOR_NOT_SUPPORTED` — unless a test mock has been
 * injected via `setObjectListForTesting()` / `setObjectFetchForTesting()`.
 *
 * Supported `SourceConfig::options` keys:
 * | Key                  | Provider     | Description                                         | Default          |
 * |----------------------|--------------|-----------------------------------------------------|------------------|
 * | `provider`           | all          | `"s3"`, `"gcs"`, or `"azure"`                       | `s3`             |
 * | `bucket`             | S3 / GCS     | Bucket name (overrides `SourceConfig::location`)     | `location` field |
 * | `container`          | Azure        | Container name (overrides `SourceConfig::location`)  | `location` field |
 * | `prefix`             | all          | Key/blob prefix to filter listed objects             | (none)           |
 * | `region`             | S3           | AWS region (e.g. `us-east-1`)                        | `us-east-1`      |
 * | `endpoint_url`       | S3           | Custom endpoint URL for S3-compatible stores         | (AWS default)    |
 * | `access_key`         | S3           | AWS access key ID (never logged)                     | (env / IAM role) |
 * | `secret_key`         | S3           | AWS secret access key (never logged)                 | (env / IAM role) |
 * | `project_id`         | GCS          | GCP project ID                                       | (ADC)            |
 * | `service_account_json` | GCS        | Path to service-account JSON key file (never logged) | (ADC)            |
 * | `connection_string`  | Azure        | Azure storage connection string (never logged)       | (env)            |
 * | `sas_token`          | Azure        | Shared access signature token (never logged)         | (none)           |
 * | `max_keys`           | all          | Max objects to list per run (0 = unlimited)          | `0`              |
 * | `text_field`         | all          | JSON key to use as document text for `.json` objects | `text`           |
 *
 * `SourceConfig::location` is used as the bucket / container name unless
 * the provider-specific override key is set.
 *
 * Example – S3:
 * @code
 * SourceConfig cfg{
 *     .source_id = "data_lake",
 *     .type      = SourceType::OBJECT_STORAGE,
 *     .location  = "my-bucket",
 *     .options   = {{"provider","s3"},
 *                   {"prefix","docs/2026/"},
 *                   {"region","eu-west-1"},
 *                   {"max_keys","500"}}
 * };
 * ObjectStorageConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 *
 * Example via IngestionBuilder:
 * @code
 * auto mgr = IngestionBuilder("mydb")
 *     .withObjectStorageSource("lake", "my-bucket",
 *                              {{"provider","s3"},{"prefix","2026/"}})
 *     .build();
 * auto report = mgr->ingestAll();
 * @endcode
 */
class ObjectStorageConnector : public ISourceConnector {
public:
    ObjectStorageConnector();
    ~ObjectStorageConnector() override;

    // Non-copyable
    ObjectStorageConnector(const ObjectStorageConnector&) = delete;
    ObjectStorageConnector& operator=(const ObjectStorageConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration.
     * @param config  Must have `type == SourceType::OBJECT_STORAGE`.
     *                `location` is the bucket / container name.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /**
     * @brief Check whether the bucket / container is reachable.
     *
     * Returns true when a test mock is injected.  Without a mock the result
     * depends on whether the required provider SDK flag is enabled and the
     * credentials are valid.
     */
    bool isAvailable() const override;

    /**
     * @brief Returns 0 — object counts are not cheaply available without a
     * full bucket listing.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief List objects in the bucket and ingest each one as a document.
     *
     * For each listed object key the connector downloads the object body,
     * extracts text (plain text, or the configured `text_field` from a JSON
     * object), and accumulates statistics.  Pagination continues until all
     * objects under the prefix have been processed or `max_keys` is reached.
     *
     * When neither a test mock nor a provider SDK is available, returns
     * immediately with a `CONNECTOR_NOT_SUPPORTED` error.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Configure retry behaviour for this connector.
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Function type for providing object-key batches.
     *
     * Usable both for tests and for custom production integrations that want to
     * bridge an external object-storage listing backend into the connector.
     *
     * Each call should return the next batch of object keys.  Return an empty
     * vector to signal end-of-listing.
     */
    using ObjectListFn = std::function<std::vector<std::string>()>;

    /**
     * @brief Function type for providing object bodies.
     *
     * Usable both for tests and for custom production integrations that want to
     * bridge an external object fetch backend into the connector.
     *
     * Given an object key the function returns the raw object body as a
     * string.  Return an empty string to simulate a fetch failure.
     */
    using ObjectFetchFn = std::function<std::string(const std::string& key)>;

    /**
     * @brief Inject object-listing and object-fetch providers.
     *
     * When both are set, every cloud API call is replaced by these functions.
     * This is suitable both for tests and for custom production bridges to
     * non-native or out-of-process object-storage backends.
     *
     * Pass empty `ObjectListFn{}` / `ObjectFetchFn{}` to restore the real
     * provider path.
     */
    void setObjectListProvider(ObjectListFn list_fn);
    void setObjectFetchProvider(ObjectFetchFn fetch_fn);
    void setObjectListForTesting(ObjectListFn list_fn);
    void setObjectFetchForTesting(ObjectFetchFn fetch_fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
