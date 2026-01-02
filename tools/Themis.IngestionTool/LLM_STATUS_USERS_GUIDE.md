# LLM Status Überwachung - Benutzerhandbuch

## Was ist neu?

Das Themis Ingestion Tool zeigt jetzt den **Live-Status des llama.cpp/Ollama-Services** in der Anwendungs-Statusleiste an.

## Statusleiste verstehen

```
Status: Bereit | 🟢 Themis: Online | 🟢 LLM: Aktiv: llama2:7b
```

### Farben:

| Farbe | Status | Bedeutung |
|-------|--------|-----------|
| 🟢 **Grün** | Aktiv | Ollama läuft und das konfigurierte Modell ist geladen |
| 🟠 **Orange** | Warnung | Ollama läuft, aber das Modell ist nicht geladen |
| 🔴 **Rot** | Offline | Ollama ist nicht erreichbar oder nicht aktiv |

## Einstellungen

### So aktivieren/deaktivieren Sie die Überwachung:

1. Öffnen Sie das Hauptfenster
2. Gehen Sie zu **Bearbeiten → Einstellungen**
3. Scrollen Sie zum Abschnitt **"LLM Status Überwachung"**

### Verfügbare Optionen:

- **☑ LLM-Status-Überwachung aktivieren**  
  Aktiviert/deaktiviert die automatische Überprüfung des LLM-Status

- **☑ LLM-Status in Statusleiste anzeigen**  
  Zeigt/versteckt den LLM-Status in der unteren Statusleiste

- **Status-Check Intervall (Sekunden)**  
  Wie oft soll die Verbindung überprüft werden? (Standard: 10 Sekunden)
  - Kleinere Werte = häufigere Updates, aber mehr CPU-Last
  - Größere Werte = weniger CPU-Last, aber langsamere Updates

## Was wird überprüft?

Der Service prüft automatisch:

✅ **Ist Ollama erreichbar?**
- Verbindet sich zu `http://localhost:11434/api/tags`
- Timeout: 5 Sekunden

✅ **Ist das Modell geladen?**
- Sucht das konfigurierte Modell (Standard: "llama2")
- Zeigt Modellname und Größe

✅ **Fehler**
- Zeigt aussagekräftige Fehlermeldungen bei Problemen

## Konfigurierte Modelloptionen

Das System prüft standardmäßig auf das Modell **"llama2"**.  
Um ein anderes Modell zu überwachen, ändern Sie in den Einstellungen:
- **LLM Settings → LlamaModel**: "llama2" → "your-model-name"

## Beispiel-Szenarien

### Szenario 1: Alles läuft normal
```
🟢 LLM: Aktiv: llama2:7b
→ Ollama läuft und das Modell ist vollständig geladen
```

### Szenario 2: Ollama läuft, Modell wird noch geladen
```
🟠 LLM: Aktiv: llama2:7b
→ Ollama ist verfügbar, aber das Modell wird gerade geladen
```

### Szenario 3: Ollama nicht gestartet
```
🔴 LLM: Offline
→ Ollama ist nicht erreichbar (nicht gestartet oder falscher Host/Port)
```

### Szenario 4: Falsches Modell installiert
```
🔴 LLM: Offline - Modell 'llama2' ist nicht geladen
→ Das erwartete Modell ist nicht in Ollama installiert
```

## Häufig gestellte Fragen

**F: Wo kann ich Ollama herunterladen?**  
A: Von https://ollama.ai (Linux/Mac/Windows)

**F: Wie lade ich ein Modell in Ollama?**  
A: Öffnen Sie ein Terminal und führen Sie aus:
```bash
ollama pull llama2
```

**F: Kann ich den Update-Intervall ändern?**  
A: Ja, in den Einstellungen unter "Status-Check Intervall (Sekunden)"

**F: Warum zeigt es "Offline" an, obwohl Ollama läuft?**  
A: Prüfen Sie:
1. Ollama läuft und hört auf http://localhost:11434
2. Das Modell ist geladen: `ollama list`
3. Der Modellname in den Einstellungen ist korrekt

**F: Kann ich die Überwachung komplett ausschalten?**  
A: Ja, deaktivieren Sie in den Einstellungen "LLM-Status-Überwachung aktivieren"

## Fehlerbehebung

### Problem: "Status-Check Intervall" wird nicht berücksichtigt
**Lösung**: Das Intervall wird beim Neustart der Anwendung übernommen.

### Problem: Status wird nicht aktualisiert
**Lösung**: 
- Prüfen Sie, ob "LLM-Status-Überwachung aktivieren" aktiviert ist
- Starten Sie die Anwendung neu
- Prüfen Sie die Firewall-Einstellungen

### Problem: Ollama ist online, aber "Offline" wird angezeigt
**Lösung**:
- Prüfen Sie den Host und Port in den Einstellungen
- Führen Sie aus: `ollama list` (prüft, ob Modelle geladen sind)
- Starten Sie Ollama neu

## Technische Informationen

**Service:** `ILlmStatusService`  
**Update-Frequenz:** Konfigurierbar (Standard: 10 Sekunden)  
**Timeout:** 5 Sekunden pro Check  
**Logging:** Fehler werden im Console-Fenster protokolliert

## Kontakt & Support

Bei Problemen oder Fragen:
1. Prüfen Sie die Logs in der Anwendung
2. Überprüfen Sie die Ollama-Verbindung manuell
3. Konsultieren Sie die Dokumentation des Projekts
