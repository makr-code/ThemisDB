/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AdvancedOptimizationEngine.cs                      ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     413                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// Phase 7: Advanced optimization layer for graph rendering.
/// Focus: frustum culling, LOD selection, instancing, pipeline state caching, draw-call reduction.
/// </summary>
public class AdvancedOptimizationEngine
{
    private readonly FrustumCuller _frustumCuller = new();
    private readonly LevelOfDetailSystem _lodSystem = new();
    private readonly InstanceBatcher _instanceBatcher = new();
    private readonly PipelineStateCache _pipelineStateCache = new();

    /// <summary>
    /// Optimize a graph frame: cull invisible items, select LOD, batch instances, and cache pipeline states.
    /// </summary>
    public OptimizationResult OptimizeGraph(
        Graph graph,
        float[] viewMatrix,
        float[] projectionMatrix,
        RenderOptimizationConfig config)
    {
        _instanceBatcher.Reset();

        var metrics = new RenderOptimizationMetrics();
        metrics.DrawCallsBefore = graph.Nodes.Count + graph.Edges.Count;

        // Build frustum from view-projection matrix
        var viewProj = MatrixHelper.Multiply(viewMatrix, projectionMatrix);
        var frustum = _frustumCuller.BuildFrustum(viewProj, config.FrustumPadding);

        // Cull and LOD selection for nodes
        foreach (var node in graph.Nodes)
        {
            metrics.TotalNodes++;
            var center = new Vector3((float)node.Position.X, (float)node.Position.Y, (float)node.Position.Z);
            float radius = Math.Max(1f, (float)node.Radius / 100.0f);

            if (!_frustumCuller.IsSphereVisible(frustum, center, radius))
            {
                metrics.CulledNodes++;
                continue;
            }

            float distance = center.Length();
            int lod = _lodSystem.GetLodLevel(distance, config);

            _instanceBatcher.AddInstance(node.Id, lod, center, radius, node.Color);
            metrics.InstancedNodes++;
        }

        // Cull edges based on node visibility or frustum overlap
        var visibleNodeIds = _instanceBatcher.GetVisibleNodeIds();
        foreach (var edge in graph.Edges)
        {
            metrics.TotalEdges++;
            if (!visibleNodeIds.Contains(edge.SourceNodeId) || !visibleNodeIds.Contains(edge.TargetNodeId))
            {
                metrics.CulledEdges++;
                continue;
            }

            var source = graph.Nodes.FirstOrDefault(n => n.Id == edge.SourceNodeId);
            var target = graph.Nodes.FirstOrDefault(n => n.Id == edge.TargetNodeId);
            if (source == null || target == null)
            {
                metrics.CulledEdges++;
                continue;
            }

            var mid = new Vector3(
                (float)((source.Position.X + target.Position.X) * 0.5),
                (float)((source.Position.Y + target.Position.Y) * 0.5),
                (float)((source.Position.Z + target.Position.Z) * 0.5));
            float length = (float)Math.Sqrt(
                Math.Pow(target.Position.X - source.Position.X, 2) +
                Math.Pow(target.Position.Y - source.Position.Y, 2) +
                Math.Pow(target.Position.Z - source.Position.Z, 2));
            float edgeRadius = Math.Max(1f, length * 0.5f);

            if (!_frustumCuller.IsSphereVisible(frustum, mid, edgeRadius))
            {
                metrics.CulledEdges++;
                continue;
            }

            metrics.VisibleEdges.Add(edge);
        }

        // Finalize batches and metrics
        var batches = _instanceBatcher.FlushBatches(config.InstanceBatchSize);
        metrics.InstanceGroups = batches.Count;
        metrics.DrawCallsAfter = batches.Count + metrics.VisibleEdges.Count;

        // Simulated pipeline state cache lookups for each batch
        foreach (var batch in batches)
        {
            var descriptor = new PipelineStateDescriptor
            {
                ShaderProfile = batch.Lod switch
                {
                    0 => "vs_5_0|ps_5_0|high",
                    1 => "vs_5_0|ps_5_0|medium",
                    _ => "vs_5_0|ps_5_0|low"
                },
                BlendState = batch.Lod <= 1 ? "AlphaBlend" : "Opaque",
                RasterizerState = batch.Lod == 0 ? "Solid" : "Solid_NoCull",
                DepthState = "LessEqual",
                Topology = "TriangleList"
            };

            _pipelineStateCache.GetOrAdd(descriptor, out bool cacheHit);
            if (cacheHit) metrics.PipelineStateHits++; else metrics.PipelineStateMisses++;
        }

        return new OptimizationResult
        {
            InstanceBatches = batches,
            VisibleEdges = metrics.VisibleEdges,
            VisibleNodeIds = _instanceBatcher.GetVisibleNodeIds(),
            Metrics = metrics
        };
    }
}

