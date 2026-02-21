/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphView3D.xaml.cs                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     277                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Input;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Linq;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Features.Graph.Services;
using Model = Themis.DocumentManager.Models;
using Themis.DocumentManager.Services.DirectX;

namespace Themis.DocumentManager.Features.Graph.Views;

/// <summary>
/// GraphView mit DirectX 11 3D Rendering
/// </summary>
public partial class GraphView3D : UserControl
{
    private readonly IDirectX3DGraphRenderer _renderer;
    private readonly IGraphVisualizationService _graphService;
    
    private Model.Graph? _currentGraph;
    private bool _isInitialized = false;
    
    private Point _lastMousePosition;
    private bool _isMouseDown = false;
    private CancellationTokenSource? _layoutCancellation;

    public GraphView3D()
    {
        InitializeComponent();
        
        _renderer = App.GetService<IDirectX3DGraphRenderer>() 
            ?? throw new InvalidOperationException("IDirectX3DGraphRenderer not registered");
        _graphService = App.GetService<IGraphVisualizationService>() 
            ?? throw new InvalidOperationException("IGraphVisualizationService not registered");

        Loaded += OnLoaded;
        Unloaded += OnUnloaded;
        MouseWheel += OnMouseWheel;
        MouseDown += OnMouseDown;
        MouseUp += OnMouseUp;
        MouseMove += OnMouseMove;
    }

    private async void OnLoaded(object sender, RoutedEventArgs e)
    {
        await InitializeDirectXAsync();
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        _renderer?.Cleanup();
        _layoutCancellation?.Dispose();
    }

    private async Task InitializeDirectXAsync()
    {
        try
        {
            if (_isInitialized)
                return;

            var helper = new WindowInteropHelper(Window.GetWindow(this) 
                ?? throw new InvalidOperationException("Window not found"));
            var hwnd = helper.Handle;

            int width = (int)RenderSurface.ActualWidth;
            int height = (int)RenderSurface.ActualHeight;

            if (width <= 0 || height <= 0)
            {
                await Task.Delay(100);
                width = (int)RenderSurface.ActualWidth;
                height = (int)RenderSurface.ActualHeight;
            }

            _renderer.Initialize(hwnd, width, height);

            _currentGraph = CreateExampleGraph();
            await CalculateLayoutAsync();

            RenderFrame();
            StartRenderLoop();

            _isInitialized = true;
            StatusText.Text = $"DirectX 11 Ready | Nodes: {_currentGraph.Nodes.Count} | Edges: {_currentGraph.Edges.Count}";
        }
        catch (Exception ex)
        {
            MessageBox.Show($"DirectX Error: {ex.Message}", 
                "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private Model.Graph CreateExampleGraph()
    {
        var graph = new Model.Graph
        {
            Id = "example-graph",
            Name = "Example Graph",
            Nodes = new List<GraphNode>(),
            Edges = new List<GraphEdge>()
        };

        for (int i = 0; i < 10; i++)
        {
            graph.Nodes.Add(new GraphNode
            {
                Id = $"node-{i}",
                Label = $"Node {i}",
                Type = i < 2 ? GraphNodeType.Central : GraphNodeType.Standard,
                Position = new Vector3D { X = 0, Y = 0, Z = 0 },
                Velocity = new Vector3D { X = 0, Y = 0, Z = 0 },
                Radius = i < 2 ? 30 : 20,
                Mass = i < 2 ? 2.0 : 1.0,
                Color = i < 2 ? "#2196F3" : "#4CAF50",
                IconShape = "circle"
            });
        }

        for (int hub = 0; hub < 2; hub++)
        {
            for (int i = 0; i < 4; i++)
            {
                int target = 2 + hub * 4 + i;
                if (target < 10)
                {
                    graph.Edges.Add(new GraphEdge
                    {
                        Id = $"edge-{hub}-{target}",
                        SourceNodeId = $"node-{hub}",
                        TargetNodeId = $"node-{target}",
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

        return graph;
    }

    private async Task CalculateLayoutAsync()
    {
        if (_currentGraph == null)
            return;

        try
        {
            _layoutCancellation?.Cancel();
            _layoutCancellation = new CancellationTokenSource();

            var forceParams = new ForceDirectedLayoutParams
            {
                K = 100.0,
                Iterations = 100,
                Use3D = true,
                StepSize = 0.1,
                Cooling = 0.95
            };

            var result = await _graphService.CalculateForceDirectedLayoutAsync(
                _currentGraph, forceParams, _layoutCancellation.Token);

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

            StatusText.Text = $"Layout: {result.ComputationTime.TotalMilliseconds:F1}ms";
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Error: {ex.Message}";
        }
    }

    private void RenderFrame()
    {
        if (!_isInitialized || _currentGraph == null)
            return;

        try
        {
            _renderer.Render(_currentGraph);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Render Error: {ex.Message}");
        }
    }

    private void StartRenderLoop()
    {
        _ = Task.Run(async () =>
        {
            while (_isInitialized)
            {
                RenderFrame();
                await Task.Delay(16);
            }
        });
    }

    private void OnMouseDown(object sender, MouseButtonEventArgs e)
    {
        _isMouseDown = true;
        _lastMousePosition = e.GetPosition(RenderSurface);
    }

    private void OnMouseUp(object sender, MouseButtonEventArgs e)
    {
        _isMouseDown = false;
    }

    private void OnMouseMove(object sender, MouseEventArgs e)
    {
        if (!_isMouseDown)
            return;

        var currentPos = e.GetPosition(RenderSurface);
        var delta = currentPos - _lastMousePosition;

        _renderer.Rotate((float)delta.X, (float)delta.Y);
        _lastMousePosition = currentPos;
    }

    private void OnMouseWheel(object sender, MouseWheelEventArgs e)
    {
        _renderer.Zoom(e.Delta > 0 ? 1.0f : -1.0f);
    }
}
