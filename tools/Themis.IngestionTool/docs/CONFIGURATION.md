> **Hinweis:** Konfigurationsoptionen gegen aktuelle config-Dateien und Source verifizieren.

# Konfigurations-Dokumentation

## Übersicht

Das Themis Ingestion Tool nutzt mehrere Konfigurationsquellen für flexible Anpassung an verschiedene Umgebungen.

## Konfigurations-Dateien

### appsettings.json

**Location**: `%AppData%\ThemisIngestionTool\appsettings.json`

**Struktur**:
```json
{
  "ThemisHost": "localhost",
  "ThemisPort": 8765,
  "DatabasePath": "ingestion_tracker.db",
  "MaxFileSize": 100,
  "EnableVectorMetadata": true,
  "EnableGraphMetadata": true,
  "EnableRelationalMetadata": true,
  "LastSourceFolder": "C:\\src\\myproject",
  "LastOutputFile": "ingestion_output.json"
}
```

**Eigenschaften**:

| Property | Type | Default | Beschreibung |
|----------|------|---------|--------------|
| `ThemisHost` | string | "localhost" | ThemisDB Server Hostname/IP |
| `ThemisPort` | int | 8765 | ThemisDB Server Port |
| `DatabasePath` | string | "ingestion_tracker.db" | SQLite-Tracking-Datenbank |
| `MaxFileSize` | int | 100 | Max. Dateigröße in MB |
| `EnableVectorMetadata` | bool | true | Vector-Embeddings aktivieren |
| `EnableGraphMetadata` | bool | true | Graph-Strukturen extrahieren |
| `EnableRelationalMetadata` | bool | true | Relationale Metadaten |
| `LastSourceFolder` | string | "" | Letzter Quellordner (auto-saved) |
| `LastOutputFile` | string | "ingestion_output.json" | Letzte Ausgabedatei |

### Manuelle Bearbeitung

```powershell
# Öffne appsettings.json im Editor
notepad "$env:APPDATA\ThemisIngestionTool\appsettings.json"
```

**Beispiel-Konfigurationen**:

#### Lokale Entwicklung
```json
{
  "ThemisHost": "localhost",
  "ThemisPort": 8765,
  "MaxFileSize": 50,
  "EnableVectorMetadata": true,
  "EnableGraphMetadata": true,
  "EnableRelationalMetadata": true
}
```

#### Produktions-Server
```json
{
  "ThemisHost": "themis-prod.company.com",
  "ThemisPort": 8765,
  "MaxFileSize": 200,
  "EnableVectorMetadata": true,
  "EnableGraphMetadata": true,
  "EnableRelationalMetadata": true
}
```

#### Docker-Container
```json
{
  "ThemisHost": "172.17.0.2",
  "ThemisPort": 18765,
  "MaxFileSize": 100,
  "EnableVectorMetadata": true,
  "EnableGraphMetadata": false,
  "EnableRelationalMetadata": true
}
```

## Einstellungs-Dialog

### Zugriff
**Menü → Bearbeiten → Einstellungen** oder `Strg+S`

### Felder

#### Themis-Verbindung

**Host**:
- Format: Hostname oder IP-Adresse
- Beispiele: `localhost`, `192.168.1.100`, `themis.local`
- Validation: Nicht leer

**Port**:
- Format: Integer 1-65535
- Standard: 8765
- Docker-Mapping beachten (z.B. 8765:18765)

**Test-Button**:
- Führt sofortigen Connection-Test aus
- Zeigt Erfolg/Fehler im Dialog
- Nutzt `/health` Endpoint

#### Ingestion

**Datenbank-Pfad**:
- SQLite-Datei für Tracking
- Relative oder absolute Pfade
- Wird automatisch erstellt
- Default: `ingestion_tracker.db` im AppData

**Max. Dateigröße (MB)**:
- Limit für Datei-Analyse
- Größere Dateien werden übersprungen
- Empfehlung: 50-200 MB je nach RAM

#### Metadaten-Extraktion

**☑ Vector-Metadaten aktivieren**:
- Embedding-Generierung
- Vector-Store-Integration
- Performance-Impact: Mittel
- Empfehlung: Aktiviert

