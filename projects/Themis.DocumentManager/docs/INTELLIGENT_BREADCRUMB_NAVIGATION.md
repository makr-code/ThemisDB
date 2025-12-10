# Intelligente Positionsanzeige / Breadcrumb Navigation

**Datum:** 2025-12-10  
**Feature:** AI-powered intelligent breadcrumb navigation  
**Status:** ✅ Implementiert

---

## Anforderung

> "Über der Content-View soll eine intelligente Positionsanzeige / Historie, im Sinne: **Behörde > Ablage > Akte > Vorgang > Dokument** dargestellt werden. Jedes Element verfügt über eine Dropdown-Fähigkeit zu verwandten Entitäten zu springen. Der Algorythmus soll aus der Benutzerhistorie das springen zu verwandten/relevanten Entitäten prognostizieren und vorschlagen."

---

## Übersicht

Die **Intelligente Positionsanzeige** ist eine KI-gestützte Breadcrumb-Navigation, die:
1. Die aktuelle Position in der Hierarchie anzeigt
2. Dropdown-Menüs mit verwandten Entitäten bietet
3. Vorschläge basierend auf Benutzerhistorie und KI macht
4. Relevanz-Scores für intelligente Sortierung nutzt

---

## Hierarchie-Struktur

```
Behörde (Authority)
    └─> Ablage (Repository)
           └─> Akte (File)
                  └─> Vorgang (Process)
                         └─> Dokument (Document)
```

**Beispiel:**
```
🏛️ Stadtverwaltung München › 📁 Bauamt › 📂 Baugenehmigungen 2025 › 📋 Antrag Mustermann › 📄 Bauplan_Entwurf_v2.pdf
```

---

## KI-Algorithmus für Vorschläge

### 1. Relevanz-Berechnung

Der Algorithmus berechnet Relevanz-Scores (0-1) basierend auf mehreren Faktoren:

**Faktoren:**
```csharp
public class RelevanceCalculation
{
    // Weight factors (sum = 1.0)
    private const double HISTORY_WEIGHT = 0.35;      // User navigation history
    private const double FREQUENCY_WEIGHT = 0.25;    // Access frequency
    private const double SIMILARITY_WEIGHT = 0.20;   // Semantic similarity
    private const double TEMPORAL_WEIGHT = 0.20;     // Temporal patterns
    
    public double CalculateRelevance(Entity entity, UserHistory history)
    {
        var historyScore = CalculateHistoryScore(entity, history);
        var frequencyScore = CalculateFrequencyScore(entity);
        var similarityScore = CalculateSimilarityScore(entity);
        var temporalScore = CalculateTemporalScore(entity);
        
        return (historyScore * HISTORY_WEIGHT) +
               (frequencyScore * FREQUENCY_WEIGHT) +
               (similarityScore * SIMILARITY_WEIGHT) +
               (temporalScore * TEMPORAL_WEIGHT);
    }
}
```

### 2. History-Based Prediction

**Sequenzanalyse:**
```csharp
// Analyse: Welche Entitäten werden häufig zusammen aufgerufen?
// Beispiel: Wenn Benutzer Dokument A öffnet, folgt häufig Dokument B

public class SequenceAnalyzer
{
    public Dictionary<string, List<Transition>> AnalyzeSequences(UserHistory history)
    {
        var sequences = new Dictionary<string, List<Transition>>();
        
        for (int i = 0; i < history.Entries.Count - 1; i++)
        {
            var from = history.Entries[i].EntityId;
            var to = history.Entries[i + 1].EntityId;
            
            if (!sequences.ContainsKey(from))
                sequences[from] = new List<Transition>();
            
            sequences[from].Add(new Transition
            {
                To = to,
                Timestamp = history.Entries[i + 1].Timestamp
            });
        }
        
        return sequences;
    }
}
```

### 3. Semantic Similarity

**Vektorbasierte Ähnlichkeit:**
```csharp
// Verwendung von Embeddings für semantische Ähnlichkeit
public class SemanticSimilarity
{
    public double CalculateSimilarity(Entity entity1, Entity entity2)
    {
        // Vector embeddings aus Titel, Beschreibung, Tags, etc.
        var embedding1 = GetEmbedding(entity1);
        var embedding2 = GetEmbedding(entity2);
        
        // Cosine Similarity
        return CosineSimilarity(embedding1, embedding2);
    }
    
    private double CosineSimilarity(double[] vec1, double[] vec2)
    {
        var dotProduct = vec1.Zip(vec2, (a, b) => a * b).Sum();
        var magnitude1 = Math.Sqrt(vec1.Sum(x => x * x));
        var magnitude2 = Math.Sqrt(vec2.Sum(x => x * x));
        
        return dotProduct / (magnitude1 * magnitude2);
    }
}
```

