/**
 * @file wal_shipper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/wal_manager.h"
#include "sharding/mtls_client.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace themis::sharding {

/**
 * WAL Shipper
 * 
 * Asynchronously ships WAL entries to replica shards for replication.
 * Inspired by PostgreSQL's WAL sender process.
 * 
 * Features:
 * - Background thread for continuous shipping
 * - Retry logic with exponential backoff
 * - Replication lag monitoring
 * - Batch shipping for efficiency
 * - Automatic recovery from network failures
 */

/** @brief Runtime replication health/lag snapshot for one replica target. */
struct ReplicaInfo {
    std::string replica_id;         ///< Eindeutige Replika-ID.
    std::string endpoint;           ///< Ziel-Endpoint fuer WAL-Batches.
    LSN last_confirmed_lsn;         ///< Letzte durch Replika bestaetigte LSN.
    uint64_t lag_bytes = 0;         ///< Replikationsrueckstand in Bytes.
    uint64_t lag_ms = 0;            ///< Replikationsrueckstand in Millisekunden.
    bool is_healthy = true;         ///< Letztes Health-/Retry-Ergebnis.
    uint64_t last_success_ts = 0;   ///< Unix-Zeit ms des letzten erfolgreichen Shipments.
    uint64_t consecutive_failures = 0; ///< Aufeinanderfolgende fehlgeschlagene Shipments.
};

/** @brief Runtime configuration for asynchronous WAL shipping. */
struct WALShipperConfig {
    std::string primary_id;               ///< Kennung des primaries im Replikationsstrom.
    size_t batch_size = 100;              ///< Maximale Anzahl WAL-Eintraege pro Batch.
    size_t max_batch_bytes = 1024 * 1024; ///< Maximale Batch-Groesse in Bytes.
    uint64_t ship_interval_ms = 100;      ///< Zyklusintervall des Shipping-Loops.
    uint64_t retry_delay_ms = 1000;       ///< Initiale Retry-Verzoegerung.
    uint64_t max_retry_delay_ms = 60000;  ///< Obergrenze fuer exponentiellen Backoff.
    size_t max_retries = 5;               ///< Maximale Retry-Versuche pro Fehlerfall.
    uint64_t health_check_interval_ms = 10000; ///< Health-Check-Intervall in Millisekunden.
    
    // Compression configuration
    enum class CompressionType {
        None, ///< Keine Kompression.
        LZ4,  ///< Schnelle Kompression mit niedriger CPU-Last.
        Zstd  ///< Hoehere Kompressionsrate bei hoehrem CPU-Verbrauch.
    };
    CompressionType compression = CompressionType::Zstd; ///< Standard-Kompressionsalgorithmus.
    int compression_level = 3; ///< Level fuer Zstd/LZ4.
    
    // Adaptive batching
    bool adaptive_batch_size = false; ///< Aktiviert adaptive Batch-Groesse nach Laufzeitmetriken.
    size_t min_batch_size = 10;       ///< Untere Grenze fuer adaptive Batch-Groesse.
    size_t max_batch_size = 1000;     ///< Obere Grenze fuer adaptive Batch-Groesse.
    
    // mTLS configuration
    std::string cert_path;    ///< mTLS Client-Zertifikat.
    std::string key_path;     ///< mTLS Private Key.
    std::string ca_cert_path; ///< mTLS CA-Kette fuer Server-Validierung.
};

/** @brief Aggregated counters and lag/throughput statistics for WAL shipping. */
struct WALShipperStats {
    uint64_t total_entries_shipped = 0;     ///< Gesamtzahl versendeter WAL-Eintraege.
    uint64_t total_bytes_shipped = 0;       ///< Gesamtzahl gesendeter WAL-Bytes (logisch).
    uint64_t total_bytes_uncompressed = 0;  ///< Gesamtzahl unkomprimierter Payload-Bytes.
    uint64_t total_batches = 0;             ///< Anzahl gesendeter Batches.
    uint64_t failed_ships = 0;              ///< Anzahl fehlgeschlagener Ship-Vorgaenge.
    uint64_t retries = 0;                   ///< Anzahl ausgefuehrter Retry-Versuche.
    std::chrono::milliseconds avg_ship_time{0}; ///< Gleitender Mittelwert der Loop-Dauer.
    std::chrono::milliseconds max_lag{0};   ///< Beobachteter maximaler Lag.
    double avg_compression_ratio = 1.0;     ///< Mittelwert unkomprimiert/komprimiert.
    uint64_t total_snapshot_chunks_sent = 0; ///< Versendete Snapshot-Chunks.
    uint64_t total_snapshot_bytes_sent = 0;  ///< Versendete Snapshot-Bytes.
};

