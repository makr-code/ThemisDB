# FLARE-Loop mit TPT-Gating – Produktionsleitfaden (v1.4.0+)

> **Status:** ✅ Produktiv seit v1.4.0 · Standard-aktiviert · 37 Unit-Tests

---

## Überblick

**FLARE** (Feedback Loop Active Retrieval) kombiniert mit **TPT** (Token Perplexity
Threshold) Gating ist ab v1.4.0 **standardmäßig aktiviert**. Das System überwacht
während der LLM-Generierung fortlaufend die Token-Perplexität und löst bei Unsicherheit
automatisch eine erneute Retrieval-Runde aus.

```
Query
  ↓
Initiales Retrieval (semantische Suche)
  ↓
LLM generiert Antwort Satz für Satz
  ↓
TPT-Gate: Perplexität > Schwellenwert?
  ├─ NEIN → Weiter generieren (ausreichend Kontext)
  └─ JA   → FLARE: Query neu formulieren + Re-Retrieval
               └─ Bis zu 3 Runden
                  └─ Duplikate entfernen → Generierung fortsetzen
```

---

## Konfiguration

Die Standardwerte sind in `config/rag/default_production.yaml` dokumentiert:

| Parameter | Standardwert | Beschreibung |
|---|---|---|
| `enable_flare` | `true` | FLARE aktiv seit v1.4.0 |
| `perplexity_threshold` | `100.0` | TPT-Gate-Schwellenwert |
| `perplexity_window_size` | `10` | Gleitendes Fenster für lokale Analyse |
| `max_retrieval_rounds` | `3` | Maximale Re-Retrieval-Runden |
| `flare_confidence_threshold` | `0.5` | Minimale Konfidenz vor erneutem Retrieval |
| `enable_token_probability` | `true` | Token-Wahrscheinlichkeits-Tracking |
| `self_consistency_samples` | `5` | Anzahl Samples für Konsistenzprüfung |
| `consistency_threshold` | `0.6` | Minimaler Konsistenz-Score |

---

## Verwendung

### Produktionsmodus (Standard, v1.4.0+)

```cpp
// Option A: Standard-Konstruktor (FLARE automatisch aktiv)
KnowledgeGapDetector detector;

// Option B: Explizite Factory-Methode
auto detector = KnowledgeGapDetectorFactory::createProductionReady();

// FLARE-Loop ausführen
auto docs = getInitialDocuments(query);
auto result = detector->detectWithActiveRetrieval(query, docs);
```

### Legacy-Modus (v1.3-Kompatibilität)

```cpp
// FLARE explizit deaktivieren für v1.3-kompatibles Verhalten
auto detector = KnowledgeGapDetectorFactory::createLegacy();
```

---

## Performance

| Metrik | Wert |
|---|---|
| Token-Probability-Overhead | < 1 ms |
| Perplexitätsberechnung | < 5 ms |
| Selbstkonsistenz (5 Samples) | < 2 s |
| FLARE Re-Retrieval pro Runde | < 500 ms |

---

## Weiterführende Dokumente

- [Migration v1.3 → v1.4](../../migration/MIGRATION_v1.3_to_v1.4.md)
- [Knowledge Gap Detector Usage](../llm/RAG_KNOWLEDGE_GAP_DETECTOR_USAGE.md)
- [Phase-2-Implementierungshistorie](../../implementation-history/KNOWLEDGE_GAP_DETECTOR_PHASE2_COMPLETE.md)
