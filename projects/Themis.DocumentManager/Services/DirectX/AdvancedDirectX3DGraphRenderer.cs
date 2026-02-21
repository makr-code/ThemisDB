/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AdvancedDirectX3DGraphRenderer.cs                  ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     363                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// Advanced DirectX 3D Graph Renderer mit vollständiger Pipeline
/// </summary>
public class AdvancedDirectX3DGraphRenderer : IDirectX3DGraphRenderer
{
    private readonly Camera3D _camera;
    private readonly Light3D _lighting;
    private readonly RenderPerformanceMonitor _performanceMonitor;
    private readonly RenderCommandQueue _commandQueue;
    private readonly AdvancedOptimizationEngine _optimizer;
    private readonly RenderOptimizationConfig _optimizationConfig;

    private Dictionary<string, MeshGenerator.SimpleVertex[]> _vertexCache = new();
    private Dictionary<string, uint[]> _indexCache = new();

    private float _cameraRotX = 0;
    private float _cameraRotY = 0;
    private float _cameraZoom = 1.0f;

    public AdvancedDirectX3DGraphRenderer()
    {
        _camera = new Camera3D();
        _lighting = new Light3D();
        _performanceMonitor = new RenderPerformanceMonitor();
        _commandQueue = new RenderCommandQueue(50000);
        _optimizer = new AdvancedOptimizationEngine();
        _optimizationConfig = new RenderOptimizationConfig();
    }

    public void Initialize(IntPtr windowHandle, int width, int height)
    {
        _camera.AspectRatio = width / (float)height;
        System.Diagnostics.Debug.WriteLine(
            $"Advanced DirectX 3D Renderer initialized for {width}x{height}");
    }

    /// <summary>
    /// Render Graph mit Advanced Batching
    /// </summary>
    public void Render(Graph graph)
    {
        _performanceMonitor.BeginFrame();

        try
        {
            // Clear command queue
            _commandQueue.Clear();

            // Prepare view/projection once per frame
            var viewMatrix = _camera.GetViewMatrix();
            var projMatrix = _camera.GetProjectionMatrix();

            // Optimization pass: culling, LOD, instancing hints
            var optResult = _optimizer.OptimizeGraph(graph, viewMatrix, projMatrix, _optimizationConfig);

            // Generate mesh geometry für nodes
            GenerateNodeMeshes(graph, optResult.VisibleNodeIds);

            // Enqueue render commands
            EnqueueRenderCommands(graph, optResult.VisibleNodeIds, optResult.VisibleEdges, viewMatrix, projMatrix);

            // Process queue
            ProcessRenderQueue();

            // Update performance stats
            System.Diagnostics.Debug.WriteLine(
                $"Frame: Nodes={graph.Nodes.Count} Edges={graph.Edges.Count} " +
                $"Queue={_commandQueue.Count} | {_performanceMonitor.GetStats()} | Optimizer: {optResult.Metrics.Summary()}");
        }
        finally
        {
            _performanceMonitor.EndFrame();
        }
    }

    /// <summary>
    /// Generate und cache Mesh Geometries
    /// </summary>
    private void GenerateNodeMeshes(Graph graph, HashSet<string> visibleNodeIds)
    {
        foreach (var node in graph.Nodes)
        {
            if (!visibleNodeIds.Contains(node.Id))
                continue;

            string key = node.Id;

            // Cache hit
            if (_vertexCache.ContainsKey(key))
                continue;

            // Parse color
            var color = HexToTuple(node.Color);
            float scaledRadius = (float)node.Radius / 100.0f;

            // Generate sphere mesh
            var (verts, inds) = MeshGenerator.GenerateSphereMesh(
                radius: scaledRadius,
                segments: 12,
                rings: 12,
                color: color);

            _vertexCache[key] = verts;
            _indexCache[key] = inds;
        }
    }

    /// <summary>
    /// Enqueue Render Commands für alle Nodes und Edges
    /// </summary>
    private void EnqueueRenderCommands(
        Graph graph,
        HashSet<string> visibleNodeIds,
        List<GraphEdge> visibleEdges,
        float[] viewMatrix,
        float[] projMatrix)
    {
        // Render edges first (back-to-front)
        foreach (var edge in visibleEdges)
        {
            var sourceNode = graph.Nodes.FirstOrDefault(n => n.Id == edge.SourceNodeId);
            var targetNode = graph.Nodes.FirstOrDefault(n => n.Id == edge.TargetNodeId);

            if (sourceNode != null && targetNode != null)
            {
                // Calculate edge position (midpoint)
                float dx = (float)(targetNode.Position.X - sourceNode.Position.X);
                float dy = (float)(targetNode.Position.Y - sourceNode.Position.Y);
                float dz = (float)(targetNode.Position.Z - sourceNode.Position.Z);
                float length = (float)Math.Sqrt(dx * dx + dy * dy + dz * dz);

                float midX = (float)(sourceNode.Position.X + targetNode.Position.X) / 2;
                float midY = (float)(sourceNode.Position.Y + targetNode.Position.Y) / 2;
                float midZ = (float)(sourceNode.Position.Z + targetNode.Position.Z) / 2;

                var modelMatrix = MatrixHelper.Translation(midX, midY, midZ);

                var cmd = new GraphRenderCommand
                {
                    Type = GraphRenderCommand.CommandType.DrawEdge,
                    Data = edge,
                    ModelMatrix = modelMatrix,
                    ViewMatrix = viewMatrix,
                    ProjectionMatrix = projMatrix
                };

                _commandQueue.Enqueue(cmd);
            }
        }

        // Render nodes (front)
        foreach (var node in graph.Nodes)
        {
            if (!visibleNodeIds.Contains(node.Id))
                continue;

            var modelMatrix = MatrixHelper.Translation(
                (float)node.Position.X,
                (float)node.Position.Y,
                (float)node.Position.Z);

            var cmd = new GraphRenderCommand
            {
                Type = GraphRenderCommand.CommandType.DrawNode,
                Data = node,
                ModelMatrix = modelMatrix,
                ViewMatrix = viewMatrix,
                ProjectionMatrix = projMatrix
            };

            _commandQueue.Enqueue(cmd);
        }
    }

