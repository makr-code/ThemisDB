/**
 * @file data_migrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declarations
class PrometheusMetrics;
class ShardTopology;
class WALShipper;

/**
 * @brief Fortschrittsdaten einer laufenden Datenmigration.
 *
 * Wird bei jeder verarbeiteten Batch aktualisiert und optional ueber
 * ProgressCallback an Aufrufer weitergereicht.
 */
struct MigrationProgress {
    uint64_t records_migrated = 0; ///< Bereits migrierte Datensaetze.
    uint64_t total_records = 0;    ///< Erwartete Gesamtanzahl im Token-Bereich.
    uint64_t bytes_transferred = 0; ///< Uebertragene Bytes ueber alle Batches.
    uint64_t errors = 0;           ///< Anzahl erkannter Batch-Fehler.
    double progress_percent = 0.0; ///< Fortschritt in Prozent [0, 100].
    std::string migration_id;      ///< Deterministische Migrations-ID.
};

/**
 * @brief Ergebnis einer abgeschlossenen Migration.
 */
struct MigrationResult {
    bool success = false;               ///< True bei erfolgreichem Abschluss.
    uint64_t records_migrated = 0;      ///< Migrierte Datensaetze.
    uint64_t bytes_transferred = 0;     ///< Uebertragene Gesamtdatenmenge in Bytes.
    std::vector<std::string> errors;    ///< Batch-spezifische Fehlerdetails.
    std::string error_message;          ///< Zusammenfassende Fehlermeldung.
    std::string migration_id;           ///< Deterministische Tracking-ID.
    bool was_already_completed = false; ///< True, falls Idempotenz die Arbeit uebersprang.
};

/**
 * Configuration for live (dual-write) shard migration.
 *
 * During a live migration the old shard continues to accept writes.  The new
 * shard receives a bulk copy of existing data (via migrate()), then catches up
 * with writes that arrived during the copy by replaying WAL entries shipped
 * from the source via WALShipper.  Once the WAL lag falls below
 * max_wal_lag_bytes an atomic topology cutover is performed via ShardTopology,
 * making the new shard the authoritative owner of the token range while the
 * old shard is downgraded to read-only and eventually decommissioned.
 *
 * This protocol guarantees 0 ms read-unavailability: reads are served from
 * the source shard until the moment of cutover, and from the target shard
 * immediately afterwards.
 */
struct LiveMigrationConfig {
    /// Allow dual writes during migration (old shard always accepts writes).
    bool enable_dual_write = true;

    /// How often to poll the WAL shipper for lag convergence.
    std::chrono::milliseconds catchup_poll_interval{200};

    /// Maximum total time to wait for WAL catchup before failing the migration.
    std::chrono::milliseconds catchup_timeout{std::chrono::minutes(10)};

    /// Target WAL lag (bytes) that must be reached before atomic cutover.
    uint64_t max_wal_lag_bytes = 1024 * 1024;  // 1 MB

    /// Perform a final integrity verification after the bulk copy but before
    /// committing to WAL catchup.
    bool verify_after_bulk_copy = true;
};

/// Result of a dual-write live migration.
struct LiveMigrationResult {
    bool success = false;

    /// Result of the initial bulk data copy phase.
    MigrationResult bulk_migration;

    /// Number of WAL entries applied during the catchup phase.
    uint64_t wal_entries_applied = 0;

    /// Final WAL lag at cutover time (0 means fully caught up).
    uint64_t final_wal_lag_bytes = 0;

    std::string error_message;
    std::string migration_id;
};

/**
 * @brief Konfiguration des DataMigrator.
 */
