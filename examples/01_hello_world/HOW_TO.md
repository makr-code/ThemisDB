# Hello World - Schritt-für-Schritt-Anleitung

> **Historischer Stand:** 2025-12-22 — Inhalte nicht gegen aktuelle Quellen geprüft.

Diese Anleitung führt Sie durch die Verwendung der Hello World Anwendung und erklärt jeden Schritt im Detail.

## 🎯 Lernziele

Nach diesem Tutorial können Sie:
- Eine Verbindung zu ThemisDB herstellen
- CRUD-Operationen durchführen
- Die Tkinter-Benutzeroberfläche bedienen
- Fehler verstehen und beheben

## 📝 Vorbereitungen

### 1. ThemisDB Server starten

Öffnen Sie ein Terminal und starten Sie ThemisDB mit Docker:

```bash
docker run -d -p 8080:8080 themisdb/themisdb:latest
```
<!-- TODO: verify against current source -->

**Überprüfen Sie den Status**:
```bash
curl http://localhost:8080/health
```

Erwartete Antwort:
```json
{"status": "healthy"}
```

### 2. Beispiel vorbereiten

```bash
cd examples/01_hello_world
pip install -r requirements.txt
pip install themisdb-client
```
<!-- TODO: verify against current source -->

### 3. Anwendung starten

```bash
python main.py
```

## 🖥️ Benutzeroberfläche

Die Anwendung zeigt ein Fenster mit folgenden Elementen:

```
┌─────────────────────────────────────────┐
│  ThemisDB - Hello World                 │
├─────────────────────────────────────────┤
│  Connection: ● Connected                │
├─────────────────────────────────────────┤
│  User ID:    [________________]         │
│  Name:       [________________]         │
│  Email:      [________________]         │
├─────────────────────────────────────────┤
│  [Create]  [Get]  [Update]  [Delete]   │
├─────────────────────────────────────────┤
│  Status: Ready                          │
│  ┌─────────────────────────────────┐   │
│  │ Output:                         │   │
│  │                                 │   │
│  │                                 │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### UI-Komponenten

- **Connection Status**: Zeigt an, ob die Verbindung zu ThemisDB besteht
- **Eingabefelder**: User ID, Name, Email
- **Aktions-Buttons**: Führen CRUD-Operationen aus
- **Status-Leiste**: Zeigt Erfolgs- oder Fehlermeldungen
- **Output-Bereich**: Zeigt Details der Operationen

## 🚀 Tutorial: CRUD-Operationen

### Tutorial 1: Benutzer erstellen (CREATE)

**Ziel**: Einen neuen Benutzer in ThemisDB speichern

**Schritte**:

1. Geben Sie in das Feld "User ID" ein: `user1`
2. Geben Sie in das Feld "Name" ein: `Max Mustermann`
3. Geben Sie in das Feld "Email" ein: `max@example.com`
4. Klicken Sie auf den Button **"Create"**

**Was passiert**:
- Die Anwendung sendet eine PUT-Anfrage an ThemisDB
- ThemisDB speichert die Daten im Relational Model
- Die Status-Leiste zeigt: `✓ User created successfully`
- Der Output zeigt die gespeicherten Daten

**Beispiel-Output**:
```json
{
  "id": "user1",
  "name": "Max Mustermann",
  "email": "max@example.com",
  "created_at": "2025-12-22T10:30:00Z"
}
```

**Tastenkombination**: `Ctrl+N` (Create New)

---

### Tutorial 2: Benutzer abrufen (READ)

**Ziel**: Einen existierenden Benutzer aus ThemisDB lesen

**Schritte**:

1. Geben Sie in das Feld "User ID" ein: `user1`
2. Klicken Sie auf den Button **"Get"**

**Was passiert**:
- Die Anwendung sendet eine GET-Anfrage an ThemisDB
- ThemisDB sucht den Benutzer mit der ID "user1"
- Die Daten werden in den Feldern angezeigt
- Der Output zeigt alle Benutzer-Details

**Beispiel-Output**:
```json
{
  "id": "user1",
  "name": "Max Mustermann",
  "email": "max@example.com",
  "created_at": "2025-12-22T10:30:00Z"
}
```

**Tastenkombination**: `Ctrl+G` (Get)

---

### Tutorial 3: Benutzer aktualisieren (UPDATE)

**Ziel**: Bestehende Benutzerdaten ändern

**Schritte**:

1. Geben Sie in das Feld "User ID" ein: `user1`
2. Klicken Sie auf **"Get"**, um die aktuellen Daten zu laden
3. Ändern Sie das Feld "Email" zu: `max.mustermann@example.com`
4. Klicken Sie auf den Button **"Update"**

**Was passiert**:
- Die Anwendung sendet eine PUT-Anfrage mit den neuen Daten
- ThemisDB aktualisiert den Benutzer
- Die Status-Leiste zeigt: `✓ User updated successfully`
- Der Output zeigt die aktualisierten Daten

**Beispiel-Output**:
```json
{
  "id": "user1",
  "name": "Max Mustermann",
  "email": "max.mustermann@example.com",
  "updated_at": "2025-12-22T10:35:00Z"
}
```

**Tastenkombination**: `Ctrl+S` (Save/Update)

---

### Tutorial 4: Benutzer löschen (DELETE)

**Ziel**: Einen Benutzer aus ThemisDB entfernen

**Schritte**:

1. Geben Sie in das Feld "User ID" ein: `user1`
2. Klicken Sie auf den Button **"Delete"**
3. Bestätigen Sie den Löschvorgang im Dialog

**Was passiert**:
- Die Anwendung zeigt einen Bestätigungs-Dialog
- Bei Bestätigung sendet sie eine DELETE-Anfrage
- ThemisDB löscht den Benutzer
- Die Status-Leiste zeigt: `✓ User deleted successfully`
- Die Eingabefelder werden geleert

**Tastenkombination**: `Ctrl+D` (Delete)

---

## 🎓 Erweiterte Übungen

### Übung 1: Multiple Benutzer

Erstellen Sie mehrere Benutzer:

```
user1: Alice, alice@example.com
user2: Bob, bob@example.com
user3: Charlie, charlie@example.com
```

**Aufgabe**: Erstellen Sie alle drei Benutzer und rufen Sie jeden einzeln ab.

### Übung 2: Daten-Validierung

Versuchen Sie, ungültige Daten einzugeben:

- Leere User ID
- Ungültige Email-Adresse
- Sehr lange Namen

**Beobachtung**: Wie reagiert die Anwendung auf fehlerhafte Eingaben?

### Übung 3: Nicht existierender Benutzer

Versuchen Sie, einen Benutzer abzurufen, der nicht existiert:

1. Geben Sie "User ID" ein: `user999`
2. Klicken Sie auf "Get"

**Erwartung**: Die Anwendung zeigt eine Fehlermeldung: "User not found"

### Übung 4: Update ohne Get

Ändern Sie Daten direkt ohne vorheriges Abrufen:

1. Geben Sie alle Felder manuell ein
2. Klicken Sie auf "Update"

**Frage**: Was ist der Unterschied zum vorherigen Tutorial?

## 🔍 Hinter den Kulissen

### Was passiert bei CREATE?

```python
# 1. Daten werden gesammelt
user_data = {
    "id": "user1",
    "name": "Max Mustermann",
    "email": "max@example.com"
}

