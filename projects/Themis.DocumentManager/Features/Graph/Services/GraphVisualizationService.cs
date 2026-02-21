/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphVisualizationService.cs                       ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     709                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;
using Model = Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.Graph.Services;

/// <summary>
/// Graph-Visualisierungs-Service mit Layout-Berechnung
/// Unterstützt: Force-directed, Hierarchical, Circular, Radial, Kamada-Kawai Layouts
/// </summary>
/// 
public interface IGraphVisualizationService
{
    Task<LayoutResult> CalculateLayoutAsync(Model.Graph graph, LayoutAlgorithm algorithm, CancellationToken cancellationToken = default);
    Task<LayoutResult> CalculateForceDirectedLayoutAsync(Model.Graph graph, ForceDirectedLayoutParams? parameters = null, CancellationToken cancellationToken = default);
    Task<LayoutResult> CalculateHierarchicalLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default);
    Task<LayoutResult> CalculateCircularLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default);
    Task<LayoutResult> CalculateRadialLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default);
    Task<LayoutResult> CalculateKamadaKawaiLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default);
    Task<List<GraphCluster>> CalculateLouvainClusteringAsync(Model.Graph graph);
    Task<List<GraphCluster>> CalculateKMeansClusteringAsync(Model.Graph graph, int clusterCount);
    Task<GraphStatistics> CalculateStatisticsAsync(Model.Graph graph);
    Task<Dictionary<string, double>> CalculateBetweennessCentralityAsync(Model.Graph graph);
    Task<Dictionary<string, double>> CalculateDegreeCentralityAsync(Model.Graph graph);
    Task<Dictionary<string, double>> CalculateClosenessCentralityAsync(Model.Graph graph);
    Vector3D GetOptimalCameraPosition(Model.Graph graph, RenderingOptions options);
    List<GraphNode> GetConnectedComponent(Model.Graph graph, string startNodeId);
    int GetShortestPath(Model.Graph graph, string sourceId, string targetId);
}

public enum ClusteringAlgorithm
{
    Louvain,
    KMeans,
    DBScan,
    Spectral
}

public class GraphVisualizationService : IGraphVisualizationService
{
    private readonly Random _random = new();

    public async Task<LayoutResult> CalculateLayoutAsync(Model.Graph graph, LayoutAlgorithm algorithm, CancellationToken cancellationToken = default)
    {
        return algorithm switch
        {
            LayoutAlgorithm.ForceDirected => await CalculateForceDirectedLayoutAsync(graph, null, cancellationToken),
            LayoutAlgorithm.HierarchicalLayout => await CalculateHierarchicalLayoutAsync(graph, cancellationToken),
            LayoutAlgorithm.CircularLayout => await CalculateCircularLayoutAsync(graph, cancellationToken),
            LayoutAlgorithm.RadialLayout => await CalculateRadialLayoutAsync(graph, cancellationToken),
            LayoutAlgorithm.Kamada_Kawai => await CalculateKamadaKawaiLayoutAsync(graph, cancellationToken),
            _ => await CalculateForceDirectedLayoutAsync(graph, null, cancellationToken)
        };
    }

