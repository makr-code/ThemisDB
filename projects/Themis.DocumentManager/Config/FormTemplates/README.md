# Form Template System - Dokumentation

## Überblick

Das Form Template System ermöglicht die **dynamische Erzeugung von Metadaten-Masken** für die Themis Dokumentenverwaltung basierend auf konfigurierbaren YAML/JSON-Templates. Das System ist inspiriert von **PDV VIS 5 Standard** für öffentliche Verwaltungen.

## Kernkomponenten

### 1. FormTemplate & FormField
- **FormTemplate**: Container für komplette Formularmasken mit Metadaten
- **FormSection**: Logische Gruppierung von Feldern (z.B. "Grunddaten", "Verwaltung")
- **FormField**: Einzelnes Eingabefeld mit Validierungsregeln
- **FieldType**: 24 verschiedene Feldtypen (Text, Textarea, Date, Dropdown, etc.)

### 2. Services

#### IFormTemplateService
Hauptservice für Formularverwaltung:
- `GetTemplateAsync(templateId)` - Abrufen eines Templates
- `GetTemplatesByCategoryAsync(category)` - Kategoriebasierte Suche
- `CreateTemplateAsync()` - Neues Template erstellen
- `UpdateTemplateAsync()` - Template aktualisieren
- `ValidateFormAsync()` - Validierung der Eingabedaten
- `SubmitFormAsync()` - Absendung und Persistierung

#### IFormConfigurationLoader
Laden/Speichern von Templates aus Dateien:
- `LoadFromJsonAsync()` - JSON-Datei laden
- `LoadFromYamlAsync()` - YAML-Datei laden
- `LoadAllTemplatesFromDirectoryAsync()` - Batch-Laden
- `SaveTemplateAsJsonAsync()` - JSON speichern
- `SaveTemplateAsYamlAsync()` - YAML speichern

#### IFormDatabaseMappingService
Datenbankintegration:
- `GetMappingAsync()` - Abrufen der Feldmappings
- `MapFormDataToDatabaseAsync()` - Umwandlung Formularfeld → Datenbankfeld

#### IFormTestDataService
Testdaten-Generierung:
- `GetSamplePDVSubmissionAsync()` - PDV VIS 5 Muster
- `GetAllSampleSubmissionsAsync()` - Alle Muster
- `GenerateRandomSubmissionAsync()` - Zufällische Daten

## Template-Struktur

### YAML-Format Beispiel

```yaml
form:
  id: pdv-vis5-document
  name: PDV VIS 5 Dokumentenverwaltung
  
  sections:
    - id: grunddaten
      title: "📋 Grunddaten"
      
      fields:
        - id: document-number
          name: document_number
          label: "Dokumentennummer"
          type: text
          required: true
          pattern: "^[A-Z]{2}\\d{6}$"
```

### JSON-Format Beispiel

```json
{
  "id": "pdv-vis5-document",
  "name": "PDV VIS 5 Dokumentenverwaltung",
  "sections": [
    {
      "id": "grunddaten",
      "title": "📋 Grunddaten",
      "fields": [
        {
          "id": "document-number",
          "name": "document_number",
          "label": "Dokumentennummer",
          "type": "Text",
          "isRequired": true,
          "pattern": "^[A-Z]{2}\\d{6}$"
        }
      ]
    }
  ]
}
```

## Unterstützte Feldtypen

| Typ | Beschreibung | Beispiel |
|-----|--------------|----------|
| Text | Einzeiliger Text | "Dokumententitel" |
| TextArea | Mehrzeiliger Text | "Beschreibung" |
| Number | Ganzzahl | "Seiten: 42" |
| Decimal | Dezimalzahl | "Preis: 19,99" |
| Currency | Währungsbetrag | "1.500,00 EUR" |
| Date | Datum | "2025-12-09" |
| DateTime | Datum + Zeit | "2025-12-09 14:30" |
| Email | E-Mail-Adresse | "max@example.com" |
| Phone | Telefonnummer | "+49 123 456789" |
| Dropdown | Auswahlbox | ["Auswahl 1", "Auswahl 2"] |
| MultiSelect | Mehrfachauswahl | Checkboxen |
| RadioButton | Optionsschaltflächen | Nur eine Auswahl |
| Checkbox | Kontrollkästchen | true/false |
| ComboBox | Kombinationsfeld | Text + Dropdown |
| FileUpload | Datei-Upload | Dateiauswahl |
| Signature | Signaturfeld | Digitale Unterschrift |
| Image | Bildfeld | PNG/JPG |
| Hidden | Verstecktes Feld | Nicht sichtbar |
| Label | Nur-Lese-Text | Statischer Text |
| Custom | Benutzerdefiniert | Plugin-basiert |

## Validierungsregeln

### Feldebene

```json
{
  "id": "document-title",
  "label": "Dokumententitel",
  "type": "Text",
  "isRequired": true,
  "maxLength": 255,
  "minLength": 5,
  "pattern": "^[A-Za-z0-9\\s-]+$"
}
```

### Zahlfelder

```json
{
  "id": "retention-period",
  "type": "Number",
  "minValue": 0,
  "maxValue": 99,
  "defaultValue": 10
}
```

### Custom Validierung

```csharp
var submission = await formService.ValidateFormAsync(template, formData);
if (submission.ValidationErrors.Count > 0)
{
    foreach (var error in submission.ValidationErrors)
    {
        Console.WriteLine($"Fehler in {error.Key}: {error.Value}");
    }
}
```

## Testdaten

### Verfügbare Test-Submissions

1. **PDV VIS 5 Standard** - Vollständige Dokumentenverwaltung
   - Alle Felder gefüllt
   - Status: Valid
   - Mit Genehmigung

