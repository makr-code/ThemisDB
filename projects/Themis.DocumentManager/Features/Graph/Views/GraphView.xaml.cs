/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphView.xaml.cs                                  ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     722                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8c92adc5e  2025-12-16  Restructure DocumentManager features into modular folders ║
    • e35bb0178  2025-12-10  Phase 25: Complete UI implementation (GeoView, GraphView,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.Wpf;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Features.Graph.Services;
using Model = Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.Graph.Views
{
    /// <summary>
    /// Interaction logic for GraphView.xaml
    /// Displays 3D graph visualization using Three.js/Babylon.js via WebGL
    /// </summary>
    public partial class GraphView : UserControl
    {
        private readonly IGraphVisualizationService _graphService;
        
        private WebView2? _webView;
        private Model.Graph? _currentGraph;
        private LayoutAlgorithm _currentLayoutAlgorithm = LayoutAlgorithm.ForceDirected;
        private bool _isGraphInitialized = false;
        private GraphNode? _selectedNode;
        private CancellationTokenSource? _layoutCancellation;

        public GraphView()
        {
            InitializeComponent();
            
            // Service via DI
            _graphService = App.GetService<IGraphVisualizationService>() ?? 
                throw new InvalidOperationException("IGraphVisualizationService not registered");

            Loaded += OnLoaded;
            
            // Wire up UI events
            LayoutCombo.SelectionChanged += OnLayoutSelectionChanged;
            ResetViewBtn.Click += OnResetViewClicked;
            ExportBtn.Click += OnExportClicked;
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            await InitializeGraphAsync();
        }

        /// <summary>
        /// Initialize WebView2 and load Three.js/WebGL viewer
        /// </summary>
        private async Task InitializeGraphAsync()
        {
            try
            {
                // Create WebView2 instance
                _webView = new WebView2();
                _webView.CoreWebView2InitializationCompleted += OnWebViewInitialized;

                // Add to Frame
                GraphFrame.Content = _webView;

                // Initialize WebView2 environment
                await _webView.EnsureCoreWebView2Async(null);

                // Load default example graph
                await LoadDefaultGraphAsync();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler beim Initialisieren des Graphen: {ex.Message}", 
                    "Graph-Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void OnWebViewInitialized(object? sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            if (e.IsSuccess)
            {
                _isGraphInitialized = true;
                
                // Enable developer tools in debug mode
#if DEBUG
                if (_webView?.CoreWebView2 != null)
                {
                    _webView.CoreWebView2.Settings.AreDevToolsEnabled = true;
                    
                    // Setup message handler for node selection
                    _webView.CoreWebView2.WebMessageReceived += OnWebMessageReceived;
                }
#else
                if (_webView?.CoreWebView2 != null)
                {
                    _webView.CoreWebView2.WebMessageReceived += OnWebMessageReceived;
                }
#endif
            }
            else
            {
                MessageBox.Show($"WebView2 Initialisierung fehlgeschlagen: {e.InitializationException?.Message}",
                    "WebView2 Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Handle messages from JavaScript (e.g., node clicks)
        /// </summary>
        private void OnWebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
        {
            try
            {
                var message = e.TryGetWebMessageAsString();
                if (string.IsNullOrEmpty(message))
                    return;

                // Parse message (expected format: "NODE_CLICK:nodeId")
                if (message.StartsWith("NODE_CLICK:"))
                {
                    var nodeId = message.Substring("NODE_CLICK:".Length);
                    OnNodeSelected(nodeId);
                }
                else if (message.StartsWith("EDGE_CLICK:"))
                {
                    var edgeId = message.Substring("EDGE_CLICK:".Length);
                    OnEdgeSelected(edgeId);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error handling web message: {ex.Message}");
            }
        }

        /// <summary>
        /// Handle node selection from graph
        /// </summary>
        private void OnNodeSelected(string nodeId)
        {
            if (_currentGraph == null)
                return;

            _selectedNode = _currentGraph.Nodes.FirstOrDefault(n => n.Id == nodeId);
            
            if (_selectedNode != null)
            {
                UpdateNodeInfoPanel();
            }
        }

        /// <summary>
        /// Handle edge selection from graph
        /// </summary>
        private void OnEdgeSelected(string edgeId)
        {
            if (_currentGraph == null)
                return;

            var edge = _currentGraph.Edges.FirstOrDefault(e => e.Id == edgeId);
            
            if (edge != null)
            {
                SelectedNodeInfo.Text = $"Kante: {edge.Label ?? edge.Id}\n" +
                                        $"Von: {edge.SourceNodeId}\n" +
                                        $"Nach: {edge.TargetNodeId}\n" +
                                        $"Gewicht: {edge.Weight:F2}\n" +
                                        $"Typ: {edge.RelationType}";
            }
        }

        /// <summary>
        /// Update node information panel
        /// </summary>
        private void UpdateNodeInfoPanel()
        {
            if (_selectedNode == null || _currentGraph == null)
            {
                SelectedNodeInfo.Text = "Kein Knoten ausgewählt";
                return;
            }

            // Calculate node degree
            var degree = _currentGraph.GetNodeDegree(_selectedNode.Id);
            var neighbors = _currentGraph.GetNeighbors(_selectedNode.Id);

            SelectedNodeInfo.Text = $"Knoten: {_selectedNode.Label ?? _selectedNode.Id}\n" +
                                    $"Typ: {_selectedNode.Type}\n" +
                                    $"Grad: {degree}\n" +
                                    $"Nachbarn: {neighbors.Count}\n" +
                                    $"Position: ({_selectedNode.Position.X:F2}, {_selectedNode.Position.Y:F2}, {_selectedNode.Position.Z:F2})\n" +
                                    $"Cluster: {_selectedNode.ClusterId ?? "Keine"}";
        }

        /// <summary>
        /// Load default example graph
        /// </summary>
        private async Task LoadDefaultGraphAsync()
        {
            // Create example graph with social network structure
            _currentGraph = CreateExampleGraph();

            // Calculate layout
            await CalculateLayoutAsync();

            // Render graph
            await RenderGraphAsync();
        }

        /// <summary>
        /// Create example graph for demonstration
        /// </summary>
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
                    Position = new Vector3D { X = 0, Y = 0, Z = 0 }, // Will be calculated by layout
                    Velocity = new Vector3D { X = 0, Y = 0, Z = 0 },
                    Radius = i < 5 ? 30 : 20,
                    Mass = i < 5 ? 2.0 : 1.0,
                    Color = i < 5 ? "#2196F3" : "#4CAF50",
                    IconShape = "circle"
                });
            }

            // Create edges (connect nodes to create interesting structure)
            var random = new Random(42); // Fixed seed for reproducibility
            
            // Create hub structure (central nodes connected to others)
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

            // Add some random connections between non-hub nodes
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

        /// <summary>
        /// Calculate graph layout using selected algorithm
        /// </summary>
        private async Task CalculateLayoutAsync()
        {
            if (_currentGraph == null)
                return;

            try
            {
                // Cancel previous layout calculation
                _layoutCancellation?.Cancel();
                _layoutCancellation = new CancellationTokenSource();

                LayoutResult result;

                switch (_currentLayoutAlgorithm)
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
                            _currentGraph, forceParams, _layoutCancellation.Token);
                        break;

                    case LayoutAlgorithm.Circular:
                        result = await _graphService.CalculateCircularLayoutAsync(
                            _currentGraph, _layoutCancellation.Token);
                        break;

                    case LayoutAlgorithm.Hierarchical:
                        result = await _graphService.CalculateHierarchicalLayoutAsync(
                            _currentGraph, _layoutCancellation.Token);
                        break;

                    case LayoutAlgorithm.Radial:
                        result = await _graphService.CalculateRadialLayoutAsync(
                            _currentGraph, _layoutCancellation.Token);
                        break;

                    default:
                        result = await _graphService.CalculateForceDirectedLayoutAsync(
                            _currentGraph, null, _layoutCancellation.Token);
                        break;
                }

                // Apply calculated positions to nodes
                if (result.NodePositions != null)
                {
                    foreach (var kvp in result.NodePositions)
                    {
                        var node = _currentGraph.Nodes.FirstOrDefault(n => n.Id == kvp.Key);
                        if (node != null)
                        {
                            node.Position = kvp.Value;
                        }
                    }
                }
            }
            catch (OperationCanceledException)
            {
                // Layout calculation was cancelled, ignore
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler bei Layout-Berechnung: {ex.Message}",
                    "Layout-Fehler", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Render graph using Three.js WebGL
        /// </summary>
        private async Task RenderGraphAsync()
        {
            if (_webView?.CoreWebView2 == null || _currentGraph == null)
                return;

            try
            {
                // Generate Three.js HTML
                var html = GenerateThreeJsHtml();

                // Navigate to HTML
                _webView.CoreWebView2.NavigateToString(html);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler beim Rendern des Graphen: {ex.Message}",
                    "Render-Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Generate complete HTML with Three.js for graph rendering
        /// </summary>
        private string GenerateThreeJsHtml()
        {
            if (_currentGraph == null)
                return "<html><body>No graph loaded</body></html>";

            // Convert graph to JSON
            var nodesJson = string.Join(",\n", _currentGraph.Nodes.Select(n => $@"
                {{
                    id: ""{n.Id}"",
                    label: ""{n.Label ?? n.Id}"",
                    x: {n.Position.X},
                    y: {n.Position.Y},
                    z: {n.Position.Z},
                    color: ""{n.Color}"",
                    radius: {n.Radius}
                }}"));

            var edgesJson = string.Join(",\n", _currentGraph.Edges.Select(e => $@"
                {{
                    id: ""{e.Id}"",
                    source: ""{e.SourceNodeId}"",
                    target: ""{e.TargetNodeId}"",
                    color: ""{e.Color}"",
                    width: {e.StrokeWidth}
                }}"));

            return $@"
<!DOCTYPE html>
<html>
<head>
    <meta charset=""utf-8"">
    <title>Graph Visualization</title>
    <style>
        body {{ margin: 0; overflow: hidden; background: #f5f5f5; }}
        #info {{
            position: absolute;
            top: 10px;
            left: 10px;
            background: rgba(255,255,255,0.9);
            padding: 10px;
            border-radius: 4px;
            font-family: Arial, sans-serif;
            font-size: 12px;
        }}
    </style>
</head>
<body>
    <div id=""info"">
        Knoten: {_currentGraph.Nodes.Count}<br>
        Kanten: {_currentGraph.Edges.Count}<br>
        Layout: {_currentLayoutAlgorithm}<br>
        <small>Klicken & Ziehen: Rotieren | Mausrad: Zoom</small>
    </div>
    
    <script src=""https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js""></script>
    <script>
        // Graph data
        const nodes = [{nodesJson}];
        const edges = [{edgesJson}];

        // Scene setup
        const scene = new THREE.Scene();
        scene.background = new THREE.Color(0xf5f5f5);
        
        const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 10000);
        camera.position.set(0, 0, 500);
        
        const renderer = new THREE.WebGLRenderer({{ antialias: true }});
        renderer.setSize(window.innerWidth, window.innerHeight);
        document.body.appendChild(renderer.domElement);

        // Lighting
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
        scene.add(ambientLight);
        
        const directionalLight = new THREE.DirectionalLight(0xffffff, 0.4);
        directionalLight.position.set(100, 100, 100);
        scene.add(directionalLight);

        // Node meshes
        const nodeMeshes = [];
        nodes.forEach(node => {{
            const geometry = new THREE.SphereGeometry(node.radius, 32, 32);
            const material = new THREE.MeshPhongMaterial({{ 
                color: node.color,
                emissive: node.color,
                emissiveIntensity: 0.2
            }});
            const sphere = new THREE.Mesh(geometry, material);
            sphere.position.set(node.x, node.y, node.z);
            sphere.userData = {{ nodeId: node.id, label: node.label }};
            scene.add(sphere);
            nodeMeshes.push(sphere);
        }});

        // Edge lines
        const edgeMeshes = [];
        edges.forEach(edge => {{
            const sourceNode = nodes.find(n => n.id === edge.source);
            const targetNode = nodes.find(n => n.id === edge.target);
            
            if (sourceNode && targetNode) {{
                const points = [];
                points.push(new THREE.Vector3(sourceNode.x, sourceNode.y, sourceNode.z));
                points.push(new THREE.Vector3(targetNode.x, targetNode.y, targetNode.z));
                
                const geometry = new THREE.BufferGeometry().setFromPoints(points);
                const material = new THREE.LineBasicMaterial({{ 
                    color: edge.color,
                    linewidth: edge.width
                }});
                const line = new THREE.Line(geometry, material);
                line.userData = {{ edgeId: edge.id }};
                scene.add(line);
                edgeMeshes.push(line);
            }}
        }});

        // Mouse interaction
        const raycaster = new THREE.Raycaster();
        const mouse = new THREE.Vector2();
        let isDragging = false;
        let previousMousePosition = {{ x: 0, y: 0 }};

        renderer.domElement.addEventListener('mousedown', (e) => {{
            isDragging = true;
            previousMousePosition = {{ x: e.clientX, y: e.clientY }};
        }});

        renderer.domElement.addEventListener('mousemove', (e) => {{
            if (isDragging) {{
                const deltaX = e.clientX - previousMousePosition.x;
                const deltaY = e.clientY - previousMousePosition.y;
                
                scene.rotation.y += deltaX * 0.01;
                scene.rotation.x += deltaY * 0.01;
                
                previousMousePosition = {{ x: e.clientX, y: e.clientY }};
            }}
        }});

        renderer.domElement.addEventListener('mouseup', () => {{
            isDragging = false;
        }});

        renderer.domElement.addEventListener('click', (e) => {{
            if (isDragging) return;
            
            mouse.x = (e.clientX / window.innerWidth) * 2 - 1;
            mouse.y = -(e.clientY / window.innerHeight) * 2 + 1;
            
            raycaster.setFromCamera(mouse, camera);
            const intersects = raycaster.intersectObjects(nodeMeshes);
            
            if (intersects.length > 0) {{
                const nodeId = intersects[0].object.userData.nodeId;
                window.chrome.webview.postMessage('NODE_CLICK:' + nodeId);
            }}
        }});

        // Mouse wheel zoom
        renderer.domElement.addEventListener('wheel', (e) => {{
            e.preventDefault();
            camera.position.z += e.deltaY * 0.5;
            camera.position.z = Math.max(100, Math.min(2000, camera.position.z));
        }});

        // Window resize
        window.addEventListener('resize', () => {{
            camera.aspect = window.innerWidth / window.innerHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(window.innerWidth, window.innerHeight);
        }});

        // Animation loop
        function animate() {{
            requestAnimationFrame(animate);
            renderer.render(scene, camera);
        }}
        animate();

        // Public API for C# interaction
        window.resetView = function() {{
            camera.position.set(0, 0, 500);
            scene.rotation.set(0, 0, 0);
        }};

        window.highlightNode = function(nodeId) {{
            nodeMeshes.forEach(mesh => {{
                if (mesh.userData.nodeId === nodeId) {{
                    mesh.material.emissiveIntensity = 0.8;
                }} else {{
                    mesh.material.emissiveIntensity = 0.2;
                }}
            }});
        }};
    </script>
</body>
</html>";
        }

        /// <summary>
        /// Handle layout selection change
        /// </summary>
        private async void OnLayoutSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (LayoutCombo.SelectedItem is ComboBoxItem item && 
                Enum.TryParse<LayoutAlgorithm>(item.Tag?.ToString(), out var algorithm))
            {
                _currentLayoutAlgorithm = algorithm;
                await CalculateLayoutAsync();
                await RenderGraphAsync();
            }
        }

        /// <summary>
        /// Reset camera view
        /// </summary>
        private async void OnResetViewClicked(object sender, RoutedEventArgs e)
        {
            if (_webView?.CoreWebView2 != null && _isGraphInitialized)
            {
                try
                {
                    await _webView.CoreWebView2.ExecuteScriptAsync("window.resetView();");
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error resetting view: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Export graph data
        /// </summary>
        private async void OnExportClicked(object sender, RoutedEventArgs e)
        {
            if (_currentGraph == null)
                return;

            try
            {
                var dialog = new Microsoft.Win32.SaveFileDialog
                {
                    Filter = "JSON Dateien (*.json)|*.json|Alle Dateien (*.*)|*.*",
                    FileName = $"{_currentGraph.Name ?? "graph"}.json"
                };

                if (dialog.ShowDialog() == true)
                {
                    var json = System.Text.Json.JsonSerializer.Serialize(_currentGraph, new System.Text.Json.JsonSerializerOptions
                    {
                        WriteIndented = true
                    });

                    await System.IO.File.WriteAllTextAsync(dialog.FileName, json);

                    MessageBox.Show($"Graph erfolgreich exportiert nach:\n{dialog.FileName}",
                        "Export erfolgreich", MessageBoxButton.OK, MessageBoxImage.Information);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler beim Exportieren: {ex.Message}",
                    "Export-Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Public method to load custom graph
        /// </summary>
        public async Task LoadGraphAsync(Model.Graph graph)
        {
            _currentGraph = graph;
            await CalculateLayoutAsync();
            await RenderGraphAsync();
        }

        /// <summary>
        /// Public method to highlight specific node
        /// </summary>
        public async Task HighlightNodeAsync(string nodeId)
        {
            if (_webView?.CoreWebView2 != null && _isGraphInitialized)
            {
                try
                {
                    await _webView.CoreWebView2.ExecuteScriptAsync($"window.highlightNode('{nodeId}');");
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error highlighting node: {ex.Message}");
                }
            }
        }
    }
}
