# Phase 24 - Service Index

Technische Referenz für alle neuen Services in Phase 24.

---

## 🔗 **IGraphVisualizationService**

**Namespace**: `Themis.DocumentManager.Services`
**Implementation**: `GraphVisualizationService`
**Scope**: Singleton

### **Methoden**

#### Layout-Berechnung

##### `CalculateLayoutAsync(Graph graph, LayoutAlgorithm algorithm, CancellationToken)`
- **Returns**: `Task<LayoutResult>`
- **Beschreibung**: Universelle Layout-Berechnung basierend auf Algorithmus-Typ
- **Use Case**: Als Dispatcher für verschiedene Layout-Algorithmen

##### `CalculateForceDirectedLayoutAsync(Graph graph, ForceDirectedLayoutParams parameters, CancellationToken)`
- **Returns**: `Task<LayoutResult>`
- **Parameters**:
  - `graph`: Der zu layoutierende Graph
  - `parameters`: Force-Directed spezifische Parameter (optional)
- **Beschreibung**: Fruchterman-Reingold Force-Directed Layout mit Coulomb-Abstoßung & Hooke-Anziehung
- **Performance**: O(n²) pro Iteration, ~100-500 Iterationen
- **Best For**: Komplexe Netzwerk-Topologien, ästhetisch ansprechend
- **Example**:
```csharp
var params = new ForceDirectedLayoutParams 
{ 
    K = 100.0,
    Iterations = 1000,
    Use3D = true
};
var result = await service.CalculateForceDirectedLayoutAsync(graph, params);
```

##### `CalculateHierarchicalLayoutAsync(Graph graph, CancellationToken)`
- **Returns**: `Task<LayoutResult>`
- **Beschreibung**: Top-Down hierarchisches Layout via BFS
- **Performance**: O(n + m) linear
- **Best For**: Organigramme, DAGs, Tree-Strukturen
- **Example**:
```csharp
var result = await service.CalculateHierarchicalLayoutAsync(graph);
```

##### `CalculateCircularLayoutAsync(Graph graph, CancellationToken)`
- **Returns**: `Task<LayoutResult>`
- **Beschreibung**: Knoten auf Kreis mit gleichmäßiger Winkelverteilung
- **Performance**: O(n) sehr schnell
- **Best For**: Kleine Graphen, Überblicks-Visualisierung
- **Example**:
```csharp
var result = await service.CalculateCircularLayoutAsync(graph);
```

##### `CalculateRadialLayoutAsync(Graph graph, CancellationToken)`
- **Returns**: `Task<LayoutResult>`
- **Beschreibung**: Hub-and-Spoke mit zentral-Knoten + konzentrische Ringe
- **Performance**: O(n) linear
- **Best For**: Star-topologies, Social Networks mit Hub
- **Example**:
```csharp
var result = await service.CalculateRadialLayoutAsync(graph);
```

##### `CalculateKamadaKawaiLayoutAsync(Graph graph, CancellationToken)`
- **Returns**: `Task<LayoutResult>`
- **Beschreibung**: Distanzerhaltender Layout via Floyd-Warshall
- **Performance**: O(n³) sehr langsam für große Graphen
- **Best For**: Kleine Graphen mit wichtigen Distanzen
- **Example**:
```csharp
var result = await service.CalculateKamadaKawaiLayoutAsync(graph);
```

#### Clustering

##### `CalculateLouvainClusteringAsync(Graph graph)`
- **Returns**: `Task<List<GraphCluster>>`
- **Beschreibung**: Community Detection via BFS-basierte Connected Components
- **Performance**: O(n + m)
- **Returns**: Liste von Clustern mit automatischer Farb-Zuweisung
- **Example**:
```csharp
var clusters = await service.CalculateLouvainClusteringAsync(graph);
foreach (var cluster in clusters)
{
    Console.WriteLine($"{cluster.Name}: {cluster.NodeIds.Count} Knoten");
}
```

