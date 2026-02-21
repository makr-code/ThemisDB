/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphModels.cs                                     ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     562                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Graph-Visualisierungs-Modelle für 3D-Netzwerk-Darstellung
/// URN: urn:themis:graph:*
/// 
/// Unterstützt:
/// - Force-directed Graph Layouts (FR, Kamada-Kawai)
/// - 3D-Koordinaten und Rendering
/// - Hierarchische Netzwerke
/// - Cluster-Visualisierung
/// </summary>
/// 
#region Graph Node & Edge

/// <summary>
/// Graph-Knoten
/// URN: urn:themis:graph:node:{id}
/// </summary>
public class GraphNode
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:graph:node:{Id}";

    // Identität
    public string Label { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public GraphNodeType Type { get; set; } = GraphNodeType.Default;

    // Geometrie (3D)
    public Vector3D Position { get; set; } = new();
    public Vector3D Velocity { get; set; } = new(); // Für Animations-Simulationen
    public double Radius { get; set; } = 10.0;
    public double Mass { get; set; } = 1.0; // Für Force-Simulation

    // Styling
    public string Color { get; set; } = "#3388ff";
    public string IconUrl { get; set; } = string.Empty;
    public string IconShape { get; set; } = "circle"; // circle, square, star, diamond
    public int IconSize { get; set; } = 25;
    public bool IsHighlighted { get; set; } = false;
    public bool IsSelected { get; set; } = false;

    // Daten-Zuordnung
    public string? DocumentId { get; set; }
    public string? ProcessId { get; set; }
    public string? EntityId { get; set; }
    public Dictionary<string, object> Data { get; set; } = new();

    // Clustering
    public string? ClusterId { get; set; }
    public List<string> ChildNodeIds { get; set; } = new();
    public string? ParentNodeId { get; set; }

    // Metadaten
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public enum GraphNodeType
{
    Default,
    Central,
    Standard,
    Document,
    Process,
    Entity,
    Person,
    Organization,
    Location,
    Event,
    Concept,
    Cluster
}

/// <summary>
/// 3D-Vektor für Positionierung
/// </summary>
public class Vector3D
{
    public double X { get; set; } = 0.0;
    public double Y { get; set; } = 0.0;
    public double Z { get; set; } = 0.0;

    public Vector3D() { }

    public Vector3D(double x, double y, double z)
    {
        X = x;
        Y = y;
        Z = z;
    }

    /// <summary>
    /// Euklidische Distanz zu anderem Punkt
    /// </summary>
    public double DistanceTo(Vector3D other)
    {
        var dx = X - other.X;
        var dy = Y - other.Y;
        var dz = Z - other.Z;
        return Math.Sqrt(dx * dx + dy * dy + dz * dz);
    }

    /// <summary>
    /// Vektor-Addition
    /// </summary>
    public Vector3D Add(Vector3D other) => new(X + other.X, Y + other.Y, Z + other.Z);

    /// <summary>
    /// Skalare Multiplikation
    /// </summary>
    public Vector3D Multiply(double scalar) => new(X * scalar, Y * scalar, Z * scalar);

    /// <summary>
    /// Länge des Vektors
    /// </summary>
    public double Length => Math.Sqrt(X * X + Y * Y + Z * Z);

    /// <summary>
    /// Normalisierter Vektor
    /// </summary>
    public Vector3D Normalize()
    {
        var length = Length;
        if (length == 0) return new Vector3D();
        return new Vector3D(X / length, Y / length, Z / length);
    }
}

/// <summary>
/// Graph-Kante
/// URN: urn:themis:graph:edge:{id}
/// </summary>
public class GraphEdge
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:graph:edge:{Id}";

    // Verbindung
    public string SourceNodeId { get; set; } = string.Empty;
    public string TargetNodeId { get; set; } = string.Empty;

    // Metadaten
    public string Label { get; set; } = string.Empty;
    public string RelationType { get; set; } = "related"; // "related", "parent", "child", "connected", etc.
    public double Strength { get; set; } = 1.0; // Für Layout-Berechnung

    // Styling
    public string Color { get; set; } = "#888888";
    public int StrokeWidth { get; set; } = 2;
    public double Opacity { get; set; } = 0.8;
    public bool IsDirected { get; set; } = false;
    public string StrokePattern { get; set; } = "solid"; // solid, dashed, dotted

    // Daten
    public double Weight { get; set; } = 1.0; // Für gewichtete Graphen
    public Dictionary<string, object> Data { get; set; } = new();

    // Visualisierung
    public bool IsHighlighted { get; set; } = false;
    public bool ShowLabel { get; set; } = false;

    // Metadaten
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

#endregion

#region Graph Configuration

/// <summary>
/// Graph-Konfiguration
/// URN: urn:themis:graph:config:{id}
/// </summary>
public class GraphConfiguration
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:graph:config:{Id}";

    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;

    // Layout-Einstellungen
    public LayoutAlgorithm LayoutAlgorithm { get; set; } = LayoutAlgorithm.ForceDirected;
    public LayoutOptions LayoutOptions { get; set; } = new();

    // Rendering
    public RenderingOptions RenderingOptions { get; set; } = new();

    // Physik-Simulation
    public PhysicsSimulation PhysicsSimulation { get; set; } = new();

    // Interaktion
    public bool EnableNodeDragging { get; set; } = true;
    public bool EnableZooming { get; set; } = true;
    public bool EnablePanning { get; set; } = true;
    public bool EnableNodeSelection { get; set; } = true;
    public bool EnableEdgeSelection { get; set; } = true;

    // Display
    public bool ShowNodeLabels { get; set; } = true;
    public bool ShowEdgeLabels { get; set; } = false;
    public bool ShowLegend { get; set; } = true;
    public bool EnableNodeClustering { get; set; } = false;

    // Performance
    public int MaxNodes { get; set; } = 5000;
    public int MaxEdges { get; set; } = 10000;
    public bool EnableLOD { get; set; } = true; // Level of Detail

    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

public enum LayoutAlgorithm
{
    ForceDirected,      // Fruchterman-Reingold
    HierarchicalLayout, // Top-down hierarchical
    Circular,           // Circular arrangement
    CircularLayout,     // Circular arrangement (alias)
    Hierarchical,       // Hierarchical (alias)
    Radial,             // Radial from center
    RadialLayout,       // Radial from center (alias)
    TreeLayout,         // Tree structure
    Kamada_Kawai,       // Kamada-Kawai algorithm
    Custom              // Benutzerdefiniert
}

/// <summary>
/// Layout-Optionen
/// </summary>
public class LayoutOptions
{
    // Force-directed Parameter
    public double RepulsiveForce { get; set; } = 100.0; // Abstoßung zwischen Knoten
    public double AttractiveForce { get; set; } = 0.1; // Anziehung entlang Kanten
    public double Damping { get; set; } = 0.85; // Dämpfung für Stabilität
    public int MaxIterations { get; set; } = 1000;
    public double ConvergenceThreshold { get; set; } = 0.01;

    // Raumoptionen
    public double Width { get; set; } = 1000.0;
    public double Height { get; set; } = 800.0;
    public double Depth { get; set; } = 800.0;

    // Spacing
    public double NodeSpacing { get; set; } = 50.0;
    public double LevelHeight { get; set; } = 100.0;

    // Animation
    public bool EnableAnimation { get; set; } = true;
    public int AnimationDuration { get; set; } = 300; // ms
    public bool EnableSmoothLayout { get; set; } = true;
}

/// <summary>
/// Rendering-Optionen
/// </summary>
public class RenderingOptions
{
    // Rendering-Modus
    public RenderingMode Mode { get; set; } = RenderingMode.WebGL;
    public bool Enable3D { get; set; } = true;

    // Kamera
    public double CameraDistance { get; set; } = 1500.0;
    public double CameraFOV { get; set; } = 45.0; // Field of View

    // Beleuchtung
    public bool EnableLighting { get; set; } = true;
    public Vector3D LightDirection { get; set; } = new(-1, 1, 1);
    public double Ambience { get; set; } = 0.5;

    // Effekte
    public bool EnableShadows { get; set; } = false;
    public bool EnableAntialias { get; set; } = true;
    public bool EnableBloom { get; set; } = false;

    // Performance
    public int TargetFPS { get; set; } = 60;
    public bool EnableVSync { get; set; } = true;
    public bool UseHardwareAcceleration { get; set; } = true;

    // Styling Defaults
    public string DefaultNodeColor { get; set; } = "#3388ff";
    public string DefaultEdgeColor { get; set; } = "#888888";
    public string DefaultBackgroundColor { get; set; } = "#ffffff";
    public string SelectionColor { get; set; } = "#ff0000";
    public string HighlightColor { get; set; } = "#ffff00";
}

public enum RenderingMode
{
    Canvas2D,    // 2D Canvas (fallback)
    WebGL,       // WebGL via HTMLElement/WebView
    Direct3D11,  // DirectX 11 (native)
    Vulkan       // Vulkan (native, cross-platform)
}

/// <summary>
/// Physik-Simulation
/// </summary>
public class PhysicsSimulation
{
    public bool Enabled { get; set; } = true;
    public double TimeStep { get; set; } = 0.016; // ~60fps
    public double Gravity { get; set; } = 0.0; // Schwerkraft (0 für 3D freischweben)
    public double AirResistance { get; set; } = 0.95;
    public bool EnableCollisionDetection { get; set; } = false;
}

#endregion

#region Complete Graph Structure

/// <summary>
/// Vollständige Graph-Struktur
/// URN: urn:themis:graph:instance:{id}
/// </summary>
public class Graph
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:graph:instance:{Id}";

    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;

    // Struktur
    public List<GraphNode> Nodes { get; set; } = new();
    public List<GraphEdge> Edges { get; set; } = new();

    // Konfiguration
    public GraphConfiguration Configuration { get; set; } = new();

    // Statistiken
    public int NodeCount => Nodes.Count;
    public int EdgeCount => Edges.Count;

    public double AverageDegree => EdgeCount > 0
        ? 2.0 * EdgeCount / Math.Max(1, NodeCount)
        : 0.0;

    public double Density => NodeCount > 1
        ? (2.0 * EdgeCount) / (NodeCount * (NodeCount - 1))
        : 0.0;

    // Clustering
    public List<GraphCluster> Clusters { get; set; } = new();

    // Metadaten
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();

    /// <summary>
    /// Knoten nach ID finden
    /// </summary>
    public GraphNode? GetNode(string nodeId) => Nodes.FirstOrDefault(n => n.Id == nodeId);

    /// <summary>
    /// Nachbarn eines Knotens
    /// </summary>
    public List<GraphNode> GetNeighbors(string nodeId)
    {
        var neighbors = new List<GraphNode>();
        var edges = Edges.Where(e => e.SourceNodeId == nodeId || e.TargetNodeId == nodeId);

        foreach (var edge in edges)
        {
            var neighborId = edge.SourceNodeId == nodeId ? edge.TargetNodeId : edge.SourceNodeId;
            var neighbor = GetNode(neighborId);
            if (neighbor != null)
                neighbors.Add(neighbor);
        }

        return neighbors;
    }

    /// <summary>
    /// Eingehende Kanten
    /// </summary>
    public List<GraphEdge> GetIncomingEdges(string nodeId) =>
        Edges.Where(e => e.TargetNodeId == nodeId).ToList();

    /// <summary>
    /// Ausgehende Kanten
    /// </summary>
    public List<GraphEdge> GetOutgoingEdges(string nodeId) =>
        Edges.Where(e => e.SourceNodeId == nodeId).ToList();

    /// <summary>
    /// Grad eines Knotens
    /// </summary>
    public int GetNodeDegree(string nodeId) =>
        Edges.Count(e => e.SourceNodeId == nodeId || e.TargetNodeId == nodeId);
}