### 4. Temporal Patterns

**Zeitbasierte Muster:**
```csharp
public class TemporalAnalyzer
{
    public double CalculateTemporalRelevance(Entity entity, DateTime currentTime)
    {
        if (!entity.LastAccessedAt.HasValue)
            return 0.0;
        
        var timeDiff = currentTime - entity.LastAccessedAt.Value;
        
        // Recency decay function
        // Recent accesses get higher scores
        var recencyScore = Math.Exp(-timeDiff.TotalHours / 24.0);
        
        // Day-of-week patterns
        var dayPattern = GetDayOfWeekPattern(entity, currentTime.DayOfWeek);
        
        return (recencyScore * 0.7) + (dayPattern * 0.3);
    }
}
```

---

## CQRS Implementation

### Query 1: GetNavigationPathQuery

**Zweck:** Aktuelle Positionsanzeige aufbauen

```csharp
var query = new GetNavigationPathQuery
{
    EntityId = "doc123",
    EntityType = EntityType.Document
};

var path = await _mediator.Send(query);

// Result:
// [0] Behörde: Stadtverwaltung München
// [1] Ablage: Bauamt
// [2] Akte: Baugenehmigungen 2025
// [3] Vorgang: Antrag Mustermann
// [4] Dokument: Bauplan_Entwurf_v2.pdf (current)
```

### Query 2: GetRelatedEntitiesQuery

**Zweck:** KI-Vorschläge für Dropdown

```csharp
var query = new GetRelatedEntitiesQuery
{
    EntityId = "doc123",
    EntityType = EntityType.Document,
    UserId = "current-user"
};

var result = await _mediator.Send(query);

// Result Groups:
// Group 1: "Verwandte Dokumente"
//   - Bauplan_Entwurf_v1.pdf (Score: 0.95, "Frühere Version")
//   - Statik_Berechnung.pdf (Score: 0.87, "Häufig zusammen geöffnet")
//   - Genehmigung_Nachbar.pdf (Score: 0.76, "Ähnlicher Kontext")
//
// Group 2: "Verwandte Vorgänge"
//   - Antrag Schmidt (Score: 0.82, "Ähnlicher Vorgangstyp")
//
// Group 3: "Häufig verwendete Akten"
//   - Baugenehmigungen 2024 (Score: 0.71, "Vorjahresakte")
```

---

## UI-Komponenten

### Breadcrumb View

```
┌────────────────────────────────────────────────────────────────────────┐
│ 🏠 › 🏛️ Stadtverwaltung München ▼ › 📁 Bauamt ▼ › 📂 Baugenehmigungen│
│    2025 ▼ › 📋 Antrag Mustermann ▼ › 📄 Bauplan_Entwurf_v2.pdf       │
└────────────────────────────────────────────────────────────────────────┘
```

### Dropdown-Menü (bei Klick auf ▼)

```
Behörde: Stadtverwaltung München
├─ Verwandte Behörden
│  └─ (keine Vorschläge auf dieser Ebene)
│
Ablage: Bauamt
├─ Andere Ablagen in dieser Behörde
│  ├─ 🔥 📁 Ordnungsamt (Score: 0.92, "Kürzlich verwendet")
│  └─ ⭐ 📁 Finanzamt (Score: 0.78, "Häufig verwendet")
│
Akte: Baugenehmigungen 2025
├─ Verwandte Akten
│  ├─ 🔥 📂 Baugenehmigungen 2024 (Score: 0.95) [Häufig]
│  └─ ⭐ 📂 Baugenehmigungen Sonderverfahren (Score: 0.82)
│
Vorgang: Antrag Mustermann
├─ Verwandte Vorgänge
│  ├─ 🔥 📋 Antrag Schmidt (Score: 0.87, "Ähnlich") [Häufig]
│  └─ ⭐ 📋 Antrag Weber (Score: 0.76)
│
Dokument: Bauplan_Entwurf_v2.pdf
├─ Verwandte Dokumente
│  ├─ 🔥 📄 Bauplan_Entwurf_v1.pdf (Score: 0.95, "Frühere Version")
│  ├─ ⭐ 📄 Statik_Berechnung.pdf (Score: 0.87, "Häufig zusammen")
│  └─ ▶️ 📄 Genehmigung_Nachbar.pdf (Score: 0.76, "Ähnlich")
│
├─ Verwandte Vorgänge
│  └─ ⭐ 📋 Antrag Schmidt (Score: 0.82, "Gleiche Kategorie")
│
└─ Häufig verwendete Akten
   └─ ⭐ 📂 Baugenehmigungen 2024 (Score: 0.71)
```

