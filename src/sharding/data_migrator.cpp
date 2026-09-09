/**
 * @file data_migrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=4, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/data_migrator.h"
#include "sharding/mtls_client.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/wal_shipper.h"
#include "sharding/shard_topology.h"
#include <stdexcept>
#include <thread>
#include <filesystem>
#include <fstream>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <limits>

namespace themis {
namespace sharding {

/**
 * @brief Erstellt einen mTLS-Client aus DataMigratorConfig.
 * @param config DataMigrator-Konfiguration mit Zertifikatspfaden und Retrywerten.
 * @return Initialisierter mTLS-Client.
 */
static std::unique_ptr<themis::sharding::MTLSClient> createMTLSClient(const DataMigratorConfig& config) {
    themis::sharding::MTLSClient::Config mtls_config;
    mtls_config.cert_path = config.cert_path;
    mtls_config.key_path = config.key_path;
    mtls_config.ca_cert_path = config.ca_cert_path;
    mtls_config.tls_version = "TLSv1.3";
    mtls_config.verify_peer = true;
    mtls_config.verify_hostname = true;
    mtls_config.connect_timeout_ms = 5000;
    mtls_config.request_timeout_ms = 30000;
    mtls_config.max_retries = config.max_retries;
    mtls_config.retry_delay_ms = config.retry_delay_ms;
    
    return std::make_unique<themis::sharding::MTLSClient>(mtls_config);
}

DataMigrator::DataMigrator(
    const DataMigratorConfig& config,
    std::shared_ptr<PrometheusMetrics> metrics)
    : config_(config),
      metrics_(metrics) {
    
    if (config_.source_endpoint.empty() || config_.target_endpoint.empty()) {
        throw std::invalid_argument("Source and target endpoints must not be empty");
    }
    
    if (config_.batch_size == 0) {
        throw std::invalid_argument("Batch size must be greater than 0");
    }
    
    // Load idempotency state if enabled
    if (config_.enable_idempotency) {
        loadIdempotencyState();
    }
}

/**
 * @brief Migriert einen Token-Bereich in Batches vom Quell- zum Ziel-Shard.
 *
 * Fuehrt Batch-Fetch, optionale Integritaetspruefung und idempotentes
 * Abschluss-Tracking aus. Bei aktivierter Idempotenz werden bereits
 * abgeschlossene Migrationen bzw. Batches uebersprungen.
 */
