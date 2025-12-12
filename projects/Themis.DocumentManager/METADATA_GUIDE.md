# Metadaten-System - Benutzerhandbuch

## Übersicht

Das Metadaten-System ermöglicht die strukturierte Verwaltung von Dokumentmetadaten mit automatischer Gruppierung, Validierung und Persistierung.

## Features

### 1. Gruppierte Metadaten-Ansicht

Die Metadaten werden automatisch in logische Gruppen organisiert:
- **📁 Vorgang** - Aktenzeichen, Betreff, Vorgangsart
- **⚡ Status & Workflow** - Status, Priorität, Fristen
- **🏢 Organisation** - Sachbearbeiter, Behörde, Abteilung
- **🕒 Zeitliche Daten** - Erstellungs- und Änderungsdatum
- **🏷️ Schlagwörter & Themen** - Tags und Themenbereiche

### 2. Intelligentes Verstecken leerer Felder

Je nach Strategie werden leere Felder automatisch ausgeblendet:
- `HideEmptyFields` - Leere Felder werden versteckt (Standardeinstellung)
- `HideEmptySections` - Komplette leere Gruppen werden versteckt
- `ShowAllExpanded` - Alle Felder werden angezeigt, alle Gruppen offen
- `ShowAllCollapsed` - Alle Felder werden angezeigt, alle Gruppen geschlossen

### 3. Editierbare Felder

Verschiedene Eingabetypen werden unterstützt:
- **Text** - Einzeilige und mehrzeilige Texteingaben
- **Date** - Datumspicker für Datumsangaben
- **Number** - Numerische Eingaben
- **Boolean** - Checkboxen für Ja/Nein-Werte
- **Dropdown** - Auswahllisten (zukünftig)

### 4. Validierung

Pflichtfelder werden automatisch erkannt:
- **⚠️ Validierungs-Panel** - Zeigt fehlende Pflichtfelder an
- **Speichern mit Warnung** - Warnt vor unvollständigen Daten
- **Visuelle Kennzeichnung** - Pflichtfelder mit * markiert

### 5. Speicher-Operationen

Drei Aktionen stehen zur Verfügung:

#### 💾 Speichern
- Validiert alle Pflichtfelder
- Speichert Änderungen in den Cache
- Zeigt Bestätigung mit Feldanzahl

#### 🔄 Neu laden
- Verwirft nicht gespeicherte Änderungen
- Lädt Daten neu aus dem Cache/Datenquelle
- Aktualisiert die Anzeige

#### 🔒 Finalisieren
- Sperrt das Dokument gegen weitere Änderungen
- Setzt Status auf "Finalized"
- Erfordert Bestätigung

## Konfiguration

### YAML-Layout anpassen

Die Metadaten-Struktur wird in `Config/metadata_layout.yaml` definiert:

```yaml
strategy: HideEmptyFields

groups:
  - id: vorgang
    title: Vorgang
    icon: "📁"
    displayOrder: 1
    fields:
      - name: Aktenzeichen
        path: process.fileReference
        type: Text
        required: true
      - name: Betreff
        path: process.subject
        type: Text
        required: true
```

### Strategien ändern

Im YAML die `strategy` anpassen:
```yaml
strategy: HideEmptyFields  # oder HideEmptySections, ShowAllExpanded, ShowAllCollapsed
```

### Neue Gruppen hinzufügen

```yaml
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

### Feldtypen

Verfügbare Typen:
- `Text` - Textfeld (mehrzeilig)
- `Date` - Datumsauswahl
- `Number` - Zahlen
- `Boolean` - Ja/Nein
- `Dropdown` - Auswahlliste (mit `options`)
- `RichText` - Formatierter Text

#### Dropdown-Felder konfigurieren

```yaml
fields:
  - name: Status
    path: process.status
    type: Dropdown
    options: 
      - Eingang
      - In Bearbeitung
      - Rückfrage
      - Externe Prüfung
      - Genehmigt
      - Abgelehnt
  
  - name: Priorität
    path: process.priority
    type: Dropdown
    options: [Niedrig, Normal, Hoch, Dringend]
```

**Hinweis**: Bei Dropdown-Feldern wird automatisch eine ComboBox generiert. Die erste Option wird bei leerem Wert nicht automatisch selektiert.

## Programmierung

### MetadataBindingService verwenden

```csharp
var service = new MetadataBindingService();

// Binding laden oder erstellen
var binding = service.GetOrCreateBinding("DOC-12345");

// Einzelnes Feld aktualisieren
await service.UpdateFieldAsync("DOC-12345", "Aktenzeichen", "AZ-2024-001");

// Komplettes Binding speichern
await service.SaveBindingAsync(binding);

// Finalisieren
await service.FinalizeBindingAsync("DOC-12345");

// Cache verwalten
service.InvalidateCache("DOC-12345");
service.ClearCache();
```

### MetadataGroupedAccordion verwenden

```csharp
var accordion = new MetadataGroupedAccordion
{
    Strategy = CollapseStrategy.HideEmptyFields,
    FieldGroups = layoutService.BuildGroups(binding, layout)
};

// Validierung
var (isValid, errors) = accordion.ValidateRequiredFields();

