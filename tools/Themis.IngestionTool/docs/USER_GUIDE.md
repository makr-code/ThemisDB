> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Benutzerhandbuch

## Inhaltsverzeichnis
1. [Installation](#installation)
2. [Erste Schritte](#erste-schritte)
3. [Hauptfunktionen](#hauptfunktionen)
4. [Einstellungen](#einstellungen)
5. [Analyse-Ergebnisse](#analyse-ergebnisse)
6. [Fehlerbehebung](#fehlerbehebung)

## Installation

### Voraussetzungen
- **Betriebssystem**: Windows 10/11 (64-bit)
- **.NET Runtime**: 8.0 oder höher
- **Speicherplatz**: Min. 100 MB
- **RAM**: Min. 4 GB empfohlen
- **ThemisDB**: Optional (DryRun funktioniert ohne)

### Schritte
1. Laden Sie die neueste Version herunter
2. Entpacken Sie das ZIP-Archiv
3. Starten Sie `Themis.IngestionTool.exe`
4. Bei erster Ausführung: Einstellungen konfigurieren

## Erste Schritte

### 1. ThemisDB-Verbindung einrichten

#### Lokale Installation
Wenn ThemisDB lokal läuft:
- Standard-Port: `8765`
- Health-Check: `http://localhost:8765/health`

Status-Anzeige in der Statusbar:
- 🟢 **Online**: Grüner Punkt + "Online"
- 🔴 **Offline**: Roter Punkt + "Offline"

#### Docker-Container
```powershell
docker run -d --name themis-server -p 8765:18765 themisdb/themisdb:latest
```

Prüfen Sie die Verbindung:
```powershell
curl http://localhost:8765/health
```

### 2. Quellordner auswählen

1. Klicken Sie auf **"..."** neben dem Quellordner-Feld
2. Wählen Sie den Ordner mit den zu analysierenden Dateien
3. Der Pfad wird im Textfeld angezeigt
4. Unterordner werden automatisch durchsucht

**Unterstützte Dateitypen**:
- Code: `.cs`, `.java`, `.py`, `.js`, `.ts`, `.cpp`, `.h`
- Text: `.txt`, `.md`
- Daten: `.json`, `.xml`, `.yaml`, `.yml`
- Datenbank: `.sql`

### 3. DryRun aktivieren (optional)

**Was ist DryRun?**
- Vollständige Analyse OHNE Schreiben in ThemisDB
- Ideal zum Testen und Evaluieren
- Exportiert JSON-Ergebnisse zur späteren Verwendung
- Funktioniert auch ohne ThemisDB-Verbindung

**Wann DryRun nutzen?**
- ✅ Erste Tests mit neuen Ordnern
- ✅ Evaluierung der Analyse-Qualität
- ✅ Export für externe Tools
- ✅ Keine ThemisDB verfügbar

### 4. Ingestion starten

1. Klicken Sie auf **"Start"**
2. Pipeline startet automatisch:
   - Datei-Sammlung (Stage 1)
   - LLM-Initialisierung (Stage 2)
   - Analyse (Stage 3)
   - Abschluss (Stage 4)
3. Live-Metriken erscheinen rechts
4. Fortschrittsbalken zeigt Prozentsatz

**Während der Analyse**:
- Status-Updates in der Statusbar
- Stage-Beschreibung wird angezeigt
- Live-Ergebnisse füllen das DataGrid
- Fortschritt in Prozent

**Abbrechen**:
- Klicken Sie auf **"Stop"**
- Aktuelle Datei wird fertig analysiert
- Teilweise Ergebnisse werden gespeichert

## Hauptfunktionen

### Live-Metriken-Dashboard

Das DataGrid rechts zeigt Echtzeit-Analyse-Ergebnisse:

| Spalte | Bedeutung | Wertebereich |
|--------|-----------|--------------|
| **Datei** | Dateiname | - |
| **Relevanz** | Wichtigkeit/Relevanz | 0.00 - 1.00 |
| **Impact** | Einfluss auf System | 0.00 - 1.00 |
| **Qualität** | Code-Qualität | 0.00 - 1.00 |
| **Knoten** | Graph-Knoten (Klassen, Funktionen) | 0 - ∞ |
| **Bez.** | Beziehungen (Imports, Refs) | 0 - ∞ |
| **Sprache** | Erkannte Sprache | de, en, unknown |

### Score-Interpretationen

#### Relevanz-Score (0-1)
Bewertet die Wichtigkeit der Datei:
- **0.8 - 1.0**: Sehr relevant (Core-Komponenten)
- **0.6 - 0.8**: Relevant (Wichtige Features)
- **0.4 - 0.6**: Mittel (Support-Code)
- **0.0 - 0.4**: Niedrig (Config, Tests)

**Faktoren**:
- Dateilänge (mehr Code = relevanter)
- Struktur (Klassen/Funktionen)
- Keyword-Dichte

#### Impact-Score (0-1)
Bewertet den Einfluss auf das Gesamtsystem:
- **0.8 - 1.0**: Kritisch (zentrale Komponenten)
- **0.6 - 0.8**: Hoch (viele Abhängigkeiten)
- **0.4 - 0.6**: Mittel (normale Integration)
- **0.0 - 0.4**: Gering (isolierte Komponenten)

**Faktoren**:
- Anzahl Graph-Knoten
- Anzahl Beziehungen
- Code-Komplexität

#### Quality-Score (0-1)
Bewertet die Code-Qualität:
- **0.8 - 1.0**: Exzellent (gut dokumentiert)
- **0.6 - 0.8**: Gut (saubere Struktur)
- **0.4 - 0.6**: Akzeptabel (verbesserungswürdig)
- **0.0 - 0.4**: Schlecht (Refactoring nötig)

**Faktoren**:
- Kommentare
- Zeilenlänge (Lesbarkeit)
- Dateilänge (nicht zu lang/kurz)

### Graph-Metriken

#### Knoten-Zählung
Zählt strukturelle Elemente:
- **Klassen**: `class ClassName`
- **Funktionen**: `function funcName`, `def funcName`
- **Interfaces**: `interface IName`
- **Base**: Die Datei selbst (immer +1)

**Interpretation**:
- 1-5 Knoten: Kleine Komponente
- 6-20 Knoten: Normale Größe
- 21+: Große Komponente (ggf. aufteilen)

#### Beziehungs-Zählung
Zählt Verknüpfungen:
- **Imports**: `import x`, `using x`, `require(x)`
- **From-Imports**: `from x import y`
- **Vererbung**: (implicit)
- **Calls**: (implicit)

**Interpretation**:
- 0-3: Wenig gekoppelt (gut)
- 4-10: Normal gekoppelt
- 11+: Stark gekoppelt (ggf. Refactoring)

### NLP-Analyse

#### Keyword-Extraktion
Häufigste Wörter (min. 4 Zeichen):
- Top 10 Keywords pro Datei
- Lowercase-Normalisierung
- Stopword-Filterung (TODO)

#### Entity-Extraktion
Named Entities:
- Großgeschriebene Wörter
- Namen, Orte, Organisationen
- Top 15 Entities

#### Topic-Modelling
Automatische Kategorisierung:
- **Database**: SQL, database-Keyword
- **API**: api, http-Keywords
- **Architecture**: class, interface
- **Testing**: test, assert
- **Security**: security, auth
- **General**: Fallback

#### Sprach-Erkennung
Heuristisch:
- **de**: der, die, das, und, oder
- **en**: the, and, or, not, is
- **unknown**: Keine Treffer

## Einstellungen

Öffnen: **Menü → Bearbeiten → Einstellungen**

### Themis-Verbindung
- **Host**: IP oder Hostname (Standard: `localhost`)
- **Port**: Port-Nummer (Standard: `8765`)
- **Test**: Button zum Verbindungs-Test

### Ingestion-Optionen
- **Datenbank-Pfad**: SQLite-Tracking-DB (Standard: `ingestion_tracker.db`)
- **Max. Dateigröße**: Limit in MB (Standard: `100`)

### Metadaten-Extraktion
- ☑ **Vector-Metadaten**: Embeddings, Vektorsuche
- ☑ **Graph-Metadaten**: Knoten, Beziehungen
- ☑ **Relational-Metadaten**: Strukturierte Daten

**Empfehlung**: Alle aktivieren für vollständige Analyse

### Speichern
- **OK**: Speichert und schließt
- **Abbrechen**: Verwirft Änderungen

Einstellungen werden in `%AppData%\ThemisIngestionTool\appsettings.json` gespeichert.

## Analyse-Ergebnisse

### JSON-Export

Nach Abschluss wird eine JSON-Datei erstellt:
- **Location**: Neben dem Quellordner
- **Name**: `ingestion_output.json` (konfigurierbar)

**Struktur**:
```json
{
  "TotalFiles": 150,
  "ProcessedFiles": 145,
  "SkippedFiles": 0,
  "ErrorFiles": 3,
  "DuplicateFiles": 2,
  "TotalTime": "00:02:35",
  "IsDryRun": false,
  "Results": [
    {
      "FilePath": "C:\\...\\Program.cs",
      "FileName": "Program.cs",
      "FileSize": 5420,
      "FileType": ".cs",
      "ContentHash": "abc123...",
      "RelevanceScore": 0.85,
      "ImpactScore": 0.72,
      "QualityScore": 0.91,
      "GraphNodeCount": 12,
      "RelationshipCount": 8,
      "ExtractedEntities": ["ServiceProvider", "MainWindow"],
      "Keywords": ["service", "dependency", "injection"],
      "Topics": ["Architecture"],
      "Summary": "Main entry point...",
      "Language": "en",
      "Metadata": {
        "Size": "5420",
        "Extension": ".cs",
        "CreatedDate": "2026-01-01 10:00:00"
      },
      "AnalysisTimestamp": "2026-01-01T17:45:12",
      "ProcessingTime": "00:00:00.156",
      "IsProcessed": true,
      "IsDuplicate": false
    }
  ]
}
```

### Ergebnis-Zusammenfassung

Nach Abschluss erscheint ein Dialog mit:
- **Gesamt-Dateien**: Anzahl gefundener Dateien
- **Verarbeitet**: Erfolgreich analysiert
- **Duplikate**: Identische Dateien (SHA256)
- **Übersprungen**: Nicht-analysierbare Dateien
- **Fehler**: Fehlerhafte Dateien
- **Zeit**: Gesamtdauer (mm:ss)
- **Top 5 Dateien**: Nach Relevanz sortiert

### Duplikat-Erkennung

Dateien werden als Duplikate erkannt wenn:
- **SHA256-Hash identisch** ist
- Gleicher Inhalt, unterschiedlicher Pfad
- Erstes Vorkommen wird analysiert
- Folgende werden übersprungen

## Fehlerbehebung

### Themis zeigt "Offline"

**Ursachen**:
1. ThemisDB-Server läuft nicht
2. Falscher Port konfiguriert
3. Firewall blockiert Verbindung
4. Netzwerk-Problem

**Lösungen**:
```powershell
# 1. Prüfen Sie ob Server läuft
docker ps -a

# 2. Starten Sie ThemisDB
docker start themis-server

# 3. Testen Sie die Verbindung
curl http://localhost:8765/health

# 4. Prüfen Sie Einstellungen
# Menü → Einstellungen → Host/Port
```

### "Keine Dateien gefunden"

**Ursachen**:
1. Ordner ist leer
2. Keine unterstützten Dateitypen
3. Berechtigungen fehlen

**Lösungen**:
- Prüfen Sie Ordner-Inhalt
- Unterstützte Typen: .cs, .py, .js, .txt, .md, .json, .xml, .sql
- Ausführen Sie als Administrator

### Analyse bleibt hängen

**Ursachen**:
1. Sehr große Dateien
2. Netzwerk-Timeout
3. LLM-Service hängt

**Lösungen**:
- Klicken Sie "Stop"
- Reduzieren Sie Max. Dateigröße (Einstellungen)
- Nutzen Sie DryRun für Tests

### "Fehler bei Datei X"

**Ursachen**:
1. Datei ist gesperrt
2. Encoding-Probleme
3. Unerwartetes Format

**Lösungen**:
- Schließen Sie die Datei in anderen Programmen
- Prüfen Sie Datei-Encoding (UTF-8 empfohlen)
- Datei wird in JSON als ErrorFile markiert

### Live-Ergebnisse nicht sichtbar

**Ursachen**:
1. UI-Thread blockiert
2. Keine Dateien verarbeitet
3. Nur Duplikate gefunden

**Lösungen**:
- Warten Sie auf ersten verarbeiteten File
- Duplikate werden NICHT angezeigt
- Prüfen Sie JSON-Export für Details

## Tastenkürzel

| Kürzel | Aktion |
|--------|--------|
| `Strg+O` | Ordner auswählen |
| `Strg+S` | Einstellungen |
| `F5` | Start |
| `Esc` | Stop |
| `Alt+F4` | Beenden |

## Performance-Tipps

### Große Ordner (>1000 Dateien)
1. Nutzen Sie DryRun zuerst
2. Filtern Sie Unterordner
3. Erhöhen Sie Max. Dateigröße-Limit
4. Planen Sie 30-60 Minuten ein

### Optimale Settings
- Max. Dateigröße: 100 MB
- Alle Metadaten: Aktiviert
- DryRun: Für erste Tests

### System-Anforderungen für große Analysen
- RAM: 8 GB+
- CPU: Mehrkern (4+)
- SSD für bessere I/O