MigrationResult DataMigrator::migrate(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end,
    ProgressCallback progress_callback
) {
    MigrationResult result;
    MigrationProgress progress;
    
    // Generate deterministic migration ID
    std::string migration_id = generateMigrationId(
        source_shard_id, target_shard_id,
        token_range_start, token_range_end
    );
    result.migration_id = migration_id;
    progress.migration_id = migration_id;
    
    // Check if already completed (idempotency)
    if (config_.enable_idempotency && isMigrationCompleted(migration_id)) {
        result.success = true;
        result.was_already_completed = true;
        return result;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    std::string operation_id = source_shard_id + "_to_" + target_shard_id;
    
    try {
        // Query the source shard for the actual record count in the token range
        // so that progress percentages are accurate.  Fall back to a conservative
        // 10 000 estimate if the endpoint is not available or returns a bad response.
        {
            auto count_client = createMTLSClient(config_);
            std::ostringstream count_path = {};
            count_path << "/api/v1/data/migrate/count"
                       << "?token_range_start=" << token_range_start
                       << "&token_range_end=" << token_range_end
                       << "&shard_id=" << source_shard_id;
            uint64_t shard_count = 10000;
            if (count_client->isReady()) {
                auto resp = count_client->get(config_.source_endpoint, count_path.str());
                if (resp.success && resp.body.is_object() &&
                    resp.body.contains("count") && resp.body["count"].is_number()) {
                    shard_count = resp.body["count"].get<uint64_t>();
                }
            }
            progress.total_records = static_cast<uint32_t>(
                std::min<uint64_t>(shard_count, std::numeric_limits<uint32_t>::max()));
        }
        
        uint32_t offset = 0;
        uint32_t batch_index = 0;
        uint32_t records_in_batch = config_.batch_size;
        
        while (records_in_batch == config_.batch_size) {
            // Generate deterministic batch ID
            std::string batch_id = generateBatchId(migration_id, batch_index);
            
            // Skip already completed batches (idempotency)
            if (config_.enable_idempotency && isBatchCompleted(batch_id)) {
                offset += config_.batch_size;
                batch_index++;
                continue;
            }
            
            // Fetch batch from source
            auto batch = fetchBatch(source_shard_id, token_range_start, 
                                  token_range_end, offset, config_.batch_size);
            
            records_in_batch = batch.is_array() ? static_cast<uint32_t>(batch.size()) : 0;
            
            if (records_in_batch == 0) {
                break;
            }
            
            // Calculate hash if verification enabled
            std::string batch_hash = {};
            if (config_.verify_integrity) {
                batch_hash = calculateHash(batch);
            }
            
            // Write batch to target (atomic operation)
            if (!writeBatch(target_shard_id, batch)) {
                result.errors.push_back("Failed to write batch at offset " + 
                                      std::to_string(offset));
                progress.errors++;
                
                if (progress.errors > config_.max_retries) {
                    result.error_message = "Too many errors during migration";
                    return result;
                }
                // Do not mark batch as completed on failure - allows retry
                offset += config_.batch_size;
                batch_index++;
                continue;
            }
            
            // Mark batch as completed (idempotency tracking)
            if (config_.enable_idempotency) {
                markBatchCompleted(batch_id);
            }
            
            // Update progress
            progress.records_migrated += records_in_batch;
            progress.bytes_transferred += batch.dump().size();
            progress.progress_percent = 
                (static_cast<double>(progress.records_migrated) / progress.total_records) * 100.0;
            
            // Record metrics
            if (metrics_) {
                metrics_->recordMigrationProgress(
                    operation_id,
                    progress.records_migrated,
                    progress.bytes_transferred,
                    progress.progress_percent
                );
            }
            
            if (progress_callback) {
                progress_callback(progress);
            }
            
            offset += records_in_batch;
            result.records_migrated = progress.records_migrated;
            result.bytes_transferred = progress.bytes_transferred;
        }
        
        // Verify integrity if enabled
        if (config_.verify_integrity) {
            if (!verifyIntegrity(source_shard_id, target_shard_id, 
                               token_range_start, token_range_end)) {
                result.error_message = "Integrity verification failed";
                return result;
            }
        }
        
        result.success = true;
        
        // Mark migration as completed (idempotency)
        if (config_.enable_idempotency) {
            markMigrationCompleted(migration_id);
        }
        
        // Record final metrics
        auto end_time = std::chrono::steady_clock::now();
        auto duration_sec = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time).count();
        
        if (metrics_) {
            metrics_->recordMigrationDuration(operation_id, static_cast<double>(duration_sec));
        }
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Migration failed: ") + e.what();
        result.success = false;
    }
    
    return result;
}

/**
 * @brief Verifiziert die Integritaet zwischen Quell- und Ziel-Shard.
 * @return true, wenn die Hashes der geladenen Datensaetze identisch sind.
 */
bool DataMigrator::verifyIntegrity(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end
) {
    try {
        // Fetch data from both shards
        auto source_data = fetchBatch(source_shard_id, token_range_start, 
                                     token_range_end, 0, 10000);
        auto target_data = fetchBatch(target_shard_id, token_range_start, 
                                     token_range_end, 0, 10000);
        
        // Calculate hashes
        std::string source_hash = calculateHash(source_data);
        std::string target_hash = calculateHash(target_data);
        
        return source_hash == target_hash;
        
    } catch (...) {
        return false;
    }
}

