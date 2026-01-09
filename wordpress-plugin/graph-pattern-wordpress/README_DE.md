# ThemisDB Graph Pattern Visualizer - WordPress Plugin

Interaktive Graph-Pattern-Visualisierung mit Filterung, Suche und farbcodierten Knotengruppen. Inspiriert von Neo4j Bloom.

## Funktionen

### 🎨 Neo4j Bloom-Inspirierte Oberfläche
- **Options-Overlay-Panel**: Ausklappbares Panel mit umfassenden Steuerelementen
- **Suche**: Finden Sie Knoten nach Namen mit Echtzeit-Hervorhebung
- **Gruppenfilter**: Ein-/Ausblenden von Knotenkategorien mit Checkboxen
- **Farbanpassung**: Ändern Sie Gruppenfarben dynamisch mit Farbauswählern
- **Knotendetails**: Klicken Sie auf Knoten, um Eigenschaften und Verbindungen zu sehen

### 📊 Interaktive Visualisierung
- **Mehrere Layouts**: Force-directed, hierarchisch (oben-unten/links-rechts), kreisförmig
- **Zoom & Pan**: Intuitive Navigationssteuerung
- **Physiksimulation**: Natürliche Graph-Layouts mit einstellbaren Parametern
- **Geschwungene Kanten**: Gebogene Verbindungen für bessere Visualisierung
- **Hover-Effekte**: Tooltips und Hervorhebung

### 🎛️ Erweiterte Steuerelemente
- **Schieberegler**: Passen Sie Knotenabstand und Kantenstärke in Echtzeit an
- **Umschaltschalter**: Aktivieren/Deaktivieren von Physik, Beschriftungen und mehr
- **Layout-Auswahl**: Wechseln zwischen verschiedenen Layout-Algorithmen
- **Vollbildmodus**: Immersive Graph-Erkundung

### 📥 Export-Funktionen
- **PNG-Export**: Hochwertige Bilder herunterladen
- **JSON-Export**: Graph-Daten für Analysen speichern
- **Druckunterstützung**: Optimiert für den Druck

### 🔍 Best Practices von Neo4j Bloom
- **Knotengruppierung**: Farbcodierte Kategorien (Client, API, Query, Storage, etc.)
- **Suche mit Filterung**: Echtzeit-Knotensuche mit Ergebnis-Hervorhebung
- **Erweitern/Reduzieren**: Doppelklick zum Erweitern (zukünftiges Feature)
- **Layouts speichern**: Benutzerdefinierte Layout-Erhaltung (pro Benutzer)
- **Legende**: Klare visuelle Anleitung zu Knotenfarben

## Installation

### Manuelle Installation

1. **Plugin herunterladen oder kopieren**
   ```bash
   cd /pfad/zu/wordpress/wp-content/plugins/
   cp -r /pfad/zu/ThemisDB/wordpress-plugin/graph-pattern-wordpress ./themisdb-graph-pattern
   ```

2. **Plugin aktivieren**
   - Gehen Sie zu WordPress Admin → Plugins
   - Finden Sie "ThemisDB Graph Pattern Visualizer"
   - Klicken Sie auf "Aktivieren"

3. **Einstellungen konfigurieren** (Optional)
   - Gehen Sie zu Einstellungen → Graph Pattern
   - Passen Sie Standard-Layout, Farben und Funktionen an

## Verwendung

### Basis-Shortcode

```php
[themisdb_graph]
```

### Shortcode mit Parametern

```php
[themisdb_graph data_source="default" layout="force_directed" height="600px" show_controls="true" show_overlay="true"]
```

### Verfügbare Parameter

| Parameter | Beschreibung | Standard | Optionen |
|-----------|--------------|----------|----------|
| `data_source` | Datenquellen-Identifier | `"default"` | Beliebiger String |
| `layout` | Layout-Algorithmus | `"force_directed"` | `force_directed`, `hierarchical_top`, `hierarchical_left`, `circular` |
| `height` | Höhe des Graph-Containers | `"600px"` | Beliebiger CSS-Höhenwert |
| `show_controls` | Steuerleiste anzeigen | `"true"` | `"true"`, `"false"` |
| `show_overlay` | Options-Overlay anzeigen | `"true"` | `"true"`, `"false"` |

## Beispiele

### Einfache Visualisierung
```php
[themisdb_graph]
```

### Hierarchisches Layout
```php
[themisdb_graph layout="hierarchical_top" height="800px"]
```

### Benutzerdefinierte Datenquelle
```php
[themisdb_graph data_source="my_custom_graph"]
```

## Benutzeroberfläche

### Hauptsteuerungen
- **Layout-Auswahl**: Zwischen verschiedenen Graph-Layouts wechseln
- **Zoom-Steuerung**: + / - / An Bildschirm anpassen
- **Vollbild**: Immersiver Anzeigemodus
- **Knotenzähler**: Zeigt sichtbare/gesamt Knoten

### Options-Overlay-Panel
Auf der rechten Seite des Graphen:

1. **Suchfeld**
   - Knotennamen eingeben zum Suchen
   - Ergebnisse werden automatisch hervorgehoben
   - Erstes Ergebnis wird fokussiert

2. **Knotengruppen**
   - Checkboxen zum Ein-/Ausblenden von Gruppen
   - Farbindikatoren für jede Gruppe
   - Knotenanzahl pro Gruppe
   - Farbauswähler zum Anpassen der Farben

3. **Layout-Einstellungen**
   - Physiksimulation ein-/ausschalten
   - Beschriftungen ein-/ausblenden
   - Knotenabstand-Schieberegler (50-300)
   - Kantenstärke-Schieberegler (1-10)

4. **Export**
   - Als PNG exportieren
   - Als JSON exportieren