struct DataMigratorConfig {
    std::string source_endpoint; ///< Source-Endpoint fuer Fetch/Count-Anfragen.
    std::string target_endpoint; ///< Target-Endpoint fuer Write-Anfragen.
    std::string cert_path;       ///< Client-Zertifikatspfad fuer mTLS.
    std::string key_path;        ///< Private-Key-Pfad fuer mTLS.
    std::string ca_cert_path;    ///< CA-Zertifikat zur Peer-Validierung.
    uint32_t batch_size = 1000;  ///< Anzahl Records pro Batch.
    bool verify_integrity = true; ///< Fuehrt Hash-basierte Integritaetspruefung aus.
    uint32_t max_retries = 3;    ///< Maximalzahl Retries pro fehlgeschlagener Operation.
    uint32_t retry_delay_ms = 1000; ///< Basiswartezeit zwischen Retries in Millisekunden.
    
    // Idempotency configuration
    bool enable_idempotency = true; ///< Aktiviert idempotente Wiederaufnahme.
    std::string idempotency_store_path = "./migrations"; ///< Persistenzpfad fuer Idempotenzstatus.
};

/**
 * Handles data migration between shards
 * 
 * Features:
 * - Stream-based batch processing
 * - Data integrity verification (hash-based)
 * - Atomic cutover
 * - Progress tracking
 * - Error handling with retry
 */
class DataMigrator {
public:
    /** @brief Callback fuer MigrationProgress-Updates. */
    using ProgressCallback = std::function<void(const MigrationProgress&)>;

    /**
     * @brief Erzeugt einen DataMigrator.
     * @param config Laufzeitkonfiguration inklusive Endpoints und mTLS-Pfaden.
     * @param metrics Optionales Prometheus-Metrics-Backend.
     * @throws std::invalid_argument Wenn Endpoints leer sind oder batch_size == 0.
     */
    explicit DataMigrator(
        const DataMigratorConfig& config,
        std::shared_ptr<PrometheusMetrics> metrics = nullptr
    );

    /** @brief Standard-Destruktor ohne spezielle Ressourcenlogik. */
    ~DataMigrator() = default;

    /**
     * Migrate data for a token range from source to target shard
     * 
     * @param source_shard_id Source shard identifier
     * @param target_shard_id Target shard identifier
     * @param token_range_start Start of token range to migrate
     * @param token_range_end End of token range to migrate
     * @param progress_callback Optional callback for progress updates
     * @return MigrationResult with success status and statistics
     */
    MigrationResult migrate(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end,
        ProgressCallback progress_callback = nullptr
    );

    /**
     * Verify data integrity between source and target
     * 
     * @param source_shard_id Source shard identifier
     * @param target_shard_id Target shard identifier
     * @param token_range_start Start of token range
     * @param token_range_end End of token range
     * @return true if data matches, false otherwise
     */
    bool verifyIntegrity(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end
    );