/**
 * @brief Holt eine paginierte Batch aus dem Quell-Shard per mTLS.
 * @return Records-Array oder leeres Array bei Netzwerk-/Formatfehlern.
 */
nlohmann::json DataMigrator::fetchBatch(
    const std::string& source_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end,
    uint32_t offset,
    uint32_t limit
) {
    // Create mTLS client for secure shard-to-shard communication
    auto mtls_client = createMTLSClient(config_);
    
    if (!mtls_client->isReady()) {
        std::cerr << "DataMigrator: mTLS client not ready (missing certificates?)" << std::endl;
        return nlohmann::json::array();
    }
    
    // Build the query endpoint for fetching data in the specified token range
    std::ostringstream path_oss = {};
    path_oss << "/api/v1/data/migrate/fetch"
             << "?token_range_start=" << token_range_start
             << "&token_range_end=" << token_range_end
             << "&offset=" << offset
             << "&limit=" << limit
             << "&shard_id=" << source_shard_id;
    
    // Execute the GET request via mTLS
    auto response = mtls_client->get(config_.source_endpoint, path_oss.str());
    
    if (!response.success) {
        std::cerr << "DataMigrator::fetchBatch: Failed to fetch from source shard " 
                  << source_shard_id << " - " << response.error << std::endl;
        return nlohmann::json::array();
    }
    
    // Validate response structure
    if (!response.body.is_object()) {
        std::cerr << "DataMigrator::fetchBatch: Invalid response format (expected object)" << std::endl;
        return nlohmann::json::array();
    }
    
    // Extract the records array from the response
    if (response.body.contains("records") && response.body["records"].is_array()) {
        return response.body["records"];
    }
    
    // Legacy format: response body itself might be the array
    if (response.body.is_array()) {
        return response.body;
    }
    
    std::cerr << "DataMigrator::fetchBatch: Response did not contain 'records' array" << std::endl;
    return nlohmann::json::array();
}

/**
 * @brief Schreibt eine Batch per mTLS auf den Ziel-Shard.
 * @return true bei erfolgreicher Zielbestaetigung oder HTTP-2xx.
 */
bool DataMigrator::writeBatch(
    const std::string& target_shard_id,
    const nlohmann::json& batch
) {
    if (!batch.is_array() || batch.empty()) {
        return false;
    }
    
    // Create mTLS client for secure shard-to-shard communication
    auto mtls_client = createMTLSClient(config_);
    
    if (!mtls_client->isReady()) {
        std::cerr << "DataMigrator: mTLS client not ready (missing certificates?)" << std::endl;
        return false;
    }
    
    // Build the request body for the batch write operation
    nlohmann::json request_body;
    request_body["shard_id"] = target_shard_id;
    request_body["records"] = batch;
    request_body["operation"] = "migration_write";
    
    // Retry the write operation with exponential backoff
    return retryOperation([&]() {
        // Build the POST endpoint for migration writes
        std::string path = "/api/v1/data/migrate/write";
        
        // Execute the POST request via mTLS
        auto response = mtls_client->post(config_.target_endpoint, path, request_body);
        
        if (!response.success) {
            std::cerr << "DataMigrator::writeBatch: Failed to write to target shard " 
                      << target_shard_id << " - " << response.error 
                      << " (HTTP " << response.status_code << ")" << std::endl;
            return false;
        }
        
        // Check response for success confirmation
        if (response.body.is_object() && response.body.contains("success")) {
            return response.body["success"].get<bool>();
        }
        
        // HTTP 2xx status codes indicate success
        return response.status_code >= 200 && response.status_code < 300;
    });
}

/**
 * @brief Berechnet SHA-256 ueber die serialisierte JSON-Repraesentation.
 * @param data Eingabedaten.
 * @return Hex-codierter Hashwert.
 */
