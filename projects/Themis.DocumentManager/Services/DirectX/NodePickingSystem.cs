/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NodePickingSystem.cs                               ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     456                                            ║
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
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// 3D Node Picking via Ray-Casting für Selection & Interaction
/// </summary>
public class NodePickingSystem
{
    private Camera3D _camera;
    private List<RayHit> _rayHitCache = new();

    public NodePickingSystem(Camera3D camera)
    {
        _camera = camera;
    }

    /// <summary>
    /// Generate Ray from Screen Coordinates
    /// </summary>
    public Ray GenerateRayFromScreenCoords(int screenX, int screenY, int screenWidth, int screenHeight)
    {
        // Normalize screen coordinates to [-1, 1]
        float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * screenY) / screenHeight;

        // Create ray direction in camera space
        float fov = 45.0f; // Default FOV
        float aspect = screenWidth / (float)screenHeight;
        float tanHalfFov = (float)Math.Tan(fov * Math.PI / 360.0f);

        float rayDirX = ndcX * tanHalfFov * aspect;
        float rayDirY = ndcY * tanHalfFov;
        float rayDirZ = -1.0f;

        // Normalize direction
        float length = (float)Math.Sqrt(rayDirX * rayDirX + rayDirY * rayDirY + rayDirZ * rayDirZ);
        rayDirX /= length;
        rayDirY /= length;
        rayDirZ /= length;

        return new Ray
        {
            OriginX = _camera.EyeX,
            OriginY = _camera.EyeY,
            OriginZ = _camera.EyeZ,
            DirectionX = rayDirX,
            DirectionY = rayDirY,
            DirectionZ = rayDirZ
        };
    }

    /// <summary>
    /// Test Ray-Sphere Intersection
    /// </summary>
    public bool TestRaySphereIntersection(Ray ray, Sphere sphere, out float distance)
    {
        distance = float.MaxValue;

        // Vector from ray origin to sphere center
        float ocX = sphere.CenterX - ray.OriginX;
        float ocY = sphere.CenterY - ray.OriginY;
        float ocZ = sphere.CenterZ - ray.OriginZ;

        // Coefficients for quadratic equation
        float a = ray.DirectionX * ray.DirectionX +
                  ray.DirectionY * ray.DirectionY +
                  ray.DirectionZ * ray.DirectionZ;

        float b = -2.0f * (ray.DirectionX * ocX +
                           ray.DirectionY * ocY +
                           ray.DirectionZ * ocZ);

        float c = ocX * ocX + ocY * ocY + ocZ * ocZ - (sphere.Radius * sphere.Radius);

        // Discriminant
        float discriminant = b * b - 4.0f * a * c;

        if (discriminant < 0)
            return false;  // No intersection

        float sqrtDiscriminant = (float)Math.Sqrt(discriminant);
        float t0 = (-b - sqrtDiscriminant) / (2.0f * a);
        float t1 = (-b + sqrtDiscriminant) / (2.0f * a);

        // Find nearest intersection in front of camera
        if (t0 > 0.001f)
        {
            distance = t0;
            return true;
        }
        if (t1 > 0.001f)
        {
            distance = t1;
            return true;
        }

        return false;
    }

    /// <summary>
    /// Pick Nodes from Graph via Ray-Casting
    /// </summary>
    public List<PickedNode> PickNodes(Graph graph, Ray ray, float maxDistance = 1000.0f)
    {
        _rayHitCache.Clear();
        var pickedNodes = new List<PickedNode>();

        foreach (var node in graph.Nodes)
        {
            var sphere = new Sphere
            {
                CenterX = (float)node.Position.X,
                CenterY = (float)node.Position.Y,
                CenterZ = (float)node.Position.Z,
                Radius = (float)node.Radius / 100.0f
            };

            if (TestRaySphereIntersection(ray, sphere, out float distance))
            {
                if (distance <= maxDistance)
                {
                    var pickedNode = new PickedNode
                    {
                        NodeId = node.Id,
                        NodeLabel = node.Label,
                        Distance = distance,
                        HitPoint = new Vector3D
                        {
                            X = ray.OriginX + ray.DirectionX * distance,
                            Y = ray.OriginY + ray.DirectionY * distance,
                            Z = ray.OriginZ + ray.DirectionZ * distance
                        }
                    };

                    pickedNodes.Add(pickedNode);
                    _rayHitCache.Add(new RayHit { NodeId = node.Id, Distance = distance });
                }
            }
        }

        // Sort by distance (nearest first)
        return pickedNodes.OrderBy(p => p.Distance).ToList();
    }

    /// <summary>
    /// Get Nearest Picked Node
    /// </summary>
    public PickedNode? GetNearestPickedNode(Graph graph, Ray ray)
    {
        var picked = PickNodes(graph, ray);
        return picked.Count > 0 ? picked[0] : null;
    }

    /// <summary>
    /// Get Picking Statistics
    /// </summary>
    public PickingStatistics GetStatistics()
    {
        return new PickingStatistics
        {
            LastRayHitCount = _rayHitCache.Count,
            AverageHitDistance = _rayHitCache.Count > 0 
                ? _rayHitCache.Average(h => h.Distance)
                : 0,
            NearestHitDistance = _rayHitCache.Count > 0
                ? _rayHitCache.Min(h => h.Distance)
                : float.MaxValue
        };
    }
}

