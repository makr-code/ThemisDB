/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DirectXRendererUsageExample.cs                     ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     397                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Services.DirectX;

namespace Themis.DocumentManager.Examples;

/// <summary>
/// Example: Wie man den Advanced DirectX 3D Graph Renderer verwendet
/// </summary>
public class DirectXRendererUsageExample
{
    /// <summary>
    /// Example 1: Einfacher Graph mit 3 Nodes
    /// </summary>
    public static Graph CreateSimpleGraph()
    {
        var graph = new Graph
        {
            Id = "simple-graph",
            Name = "Simple Graph",
            Nodes = new List<GraphNode>(),
            Edges = new List<GraphEdge>()
        };

        // 3 Nodes erstellen
        for (int i = 0; i < 3; i++)
        {
            graph.Nodes.Add(new GraphNode
            {
                Id = $"node-{i}",
                Label = $"Node {i}",
                Type = i == 0 ? GraphNodeType.Central : GraphNodeType.Standard,
                Position = new Vector3D { X = i * 2, Y = 0, Z = 0 },
                Radius = i == 0 ? 30 : 20,
                Color = i == 0 ? "#FF5722" : "#2196F3"
            });
        }

        // Edges zwischen Nodes
        graph.Edges.Add(new GraphEdge
        {
            Id = "edge-0-1",
            SourceNodeId = "node-0",
            TargetNodeId = "node-1",
            Color = "#555555",
            StrokeWidth = 2
        });

        graph.Edges.Add(new GraphEdge
        {
            Id = "edge-0-2",
            SourceNodeId = "node-0",
            TargetNodeId = "node-2",
            Color = "#555555",
            StrokeWidth = 2
        });

        return graph;
    }

    /// <summary>
    /// Example 2: Graph mit Metadaten
    /// </summary>
    public static Graph CreateMetadataGraph()
    {
        var graph = new Graph
        {
            Id = "metadata-graph",
            Name = "Metadata Graph",
            Nodes = new List<GraphNode>(),
            Edges = new List<GraphEdge>(),
            Metadata = new Dictionary<string, object>
            {
                { "CreatedDate", DateTime.UtcNow },
                { "Version", "1.0" },
                { "GraphType", "KnowledgeGraph" }
            }
        };

        // 5 Nodes mit verschiedenen Types
        var types = new[] { GraphNodeType.Central, GraphNodeType.Standard };
        for (int i = 0; i < 5; i++)
        {
            int typeIdx = i % types.Length;
            graph.Nodes.Add(new GraphNode
            {
                Id = $"meta-node-{i}",
                Label = $"Meta-{i}",
                Type = types[typeIdx],
                Position = new Vector3D { X = i, Y = i % 2, Z = 0 },
                Radius = typeIdx == 0 ? 40 : 25,
                Color = typeIdx == 0 ? "#4CAF50" : "#9C27B0",
                Metadata = new Dictionary<string, object>
                {
                    { "Index", i },
                    { "ProcessedAt", DateTime.UtcNow }
                }
            });
        }

        return graph;
    }

    /// <summary>
    /// Example 3: Renderer Initialization (Typical WPF Setup)
    /// </summary>
    public static void InitializeRendererExample(
        IDirectX3DGraphRenderer renderer,
        Graph graph)
    {
        // 1. Initialize mit Window Handle und Größe
        IntPtr hwnd = IntPtr.Zero;  // Würde vom WindowInteropHelper kommen
        int width = 1024;
        int height = 768;

        renderer.Initialize(hwnd, width, height);
        System.Diagnostics.Debug.WriteLine("Renderer initialized");

        // 2. Render Graph
        renderer.Render(graph);
        System.Diagnostics.Debug.WriteLine($"Rendered {graph.Nodes.Count} nodes");

        // 3. Camera Control
        renderer.SetCameraPosition(0, 0, 5);
        renderer.Rotate(15f, 30f);
        renderer.Zoom(-0.5f);

        System.Diagnostics.Debug.WriteLine("Camera adjusted");
    }