##### `CalculateKMeansClusteringAsync(Graph graph, int clusterCount)`
- **Returns**: `Task<List<GraphCluster>>`
- **Beschreibung**: K-Means Clustering (Struktur vorhanden, Implementierung pending)
- **Parameters**: 
  - `clusterCount`: Anzahl der gewünschten Cluster
- **Example**:
```csharp
var clusters = await service.CalculateKMeansClusteringAsync(graph, clusterCount: 5);
```

#### Analyse

##### `CalculateStatisticsAsync(Graph graph)`
- **Returns**: `Task<GraphStatistics>`
- **Beschreibung**: Umfassende Graph-Statistiken berechnen
- **Returns**: 
  - NodeCount, EdgeCount, Density, AverageDegree
  - DegreeCentrality pro Knoten
- **Example**:
```csharp
var stats = await service.CalculateStatisticsAsync(graph);
Console.WriteLine($"Dichte: {stats.Density:F4}");
Console.WriteLine($"Durchschn. Grad: {stats.AverageDegree:F2}");
```

##### `CalculateBetweennessCentralityAsync(Graph graph)`
- **Returns**: `Task<Dictionary<string, double>>` (NodeId → Centrality Score)
- **Beschreibung**: Betweenness Centrality (Wichtigkeit als Vermittler)
- **Example**:
```csharp
var centrality = await service.CalculateBetweennessCentralityAsync(graph);
var mostCentral = centrality.OrderByDescending(x => x.Value).First();
```

##### `CalculateDegreeCentralityAsync(Graph graph)`
- **Returns**: `Task<Dictionary<string, double>>`
- **Beschreibung**: Degree Centrality (Knoten-Grad normalisiert)
- **Example**:
```csharp
var centrality = await service.CalculateDegreeCentralityAsync(graph);
```

##### `CalculateClosenessCentralityAsync(Graph graph)`
- **Returns**: `Task<Dictionary<string, double>>`
- **Beschreibung**: Closeness Centrality (Durchschnitt-Entfernung)
- **Example**:
```csharp
var centrality = await service.CalculateClosenessCentralityAsync(graph);
```

#### Utilities

##### `GetOptimalCameraPosition(Graph graph, RenderingOptions options)`
- **Returns**: `Vector3D`
- **Beschreibung**: Berechnet optimale Kamera-Position für 3D-Darstellung
- **Example**:
```csharp
var cameraPos = service.GetOptimalCameraPosition(graph, options);
```

##### `GetConnectedComponent(Graph graph, string startNodeId)`
- **Returns**: `List<GraphNode>`
- **Beschreibung**: Findet zusammenhängende Komponente via BFS
- **Example**:
```csharp
var component = service.GetConnectedComponent(graph, "node123");
Console.WriteLine($"Komponente hat {component.Count} Knoten");
```

##### `GetShortestPath(Graph graph, string sourceId, string targetId)`
- **Returns**: `int` (Anzahl Hops)
- **Beschreibung**: Kürzeste Pfad-Länge via BFS (Dijkstra)
- **Example**:
```csharp
var distance = service.GetShortestPath(graph, source, target);
Console.WriteLine($"Shortest Path: {distance} Hops");
```

---

## 🗺️ **IOsmMapRenderer**

**Namespace**: `Themis.DocumentManager.Services`
**Implementation**: `OsmMapRenderer`
**Scope**: Singleton

### **Methoden**

##### `GenerateMapHtml(MapConfiguration config, List<GeoLayer> layers)`
- **Returns**: `string` (HTML)
- **Beschreibung**: Generiert komplette HTML + JavaScript für Leaflet-Karte
- **Includes**:
  - Leaflet.js CDN Links
  - Base-Layer Configuration
  - Layer-Controls
  - Legend
  - MarkerCluster Support
- **Example**:
```csharp
var html = renderer.GenerateMapHtml(mapConfig, layers);
webViewFrame.Navigate(new Uri("data:text/html," + Uri.EscapeDataString(html)));
```

