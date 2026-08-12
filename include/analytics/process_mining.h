/**
 * @file process_mining.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/graph_index.h"
#include "index/graph_analytics.h"
#include "index/process_graph.h"
#include "analytics/olap.h"
#include "storage/base_entity.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <chrono>

namespace themis {

/**
 * @brief Process Mining & Discovery für ThemisDB
 * 
 * Dieses Modul ermöglicht die **Ableitung von Prozessen aus bestehenden Daten**:
 * 
 * ## Bidirektionale Prozess-Architektur
 * 
 * ```
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                    ThemisDB Process Mining                          │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │                                                                     │
 * │   Modellierung → Ausführung    (Top-Down: BPMN/EPK → Instanzen)    │
 * │                                                                     │
 * │   ◄──────────── UND ────────────►                                   │
 * │                                                                     │
 * │   Daten → Prozess-Erkennung    (Bottom-Up: Event-Log → Modell)     │
 * │                                                                     │
 * └─────────────────────────────────────────────────────────────────────┘
 * ```
 * 
 * ## Process Mining Techniken
 * 
 * ### 1. Process Discovery (Alpha Miner, Heuristic Miner, Inductive Miner)
 * - Extrahiert Prozessmodelle aus Event-Logs
 * - Erkennt Sequenzen, Parallelität, Schleifen
 * 
 * ### 2. Conformance Checking
 * - Vergleicht Ist-Prozesse mit Soll-Modellen
 * - Token-Replay, Alignment-basiert
 * 
 * ### 3. Process Enhancement
 * - Erweitert Modelle mit Performance-Daten
 * - Bottleneck-Erkennung, Wartezeiten
 * 
 * ## Integration mit bestehenden Modulen
 * 
 * - **GraphAnalytics**: Community Detection für Prozess-Varianten
 * - **OLAPEngine**: Aggregation von Prozess-Metriken
 * - **VectorIndex**: Ähnliche Prozess-Muster finden
 * - **TemporalGraph**: Zeitliche Prozess-Evolution
 * 
 * ## Verwendung
 * 
 * ```cpp
 * ProcessMining mining(db);
 * 
 * // Event-Log aus Dokumenten extrahieren
 * auto eventLog = mining.extractEventLog("audit_log", {
 *     .case_id_field = "order_id",
 *     .activity_field = "action",
 *     .timestamp_field = "timestamp"
 * });
 * 
 * // Prozess-Modell ableiten
 * auto model = mining.discoverProcess(eventLog, MiningAlgorithm::HEURISTIC);
 * 
 * // Als BPMN exportieren
 * std::string bpmn = mining.exportToBPMN(model);
 * ```
 */

// ============================================================================
// Event Log Strukturen
// ============================================================================

/**
 * @brief Ein einzelnes Event im Event-Log
 */
struct ProcessEvent {
    std::string case_id;            ///< Prozess-Instanz-ID (z.B. Bestellnummer)
    std::string activity;           ///< Aktivitätsname (z.B. "Bestellung erfasst")
    int64_t timestamp_ms;           ///< Zeitstempel in Millisekunden
    
    // Optionale Attribute
    std::optional<std::string> resource;    ///< Ausführender (Benutzer/System)
    std::optional<std::string> lifecycle;   ///< start, complete, suspend, etc.
    nlohmann::json attributes;              ///< Weitere Attribute
    
    // Multi-Model-Erweiterungen
    std::optional<std::vector<float>> embedding;  ///< Vektor-Embedding der Aktivität
    std::optional<std::string> location;          ///< Geo-Location (WKT)
};

/**
 * @brief Ein Trace (Sequenz von Events für eine Case-ID)
 */
struct ProcessTrace {
    std::string case_id;
    std::vector<ProcessEvent> events;
    
    // Abgeleitete Attribute
    int64_t start_time_ms = 0;
    int64_t end_time_ms = 0;
    int64_t duration_ms = 0;
    bool is_complete = false;
    
    // Varianten-Erkennung
    std::string variant_signature;  ///< Hash der Aktivitätssequenz
    int variant_id = 0;
};

/**
 * @brief Vollständiges Event-Log
 */
struct EventLog {
    std::vector<ProcessTrace> traces;
    
    // Statistiken
    size_t total_events = 0;
    size_t unique_activities = 0;
    size_t unique_cases = 0;
    size_t unique_variants = 0;
    
    // Aktivitäts-Mapping
    std::map<std::string, int> activity_to_id;
    std::vector<std::string> id_to_activity;
    
    // Zeitbereich
    int64_t min_timestamp = 0;
    int64_t max_timestamp = 0;
};

