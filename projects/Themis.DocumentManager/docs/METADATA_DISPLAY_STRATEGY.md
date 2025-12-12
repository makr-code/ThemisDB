# Strategie: Kompakte & Intelligente Metadaten-Darstellung

**Datum:** 11. Dezember 2025  
**Ziel:** Platzsparende, fachlich strukturierte Metadaten-Anzeige mit automatischer Badge-Gruppierung

---

## Problem

Umfangreiche Metadaten (50+ Felder) führen zu:
- Unübersichtlichen, langen Formularen
- Vielen leeren Feldern (Nutzer füllt nur 10-20% aus)
- Verlust der Übersicht über relevante Informationen
- Scroll-Fatigue bei großen Dokumenten

---

## Lösung: 3-Stufiges Darstellungskonzept

### Stufe 1: Smart Badge Summary (Kompakt-Ansicht)
**Standard-Anzeige:** Nur ausgefüllte Metadaten als farbliche Badges

```
┌─────────────────────────────────────────────────────────────┐
│ Dokument: Stellungnahme GV078/22                           │
├─────────────────────────────────────────────────────────────┤
│ 📅 12.11.2025  🏢 T26  📁 GV078/22  ⚡ Offen  🔥 Hoch      │
│ 👤 Max Mustermann  📋 Stellungnahme  ⏰ Frist: 15.12.2025  │
│                                                             │
│ [+ Weitere Metadaten anzeigen]  [✏️ Bearbeiten]            │
└─────────────────────────────────────────────────────────────┘
```

**Vorteile:**
- Sofort sichtbare Kerninformationen
- Farbcodierung nach Fachbereich (siehe Badge-System)
- Kein Scrollen nötig
- Klickbare Badges zum Filtern

### Stufe 2: Grouped Accordion View (Erweitert)
**Bei Klick auf "Weitere Metadaten":** Gruppierte, kollabierbare Sektionen

```
┌─────────────────────────────────────────────────────────────┐
│ ▼ Basis-Informationen (5 ausgefüllt)                       │
│   📅 Datum: 12.11.2025          📁 Aktenzeichen: GV078/22  │
│   📋 Typ: Stellungnahme         ⚡ Status: Offen           │
│   🔥 Priorität: Hoch                                        │
├─────────────────────────────────────────────────────────────┤
│ ▼ Beteiligte (2 ausgefüllt)                                │
│   👤 Ersteller: Max Mustermann  🏢 Abteilung: T26          │
├─────────────────────────────────────────────────────────────┤
│ ▶ Fristen & Termine (1 ausgefüllt)                         │
├─────────────────────────────────────────────────────────────┤
│ ▶ Erweiterte Informationen (0 ausgefüllt) [ausgeblendet]   │
├─────────────────────────────────────────────────────────────┤
│ ▶ Technische Details (0 ausgefüllt) [ausgeblendet]         │
└─────────────────────────────────────────────────────────────┘
```

**Intelligentes Verhalten:**
- Sektionen mit 0 ausgefüllten Feldern: **initial collapsed**
- Sektionen mit 1+ ausgefüllten Feldern: **initial expanded**
- Badge-Count im Header zeigt Füllgrad
- Leere Felder **innerhalb** expandierter Sektionen: **ausgeblendet**
- Button "Alle Felder anzeigen" pro Sektion

### Stufe 3: Full Edit Mode (Vollständig)
**Bei Klick auf "Bearbeiten" oder "Alle Felder anzeigen":** Alle Felder sichtbar

```
┌─────────────────────────────────────────────────────────────┐
│ ▼ Basis-Informationen                                       │
│   📅 Datum: [12.11.2025]        📁 Aktenzeichen: [GV078/22]│
│   📋 Typ: [Stellungnahme ▼]     ⚡ Status: [Offen ▼]       │
│   🔥 Priorität: [Hoch ▼]        📍 Standort: [________]    │
│   🏷️ Schlagworte: [________]    🔗 Referenz: [________]    │
└─────────────────────────────────────────────────────────────┘
```

**Features:**
- Alle Felder editierbar (auch leere)
- Inline-Validierung
- Auto-Completion für bekannte Werte
- Badge-Vorschau während Eingabe
- "Nur ausgefüllte Felder" Toggle zum Zurückschalten

---

## Fachliche Badge-Gruppierung

### Kategorie-System (12 Hauptgruppen)

#### 1. **Zeitbezug** (Blau: #E3F2FD)
- 📅 Datum
- ⏰ Frist
- 🕒 Bearbeitungszeit
- 📆 Wiedervorlage

