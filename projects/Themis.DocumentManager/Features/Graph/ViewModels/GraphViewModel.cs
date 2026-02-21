/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphViewModel.cs                                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     863                                            ║
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
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using Themis.DocumentManager.Models;
using Model = Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Features.Graph.Services;
using Themis.DocumentManager.Common;

namespace Themis.DocumentManager.Features.Graph.ViewModels
{
    /// <summary>
    /// ViewModel for GraphView - Manages 3D graph visualization state and interactions
    /// Follows MVVM pattern for clean separation of concerns
    /// </summary>
    public partial class GraphViewModel : INotifyPropertyChanged
    {
        private readonly IGraphVisualizationService _graphService;
        private readonly DsmLocalDataStore? _dsmStore;

        private Model.Graph? _currentGraph;
        private GraphNode? _selectedNode;
        private GraphEdge? _selectedEdge;
        private ObservableCollection<GraphNode> _nodes;
        private ObservableCollection<GraphEdge> _edges;
        private ObservableCollection<GraphCluster> _clusters;
        private LayoutAlgorithm _selectedLayoutAlgorithm;
        private GraphStatistics? _statistics;
        private bool _isLayoutCalculating;
        private bool _isLoading;
        private string _statusMessage = "Bereit";
        private CancellationTokenSource? _layoutCancellation;

        public GraphViewModel(IGraphVisualizationService graphService, DsmLocalDataStore? dsmStore = null)
        {
            _graphService = graphService ?? throw new ArgumentNullException(nameof(graphService));
            _dsmStore = dsmStore;

            _nodes = new ObservableCollection<GraphNode>();
            _edges = new ObservableCollection<GraphEdge>();
            _clusters = new ObservableCollection<GraphCluster>();
            _selectedLayoutAlgorithm = LayoutAlgorithm.ForceDirected;

            // Initialize commands
            LoadGraphCommand = new RelayCommand(async () => await LoadGraphAsync());
            CalculateLayoutCommand = new RelayCommand(async () => await CalculateLayoutAsync(), () => !IsLayoutCalculating);
            CalculateStatisticsCommand = new RelayCommand(async () => await CalculateStatisticsAsync());
            CalculateClustersCommand = new RelayCommand(async () => await CalculateClustersAsync());
            SelectNodeCommand = new RelayCommand<string>(async nodeId => await SelectNodeAsync(nodeId));
            SelectEdgeCommand = new RelayCommand<string>(async edgeId => await SelectEdgeAsync(edgeId));
            AddNodeCommand = new RelayCommand<GraphNode>(async node => await AddNodeAsync(node));
            RemoveNodeCommand = new RelayCommand<GraphNode>(async node => await RemoveNodeAsync(node));
            AddEdgeCommand = new RelayCommand<GraphEdge>(async edge => await AddEdgeAsync(edge));
            RemoveEdgeCommand = new RelayCommand<GraphEdge>(async edge => await RemoveEdgeAsync(edge));
            ResetViewCommand = new RelayCommand(async () => await ResetViewAsync());
            ExportGraphCommand = new RelayCommand(async () => await ExportGraphAsync());
        }

        #region Properties

        public Model.Graph? CurrentGraph
        {
            get => _currentGraph;
            set
            {
                if (_currentGraph != value)
                {
                    _currentGraph = value;
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(HasGraph));
                    OnPropertyChanged(nameof(NodeCount));
                    OnPropertyChanged(nameof(EdgeCount));
                }
            }
        }

