---
type: enhancement
labels: ["type:enhancement", "module:server", "module:training", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-B
session: S-7
layer: 8
---

# [IMPL-B8] Layer 8: WorkloadFingerprintEngine

## Aufgabe

`LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer-8 definiert den `WorkloadFingerprintEngine`:
Er destilliert den aktuellen Workload eines Tenants auf einen kompakten
**Fingerprint-Vektor** — eine semantische Signatur die beschreibt WAS der Tenant
gerade tut (Batch-Analytics, OLTP-Mikrotransaktionen, Reporting usw.) —
und leitet daraus Ressourcen-Policy-Anpassungen ab.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `WorkloadFingerprintEngine` Klasse | Neuer Ressourcen-Manager |
| `WorkloadFingerprint` Struct (Vektor + Metadaten) | ML-Training des Fingerprint-Modells |
| Pattern-Matching gegen bekannte Profile (OLTP/OLAP/Batch/Mixed) | Tatsächliche Ressourcen-Zuteilung (Advisory an TenantManager) |
| `TenantManager`-Integration via Advisory-Callback | Multi-Cluster-Scheduling |
| Cross-Shard-Transfer-Hinweis (für Layer 11, DK-2/DK-4) | Vollständige Tenant-Isolation-Engine |

## Idee / Konzept

```
Tenant-A: 10.000 SELECT p99=2ms, 50 UPDATE p99=5ms → Fingerprint ≈ OLTP
Tenant-B: 5 SELECT p99=8s, alle auf aggregierten Views → Fingerprint ≈ OLAP
Tenant-C: 1 Bulk-INSERT 100k Rows alle 6h → Fingerprint ≈ BATCH

WorkloadFingerprintEngine.classify(tenant_stats) →
  WorkloadFingerprint{
    tenant_id: "A",
    pattern: OLTP,
    vector: [0.92, 0.05, 0.03],  // [OLTP, OLAP, BATCH] Wahrscheinlichkeiten
    confidence: 0.92,
    recommended_policy: {max_connections: 200, memory_limit: "4GB", priority: HIGH}
  }

→ TenantManager erhält Advisory → kann Policy hot-reload wenn DBA bestätigt
```

**Cross-Shard-Wert (Layer 11):** Wenn Shard A merkt dass Tenant X OLAP-Fingerprint
hat, kann Shard B sofort OLAP-optimierte Ressourcen zuweisen ohne eigenes Profiling.
Fingerprint-Vektoren werden via Gossip propagiert (DK-2 Adapter-Capability-Erweiterung).

## Technische Details

```cpp
// Neue Datei: include/server/workload_fingerprint_engine.h
class WorkloadFingerprintEngine {
public:
    enum class WorkloadPattern {
        OLTP,     // viele kleine Transaktionen
        OLAP,     // wenige schwere Queries
        BATCH,    // periodische Bulk-Operationen
        MIXED,    // keines der obigen dominiert
        UNKNOWN
    };

    struct WorkloadFingerprint {
        std::string tenant_id;
        WorkloadPattern pattern;
        std::vector<double> vector;        // Wahrscheinlichkeitsvektor [OLTP, OLAP, BATCH, MIXED]
        double confidence;
        struct PolicyRecommendation {
            uint32_t    max_connections;
            std::string memory_limit;      // "4GB" etc.
            std::string priority;          // "HIGH" | "MEDIUM" | "LOW"
            bool        suggest_read_replica;
        } recommended_policy;
    };

    // Klassifiziere Workload aus Tenant-Statistiken
    WorkloadFingerprint classify(
        const std::string& tenant_id,
        const TenantWorkloadStats& stats     // query_count, avg_p99_ms, write_ratio, batch_intervals
    ) const;

    // Ähnlichkeits-Match gegen bekannte Referenz-Profile
    double similarityTo(
        const WorkloadFingerprint& a,
        const WorkloadFingerprint& b
    ) const;   // Kosinus-Ähnlichkeit der vector-Felder
};
```

## Erfolgskriterien

- [ ] `WorkloadFingerprintEngine` Klasse vorhanden
- [ ] `classify()` für OLTP-Stats → `pattern = OLTP`, confidence > 0.8
- [ ] `classify()` für Bulk-INSERT-Stats → `pattern = BATCH`, confidence > 0.8
- [ ] `similarityTo()` gibt > 0.9 für zwei OLTP-Profile zurück
- [ ] `similarityTo()` gibt < 0.3 für OLTP vs BATCH zurück
- [ ] `recommended_policy` enthält sinnvolle Werte für jedes Pattern
- [ ] Schreibt `DecisionRecord{decision_type="WORKLOAD_FINGERPRINT"}` in `AIDecisionAuditor`
- [ ] `WorkloadFingerprint::vector` hat Dimension 4 (OLTP, OLAP, BATCH, MIXED)
- [ ] Layer-8-Performance: `classify()` ≤ 50 ms p99
- [ ] 8 neue Tests in `tests/test_workload_fingerprint_engine.cpp`

## Definition of Done

Drei Tenant-Profile (OLTP, OLAP, BATCH) werden korrekt klassifiziert.
`similarityTo()` trennt OLTP von BATCH mit Score < 0.3.