#endregion

#region Graph Clustering

/// <summary>
/// Graph-Cluster
/// </summary>
public class GraphCluster
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;

    public List<string> NodeIds { get; set; } = new();
    public List<string> EdgeIds { get; set; } = new();

    public Vector3D CenterPosition { get; set; } = new();
    public double Radius { get; set; } = 0.0;

    public string Color { get; set; } = "#3388ff";
    public double Density { get; set; } = 0.0;
    public double Modularity { get; set; } = 0.0;

    // Hierarchie
    public string? ParentClusterId { get; set; }
    public List<string> ChildClusterIds { get; set; } = new();
}

#endregion

#region Layout Calculation

/// <summary>
/// Ergebnis einer Layout-Berechnung
/// </summary>
public class LayoutResult
{
    public bool IsConverged { get; set; }
    public int IterationCount { get; set; }
    public double FinalEnergy { get; set; }
    public TimeSpan ComputationTime { get; set; }

    public Dictionary<string, Vector3D> NodePositions { get; set; } = new();
    public List<string> WarningsAndErrors { get; set; } = new();
}

/// <summary>
/// Force-directed Layout Parameter
/// </summary>
public class ForceDirectedLayoutParams
{
    public double K { get; set; } = 100.0; // Ideale Kantenlänge
    public double Iterations { get; set; } = 1000;
    public double Threshold { get; set; } = 0.01; // Konvergenz-Schwelle
    public double CoulombForce { get; set; } = 1.0;
    public double HookeForce { get; set; } = 0.1;
    public double StepSize { get; set; } = 0.01;
    public double Cooling { get; set; } = 0.95;
    public bool Use3D { get; set; } = true;
}