    /**
     * Perform a live (dual-write) migration with zero read-unavailability.
     *
     * Protocol:
     *  1. Bulk-copy existing data from source to target using migrate().
     *  2. While the copy runs the source shard continues accepting writes.
     *  3. After the bulk copy the target shard is registered with wal_shipper
     *     so it receives incremental WAL entries for the migrated token range.
     *  4. Once the WAL lag drops below live_cfg.max_wal_lag_bytes an atomic
     *     cutover is performed: topology is updated via ShardTopology, the
     *     source shard becomes read-only for the token range, and the target
     *     shard becomes authoritative.
     *
     * @param source_shard_id     Source (hot) shard
     * @param target_shard_id     Destination shard for the token range
     * @param token_range_start   Start of token range to migrate
     * @param token_range_end     End of token range to migrate
     * @param topology            ShardTopology for atomic cutover (may be nullptr to skip cutover)
     * @param wal_shipper         WALShipper for incremental catchup (may be nullptr to skip WAL phase)
     * @param live_cfg            Live migration configuration
     * @param progress_callback   Optional progress callback for bulk-copy phase
     * @return                    LiveMigrationResult
     */
    LiveMigrationResult liveMigrate(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end,
        std::shared_ptr<ShardTopology> topology = nullptr,
        std::shared_ptr<WALShipper> wal_shipper = nullptr,
        const LiveMigrationConfig& live_cfg = LiveMigrationConfig{},
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Erzeugt eine deterministische Migrations-ID aus Quelle/Ziel/Range.
     * @param source_shard_id Quell-Shard.
     * @param target_shard_id Ziel-Shard.
     * @param token_range_start Range-Start.
     * @param token_range_end Range-Ende.
     * @return Deterministische, hash-basierte Migrations-ID.
     */
    std::string generateMigrationId(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end
    );
    
    /**
     * @brief Erzeugt eine deterministische Batch-ID innerhalb einer Migration.
     * @param migration_id Deterministische Migrations-ID.
     * @param batch_index Laufender Batch-Index.
     * @return Eindeutige Batch-ID fuer Idempotenztracking.
     */
    std::string generateBatchId(
        const std::string& migration_id,
        uint32_t batch_index
    );
    
    /** @brief Prueft, ob eine Migration bereits als abgeschlossen markiert ist. */
    bool isMigrationCompleted(const std::string& migration_id);

    /** @brief Markiert eine Migration als abgeschlossen und persistiert den Zustand. */
    void markMigrationCompleted(const std::string& migration_id);

    /** @brief Prueft, ob eine Batch bereits erfolgreich verarbeitet wurde. */
    bool isBatchCompleted(const std::string& batch_id);

    /** @brief Markiert eine Batch als abgeschlossen; persistiert periodisch den Status. */
    void markBatchCompleted(const std::string& batch_id);

private:
    DataMigratorConfig config_; ///< Laufzeitkonfiguration.
    std::shared_ptr<PrometheusMetrics> metrics_; ///< Optionales Metrics-Backend.
    
    // Idempotency tracking
    mutable std::mutex idempotency_mutex_; ///< Schutz fuer idempotente Statusmengen.
    std::unordered_set<std::string> completed_migrations_; ///< Bereits finalisierte Migrationen.
    std::unordered_set<std::string> completed_batches_; ///< Bereits verarbeitete Batch-IDs.
    std::atomic<size_t> batch_counter_{0}; ///< Zaehler fuer periodische Statuspersistenz.
    
    /**
     * Load idempotency state from persistent storage
     */
    void loadIdempotencyState();
    
    /**
     * Save idempotency state to persistent storage
     */
    void saveIdempotencyState();

    /**
     * @brief Holt eine Daten-Batch vom Quell-Shard.
     * @param source_shard_id Quell-Shard-ID.
     * @param token_range_start Range-Start.
     * @param token_range_end Range-Ende.
     * @param offset Paging-Offset.
     * @param limit Maximale Anzahl Records.
     * @return JSON-Array mit Records; bei Fehlern leeres Array.
     */
    nlohmann::json fetchBatch(
        const std::string& source_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end,
        uint32_t offset,
        uint32_t limit
    );

    /**
     * @brief Schreibt eine Batch atomar auf den Ziel-Shard.
     * @param target_shard_id Ziel-Shard-ID.
     * @param batch Zu schreibende Records als JSON-Array.
     * @return true bei bestaetigtem erfolgreichem Write.
     */
    bool writeBatch(
        const std::string& target_shard_id,
        const nlohmann::json& batch
    );

    /**
     * @brief Berechnet einen SHA-256-Hash ueber serialisierte JSON-Daten.
     * @param data Eingabedaten.
     * @return Hex-codierter SHA-256-Hash.
     */
    std::string calculateHash(const nlohmann::json& data);

    /**
     * @brief Fuehrt eine Operation mit begrenzten Retries aus.
     * @tparam Func Callable mit Rueckgabe bool.
     * @param func Auszufuehrende Operation.
     * @return true sobald ein Versuch erfolgreich war, sonst false.
     */
    template<typename Func>
    bool retryOperation(Func func);
};

} // namespace sharding
} // namespace themis