#### 2. **Organisation** (Orange: #FFF3E0)
- 🏢 Abteilung (T26, T11, etc.)
- 🏛️ Behörde
- 🌍 Standort
- 📍 Organisationseinheit

#### 3. **Vorgangsbezug** (Lila: #F3E5F5)
- 📁 Aktenzeichen
- 📋 Vorgangstyp (STN, GEN, etc.)
- 🔗 Referenz-AZ
- 📎 Anhänge

#### 4. **Status & Workflow** (Gelb: #FFF9C4)
- ⚡ Status (Offen, In Bearbeitung, Abgeschlossen)
- ✅ Genehmigungsstatus
- 🔄 Workflow-Phase
- ⏳ Bearbeitungsstand

#### 5. **Priorität & Relevanz** (Rot: #FFEBEE)
- 🔥 Priorität (Hoch, Normal, Niedrig)
- ⚠️ Dringlichkeit
- 💡 Wichtigkeit
- 🎯 Kritikalität

#### 6. **Personen** (Grün: #E8F5E9)
- 👤 Ersteller
- 👥 Bearbeiter
- ✍️ Unterschrift
- 📧 Kontakt

#### 7. **Rechtsgrundlagen** (Indigo: #E8EAF6)
- ⚖️ Rechtsgrundlage
- 📜 Gesetz
- 📖 Verordnung
- 🔖 Vorschrift

#### 8. **Finanzen** (Teal: #E0F2F1)
- 💰 Betrag
- 💳 Kostenstelle
- 📊 Budget
- 💵 PSP-Element

#### 9. **Räumlicher Bezug** (Lime: #F9FBE7)
- 🗺️ Adresse
- 📍 Koordinaten
- 🏘️ Stadtteil
- 🌐 Region

#### 10. **Thematik** (Pink: #FCE4EC)
- 🏷️ Schlagworte
- 📚 Kategorien
- 🔍 Tags
- 💬 Themen

#### 11. **Technisch** (Grey: #ECEFF1)
- 🔒 Vertraulichkeit
- 🔐 Verschlüsselung
- 📋 Version
- 🆔 ID

#### 12. **Aktionen** (Deep Orange: #FBE9E7)
- ✔️ Genehmigung
- ❌ Ablehnung
- 📤 Weiterleitung
- 🔔 Benachrichtigung

---

## Implementierungs-Komponenten

### 1. MetadataCompactDisplay.cs
**Zweck:** Badge-basierte Kompakt-Ansicht

```csharp
public class MetadataCompactDisplay : UserControl
{
    public List<MetadataBadge> Badges { get; set; }
    public event EventHandler? ShowDetailsRequested;
    public event EventHandler? EditRequested;
    
    // Rendert nur ausgefüllte Metadaten als Badges
    public void RenderCompactView(DocumentMetadataBinding metadata);
    
    // Gruppiert Badges nach Kategorie
    private Dictionary<string, List<MetadataBadge>> GroupBadgesByCategory();
}
```

### 2. MetadataGroupedAccordion.cs
**Zweck:** Erweiterte gruppierte Ansicht mit Auto-Collapse

```csharp
public class MetadataGroupedAccordion : UserControl
{
    public List<MetadataFieldGroup> FieldGroups { get; set; }
    public CollapseStrategy Strategy { get; set; } = CollapseStrategy.HideEmptySections;
    
    // Rendert Accordion mit intelligenten Expand/Collapse
    public void RenderGroupedView(DocumentMetadataBinding metadata);
    
    // Bestimmt initial expanded/collapsed pro Gruppe
    private bool ShouldExpandGroup(MetadataFieldGroup group);
    
    // Blendet leere Felder innerhalb Gruppe aus
    private void HideEmptyFields(MetadataFieldGroup group);
}

public enum CollapseStrategy
{
    HideEmptySections,      // Sektionen mit 0 Feldern ausblenden
    HideEmptyFields,        // Leere Felder innerhalb Sektionen ausblenden
    ShowAllCollapsed,       // Alle Sektionen initial collapsed
    ShowAllExpanded         // Alle Sektionen initial expanded
}
```

### 3. MetadataFieldGroup.cs
**Zweck:** Gruppierungs-Modell

```csharp
public class MetadataFieldGroup
{
    public string Id { get; set; }
    public string Title { get; set; }              // "Basis-Informationen"
    public string Icon { get; set; }               // "📋"
    public string ColorCategory { get; set; }      // "Organization"
    public List<MetadataField> Fields { get; set; }
    public int FilledFieldCount => Fields.Count(f => !string.IsNullOrEmpty(f.CurrentValue));
    public bool IsEmpty => FilledFieldCount == 0;
    public bool IsExpanded { get; set; } = true;
    public int DisplayOrder { get; set; }
}
```