##### `GenerateLayerJs(GeoLayer layer)`
- **Returns**: `string` (JavaScript Code)
- **Beschreibung**: Generiert JavaScript für einzelnen Layer
- **Supports**:
  - Markers
  - GeoJSON
  - Heatmap
  - Custom Layers
- **Example**:
```csharp
var layerJs = renderer.GenerateLayerJs(geoJsonLayer);
```

##### `GenerateFeaturePopup(GeoFeature feature)`
- **Returns**: `string` (HTML)
- **Beschreibung**: Generiert PopUp-HTML für Feature
- **Includes**:
  - Title, Description
  - Properties als DL (Definition List)
  - Styled Formatting
- **Example**:
```csharp
var popup = renderer.GenerateFeaturePopup(feature);
// Use in L.popup().setContent(popup)
```

##### `GenerateBaseLayerUrlAsync(string layerType)`
- **Returns**: `Task<string>` (URL Template)
- **Beschreibung**: Gibt Tile-Server URL für Layer-Type zurück
- **Supported Types**:
  - "OpenStreetMap"
  - "Satellite" (ESRI)
  - "Terrain" (ESRI)
  - "CartoDB"
  - "CartoDB Dark"
- **Example**:
```csharp
var url = await renderer.GenerateBaseLayerUrlAsync("Satellite");
```

---

## 📊 **Model Classes**

### **GraphModels.cs**

#### **GraphNode**
```csharp
public class GraphNode
{
    public string Id { get; set; }
    public string Label { get; set; }
    public GraphNodeType Type { get; set; }
    
    // 3D Position & Physik
    public Vector3D Position { get; set; }
    public Vector3D Velocity { get; set; }
    public double Radius { get; set; }
    public double Mass { get; set; }
    
    // Styling
    public string Color { get; set; }
    public string IconShape { get; set; }
    public int IconSize { get; set; }
    
    // Clustering
    public string ClusterId { get; set; }
    public List<string> ChildNodeIds { get; set; }
    
    // Data
    public Dictionary<string, object> Data { get; set; }
}
```

#### **Vector3D**
```csharp
public class Vector3D
{
    public double X { get; set; }
    public double Y { get; set; }
    public double Z { get; set; }
    
    // Methods
    public double DistanceTo(Vector3D other);
    public Vector3D Add(Vector3D other);
    public Vector3D Multiply(double scalar);
    public double Length { get; }
    public Vector3D Normalize();
}
```

#### **GraphEdge**
```csharp
public class GraphEdge
{
    public string Id { get; set; }
    public string SourceNodeId { get; set; }
    public string TargetNodeId { get; set; }
    
    public string Label { get; set; }
    public string RelationType { get; set; } // related, parent, child
    public double Strength { get; set; } // für Layout
    public double Weight { get; set; } // für gewichtete Graphen
    
    // Styling
    public string Color { get; set; }
    public int StrokeWidth { get; set; }
    public bool IsDirected { get; set; }
}
```

#### **Graph**
```csharp
public class Graph
{
    public string Id { get; set; }
    public string Name { get; set; }
    
    public List<GraphNode> Nodes { get; set; }
    public List<GraphEdge> Edges { get; set; }
    public GraphConfiguration Configuration { get; set; }
    public List<GraphCluster> Clusters { get; set; }
    
    // Properties
    public int NodeCount { get; }
    public int EdgeCount { get; }
    public double Density { get; }
    public double AverageDegree { get; }
    
    // Methods
    public GraphNode? GetNode(string nodeId);
    public List<GraphNode> GetNeighbors(string nodeId);
    public List<GraphEdge> GetIncomingEdges(string nodeId);
    public List<GraphEdge> GetOutgoingEdges(string nodeId);
    public int GetNodeDegree(string nodeId);
}
```

