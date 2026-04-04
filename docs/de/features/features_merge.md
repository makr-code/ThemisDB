# Three-Way Merge - Git-ähnliches Merging für MVCC

**Version:** 1.5.0  
**Kategorie:** 🔀 Transaktionsverwaltung  
**Status:** ✅ Implementiert (Schritt 5)

---

## 📑 Inhaltsverzeichnis

- [Überblick](#überblick)
- [Funktionen](#funktionen)
- [Three-Way-Merge-Algorithmus](#three-way-merge-algorithmus)
- [Konflikterkennung](#konflikterkennung)
- [Konfliktauflösung](#konfliktauflösung)
- [API-Referenz](#api-referenz)
- [Verwendungsbeispiele](#verwendungsbeispiele)
- [Integration](#integration)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Überblick

Die Three-Way-Merge-Engine bietet Git-ähnliche Merge-Funktionalität für ThemisDBs MVCC-System. Sie ermöglicht das Zusammenführen von Änderungen aus zwei divergierenden Branches oder Snapshots durch Analyse der Unterschiede von einem gemeinsamen Vorgänger und automatisches Auflösen oder Erkennen von Konflikten.

### Kernfähigkeiten

- **Three-Way Merge**: Änderungen aus zwei Branches mit gemeinsamem Basis zusammenführen
- **Konflikterkennung**: Automatische Erkennung überlappender Änderungen
- **Mehrere Strategien**: Auswahl zwischen ours, theirs, manual oder fast-forward
- **Dry-Run-Modus**: Vorschau der Merge-Ergebnisse ohne Anwendung
- **Tag-basiertes Merging**: Merging mit semantischen Snapshot-Tags
- **Integration**: Nahtlose Integration mit Snapshot- und Diff-Infrastruktur

### Anwendungsfälle

- **Multi-User-Schema-Migrationen**: Parallele Schema-Evolution zusammenführen
- **Verteilte Datenbank-Reconciliation**: Änderungen von verteilten Knoten reconcilieren
- **Branch-Merging**: Feature-Branches zurück zum Main zusammenführen
- **Konfliktauflösung**: Gleichzeitige Modifikationen systematisch handhaben
- **Entwicklungs-Workflows**: Git-ähnliche Workflows für Datenbankevolution

---

## Funktionen

### ✅ Implementiert

- Three-Way-Merge-Algorithmus (Basis → Quelle, Basis → Ziel)
- Automatische Konflikterkennung für überlappende Schlüssel
- Mehrere Merge-Strategien (ours, theirs, manual, fast-forward)
- Fast-Forward-Erkennung und -Optimierung
- Dry-Run/Vorschaumodus
- Tag-basierte Merge-Operationen
- Unterstützung für manuelle Konfliktauflösung
- Detaillierte Merge-Statistiken und Reporting
- REST-API-Endpunkte
- Integration mit Changefeed für Änderungsanwendung

### 🔜 Zukünftige Erweiterungen

- Interaktiver Merge-Modus (CLI)
- Benutzerdefinierte Merge-Funktionen für spezifische Datentypen
- Merge-History-Tracking
- Three-Way-Merge für strukturierte Daten (JSON-Merge)
- Partielle Merges (Filterung nach Tabelle/Schlüsselpräfix)

---

## Three-Way-Merge-Algorithmus

Der Three-Way-Merge-Algorithmus vergleicht Änderungen von einem gemeinsamen Vorgänger (Basis) zu zwei divergierenden Zuständen (Quelle und Ziel):

```
        Basis (Sequenz 100)
         /              \
        /                \
Quelle (Seq 150)    Ziel (Seq 200)
       \                 /
        \               /
      Gemergt (Seq 250)
```

### Algorithmus-Schritte

1. **Diffs berechnen**: Änderungen von Basis zu Quelle und Basis zu Ziel berechnen
2. **Konflikte erkennen**: Schlüssel finden, die in beiden Branches geändert wurden
3. **Konflikte klassifizieren**: Konflikttyp bestimmen (modify-modify, delete-modify, etc.)
4. **Konflikte auflösen**: Auflösungsstrategie anwenden (automatisch oder manuell)
5. **Nicht-konfliktbehaftete Änderungen zusammenführen**: Änderungen von Quelle anwenden, die nicht konfliktieren
6. **Änderungen anwenden**: Zusammengeführte Änderungen in Datenbank schreiben (wenn nicht Dry-Run)

### Fast-Forward-Optimierung

Wenn Ziel keine Änderungen von Basis hat, kann der Merge durch einfaches Anwenden aller Quell-Änderungen fast-forward durchgeführt werden:

```
Basis → Ziel (keine Änderungen)
Basis → Quelle (Änderungen existieren)
Ergebnis: Fast-Forward zu Quelle
```

---

## Konflikterkennung

### Konflikttypen

1. **MODIFY_MODIFY**: Beide Branches haben denselben Schlüssel geändert
   - Beispiel: Quelle setzt `users:1 = "Alice Aktualisiert"`, Ziel setzt `users:1 = "Alice Geändert"`

2. **DELETE_MODIFY**: Quelle gelöscht, Ziel geändert
   - Beispiel: Quelle löscht `users:1`, Ziel setzt `users:1 = "Alice Aktualisiert"`

3. **MODIFY_DELETE**: Quelle geändert, Ziel gelöscht
   - Beispiel: Quelle setzt `users:1 = "Alice Aktualisiert"`, Ziel löscht `users:1`

4. **DELETE_DELETE**: Beide gelöscht (automatisch auflösbar)
   - Beispiel: Beide Branches löschen `users:1` → Automatisch als Löschung aufgelöst

### Automatisch auflösbare Konflikte

- **DELETE_DELETE**: Beide Branches stimmen über Löschung überein
- **Identische Änderungen**: Beide Branches haben exakt die gleiche Änderung vorgenommen

---

## Konfliktauflösung

### Strategien

#### 1. Manual (Standard)

Erfordert explizite Auflösung für jeden Konflikt. Am besten für kritische Daten.

```json
{
  "strategy": "manual",
  "manual_resolutions": [
    {"key": "users:1", "resolved_value": "Alice Final"}
  ]
}
```

#### 2. Ours

Bevorzugt Ziel-Branch-Änderungen. Nützlich, wenn Ziel autoritativ ist.

```json
{
  "strategy": "ours"
}
```

#### 3. Theirs

Bevorzugt Quell-Branch-Änderungen. Nützlich, wenn Quelle Priorität hat.

```json
{
  "strategy": "theirs"
}
```

#### 4. Fast-Forward

Nur mergen, wenn keine Konflikte existieren. Scheitert bei jedem Konflikt.

```json
{
  "strategy": "fast_forward",
  "fail_on_conflict": true
}
```

---

## API-Referenz

### Endpunkte

#### POST /api/v1/merge

Führt Three-Way-Merge nach Sequenznummern durch.

**Request Body:**

```json
{
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 200,
  "strategy": "manual",
  "fail_on_conflict": false,
  "manual_resolutions": [
    {
      "key": "users:1",
      "resolved_value": "Alice Final"
    }
  ]
}
```

**Response (200 OK):**

```json
{
  "success": true,
  "message": "Merge erfolgreich: 5 Änderungen angewendet, 1 Konflikte aufgelöst",
  "stats": {
    "changes_applied": 5,
    "conflicts_detected": 1,
    "conflicts_auto_resolved": 0,
    "conflicts_manual": 1,
    "has_conflicts": true,
    "is_fast_forward": false
  },
  "conflicts": [],
  "changes_applied": [...],
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 200,
  "result_sequence": 250
}
```

**Error Response (400 Bad Request):**

```json
{
  "success": false,
  "error": "Merge erfordert manuelle Konfliktauflösung"
}
```

---

#### POST /api/v1/merge/preview

Vorschau des Merge ohne Anwendung der Änderungen (Dry-Run).

**Request Body:**

```json
{
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 200
}
```

**Response:** Gleiche Struktur wie Merge, aber keine Änderungen angewendet.

---

#### POST /api/v1/merge/by-tag

Führt Merge mit Snapshot-Tags durch.

**Request Body:**

```json
{
  "base_tag": "v1.0.0",
  "source_tag": "feature-branch",
  "target_tag": "current",
  "strategy": "theirs"
}
```

**Spezielle Tags:**
- `current` oder `HEAD`: Verwendet neueste Sequenz

**Response:** Gleich wie Merge-Endpunkt.

---

#### GET /api/v1/merge/can-fast-forward

Prüft, ob Fast-Forward-Merge möglich ist.

**Query-Parameter:**
- `base_sequence`: Basis-Sequenznummer
- `source_sequence`: Quell-Sequenznummer
- `target_sequence`: Ziel-Sequenznummer

**Response:**

```json
{
  "can_fast_forward": true,
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 100
}
```

---

## Verwendungsbeispiele

### Beispiel 1: Einfacher Three-Way-Merge

```bash
curl -X POST http://localhost:8080/api/v1/merge \
  -H "Content-Type: application/json" \
  -d '{
    "base_sequence": 1000,
    "source_sequence": 1100,
    "target_sequence": 1200,
    "strategy": "theirs"
  }'
```

### Beispiel 2: Merge mit manueller Konfliktauflösung

```bash
# Zuerst Vorschau, um Konflikte zu sehen
curl -X POST http://localhost:8080/api/v1/merge/preview \
  -H "Content-Type: application/json" \
  -d '{
    "base_sequence": 1000,
    "source_sequence": 1100,
    "target_sequence": 1200
  }'

# Dann Merge mit Auflösungen
curl -X POST http://localhost:8080/api/v1/merge \
  -H "Content-Type: application/json" \
  -d '{
    "base_sequence": 1000,
    "source_sequence": 1100,
    "target_sequence": 1200,
    "strategy": "manual",
    "manual_resolutions": [
      {
        "key": "users:alice",
        "resolved_value": "{\"name\":\"Alice Schmidt\",\"role\":\"admin\"}"
      }
    ]
  }'
```

### Beispiel 3: Tag-basierter Merge

```bash
curl -X POST http://localhost:8080/api/v1/merge/by-tag \
  -H "Content-Type: application/json" \
  -d '{
    "base_tag": "v1.0.0",
    "source_tag": "feature-neue-auth",
    "target_tag": "current",
    "strategy": "fast_forward",
    "fail_on_conflict": true
  }'
```

### Beispiel 4: Fast-Forward prüfen

```bash
curl "http://localhost:8080/api/v1/merge/can-fast-forward?base_sequence=1000&source_sequence=1100&target_sequence=1000"
```

---

## Integration

### Mit Snapshot Manager

Merge-Engine integriert mit SnapshotManager für Tag-basierte Operationen:

```cpp
// Tags für Merge erstellen
snapshot_manager.createTag("basis", "Gemeinsamer Vorgänger");
snapshot_manager.createTag("feature", "Feature-Branch");

// Merge nach Tags
auto result = merge_engine.mergeByTag("basis", "feature", "current", options);
```

### Mit Diff Engine

Merge-Engine verwendet DiffEngine zur Berechnung von Änderungen:

```cpp
// Diffs werden automatisch berechnet
auto source_diff = diff_engine.computeDiff(base_seq, source_seq);
auto target_diff = diff_engine.computeDiff(base_seq, target_seq);

// Konflikte durch Vergleich der Diffs erkannt
auto conflicts = detectConflicts(source_diff, target_diff);
```

### Mit Changefeed

Angewendete Änderungen werden im Changefeed aufgezeichnet:

```cpp
// Jede zusammengeführte Änderung erzeugt ein Changefeed-Event
for (const auto& change : merged_changes) {
    changefeed.recordEvent(event); // Mit Merge-Metadaten aufgezeichnet
}
```

---

## Performance

### Benchmarks

| Operation | Ziel | Gemessen |
|-----------|------|----------|
| Merge 1K Änderungen (keine Konflikte) | <100ms | 85ms |
| Merge 10K Änderungen (keine Konflikte) | <1s | 890ms |
| Konflikterkennung (1K Schlüssel) | <50ms | 42ms |
| Fast-Forward-Prüfung | <10ms | 5ms |

### Optimierungstipps

1. **Fast-Forward verwenden wenn möglich**: Zuerst mit `canFastForward()` prüfen
2. **Diff-Bereich begrenzen**: Kleinere Sequenzbereiche = schnellere Merges
3. **Auflösungen batchen**: Alle manuellen Auflösungen im Voraus bereitstellen
4. **Änderungen filtern**: Tabellen-/Schlüsselfilter verwenden, um Merge-Umfang zu reduzieren (zukünftig)

---

## Best Practices

### 1. Immer zuerst Vorschau

```bash
# Vorschau vor tatsächlichem Merge
POST /api/v1/merge/preview
# Konflikte überprüfen
POST /api/v1/merge mit Auflösungen
```

### 2. Geeignete Strategie verwenden

- **Kritische Daten**: `manual`-Strategie verwenden
- **Feature-Branches**: `theirs` verwenden, um neue Features zu bevorzugen
- **Hotfixes**: `ours` verwenden, um Produktionsstatus zu erhalten
- **Sichere Merges**: `fast_forward` verwenden, um bei Konflikten zu scheitern

### 3. Tags vor Merge erstellen

```bash
# Aktuellen Zustand vor Merge taggen
POST /api/v1/snapshots/tags {"tag_name": "pre-merge-backup"}

# Merge durchführen
POST /api/v1/merge/by-tag

# Bei Bedarf zurückrollen
POST /api/v1/restore {"tag_name": "pre-merge-backup"}
```

### 4. Merge-Operationen überwachen

```yaml
# Prometheus-Metriken
themis_merge_operations_total
themis_merge_duration_seconds
themis_merge_conflicts_total
themis_merge_conflicts_resolved_total
```

### 5. Konflikte systematisch handhaben

- Konflikttypen sorgfältig überprüfen
- Automatische Auflösung bevorzugen, wenn sicher
- Manuelle Auflösungsentscheidungen dokumentieren
- Zusammengeführte Ergebnisse gründlich testen

---

## Fehlerbehandlung

### Häufige Fehler

**Tag nicht gefunden:**
```json
{
  "success": false,
  "message": "Quell-Tag nicht gefunden: feature-branch"
}
```

**Unaufgelöste Konflikte:**
```json
{
  "success": false,
  "message": "Merge erfordert manuelle Konfliktauflösung"
}
```

**Fast-Forward fehlgeschlagen:**
```json
{
  "success": false,
  "message": "Merge abgebrochen: 3 Konflikte erkannt"
}
```

---

## Sicherheitsaspekte

1. **Autorisierung**: Merge-Operationen erfordern erhöhte Berechtigungen
2. **Audit-Trail**: Alle Merges mit Benutzer, Zeitstempel, Strategie protokolliert
3. **Backup**: Immer vor Merge sichern (Auto-Backup empfohlen)
4. **Validierung**: Konfliktauflösungen vor Anwendung validieren
5. **Rate-Limiting**: Merge-API-Aufrufe begrenzen, um Missbrauch zu verhindern

---

## Referenzen

- [Named Snapshots Dokumentation](features_snapshots.md)
- [Diff API Dokumentation](features_diff.md)
- [MVCC Tuning Guide](../MVCC_TUNING_GUIDE.md)
- [Transaction Best Practices](TRANSACTION_BEST_PRACTICES.md)

---

**Erstellt:** Januar 2026  
**Zuletzt aktualisiert:** Februar 2026  
**Version:** 1.5.0  
**Autor:** ThemisDB Development Team
