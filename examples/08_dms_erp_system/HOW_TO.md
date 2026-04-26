> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# DMS/ERP-System - Bedienungsanleitung

## 🚀 Start

```bash
python main.py
```

**Standard-Login**:
- Admin: `admin / admin123`
- Manager: `manager / manager123`
- Employee: `employee / employee123`

## 📋 Hauptfunktionen

### Dokumente verwalten

**Dokument hochladen**:
1. "Neues Dokument" → Datei wählen
2. Typ auswählen (Rechnung, Vertrag, etc.)
3. Metadaten eingeben
4. Tags hinzufügen
5. Upload → System erstellt Version 1

**Dokument bearbeiten**:
1. Dokument öffnen
2. "Bearbeiten" → Datei hochladen
3. Änderungen beschreiben
4. Speichern → Neue Version erstellt

**Versionen anzeigen**:
- "Versionshistorie" öffnen
- Alle Versionen auflisten
- Vergleichen und Wiederherstellen

### Workflows

**Dokument zur Genehmigung senden**:
1. Dokument auswählen
2. "Workflow starten"
3. Workflow-Template wählen
4. Absenden

**Genehmigung durchführen**:
1. Tab "Zu genehmigen"
2. Dokument prüfen
3. Kommentar eingeben
4. "Genehmigen" oder "Ablehnen"

**Workflow-Status**:
- Zeigt aktuellen Schritt
- Bearbeitungshistorie
- Benachrichtigungen

### Berechtigungen

**Berechtigungen setzen**:
1. Dokument auswählen
2. "Berechtigungen verwalten"
3. Benutzer/Rollen hinzufügen
4. Rechte wählen (Lesen, Schreiben, Löschen)

**Rollen**:
- **Admin**: Volle Rechte
- **Manager**: Genehmigen, Lesen, Schreiben
- **Employee**: Lesen, Erstellen
- **Guest**: Nur Lesen

### Suche

**Erweiterte Suche**:
- Volltext-Suche im Inhalt
- Metadaten-Filter
- Tag-Suche
- Semantische Suche (Vector)
- Datumsbereich

### Audit-Log

**Aktivitäten anzeigen**:
1. Tab "Audit"
2. Filter nach Benutzer, Aktion, Zeitraum
3. Details jeder Aktion
4. Export für Compliance

## ⌨️ Tastenkombinationen

- `Ctrl+N` - Neues Dokument
- `Ctrl+U` - Upload
- `Ctrl+F` - Suche
- `Ctrl+W` - Workflow
- `Ctrl+P` - Berechtigungen

## 💡 Best Practices

1. **Metadaten pflegen**: Vollständig ausfüllen
2. **Tags nutzen**: Für bessere Suche
3. **Workflows definieren**: Standardisieren
4. **Regelmäßig archivieren**: Alte Dokumente
5. **Berechtigungen prüfen**: Least Privilege

---

**Weitere Guides**: ADMIN_GUIDE.md, SECURITY.md, WORKFLOW_DESIGN.md
