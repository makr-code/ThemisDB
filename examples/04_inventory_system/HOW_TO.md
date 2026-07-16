> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Inventarsystem - Bedienungsanleitung

## 🎯 Übersicht

Professionelle Lagerverwaltung mit Bestandsverfolgung und Lieferanten-Management.

## 🚀 Start

```bash
python main.py
```

## 📋 Hauptfunktionen

### Produkte verwalten

**Neues Produkt anlegen**:
1. Tab "Produkte" → "Neu"
2. SKU, Name, Preis eingeben
3. Mindestbestand festlegen
4. Kategorie und Lagerort wählen
5. Speichern

**Produkt bearbeiten**:
1. Produkt aus Liste auswählen
2. "Bearbeiten" klicken
3. Felder ändern → Speichern

### Bestand verwalten

**Wareneingang**:
1. Tab "Bestand" → Produkt auswählen
2. "Wareneingang" klicken
3. Menge und Lieferant eingeben
4. Grund angeben → Buchen

**Warenausgang**:
1. Produkt auswählen
2. "Warenausgang" klicken
3. Menge eingeben
4. Grund (Verkauf, Verbrauch) → Buchen

**Inventur**:
1. Tab "Bestand" → "Inventur"
2. Gezählte Menge eingeben
3. System berechnet Differenz
4. Korrektur buchen

### Lieferanten verwalten

**Lieferant hinzufügen**:
1. Tab "Lieferanten" → "Neu"
2. Name, Kontaktdaten eingeben
3. Speichern

**Produkt zu Lieferant zuordnen**:
1. Produkt auswählen
2. "Lieferant zuordnen"
3. Lieferant wählen
4. Lieferzeit und Preis angeben

### Dashboard und Reports

**Dashboard öffnen**:
- Tab "Dashboard" zeigt:
  - Gesamtwert des Lagers
  - Produkte mit niedrigem Bestand
  - Letzte Bewegungen
  - Statistik-Charts

**Charts**:
- **Bestandswert nach Kategorie** (Pie Chart)
- **Bestandsbewegungen** (Line Chart)
- **Top 10 Produkte** (Bar Chart)

## 📊 Wichtige Funktionen

### Warnungen

System zeigt automatisch:
- 🔴 Kritisch: Bestand < 25% des Mindestbestands
- 🟡 Warnung: Bestand < 50% des Mindestbestands
- 🟢 OK: Bestand über Mindestbestand

### Suche und Filter

- **Produktsuche**: Nach SKU, Name, Kategorie
- **Bestandsfilter**: Niedrig/Normal/Hoch
- **Bewegungsfilter**: Nach Typ, Datum, Benutzer

## ⌨️ Tastenkombinationen

- `Ctrl+N` - Neues Produkt
- `Ctrl+F` - Suche
- `Ctrl+I` - Wareneingang
- `Ctrl+O` - Warenausgang
- `F5` - Aktualisieren
- `Ctrl+P` - Drucken/Export

## 💡 Best Practices

1. **Regelmäßige Inventur**: Monatlich Bestände prüfen
2. **Mindestbestände setzen**: Für alle Produkte
3. **Lieferanten pflegen**: Aktuelle Preise und Lieferzeiten
4. **Bewegungen dokumentieren**: Immer Grund angeben
5. **Backup**: Regelmäßig Daten exportieren

## 🔍 Workflows

### Workflow 1: Neues Produkt aufnehmen

1. Produkt anlegen (SKU, Name, Preis)
2. Lieferant zuordnen
3. Mindestbestand festlegen
4. Wareneingang buchen (Erstbestand)

### Workflow 2: Bestellung beim Lieferanten

1. Dashboard → Produkte mit niedrigem Bestand
2. Produkt auswählen → "Lieferanten anzeigen"
3. Besten Lieferant wählen (Preis, Lieferzeit)
4. Bestellung aufgeben (extern)
5. Bei Lieferung: Wareneingang buchen

### Workflow 3: Verkauf/Ausgabe

1. Tab "Bestand"
2. Produkt suchen
3. Warenausgang buchen
4. Menge und Grund angeben
5. System aktualisiert Bestand automatisch

## 📈 Reports verstehen

**Bestandswert-Report**:
- Zeigt Gesamtwert aller Produkte
- Aufschlüsselung nach Kategorie
- Hilft bei Finanzplanung

**Bewegungs-Report**:
- Chronologische Historie
- Filter nach Zeitraum
- Export für Buchhaltung

## 🐛 Troubleshooting

**Problem**: Warenausgang nicht möglich
- **Ursache**: Nicht genug Bestand
- **Lösung**: Prüfen Sie die aktuelle Menge

**Problem**: Lieferant nicht in Liste
- **Ursache**: Lieferant nicht angelegt
- **Lösung**: Zuerst Lieferant in Tab "Lieferanten" anlegen

**Problem**: Charts werden nicht angezeigt
- **Ursache**: matplotlib nicht installiert
- **Lösung**: `pip install matplotlib`

---

**Weitere Details**: Siehe DATA_MODEL.md und API_USAGE.md
