# Dropdown-Implementierung - Technische Dokumentation

## Übersicht

**Version**: 1.1.0  
**Datum**: 11. Dezember 2025  
**Feature**: ComboBox-Support für Dropdown-Felder

## Implementierte Änderungen

### 1. Model-Erweiterung

**Datei**: `Models/DynamicMetadataModels.cs`

```csharp
public class MetadataField
{
    // ... existing properties ...
    public List<string>? Options { get; set; }  // NEW: For Dropdown fields
}
```

**Zweck**: Speichert die verfügbaren Auswahlmöglichkeiten für Dropdown-Felder.

---

### 2. YAML-Konfiguration

**Datei**: `Config/metadata_layout.yaml`

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
```

**Unterstützte Formate**:
- **Mehrzeilig** (YAML-Array):
  ```yaml
  options:
    - Option 1
    - Option 2
  ```
- **Einzeilig** (Inline-Array):
  ```yaml
  options: [Option 1, Option 2, Option 3]
  ```

---

### 3. Service-Mapping

**Datei**: `Services/MetadataLayoutService.cs`

```csharp
group.Fields.Add(new MetadataField
{
    // ... existing mappings ...
    Options = fieldDef.Options  // Transfer dropdown options from YAML
});
```

**Funktion**: Überträgt die `options` aus der YAML-Konfiguration zur `MetadataField`-Instanz.

---

### 4. UI-Komponente

**Datei**: `UI/MetadataGroupedAccordion.cs`

**Methode**: `CreateFieldControl()`

```csharp
else if (field.Type == FieldType.Dropdown && field.Options != null && field.Options.Count > 0)
{
    var comboBox = new ComboBox
    {
        ItemsSource = field.Options,
        SelectedItem = field.CurrentValue,
        Padding = new Thickness(8, 6, 8, 6),
        FontSize = 12,
        Background = new SolidColorBrush(Colors.White),
        IsEditable = false
    };

    comboBox.SelectionChanged += (s, e) =>
    {
        if (comboBox.SelectedItem != null)
        {
            field.CurrentValue = comboBox.SelectedItem.ToString();
            FieldValueChanged?.Invoke(this, new FieldEditEventArgs 
            { 
                Field = field, 
                NewValue = field.CurrentValue 
            });
        }
    };

    inputControl = comboBox;
}
```

**Funktionsweise**:
1. Prüft ob Feld vom Typ `Dropdown` ist und `Options` vorhanden sind
2. Erstellt `ComboBox` mit `ItemsSource` aus Options-Liste
3. Setzt `SelectedItem` auf aktuellen Wert (wenn vorhanden)
4. Registriert `SelectionChanged`-Event für automatisches Update
5. Feuert `FieldValueChanged`-Event bei Auswahl-Änderung

---

## Verwendete Feldtypen

Nach Implementierung:

| FieldType | UI-Control | Beschreibung |
|-----------|-----------|--------------|
| `Text` | `TextBox` | Mehrzeilige Texteingabe |
| `Number` | `TextBox` | Numerische Eingabe |
| `Date` | `DatePicker` | Datumsauswahl |
| `Boolean` | `CheckBox` | Ja/Nein-Werte |
| **`Dropdown`** | **`ComboBox`** | **Auswahlfelder mit vordefinierten Optionen** |
| `RichText` | `TextBox` | Formatierter Text (noch nicht mit Editor) |

---

## Beispiel-Konfiguration

```yaml
groups:
  - id: vorgang
    title: Vorgang
    icon: "📁"
    fields:
      - name: Vorgangsart
        path: process.type
        type: Dropdown
        options: 
          - Baugenehmigung
          - Betriebsgenehmigung
          - Gewerbeanmeldung
          - Umweltgenehmigung
          - Verkehrsregelung

  - id: status
    title: Status & Workflow
    icon: "⚡"
    fields:
      - name: Status
        path: process.status
        type: Dropdown
        required: true
        options: [Eingang, In Bearbeitung, Rückfrage, Externe Prüfung, Genehmigt, Abgelehnt]
      
      - name: Priorität
        path: process.priority
        type: Dropdown
        options: [Niedrig, Normal, Hoch, Dringend]