5. **Knotendetails**
   - Erscheint beim Klicken auf einen Knoten
   - Zeigt ID, Gruppe, Ebene, Verbindungen

### Tastaturkürzel
- **Pfeiltasten**: Navigieren im Graphen
- **Scrollen**: Ein-/Auszoomen
- **Strg + Klick**: Mehrere Knoten auswählen (in vis-network integriert)

## Technische Details

### Abhängigkeiten
- **vis-network.js**: Graph-Visualisierungsbibliothek (v9.1.2)
- **jQuery**: WordPress-Standard
- **WordPress**: 5.0+
- **PHP**: 7.4+

### Datenstruktur

#### Knoten
```javascript
{
    id: 1,
    label: "Knotenname",
    group: "kategorie_id",
    level: 1,
    size: 25
}
```

#### Kanten
```javascript
{
    from: 1,
    to: 2,
    label: "Verbindung",
    dashes: false
}
```

#### Gruppen
```javascript
{
    id: "kategorie_id",
    label: "Kategoriename",
    color: "#2ea44f",
    visible: true
}
```

### Architektur
Das Plugin folgt dem etablierten ThemisDB WordPress-Plugin-Muster:

```
graph-pattern-wordpress/
├── themisdb-graph-pattern.php    # Haupt-Plugin-Datei
├── assets/
│   ├── css/
│   │   └── graph-pattern.css     # Styles
│   └── js/
│       └── graph-pattern.js      # Client-seitige Logik
├── templates/
│   ├── graph.php                 # Haupt-Visualisierungs-Template
│   └── admin-settings.php        # Admin-Einstellungsseite
├── includes/                     # (Zukünftig: Zusätzliche PHP-Klassen)
├── README.md                     # Englische Dokumentation
├── README_DE.md                  # Diese Datei
├── LICENSE                       # MIT-Lizenz
└── uninstall.php                # Bereinigung bei Deinstallation
```

## Beispiel-Graph-Daten

Das Plugin enthält einen Standard-ThemisDB-Architektur-Graphen mit:
- **7 Knotengruppen**: Client, API, Query, LLM, Transaction, Index, Storage
- **20 Knoten**: Repräsentieren ThemisDB-Komponenten
- **22 Kanten**: Komponenten-Beziehungen
- **Farbcodiert**: Jede Gruppe hat eine eindeutige Farbe

## Anpassung

### Eigene Graph-Daten hinzufügen

Um eigene Graph-Daten hinzuzufügen, ändern Sie die `get_graph_data()` Methode in der Haupt-Plugin-Datei:

```php
private function get_graph_data($data_source = 'default') {
    if ($data_source === 'mein_eigener_graph') {
        return array(
            'nodes' => [...],
            'edges' => [...],
            'groups' => [...]
        );
    }
    // Standard-Graph-Daten
    return array(...);
}
```

### Eigene Farben

Farben können angepasst werden:
1. In den Admin-Einstellungen (Einstellungen → Graph Pattern)
2. Über den Farbauswähler im Overlay-Panel
3. Durch Änderung der Standard-Gruppen in PHP

### Layout-Algorithmen

Vier Layout-Algorithmen sind verfügbar:
1. **Force-Directed**: Natürliche Clusterung, am besten für allgemeine Verwendung
2. **Hierarchisch Oben-Unten**: Organisierte Ebenen, gut für Systemarchitektur
3. **Hierarchisch Links-Rechts**: Horizontaler Fluss, gut für Prozessabläufe
4. **Kreisförmig**: Knoten in Kreisanordnung

## Performance

### Optimierungstipps
- Begrenzen Sie Knoten auf 500 für flüssige Performance (konfigurierbar)
- Deaktivieren Sie Physik für sehr große Graphen
- Verwenden Sie hierarchisches Layout für bessere Organisation großer Graphen
- Blenden Sie weniger wichtige Gruppen aus, um visuelle Komplexität zu reduzieren

### Browser-Anforderungen
- Moderne Browser (Chrome, Firefox, Safari, Edge)
- JavaScript aktiviert
- Canvas-Unterstützung

## Inspiration & Referenzen

Dieses Plugin ist inspiriert von:
- **Neo4j Bloom**: Graph-Erkundungs- und Visualisierungstool
- **ThemisDB Architecture Diagrams Plugin**: Design-Muster und Styling
- **vis-network.js**: Leistungsstarke Graph-Visualisierungsbibliothek

## Roadmap

Zukünftige Verbesserungen:
- [ ] Dynamische Knotenerweiterung bei Doppelklick
- [ ] Benutzerdefinierte Layouts pro Benutzer speichern
- [ ] Benutzerdefinierte Graph-JSON-Dateien importieren
- [ ] Mehr Farbschemata
- [ ] Knotenform-Anpassung
- [ ] Kantentyp-Filterung
- [ ] Pfadfindung zwischen Knoten
- [ ] Clustering für große Graphen
- [ ] Zeitbasierte Animationen
- [ ] Mehrsprachige Unterstützung

## Mitwirken

Beiträge sind willkommen! Bitte:
1. Repository forken
2. Feature-Branch erstellen
3. Änderungen vornehmen
4. Pull-Request einreichen

## Lizenz

MIT-Lizenz - Siehe LICENSE-Datei für Details

## Support

- **Repository**: https://github.com/makr-code/ThemisDB
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Dokumentation**: Siehe Haupt-ThemisDB-Dokumentation

## Credits

- **ThemisDB Team**: Plugin-Entwicklung
- **vis-network.js**: Graph-Visualisierungsbibliothek
- **Neo4j Bloom**: Interface-Inspiration

---

**Version**: 1.0.0  
**Letzte Aktualisierung**: Januar 2026