// ============================================================================
// Extrahierte Prozess-Strukturen
// ============================================================================

/**
 * @brief Direkt-Follows Graph (DFG)
 * 
 * Zeigt, welche Aktivitäten direkt aufeinander folgen.
 */
struct DirectlyFollowsGraph {
    std::set<std::string> activities;
    
    struct Edge {
        std::string from;
        std::string to;
        int frequency = 0;          ///< Wie oft diese Sequenz vorkommt
        double avg_duration_ms = 0; ///< Durchschnittliche Zeit zwischen den Aktivitäten
        double confidence = 0;      ///< Anteil an allen Fällen
    };
    std::vector<Edge> edges;
    
    // Start- und End-Aktivitäten
    std::set<std::string> start_activities;
    std::set<std::string> end_activities;
    
    // Selbst-Schleifen
    std::map<std::string, int> self_loops;
};

/**
 * @brief Entdecktes Prozess-Modell
 */
struct DiscoveredProcess {
    std::string id;
    std::string name;
    
    // Modell-Struktur (als Graph)
    struct Node {
        std::string id;
        std::string name;
        std::string type;           ///< TASK, EVENT, GATEWAY
        std::string gateway_type;   ///< XOR, AND, OR (für Gateways)
        
        // Statistiken
        int frequency = 0;
        double avg_duration_ms = 0;
    };
    std::vector<Node> nodes;
    
    struct Edge {
        std::string id;
        std::string from;
        std::string to;
        int frequency = 0;
        double probability = 0;     ///< Wahrscheinlichkeit dieser Transition
    };
    std::vector<Edge> edges;
    
    // Qualitätsmetriken
    double fitness = 0;             ///< Wie gut das Modell das Log erklärt
    double precision = 0;           ///< Wie präzise ist das Modell
    double generalization = 0;      ///< Wie gut generalisiert es
    double simplicity = 0;          ///< Strukturelle Komplexität
};

// ============================================================================
// Mining-Algorithmen
// ============================================================================

/**
 * @brief Verfügbare Mining-Algorithmen
 */
enum class MiningAlgorithm {
    ALPHA,          ///< Alpha Miner - klassisch, findet Parallelität
    ALPHA_PLUS,     ///< Alpha+ - behandelt Schleifen besser
    HEURISTIC,      ///< Heuristic Miner - robust gegen Rauschen
    INDUCTIVE,      ///< Inductive Miner - garantiert Sound-Modell
    SPLIT,          ///< Split Miner - balanciert Fitness/Precision
    FUZZY           ///< Fuzzy Miner - für komplexe Logs
};

/**
 * @brief Konfiguration für Event-Log-Extraktion
 */
struct EventLogConfig {
    std::string case_id_field;      ///< Feld für Case-ID
    std::string activity_field;     ///< Feld für Aktivitätsname
    std::string timestamp_field;    ///< Feld für Zeitstempel
    
    // Optionale Felder
    std::optional<std::string> resource_field;
    std::optional<std::string> lifecycle_field;
    std::optional<std::string> embedding_field;
    std::optional<std::string> location_field;
    
    // Filter
    std::optional<int64_t> start_time;
    std::optional<int64_t> end_time;
    std::vector<std::string> include_activities;
    std::vector<std::string> exclude_activities;
};

/**
 * @brief Konfiguration für Mining-Algorithmen
 */
struct MiningConfig {
    MiningAlgorithm algorithm = MiningAlgorithm::HEURISTIC;
    
    // Heuristic Miner Parameter
    double dependency_threshold = 0.9;      ///< Min. Abhängigkeit für Kante
    double positive_observations = 10;       ///< Min. Häufigkeit
    double relative_to_best = 0.05;         ///< Relative Threshold
    
    // Inductive Miner Parameter
    double noise_threshold = 0.2;           ///< Erlaubtes Rauschen
    
    // Allgemeine Parameter
    bool detect_loops = true;
    bool detect_parallelism = true;
    int max_activities = 100;               ///< Limit für Komplexität
};

// ============================================================================
// Process Mining Engine
// ============================================================================

class RocksDBWrapper;

/**
 * @brief Process Mining Engine
 * 
 * Hauptklasse für Process Discovery und Conformance Checking.
 */
class ProcessMining {
public:
    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    explicit ProcessMining(RocksDBWrapper& db);
    
    // ===== Event Log Extraktion =====
    
    /**
     * @brief Extrahiert Event-Log aus einer Collection
     * 
     * ```aql
     * -- Äquivalente AQL-Abfrage:
     * FOR doc IN audit_log
     *   COLLECT case_id = doc.order_id INTO events
     *   RETURN { case_id, events: events[*].doc }
     * ```
     */
    std::pair<Status, EventLog> extractEventLog(
        std::string_view collection,
        const EventLogConfig& config
    );
    