    /// <summary>
    /// Process Render Command Queue
    /// </summary>
    private void ProcessRenderQueue()
    {
        int nodeCount = 0;
        int edgeCount = 0;

        while (_commandQueue.Count > 0)
        {
            var cmd = _commandQueue.Dequeue();
            if (cmd == null) break;

            switch (cmd.Type)
            {
                case GraphRenderCommand.CommandType.DrawNode:
                    if (cmd.Data is GraphNode node)
                    {
                        RenderNodeMesh(node, cmd.ModelMatrix);
                        nodeCount++;
                    }
                    break;

                case GraphRenderCommand.CommandType.DrawEdge:
                    if (cmd.Data is GraphEdge edge)
                    {
                        RenderEdgeMesh(edge, cmd.ModelMatrix);
                        edgeCount++;
                    }
                    break;
            }
        }

        System.Diagnostics.Debug.WriteLine(
            $"Rendered {nodeCount} nodes, {edgeCount} edges");
    }

    /// <summary>
    /// Render einzelnen Node
    /// </summary>
    private void RenderNodeMesh(GraphNode node, float[] modelMatrix)
    {
        if (!_vertexCache.ContainsKey(node.Id))
            return;

        var vertices = _vertexCache[node.Id];
        var indices = _indexCache[node.Id];

        // Apply lighting
        ApplyLighting(ref vertices);

        System.Diagnostics.Debug.WriteLine(
            $"Rendering node {node.Label}: {vertices.Length} verts, {indices.Length} inds");
    }

    /// <summary>
    /// Render einzelne Edge
    /// </summary>
    private void RenderEdgeMesh(GraphEdge edge, float[] modelMatrix)
    {
        var color = HexToTuple(edge.Color);
        float radius = edge.StrokeWidth / 100.0f;

        var (verts, inds) = MeshGenerator.GenerateCylinderMesh(
            radius: radius,
            height: 1.0f,
            segments: 6,
            color: color);

        System.Diagnostics.Debug.WriteLine(
            $"Rendering edge {edge.Id}: {verts.Length} verts, {inds.Length} inds");
    }

    /// <summary>
    /// Apply Lighting zu Vertices
    /// </summary>
    private void ApplyLighting(ref MeshGenerator.SimpleVertex[] vertices)
    {
        _lighting.NormalizeDirection();

        for (int i = 0; i < vertices.Length; i++)
        {
            var v = vertices[i];

            // Dot product mit light direction
            float dot = Math.Max(0.2f, v.NX * _lighting.DirectionX +
                                       v.NY * _lighting.DirectionY +
                                       v.NZ * _lighting.DirectionZ);

            // Apply diffuse + ambient
            v.R *= _lighting.AmbientR + _lighting.DiffuseR * dot;
            v.G *= _lighting.AmbientG + _lighting.DiffuseG * dot;
            v.B *= _lighting.AmbientB + _lighting.DiffuseB * dot;

            vertices[i] = v;
        }
    }

    public void SetCameraPosition(double x, double y, double z)
    {
        _camera.CenterX = (float)x;
        _camera.CenterY = (float)y;
        _camera.CenterZ = (float)z;
    }

    public void Rotate(float deltaX, float deltaY)
    {
        _cameraRotY += deltaX * 0.005f;
        _cameraRotX += deltaY * 0.005f;

        _camera.Rotate(deltaX, deltaY);
    }

    public void Zoom(float delta)
    {
        _cameraZoom -= delta * 0.1f;
        if (_cameraZoom < 0.1f) _cameraZoom = 0.1f;
        if (_cameraZoom > 10.0f) _cameraZoom = 10.0f;

        _camera.Zoom(delta);
    }

    public void Resize(int width, int height)
    {
        _camera.AspectRatio = width / (float)height;
    }

    public void Cleanup()
    {
        _vertexCache.Clear();
        _indexCache.Clear();
    }

    /// <summary>
    /// Parse Hex Color
    /// </summary>
    private static (float R, float G, float B, float A) HexToTuple(string hex)
    {
        try
        {
            hex = hex.TrimStart('#');
            if (hex.Length != 6)
                return (1, 1, 1, 1);

            int r = int.Parse(hex.Substring(0, 2), System.Globalization.NumberStyles.HexNumber);
            int g = int.Parse(hex.Substring(2, 2), System.Globalization.NumberStyles.HexNumber);
            int b = int.Parse(hex.Substring(4, 2), System.Globalization.NumberStyles.HexNumber);

            return (r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        }
        catch
        {
            return (1, 1, 1, 1);
        }
    }
}