**☑ Graph-Metadaten aktivieren**:
- Code-Struktur-Analyse
- Beziehungs-Erkennung
- Performance-Impact: Gering
- Empfehlung: Aktiviert

**☑ Relational-Metadaten aktivieren**:
- Entity-Properties
- Strukturierte Daten
- Performance-Impact: Gering
- Empfehlung: Aktiviert

### Speichern & Validierung

**OK-Button**:
- Validiert alle Felder
- Speichert in appsettings.json
- Schließt Dialog

**Validierungs-Regeln**:
```csharp
// Host: Nicht leer
if (string.IsNullOrWhiteSpace(ThemisHost))
    throw new ValidationException("Host ist erforderlich");

// Port: 1-65535
if (ThemisPort < 1 || ThemisPort > 65535)
    throw new ValidationException("Port muss zwischen 1 und 65535 liegen");

// MaxFileSize: > 0
if (MaxFileSize <= 0)
    throw new ValidationException("Max. Dateigröße muss größer 0 sein");
```

## Umgebungsvariablen (TODO)

Für Container-Deployments:

```bash
# Docker
docker run -e THEMIS_HOST=themis-server \
           -e THEMIS_PORT=8765 \
           -e MAX_FILE_SIZE=150 \
           themis-ingestion-tool

# Kubernetes ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingestion-config
data:
  THEMIS_HOST: "themis-service"
  THEMIS_PORT: "8765"
  MAX_FILE_SIZE: "100"
```

## Kommandozeilen-Parameter (TODO)

```powershell
# Geplante CLI-Optionen
Themis.IngestionTool.exe `
    --host themis.local `
    --port 8765 `
    --source "C:\src\project" `
    --output "results.json" `
    --dry-run `
    --max-size 150
```

## Erweiterte Konfiguration

### LLM-Service-Konfiguration

**TODO**: Eigene Config-Section für LLM

```json
{
  "LlamaService": {
    "Endpoint": "http://localhost:11434",
    "Model": "llama2",
    "MaxTokens": 200,
    "Temperature": 0.7,
    "Timeout": 30000
  }
}
```

### Pipeline-Tuning

**TODO**: Performance-Optimierungen konfigurierbar

```json
{
  "Pipeline": {
    "MaxParallelFiles": 4,
    "EnableCaching": true,
    "CacheSize": 1000,
    "ChunkSize": 100,
    "EnableBatching": true
  }
}
```

### Logging

**TODO**: Strukturiertes Logging

```json
{
  "Logging": {
    "LogLevel": {
      "Default": "Information",
      "Themis.IngestionTool": "Debug"
    },
    "File": {
      "Path": "logs/ingestion_{Date}.log",
      "RollingInterval": "Day",
      "RetainedFileCountLimit": 7
    }
  }
}
```

### Themis-API-Konfiguration

**TODO**: Multi-Model-Optionen

```json
{
  "ThemisApi": {
    "Host": "localhost",
    "Port": 8765,
    "Timeout": 30000,
    "RetryCount": 3,
    "RetryDelay": 1000,
    "EnableCircuitBreaker": true,
    "Features": {
      "UseTransactions": true,
      "UseBatchOperations": true,
      "StoreContentFS": false,
      "UseTimeSeriesTracking": true
    },
    "VectorStore": {
      "ObjectName": "documents",
      "Dimension": 1536,
      "Metric": "COSINE"
    },
    "GraphOptions": {
      "CreateHierarchy": true,
      "CreateImportRelations": true,
      "CreateCallGraph": false
    }
  }
}
```

## Migrations

### v1.0 → v2.0

Wenn neue Config-Felder hinzugefügt werden:

```csharp
public class SettingsService : ISettingsService
{
    public AppSettings LoadSettings()
    {
        var json = File.ReadAllText(_settingsPath);
        var settings = JsonSerializer.Deserialize<AppSettings>(json);
        
        // Migration: Neue Felder mit Defaults
        if (settings.MaxFileSize == 0)
            settings.MaxFileSize = 100;
        
        if (string.IsNullOrEmpty(settings.ThemisHost))
            settings.ThemisHost = "localhost";
        
        return settings;
    }
}
```