public class RenderOptimizationConfig
{
    public float FrustumPadding { get; set; } = 1.05f; // Slightly expand frustum to avoid popping
    public float LodNearDistance { get; set; } = 500f;
    public float LodMidDistance { get; set; } = 1500f;
    public float LodFarDistance { get; set; } = 3000f;
    public int InstanceBatchSize { get; set; } = 2048;
}

public class OptimizationResult
{
    public List<InstanceBatch> InstanceBatches { get; set; } = new();
    public List<GraphEdge> VisibleEdges { get; set; } = new();
    public HashSet<string> VisibleNodeIds { get; set; } = new();
    public RenderOptimizationMetrics Metrics { get; set; } = new();
}

public class RenderOptimizationMetrics
{
    public int TotalNodes { get; set; }
    public int CulledNodes { get; set; }
    public int InstancedNodes { get; set; }
    public int InstanceGroups { get; set; }
    public int TotalEdges { get; set; }
    public int CulledEdges { get; set; }
    public int DrawCallsBefore { get; set; }
    public int DrawCallsAfter { get; set; }
    public int PipelineStateHits { get; set; }
    public int PipelineStateMisses { get; set; }
    public List<GraphEdge> VisibleEdges { get; } = new();

    public string Summary()
    {
        return $"Nodes: {TotalNodes} (culled {CulledNodes}) | Edges: {TotalEdges} (culled {CulledEdges}) | " +
               $"DrawCalls: {DrawCallsBefore}->{DrawCallsAfter} | PSO cache hit rate: {HitRate:P1}";
    }

    public float HitRate => (PipelineStateHits + PipelineStateMisses) > 0
        ? (float)PipelineStateHits / (PipelineStateHits + PipelineStateMisses)
        : 1f;
}

public class FrustumCuller
{
    /// <summary>
    /// Build 6 planes from view-projection matrix.
    /// </summary>
    public Plane[] BuildFrustum(float[] viewProjection, float padding)
    {
        var planes = new Plane[6];

        float m00 = viewProjection[0];  float m01 = viewProjection[1];  float m02 = viewProjection[2];  float m03 = viewProjection[3];
        float m10 = viewProjection[4];  float m11 = viewProjection[5];  float m12 = viewProjection[6];  float m13 = viewProjection[7];
        float m20 = viewProjection[8];  float m21 = viewProjection[9];  float m22 = viewProjection[10]; float m23 = viewProjection[11];
        float m30 = viewProjection[12]; float m31 = viewProjection[13]; float m32 = viewProjection[14]; float m33 = viewProjection[15];

        planes[0] = new Plane(m03 + m00, m13 + m10, m23 + m20, m33 + m30); // Left
        planes[1] = new Plane(m03 - m00, m13 - m10, m23 - m20, m33 - m30); // Right
        planes[2] = new Plane(m03 - m01, m13 - m11, m23 - m21, m33 - m31); // Top
        planes[3] = new Plane(m03 + m01, m13 + m11, m23 + m21, m33 + m31); // Bottom
        planes[4] = new Plane(m03 + m02, m13 + m12, m23 + m22, m33 + m32); // Near
        planes[5] = new Plane(m03 - m02, m13 - m12, m23 - m22, m33 - m32); // Far

        for (int i = 0; i < planes.Length; i++)
        {
            planes[i] = NormalizePlane(planes[i], padding);
        }

        return planes;
    }

    /// <summary>
    /// Sphere vs frustum test. Returns true when sphere intersects or is inside.
    /// </summary>
    public bool IsSphereVisible(Plane[] frustum, Vector3 center, float radius)
    {
        for (int i = 0; i < frustum.Length; i++)
        {
            float distance = frustum[i].A * center.X + frustum[i].B * center.Y + frustum[i].C * center.Z + frustum[i].D;
            if (distance < -radius)
            {
                return false;
            }
        }
        return true;
    }