    /// <summary>
    /// Example 4: Render Loop in Background Task
    /// </summary>
    public static async void StartRenderLoopExample(
        IDirectX3DGraphRenderer renderer,
        Graph graph)
    {
        // Render Loop - läuft 60 Frames/Sekunde
        await System.Threading.Tasks.Task.Run(async () =>
        {
            int frameCount = 0;
            while (frameCount < 3600)  // 1 Minute bei 60 FPS
            {
                // Render Frame
                renderer.Render(graph);

                // Kontinuierliche Rotation für Demo
                renderer.Rotate(1f, 0f);

                // 16ms = ~60 FPS
                await System.Threading.Tasks.Task.Delay(16);
                frameCount++;

                // Alle 60 Frames (1 Sekunde) Log ausgeben
                if (frameCount % 60 == 0)
                {
                    System.Diagnostics.Debug.WriteLine(
                        $"Frame: {frameCount}, GraphInfo: {graph.Nodes.Count} nodes, " +
                        $"{graph.Edges.Count} edges");
                }
            }
        });
    }

    /// <summary>
    /// Example 5: Dynamische Graph Manipulation
    /// </summary>
    public static void DynamicGraphManipulationExample(
        Graph graph,
        IDirectX3DGraphRenderer renderer)
    {
        // Node hinzufügen
        var newNode = new GraphNode
        {
            Id = "new-node",
            Label = "New Node",
            Type = GraphNodeType.Standard,
            Position = new Vector3D { X = 5, Y = 5, Z = 0 },
            Radius = 25,
            Color = "#FF9800"
        };
        graph.Nodes.Add(newNode);

        System.Diagnostics.Debug.WriteLine($"Added node. Total: {graph.Nodes.Count}");

        // Edge hinzufügen
        if (graph.Nodes.Count >= 2)
        {
            var edge = new GraphEdge
            {
                Id = "new-edge",
                SourceNodeId = graph.Nodes[0].Id,
                TargetNodeId = graph.Nodes[graph.Nodes.Count - 1].Id,
                Color = "#666666",
                StrokeWidth = 3
            };
            graph.Edges.Add(edge);

            System.Diagnostics.Debug.WriteLine($"Added edge. Total: {graph.Edges.Count}");
        }

        // Re-render
        renderer.Render(graph);
    }

    /// <summary>
    /// Example 6: Performance Monitoring (Debug Output)
    /// </summary>
    public static void PerformanceMonitoringExample()
    {
        // RenderPerformanceMonitor wird intern verwendet in AdvancedDirectX3DGraphRenderer
        // Hier ist ein Beispiel, wie man FPS-Info aus Debug Output liest:

        System.Diagnostics.Debug.WriteLine(
            "AdvancedDirectX3DGraphRenderer nutzt RenderPerformanceMonitor:");
        System.Diagnostics.Debug.WriteLine(
            "- BeginFrame() / EndFrame() pro Render-Zyklus");
        System.Diagnostics.Debug.WriteLine(
            "- Output Format: 'FPS: 60.0 | Min: 16.5ms | Max: 17.2ms | Frames: 3600'");
        System.Diagnostics.Debug.WriteLine(
            "- Verfügbar via _performanceMonitor.GetStats()");
    }

    /// <summary>
    /// Example 7: Mesh Caching Benefits
    /// </summary>
    public static void MeshCachingBenefitsExample()
    {
        /*
         * AdvancedDirectX3DGraphRenderer nutzt Mesh-Caching:
         * 
         * First Render (100 nodes):
         * - GenerateNodeMeshes() creates 100 Sphere geometries
         * - Stores in _vertexCache and _indexCache
         * - Time: ~50ms
         * 
         * Subsequent Renders (same 100 nodes):
         * - Meshes already cached
         * - O(1) lookup time
         * - Time: <1ms
         * 
         * Performance Impact:
         * - First frame: Initialize geometry
         * - Frames 2-60: Use cached geometry
         * - No vertex/index buffer recreation
         * - Minimal GC pressure
         */

        System.Diagnostics.Debug.WriteLine(
            "Mesh Caching: First render generates geometry, " +
            "subsequent renders use cached meshes");
    }

