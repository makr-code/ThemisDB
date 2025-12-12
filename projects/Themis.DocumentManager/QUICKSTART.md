# 🚀 Quick Start - Metadaten-System

## In 5 Minuten loslegen

### 1. Anwendung starten
```bash
cd C:\VCC\themis\projects\Themis.DocumentManager
dotnet run
```

### 2. Dokument auswählen
1. Klicke auf ein Element im **linken TreeView** (z.B. "📄 Projekt_Spec.docx")
2. Die **rechte Sidebar** zeigt automatisch die Metadaten an

### 3. Metadaten bearbeiten
1. Klicke in ein **Textfeld** und tippe
2. Wähle ein **Datum** mit dem DatePicker
3. Toggle eine **Checkbox** für Ja/Nein-Werte
4. Wähle aus einem **Dropdown** (z.B. Status: "In Bearbeitung", Priorität: "Hoch")

### 4. Speichern
1. Klicke **💾 Speichern** in der rechten Sidebar
2. Bei fehlenden Pflichtfeldern erscheint eine Warnung
3. Bestätige oder korrigiere die Eingaben

---

## 📋 Wichtigste Features

### Automatisches Verstecken
- Leere Felder werden **automatisch ausgeblendet**
- Klicke **"+ X leere Felder anzeigen"** um alle zu sehen

### Validierung
- **⚠️ Panel** zeigt fehlende Pflichtfelder
- **\* Pflichtfelder** sind mit Asterisk markiert

### Gruppierung
- 📁 **Vorgang** - Aktenzeichen, Betreff, Vorgangsart
- ⚡ **Status & Workflow** - Status, Priorität, Fristen
- 🏢 **Organisation** - Sachbearbeiter, Behörde
- 🕒 **Zeitliche Daten** - Erstellung, Änderung
- 🏷️ **Schlagwörter** - Tags, Themen

---

## 🎨 Layout anpassen

### YAML-Datei editieren
Öffne: `Config/metadata_layout.yaml`

```yaml
strategy: HideEmptyFields  # oder: HideEmptySections, ShowAllExpanded

groups:
  - id: meine-gruppe
    title: Meine Gruppe
    icon: "🎯"
    displayOrder: 10
    fields:
      - name: Mein Feld
        path: custom.myField
        type: Text
        required: false
```

### Neu laden
1. Ändere YAML-Datei
2. Klicke **🔄 Neu laden**
3. Layout wird aktualisiert

---

## ⚡ Tipps & Tricks

### Schnell navigieren
- **Einfachklick** im TreeView = Vorschau
- **Doppelklick** im TreeView = Neuer Tab

### Sidebar anpassen
- **Ziehe den Splitter** zwischen Center und Sidebar
- Ändere die Breite nach Bedarf

### Dokument finalisieren
1. Klicke **🔒 Finalisieren**
2. Bestätige im Dialog
3. Dokument ist nun **schreibgeschützt**

---

## 🆘 Häufige Fragen

**Q: Metadaten werden nicht angezeigt?**  
A: Prüfe ob ein Dokument im TreeView ausgewählt ist.

**Q: Änderungen werden nicht gespeichert?**  
A: Klicke auf **💾 Speichern** Button.

**Q: Leere Felder anzeigen?**  
A: Klicke auf **"+ X leere Felder anzeigen"** Button.

**Q: Layout zurücksetzen?**  
A: Lösche `metadata_layout.yaml` → Standardlayout wird verwendet.

---

## 📚 Weiterführende Dokumentation

- **METADATA_GUIDE.md** - Vollständiges Handbuch
- **METADATA_CHANGELOG.md** - Feature-Liste
- **METADATA_IMPLEMENTATION_SUMMARY.md** - Technische Details

---

**Viel Erfolg! 🎉**