/**
 * A single chunk of a snapshot being transferred to a lagging replica.
 *
 * Snapshot data is compressed on the sender side (e.g. with ZSTD) and split
 * into fixed-size pieces.  Each piece carries a SHA-256 checksum of its own
 * payload so the receiver can detect corruption independently of adjacent
 * chunks, tolerating network interruption and partial retries.
 */
struct SnapshotChunk {
    uint64_t snapshot_index;    ///< Identifies the snapshot this chunk belongs to
    uint64_t snapshot_term;     ///< Raft term of the last entry covered by the snapshot
    uint64_t chunk_index;       ///< 0-based index of this chunk within the snapshot
    uint64_t total_chunks;      ///< Total number of chunks for this snapshot
    std::vector<uint8_t> data;  ///< Compressed chunk payload
    std::string checksum;       ///< SHA-256 of this chunk's data (hex string)
    bool last_chunk = false;    ///< True if this is the final chunk
};

/** @brief Result summary for one snapshot transfer attempt. */
struct SnapshotTransferResult {
    bool success = false;
    uint64_t chunks_sent = 0;
    uint64_t bytes_sent = 0;
    std::string error_message;
};

/**
 * WAL Shipper
 * 
 * Ships WAL entries from primary to replicas asynchronously
 */
class WALShipper {
public:
    /**
     * @brief Construct WAL shipper.
     * @param wal_manager WAL source manager.
     * @param config Shipping configuration.
     */
    WALShipper(std::shared_ptr<WALManager> wal_manager,
               const WALShipperConfig& config);

    /** @brief Destructor stops background shipping thread. */
    ~WALShipper();
    
    /**
     * @brief Register replica target for shipping.
     * @param replica_id Replica identifier.
     * @param endpoint Replica endpoint.
     */
    void addReplica(const std::string& replica_id, const std::string& endpoint);
    
    /** @brief Remove replica target by id. */
    void removeReplica(const std::string& replica_id);
    
    /** @brief Start background shipping loop. */
    void start();
    
    /** @brief Stop background shipping loop. */
    void stop();
    
    /** @brief Return whether shipping loop is active. */
    /**
     * @return true, wenn der Hintergrundthread aktiv laeuft.
     */
    bool isRunning() const;
    
    /** @brief Return snapshot of registered replica state. */
    /**
     * @return Kopie der aktuellen ReplicaInfo-Eintraege.
     */
    std::vector<ReplicaInfo> getReplicaInfo() const;
    
    /** @brief Return shipper statistics snapshot. */
    /**
     * @return Thread-sichere Momentaufnahme der WALShipperStats.
     */
    WALShipperStats getStatistics() const;
    
    /** @brief Wake shipping loop for immediate processing cycle. */
    void forceShip();
    
    /** @brief Set optional Prometheus metrics exporter. */
    void setMetricsExporter(std::shared_ptr<class PrometheusMetrics> metrics);
    
    /**
     * @brief Compute adaptive batch size from current network/CPU/IOPS telemetry.
     * @param network_latency_ms Average network round-trip latency in milliseconds.
     * @param cpu_utilization CPU utilization ratio in [0.0, 1.0].
     * @param disk_iops_available Estimated available disk IOPS.
     * @return Clamped batch size in range [min_batch_size, max_batch_size].
     */
    size_t calculateOptimalBatchSize(double network_latency_ms,
                                     double cpu_utilization,
                                     size_t disk_iops_available) const;
    
    /**
     * @brief Select compression strategy for one payload under current CPU load.
     * @param payload_size Payload size in bytes.
     * @param is_repetitive True when payload is expected to compress well.
     * @param cpu_utilization CPU utilization ratio in [0.0, 1.0].
     * @return Selected compression type (None/LZ4/Zstd).
     */
    WALShipperConfig::CompressionType selectCompressionType(size_t payload_size,
                                                            bool is_repetitive,
                                                            double cpu_utilization) const;

    // ------------------------------------------------------------------
    // Snapshot transfer API (for lagging-replica catch-up)
    // ------------------------------------------------------------------

