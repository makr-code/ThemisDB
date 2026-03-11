# Geplante Semantische Graph-Kanten-Aktualisierung

**Modul:** `graph/scheduled_edge_refresh`  
**Version:** 0.1.0  
**Status:** 🚧 Beta  
**Issue:** #FEATURE/ScheduledGraphEdgeRefresh  
**Ziel-Meilenstein:** Q4 2026

---

## Überblick

Das Modul **Scheduled Graph Edge Refresh** ermöglicht die automatische, richtliniengesteuerte Pflege von Graph-Kanten in ThemisDB. Es bewertet Kanten regelmäßig anhand einer Kombination aus:

- **Vektorähnlichkeit** (Kosinus, Skalarprodukt oder euklidische Distanz zwischen Knoten-Einbettungen)
- **Zeitlichem Verfall** (exponentielles Halbwertszeit-Modell auf das Kantenalter angewendet)
- **Zentralitätsgewichtung** (gradbasierte Dämpfung zur Vermeidung von Hub-Überrepräsentation)

Kanten mit niedriger Relevanz werden entfernt und neue Kanten mit hoher Ähnlichkeit entdeckt, sodass der Graph semantisch aktuell bleibt — ohne manuelle Eingriffe.

### Anwendungsfälle

| Domäne | Nutzen |
|--------|--------|
| Wissensgraphen | Semantisch verwandte Entitäten dynamisch verknüpfen; veraltete Verbindungen entfernen |
| Soziale Graphen | Verbindungen auf Basis sich ändernder Nutzerdaten anreichern; inaktive Links abklingen lassen |
| ML-Feature-/Embedding-Graphen | Verteilte Repräsentationen mit der Datenentwicklung aktuell halten |

---

## Architektur

```
┌─────────────────────────────────────────────────────────────────────┐
│  ScheduledGraphEdgeRefreshEngine                                    │
│                                                                     │
│  ┌─────────────┐   ┌──────────────────┐   ┌───────────────────┐   │
│  │  Scheduler  │──▶│  runRefreshCycle │──▶│  scoreAllEdges    │   │
│  │  Thread     │   │                  │   │  (Ähnl. + Verfall │   │
│  └─────────────┘   │  collectEdges()  │   │   + Zentralität)  │   │
│                    │                  │   └───────────────────┘   │
│  triggerRefresh()──▶  discoverCand.() │         │                  │
│                    │                  │   ┌──────▼──────────────┐  │
│                    │  applyBatch()    │◀──│  Sicherheitssperren │  │
│                    │  (ACID-Commit)   │   │  (max_removal_frac) │  │
│                    └──────────────────┘   └─────────────────────┘  │
│                           │                                         │
│                    ┌──────▼──────────────────────────────────────┐ │
│                    │  Prüfspur (im Speicher, begrenzter Ring)    │ │
│                    └─────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
         │                    │
         ▼                    ▼
  GraphIndexManager    NodeEmbeddingProvider
  (ACID-WriteBatch)    (Benutzerdefinierter Callback)
```

---

## Konfiguration: `RefreshPolicy`

| Feld | Typ | Standard | Beschreibung |
|------|-----|----------|--------------|
| `refresh_interval` | `std::chrono::seconds` | 3600 | Intervall zwischen automatischen Zyklen. 0 = nur manueller Modus. |
| `similarity_metric` | `SimilarityMetric` | `COSINE` | Ähnlichkeitsmetrik für Knoten-Einbettungsvektoren. |
| `relevance_threshold` | `float` [0,1] | 0.5 | Kanten mit `relevance < threshold` sind Löschkandidaten. |
| `add_threshold` | `float` [0,1] | 0.7 | Mindeständlichkeit für neue Kanten. |
| `top_k_candidates` | `uint32_t` | 10 | Top-k ähnlichste Nachbarn pro Knoten bei der Entdeckung. |
| `decay_half_life_seconds` | `double` | 86400 | Halbwertszeit für den zeitlichen Verfall (Sekunden). 0 = deaktiviert. |
| `max_removal_fraction` | `float` [0,1] | 0.10 | **Sicherheitssperre**: maximaler Anteil löschbarer Kanten pro Zyklus. |
| `max_edges_to_add` | `uint32_t` | 1000 | Maximale Anzahl hinzuzufügender Kanten pro Zyklus (0 = unbegrenzt). |
| `max_edges_to_remove` | `uint32_t` | 500 | Maximale Anzahl zu löschender Kanten pro Zyklus (0 = unbegrenzt). |
| `graph_id` | `std::string` | `""` | Refresh auf einen bestimmten Graphen einschränken. Leer = alle Graphen. |