/// <summary>
/// 3D Ray für Ray-Casting
/// </summary>
public struct Ray
{
    public float OriginX { get; set; }
    public float OriginY { get; set; }
    public float OriginZ { get; set; }
    public float DirectionX { get; set; }
    public float DirectionY { get; set; }
    public float DirectionZ { get; set; }

    public override string ToString()
    {
        return $"Ray(Origin=({OriginX:F2},{OriginY:F2},{OriginZ:F2}), " +
               $"Dir=({DirectionX:F2},{DirectionY:F2},{DirectionZ:F2}))";
    }
}

/// <summary>
/// Sphere für Intersection Testing
/// </summary>
public struct Sphere
{
    public float CenterX { get; set; }
    public float CenterY { get; set; }
    public float CenterZ { get; set; }
    public float Radius { get; set; }
}

/// <summary>
/// Picked Node Information
/// </summary>
public class PickedNode
{
    public string NodeId { get; set; } = "";
    public string NodeLabel { get; set; } = "";
    public float Distance { get; set; }
    public Vector3D HitPoint { get; set; } = new Vector3D();

    public override string ToString()
    {
        return $"{NodeLabel} (ID: {NodeId}, Distance: {Distance:F2})";
    }
}

/// <summary>
/// Ray Hit Cache Entry
/// </summary>
public struct RayHit
{
    public string NodeId { get; set; }
    public float Distance { get; set; }
}

/// <summary>
/// Picking Statistics
/// </summary>
public class PickingStatistics
{
    public int LastRayHitCount { get; set; }
    public float AverageHitDistance { get; set; }
    public float NearestHitDistance { get; set; }

    public override string ToString()
    {
        return $"Picking: Hits={LastRayHitCount} | Avg Distance={AverageHitDistance:F2} | " +
               $"Nearest={NearestHitDistance:F2}";
    }
}

/// <summary>
/// Node Selection Manager mit History
/// </summary>
public class NodeSelectionManager
{
    private Stack<string> _selectionHistory = new();
    private HashSet<string> _selectedNodeIds = new();
    private string? _primarySelection = null;

    public event Action<string>? OnNodeSelected;
    public event Action<string>? OnNodeDeselected;
    public event Action? OnSelectionCleared;

    /// <summary>
    /// Select Single Node (Deselects Others)
    /// </summary>
    public void SelectNode(string nodeId)
    {
        // Deselect previous
        if (_primarySelection != null)
        {
            _selectedNodeIds.Remove(_primarySelection);
            OnNodeDeselected?.Invoke(_primarySelection);
        }

        // Select new
        _primarySelection = nodeId;
        _selectedNodeIds.Add(nodeId);
        _selectionHistory.Push(nodeId);

        OnNodeSelected?.Invoke(nodeId);
        System.Diagnostics.Debug.WriteLine($"Selected node: {nodeId}");
    }