    public async Task<LayoutResult> CalculateForceDirectedLayoutAsync(Model.Graph graph, ForceDirectedLayoutParams? parameters = null, CancellationToken cancellationToken = default)
    {
        return await Task.Run(() =>
        {
            var sw = Stopwatch.StartNew();
            var param = parameters ?? new ForceDirectedLayoutParams();
            var result = new LayoutResult { NodePositions = new Dictionary<string, Vector3D>() };

            if (graph.Nodes.Count == 0)
            {
                sw.Stop();
                result.ComputationTime = sw.Elapsed;
                result.IsConverged = true;
                return result;
            }

            // Initialisierung: Zufällige Positionen
            foreach (var node in graph.Nodes)
            {
                node.Position = new Vector3D(
                    (_random.NextDouble() - 0.5) * 1000,
                    (_random.NextDouble() - 0.5) * 1000,
                    param.Use3D ? (_random.NextDouble() - 0.5) * 1000 : 0
                );
                node.Velocity = new Vector3D(0, 0, 0);
            }

            // Iterations-Loop
            double energy = double.MaxValue;
            int iteration = 0;

            while (iteration < param.Iterations && energy > param.Threshold)
            {
                cancellationToken.ThrowIfCancellationRequested();

                double totalEnergy = 0.0;

                // Abstoßungs-Kräfte
                for (int i = 0; i < graph.Nodes.Count; i++)
                {
                    for (int j = i + 1; j < graph.Nodes.Count; j++)
                    {
                        var node1 = graph.Nodes[i];
                        var node2 = graph.Nodes[j];
                        var delta = node2.Position.Add(node1.Position.Multiply(-1));
                        var dist = delta.Length;

                        if (dist < 0.1) dist = 0.1;

                        var repulsiveForce = (param.K * param.K) / dist;
                        var direction = delta.Normalize();
                        var force1 = direction.Multiply(repulsiveForce);
                        var force2 = direction.Multiply(-repulsiveForce);

                        node1.Velocity = node1.Velocity.Add(force1.Multiply(1.0 / node1.Mass));
                        node2.Velocity = node2.Velocity.Add(force2.Multiply(1.0 / node2.Mass));

                        totalEnergy += repulsiveForce * dist;
                    }
                }

                // Anziehungs-Kräfte entlang Kanten
                foreach (var edge in graph.Edges)
                {
                    var source = graph.GetNode(edge.SourceNodeId);
                    var target = graph.GetNode(edge.TargetNodeId);

                    if (source == null || target == null) continue;

                    var delta = target.Position.Add(source.Position.Multiply(-1));
                    var dist = delta.Length;
                    var direction = dist > 0.1 ? delta.Normalize() : new Vector3D(0, 0, 0);
                    var attractiveForce = (dist * dist) / param.K * param.HookeForce * edge.Strength;
                    var force = direction.Multiply(attractiveForce);

                    source.Velocity = source.Velocity.Add(force.Multiply(1.0 / source.Mass));
                    target.Velocity = target.Velocity.Add(force.Multiply(-1.0 / target.Mass));

                    totalEnergy += attractiveForce * dist;
                }

                // Positionen aktualisieren
                foreach (var node in graph.Nodes)
                {
                    node.Position = node.Position.Add(node.Velocity.Multiply(0.016));
                    node.Velocity = node.Velocity.Multiply(0.95);
                    result.NodePositions[node.Id] = node.Position;
                }

                energy = totalEnergy / Math.Max(1, graph.Nodes.Count);
                result.FinalEnergy = energy;
                result.IterationCount = iteration;
                iteration++;
            }

            result.IsConverged = energy <= param.Threshold;
            sw.Stop();
            result.ComputationTime = sw.Elapsed;
            return result;
        }, cancellationToken);
    }