    /**
     * @brief Extrahiert Event-Log aus Graph-Kanten (temporale Edges)
     * 
     * Nutzt _from, _to und Zeitstempel um Sequenzen zu erkennen.
     */
    std::pair<Status, EventLog> extractEventLogFromGraph(
        std::string_view edge_collection,
        std::string_view case_id_field = "case_id"
    );
    
    /**
     * @brief Extrahiert Event-Log aus vernetzten Dokumenten
     * 
     * Folgt Referenzen zwischen Dokumenten und erstellt Traces.
     */
    std::pair<Status, EventLog> extractEventLogFromReferences(
        std::string_view start_collection,
        const std::vector<std::string>& reference_fields,
        std::string_view activity_field = "_type"
    );

    // ===== Process Discovery =====
    
    /**
     * @brief Erstellt Directly-Follows Graph aus Event-Log
     */
    std::pair<Status, DirectlyFollowsGraph> createDFG(const EventLog& log);
    
    /**
     * @brief Entdeckt Prozess-Modell aus Event-Log
     */
    std::pair<Status, DiscoveredProcess> discoverProcess(
        const EventLog& log,
        const MiningConfig& config = {}
    );
    
    /**
     * @brief Entdeckt Prozess-Modell direkt aus Collection
     */
    std::pair<Status, DiscoveredProcess> discoverProcessFromCollection(
        std::string_view collection,
        const EventLogConfig& log_config,
        const MiningConfig& mining_config = {}
    );

    // ===== Varianten-Analyse =====
    
    /**
     * @brief Identifiziert Prozess-Varianten
     */
    struct VariantInfo {
        int variant_id;
        std::vector<std::string> activities;    ///< Aktivitätssequenz
        int frequency;                          ///< Anzahl Cases
        double percentage;                      ///< Anteil am Gesamt
        double avg_duration_ms;
        std::vector<std::string> case_ids;
    };
    std::pair<Status, std::vector<VariantInfo>> analyzeVariants(
        const EventLog& log,
        int top_n = 20
    );
    
    /**
     * @brief Clustert ähnliche Varianten (nutzt VectorIndex)
     */
    std::pair<Status, std::map<int, std::vector<int>>> clusterVariants(
        const EventLog& log,
        int num_clusters = 5
    );

    // ===== Conformance Checking =====
    
    /**
     * @brief Token-Replay Conformance
     */
    struct ConformanceResult {
        double fitness = 0.0;                   ///< 0.0 - 1.0
        double precision = 0.0;                 ///< Approximation of precision
        int consumed_tokens = 0;
        int produced_tokens = 0;
        int missing_tokens = 0;
        int remaining_tokens = 0;
        
        std::vector<std::string> deviations;    ///< Beschreibung der Abweichungen
    };
    std::pair<Status, ConformanceResult> checkConformance(
        const EventLog& log,
        const DiscoveredProcess& model
    );
    
    /**
     * @brief Alignment-basierte Conformance (präziser, aber langsamer)
     */
    struct AlignmentResult {
        double fitness;
        double precision;
        
        struct Move {
            std::string type;   ///< "sync", "model", "log"
            std::string activity;
            double cost;
        };
        std::vector<std::vector<Move>> alignments;  ///< Pro Trace
    };
    std::pair<Status, AlignmentResult> computeAlignment(
        const EventLog& log,
        const DiscoveredProcess& model
    );

    // ===== Process Enhancement =====
    
    /**
     * @brief Fügt Performance-Daten zum Modell hinzu
     */
    struct EnhancedProcess {
        DiscoveredProcess model;
        
        // Performance-Overlay
        std::map<std::string, double> node_avg_duration;
        std::map<std::string, double> node_avg_waiting;
        std::map<std::string, int> node_frequency;
        
        std::map<std::string, double> edge_avg_duration;
        std::map<std::string, double> edge_probability;
        
        // Bottlenecks
        std::vector<std::string> bottleneck_nodes;
        std::vector<std::string> rework_loops;
    };
    std::pair<Status, EnhancedProcess> enhanceWithPerformance(
        const DiscoveredProcess& model,
        const EventLog& log
    );
    
    /**
     * @brief Erkennt Bottlenecks (nutzt GraphAnalytics)
     */
    std::pair<Status, std::vector<std::string>> detectBottlenecks(
        const EnhancedProcess& process,
        double threshold_percentile = 0.9
    );