### Validierung

Alle Felder werden beim Erstellen des Engines geprüft. Ungültige Werte werfen `std::invalid_argument`.

---

## Bewertungsmodell

Die Relevanz jeder Kante wird berechnet als:

```
relevance = similarity × temporal_factor × centrality_weight
```

### Ähnlichkeit

Berechnet zwischen den Einbettungsvektoren von Quell- und Zielknoten der Kante:

| Metrik | Formel | Bereich |
|--------|--------|---------|
| `COSINE` | `(cos(a,b) + 1) / 2` | [0, 1] |
| `DOT_PRODUCT` | `dot(a,b) / (‖a‖ · ‖b‖)`, auf [0,1] normiert | [0, 1] |
| `EUCLIDEAN` | `1 / (1 + dist(a,b))` | (0, 1] |

### Zeitlicher Verfall

Exponentieller Verfall basierend auf dem Kantenalter:

```
temporal_factor = 2^(−Alter / Halbwertszeit)
```

Das Alter wird aus dem Feld `_created_at` der Kante gelesen (Sekunden seit Epoch). Fehlt das Feld oder ist `decay_half_life_seconds = 0`, gilt `temporal_factor = 1.0` (kein Verfall).

### Zentralitätsgewichtung

Proportional zum inversen Log-Grad des Quellknotens:

```
centrality_weight = 1 / (1 + log(1 + Ausgangsgrad))
```

Dies dämpft Bewertungen für Hub-Knoten und verhindert, dass stark vernetzte Knoten den Refresh dominieren.

---

## Refresh-Zyklus

Jeder Zyklus folgt diesen Schritten:

1. **Kanten sammeln** — Alle Kanten aufzählen (ggf. nach `graph_id` gefiltert).
2. **Kanten bewerten** — `relevance` für jede Kante berechnen.
3. **Löschkandidaten bestimmen** — Kanten mit `relevance < relevance_threshold`.
4. **Sicherheitssperre prüfen** — Falls `|Kandidaten| / |Gesamtkanten| > max_removal_fraction`, Abbruch mit `aborted_safety_gate = true`.
5. **Neue Kandidaten entdecken** — Für jeden Knoten die top-k ähnlichsten Nachbarn finden. Bereits vorhandene Kanten und solche unterhalb von `add_threshold` herausfiltern.
6. **ACID-Batch anwenden** — Alle Löschungen und Einfügungen werden atomar über einen `WriteBatchWrapper` übermittelt. Bei Commit-Fehler wird kein teilweise Zustand übernommen.
7. **Prüfspur aktualisieren** — Ein `RefreshAuditEntry` pro Mutation.
8. **Statistiken aktualisieren** — `RefreshStats` wird atomar aktualisiert.

---

## ACID-Garantien

Alle Kantenmutationen eines Refresh-Zyklus werden als einzelne `RocksDBWrapper::WriteBatchWrapper` übermittelt. Bei einem Commit-Fehler werden keine partiellen Mutationen angewendet. Wenn eine Sicherheitssperre greift, wird der Batch nie übermittelt — der Graph bleibt unverändert.

---

## Prüfspur

Jede Kantenmutation wird als `RefreshAuditEntry` protokolliert:

```cpp
struct RefreshAuditEntry {
    enum class Action { ADD, REMOVE };
    Action  action;
    std::string edge_id;
    std::string from_vertex;
    std::string to_vertex;
    float   relevance_score;
    std::chrono::system_clock::time_point timestamp;
    uint64_t cycle_number;
};
```

Die Prüfspur ist auf **10.000 Einträge** begrenzt (älteste Einträge werden verdrängt). Zugriff über `getAuditTrail()`.

---

## API-Referenz

### Konstruktor

```cpp
ScheduledGraphEdgeRefreshEngine(
    GraphIndexManager& graph_mgr,
    const RefreshPolicy& policy,
    NodeEmbeddingProvider embedding_fn = nullptr);
```

- `graph_mgr` — Muss den Engine überleben.
- `policy` — Wird beim Erstellen validiert; wirft `std::invalid_argument` bei Fehler.
- `embedding_fn` — Optional. Wenn null, wird die Ähnlichkeitsbewertung übersprungen und `similarity = 1.0` verwendet.

### Lebenszyklus

```cpp
void start();  // Hintergrund-Scheduler-Thread starten
void stop();   // Scheduler stoppen und Thread joinen (idempotent)
```

### Manueller Trigger

