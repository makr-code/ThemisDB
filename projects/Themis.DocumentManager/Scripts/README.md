# ThemisDB Test Data Scripts

## Übersicht

Dieses Verzeichnis enthält Skripte zum Befüllen der ThemisDB mit umfangreichen Testdaten für das Document Management System (DSM).

## Verwendung

### 1. Schema initialisieren (init-schema.ps1)

**Zuerst** das Schema und Collections anlegen:

```powershell
# Standard (localhost:8765, admin/admin)
.\init-schema.ps1

# Custom ThemisDB URL
.\init-schema.ps1 -ThemisDbUrl "http://themis.example.com:8765"
```

Erstellt:
- 16 Document Collections (users, documents, processes, etc.)
- 6 Edge Collections (Relationen)
- ~25 Indexes für Performance
- 3 System Views (active_documents, pending_wiedervorlagen, recent_audit_logs)

### 2. Testdaten laden (seed-themisdb.ps1)

**Danach** Testdaten einfügen:

```powershell
# Standard (localhost:8765, admin/admin)
.\seed-themisdb.ps1

# Custom ThemisDB URL
.\seed-themisdb.ps1 -ThemisDbUrl "http://themis.example.com:8765"

# Mit Credentials
.\seed-themisdb.ps1 -Username "myuser" -Password "mypass"
```

## Generierte Testdaten

### 1. Benutzer (10)
- `max.mustermann`, `anna.schmidt`, `thomas.mueller`, etc.
- Rollen: User, Editor, Admin, Manager
- Abteilungen: Recht, Personal, IT, Finanzen, Einkauf

### 2. Dokumente (~555)
- **Verträge**: 50 Stück
- **Rechnungen**: 100 Stück
- **Protokolle**: 75 Stück
- **Berichte**: 60 Stück
- **Anträge**: 40 Stück
- **Beschlüsse**: 30 Stück
- **Akten**: 80 Stück
- **Schriftverkehr**: 120 Stück

Jedes Dokument enthält:
- Titel, Autor, Typ, Status
- Klassifizierung (Öffentlich, Intern, Vertraulich, Geheim)
- Aufbewahrungsfrist (5-30 Jahre)
- Metadata (Abteilung, Projekt, Vertraulichkeit)
- Tags (2-5 zufällige Tags)
- Dateigröße, MIME-Type, Prüfsumme

### 3. Prozesse (50)
- Typen: Genehmigungsverfahren, Beschaffung, Personalantrag, etc.
- Status: InProgress, Completed, Cancelled
- Prioritäten: Low, Medium, High, Critical
- Workflow-Steps mit Completion-Status
- Teilnehmer und Verantwortliche

### 4. Dateistruktur (200 Dateien)
Ordner:
- `/Verträge/2024`, `/Verträge/2023`
- `/Rechnungen/Eingang/2024`, `/Rechnungen/Ausgang/2024`
- `/Personal/Bewerbungen`, `/Personal/Verträge`
- `/Projekte/PRJ-001`, `/Projekte/PRJ-002`
- `/Akten/Laufend`, `/Akten/Archiv`

Dateitypen: PDF, DOCX, XLSX, PPTX, TXT, JPG

### 5. Relationen (~100)
- Dokument → Prozess Verknüpfungen
- Edge-Collection: `document_process_edges`

### 6. Audit Logs (500)
- Aktionen: VIEW, EDIT, DELETE
- Zeitstempel über 90 Tage verteilt
- IP-Adressen, User-Agents

### 7. Tags & Metadata
- 300 Dokumente mit 1-5 Tags versehen
- Tags: tag-0 bis tag-49

### 8. Wiedervorlagen (80)
- Verknüpft mit Dokumenten
- Status: Pending, Completed
- Prioritäten: Low, Medium, High
- Fälligkeitsdaten über 120 Tage verteilt
- Erinnerungen (reminderSent)

### 9. Mitzeichnungen (60)
- Für Verträge, Beschlüsse, Anträge
- Status: InProgress, Approved, Rejected
- 2-5 Unterzeichner pro Mitzeichnung
- Kommentare und Zeitstempel
- Deadlines

### 10. E-Mail Threads (100)
- Betreff, Sender, Empfänger
- Thread-Verkettung (InReplyTo)
- 0-3 Anhänge pro E-Mail
- Flags: Read, Flagged
- Tags: Inbox

### 11. Aufbewahrungs- & Klassifizierungsregeln (5)
- Verträge: 30 Jahre, Vertraulich
- Rechnungen: 10 Jahre, Intern (Steuerrelevant)
- Protokolle: 7 Jahre, Intern
- Personalakten: 15 Jahre, Geheim (DSGVO)
- Schriftverkehr: 5 Jahre, Intern

## Collection-Schema

### Document Collections (16)
- `users` - Benutzer
- `documents` - Dokumente
- `processes` - Prozesse
- `files` - Dateien
- `audit_logs` - Audit-Protokolle
- `wiedervorlagen` - Wiedervorlagen/Reminders
- `mitzeichnungen` - Mitzeichnungen/Co-Signatures
- `emails` - E-Mail Threads
- `retention_rules` - Aufbewahrungsregeln
- `folders` - Ordnerstruktur
- `tags` - Tags für Dokumenten
- `comments` - Kommentare
- `versions` - Dokumentversionen
- `notifications` - Benachrichtigungen
- `tasks` - Aufgaben
- `calendar_events` - Kalenderereignisse

### Edge Collections (6)
- `document_process_edges` - Dokument ↔ Prozess
- `document_file_edges` - Dokument ↔ Datei
- `user_document_edges` - Benutzer ↔ Dokument (Berechtigungen)
- `process_task_edges` - Prozess ↔ Aufgabe
- `document_version_edges` - Dokument ↔ Version
- `folder_document_edges` - Ordner ↔ Dokument

### System Views (3)
- `active_documents` - Aktive Dokumente (sortiert nach Änderungsdatum)
- `pending_wiedervorlagen` - Fällige Wiedervorlagen (nächste 7 Tage)
- `recent_audit_logs` - Audit-Logs der letzten 30 Tage

## Hinweise

- Alle Zeitstempel sind randomisiert über die letzten 1-2 Jahre
- Dateien haben realistische Größen (1KB - 10MB)
- Klassifizierungen und Rollen folgen DSGVO-Prinzipien
- Testdaten sind für Development/Testing, **NICHT für Produktion**

## Troubleshooting

### "Connection refused"
→ ThemisDB Docker Container starten:
```bash
docker start themis-db
```

### "Authentication failed"
→ Credentials prüfen (Default: admin/admin)

### "Collection not found"
→ Collections müssen in ThemisDB bereits existieren. Ggf. Schema vorher initialisieren.

## Erweiterung

Um weitere Testdaten hinzuzufügen, bearbeiten Sie:
- `$documentTypes` - Dokumenttypen und Anzahl
- `$processTypes` - Prozesstypen
- `$folders` - Ordnerstruktur
- Anzahl in den For-Schleifen (z.B. `1..50` → `1..100`)