## Best Practices

### Sicherheit

1. **Keine Passwörter in appsettings.json**
   - Nutzen Sie Credential Manager
   - Oder Azure Key Vault
   - Oder Environment Variables

2. **Verschlüsselte Verbindungen**
   - HTTPS für Produktion
   - TLS-Zertifikate validieren

3. **Least Privilege**
   - Nur benötigte Features aktivieren
   - Read-Only-Access wenn möglich

### Performance

1. **MaxFileSize optimieren**
   - Große Projekte: 50-100 MB
   - Kleine Projekte: 10-20 MB
   - Server: 200+ MB

2. **Metadaten selektiv**
   - Vector: CPU/RAM-intensiv
   - Graph: Schnell
   - Relational: Minimal

3. **Batch-Größen**
   - Small batches: Besseres Progress-Feedback
   - Large batches: Höherer Durchsatz

### Wartbarkeit

1. **Versionierung**
   ```json
   {
     "_version": "1.0",
     "ThemisHost": "..."
   }
   ```

2. **Kommentare** (JSON5 oder YAML nutzen)
   ```yaml
   # Production ThemisDB Server
   ThemisHost: themis-prod.company.com
   ThemisPort: 8765
   
   # Analysis Options
   MaxFileSize: 200 # MB
   ```

3. **Backup**
   ```powershell
   # Backup vor Änderungen
   Copy-Item "$env:APPDATA\ThemisIngestionTool\appsettings.json" `
             "$env:APPDATA\ThemisIngestionTool\appsettings.json.bak"
   ```

## Fehlerbehebung

### "Einstellungen konnten nicht geladen werden"

**Ursache**: Korrupte JSON-Datei

**Lösung**:
```powershell
# Lösche korrupte Datei
Remove-Item "$env:APPDATA\ThemisIngestionTool\appsettings.json"

# Tool erstellt neue mit Defaults
```

### "Port bereits in Verwendung"

**Ursache**: Anderer Service nutzt Port 8765

**Lösung**:
```powershell
# Prüfe welcher Prozess den Port nutzt
netstat -ano | findstr :8765

# Ändere Port in Einstellungen
```

### "Connection Timeout"

**Ursache**: Themis-Server nicht erreichbar

**Checkliste**:
- [ ] Server läuft? `docker ps`
- [ ] Firewall? `Test-NetConnection -Port 8765`
- [ ] Netzwerk? `ping themis-host`
- [ ] Port-Mapping? Docker: `-p 8765:18765`

## Profil-Management (Feature-Idee)

```csharp
// TODO: Mehrere Profile speichern
public class ProfileManager
{
    public void SaveProfile(string name, AppSettings settings)
    {
        var profilePath = Path.Combine(_appDataPath, $"profiles\\{name}.json");
        File.WriteAllText(profilePath, JsonSerializer.Serialize(settings));
    }
    
    public AppSettings LoadProfile(string name)
    {
        var profilePath = Path.Combine(_appDataPath, $"profiles\\{name}.json");
        return JsonSerializer.Deserialize<AppSettings>(File.ReadAllText(profilePath));
    }
    
    public List<string> GetProfiles()
    {
        return Directory.GetFiles(Path.Combine(_appDataPath, "profiles"), "*.json")
            .Select(Path.GetFileNameWithoutExtension)
            .ToList();
    }
}
```

**UI**:
- ComboBox mit Profil-Auswahl
- "Neu", "Speichern", "Löschen" Buttons
- Quick-Switch zwischen Dev/Prod/Test

## Schema-Validation (TODO)

```json
{
  "$schema": "https://themisdb.org/schemas/ingestion-config-v1.json",
  "ThemisHost": "localhost",
  ...
}
```

Mit JSON-Schema-Validation:
```csharp
public void ValidateSettings(AppSettings settings)
{
    var schema = JsonSchema.FromFile("config-schema.json");
    var json = JsonSerializer.Serialize(settings);
    var errors = schema.Validate(json);
    
    if (errors.Any())
        throw new ValidationException(string.Join(", ", errors));
}
```
