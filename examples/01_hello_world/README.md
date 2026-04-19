> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Hello World - Erste Schritte mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-easy-green)
![Duration](https://img.shields.io/badge/duration-5--10%20min-blue)

## 📝 Übersicht

Das "Hello World" Beispiel zeigt die grundlegenden CRUD-Operationen (Create, Read, Update, Delete) mit ThemisDB. Sie lernen, wie man:
- Eine Verbindung zu ThemisDB herstellt
- Daten speichert und abruft
- Daten aktualisiert
- Daten löscht

Die Anwendung verfügt über eine einfache Tkinter-GUI, die alle Operationen visuell demonstriert.

## ✨ Features

- ✅ **Verbindung zu ThemisDB** - Client-Setup und Health-Check
- ✅ **CREATE** - Neue Benutzer anlegen
- ✅ **READ** - Benutzer abrufen und anzeigen
- ✅ **UPDATE** - Benutzerdaten aktualisieren
- ✅ **DELETE** - Benutzer löschen
- ✅ **Tkinter GUI** - Einfache und intuitive Benutzeroberfläche
- ✅ **Fehlerbehandlung** - Aussagekräftige Fehlermeldungen

## 🖼️ Screenshots

*UI-Screenshot wird nach Implementierung hinzugefügt*

## 📋 Voraussetzungen

### ThemisDB Server

Der ThemisDB-Server muss laufen. Starten Sie ihn mit Docker:

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest

# Überprüfen Sie, ob der Server läuft
curl http://localhost:8080/health
```

### Python und Dependencies

- Python 3.8 oder höher
- Tkinter (normalerweise mit Python vorinstalliert)

## 🚀 Installation

1. **Navigieren Sie zum Beispiel-Verzeichnis**:
   ```bash
   cd examples/01_hello_world
   ```

2. **Installieren Sie die Abhängigkeiten**:
   ```bash
   pip install -r requirements.txt
   ```

## 🎮 Verwendung

### Anwendung starten

```bash
python main.py
```

### Grundlegende Operationen

1. **Benutzer erstellen**:
   - Geben Sie Name und Email ein
   - Klicken Sie auf "Create User"
   - Der Benutzer wird in ThemisDB gespeichert

2. **Benutzer abrufen**:
   - Geben Sie die User-ID ein
   - Klicken Sie auf "Get User"
   - Die Benutzerdaten werden angezeigt

3. **Benutzer aktualisieren**:
   - Ändern Sie Name oder Email
   - Klicken Sie auf "Update User"
   - Die Änderungen werden gespeichert

4. **Benutzer löschen**:
   - Geben Sie die User-ID ein
   - Klicken Sie auf "Delete User"
   - Der Benutzer wird aus ThemisDB entfernt

Siehe [HOW_TO.md](HOW_TO.md) für detaillierte Schritt-für-Schritt-Anleitungen.

## 📊 Datenmodell

Das Beispiel verwendet ein einfaches Benutzer-Modell:

```python
{
    "id": "user1",          # Eindeutige Benutzer-ID
    "name": "Max Mustermann",  # Benutzername
    "email": "max@example.com"  # Email-Adresse
}
```

Daten werden im **Relational Model** von ThemisDB gespeichert.

## 🏗️ Architektur

```
┌─────────────────┐
│   Tkinter UI    │  ← Benutzeroberfläche
└────────┬────────┘
         │
┌────────▼────────┐
│  Main App Logic │  ← Anwendungslogik
└────────┬────────┘
         │
┌────────▼────────┐
│ ThemisDB Client │  ← HTTP REST API
└────────┬────────┘
         │
┌────────▼────────┐
│  ThemisDB Server│  ← Datenbank
└─────────────────┘
```

## 📁 Dateien

- `main.py` - Hauptanwendung mit Tkinter-UI
- `themis_client.py` - ThemisDB-Client-Wrapper
- `README.md` - Diese Datei
- `HOW_TO.md` - Detaillierte Bedienungsanleitung
- `requirements.txt` - Python-Abhängigkeiten

## 🔧 Konfiguration

Die Standard-Konfiguration verbindet sich mit:
- **Host**: `localhost`
- **Port**: `8080`
- **Protokoll**: HTTP

Um die Verbindung anzupassen, bearbeiten Sie die Konstanten in `main.py`:

```python
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
```

## 🐛 Troubleshooting

### Server nicht erreichbar

```
Error: Connection refused
```

**Lösung**: Stellen Sie sicher, dass der ThemisDB-Server läuft:
```bash
curl http://localhost:8080/health
```

### Tkinter nicht verfügbar

```
Error: No module named 'tkinter'
```

**Lösung**: 
- **Ubuntu/Debian**: `sudo apt-get install python3-tk`
- **macOS**: Tkinter ist normalerweise vorinstalliert
- **Windows**: Tkinter ist normalerweise vorinstalliert

### Port bereits belegt

```
Error: Address already in use
```

**Lösung**: Ändern Sie den Port in der Docker-Konfiguration oder stoppen Sie den anderen Prozess.

## 📚 Weiterführende Ressourcen

- [ThemisDB Dokumentation](../../docs/)
- [Python Client API](../../clients/python/)
- [AQL Syntax](../../docs/aql/)
- [Nächstes Beispiel: Todo-App](../02_todo_app/)

## 🤝 Beitragen

Feedback und Verbesserungsvorschläge sind willkommen! Öffnen Sie ein Issue oder Pull Request auf GitHub.

## 📄 Lizenz

Dieses Beispiel ist unter der MIT-Lizenz lizenziert - siehe [LICENSE](../../LICENSE) für Details.

---

**Nächste Schritte**: Schauen Sie sich das [Todo-App Beispiel](../02_todo_app/) an, um zu lernen, wie man mit Listen und Filtern arbeitet.