        public GraphNode? SelectedNode
        {
            get => _selectedNode;
            set
            {
                if (_selectedNode != value)
                {
                    _selectedNode = value;
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(HasSelectedNode));
                    OnPropertyChanged(nameof(SelectedNodeInfo));
                }
            }
        }

        public GraphEdge? SelectedEdge
        {
            get => _selectedEdge;
            set
            {
                if (_selectedEdge != value)
                {
                    _selectedEdge = value;
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(HasSelectedEdge));
                    OnPropertyChanged(nameof(SelectedEdgeInfo));
                }
            }
        }

        public ObservableCollection<GraphNode> Nodes
        {
            get => _nodes;
            set
            {
                if (_nodes != value)
                {
                    _nodes = value;
                    OnPropertyChanged();
                }
            }
        }

        public ObservableCollection<GraphEdge> Edges
        {
            get => _edges;
            set
            {
                if (_edges != value)
                {
                    _edges = value;
                    OnPropertyChanged();
                }
            }
        }

        public ObservableCollection<GraphCluster> Clusters
        {
            get => _clusters;
            set
            {
                if (_clusters != value)
                {
                    _clusters = value;
                    OnPropertyChanged();
                }
            }
        }

        public LayoutAlgorithm SelectedLayoutAlgorithm
        {
            get => _selectedLayoutAlgorithm;
            set
            {
                if (_selectedLayoutAlgorithm != value)
                {
                    _selectedLayoutAlgorithm = value;
                    OnPropertyChanged();
                }
            }
        }

        public GraphStatistics? Statistics
        {
            get => _statistics;
            set
            {
                if (_statistics != value)
                {
                    _statistics = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool IsLayoutCalculating
        {
            get => _isLayoutCalculating;
            set
            {
                if (_isLayoutCalculating != value)
                {
                    _isLayoutCalculating = value;
                    OnPropertyChanged();
                    CommandManager.InvalidateRequerySuggested();
                }
            }
        }

        public bool IsLoading
        {
            get => _isLoading;
            set
            {
                if (_isLoading != value)
                {
                    _isLoading = value;
                    OnPropertyChanged();
                }
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set
            {
                if (_statusMessage != value)
                {
                    _statusMessage = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool HasGraph => _currentGraph != null;
        public bool HasSelectedNode => _selectedNode != null;
        public bool HasSelectedEdge => _selectedEdge != null;
        public int NodeCount => _currentGraph?.Nodes?.Count ?? 0;
        public int EdgeCount => _currentGraph?.Edges?.Count ?? 0;

        public string SelectedNodeInfo
        {
            get
            {
                if (_selectedNode == null || _currentGraph == null)
                    return "Kein Knoten ausgewählt";

                var degree = _currentGraph.GetNodeDegree(_selectedNode.Id);
                var neighbors = _currentGraph.GetNeighbors(_selectedNode.Id);

                return $"Knoten: {_selectedNode.Label ?? _selectedNode.Id}\n" +
                       $"Typ: {_selectedNode.Type}\n" +
                       $"Grad: {degree}\n" +
                       $"Nachbarn: {neighbors.Count}\n" +
                       $"Position: ({_selectedNode.Position.X:F2}, {_selectedNode.Position.Y:F2}, {_selectedNode.Position.Z:F2})\n" +
                       $"Cluster: {_selectedNode.ClusterId ?? "Keine"}";
            }
        }

        public string SelectedEdgeInfo
        {
            get
            {
                if (_selectedEdge == null)
                    return "Keine Kante ausgewählt";

                return $"Kante: {_selectedEdge.Label ?? _selectedEdge.Id}\n" +
                       $"Von: {_selectedEdge.SourceNodeId}\n" +
                       $"Nach: {_selectedEdge.TargetNodeId}\n" +
                       $"Gewicht: {_selectedEdge.Weight:F2}\n" +
                       $"Typ: {_selectedEdge.RelationType}";
            }
        }

        #endregion

        #region Commands

        public ICommand LoadGraphCommand { get; }
        public ICommand CalculateLayoutCommand { get; }
        public ICommand CalculateStatisticsCommand { get; }
        public ICommand CalculateClustersCommand { get; }
        public ICommand SelectNodeCommand { get; }
        public ICommand SelectEdgeCommand { get; }
        public ICommand AddNodeCommand { get; }
        public ICommand RemoveNodeCommand { get; }
        public ICommand AddEdgeCommand { get; }
        public ICommand RemoveEdgeCommand { get; }
        public ICommand ResetViewCommand { get; }
        public ICommand ExportGraphCommand { get; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Initialize ViewModel with example graph
        /// </summary>
        public async Task InitializeAsync()
        {
            await LoadDefaultGraphAsync();
        }

        /// <summary>
        /// Load specific graph
        /// </summary>
        public async Task LoadGraphAsync(Model.Graph graph)
        {
            if (graph == null)
                throw new ArgumentNullException(nameof(graph));

            try
            {
                IsLoading = true;
                StatusMessage = $"Lade Graph '{graph.Name}'...";

                CurrentGraph = graph;

                // Update collections
                Nodes.Clear();
                foreach (var node in graph.Nodes)
                {
                    Nodes.Add(node);
                }

                Edges.Clear();
                foreach (var edge in graph.Edges)
                {
                    Edges.Add(edge);
                }

                // Calculate layout if nodes have no positions
                if (graph.Nodes.Any(n => n.Position.X == 0 && n.Position.Y == 0 && n.Position.Z == 0))
                {
                    await CalculateLayoutAsync();
                }

                // Calculate statistics
                await CalculateStatisticsAsync();

                StatusMessage = $"Graph '{graph.Name}' geladen - {NodeCount} Knoten, {EdgeCount} Kanten";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Laden des Graphen: {ex.Message}";
                throw;
            }
            finally
            {
                IsLoading = false;
            }
        }

        /// <summary>
        /// Calculate graph layout using selected algorithm
        /// </summary>
        public async Task CalculateLayoutAsync()
        {
            if (CurrentGraph == null)
                return;

            try
            {
                IsLayoutCalculating = true;
                StatusMessage = $"Berechne Layout ({SelectedLayoutAlgorithm})...";

                // Cancel previous calculation
                _layoutCancellation?.Cancel();
                _layoutCancellation = new CancellationTokenSource();

                LayoutResult result;

                switch (SelectedLayoutAlgorithm)
                {
                    case LayoutAlgorithm.ForceDirected:
                        var forceParams = new ForceDirectedLayoutParams
                        {
                            K = 100.0,
                            Iterations = 500,
                            Use3D = true,
                            StepSize = 0.1,
                            Cooling = 0.95
                        };
                        result = await _graphService.CalculateForceDirectedLayoutAsync(
                            CurrentGraph, forceParams, _layoutCancellation.Token);
                        break;

                    case LayoutAlgorithm.Circular:
                        result = await _graphService.CalculateCircularLayoutAsync(
                            CurrentGraph, _layoutCancellation.Token);
                        break;

                    case LayoutAlgorithm.Hierarchical:
                        result = await _graphService.CalculateHierarchicalLayoutAsync(
                            CurrentGraph, _layoutCancellation.Token);
                        break;

                    case LayoutAlgorithm.Radial:
                        result = await _graphService.CalculateRadialLayoutAsync(
                            CurrentGraph, _layoutCancellation.Token);
                        break;

                    default:
                        result = await _graphService.CalculateForceDirectedLayoutAsync(
                            CurrentGraph, null, _layoutCancellation.Token);
                        break;
                }

                // Apply positions to nodes
                if (result.NodePositions != null)
                {
                    foreach (var kvp in result.NodePositions)
                    {
                        var node = CurrentGraph.Nodes.FirstOrDefault(n => n.Id == kvp.Key);
                        if (node != null)
                        {
                            node.Position = kvp.Value;
                        }
                    }
                }

                StatusMessage = $"Layout berechnet in {result.ComputationTime.TotalMilliseconds:F0}ms ({result.IterationCount} Iterationen)";
            }
            catch (OperationCanceledException)
            {
                StatusMessage = "Layout-Berechnung abgebrochen";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler bei Layout-Berechnung: {ex.Message}";
                throw;
            }
            finally
            {
                IsLayoutCalculating = false;
            }
        }

        /// <summary>
        /// Calculate graph statistics
        /// </summary>
        public async Task CalculateStatisticsAsync()
        {
            if (CurrentGraph == null)
                return;

            try
            {
                StatusMessage = "Berechne Statistiken...";

                Statistics = await _graphService.CalculateStatisticsAsync(CurrentGraph);

                StatusMessage = $"Statistiken: Dichte {Statistics.Density:F4}, Durchschn. Grad {Statistics.AverageDegree:F2}";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler bei Statistik-Berechnung: {ex.Message}";
            }
        }

        /// <summary>
        /// Calculate graph clusters
        /// </summary>
        public async Task CalculateClustersAsync()
        {
            if (CurrentGraph == null)
                return;

            try
            {
                StatusMessage = "Berechne Cluster...";

                var clusters = await _graphService.CalculateLouvainClusteringAsync(CurrentGraph);

                Clusters.Clear();
                foreach (var cluster in clusters)
                {
                    Clusters.Add(cluster);
                }

                StatusMessage = $"{Clusters.Count} Cluster gefunden";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler bei Cluster-Berechnung: {ex.Message}";
            }
        }

        /// <summary>
        /// Select node by ID
        /// </summary>
        public async Task SelectNodeAsync(string? nodeId)
        {
            if (CurrentGraph == null || string.IsNullOrEmpty(nodeId))
                return;

            SelectedNode = CurrentGraph.Nodes.FirstOrDefault(n => n.Id == nodeId);
            SelectedEdge = null;

            await Task.CompletedTask;
        }

        /// <summary>
        /// Select edge by ID
        /// </summary>
        public async Task SelectEdgeAsync(string? edgeId)
        {
            if (CurrentGraph == null || string.IsNullOrEmpty(edgeId))
                return;

            SelectedEdge = CurrentGraph.Edges.FirstOrDefault(e => e.Id == edgeId);
            SelectedNode = null;

            await Task.CompletedTask;
        }

        /// <summary>
        /// Add node to graph
        /// </summary>
        public async Task AddNodeAsync(GraphNode? node)
        {
            if (CurrentGraph == null || node == null)
                return;

            try
            {
                CurrentGraph.Nodes.Add(node);
                Nodes.Add(node);

                OnPropertyChanged(nameof(NodeCount));
                StatusMessage = $"Knoten '{node.Label}' hinzugefügt";

                await Task.CompletedTask;
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Hinzufügen des Knotens: {ex.Message}";
            }
        }

        /// <summary>
        /// Remove node from graph
        /// </summary>
        public async Task RemoveNodeAsync(GraphNode? node)
        {
            if (CurrentGraph == null || node == null)
                return;

            try
            {
                // Remove connected edges
                var connectedEdges = CurrentGraph.Edges
                    .Where(e => e.SourceNodeId == node.Id || e.TargetNodeId == node.Id)
                    .ToList();

                foreach (var edge in connectedEdges)
                {
                    CurrentGraph.Edges.Remove(edge);
                    Edges.Remove(edge);
                }

                CurrentGraph.Nodes.Remove(node);
                Nodes.Remove(node);

                if (SelectedNode == node)
                {
                    SelectedNode = null;
                }

                OnPropertyChanged(nameof(NodeCount));
                OnPropertyChanged(nameof(EdgeCount));
                StatusMessage = $"Knoten '{node.Label}' und {connectedEdges.Count} Kanten entfernt";

                await Task.CompletedTask;
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Entfernen des Knotens: {ex.Message}";
            }
        }

        /// <summary>
        /// Add edge to graph
        /// </summary>
        public async Task AddEdgeAsync(GraphEdge? edge)
        {
            if (CurrentGraph == null || edge == null)
                return;

            try
            {
                // Validate source and target nodes exist
                if (!CurrentGraph.Nodes.Any(n => n.Id == edge.SourceNodeId))
                {
                    StatusMessage = $"Quell-Knoten '{edge.SourceNodeId}' nicht gefunden";
                    return;
                }

                if (!CurrentGraph.Nodes.Any(n => n.Id == edge.TargetNodeId))
                {
                    StatusMessage = $"Ziel-Knoten '{edge.TargetNodeId}' nicht gefunden";
                    return;
                }

                CurrentGraph.Edges.Add(edge);
                Edges.Add(edge);

                OnPropertyChanged(nameof(EdgeCount));
                StatusMessage = $"Kante hinzugefügt: {edge.SourceNodeId} → {edge.TargetNodeId}";

                await Task.CompletedTask;
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Hinzufügen der Kante: {ex.Message}";
            }
        }

        /// <summary>
        /// Remove edge from graph
        /// </summary>
        public async Task RemoveEdgeAsync(GraphEdge? edge)
        {
            if (CurrentGraph == null || edge == null)
                return;

            try
            {
                CurrentGraph.Edges.Remove(edge);
                Edges.Remove(edge);

                if (SelectedEdge == edge)
                {
                    SelectedEdge = null;
                }

                OnPropertyChanged(nameof(EdgeCount));
                StatusMessage = $"Kante entfernt";

                await Task.CompletedTask;
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Entfernen der Kante: {ex.Message}";
            }
        }

        #endregion

        #region Private Methods

        private async Task LoadGraphAsync()
        {
            await LoadDefaultGraphAsync();
        }

        private async Task LoadDefaultGraphAsync()
        {
            if (_dsmStore != null)
            {
                var edges = await _dsmStore.GetAllEdgesAsync();
                if (edges.Any())
                {
                    var graph = BuildGraphFromDsmEdges(edges);
                    await LoadGraphAsync(graph);
                    return;
                }
            }

            var fallback = CreateExampleGraph();
            await LoadGraphAsync(fallback);
        }

        private Model.Graph BuildGraphFromDsmEdges(IEnumerable<DsmEntityGraphEdge> edges)
        {
            var graph = new Model.Graph
            {
                Id = "dsm-local-graph",
                Name = "DSM Graph",
                Nodes = new List<GraphNode>(),
                Edges = new List<GraphEdge>()
            };

            var nodes = new Dictionary<string, GraphNode>(StringComparer.OrdinalIgnoreCase);

            foreach (var edge in edges)
            {
                var sourceId = $"{edge.SourceType}:{edge.SourceId}";
                var targetId = $"{edge.TargetType}:{edge.TargetId}";

                if (!nodes.ContainsKey(sourceId))
                {
                    nodes[sourceId] = new GraphNode
                    {
                        Id = sourceId,
                        Label = sourceId,
                        Type = edge.SourceType.Equals("document", StringComparison.OrdinalIgnoreCase) ? GraphNodeType.Document : GraphNodeType.Entity,
                        Color = edge.SourceType.Equals("document", StringComparison.OrdinalIgnoreCase) ? "#2196F3" : "#4CAF50",
                        EntityId = edge.SourceId,
                        Data = new Dictionary<string, object>
                        {
                            {"entityType", edge.SourceType},
                            {"entityId", edge.SourceId}
                        }
                    };
                }

                if (!nodes.ContainsKey(targetId))
                {
                    nodes[targetId] = new GraphNode
                    {
                        Id = targetId,
                        Label = targetId,
                        Type = edge.TargetType.Equals("document", StringComparison.OrdinalIgnoreCase) ? GraphNodeType.Document : GraphNodeType.Entity,
                        Color = edge.TargetType.Equals("document", StringComparison.OrdinalIgnoreCase) ? "#2196F3" : "#4CAF50",
                        EntityId = edge.TargetId,
                        Data = new Dictionary<string, object>
                        {
                            {"entityType", edge.TargetType},
                            {"entityId", edge.TargetId}
                        }
                    };
                }

                graph.Edges.Add(new GraphEdge
                {
                    Id = string.IsNullOrWhiteSpace(edge.Id) ? Guid.NewGuid().ToString("N") : edge.Id,
                    SourceNodeId = sourceId,
                    TargetNodeId = targetId,
                    RelationType = edge.Relation,
                    Label = edge.Relation,
                    Strength = edge.Weight,
                    Weight = edge.Weight,
                    Data = new Dictionary<string, object>(edge.Properties)
                });
            }

            graph.Nodes = nodes.Values.ToList();
            graph.Description = "Aus DSM-Cache aufgebaut";
            return graph;
        }

        private Model.Graph CreateExampleGraph()
        {
            var graph = new Model.Graph
            {
                Id = "example-graph",
                Name = "Beispiel Graph",
                Nodes = new List<GraphNode>(),
                Edges = new List<GraphEdge>()
            };

            // Create nodes (simple network with 20 nodes)
            for (int i = 0; i < 20; i++)
            {
                graph.Nodes.Add(new GraphNode
                {
                    Id = $"node-{i}",
                    Label = $"Knoten {i}",
                    Type = i < 5 ? GraphNodeType.Central : GraphNodeType.Standard,
                    Position = new Vector3D { X = 0, Y = 0, Z = 0 },
                    Velocity = new Vector3D { X = 0, Y = 0, Z = 0 },
                    Radius = i < 5 ? 30 : 20,
                    Mass = i < 5 ? 2.0 : 1.0,
                    Color = i < 5 ? "#2196F3" : "#4CAF50",
                    IconShape = "circle"
                });
            }

            // Create hub structure
            var random = new Random(42);
            for (int hub = 0; hub < 5; hub++)
            {
                for (int i = 0; i < 4; i++)
                {
                    int targetIdx = 5 + hub * 3 + i;
                    if (targetIdx < 20)
                    {
                        graph.Edges.Add(new GraphEdge
                        {
                            Id = $"edge-{hub}-{targetIdx}",
                            SourceNodeId = $"node-{hub}",
                            TargetNodeId = $"node-{targetIdx}",
                            Label = "",
                            RelationType = "connection",
                            Strength = 1.0,
                            Weight = 1.0,
                            Color = "#666666",
                            StrokeWidth = 2,
                            IsDirected = false
                        });
                    }
                }
            }

            // Random connections
            for (int i = 0; i < 10; i++)
            {
                int source = random.Next(5, 20);
                int target = random.Next(5, 20);

                if (source != target && !graph.Edges.Any(e =>
                    (e.SourceNodeId == $"node-{source}" && e.TargetNodeId == $"node-{target}") ||
                    (e.SourceNodeId == $"node-{target}" && e.TargetNodeId == $"node-{source}")))
                {
                    graph.Edges.Add(new GraphEdge
                    {
                        Id = $"edge-rand-{i}",
                        SourceNodeId = $"node-{source}",
                        TargetNodeId = $"node-{target}",
                        RelationType = "connection",
                        Strength = 0.5,
                        Weight = 1.0,
                        Color = "#CCCCCC",
                        StrokeWidth = 1,
                        IsDirected = false
                    });
                }
            }

            return graph;
        }

        private async Task ResetViewAsync()
        {
            // This would trigger an event for the View to reset camera
            StatusMessage = "Ansicht zurückgesetzt";
            await Task.CompletedTask;
        }

        private async Task ExportGraphAsync()
        {
            if (CurrentGraph == null)
                return;

            // This would trigger a file save dialog in the View
            StatusMessage = "Graph-Export...";
            await Task.CompletedTask;
        }

        #endregion

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion
    }
}