```

---

## Event-Flow

```
User wählt Option in ComboBox
         ↓
SelectionChanged Event wird ausgelöst
         ↓
field.CurrentValue wird aktualisiert
         ↓
FieldValueChanged Event wird gefeuert
         ↓
MetadataBindingService kann reagieren (z.B. Auto-Save)
```

---

## Besonderheiten

### 1. Null-Safety
```csharp
field.Type == FieldType.Dropdown && field.Options != null && field.Options.Count > 0
```
Prüft explizit auf:
- Korrekten Feldtyp
- Vorhandensein der Options-Liste
- Mindestens eine Option vorhanden

### 2. IsEditable = false
```csharp
IsEditable = false
```
ComboBox ist **nicht editierbar** → User kann nur aus vorgegebenen Optionen wählen.

### 3. Kein Default-Select
Wenn `CurrentValue` leer ist, wird **keine** Option automatisch ausgewählt.  
User muss explizit eine Auswahl treffen.

### 4. ToString() Konvertierung
```csharp
field.CurrentValue = comboBox.SelectedItem.ToString();
```
Alle Werte werden als String gespeichert (konsistent mit anderen Feldtypen).

---

## Testing

### Manueller Test
1. Anwendung starten: `dotnet run`
2. Dokument auswählen
3. Rechte Sidebar öffnen
4. Dropdown-Felder finden (z.B. "Status", "Priorität", "Vorgangsart")
5. Option auswählen
6. Speichern und neu laden → Wert bleibt erhalten

### Erwartetes Verhalten
- ✅ ComboBox wird angezeigt
- ✅ Alle Optionen aus YAML sind sichtbar
- ✅ Auswahl triggert Event
- ✅ Wert wird in `CurrentValue` gespeichert
- ✅ Nach Reload bleibt Auswahl erhalten
- ✅ Pflichtfeld-Validierung funktioniert

---

## Erweiterungsmöglichkeiten

### 1. Multi-Select Dropdown
```csharp
// Verwende ListBox statt ComboBox
var listBox = new ListBox 
{ 
    SelectionMode = SelectionMode.Multiple 
};
```

### 2. Dynamische Options (von API)
```csharp
// Options aus API laden
field.Options = await apiClient.GetOptionsAsync(field.ThemisPath);
```

### 3. Editable ComboBox
```csharp
IsEditable = true  // Erlaubt Custom-Eingaben
```

### 4. Autocomplete
```csharp
comboBox.IsTextSearchEnabled = true;
comboBox.StaysOpenOnEdit = true;
```

---

## Abhängigkeiten

- **YamlDotNet**: YAML-Parsing für `options`-Listen
- **WPF ComboBox**: Standard-Control (keine zusätzliche Dependency)
- **System.Collections.Generic**: List<string> für Options

---

## Changelog-Eintrag

**Version 1.1.0** - 11. Dezember 2025
- ✅ Dropdown-Felder implementiert
- ✅ ComboBox-Support in UI
- ✅ YAML-Konfiguration mit `options`
- ✅ Event-Handling für SelectionChanged
- ✅ Integration in MetadataLayoutService
- ✅ Dokumentation aktualisiert

---

## Bekannte Limitierungen

1. **Keine Gruppierung in Dropdown**: Optionen werden als flache Liste angezeigt
2. **Keine Icons**: ComboBox-Items haben keine Icons (nur Text)
3. **Statische Options**: Werden beim Laden fixiert, keine dynamischen Updates
4. **String-Only**: Optionen sind immer Strings (keine komplexen Objekte)

---

## Nächste Schritte

- [ ] Multi-Select Dropdown implementieren
- [ ] Icons für ComboBox-Items
- [ ] API-Integration für dynamische Options
- [ ] Autocomplete mit Fuzzy-Search
- [ ] Custom ComboBox-Template mit Tooltips