    /**
     * @brief Transfer a full snapshot to a lagging replica in fixed-size chunks.
     *
     * Each chunk is accompanied by a SHA-256 checksum of its payload.  If the
     * connection drops mid-transfer the caller may retry from the last
     * confirmed chunk index; the receiver should verify each chunk's checksum
     * before writing it to stable storage.
     *
     * @param replica_id   Target replica identifier
     * @param chunks       Ordered sequence of chunks (chunk_index 0 … N-1)
     * @return Transfer result with success/failure details
     */
    SnapshotTransferResult sendSnapshot(const std::string& replica_id,
                                        const std::vector<SnapshotChunk>& chunks);

    /**
     * @brief Verify a snapshot chunk's integrity using its embedded checksum.
     *
     * Computes SHA-256 of chunk.data and compares it against chunk.checksum.
     * Should be called by the receiver before accepting each chunk.
     *
     * @param chunk  The chunk to verify
     * @return true if the checksum matches
     */
    static bool verifyChunkChecksum(const SnapshotChunk& chunk);

private:
    WALShipperConfig config_; ///< Laufzeitkonfiguration fuer Versand, Retry, Kompression.
    std::shared_ptr<WALManager> wal_manager_; ///< Quelle fuer lokal persistierte WAL-Eintraege.
    
    // Replicas
    mutable std::mutex replicas_mutex_; ///< Schutz der Replika-Registrierung und Zustandsdaten.
    std::map<std::string, ReplicaInfo> replicas_; ///< Replika-Zielzustand nach replica_id.
    
    // Shipping thread
    std::atomic<bool> running_{false}; ///< Lifecycle-Flag fuer shippingLoop().
    std::unique_ptr<std::thread> shipper_thread_; ///< Hintergrundthread fuer asynchronen Versand.
    std::mutex cv_mutex_; ///< Schutz fuer Wait/Wakeup-Synchronisierung.
    std::condition_variable cv_; ///< Notifikation fuer forceShip()/stop().
    
    // Statistics
    mutable std::mutex stats_mutex_; ///< Schutz fuer Statistikaktualisierungen.
    WALShipperStats stats_; ///< Laufzeitstatistiken des Shippers.
    
    // mTLS client
    std::shared_ptr<MTLSClient> mtls_client_; ///< Optionaler mTLS-Transportclient.
    
    // Prometheus metrics (optional)
    std::shared_ptr<class PrometheusMetrics> metrics_; ///< Optionales Prometheus-Metrics-Backend.
    
    /**
     * @brief Hintergrund-Loop ueber alle registrierten Repliken.
     *
     * Fuehrt periodisches Shipping, Lag-Berechnung und gleitende Timing-Statistik
     * aus. Der Loop endet, sobald running_ auf false gesetzt wird.
     */
    void shippingLoop();
    
    /**
     * @brief Versendet ausstehende WAL-Eintraege an eine Replika.
     * @param replica_id Replika-ID (nur fuer Kontext/Tracing).
     * @param replica Mutable Replika-Zustandseintrag.
     * @return true bei erfolgreichem Versand aller ausstehenden Batches.
     */
    bool shipToReplica(const std::string& replica_id, ReplicaInfo& replica);
    
    /**
     * @brief Serialisiert/komprimiert und versendet einen WAL-Batch.
     * @param endpoint Ziel-Endpoint.
     * @param entries WAL-Eintraege des Batches.
     * @return true, wenn der Endpoint den Batch akzeptiert.
     */
    bool shipBatch(const std::string& endpoint,
                   const std::vector<WALEntry>& entries);
    
    /**
     * @brief Aktualisiert Health-/Fehlerzaehler nach einem Ship-Versuch.
     * @param replica Replikaeintrag.
     * @param success Erfolg des Versuchs.
     * @param bytes_shipped Bei Erfolg versendete Nutzdatenbytes.
     */
    void updateReplicaStatus(ReplicaInfo& replica, bool success, size_t bytes_shipped);
    
    /**
     * @brief Berechnet Lag in Bytes/Zeit fuer eine Replika neu.
     * @param replica Replikaeintrag.
     */
    void calculateLag(ReplicaInfo& replica);
    
    /**
     * @brief Fuehrt einen Health-Probe-Request gegen den Replica-Endpoint aus.
     * @param replica Replikaeintrag.
     */
    void healthCheck(ReplicaInfo& replica);
};

} // namespace themis::sharding
