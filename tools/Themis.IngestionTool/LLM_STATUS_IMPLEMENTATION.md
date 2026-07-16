> ⚠️ **Historischer Statusreport** – Dieser Report beschreibt den Stand zum Zeitpunkt der Erstellung.
> Für aktuellen Stand: Quellcode in `src/llm/` und `include/llm/` prüfen.

# Implementation: LLM Status Anzeige im Themis Ingestion Tool

## Übersicht
Diese Implementierung fügt die Anzeige des **llama.cpp/Ollama LLM-Status** (mit geladenem Modell) neben dem ThemisDB-Status in der Anwendung ein. Die Einstellungen sind im Settings-Dialog konfigurierbar.

## Neue Komponenten

### 1. LlmStatusService (`Services/LlmStatusService.cs`) ✅
**Neue Datei** mit folgenden Features:

#### Interface `ILlmStatusService`
- `CheckConnectionAsync()` - Prüft Verbindung zum Ollama-Service
- `GetLlmStatusAsync()` - Ruft aktuellen LLM-Status ab
- `StatusChanged` Event - Wird ausgelöst bei Status-Änderungen

#### Klasse `LlmStatusService`
- Automatischer Heartbeat-Check alle 10 Sekunden
- Prüft Verfügbarkeit des Ollama-Services über `/api/tags`
- Überprüft, ob das konfigurierte Modell geladen ist
- Zeigt Modellgröße und Modifikationszeitpunkt
- Fehlernachrichtbehandlung

#### Klasse `LlmStatus`
- `IsAvailable` - Ollama-Service verfügbar
- `IsModelLoaded` - Konfiguriertes Modell geladen
- `LoadedModel` - Name des geladenen Modells
- `ModelSize` - Größe in Bytes
- `LastModified` - Zeitstempel
- `Error` - Fehlermeldung
- `GetStatusDescription()` - Benutzerfreundliche Beschreibung

#### Klasse `LlmStatusChangedEventArgs`
- Enthält neuen Status und Zeitstempel

---

## Konfigurationsänderungen

### 2. AppSettings.cs (`Models/AppSettings.cs`) ✅
Neue Properties:
```csharp
public bool EnableLlmStatusMonitoring { get; set; } = true;
public int LlmStatusCheckIntervalSeconds { get; set; } = 10;
public bool ShowLlmStatusInStatusBar { get; set; } = true;
```

---

## UI-Änderungen

### 3. SettingsDialog.xaml (`Views/SettingsDialog.xaml`) ✅
- **Fenster-Höhe** erhöht von 500 auf 700 Pixel
- **Neue GroupBox**: "LLM Status Überwachung" mit:
  - CheckBox: "LLM-Status-Überwachung aktivieren"
  - CheckBox: "LLM-Status in Statusleiste anzeigen"
  - Spinner: "Status-Check Intervall (Sekunden)"

### 4. SettingsDialogViewModel.cs (`ViewModels/SettingsDialogViewModel.cs`) ✅
Neue Properties mit Bindungen:
```csharp
public bool EnableLlmStatusMonitoring
public int LlmStatusCheckIntervalSeconds
public bool ShowLlmStatusInStatusBar
```

Aktualisierte Methoden:
- `LoadSettings()` - Lädt LLM-Einstellungen
- `SaveSettings()` - Speichert LLM-Einstellungen

### 5. MainWindow.xaml (`Views/MainWindow.xaml`) ✅
**StatusBar** erweitert mit LLM-Status-Anzeige:
- Neues `StatusBarItem` für LLM-Status
- Farbcodierung:
  - 🟢 **Grün** (RGB 40,167,69): LLM aktiv mit Modell geladen
  - 🟠 **Orange** (RGB 255,193,7): Ollama verfügbar, Modell nicht geladen
  - 🔴 **Rot** (RGB 220,53,69): Ollama Offline
- Zeigt Modellname an, z.B. "Aktiv: llama2:latest"

---

## ViewModel-Änderungen

### 6. MainWindowViewModel.cs (`ViewModels/MainWindowViewModel.cs`) ✅

**Neue Injektionen:**
```csharp
private readonly ILlmStatusService _llmStatusService;
```

**Neue Properties:**
```csharp
public bool ShowLlmStatusInStatusBar
public string LlmStatusText
public SolidColorBrush LlmStatusColor
```