std::string DataMigrator::calculateHash(const nlohmann::json& data) {
    // Calculate SHA-256 hash of JSON data
    std::string data_str = data.dump();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data_str.c_str()), 
           data_str.size(), hash);
    
    std::stringstream ss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

template<typename Func>
/**
 * @brief Fuehrt eine boolesche Operation mit Retry und linearem Backoff aus.
 * @tparam Func Callable mit Rueckgabetyp bool.
 * @param func Aufzurufende Operation.
 * @return true bei Erfolg eines Versuchs, sonst false.
 */
bool DataMigrator::retryOperation(Func func) {
    for (uint32_t attempt = 0; attempt < config_.max_retries; ++attempt) {
        try {
            if (func()) {
                return true;
            }
        } catch (...) {
            // Continue to retry
        }
        
        if (attempt < config_.max_retries - 1) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.retry_delay_ms * (attempt + 1))
            );
        }
    }
    
    return false;
}


// ============================================================================
// Idempotency Helper Methods
// ============================================================================

std::string DataMigrator::generateMigrationId(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end
) {
    // Create deterministic string for hashing
    std::ostringstream oss = {};
    oss << source_shard_id << ":"
        << target_shard_id << ":"
        << token_range_start << ":"
        << token_range_end;
    
    // Calculate SHA256 hash
    std::string input = oss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
           input.size(), hash);
    
    // Convert to hex string
    std::ostringstream hex_oss = {};
    hex_oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex_oss << std::setw(2) << static_cast<unsigned>(hash[i]);
    }
    
    return "migration_" + hex_oss.str();
}

/**
 * @brief Bildet eine eindeutige Batch-ID innerhalb einer Migration.
 */
std::string DataMigrator::generateBatchId(
    const std::string& migration_id,
    uint32_t batch_index
) {
    return migration_id + "_batch_" + std::to_string(batch_index);
}

/** @brief Prueft threadsicher, ob eine Migration bereits final markiert wurde. */
bool DataMigrator::isMigrationCompleted(const std::string& migration_id) {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    return completed_migrations_.find(migration_id) != completed_migrations_.end();
}

/** @brief Markiert Migration als abgeschlossen und persistiert den Zustand. */
void DataMigrator::markMigrationCompleted(const std::string& migration_id) {
    {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        completed_migrations_.insert(migration_id);
    }
    // Persist outside the lock to avoid blocking other threads during file I/O.
    saveIdempotencyState();
}

/** @brief Prueft threadsicher, ob eine Batch bereits verarbeitet wurde. */
bool DataMigrator::isBatchCompleted(const std::string& batch_id) {
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    return completed_batches_.find(batch_id) != completed_batches_.end();
}

/** @brief Markiert Batch als abgeschlossen und persistiert periodisch den Zustand. */
void DataMigrator::markBatchCompleted(const std::string& batch_id) {
    bool should_persist = false;
    {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        completed_batches_.insert(batch_id);
        // Persist after every N batches to avoid too frequent I/O (thread-safe with atomic)
        should_persist = (batch_counter_.fetch_add(1, std::memory_order_relaxed) % 10 == 0);
    }
    // Persist outside the lock to avoid blocking other threads during file I/O.
    if (should_persist) {
        saveIdempotencyState();
    }
}

/**
 * @brief Laedt den Idempotenzstatus aus Dateien in den Arbeitsspeicher.
 *
 * Datei-I/O erfolgt ausserhalb des Locks; das Uebernehmen in die geteilten
 * Datenstrukturen erfolgt anschliessend unter Mutexschutz.
 */