### 4. SmartMetadataLayoutEngine.cs
**Zweck:** Automatische Feld-Gruppierung & Layout-Optimierung

```csharp
public class SmartMetadataLayoutEngine
{
    // Analysiert Metadaten und erstellt optimale Gruppierung
    public List<MetadataFieldGroup> CreateOptimalLayout(
        DocumentMetadataBinding metadata,
        MetadataLayoutConfig config);
    
    // Erkennt fachliche Zusammenhänge
    private MetadataFieldGroup DetectFieldGroup(MetadataField field);
    
    // Sortiert Gruppen nach Relevanz (ausgefüllte zuerst)
    private List<MetadataFieldGroup> SortByRelevance(List<MetadataFieldGroup> groups);
}
```

### 5. MetadataBadgeAggregator.cs
**Zweck:** Badge-Generierung aus Metadaten

```csharp
public class MetadataBadgeAggregator
{
    private readonly IMetadataBadgeService _badgeService;
    
    // Erstellt Badges nur für ausgefüllte Felder
    public List<MetadataBadge> CreateBadgesFromMetadata(
        DocumentMetadataBinding metadata,
        BadgeDisplayMode mode = BadgeDisplayMode.FilledOnly);
    
    // Gruppiert Badges nach Kategorie für kompakte Darstellung
    public Dictionary<BadgeCategory, List<MetadataBadge>> GroupBadges(
        List<MetadataBadge> badges);
}

public enum BadgeDisplayMode
{
    FilledOnly,      // Nur ausgefüllte Felder
    All,             // Alle Felder (auch leer)
    Required,        // Nur Pflichtfelder
    Priority         // Nach Priorität gefiltert
}
```

---

## Konfiguration: MetadataDisplayConfig

```csharp
public class MetadataDisplayConfig
{
    // Kompakt-Ansicht
    public int MaxBadgesInCompactView { get; set; } = 10;
    public bool ShowBadgeIcons { get; set; } = true;
    public bool EnableBadgeFiltering { get; set; } = true;
    
    // Gruppierte Ansicht
    public CollapseStrategy CollapseStrategy { get; set; } = CollapseStrategy.HideEmptySections;
    public bool ShowFieldCountInGroupHeader { get; set; } = true;
    public bool HighlightRequiredFields { get; set; } = true;
    
    // Edit-Modus
    public bool EnableInlineValidation { get; set; } = true;
    public bool ShowAutoCompletion { get; set; } = true;
    public bool ShowBadgePreview { get; set; } = true;
    
    // Feld-Gruppen (Reihenfolge & Sichtbarkeit)
    public List<FieldGroupConfig> FieldGroups { get; set; } = new()
    {
        new() { Id = "basis", Title = "Basis-Informationen", Order = 1, Icon = "📋" },
        new() { Id = "beteiligte", Title = "Beteiligte", Order = 2, Icon = "👥" },
        new() { Id = "fristen", Title = "Fristen & Termine", Order = 3, Icon = "⏰" },
        new() { Id = "workflow", Title = "Workflow & Status", Order = 4, Icon = "🔄" },
        new() { Id = "finanzen", Title = "Finanzen", Order = 5, Icon = "💰" },
        new() { Id = "erweitert", Title = "Erweiterte Informationen", Order = 6, Icon = "📚" },
        new() { Id = "technisch", Title = "Technische Details", Order = 7, Icon = "⚙️" }
    };
}

public class FieldGroupConfig
{
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public int Order { get; set; }
    public bool IsVisible { get; set; } = true;
    public bool IsExpandedByDefault { get; set; } = true;
}
```

---

## UI-Integration in MainWindow

### Kompakt-Ansicht (Standard)
```xml
<Border Grid.Row="0" Background="White" Padding="10">
    <local:MetadataCompactDisplay 
        x:Name="CompactMetadataDisplay"
        Badges="{Binding CurrentDocumentBadges}"
        ShowDetailsRequested="OnShowMetadataDetails"
        EditRequested="OnEditMetadata"/>
</Border>
```

### Erweiterte Ansicht (Flyout/Panel)
```xml
<Border Grid.Row="1" x:Name="MetadataDetailsPanel" Visibility="Collapsed">
    <local:MetadataGroupedAccordion
        x:Name="GroupedMetadataDisplay"
        FieldGroups="{Binding MetadataGroups}"
        Strategy="HideEmptySections"/>
</Border>
```

### Full Edit (Modal/Sidesheet)
```xml
<Border x:Name="MetadataEditPanel" Visibility="Collapsed">
    <local:SmartFormRenderer
        CurrentTemplate="{Binding MetadataFormTemplate}"
        SmartFormSubmitted="OnMetadataSaved"/>
</Border>
```