    // ===== Export =====
    
    /**
     * @brief Exportiert als BPMN 2.0 XML
     */
    std::pair<Status, std::string> exportToBPMN(const DiscoveredProcess& model);
    
    /**
     * @brief Exportiert als Petri-Netz (PNML)
     */
    std::pair<Status, std::string> exportToPNML(const DiscoveredProcess& model);
    
    /**
     * @brief Speichert als ThemisDB Prozess-Definition
     * 
     * Erstellt Einträge in _process_definitions, _process_nodes, _process_edges
     */
    Status saveAsProcessDefinition(
        const DiscoveredProcess& model,
        std::string_view process_id
    );

    // ===== Multi-Model Process Mining =====
    
    /**
     * @brief Findet ähnliche Prozess-Fragmente über Vektor-Suche
     */
    struct SimilarFragment {
        std::vector<std::string> activities;
        double similarity;
        std::vector<std::string> source_cases;
    };
    std::pair<Status, std::vector<SimilarFragment>> findSimilarPatterns(
        const std::vector<std::string>& pattern,
        const EventLog& log,
        int k = 10
    );
    
    /**
     * @brief Erkennt geografische Prozess-Cluster
     */
    struct GeoProcessCluster {
        std::string region;                     ///< Geo-Region
        std::string centroid_wkt;               ///< Zentrum
        DiscoveredProcess local_model;          ///< Lokales Prozess-Modell
        int case_count;
        std::vector<std::string> deviations;    ///< Abweichungen vom Hauptmodell
    };
    std::pair<Status, std::vector<GeoProcessCluster>> discoverGeoVariants(
        const EventLog& log,
        double cluster_radius_km = 50.0
    );
    
    /**
     * @brief Temporale Prozess-Evolution
     * 
     * Wie hat sich der Prozess über Zeit verändert?
     */
    struct ProcessEvolution {
        struct Snapshot {
            int64_t period_start;
            int64_t period_end;
            DiscoveredProcess model;
            double fitness_vs_previous;
            std::vector<std::string> new_patterns;
            std::vector<std::string> removed_patterns;
        };
        std::vector<Snapshot> snapshots;
    };
    std::pair<Status, ProcessEvolution> analyzeEvolution(
        const EventLog& log,
        int num_periods = 12
    );

private:
    RocksDBWrapper& db_;
    
    // Mining-Algorithmus-Implementierungen
    DiscoveredProcess runAlphaMiner(const EventLog& log, const MiningConfig& config);
    DiscoveredProcess runHeuristicMiner(const EventLog& log, const MiningConfig& config);
    DiscoveredProcess runInductiveMiner(const EventLog& log, const MiningConfig& config);
    
    // Hilfsfunktionen
    std::string computeVariantSignature(const std::vector<std::string>& activities);
    std::vector<float> embedActivities(const std::vector<std::string>& activities);
};

// ============================================================================
// AQL-Funktionen für Process Mining
// ============================================================================

/**
 * @brief Process Mining Funktionen für AQL
 * 
 * Diese Funktionen können in regulären AQL-Abfragen verwendet werden:
 * 
 * ```aql
 * -- Event-Log aus Audit-Daten
 * LET log = EXTRACT_EVENT_LOG("audit", {
 *   caseId: "order_id",
 *   activity: "action", 
 *   timestamp: "ts"
 * })
 * 
 * -- Prozess-Modell ableiten
 * LET model = DISCOVER_PROCESS(log, { algorithm: "heuristic" })
 * 
 * -- Als BPMN speichern
 * RETURN EXPORT_BPMN(model)
 * ```
 */
namespace ProcessMiningFunctions {
    // Funktionsnamen für AQL-Integration
    constexpr const char* EXTRACT_EVENT_LOG = "EXTRACT_EVENT_LOG";
    constexpr const char* DISCOVER_PROCESS = "DISCOVER_PROCESS";
    constexpr const char* ANALYZE_VARIANTS = "ANALYZE_VARIANTS";
    constexpr const char* CHECK_CONFORMANCE = "CHECK_CONFORMANCE";
    constexpr const char* DETECT_BOTTLENECKS = "DETECT_BOTTLENECKS";
    constexpr const char* EXPORT_BPMN = "EXPORT_BPMN";
    constexpr const char* FIND_SIMILAR_PATTERNS = "FIND_SIMILAR_PATTERNS";
    constexpr const char* PROCESS_EVOLUTION = "PROCESS_EVOLUTION";
    
    /**
     * @brief Registriert Process Mining Funktionen beim AQL-Parser
     */
    void registerFunctions();
}

} // namespace themis