```cpp
RefreshStats triggerRefresh();  // Zyklus synchron ausführen (thread-sicher)
```

### Beobachtung

```cpp
RefreshStats                   getStats()       const;  // Statistiken des letzten Zyklus
std::vector<RefreshAuditEntry> getAuditTrail()  const;  // Mutationsprotokoll
const RefreshPolicy&           getPolicy()      const;  // Aktuelle Richtlinie
```

### Richtlinie zur Laufzeit aktualisieren

```cpp
void setPolicy(const RefreshPolicy& policy);  // Wirkt beim nächsten Zyklus
```

### Bewertungs-Hilfsfunktionen (testbar)

```cpp
float computeSimilarity(const std::vector<float>& a, const std::vector<float>& b) const;
float computeTemporalDecay(const BaseEntity& edge_entity) const;
EdgeScore scoreEdge(const BaseEntity& edge_entity) const;
```

---

## Verwendungsbeispiel

```cpp
#include "graph/scheduled_edge_refresh.h"
#include "index/graph_index.h"

// 1. Richtlinie konfigurieren
themis::graph::RefreshPolicy policy;
policy.refresh_interval        = std::chrono::seconds(300);  // alle 5 Minuten
policy.similarity_metric       = themis::graph::SimilarityMetric::COSINE;
policy.relevance_threshold     = 0.4f;
policy.add_threshold           = 0.75f;
policy.decay_half_life_seconds = 86400.0;  // 1 Tag
policy.max_removal_fraction    = 0.05f;    // max. 5% Löschungen pro Zyklus
policy.top_k_candidates        = 20;

// 2. Einbettungs-Provider (mit GNN-Index oder In-Memory-Cache verdrahten)
themis::graph::NodeEmbeddingProvider embedding_fn =
    [&](const std::string& node_id) -> std::vector<float> {
        return mein_gnn_index.getEmbedding(node_id);
    };

// 3. Engine erstellen und starten
themis::graph::ScheduledGraphEdgeRefreshEngine engine(
    graph_manager, policy, embedding_fn);
engine.start();

// 4. Manueller Trigger (optional, für Tests oder erzwungene Aktualisierung)
auto stats = engine.triggerRefresh();
spdlog::info("Entfernt: {} Kanten, Hinzugefügt: {} Kanten in {:.2f}ms",
             stats.edges_removed, stats.edges_added, stats.cycle_duration_ms);

// 5. Prüfspur auslesen
for (const auto& entry : engine.getAuditTrail()) {
    spdlog::info("[{}] Kante {} ({} → {})",
        entry.action == themis::graph::RefreshAuditEntry::Action::ADD ? "ADD" : "REMOVE",
        entry.edge_id, entry.from_vertex, entry.to_vertex);
}

// 6. Ordentliches Herunterfahren
engine.stop();
```

---

## Integrationspunkte

| Modul | Integration |
|-------|-------------|
| `index/graph_index.h` | Kanten-CRUD, Adjazenzabfragen, Vertex-Aufzählung |
| `acceleration` | `NodeEmbeddingProvider` auf GNN/HNSW-Index stützen |
| `analytics/cep_engine` | Kantenmutations-Ereignisse in CEP-Streams einleiten |
| `cdc/changefeed` | Prüfspur-Einträge an ChangeFeed für nachgelagerte Konsumenten weiterleiten |
| `temporal_graph` | Feld `_created_at` wird vom Temporal-Graph-Modul gesetzt |

---

## Leistungshinweise

- **Brute-Force-Ähnlichkeitssuche** wird für die Kandidatenentdeckung verwendet. Bei Graphen mit >10.000 Knoten sollte ein ANN-Index (HNSW über das `acceleration`-Modul) eingesetzt werden, um die Entdeckungszeit sublinear zu halten.
- **Batch-Schreibvorgänge** amortisieren die RocksDB-Schreibverstärkung.
- **Zentralitätsdämpfung** reduziert unnötige Fluktuation bei Hub-Knoten.
- Der Hintergrund-Scheduler führt keine parallelen Zyklen aus; jeder Zyklus hält `cycle_mutex_` für seine gesamte Laufzeit.

---

## Referenzen

- Yu et al. (2017) — STGCN: Spatio-Temporal Graph Convolutional Networks
- Leskovec et al. (2008) — Microscopic Evolution of Social Networks
- Brandes (2008) — On Variants of Shortest-Path Betweenness Centrality
- ThemisDB `analytics/docs/gnn_embeddings.md`
- ThemisDB `analytics/cep_engine.cpp`