---

## Workflow-Beispiel

### 1. Dokument öffnen
```
┌─────────────────────────────────────────────┐
│ 📄 Stellungnahme_GV078_22.docx             │
├─────────────────────────────────────────────┤
│ 📅 12.11.2025  🏢 T26  📁 GV078/22         │
│ ⚡ Offen  🔥 Hoch  👤 Max Mustermann        │
│                                             │
│ [+ Weitere Metadaten]  [✏️ Bearbeiten]     │
└─────────────────────────────────────────────┘
```

### 2. "Weitere Metadaten" klicken
```
▼ Basis-Informationen (5 ausgefüllt)
  📅 Datum: 12.11.2025
  📁 Aktenzeichen: GV078/22
  ...

▼ Beteiligte (2 ausgefüllt)
  👤 Ersteller: Max Mustermann
  🏢 Abteilung: T26

▶ Fristen & Termine (1 ausgefüllt)

[Sektionen mit 0 Feldern ausgeblendet]
```

### 3. "Bearbeiten" klicken
```
▼ Basis-Informationen
  📅 Datum: [12.11.2025] ✓
  📁 Aktenzeichen: [GV078/22] ✓
  📋 Typ: [Stellungnahme ▼] ✓
  ⚡ Status: [Offen ▼] ✓
  🔥 Priorität: [Hoch ▼] ✓
  📍 Standort: [________]        ← neu sichtbar
  🏷️ Schlagworte: [________]     ← neu sichtbar
  
[Toggle: ☑️ Nur ausgefüllte Felder anzeigen]
```

---

## Platzeinsparung: Vorher/Nachher

### Vorher (traditionell)
```
50 Felder × 60px = 3000px vertikale Höhe
→ Nutzer muss 2-3 Bildschirmhöhen scrollen
→ 40 leere Felder sichtbar (80% Leerraum)
```

### Nachher (intelligent)
```
Kompakt-Ansicht: 10 Badges × 30px = 300px (90% Reduktion!)
Erweitert: 3 Sektionen × 200px = 600px (80% Reduktion)
→ Alles auf 1 Bildschirmhöhe
→ 0 leere Felder in Kompakt/Erweitert
```

---

## Performance-Optimierung

### Lazy Loading
```csharp
// Nur sichtbare Sektionen rendern
if (group.IsExpanded)
{
    RenderGroupFields(group);
}
else
{
    // Placeholder für collapsed section
    RenderCollapsedHeader(group);
}
```

### Virtualisierung
```csharp
// Bei sehr vielen Feldern (100+)
var virtualizedPanel = new VirtualizingStackPanel
{
    VirtualizationMode = VirtualizationMode.Recycling
};
```

### Caching
```csharp
// Badge-Generierung cachen
private Dictionary<string, List<MetadataBadge>> _badgeCache = new();

public List<MetadataBadge> GetBadges(string documentId)
{
    if (!_badgeCache.ContainsKey(documentId))
    {
        _badgeCache[documentId] = _aggregator.CreateBadgesFromMetadata(metadata);
    }
    return _badgeCache[documentId];
}
```

---

## Barrierefreiheit

- **Keyboard-Navigation:** Tab-Reihenfolge durch Badges → Details → Edit
- **Screen Reader:** ARIA-Labels für Badge-Kategorien
- **Kontrast:** Alle Farben WCAG AA-konform
- **Fokus-Indikator:** Deutlich sichtbar bei Tastatur-Navigation

---

## Zusammenfassung

**3-Stufen-Konzept:**
1. **Kompakt** (Standard): Nur Badges für ausgefüllte Felder
2. **Gruppiert** (Erweitert): Accordion mit Auto-Collapse für leere Sektionen
3. **Vollständig** (Edit): Alle Felder editierbar, Toggle für Leer-Filter

**Vorteile:**
- ✅ 80-90% Platzeinsparung
- ✅ Sofortige Übersicht über Kerninformationen
- ✅ Fachliche Strukturierung (12 Badge-Kategorien)
- ✅ Null leere Felder in Standard-Ansicht
- ✅ Intelligentes Auto-Expand nur für relevante Sektionen
- ✅ Integration mit bestehendem SmartFormRenderer

**Nächste Schritte:**
1. `MetadataCompactDisplay.cs` implementieren
2. `MetadataGroupedAccordion.cs` implementieren
3. `SmartMetadataLayoutEngine.cs` implementieren
4. Integration in MainWindow/DocumentDetailView
5. User-Testing mit echten Vorgängen