    private Plane NormalizePlane(Plane plane, float padding)
    {
        float length = (float)Math.Sqrt(plane.A * plane.A + plane.B * plane.B + plane.C * plane.C);
        if (length < 1e-5f) return plane;

        float inv = 1.0f / length;
        return new Plane(
            plane.A * inv,
            plane.B * inv,
            plane.C * inv,
            (plane.D * inv) * padding);
    }
}

public readonly struct Plane
{
    public Plane(float a, float b, float c, float d)
    {
        A = a; B = b; C = c; D = d;
    }

    public float A { get; }
    public float B { get; }
    public float C { get; }
    public float D { get; }
}

public class LevelOfDetailSystem
{
    public int GetLodLevel(float distance, RenderOptimizationConfig config)
    {
        if (distance < config.LodNearDistance) return 0;   // Highest detail
        if (distance < config.LodMidDistance) return 1;    // Medium detail
        if (distance < config.LodFarDistance) return 2;    // Low detail
        return 2;
    }
}

public class InstanceBatcher
{
    private readonly Dictionary<string, InstanceBatch> _batches = new();
    private readonly HashSet<string> _visibleNodeIds = new();

    public void Reset()
    {
        _batches.Clear();
        _visibleNodeIds.Clear();
    }

    public void AddInstance(string nodeId, int lod, Vector3 position, float radius, string colorHex)
    {
        string key = $"{lod}|{colorHex}";
        if (!_batches.TryGetValue(key, out var batch))
        {
            batch = new InstanceBatch { Lod = lod, ColorHex = colorHex };
            _batches[key] = batch;
        }

        batch.Instances.Add(new InstanceData
        {
            NodeId = nodeId,
            Position = position,
            Radius = radius,
            ColorHex = colorHex
        });

        _visibleNodeIds.Add(nodeId);
    }

    public List<InstanceBatch> FlushBatches(int maxBatchSize)
    {
        var result = new List<InstanceBatch>();
        foreach (var kvp in _batches)
        {
            var batch = kvp.Value;
            if (batch.Instances.Count <= maxBatchSize)
            {
                result.Add(batch);
                continue;
            }

            // Split large batches to respect GPU instance limits
            var current = new InstanceBatch { Lod = batch.Lod, ColorHex = batch.ColorHex };
            foreach (var instance in batch.Instances)
            {
                current.Instances.Add(instance);
                if (current.Instances.Count >= maxBatchSize)
                {
                    result.Add(current);
                    current = new InstanceBatch { Lod = batch.Lod, ColorHex = batch.ColorHex };
                }
            }
            if (current.Instances.Count > 0)
            {
                result.Add(current);
            }
        }

        _batches.Clear();
        return result;
    }

    public HashSet<string> GetVisibleNodeIds() => _visibleNodeIds;
}

public class InstanceBatch
{
    public int Lod { get; set; }
    public string ColorHex { get; set; } = "#FFFFFF";
    public List<InstanceData> Instances { get; set; } = new();
}

public class InstanceData
{
    public string NodeId { get; set; } = string.Empty;
    public Vector3 Position { get; set; }
    public float Radius { get; set; }
    public string ColorHex { get; set; } = "#FFFFFF";
}

public class PipelineStateCache
{
    private readonly Dictionary<string, PipelineState> _cache = new();

    public PipelineState GetOrAdd(PipelineStateDescriptor descriptor, out bool cacheHit)
    {
        string key = descriptor.GetKey();
        if (_cache.TryGetValue(key, out var existing))
        {
            cacheHit = true;
            existing.LastUsed = DateTime.UtcNow;
            return existing;
        }

        var state = new PipelineState
        {
            Descriptor = descriptor,
            CreatedAt = DateTime.UtcNow,
            LastUsed = DateTime.UtcNow
        };

        _cache[key] = state;
        cacheHit = false;
        return state;
    }

    public int Count => _cache.Count;
}

public class PipelineStateDescriptor
{
    public string ShaderProfile { get; set; } = string.Empty;
    public string BlendState { get; set; } = string.Empty;
    public string RasterizerState { get; set; } = string.Empty;
    public string DepthState { get; set; } = string.Empty;
    public string Topology { get; set; } = string.Empty;

    public string GetKey()
    {
        return $"{ShaderProfile}|{BlendState}|{RasterizerState}|{DepthState}|{Topology}";
    }
}

public class PipelineState
{
    public PipelineStateDescriptor Descriptor { get; set; } = new();
    public DateTime CreatedAt { get; set; }
    public DateTime LastUsed { get; set; }
}