**Legende:**
- 🔥 = Sehr relevant (Score ≥ 0.9)
- ⭐ = Hoch relevant (Score ≥ 0.75)
- ▶️ = Relevant (Score ≥ 0.6)
- [Häufig] = Badge für häufig verwendete Entitäten

---

## ViewModel Implementation

```csharp
public class IntelligentBreadcrumbViewModel : ObservableObject
{
    private readonly IMediator _mediator;

    [ObservableProperty]
    private ObservableCollection<BreadcrumbItemViewModel> _breadcrumbItems;

    [RelayCommand]
    private async Task LoadNavigationPathAsync(NavigationContext context)
    {
        // 1. Load navigation path
        var pathQuery = new GetNavigationPathQuery
        {
            EntityId = context.EntityId,
            EntityType = context.EntityType
        };
        var path = await _mediator.Send(pathQuery);

        // 2. Build breadcrumb items
        foreach (var item in path.Items)
        {
            var breadcrumbItem = new BreadcrumbItemViewModel
            {
                Id = item.Id,
                Name = item.Name,
                Type = item.Type,
                Icon = GetIconForEntityType(item.Type)
            };

            // 3. Load AI-powered related entities for dropdown
            if (!item.IsCurrentItem)
            {
                await LoadRelatedEntitiesAsync(breadcrumbItem);
            }

            BreadcrumbItems.Add(breadcrumbItem);
        }
    }

    private async Task LoadRelatedEntitiesAsync(BreadcrumbItemViewModel item)
    {
        var query = new GetRelatedEntitiesQuery
        {
            EntityId = item.Id,
            EntityType = item.Type,
            UserId = "current-user"
        };

        var result = await _mediator.Send(query);

        // Sort by relevance score (AI-predicted)
        foreach (var group in result.Groups)
        {
            var sortedEntities = group.Entities
                .OrderByDescending(e => e.RelevanceScore)
                .ToList();
            
            // Add to dropdown
            item.RelatedEntityGroups.Add(new RelatedEntityGroupViewModel
            {
                GroupName = group.GroupName,
                Entities = new ObservableCollection<RelatedEntityViewModel>(
                    sortedEntities.Select(e => new RelatedEntityViewModel
                    {
                        Id = e.Id,
                        Name = e.Name,
                        RelevanceScore = e.RelevanceScore,
                        RelevanceReason = e.RelevanceReason,
                        Icon = GetIconForEntityType(e.Type),
                        RelevanceIndicator = GetRelevanceIndicator(e.RelevanceScore)
                    })
                )
            });
        }
    }
}
```

---

## Integration in MainWindow

### Position über Content-View

```xml
<Grid Grid.Row="1" Grid.Column="1">
    <Grid.RowDefinitions>
        <RowDefinition Height="Auto"/>  <!-- Breadcrumb -->
        <RowDefinition Height="Auto"/>  <!-- Tab Bar -->
        <RowDefinition Height="*"/>     <!-- Content -->
    </Grid.RowDefinitions>
    
    <!-- Intelligent Breadcrumb Navigation -->
    <local:IntelligentBreadcrumbView Grid.Row="0" 
        DataContext="{Binding IntelligentBreadcrumbViewModel}"/>
    
    <!-- Tab Bar -->
    <Border Grid.Row="1" ...>
        <StackPanel x:Name="TabBar" .../>
    </Border>
    
    <!-- Content Area -->
    <TabControl Grid.Row="2" x:Name="CenterContent" .../>
</Grid>
```

---

## Verwendungsbeispiele

### Beispiel 1: Dokument öffnen