#### **LayoutResult**
```csharp
public class LayoutResult
{
    public bool IsConverged { get; set; }
    public int IterationCount { get; set; }
    public double FinalEnergy { get; set; }
    public TimeSpan ComputationTime { get; set; }
    public Dictionary<string, Vector3D> NodePositions { get; set; }
    public List<string> WarningsAndErrors { get; set; }
}
```

---

## 🔄 **Usage Patterns**

### **Pattern 1: Complete Graph Analysis**
```csharp
// Graph laden
var graph = await loadGraph();

// Layout berechnen
var layoutService = GetService<IGraphVisualizationService>();
var layout = await layoutService.CalculateForceDirectedLayoutAsync(graph);

// Statistiken
var stats = await layoutService.CalculateStatisticsAsync(graph);
var centrality = await layoutService.CalculateDegreeCentralityAsync(graph);

// Cluster
var clusters = await layoutService.CalculateLouvainClusteringAsync(graph);

// Render
RenderGraphWithPositions(graph, layout.NodePositions);
```

### **Pattern 2: OSM-Map with Layers**
```csharp
var mapRenderer = GetService<IOsmMapRenderer>();

// Konfiguration
var config = new MapConfiguration 
{ 
    DefaultCenter = new MapCenter { Latitude = 51.1657, Longitude = 10.4515 },
    BaseLayerType = "OpenStreetMap"
};

// Layers
var layers = new List<GeoLayer> 
{
    new GeoLayer { Type = LayerType.Markers, Name = "POIs" },
    new GeoLayer { Type = LayerType.Heatmap, Name = "Density" }
};

// HTML generieren
var html = mapRenderer.GenerateMapHtml(config, layers);

// In WebView laden
MapFrame.Navigate(new Uri("data:text/html," + Uri.EscapeDataString(html)));
```

### **Pattern 3: Dynamic Layout Selection**
```csharp
var algorithm = LayoutAlgorithm.ForceDirected; // From UI ComboBox

switch (algorithm)
{
    case LayoutAlgorithm.ForceDirected:
        result = await service.CalculateForceDirectedLayoutAsync(graph);
        break;
    case LayoutAlgorithm.Circular:
        result = await service.CalculateCircularLayoutAsync(graph);
        break;
    // ... other cases
}

UpdateGraphVisualization(graph, result);
```

---

## 🧪 **Testing Patterns**

### **Unit Test Beispiel:**
```csharp
[TestClass]
public class GraphVisualizationTests
{
    private IGraphVisualizationService _service;

    [TestInitialize]
    public void Setup()
    {
        _service = new GraphVisualizationService();
    }

    [TestMethod]
    public async Task ForceDirectedLayout_ConvergesFor100Nodes()
    {
        // Arrange
        var graph = CreateTestGraph(100, 200);

        // Act
        var result = await _service.CalculateForceDirectedLayoutAsync(graph);

        // Assert
        Assert.IsTrue(result.IsConverged);
        Assert.AreEqual(100, result.NodePositions.Count);
        Assert.IsTrue(result.ComputationTime.TotalMilliseconds > 0);
    }

    [TestMethod]
    public async Task Statistics_ReturnsCorrectValues()
    {
        // Arrange
        var graph = CreateTestGraph(50, 75);

        // Act
        var stats = await _service.CalculateStatisticsAsync(graph);

        // Assert
        Assert.AreEqual(50, stats.NodeCount);
        Assert.AreEqual(75, stats.EdgeCount);
        Assert.IsTrue(stats.Density > 0);
    }
}
```

---

## 📖 **Verwandte Dokumentation**

- [PHASE_24_README.md](PHASE_24_README.md) - Quick Start Guide
- [PHASE_24_IMPLEMENTATION.md](PHASE_24_IMPLEMENTATION.md) - Technical Details
- [GraphModels.cs](Services/GraphModels.cs) - Source Code
- [GraphVisualizationService.cs](Services/GraphVisualizationService.cs) - Source Code
- [OsmMapRenderer.cs](Services/OsmMapRenderer.cs) - Source Code

---

**Phase 24 Service Reference - Complete & Ready** ✅