    /// <summary>
    /// Toggle Node Selection (Multi-Select)
    /// </summary>
    public void ToggleNode(string nodeId)
    {
        if (_selectedNodeIds.Contains(nodeId))
        {
            _selectedNodeIds.Remove(nodeId);
            OnNodeDeselected?.Invoke(nodeId);
            System.Diagnostics.Debug.WriteLine($"Deselected node: {nodeId}");
        }
        else
        {
            _selectedNodeIds.Add(nodeId);
            _primarySelection = nodeId;
            _selectionHistory.Push(nodeId);
            OnNodeSelected?.Invoke(nodeId);
            System.Diagnostics.Debug.WriteLine($"Selected node: {nodeId}");
        }
    }

    /// <summary>
    /// Get All Selected Nodes
    /// </summary>
    public IReadOnlySet<string> GetSelectedNodes()
    {
        return _selectedNodeIds;
    }

    /// <summary>
    /// Get Primary (Last) Selected Node
    /// </summary>
    public string? GetPrimarySelection()
    {
        return _primarySelection;
    }

    /// <summary>
    /// Clear All Selections
    /// </summary>
    public void ClearSelection()
    {
        _selectedNodeIds.Clear();
        _selectionHistory.Clear();
        _primarySelection = null;
        OnSelectionCleared?.Invoke();
        System.Diagnostics.Debug.WriteLine("Selection cleared");
    }

    /// <summary>
    /// Undo Selection (Back to Previous)
    /// </summary>
    public void UndoSelection()
    {
        if (_selectionHistory.Count > 0)
        {
            _selectionHistory.Pop();
            if (_selectionHistory.Count > 0)
            {
                _primarySelection = _selectionHistory.Peek();
            }
        }
    }

    /// <summary>
    /// Get Selection History
    /// </summary>
    public List<string> GetSelectionHistory()
    {
        return _selectionHistory.Reverse().ToList();
    }

    /// <summary>
    /// Check if Node is Selected
    /// </summary>
    public bool IsNodeSelected(string nodeId)
    {
        return _selectedNodeIds.Contains(nodeId);
    }
}

/// <summary>
/// Selection Highlight Renderer für UI Feedback
/// </summary>
public class SelectionHighlightRenderer
{
    private NodeSelectionManager _selectionManager;
    private Dictionary<string, SelectionHighlight> _highlights = new();

    public SelectionHighlightRenderer(NodeSelectionManager selectionManager)
    {
        _selectionManager = selectionManager;
        _selectionManager.OnNodeSelected += node => UpdateHighlight(node, true);
        _selectionManager.OnNodeDeselected += node => UpdateHighlight(node, false);
        _selectionManager.OnSelectionCleared += ClearAllHighlights;
    }

    /// <summary>
    /// Update Node Highlight
    /// </summary>
    private void UpdateHighlight(string nodeId, bool isSelected)
    {
        if (!_highlights.ContainsKey(nodeId))
        {
            _highlights[nodeId] = new SelectionHighlight
            {
                NodeId = nodeId,
                OutlineColor = "#FFFF00",  // Yellow
                OutlineWidth = 2.0f,
                IsSelected = false
            };
        }

        _highlights[nodeId].IsSelected = isSelected;
        System.Diagnostics.Debug.WriteLine(
            $"Highlight {nodeId}: {(isSelected ? "ON" : "OFF")}");
    }

    /// <summary>
    /// Clear All Highlights
    /// </summary>
    private void ClearAllHighlights()
    {
        foreach (var highlight in _highlights.Values)
        {
            highlight.IsSelected = false;
        }
        System.Diagnostics.Debug.WriteLine("All highlights cleared");
    }

    /// <summary>
    /// Get Highlight for Node
    /// </summary>
    public SelectionHighlight? GetHighlight(string nodeId)
    {
        return _highlights.ContainsKey(nodeId) ? _highlights[nodeId] : null;
    }
}

/// <summary>
/// Selection Highlight Information
/// </summary>
public class SelectionHighlight
{
    public string NodeId { get; set; } = "";
    public string OutlineColor { get; set; } = "#FFFF00";
    public float OutlineWidth { get; set; } = 2.0f;
    public bool IsSelected { get; set; }
    public DateTime SelectedAt { get; set; } = DateTime.UtcNow;
}