2. **Personaldokument** - HR-Maske
   - Mitarbeiter-ID, Name, Geburtsdatum
   - Vereinfachte Struktur

3. **Vertrag** - Contract Management
   - Vertragsnummer, Partner, Wert
   - Datum-Bereich

### Test-Generierung

```csharp
var testData = new FormTestDataGenerator();

// Muster abrufen
var sample = testData.GeneratePDVSampleSubmission();

// Alle Test-Submissionen
var allTests = testData.GenerateAllTestSubmissions();

// Validierungstests
var valid = FormValidationTestData.ValidSubmission;
var invalid = FormValidationTestData.InvalidSubmissionMissingRequired;
var security = FormValidationTestData.SecurityTestSQLInjection;
```

## Datenbankintegration

### Mapping-Konfiguration

```csharp
var mapping = new FormDatabaseMapping
{
    TemplateId = "pdv-vis5-document",
    TableName = "documents",
    SchemaName = "themis_documents",
    FieldMappings = new Dictionary<string, string>
    {
        { "document-number", "doc_number" },
        { "document-title", "title" },
        { "document-type", "type_id" },
        // ... weitere Mappings
    }
};
```

### Daten-Mapping

```csharp
var mappedData = await mappingService.MapFormDataToDatabaseAsync(
    "pdv-vis5-document",
    submission
);
// Result: { "doc_number": "MU123456", "title": "...", ... }
```

## Implementierungsschritte

### 1. Initialisierung im DI-Container

```csharp
services.AddSingleton<IFormTemplateService, FormTemplateService>();
services.AddSingleton<IFormConfigurationLoader, FormConfigurationLoader>();
services.AddSingleton<IFormDatabaseMappingService, FormDatabaseMappingService>();
services.AddSingleton<IFormTestDataService, FormTestDataService>();
services.AddSingleton<IFormManagementService, EnhancedFormManagementService>();
```

### 2. Template laden

```csharp
var loader = serviceProvider.GetRequiredService<IFormConfigurationLoader>();
var template = await loader.LoadFromJsonAsync("Config/pdv-vis5-document.json");
```

### 3. Formular rendern

```csharp
foreach (var section in template.Sections)
{
    Console.WriteLine($"[{section.Title}]");
    foreach (var field in section.Fields)
    {
        Console.WriteLine($"  - {field.Label} ({field.Type})");
    }
}
```

### 4. Validierung & Absendung

```csharp
var formData = new Dictionary<string, object>
{
    { "document-number", "MU123456" },
    { "document-title", "Test Document" },
    // ... weitere Felder
};

var submission = await formService.ValidateFormAsync(template, formData);
if (submission.Status == "Valid")
{
    await formService.SubmitFormAsync(submission);
}
```

## Verwaltete Module

### Services integriert
- ✅ FormTemplateService - Formular-Management
- ✅ FormConfigurationLoader - YAML/JSON-Laden
- ✅ FormDatabaseMappingService - DB-Integration
- ✅ FormTestDataService - Test-Daten
- ✅ EnhancedFormManagementService - Erweiterte Verwaltung

### Noch zu implementieren (optional)
- PDV VIS 5 Spezifische Validatoren
- Advanced Permission/ACL auf Feldebene
- Formular-Versioning & Change-Tracking
- Export (CSV, PDF, XML)
- Conditional Field Visibility (Show/Hide basierend auf anderen Feldern)

## Performance & Skalierbarkeit

- **In-Memory Caching**: Templates werden gecacht
- **Lazy Loading**: Felder nur bei Bedarf laden
- **Batch Operations**: Mehrere Templates gleichzeitig laden
- **Async/Await**: Vollständige async-Unterstützung

## Sicherheit

### Implementierte Maßnahmen
- ✅ Pattern Validation (Regex)
- ✅ Length Constraints
- ✅ Type Validation
- ✅ Required Field Checks
- ✅ Input Sanitization (Basis)

### Testcases verfügbar
- SQL Injection Test
- XSS Attack Test
- Boundary Value Tests
- Constraint Violation Tests

## Fehlerbehandlung

```csharp
try
{
    var template = await formService.GetTemplateAsync("invalid-id");
    if (template == null)
        Console.WriteLine("Template nicht gefunden");
}
catch (Exception ex)
{
    Console.WriteLine($"Fehler: {ex.Message}");
}
```

## Konfigurationsdateien

### Verfügbare Templates

```
Config/FormTemplates/
  ├── pdv-vis5-document.json    # PDV VIS 5 Standard
  ├── pdv-vis5-document.yaml    # YAML-Variante
  ├── simple-document.json      # Vereinfachte Maske
  ├── personnel-document.json   # HR-Maske
  └── contract.json             # Vertrag-Maske
```

## Zukunftserweiterungen

1. **Template Versioning**: Versionskontrolle für Templates
2. **A/B Testing**: Mehrere Template-Varianten
3. **Audit Trail**: Tracking aller Änderungen
4. **Webhook Integration**: Events bei Absendung
5. **Machine Learning**: Automatische Feldgenerierung basierend auf Dokumenttyp
6. **Mobile Forms**: Responsive Design für Mobile
7. **Offline Support**: Lokale Speicherung & Sync

## Literatur & Standards

- **PDV VIS 5**: Vergabe- und Vertragsordnung für Leistungen - Vergabeverfahren
- **DSGVO**: Datenschutz-Grundverordnung (Feldvalidierung & Sicherheit)
- **XSD/JSON Schema**: Form Definition Standards
- **OASIS**: OpenDocument Format Compatibility

## Kontakt & Support

Für Fragen und Issues: Siehe Projekt-Repository