// Export
var metadata = accordion.ExportMetadata();

// Events
accordion.FieldValueChanged += (s, e) => {
    Console.WriteLine($"Feld {e.Field.FieldName} geändert zu: {e.NewValue}");
};
```

### MetadataLayoutService verwenden

```csharp
var layoutService = new MetadataLayoutService();

// Layout laden
var layout = layoutService.LoadLayout();

// Gruppen erstellen
var groups = layoutService.BuildGroups(binding, layout);
```

## Architektur

### Service-Layer

```
┌─────────────────────────────────────┐
│   MetadataLayoutService             │
│   - YAML-Konfiguration laden        │
│   - Gruppen erstellen               │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│   MetadataBindingService            │
│   - Bindings laden/speichern        │
│   - Cache-Management                │
│   - Persistierung                   │
└─────────────────────────────────────┘
```

### UI-Layer

```
┌─────────────────────────────────────┐
│   MetadataGroupedAccordion          │
│   - Gruppierte Ansicht              │
│   - Editierbare Felder              │
│   - Validierung                     │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│   MetadataSummaryPanel              │
│   - Übersicht                       │
│   - Statistiken                     │
│   - Status-Anzeige                  │
└─────────────────────────────────────┘
```

### Datenfluss

```
1. Benutzer wählt Dokument
   ↓
2. UpdateMetadataAccordion() wird aufgerufen
   ↓
3. MetadataBindingService lädt/erstellt Binding
   ↓
4. MetadataLayoutService baut Gruppen aus YAML + Binding
   ↓
5. MetadataGroupedAccordion rendert UI
   ↓
6. Benutzer bearbeitet Felder → Events
   ↓
7. MetadataBindingService speichert Änderungen
```

## Erweiterungen

### Eigene Persistierung implementieren

In `MetadataBindingService.cs`:

```csharp
private DocumentMetadataBinding? LoadBindingFromSource(string documentId)
{
    // TODO: Eigene Implementierung
    // z.B. aus ThemisDB API laden
    var client = new ThemisApiClient();
    return await client.GetMetadataAsync(documentId);
}

public async Task<bool> SaveBindingAsync(DocumentMetadataBinding binding)
{
    // TODO: Eigene Implementierung
    // z.B. in ThemisDB API speichern
    var client = new ThemisApiClient();
    return await client.SaveMetadataAsync(binding);
}
```

### Neue Feldtypen hinzufügen

In `MetadataGroupedAccordion.cs`, Methode `CreateFieldControl()`:

```csharp
else if (field.Type == FieldType.MyCustomType)
{
    var customControl = new MyCustomControl
    {
        Value = field.CurrentValue
    };
    
    customControl.ValueChanged += (s, e) =>
    {
        field.CurrentValue = customControl.Value;
        FieldValueChanged?.Invoke(this, new FieldEditEventArgs 
        { 
            Field = field, 
            NewValue = customControl.Value 
        });
    };
    
    inputControl = customControl;
}
```

## Best Practices

1. **Pflichtfelder definieren** - Markiere wichtige Felder als `required: true`
2. **Gruppen sinnvoll strukturieren** - Max. 5-7 Gruppen für Übersichtlichkeit
3. **Icons verwenden** - Emoji oder Unicode für visuelle Unterscheidung
4. **Display Order** - Wichtige Gruppen zuerst (niedrige Zahl = oben)
5. **Cache invalidieren** - Nach externen Änderungen Cache neu laden
6. **Validierung nutzen** - Vor kritischen Operationen validieren
7. **Events abonnieren** - Für automatische Speicherung oder Logging

## Troubleshooting

### Metadaten werden nicht angezeigt
- Prüfe `metadata_layout.yaml` auf Syntax-Fehler
- Stelle sicher, dass `MetadataLayoutService` korrekt initialisiert ist
- Debugge `BuildGroups()` - werden Felder korrekt gemappt?

### Änderungen werden nicht gespeichert
- Prüfe ob `FieldValueChanged` Event gefeuert wird
- Debugge `MetadataBindingService.SaveBindingAsync()`
- Stelle sicher, dass DocumentId korrekt gesetzt ist

### Validierung schlägt fehl
- Prüfe `IsRequired` Flag in BoundFields
- Debugge `ValidateRequiredFields()` Methode
- Stelle sicher, dass CurrentValue korrekt gesetzt wird

### Cache-Probleme
- Verwende `InvalidateCache()` nach externen Änderungen
- `ClearCache()` bei Bedarf für kompletten Reset
- Thread-Safety ist garantiert durch Lock

## Performance

- **Caching** - Bindings werden im Speicher gecacht
- **Lazy Loading** - Gruppen werden nur bei Bedarf geladen
- **Event Throttling** - Bei Bedarf TextChanged-Events drosseln
- **Virtuelle Listen** - Bei >100 Feldern Virtualisierung erwägen

## Support

Bei Fragen oder Problemen:
1. Prüfe diese Dokumentation
2. Debugge mit Breakpoints in Services
3. Aktiviere Debug-Ausgaben: `System.Diagnostics.Debug.WriteLine()`
4. Konsultiere Code-Kommentare in Service-Klassen