```csharp
// User öffnet Dokument
var context = new NavigationContext
{
    EntityId = "doc123",
    EntityType = EntityType.Document
};

await breadcrumbViewModel.LoadNavigationPathAsync(context);

// Breadcrumb zeigt:
// 🏛️ Stadtverwaltung München › 📁 Bauamt › 📂 Baugenehmigungen 2025 
// › 📋 Antrag Mustermann › 📄 Bauplan_Entwurf_v2.pdf

// Dropdown bei "Bauplan_Entwurf_v2.pdf" zeigt:
// - Verwandte Dokumente (3)
//   - 🔥 Bauplan_Entwurf_v1.pdf (0.95, "Frühere Version")
//   - ⭐ Statik_Berechnung.pdf (0.87, "Häufig zusammen geöffnet")
//   - ▶️ Genehmigung_Nachbar.pdf (0.76, "Ähnlicher Kontext")
```

### Beispiel 2: Navigation zu verwandter Entität

```csharp
// User klickt auf "Bauplan_Entwurf_v1.pdf" im Dropdown
// System navigiert automatisch und aktualisiert Breadcrumb

var newContext = new NavigationContext
{
    EntityId = "doc002",  // Bauplan_Entwurf_v1.pdf
    EntityType = EntityType.Document
};

await breadcrumbViewModel.LoadNavigationPathAsync(newContext);

// Neue Breadcrumb:
// 🏛️ Stadtverwaltung München › 📁 Bauamt › 📂 Baugenehmigungen 2025 
// › 📋 Antrag Mustermann › 📄 Bauplan_Entwurf_v1.pdf
```

---

## Vorteile

### 1. Intelligente Navigation
✅ KI-gestützte Vorschläge basierend auf:
- Benutzerhistorie
- Zugriffshäufigkeit
- Semantische Ähnlichkeit
- Zeitliche Muster

### 2. Erhöhte Produktivität
✅ Schneller Zugriff auf verwandte Entitäten
✅ Keine manuelle Suche notwendig
✅ Kontextbasierte Empfehlungen

### 3. Lernender Algorithmus
✅ Verbessert sich mit jeder Interaktion
✅ Personalisierte Vorschläge pro Benutzer
✅ Anpassung an Arbeitsgewohnheiten

### 4. Transparenz
✅ Relevanz-Scores sichtbar
✅ Gründe für Vorschläge angezeigt
✅ Badges für häufig verwendete Entitäten

---

## Zukünftige Erweiterungen

### Phase 2: Machine Learning Integration

```csharp
// Verwendung von ML.NET für bessere Vorhersagen
public class MLPredictionService
{
    private readonly MLContext _mlContext;
    private ITransformer _model;

    public double PredictRelevance(Entity entity, UserHistory history)
    {
        var features = ExtractFeatures(entity, history);
        var prediction = _model.Transform(features);
        
        return prediction.Score;
    }
    
    private Features ExtractFeatures(Entity entity, UserHistory history)
    {
        return new Features
        {
            AccessFrequency = entity.AccessCount,
            RecencyScore = CalculateRecency(entity.LastAccessedAt),
            SimilarityScore = CalculateSimilarity(entity),
            SequenceScore = CalculateSequencePattern(entity, history),
            DayOfWeek = (float)DateTime.Now.DayOfWeek,
            HourOfDay = DateTime.Now.Hour
        };
    }
}
```

### Phase 3: Deep Learning

```csharp
// Verwendung von LSTM für Sequenz-Vorhersage
public class LSTMNavigationPredictor
{
    public List<Entity> PredictNextEntities(List<NavigationEvent> sequence, int topK = 5)
    {
        // LSTM model trained on navigation sequences
        var predictions = _lstmModel.Predict(sequence);
        
        return predictions
            .OrderByDescending(p => p.Probability)
            .Take(topK)
            .Select(p => p.Entity)
            .ToList();
    }
}
```

---

## Zusammenfassung

✅ **Intelligente Positionsanzeige implementiert:**
- Hierarchische Breadcrumb: Behörde > Ablage > Akte > Vorgang > Dokument
- Dropdown-Menüs mit verwandten Entitäten
- AI-powered Vorschläge basierend auf Benutzerhistorie
- Relevanz-Scores und Gründe angezeigt

✅ **CQRS Architecture:**
- GetNavigationPathQuery - Pfad aufbauen
- GetRelatedEntitiesQuery - KI-Vorschläge laden

✅ **KI-Algorithmus:**
- History-based prediction
- Frequency analysis
- Semantic similarity
- Temporal patterns

✅ **UI/UX:**
- Visuelle Relevanz-Indikatoren (🔥⭐▶️)
- Badges für häufige Entitäten
- Gründe für Vorschläge
- Schnelle Navigation

---

**Erstellt:** 2025-12-10  
**Version:** 1.0 - AI-Powered Navigation  
**Status:** Production Ready