void DataMigrator::loadIdempotencyState() {
    // Read from disk first (no lock needed during I/O), then populate shared state.
    std::unordered_set<std::string> loaded_migrations;
    std::unordered_set<std::string> loaded_batches;

    try {
        namespace fs = std::filesystem;
        fs::path state_dir(config_.idempotency_store_path);
        
        if (!fs::exists(state_dir)) {
            fs::create_directories(state_dir);
        } else {
            // Load completed migrations
            fs::path migrations_file = state_dir / "completed_migrations.json";
            if (fs::exists(migrations_file)) {
                std::ifstream ifs(migrations_file);
                nlohmann::json j;
                ifs >> j;
                
                if (j.is_array()) {
                    for (const auto& item : j) {
                        if (item.is_string()) {
                            loaded_migrations.insert(item.get<std::string>());
                        }
                    }
                }
            }
            
            // Load completed batches
            fs::path batches_file = state_dir / "completed_batches.json";
            if (fs::exists(batches_file)) {
                std::ifstream ifs(batches_file);
                nlohmann::json j;
                ifs >> j;
                
                if (j.is_array()) {
                    for (const auto& item : j) {
                        if (item.is_string()) {
                            loaded_batches.insert(item.get<std::string>());
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "DataMigrator: Failed to load idempotency state: " 
                  << e.what() << std::endl;
    }

    // Populate shared state under lock after I/O is complete.
    std::lock_guard<std::mutex> lock(idempotency_mutex_);
    completed_migrations_ = std::move(loaded_migrations);
    completed_batches_ = std::move(loaded_batches);
}

/**
 * @brief Persistiert den aktuellen Idempotenzstatus atomar pro Datei.
 *
 * Die zu schreibenden Snapshots werden unter Lock erstellt; der eigentliche
 * Dateizugriff erfolgt ausserhalb des kritischen Abschnitts.
 */
void DataMigrator::saveIdempotencyState() {
    // Snapshot shared state under lock, then write to disk outside the lock so
    // that file I/O does not block concurrent migration threads.
    nlohmann::json migrations_json = nlohmann::json::array();
    nlohmann::json batches_json = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        for (const auto& migration_id : completed_migrations_) {
            migrations_json.push_back(migration_id);
        }
        for (const auto& batch_id : completed_batches_) {
            batches_json.push_back(batch_id);
        }
    }

    try {
        namespace fs = std::filesystem;
        fs::path state_dir(config_.idempotency_store_path);
        
        if (!fs::exists(state_dir)) {
            fs::create_directories(state_dir);
        }
        
        fs::path migrations_file = state_dir / "completed_migrations.json";
        std::ofstream ofs_migrations(migrations_file);
        ofs_migrations << migrations_json.dump(2);
        
        fs::path batches_file = state_dir / "completed_batches.json";
        std::ofstream ofs_batches(batches_file);
        ofs_batches << batches_json.dump(2);
        
    } catch (const std::exception& e) {
        std::cerr << "DataMigrator: Failed to save idempotency state: " 
                  << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Live migration (dual-write protocol)
// ─────────────────────────────────────────────────────────────────────────────

LiveMigrationResult DataMigrator::liveMigrate(
    const std::string& source_shard_id,
    const std::string& target_shard_id,
    uint64_t token_range_start,
    uint64_t token_range_end,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<WALShipper> wal_shipper,
    const LiveMigrationConfig& live_cfg,
    ProgressCallback progress_callback
) {
    LiveMigrationResult result;
    result.migration_id = generateMigrationId(
        source_shard_id, target_shard_id, token_range_start, token_range_end
    );

    // ── Phase 1: Bulk copy ────────────────────────────────────────────────────
    // The source shard continues accepting writes throughout this phase
    // (dual-write semantics: writes go to source; once WAL shipper is
    //  registered they are also forwarded to target).
    result.bulk_migration = migrate(
        source_shard_id, target_shard_id,
        token_range_start, token_range_end,
        progress_callback
    );

    if (!result.bulk_migration.success) {
        result.error_message = "Bulk copy failed: " + result.bulk_migration.error_message;
        return result;
    }

    // ── Phase 2: Integrity verification (optional) ───────────────────────────
    if (live_cfg.verify_after_bulk_copy && config_.verify_integrity) {
        const bool integrity_ok = verifyIntegrity(
            source_shard_id, target_shard_id,
            token_range_start, token_range_end
        );
        if (!integrity_ok) {
            result.error_message = "Integrity check failed after bulk copy; migration aborted";
            return result;
        }
    }

    // ── Phase 3: WAL catch-up ─────────────────────────────────────────────────
    // Register the target shard as a replica in the WAL shipper so it receives
    // incremental WAL entries for the migrated token range while the source
    // shard is still the authoritative owner.
    if (wal_shipper) {
        // Derive a deterministic replica endpoint for the target shard.
        // In a real deployment this would come from ShardTopology; we use the
        // configured target_endpoint as a fallback.
        const std::string replica_endpoint = config_.target_endpoint;
        wal_shipper->addReplica(target_shard_id, replica_endpoint);

        // Poll until WAL lag drops below the configured threshold or we time out.
        const auto deadline = std::chrono::steady_clock::now() + live_cfg.catchup_timeout;
        uint64_t lag_bytes  = UINT64_MAX;

        while (std::chrono::steady_clock::now() < deadline) {
            for (const auto& replica : wal_shipper->getReplicaInfo()) {
                if (replica.replica_id == target_shard_id) {
                    lag_bytes = replica.lag_bytes;
                    result.wal_entries_applied =
                        wal_shipper->getStatistics().total_entries_shipped;
                    break;
                }
            }

            if (lag_bytes <= live_cfg.max_wal_lag_bytes) {
                break;
            }

            std::this_thread::sleep_for(live_cfg.catchup_poll_interval);
        }

        result.final_wal_lag_bytes = lag_bytes;

        if (lag_bytes > live_cfg.max_wal_lag_bytes) {
            // WAL catchup timed out – remove replica registration and fail
            wal_shipper->removeReplica(target_shard_id);
            result.error_message =
                "WAL catchup timed out; final lag " +
                std::to_string(lag_bytes) + " bytes exceeds threshold " +
                std::to_string(live_cfg.max_wal_lag_bytes) + " bytes";
            return result;
        }
    }

    // ── Phase 4: Atomic cutover via ShardTopology ─────────────────────────────
    // Update the topology so the target shard becomes the authoritative owner
    // of the token range.  This is the only moment of topology change; reads
    // that were in-flight to the source shard complete before the topology
    // change is visible (topology is protected by its internal mutex).
    if (topology) {
        // Fetch the source shard info and update its token range to exclude the
        // migrated portion.  The target shard's token range is updated to cover it.
        auto source_info_opt = topology->getShard(source_shard_id);
        auto target_info_opt = topology->getShard(target_shard_id);

        if (source_info_opt && target_info_opt) {
            ShardInfo updated_source = *source_info_opt;
            ShardInfo updated_target = *target_info_opt;

            // Shrink the source shard's range to the tokens below the split point.
            // When token_range_start is 0 the entire range is migrated, so the
            // source ends up with an empty range (token_start == token_end == 0).
            // In practice callers should always split at a midpoint > 0; passing
            // token_range_start = 0 signals a full range migration and the source
            // shard will be decommissioned after cutover.
            updated_source.token_end = (token_range_start > 0)
                                           ? token_range_start - 1
                                           : 0;

            // Expand the target shard's range to cover the migrated portion
            updated_target.token_start = token_range_start;
            updated_target.token_end   = token_range_end;

            // addShard() performs an upsert (atomic update)
            topology->addShard(updated_source);
            topology->addShard(updated_target);
        }
        // If either shard is not found in the topology we still consider the
        // migration successful – the caller is responsible for registering
        // the new shard before invoking liveMigrate().
    }

    // Clean up WAL shipper registration for the now-primary target shard
    // (it no longer needs to receive WAL entries as a replica).
    if (wal_shipper) {
        wal_shipper->removeReplica(target_shard_id);
    }

    result.success = true;
    return result;
}

}  // namespace sharding
}  // namespace themis