**Aktualisierte Methoden:**
- `InitializeAsync()` - Registriert LLM-Status Events und führt initiale Prüfung durch
- `UpdateLlmStatus(LlmStatus)` - Aktualisiert Anzeige mit farblicher Kodierung
- `LoadSettings()` - Lädt LLM-Einstellungen

---

## Dependency Injection

### 7. App.xaml.cs (`App.xaml.cs`) ✅
Service-Registrierung:
```csharp
services.AddSingleton<ILlmStatusService, LlmStatusService>();
```

---

## Funktionales Verhalten

### Status-Monitoring
1. **Automatische Prüfung**: Alle 10 Sekunden (konfigurierbar)
2. **Event-Getrieben**: Statusänderungen triggern UI-Updates
3. **Fehlerbehandlung**: Robuste Fehlerbehandlung mit Logging
4. **Non-Blocking**: Asynchrone Operationen

### Anzeigelogik
```
Status verfügbar && Modell geladen
├─> 🟢 "Aktiv: <Modellname>"
│
Status verfügbar && Modell nicht geladen
├─> 🟠 "Aktiv: <Modellname>" (Orange)
│
Status nicht verfügbar
└─> 🔴 "Offline"
```

### Konfigurationsoptionen im Settings-Dialog
- ✅ Enable/Disable LLM-Monitoring
- ✅ Enable/Disable Anzeige in Statusleiste
- ✅ Intervall anpassen (z.B. 5, 10, 30 Sekunden)

---

## Technische Details

### HTTP-Requests
```
GET http://{OllamaHost}:{OllamaPort}/api/tags
```
Antwortet mit geladenen Modellen und Metadaten

### Timeout
5 Sekunden pro HTTP-Request

### Standardwerte
- `OllamaHost`: "localhost"
- `OllamaPort`: 11434
- `LlamaModel`: "llama2"
- `LlmStatusCheckInterval`: 10 Sekunden
- `EnableLlmStatusMonitoring`: true
- `ShowLlmStatusInStatusBar`: true

---

## Verwendungsbeispiel

### 1. Settings öffnen
Bearbeiten → Einstellungen → "LLM Status Überwachung"

### 2. LLM-Monitoring aktivieren
```
☑ LLM-Status-Überwachung aktivieren
☑ LLM-Status in Statusleiste anzeigen
Status-Check Intervall (Sekunden): [10]
```

### 3. Status beobachten
Statusleiste zeigt Echtzeit-Status:
```
Status: Bereit | 🟢 Themis: Online | 🟢 LLM: Aktiv: llama2:7b
```

---

## Dateien-Übersicht

| Datei | Änderung | Status |
|-------|----------|--------|
| `Services/LlmStatusService.cs` | ✅ Neu | Komplett |
| `Models/AppSettings.cs` | ✅ Erweitert | +3 Properties |
| `Views/SettingsDialog.xaml` | ✅ Erweitert | +1 GroupBox, Höhe 500→700 |
| `ViewModels/SettingsDialogViewModel.cs` | ✅ Erweitert | +3 Properties, LoadSettings/SaveSettings |
| `Views/MainWindow.xaml` | ✅ Erweitert | +LLM StatusBarItem |
| `ViewModels/MainWindowViewModel.cs` | ✅ Erweitert | +LlmStatusService, +LLM Properties |
| `App.xaml.cs` | ✅ Erweitert | +Service Registration |

---

## Testing

### Szenario 1: Ollama läuft, Modell geladen
```
Expected: 🟢 "Aktiv: llama2:7b"
```

### Szenario 2: Ollama läuft, Modell nicht geladen
```
Expected: 🟠 "Aktiv: llama2:7b" (Orange)
Status sollte sich zu 🔴 Offline ändern, wenn kein Modell
```

### Szenario 3: Ollama nicht erreichbar
```
Expected: 🔴 "Offline"
```

### Szenario 4: Settings → Monitoring deaktivieren
```
Expected: LLM-Status verschwindet aus StatusBar
```

---

## Keine Fehler
✅ Alle Dateien compilieren ohne Fehler
✅ Alle Dependencies sind korrekt injiziert
✅ Alle UI-Bindungen sind gültig
