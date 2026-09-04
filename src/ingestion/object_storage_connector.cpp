/**
 * @file object_storage_connector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=3, Unimpl=0, Mock=5, Sim=2, Debt=0, C=0, H=4, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// When THEMIS_ENABLE_S3, THEMIS_ENABLE_GCS, or THEMIS_ENABLE_AZURE are
// defined at compile time the corresponding production SDK path is compiled.
// Without those flags the connector still compiles and:
//   - returns CONNECTOR_NOT_SUPPORTED on any live cloud call, OR
//   - uses injected mock functions (unit tests).

#include "ingestion/object_storage_connector.h"

#ifdef THEMIS_ENABLE_S3
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/auth/AWSCredentials.h>
#endif

#ifdef THEMIS_ENABLE_GCS
#include <google/cloud/storage/client.h>
#endif

#ifdef THEMIS_ENABLE_AZURE
#include <azure/storage/blobs.hpp>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <algorithm>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// JSON helpers (minimal, dependency-free – mirrors kafka_connector.cpp)
// ---------------------------------------------------------------------------

namespace {

/// Extract the first string value for `"key":"<value>"` from a JSON blob.
static std::string objStorageJsonExtractString(const std::string& json,
                                               const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    auto start = json.find(needle);
    if (start == std::string::npos) return {};
    start += needle.size();
    std::string value = {};
    bool escape = false;
    for (size_t i = start; i <static_cast<int>(json.size()); ++i) {
        char c = json[i];
        if (escape) { value += c; escape = false; continue; }
        if (c == '\\') { escape = true; continue; }
        if (c == '"') {
          break;
        }
        value += c;
    }
    return value;
}

/// Determine whether a key refers to a JSON object (ends with .json).
static bool isJsonKey(const std::string& key) {
    if (static_cast<int>(key.size()) < 5) {
      return false;
    }
    std::string suffix = key.substr(static_cast<int>(key.size()) - 5);
    // case-insensitive compare ".json"
    std::transform(suffix.begin(), suffix.end(), suffix.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return suffix == ".json";
}

/// Guard against path-traversal sequences in object keys.
/// Returns true if the key is safe (no ".." components).
static bool isKeySafe(const std::string& key) {
    // Reject any key containing ".." to prevent path traversal when the key
    // is later written to a local temp path or used as a document identifier.
    if (key.find("..") != std::string::npos) {
      return false;
    }
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

/** @brief Pimpl. */
class ObjectStorageConnector::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::OBJECT_STORAGE) {
          return false;
        }
        config_ = config;

        auto opt = [&](const std::string& k, const std::string& def) {
            auto it = config.options.find(k);
            return (it != config.options.end()) ? it->second : def;
        };

        // Provider selection
        std::string prov_str = opt("provider", "s3");
        if (prov_str == "gcs") {
            provider_ = ObjectStorageProvider::GCS;
        } else if (prov_str == "azure") {
            provider_ = ObjectStorageProvider::AZURE;
        } else {
            provider_ = ObjectStorageProvider::S3;
        }

        // Bucket / container name: provider-specific key overrides location.
        if (provider_ == ObjectStorageProvider::AZURE) {
            container_ = opt("container", config.location);
            bucket_    = container_;
        } else {
            bucket_    = opt("bucket", config.location);
            container_ = bucket_;
        }

        prefix_          = opt("prefix",       "");
        region_          = opt("region",        "us-east-1");
        endpoint_url_    = opt("endpoint_url",  "");
        access_key_      = opt("access_key",    "");
        secret_key_      = opt("secret_key",    "");
        project_id_      = opt("project_id",    "");
        sa_json_path_    = opt("service_account_json", "");
        connection_str_  = opt("connection_string",    "");
        sas_token_       = opt("sas_token",     "");
        text_field_      = opt("text_field",    "text");

        try {
            max_keys_ = static_cast<size_t>(std::stoull(opt("max_keys", "0")));
        } catch (...) {
            max_keys_ = 0;
        }

        return !bucket_.empty();
    }

    bool isAvailable() const {
        // When mocks are injected, always report available.
        if (list_fn_ && fetch_fn_) {
          return true;
        }

#ifdef THEMIS_ENABLE_S3
        if (provider_ == ObjectStorageProvider::S3) {
            return checkAvailableS3();
        }
#endif
#ifdef THEMIS_ENABLE_GCS
        if (provider_ == ObjectStorageProvider::GCS) {
            return checkAvailableGCS();
        }
#endif
#ifdef THEMIS_ENABLE_AZURE
        if (provider_ == ObjectStorageProvider::AZURE) {
            return checkAvailableAzure();
        }
#endif
        return false;
    }

    size_t getDocumentCount() const {
        // A full bucket listing is needed to count objects; not performed here.
        return 0;
    }

    IngestionStats ingest(const std::string& /*target_collection*/,
                          ProgressCallback progress_callback) {
        IngestionStats stats;
        auto start_time = std::chrono::steady_clock::now();

        if (bucket_.empty()) {
            stats.addError(IngestionErrorCode::SOURCE_NOT_CONFIGURED,
                           IngestionErrorSeverity::FATAL,
                           "ObjectStorageConnector not configured: bucket/container is empty",
                           config_.source_id);
            finaliseStats(stats, start_time);
            return stats;
        }

        // -------------------------------------------------------------------
        // Test mock path: no cloud SDK required
        // -------------------------------------------------------------------
        if (list_fn_ && fetch_fn_) {
            ingestFromMock(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }

        // -------------------------------------------------------------------
        // Production paths
        // -------------------------------------------------------------------
#ifdef THEMIS_ENABLE_S3
        if (provider_ == ObjectStorageProvider::S3) {
            ingestFromS3(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }
#endif
#ifdef THEMIS_ENABLE_GCS
        if (provider_ == ObjectStorageProvider::GCS) {
            ingestFromGCS(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }
#endif
#ifdef THEMIS_ENABLE_AZURE
        if (provider_ == ObjectStorageProvider::AZURE) {
            ingestFromAzure(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }
#endif

        // No SDK compiled for the requested provider
        std::string provider_name = {};
        switch (provider_) {
            case ObjectStorageProvider::GCS:   provider_name = "GCS (THEMIS_ENABLE_GCS)"; break;
            case ObjectStorageProvider::AZURE: provider_name = "Azure (THEMIS_ENABLE_AZURE)"; break;
            default:                           provider_name = "S3 (THEMIS_ENABLE_S3)"; break;
        }
        stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                       IngestionErrorSeverity::FATAL,
                       "ObjectStorageConnector requires " + provider_name +
                       " at build time",
                       config_.source_id);
        finaliseStats(stats, start_time);
        return stats;
    }

    void setRetryConfig(const RetryConfig& c)       { retry_config_ = c; }
    void setObjectListForTesting(ObjectListFn fn)    { list_fn_  = std::move(fn); }
    void setObjectFetchForTesting(ObjectFetchFn fn)  { fetch_fn_ = std::move(fn); }

private:
    // -----------------------------------------------------------------------
    // Text extraction
    // -----------------------------------------------------------------------
    std::string extractText(const std::string& key,
                            const std::string& body) const {
        if (isJsonKey(key)) {
            std::string text = objStorageJsonExtractString(body, text_field_);
            if (!text.empty()) {
              return text;
            }
            // Fall through: treat whole body as text if field absent.
        }
        return body;
    }

    // -----------------------------------------------------------------------
    // STUB/SIMULATION NOTE:
    // Purpose: Enable unit-testing of ObjectStorageConnector without a live
    //   object-storage endpoint by using injected list_fn_/fetch_fn_ lambdas.
    // Activation: Active when list_fn_ is non-null (set via
    //   ObjectStorageConnector::setListFnForTesting()).
    // Production Delta: Object keys and content come from injected lambdas
    //   instead of real HTTP/S3/GCS calls.  No authentication, no retries,
    //   no bandwidth throttling.
    // Roadmap ref: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
    // Removal Plan: Not removed — remains the test-injection path.
    // Roadmap ref: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
    // -----------------------------------------------------------------------
    void ingestFromMock(IngestionStats& stats,
                        ProgressCallback& progress_callback) {
        size_t processed = 0;
        try {
            while (true) {
                if (max_keys_ > 0 && processed >= max_keys_) {
                  break;
                }

                auto keys = list_fn_();
                if (keys.empty()) {
                  break;
                }

                for (const auto& key : keys) {
                    if (max_keys_ > 0 && processed >= max_keys_) {
                      break;
                    }

                    if (!isKeySafe(key)) {
                        stats.addError(IngestionErrorCode::FILE_READ_ERROR,
                                       IngestionErrorSeverity::WARNING,
                                       "Rejected unsafe key: " + key,
                                       config_.source_id);
                        ++stats.documents_failed;
                        ++processed;
                        continue;
                    }

                    std::string body = fetch_fn_(key);
                    if (body.empty()) {
                        stats.addError(IngestionErrorCode::FILE_READ_ERROR,
                                       IngestionErrorSeverity::WARNING,
                                       "Empty or fetch-failed object: " + key,
                                       config_.source_id);
                        ++stats.documents_failed;
                        ++processed;
                        continue;
                    }

                    std::string text = extractText(key, body);
                    if (!text.empty()) {
                        ++stats.documents_processed;
                    }
                    stats.bytes_processed += body.size();
                    ++processed;
                }

                if ([[maybe_unused]] progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0, // total unknown
                                      "processed " + std::to_string(processed) +
                                      " objects");
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in ObjectStorage mock ingest: " +
                           std::string(e.what()),
                           config_.source_id);
        }
    }

#ifdef THEMIS_ENABLE_S3
    // -----------------------------------------------------------------------
    // AWS S3 production path
    // -----------------------------------------------------------------------
    bool checkAvailableS3() const {
        try {
            Aws::Client::ClientConfiguration client_cfg;
            client_cfg.region = region_;
            if (!endpoint_url_.empty()) {
                client_cfg.endpointOverride = endpoint_url_;
            }

            std::unique_ptr<Aws::S3::S3Client> s3 = {};

            if (!access_key_.empty() && !secret_key_.empty()) {
                Aws::Auth::AWSCredentials creds(access_key_, secret_key_);
                s3 = std::make_unique<Aws::S3::S3Client>(creds, client_cfg);
            } else {
                s3 = std::make_unique<Aws::S3::S3Client>(client_cfg);
            }

            Aws::S3::Model::ListObjectsV2Request req;
            req.SetBucket(bucket_);
            req.SetMaxKeys(1);
            if (!prefix_.empty()) {
              req.SetPrefix(prefix_);
            }

            auto outcome = s3->ListObjectsV2(req);
            return outcome.IsSuccess();
        } catch (...) {
            return false;
        }
    }

    void ingestFromS3(IngestionStats& stats,
                      ProgressCallback& progress_callback) {
        Aws::Client::ClientConfiguration client_cfg;
        client_cfg.region = region_;
        if (!endpoint_url_.empty()) {
            client_cfg.endpointOverride = endpoint_url_;
        }

        std::unique_ptr<Aws::S3::S3Client> s3 = {};

        if (!access_key_.empty() && !secret_key_.empty()) {
            Aws::Auth::AWSCredentials creds(access_key_, secret_key_);
            s3 = std::make_unique<Aws::S3::S3Client>(creds, client_cfg);
        } else {
            s3 = std::make_unique<Aws::S3::S3Client>(client_cfg);
        }

        std::string continuation_token = {};
        size_t processed = 0;

        try {
            do {
                if (max_keys_ > 0 && processed >= max_keys_) {
                  break;
                }

                Aws::S3::Model::ListObjectsV2Request list_req;
                list_req.SetBucket(bucket_);
                if (!prefix_.empty()) {
                  list_req.SetPrefix(prefix_);
                }
                list_req.SetMaxKeys(1000);
                if (!continuation_token.empty()) {
                    list_req.SetContinuationToken(continuation_token);
                }

                auto list_outcome = s3->ListObjectsV2(list_req);
                if (!list_outcome.IsSuccess()) {
                    stats.addError(IngestionErrorCode::HTTP_REQUEST_FAILED,
                                   IngestionErrorSeverity::FATAL,
                                   "S3 ListObjectsV2 failed: " +
                                   list_outcome.GetError().GetMessage(),
                                   config_.source_id);
                    return;
                }

                const auto& result = list_outcome.GetResult();
                for (const auto& obj : result.GetContents()) {
                    if (max_keys_ > 0 && processed >= max_keys_) {
                      break;
                    }

                    const std::string key = obj.GetKey();
                    if (!isKeySafe(key)) {
                        stats.addError(IngestionErrorCode::FILE_READ_ERROR,
                                       IngestionErrorSeverity::WARNING,
                                       "Rejected unsafe key: " + key,
                                       config_.source_id);
                        ++stats.documents_failed;
                        ++processed;
                        continue;
                    }

                    Aws::S3::Model::GetObjectRequest get_req;
                    get_req.SetBucket(bucket_);
                    get_req.SetKey(key);

                    auto get_outcome = s3->GetObject(get_req);
                    if (!get_outcome.IsSuccess()) {
                        stats.addError(IngestionErrorCode::HTTP_REQUEST_FAILED,
                                       IngestionErrorSeverity::WARNING,
                                       "S3 GetObject failed for '" + key + "': " +
                                       get_outcome.GetError().GetMessage(),
                                       config_.source_id);
                        ++stats.documents_failed;
                        ++processed;
                        continue;
                    }

                    std::ostringstream oss = {};
                    oss << get_outcome.GetResult().GetBody().rdbuf();
                    std::string body = oss.str();

                    std::string text = extractText(key, body);
                    if (!text.empty()) {
                        ++stats.documents_processed;
                    }
                    stats.bytes_processed += body.size();
                    ++processed;

                    if ([[maybe_unused]] progress_callback) {
                        progress_callback(config_.source_id,
                                          stats.documents_processed,
                                          0,
                                          "processed " + std::to_string(processed) +
                                          " objects");
                    }
                }

                if (result.GetIsTruncated()) {
                    continuation_token = result.GetNextContinuationToken();
                } else {
                    break;
                }
            } while (true);
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in S3 ingest: " + std::string(e.what()),
                           config_.source_id);
        }
    }
#endif // THEMIS_ENABLE_S3

#ifdef THEMIS_ENABLE_GCS
    // -----------------------------------------------------------------------
    // Google Cloud Storage production path
    // -----------------------------------------------------------------------
    bool checkAvailableGCS() const {
        try {
            namespace gcs = google::cloud::storage;
            auto client = gcs::Client();
            auto it = client.ListObjects(bucket_, gcs::MaxResults(1),
                                         gcs::Prefix(prefix_));
            // If the iterator doesn't throw, the bucket is reachable.
            (void)it.begin();
            return true;
        } catch (...) {
            return false;
        }
    }

    void ingestFromGCS(IngestionStats& stats,
                       ProgressCallback& progress_callback) {
        try {
            namespace gcs = google::cloud::storage;
            auto client = gcs::Client();

            size_t processed = 0;
            for (auto& obj_meta : client.ListObjects(bucket_,
                                                     gcs::Prefix(prefix_))) {
                if (max_keys_ > 0 && processed >= max_keys_) {
                  break;
                }
                if (!obj_meta) {
                    stats.addError(IngestionErrorCode::HTTP_REQUEST_FAILED,
                                   IngestionErrorSeverity::WARNING,
                                   "GCS ListObjects error: " +
                                   obj_meta.status().message(),
                                   config_.source_id);
                    continue;
                }

                const std::string key = obj_meta->name();
                if (!isKeySafe(key)) {
                    stats.addError(IngestionErrorCode::FILE_READ_ERROR,
                                   IngestionErrorSeverity::WARNING,
                                   "Rejected unsafe key: " + key,
                                   config_.source_id);
                    ++stats.documents_failed;
                    ++processed;
                    continue;
                }

                auto reader = client.ReadObject(bucket_, key);
                if (!reader) {
                    stats.addError(IngestionErrorCode::HTTP_REQUEST_FAILED,
                                   IngestionErrorSeverity::WARNING,
                                   "GCS ReadObject failed for '" + key + "': " +
                                   reader.status().message(),
                                   config_.source_id);
                    ++stats.documents_failed;
                    ++processed;
                    continue;
                }

                std::string body(std::istreambuf_iterator<char>(reader), {});
                std::string text = extractText(key, body);
                if (!text.empty()) {
                    ++stats.documents_processed;
                }
                stats.bytes_processed += body.size();
                ++processed;

                if ([[maybe_unused]] progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0,
                                      "processed " + std::to_string(processed) +
                                      " objects");
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in GCS ingest: " + std::string(e.what()),
                           config_.source_id);
        }
    }
#endif // THEMIS_ENABLE_GCS

#ifdef THEMIS_ENABLE_AZURE
    // -----------------------------------------------------------------------
    // Azure Blob Storage production path
    // -----------------------------------------------------------------------
    bool checkAvailableAzure() const {
        try {
            using namespace Azure::Storage::Blobs;
            auto container_client = BlobContainerClient::CreateFromConnectionString(
                connection_str_, container_);
            auto props = container_client.GetProperties();
            return props.Value.ETag.HasValue();
        } catch (...) {
            return false;
        }
    }

    void ingestFromAzure(IngestionStats& stats,
                         ProgressCallback& progress_callback) {
        try {
            using namespace Azure::Storage::Blobs;
            auto container_client = BlobContainerClient::CreateFromConnectionString(
                connection_str_, container_);

            ListBlobsOptions list_opts = {};
            if (!prefix_.empty()) {
                list_opts.Prefix = prefix_;
            }

            size_t processed = 0;
            for (auto page = container_client.ListBlobs(list_opts);
                 page.HasPage(); page.MoveToNextPage()) {
                if (max_keys_ > 0 && processed >= max_keys_) {
                  break;
                }

                for (const auto& blob_item : page.Blobs) {
                    if (max_keys_ > 0 && processed >= max_keys_) {
                      break;
                    }

                    const std::string key = blob_item.Name;
                    if (!isKeySafe(key)) {
                        stats.addError(IngestionErrorCode::FILE_READ_ERROR,
                                       IngestionErrorSeverity::WARNING,
                                       "Rejected unsafe key: " + key,
                                       config_.source_id);
                        ++stats.documents_failed;
                        ++processed;
                        continue;
                    }

                    auto blob_client = container_client.GetBlockBlobClient(key);
                    Azure::Storage::Blobs::DownloadBlobOptions dl_opts;
                    auto dl = blob_client.Download(dl_opts);

                    std::string body = {};
                    auto& stream = *dl.Value.BodyStream;
                    std::vector<uint8_t> buf(4096);
                    size_t n = 0;
                    while ((n = stream.Read(buf.data(),static_cast<int>(buf.size()))) > 0) {
                        body.append(reinterpret_cast<char*>(buf.data()), n);
                    }

                    std::string text = extractText(key, body);
                    if (!text.empty()) {
                        ++stats.documents_processed;
                    }
                    stats.bytes_processed += body.size();
                    ++processed;

                    if ([[maybe_unused]] progress_callback) {
                        progress_callback(config_.source_id,
                                          stats.documents_processed,
                                          0,
                                          "processed " + std::to_string(processed) +
                                          " objects");
                    }
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in Azure ingest: " + std::string(e.what()),
                           config_.source_id);
        }
    }
#endif // THEMIS_ENABLE_AZURE

    void finaliseStats(IngestionStats& stats,
                       const std::chrono::steady_clock::time_point& start) const {
        auto end = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end - start).count();
        if (stats.elapsed_seconds > 0.0 && stats.documents_processed > 0) {
            stats.metrics.throughput_docs_per_sec =
                static_cast<double>(stats.documents_processed) /
                stats.elapsed_seconds;
        }
    }

    // Configuration
    SourceConfig            config_;
    ObjectStorageProvider   provider_       = ObjectStorageProvider::S3;
    std::string             bucket_;
    std::string             container_;
    std::string             prefix_;
    std::string             region_;
    std::string             endpoint_url_;
    std::string             access_key_;
    std::string             secret_key_;
    std::string             project_id_;
    std::string             sa_json_path_;
    std::string             connection_str_;
    std::string             sas_token_;
    std::string             text_field_;
    size_t                  max_keys_       = 0;
    RetryConfig             retry_config_;

    // Testing hooks
    ObjectListFn  list_fn_;
    ObjectFetchFn fetch_fn_;
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ObjectStorageConnector::ObjectStorageConnector()
    : impl_(std::make_unique<Impl>()) {}

ObjectStorageConnector::~ObjectStorageConnector() = default;

bool ObjectStorageConnector::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool ObjectStorageConnector::isAvailable() const {
    return impl_->isAvailable();
}

size_t ObjectStorageConnector::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats ObjectStorageConnector::ingest(
        const std::string& target_collection,
        ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, progress_callback);
}

void ObjectStorageConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void ObjectStorageConnector::setObjectListForTesting(ObjectListFn fn) {
    setObjectListProvider(std::move(fn));
}

void ObjectStorageConnector::setObjectListProvider(ObjectListFn fn) {
    impl_->setObjectListForTesting(std::move(fn));
}

void ObjectStorageConnector::setObjectFetchForTesting(ObjectFetchFn fn) {
    setObjectFetchProvider(std::move(fn));
}

void ObjectStorageConnector::setObjectFetchProvider(ObjectFetchFn fn) {
    impl_->setObjectFetchForTesting(std::move(fn));
}

} // namespace ingestion
} // namespace themis