#endregion

#region Graph Statistics

/// <summary>
/// Graph-Statistiken
/// </summary>
public class GraphStatistics
{
    public string GraphId { get; set; } = string.Empty;

    public int NodeCount { get; set; }
    public int EdgeCount { get; set; }
    public double Density { get; set; }
    public double AverageDegree { get; set; }

    // Zentralitäts-Metriken
    public Dictionary<string, double> BetweennessCentrality { get; set; } = new();
    public Dictionary<string, double> ClosenessCentrality { get; set; } = new();
    public Dictionary<string, double> DegreeCentrality { get; set; } = new();
    public Dictionary<string, double> EigenvectorCentrality { get; set; } = new();

    // Komponenten
    public int ConnectedComponents { get; set; }
    public double AverageShortetstPath { get; set; }
    public int Diameter { get; set; }

    // Clustering
    public double ClusteringCoefficient { get; set; }
    public int NumberOfTriangles { get; set; }
    public double Modularity { get; set; }

    public DateTime CalculatedAt { get; set; } = DateTime.UtcNow;
}

#endregion

#region Graph Interaction

/// <summary>
/// Graph-Interaktions-Event
/// </summary>
public class GraphInteraction
{
    public string Id { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;

    public GraphInteractionType Type { get; set; }

    public string? SelectedNodeId { get; set; }
    public string? SelectedEdgeId { get; set; }
    public List<string>? SelectedNodeIds { get; set; }

    public Vector3D? InteractionPosition { get; set; }

    public Dictionary<string, object> Data { get; set; } = new();
}

public enum GraphInteractionType
{
    NodeClick,
    NodeDoubleClick,
    NodeDrag,
    NodeHover,
    EdgeClick,
    EdgeHover,
    CameraMove,
    Zoom,
    Pan,
    SelectionChange,
    ContextMenu
}

#endregion
