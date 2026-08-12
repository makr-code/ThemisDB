/**
 * @file s3_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=12; TODO=1, Stub=1, Unimpl=0, Mock=9, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

namespace themis {
namespace ingestion {

/**
 * @brief Dedicated S3-compatible Object Storage Source Connector
 *
 * Implements `ISourceConnector` for AWS S3 and any S3-compatible endpoint
 * (MinIO, GCS via S3 interop, DigitalOcean Spaces, etc.).  Key differences
 * from the generic `ObjectStorageConnector`:
 *
 * - **Incremental mode**: checkpoints the last processed object key; on
 *   restart uses `ListObjectsV2` with a `StartAfter` marker to skip
 *   already-ingested objects.
 * - **Flat-file format support**: objects with `.jsonl`, `.csv`, `.parquet`,
 *   `.json`, `.txt`, `.html`, `.xml` extensions are written to a temporary
 *   file and parsed by `FileSystemIngester`'s format readers.
 * - **Concurrent downloads**: up to `max_concurrent_downloads` (default 4)
 *   S3 `GetObject` requests are issued in parallel, maximising throughput
 *   on high-bandwidth connections.
 * - **Per-page listing**: `max_keys_per_list` (default 1 000) controls the
 *   `MaxKeys` parameter of each `ListObjectsV2` call, keeping listing
 *   latency bounded.
 *
 * When `THEMIS_ENABLE_S3` is **not** defined at compile time the connector
 * still compiles and:
 *   - returns `CONNECTOR_NOT_SUPPORTED` for any live cloud call, OR
 *   - uses injected mock functions (unit tests).
 *
 * ### Supported `SourceConfig::options` keys
 *
 * | Key                    | Description                                         | Default          |
 * |------------------------|-----------------------------------------------------|------------------|
 * | `bucket`               | Bucket name (overrides `SourceConfig::location`)    | `location` field |
 * | `prefix`               | Key prefix filter                                   | (none)           |
 * | `region`               | AWS region (e.g. `us-east-1`)                       | `us-east-1`      |
 * | `endpoint_url`         | Custom S3-compatible endpoint URL                   | (AWS default)    |
 * | `access_key`           | AWS access key ID (never logged)                    | (env / IAM role) |
 * | `secret_key`           | AWS secret access key (never logged)                | (env / IAM role) |
 * | `max_keys_per_list`    | `MaxKeys` per `ListObjectsV2` call                  | `1000`           |
 * | `max_concurrent_downloads` | Parallel `GetObject` requests                   | `4`              |
 * | `start_after`          | Lexicographic start marker for first listing page   | (none)           |
 * | `text_field`           | JSON key for document text in `.json` objects       | `text`           |
 * | `path_style`           | Force path-style addressing (MinIO, Ceph)           | `false`          |
 *
 * ### Incremental checkpointing
 *
 * Inject a `CheckpointStore` via `setCheckpointStore()` to enable
 * incremental mode.  The connector stores the last successfully processed
 * object key in `IngestionCheckpoint::cursor`; on the next run it is used
 * as the `StartAfter` parameter of the first `ListObjectsV2` call.
 *
 * ### Example — direct use
 * @code
 * SourceConfig cfg{
 *     .source_id = "data_lake",
 *     .type      = SourceType::OBJECT_STORAGE,
 *     .location  = "my-bucket",
 *     .options   = {{"prefix","data/2026/"},
 *                   {"region","eu-west-1"},
 *                   {"max_keys_per_list","500"},
 *                   {"max_concurrent_downloads","8"}}
 * };
 * S3Connector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 *
 * ### Example — via IngestionBuilder
 * @code
 * auto mgr = IngestionBuilder("mydb")
 *     .withObjectStorageSource("lake", "my-bucket",
 *                              {{"provider","s3"},{"prefix","data/"}})
 *     .build();
 * auto report = mgr->ingestAll();
 * @endcode
 */
class S3Connector : public ISourceConnector {
public:
    S3Connector();
    ~S3Connector() override;

    // Non-copyable
    S3Connector(const S3Connector&) = delete;
    S3Connector& operator=(const S3Connector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration.
     * @param config  `type` must be `SourceType::OBJECT_STORAGE`.
     *                `location` is used as the bucket name unless
     *                `options["bucket"]` is set.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /**
     * @brief Check whether the bucket is reachable.
     *
     * With a mock injected, always returns true.  Without a mock, requires
     * `THEMIS_ENABLE_S3` at build time and valid credentials.
     */
    bool isAvailable() const override;

    /**
     * @brief Always returns 0 — object count requires a full bucket listing.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief List objects and ingest each one as a document.
     *
     * Pagination uses `ListObjectsV2` continuation tokens.  Each listed
     * object is downloaded (up to `max_concurrent_downloads` in parallel)
     * and its content is extracted via `FileSystemIngester`'s format readers
     * (for `.jsonl`, `.csv`, `.parquet`, `.json`, `.txt`, etc.) or treated
     * as raw text for unknown extensions.
     *
     * When `THEMIS_ENABLE_S3` is not defined and no mock is injected, returns
     * immediately with a `CONNECTOR_NOT_SUPPORTED` error.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Inject a `CheckpointStore` for incremental mode.
     *
     * When set, the connector reads the stored cursor on startup (used as
     * `StartAfter`) and writes an updated cursor after each successful run.
     */
    void setCheckpointStore(std::shared_ptr<CheckpointStore> store);

    /**
     * @brief Configure retry behaviour.
     */
    void setRetryConfig(const RetryConfig& config);

    // -------------------------------------------------------------------------
    // Mock injection (unit tests — no real AWS credentials required)
    // -------------------------------------------------------------------------

    /**
     * @brief Object-list mock: returns one page of keys per call.
     *
     * Return an empty vector to signal end-of-listing.
     * The second argument is the `start_after` marker (empty on the first
     * call; set to the last key from the previous page for subsequent calls).
     */
    using ObjectListFn = std::function<
        std::vector<std::string>(const std::string& start_after)>;

    /**
     * @brief Object-fetch mock: returns object body given a key.
     *
     * Return an empty string to simulate a fetch failure.
     */
    using ObjectFetchFn = std::function<std::string(const std::string& key)>;

    /**
     * @brief Inject mock listing/fetch functions (replaces all AWS SDK calls).
     *
     * Pass empty functors to restore the real provider path.
     */
    void setObjectListProvider(ObjectListFn list_fn);
    void setObjectFetchProvider(ObjectFetchFn fetch_fn);
    void setObjectListForTesting(ObjectListFn list_fn);
    void setObjectFetchForTesting(ObjectFetchFn fetch_fn);

    /**
     * @brief Inject a document-write callback for unit testing.
     *
     * When set, every successfully extracted document text is passed to this
     * callback **in addition to** incrementing `documents_processed`.  This
     * allows tests to verify the actual extracted content (e.g. that the
     * `text_field` was correctly extracted from a JSON object) without
     * requiring a live database.
     *
     * The callback receives `(key, extracted_text)`.
     */
    using DocumentWriteFn = std::function<void(const std::string& key,
                                               const std::string& text)>;
    void setDocumentWriteForTesting(DocumentWriteFn fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