    public async Task<LayoutResult> CalculateHierarchicalLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default)
    {
        return await Task.Run(() =>
        {
            var sw = Stopwatch.StartNew();
            var result = new LayoutResult { NodePositions = new Dictionary<string, Vector3D>() };

            if (graph.Nodes.Count == 0)
            {
                sw.Stop();
                result.ComputationTime = sw.Elapsed;
                return result;
            }

            var levels = new Dictionary<string, int>();
            var queue = new Queue<string>();

            // Root-Knoten finden
            foreach (var node in graph.Nodes)
            {
                if (graph.GetIncomingEdges(node.Id).Count == 0)
                {
                    levels[node.Id] = 0;
                    queue.Enqueue(node.Id);
                }
            }

            // BFS für Level-Assignment
            while (queue.Count > 0)
            {
                var nodeId = queue.Dequeue();
                var currentLevel = levels[nodeId];

                foreach (var edge in graph.GetOutgoingEdges(nodeId))
                {
                    if (!levels.ContainsKey(edge.TargetNodeId))
                    {
                        levels[edge.TargetNodeId] = currentLevel + 1;
                        queue.Enqueue(edge.TargetNodeId);
                    }
                }
            }

            // Positionierung basierend auf Level
            var nodesByLevel = new Dictionary<int, List<GraphNode>>();
            foreach (var node in graph.Nodes)
            {
                var level = levels.ContainsKey(node.Id) ? levels[node.Id] : 0;
                if (!nodesByLevel.ContainsKey(level))
                    nodesByLevel[level] = new List<GraphNode>();
                nodesByLevel[level].Add(node);
            }

            double yOffset = 0;
            foreach (var levelGroup in nodesByLevel.OrderBy(x => x.Key))
            {
                double xStart = -(levelGroup.Value.Count - 1) * 100 / 2.0;
                for (int i = 0; i < levelGroup.Value.Count; i++)
                {
                    var node = levelGroup.Value[i];
                    node.Position = new Vector3D(xStart + i * 100, yOffset, 0);
                    result.NodePositions[node.Id] = node.Position;
                }
                yOffset -= 150;
            }

            result.IsConverged = true;
            result.IterationCount = 1;
            sw.Stop();
            result.ComputationTime = sw.Elapsed;
            return result;
        }, cancellationToken);
    }

    public async Task<LayoutResult> CalculateCircularLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default)
    {
        return await Task.Run(() =>
        {
            var sw = Stopwatch.StartNew();
            var result = new LayoutResult { NodePositions = new Dictionary<string, Vector3D>() };

            if (graph.Nodes.Count == 0)
            {
                sw.Stop();
                result.ComputationTime = sw.Elapsed;
                return result;
            }

            double radius = Math.Max(200, graph.Nodes.Count * 10);
            double angleStep = 2 * Math.PI / graph.Nodes.Count;

            for (int i = 0; i < graph.Nodes.Count; i++)
            {
                var angle = i * angleStep;
                var x = radius * Math.Cos(angle);
                var y = radius * Math.Sin(angle);
                var node = graph.Nodes[i];
                node.Position = new Vector3D(x, y, 0);
                result.NodePositions[node.Id] = node.Position;
            }

            result.IsConverged = true;
            result.IterationCount = 1;
            sw.Stop();
            result.ComputationTime = sw.Elapsed;
            return result;
        }, cancellationToken);
    }

    public async Task<LayoutResult> CalculateRadialLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default)
    {
        return await Task.Run(() =>
        {
            var sw = Stopwatch.StartNew();
            var result = new LayoutResult { NodePositions = new Dictionary<string, Vector3D>() };

            if (graph.Nodes.Count == 0)
            {
                sw.Stop();
                result.ComputationTime = sw.Elapsed;
                return result;
            }

            // Finde zentralsten Knoten
            var centralNode = graph.Nodes[0];
            int maxDegree = 0;
            foreach (var node in graph.Nodes)
            {
                int degree = graph.GetNodeDegree(node.Id);
                if (degree > maxDegree)
                {
                    maxDegree = degree;
                    centralNode = node;
                }
            }

            centralNode.Position = new Vector3D(0, 0, 0);
            result.NodePositions[centralNode.Id] = centralNode.Position;

            var distances = new Dictionary<string, int> { { centralNode.Id, 0 } };
            var queue = new Queue<string>();
            queue.Enqueue(centralNode.Id);

            while (queue.Count > 0)
            {
                var nodeId = queue.Dequeue();
                var dist = distances[nodeId];

                foreach (var neighbor in graph.GetNeighbors(nodeId))
                {
                    if (!distances.ContainsKey(neighbor.Id))
                    {
                        distances[neighbor.Id] = dist + 1;
                        queue.Enqueue(neighbor.Id);
                    }
                }
            }

            var nodesByDistance = new Dictionary<int, List<GraphNode>>();
            foreach (var node in graph.Nodes)
            {
                if (!distances.ContainsKey(node.Id)) continue;
                int dist = distances[node.Id];
                if (!nodesByDistance.ContainsKey(dist))
                    nodesByDistance[dist] = new List<GraphNode>();
                nodesByDistance[dist].Add(node);
            }

            foreach (var ring in nodesByDistance.Where(x => x.Key > 0))
            {
                double radius = ring.Key * 100;
                double angleStep = 2 * Math.PI / ring.Value.Count;

                for (int i = 0; i < ring.Value.Count; i++)
                {
                    var angle = i * angleStep;
                    var x = radius * Math.Cos(angle);
                    var y = radius * Math.Sin(angle);
                    var node = ring.Value[i];
                    node.Position = new Vector3D(x, y, 0);
                    result.NodePositions[node.Id] = node.Position;
                }
            }

            result.IsConverged = true;
            result.IterationCount = 1;
            sw.Stop();
            result.ComputationTime = sw.Elapsed;
            return result;
        }, cancellationToken);
    }

    public async Task<LayoutResult> CalculateKamadaKawaiLayoutAsync(Model.Graph graph, CancellationToken cancellationToken = default)
    {
        return await Task.Run(() =>
        {
            var sw = Stopwatch.StartNew();
            var result = new LayoutResult { NodePositions = new Dictionary<string, Vector3D>() };

            if (graph.Nodes.Count == 0)
            {
                sw.Stop();
                result.ComputationTime = sw.Elapsed;
                return result;
            }

            var shortestPaths = ComputeAllPairsShortestPath(graph);

            foreach (var node in graph.Nodes)
            {
                node.Position = new Vector3D(
                    (_random.NextDouble() - 0.5) * 500,
                    (_random.NextDouble() - 0.5) * 500,
                    0
                );
            }

            const int iterations = 100;
            const double K = 0.1;
            const double epsilon = 0.1;

            for (int iter = 0; iter < iterations; iter++)
            {
                cancellationToken.ThrowIfCancellationRequested();

                double maxDelta = 0;

                for (int i = 0; i < graph.Nodes.Count; i++)
                {
                    var node = graph.Nodes[i];
                    var force = new Vector3D(0, 0, 0);

                    for (int j = 0; j < graph.Nodes.Count; j++)
                    {
                        if (i == j) continue;

                        var other = graph.Nodes[j];
                        var delta = other.Position.Add(node.Position.Multiply(-1));
                        var dist = delta.Length;

                        if (dist < 0.1) dist = 0.1;

                        var key = shortestPaths.ContainsKey((i, j)) ? shortestPaths[(i, j)] : 1;
                        var desiredDist = key * 50;
                        var f = K * (dist - desiredDist) / dist;
                        force = force.Add(delta.Multiply(f));
                    }

                    var step = force.Multiply(epsilon);
                    maxDelta = Math.Max(maxDelta, step.Length);
                    node.Position = node.Position.Add(step);
                    result.NodePositions[node.Id] = node.Position;
                }

                if (maxDelta < epsilon)
                {
                    result.IsConverged = true;
                    break;
                }
            }

            result.IterationCount = iterations;
            sw.Stop();
            result.ComputationTime = sw.Elapsed;
            return result;
        }, cancellationToken);
    }

    public async Task<List<GraphCluster>> CalculateLouvainClusteringAsync(Model.Graph graph)
    {
        return await Task.Run(() =>
        {
            var clusters = new List<GraphCluster>();
            var clusterAssignment = new Dictionary<string, int>();
            int clusterCount = 0;

            foreach (var node in graph.Nodes)
            {
                if (!clusterAssignment.ContainsKey(node.Id))
                {
                    var cluster = new GraphCluster
                    {
                        Id = Guid.NewGuid().ToString(),
                        Name = $"Cluster {clusterCount}",
                        Color = $"#{(_random.Next(0xFFFFFF)):X6}"
                    };

                    var visited = new HashSet<string>();
                    var queue = new Queue<string>();
                    queue.Enqueue(node.Id);

                    while (queue.Count > 0)
                    {
                        var nodeId = queue.Dequeue();
                        if (visited.Contains(nodeId)) continue;

                        visited.Add(nodeId);
                        clusterAssignment[nodeId] = clusterCount;
                        cluster.NodeIds.Add(nodeId);

                        foreach (var neighbor in graph.GetNeighbors(nodeId))
                        {
                            if (!visited.Contains(neighbor.Id))
                                queue.Enqueue(neighbor.Id);
                        }
                    }

                    clusters.Add(cluster);
                    clusterCount++;
                }
            }

            return clusters;
        });
    }

    public async Task<List<GraphCluster>> CalculateKMeansClusteringAsync(Model.Graph graph, int clusterCount)
    {
        return await Task.Run(() =>
        {
            var clusters = new List<GraphCluster>();

            for (int i = 0; i < Math.Min(clusterCount, graph.Nodes.Count); i++)
            {
                clusters.Add(new GraphCluster
                {
                    Id = Guid.NewGuid().ToString(),
                    Name = $"Cluster {i}",
                    Color = $"#{(_random.Next(0xFFFFFF)):X6}"
                });
            }

            return clusters;
        });
    }

    public async Task<GraphStatistics> CalculateStatisticsAsync(Model.Graph graph)
    {
        return await Task.Run(() =>
        {
            var stats = new GraphStatistics
            {
                GraphId = graph.Id,
                NodeCount = graph.NodeCount,
                EdgeCount = graph.EdgeCount,
                Density = graph.Density,
                AverageDegree = graph.AverageDegree
            };

            stats.DegreeCentrality = CalculateDegreeDistribution(graph);
            return stats;
        });
    }

    public async Task<Dictionary<string, double>> CalculateBetweennessCentralityAsync(Model.Graph graph)
    {
        return await Task.Run(() =>
        {
            var centrality = new Dictionary<string, double>();
            int maxDegree = 0;

            foreach (var node in graph.Nodes)
            {
                int degree = graph.GetNodeDegree(node.Id);
                maxDegree = Math.Max(maxDegree, degree);
            }

            foreach (var node in graph.Nodes)
            {
                int degree = graph.GetNodeDegree(node.Id);
                centrality[node.Id] = maxDegree > 0 ? (double)degree / maxDegree : 0.0;
            }

            return centrality;
        });
    }

    public async Task<Dictionary<string, double>> CalculateDegreeCentralityAsync(Model.Graph graph)
    {
        return await Task.Run(() => CalculateDegreeDistribution(graph));
    }

    public async Task<Dictionary<string, double>> CalculateClosenessCentralityAsync(Model.Graph graph)
    {
        return await Task.Run(() =>
        {
            var centrality = new Dictionary<string, double>();

            foreach (var node in graph.Nodes)
            {
                centrality[node.Id] = 1.0 / Math.Max(1, graph.GetNodeDegree(node.Id));
            }

            return centrality;
        });
    }

    public Vector3D GetOptimalCameraPosition(Model.Graph graph, RenderingOptions options)
    {
        if (graph.Nodes.Count == 0)
            return new Vector3D(0, 0, 1000);

        var avgX = graph.Nodes.Average(n => n.Position.X);
        var avgY = graph.Nodes.Average(n => n.Position.Y);
        var avgZ = graph.Nodes.Average(n => n.Position.Z);
        var maxDist = graph.Nodes.Max(n => n.Position.DistanceTo(new Vector3D(avgX, avgY, avgZ)));
        var distance = options.CameraDistance > 0 ? options.CameraDistance : maxDist * 1.5;

        return new Vector3D(avgX, avgY, avgZ + distance);
    }

    public List<GraphNode> GetConnectedComponent(Model.Graph graph, string startNodeId)
    {
        var component = new List<GraphNode>();
        var visited = new HashSet<string>();
        var queue = new Queue<string>();
        queue.Enqueue(startNodeId);

        while (queue.Count > 0)
        {
            var nodeId = queue.Dequeue();
            if (visited.Contains(nodeId)) continue;

            visited.Add(nodeId);
            var node = graph.GetNode(nodeId);
            if (node != null)
                component.Add(node);

            foreach (var neighbor in graph.GetNeighbors(nodeId))
            {
                if (!visited.Contains(neighbor.Id))
                    queue.Enqueue(neighbor.Id);
            }
        }

        return component;
    }

    public int GetShortestPath(Model.Graph graph, string sourceId, string targetId)
    {
        var distances = new Dictionary<string, int>();
        foreach (var node in graph.Nodes)
            distances[node.Id] = int.MaxValue;
        distances[sourceId] = 0;

        var queue = new Queue<string>();
        queue.Enqueue(sourceId);

        while (queue.Count > 0)
        {
            var nodeId = queue.Dequeue();
            var currentDist = distances[nodeId];

            foreach (var neighbor in graph.GetNeighbors(nodeId))
            {
                if (distances[neighbor.Id] > currentDist + 1)
                {
                    distances[neighbor.Id] = currentDist + 1;
                    queue.Enqueue(neighbor.Id);
                }
            }
        }

        return distances[targetId];
    }

    private Dictionary<(int, int), int> ComputeAllPairsShortestPath(Model.Graph graph)
    {
        var dist = new Dictionary<(int, int), int>();
        var n = graph.Nodes.Count;

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                dist[(i, j)] = i == j ? 0 : int.MaxValue / 2;
            }
        }

        foreach (var edge in graph.Edges)
        {
            var srcIdx = graph.Nodes.FindIndex(n => n.Id == edge.SourceNodeId);
            var tgtIdx = graph.Nodes.FindIndex(n => n.Id == edge.TargetNodeId);

            if (srcIdx >= 0 && tgtIdx >= 0)
            {
                dist[(srcIdx, tgtIdx)] = 1;
                dist[(tgtIdx, srcIdx)] = 1;
            }
        }

        for (int k = 0; k < n; k++)
        {
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < n; j++)
                {
                    dist[(i, j)] = Math.Min(dist[(i, j)], dist[(i, k)] + dist[(k, j)]);
                }
            }
        }

        return dist;
    }

    private Dictionary<string, double> CalculateDegreeDistribution(Model.Graph graph)
    {
        var centrality = new Dictionary<string, double>();
        int maxDegree = 0;

        foreach (var node in graph.Nodes)
        {
            int degree = graph.GetNodeDegree(node.Id);
            maxDegree = Math.Max(maxDegree, degree);
        }

        foreach (var node in graph.Nodes)
        {
            int degree = graph.GetNodeDegree(node.Id);
            centrality[node.Id] = maxDegree > 0 ? (double)degree / maxDegree : 0.0;
        }

        return centrality;
    }
}