# 2. HTTP PUT Request an ThemisDB
PUT /entities/users:user1
Content-Type: application/json
{
  "blob": "{\"name\":\"Max Mustermann\",\"email\":\"max@example.com\"}"
}

# 3. ThemisDB speichert die Daten
# 4. Antwort wird zurückgesendet
```

### Was passiert bei READ?

```python
# 1. User ID wird verwendet
user_id = "user1"

# 2. HTTP GET Request
GET /entities/users:user1

# 3. ThemisDB sucht die Daten
# 4. Daten werden zurückgesendet
{
  "id": "user1",
  "name": "Max Mustermann",
  "email": "max@example.com"
}
```

## 💡 Tipps und Tricks

### Tipp 1: Tastenkombinationen nutzen

Verwenden Sie Tastenkombinationen für schnellere Arbeit:
- `Ctrl+N` - Create
- `Ctrl+G` - Get
- `Ctrl+S` - Update
- `Ctrl+D` - Delete
- `Ctrl+Q` - Quit

### Tipp 2: Connection Status prüfen

Der grüne/rote Punkt oben zeigt den Verbindungsstatus:
- 🟢 Grün = Verbunden
- 🔴 Rot = Nicht verbunden

### Tipp 3: Output-Bereich nutzen

Der Output-Bereich zeigt detaillierte Informationen:
- Erfolgreiche Operationen
- Fehlermeldungen
- Rohe JSON-Daten

### Tipp 4: Tab-Navigation

Nutzen Sie die Tab-Taste, um zwischen Feldern zu wechseln.

## ❌ Häufige Fehler und Lösungen

### Fehler 1: Connection Failed

**Symptom**: 🔴 Roter Verbindungsstatus

**Ursache**: ThemisDB Server läuft nicht

**Lösung**:
```bash
docker ps  # Prüfen, ob Container läuft
docker start themisdb  # Falls gestoppt
```

### Fehler 2: User Not Found

**Symptom**: "Error: User not found"

**Ursache**: Benutzer mit dieser ID existiert nicht

**Lösung**: 
- Erstellen Sie den Benutzer zuerst mit "Create"
- Oder verwenden Sie eine existierende User ID

### Fehler 3: Invalid Email

**Symptom**: "Error: Invalid email format"

**Ursache**: Email-Adresse hat falsches Format

**Lösung**: Verwenden Sie Format: `name@domain.com`

### Fehler 4: Empty Fields

**Symptom**: "Error: All fields are required"

**Ursache**: Ein oder mehrere Felder sind leer

**Lösung**: Füllen Sie alle Felder aus

## 📚 Nächste Schritte

Glückwunsch! Sie haben die Grundlagen von ThemisDB gelernt. 

**Weiterführende Beispiele**:

1. **[Todo-App](../02_todo_app/)** - Listen und Filterung
2. **[Kontaktmanager](../03_contact_manager/)** - Suche und Queries
3. **[Inventarsystem](../04_inventory_system/)** - Multi-Model Features

## 🤔 Verständnisfragen

Testen Sie Ihr Wissen:

1. Was ist der Unterschied zwischen CREATE und UPDATE?
2. Wie löscht man einen Benutzer aus ThemisDB?
3. Was passiert, wenn man einen nicht existierenden Benutzer abrufen will?
4. Welche HTTP-Methoden werden für CRUD-Operationen verwendet?

## 💬 Feedback

Haben Sie Fragen oder Anregungen zu diesem Tutorial?
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

**Viel Erfolg beim Lernen mit ThemisDB!** 🚀