    /// <summary>
    /// Example 8: Color Management
    /// </summary>
    public static void ColorManagementExample()
    {
        var colorMappings = new Dictionary<string, string>
        {
            { "Central Hub", "#2196F3" },      // Blue
            { "Standard Node", "#4CAF50" },    // Green
            { "Leaf Node", "#FF9800" },        // Orange
            { "Error Node", "#F44336" },       // Red
            { "Edge Connection", "#666666" }   // Gray
        };

        foreach (var mapping in colorMappings)
        {
            System.Diagnostics.Debug.WriteLine(
                $"{mapping.Key}: {mapping.Value}");
        }

        // Hex wird zu float (0-1) in AdvancedDirectX3DGraphRenderer.HexToTuple()
        // "#2196F3" → (0.133f, 0.588f, 0.953f, 1.0f)
    }

    /// <summary>
    /// Example 9: Camera Control Patterns
    /// </summary>
    public static void CameraControlPatternsExample(
        IDirectX3DGraphRenderer renderer)
    {
        // Pattern 1: Manual Camera Positioning
        renderer.SetCameraPosition(0, 0, 10);  // Camera 10 units above graph

        // Pattern 2: Rotation via Mouse
        renderer.Rotate(10f, 20f);  // Rotate 10° X-axis, 20° Y-axis

        // Pattern 3: Zoom
        renderer.Zoom(1f);   // Zoom in (positive)
        renderer.Zoom(-1f);  // Zoom out (negative)

        // Pattern 4: Resize
        renderer.Resize(1920, 1080);  // Update aspect ratio

        System.Diagnostics.Debug.WriteLine(
            "Camera patterns: Position, Rotate, Zoom, Resize");
    }

    /// <summary>
    /// Example 10: Error Handling
    /// </summary>
    public static void ErrorHandlingExample(
        IDirectX3DGraphRenderer renderer,
        Graph graph)
    {
        try
        {
            // Guard against null graph
            if (graph == null)
                throw new ArgumentNullException(nameof(graph));

            // Guard against invalid dimensions
            if (graph.Nodes.Count == 0)
                System.Diagnostics.Debug.WriteLine("Warning: Empty graph");

            // Render with error handling
            renderer.Render(graph);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine(
                $"Render Error: {ex.GetType().Name}: {ex.Message}");

            // Fallback: Graceful degradation
            // App continues without 3D rendering
        }
    }
}

/// <summary>
/// Unit Test Examples (for testing frameworks)
/// </summary>
public class DirectXRendererTestExamples
{
    // [TestMethod]
    public void TestSimpleGraphCreation()
    {
        var graph = DirectXRendererUsageExample.CreateSimpleGraph();
        
        assert(graph.Nodes.Count == 3);
        assert(graph.Edges.Count == 2);
        assert(graph.Nodes[0].Type == GraphNodeType.Central);
    }

    // [TestMethod]
    public void TestMetadataGraphCreation()
    {
        var graph = DirectXRendererUsageExample.CreateMetadataGraph();
        
        assert(graph.Nodes.Count == 5);
        assert(graph.Metadata != null && graph.Metadata.ContainsKey("GraphType"));
    }

    // [TestMethod]
    public void TestColorParsing()
    {
        // HexToTuple would be tested separately
        // "#2196F3" → (0.133f, 0.588f, 0.953f, 1.0f)
        // This is handled by AdvancedDirectX3DGraphRenderer.HexToTuple()
    }

    private static void assert(bool condition)
    {
        if (!condition)
            throw new Exception("Assertion failed");
    }
}
