/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Layer3DManager.cs                                  ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     757                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace RailwayMonitor.WPF.Services.Rendering;

/// <summary>
/// 3D Layer Management System for Railway Monitoring (CAD-Style)
/// Inspired by SolidWorks layer system with:
/// - Hierarchical layer organization
/// - Selective visibility (show/hide)
/// - Layer locking
/// - Color-coded layers
/// - Render order management
/// 
/// Manages visibility and rendering of different object types:
/// - Forests (Wald)
/// - Buildings (Bauwerke) 
/// - Objects (Objekte)
/// - Protected Areas (Schutzgebiete: FFH, Denkmäler, etc.)
/// </summary>
public class Layer3DManager
{
    private readonly Dictionary<LayerType, Layer3D> _layers = new();
    private readonly Dictionary<string, RenderableObject3D> _allObjects = new();
    private readonly FeatureTreeManager? _featureTree;
    
    // CAD-Style display modes
    private DisplayMode _displayMode = DisplayMode.Shaded;
    private bool _showWireframe = false;
    private bool _showDimensions = false;
    private bool _showGrid = true;
    
    public DisplayMode CurrentDisplayMode
    {
        get => _displayMode;
        set => _displayMode = value;
    }
    
    public bool ShowWireframe
    {
        get => _showWireframe;
        set => _showWireframe = value;
    }
    
    public bool ShowDimensions
    {
        get => _showDimensions;
        set => _showDimensions = value;
    }
    
    public bool ShowGrid
    {
        get => _showGrid;
        set => _showGrid = value;
    }
    
    public event EventHandler<LayerEventArgs>? LayerVisibilityChanged;
    public event EventHandler<DisplayModeEventArgs>? DisplayModeChanged;
    
    public Layer3DManager(FeatureTreeManager? featureTree = null)
    {
        _featureTree = featureTree;
        InitializeLayers();
    }

    /// <summary>
    /// Initialize all layer categories
    /// </summary>
    private void InitializeLayers()
    {
        // Terrain and Base Layers
        _layers[LayerType.Terrain] = new Layer3D
        {
            Type = LayerType.Terrain,
            Name = "Gelände",
            IsVisible = true,
            IsLocked = true, // Cannot be hidden
            Category = LayerCategory.Base,
            RenderOrder = 0
        };

        // Forest Layer
        _layers[LayerType.Forest] = new Layer3D
        {
            Type = LayerType.Forest,
            Name = "Wald",
            IsVisible = true,
            Category = LayerCategory.Nature,
            RenderOrder = 10,
            Color = System.Drawing.Color.FromArgb(100, 34, 139, 34) // Semi-transparent green
        };

        // Building Layers
        _layers[LayerType.Buildings_Residential] = new Layer3D
        {
            Type = LayerType.Buildings_Residential,
            Name = "Wohngebäude",
            IsVisible = true,
            Category = LayerCategory.Buildings,
            RenderOrder = 20
        };

        _layers[LayerType.Buildings_Commercial] = new Layer3D
        {
            Type = LayerType.Buildings_Commercial,
            Name = "Gewerbegebäude",
            IsVisible = true,
            Category = LayerCategory.Buildings,
            RenderOrder = 21
        };

        _layers[LayerType.Buildings_Industrial] = new Layer3D
        {
            Type = LayerType.Buildings_Industrial,
            Name = "Industriegebäude",
            IsVisible = true,
            Category = LayerCategory.Buildings,
            RenderOrder = 22
        };

        // Infrastructure
        _layers[LayerType.Railway_Tracks] = new Layer3D
        {
            Type = LayerType.Railway_Tracks,
            Name = "Bahngleise",
            IsVisible = true,
            IsLocked = true,
            Category = LayerCategory.Infrastructure,
            RenderOrder = 30
        };

        _layers[LayerType.Railway_Stations] = new Layer3D
        {
            Type = LayerType.Railway_Stations,
            Name = "Bahnhöfe",
            IsVisible = true,
            Category = LayerCategory.Infrastructure,
            RenderOrder = 31
        };

        _layers[LayerType.Roads] = new Layer3D
        {
            Type = LayerType.Roads,
            Name = "Straßen",
            IsVisible = true,
            Category = LayerCategory.Infrastructure,
            RenderOrder = 32
        };

        _layers[LayerType.Bridges] = new Layer3D
        {
            Type = LayerType.Bridges,
            Name = "Brücken",
            IsVisible = true,
            Category = LayerCategory.Infrastructure,
            RenderOrder = 33
        };

        // Protected Areas (Schutzgebiete)
        _layers[LayerType.Protected_FFH] = new Layer3D
        {
            Type = LayerType.Protected_FFH,
            Name = "FFH-Gebiete (Natura 2000)",
            IsVisible = true,
            Category = LayerCategory.ProtectedAreas,
            RenderOrder = 100,
            RenderAsBox = true,
            Color = System.Drawing.Color.FromArgb(80, 0, 128, 0) // Semi-transparent dark green
        };

        _layers[LayerType.Protected_NatureReserve] = new Layer3D
        {
            Type = LayerType.Protected_NatureReserve,
            Name = "Naturschutzgebiete (NSG)",
            IsVisible = true,
            Category = LayerCategory.ProtectedAreas,
            RenderOrder = 101,
            RenderAsBox = true,
            Color = System.Drawing.Color.FromArgb(80, 0, 255, 0) // Semi-transparent green
        };

        _layers[LayerType.Protected_LandscapeProtection] = new Layer3D
        {
            Type = LayerType.Protected_LandscapeProtection,
            Name = "Landschaftsschutzgebiete (LSG)",
            IsVisible = true,
            Category = LayerCategory.ProtectedAreas,
            RenderOrder = 102,
            RenderAsBox = true,
            Color = System.Drawing.Color.FromArgb(60, 173, 255, 47) // Semi-transparent yellow-green
        };

        _layers[LayerType.Protected_WaterProtection] = new Layer3D
        {
            Type = LayerType.Protected_WaterProtection,
            Name = "Wasserschutzgebiete",
            IsVisible = true,
            Category = LayerCategory.ProtectedAreas,
            RenderOrder = 103,
            RenderAsBox = true,
            Color = System.Drawing.Color.FromArgb(80, 0, 191, 255) // Semi-transparent blue
        };

        _layers[LayerType.Protected_Monuments] = new Layer3D
        {
            Type = LayerType.Protected_Monuments,
            Name = "Denkmalgeschützte Objekte",
            IsVisible = true,
            Category = LayerCategory.ProtectedAreas,
            RenderOrder = 104,
            RenderAsBox = false, // Individual objects, not boxes
            Color = System.Drawing.Color.FromArgb(200, 139, 69, 19) // Brown
        };

        // Virtual/Planning Layers
        _layers[LayerType.Planning_ProposedRoutes] = new Layer3D
        {
            Type = LayerType.Planning_ProposedRoutes,
            Name = "Geplante Strecken",
            IsVisible = true,
            Category = LayerCategory.Planning,
            RenderOrder = 200,
            Color = System.Drawing.Color.FromArgb(150, 255, 0, 0) // Semi-transparent red
        };

        _layers[LayerType.Planning_CostBoxes] = new Layer3D
        {
            Type = LayerType.Planning_CostBoxes,
            Name = "Kostengebiete",
            IsVisible = false,
            Category = LayerCategory.Planning,
            RenderOrder = 201,
            RenderAsBox = true,
            Color = System.Drawing.Color.FromArgb(100, 255, 165, 0) // Semi-transparent orange
        };
    }

    /// <summary>
    /// Add a 3D object to a specific layer
    /// </summary>
    public void AddObject(LayerType layerType, RenderableObject3D obj)
    {
        if (!_layers.ContainsKey(layerType))
            throw new ArgumentException($"Layer type {layerType} not found");

        _layers[layerType].Objects.Add(obj);
        _allObjects[obj.Id] = obj;
        obj.LayerType = layerType;
    }

    /// <summary>
    /// Remove object from its layer
    /// </summary>
    public void RemoveObject(string objectId)
    {
        if (_allObjects.TryGetValue(objectId, out var obj))
        {
            _layers[obj.LayerType].Objects.Remove(obj);
            _allObjects.Remove(objectId);
        }
    }

    /// <summary>
    /// Toggle layer visibility (with event notification)
    /// </summary>
    public void SetLayerVisibility(LayerType layerType, bool visible)
    {
        if (_layers.TryGetValue(layerType, out var layer))
        {
            if (!layer.IsLocked) // Cannot hide locked layers
            {
                var oldValue = layer.IsVisible;
                layer.IsVisible = visible;
                
                if (oldValue != visible)
                {
                    LayerVisibilityChanged?.Invoke(this, new LayerEventArgs 
                    { 
                        LayerType = layerType, 
                        IsVisible = visible 
                    });
                }
            }
        }
    }

    /// <summary>
    /// Toggle entire category visibility
    /// </summary>
    public void SetCategoryVisibility(LayerCategory category, bool visible)
    {
        foreach (var layer in _layers.Values.Where(l => l.Category == category && !l.IsLocked))
        {
            SetLayerVisibility(layer.Type, visible);
        }
    }
    
    /// <summary>
    /// Isolate a single layer (hide all others in same category)
    /// </summary>
    public void IsolateLayer(LayerType layerType)
    {
        if (_layers.TryGetValue(layerType, out var targetLayer))
        {
            foreach (var layer in _layers.Values.Where(l => l.Category == targetLayer.Category && !l.IsLocked))
            {
                layer.IsVisible = (layer.Type == layerType);
            }
        }
    }
    
    /// <summary>
    /// Show all layers
    /// </summary>
    public void ShowAllLayers()
    {
        foreach (var layer in _layers.Values.Where(l => !l.IsLocked))
        {
            layer.IsVisible = true;
        }
    }
    
    /// <summary>
    /// Hide all layers in category
    /// </summary>
    public void HideAllInCategory(LayerCategory category)
    {
        SetCategoryVisibility(category, false);
    }
    
    /// <summary>
    /// Set display mode (Shaded, Wireframe, HiddenLineRemoved, etc.)
    /// </summary>
    public void SetDisplayMode(DisplayMode mode)
    {
        var oldMode = _displayMode;
        _displayMode = mode;
        
        DisplayModeChanged?.Invoke(this, new DisplayModeEventArgs 
        { 
            OldMode = oldMode, 
            NewMode = mode 
        });
    }
    
    /// <summary>
    /// Sync layer with feature tree (if feature tree is available)
    /// </summary>
    public void SyncWithFeatureTree()
    {
        if (_featureTree == null)
            return;
            
        // Get all geometry features and add them to appropriate layers
        var features = _featureTree.RootFeatures;
        
        foreach (var feature in features)
        {
            SyncFeatureToLayer(feature);
        }
    }
    
    private void SyncFeatureToLayer(Feature feature)
    {
        // Map feature types to layer types
        var layerType = feature switch
        {
            RailwayTrackFeature => LayerType.Railway_Tracks,
            BridgeFeature => LayerType.Bridges,
            TunnelFeature => LayerType.Tunnels,
            ProtectedAreaFeature pf => pf.ProtectionType,
            _ => LayerType.Planning_ProposedRoutes
        };
        
        // Create renderable object from feature
        var obj = CreateRenderableFromFeature(feature);
        if (obj != null)
        {
            AddObject(layerType, obj);
        }
        
        // Recursively sync children
        foreach (var child in feature.Children)
        {
            SyncFeatureToLayer(child);
        }
    }
    
    private RenderableObject3D? CreateRenderableFromFeature(Feature feature)
    {
        // Convert feature to renderable object based on type
        return feature switch
        {
            RailwayTrackFeature track => new RenderableObject3D
            {
                Id = feature.Id,
                Name = feature.Name,
                Properties = new Dictionary<string, object>
                {
                    ["NumberOfTracks"] = track.NumberOfTracks,
                    ["TrackGauge"] = track.TrackGauge
                }
            },
            ProtectedAreaFeature pArea => new ProtectedAreaBox
            {
                Id = feature.Id,
                Name = feature.Name,
                Polygon = pArea.Boundary,
                HeightMeters = pArea.HeightMeters,
                CostPenalty = pArea.CostPenalty,
                Description = feature.Name
            },
            _ => null
        };
    }

    /// <summary>
    /// Get all visible objects sorted by render order
    /// </summary>
    public List<RenderableObject3D> GetVisibleObjects()
    {
        return _layers.Values
            .Where(l => l.IsVisible)
            .OrderBy(l => l.RenderOrder)
            .SelectMany(l => l.Objects)
            .ToList();
    }

    /// <summary>
    /// Get visible objects by layer type
    /// </summary>
    public List<RenderableObject3D> GetVisibleObjectsByLayer(LayerType layerType)
    {
        if (_layers.TryGetValue(layerType, out var layer) && layer.IsVisible)
        {
            return layer.Objects.ToList();
        }
        return new List<RenderableObject3D>();
    }

    /// <summary>
    /// Get all layers
    /// </summary>
    public Dictionary<LayerType, Layer3D> GetAllLayers() => _layers;

    /// <summary>
    /// Get layers by category
    /// </summary>
    public List<Layer3D> GetLayersByCategory(LayerCategory category)
    {
        return _layers.Values.Where(l => l.Category == category).ToList();
    }

    /// <summary>
    /// Check if point is inside any protected area
    /// </summary>
    public List<ProtectedAreaInfo> GetProtectedAreasAt(double latitude, double longitude)
    {
        var results = new List<ProtectedAreaInfo>();

        var protectedLayers = _layers.Values
            .Where(l => l.Category == LayerCategory.ProtectedAreas && l.IsVisible)
            .ToList();

        foreach (var layer in protectedLayers)
        {
            foreach (var obj in layer.Objects)
            {
                if (obj is ProtectedAreaBox box && box.ContainsPoint(latitude, longitude))
                {
                    results.Add(new ProtectedAreaInfo
                    {
                        Name = box.Name,
                        Type = layer.Type,
                        Description = box.Description,
                        CostPenalty = box.CostPenalty,
                        BoundingBox = box.BoundingBox
                    });
                }
            }
        }

        return results;
    }

    /// <summary>
    /// Filter objects by bounding box (for viewport culling)
    /// </summary>
    public List<RenderableObject3D> GetObjectsInBoundingBox(BoundingBox3D bbox)
    {
        return GetVisibleObjects()
            .Where(obj => bbox.Intersects(obj.BoundingBox))
            .ToList();
    }
}

// ============================================================================
// Enums and Data Classes
// ============================================================================

public enum LayerType
{
    // Base
    Terrain,
    
    // Nature
    Forest,
    Water,
    
    // Buildings
    Buildings_Residential,
    Buildings_Commercial,
    Buildings_Industrial,
    Buildings_Public,
    
    // Infrastructure
    Railway_Tracks,
    Railway_Stations,
    Railway_Signals,
    Roads,
    Bridges,
    Tunnels,
    
    // Protected Areas
    Protected_FFH,              // Fauna-Flora-Habitat (Natura 2000)
    Protected_NatureReserve,    // Naturschutzgebiete (NSG)
    Protected_LandscapeProtection, // Landschaftsschutzgebiete (LSG)
    Protected_WaterProtection,  // Wasserschutzgebiete
    Protected_Monuments,        // Denkmalschutz
    
    // Planning
    Planning_ProposedRoutes,
    Planning_CostBoxes,
    Planning_ConstructionZones
}

public enum LayerCategory
{
    Base,
    Nature,
    Buildings,
    Infrastructure,
    ProtectedAreas,
    Planning
}

public class Layer3D
{
    public LayerType Type { get; set; }
    public string Name { get; set; } = "";
    public bool IsVisible { get; set; }
    public bool IsLocked { get; set; } // Cannot be hidden
    public LayerCategory Category { get; set; }
    public int RenderOrder { get; set; }
    public bool RenderAsBox { get; set; } // For protected areas
    public System.Drawing.Color Color { get; set; } = System.Drawing.Color.White;
    public ObservableCollection<RenderableObject3D> Objects { get; set; } = new();
    
    public string GetDescription() => Category switch
    {
        LayerCategory.Base => "Grundlegende Geländedarstellung",
        LayerCategory.Nature => "Natürliche Objekte (Wald, Wasser)",
        LayerCategory.Buildings => "Gebäude und Bauwerke",
        LayerCategory.Infrastructure => "Verkehrsinfrastruktur",
        LayerCategory.ProtectedAreas => "Schutzgebiete und Denkmäler",
        LayerCategory.Planning => "Planungs- und Analyseebenen",
        _ => ""
    };
}

/// <summary>
/// Base class for all 3D renderable objects
/// </summary>
public class RenderableObject3D
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = "";
    public LayerType LayerType { get; set; }
    public BoundingBox3D BoundingBox { get; set; } = new();
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double Elevation { get; set; }
    public Dictionary<string, object> Properties { get; set; } = new();
}

/// <summary>
/// Protected area represented as a 3D bounding box
/// </summary>
public class ProtectedAreaBox : RenderableObject3D
{
    public string Description { get; set; } = "";
    public decimal CostPenalty { get; set; } // Additional cost in EUR
    public List<GeoCoordinate> Polygon { get; set; } = new(); // Area boundary
    public double HeightMeters { get; set; } = 100; // Box height for visualization
    
    public bool ContainsPoint(double latitude, double longitude)
    {
        // Simple point-in-polygon test (can be optimized)
        if (Polygon.Count < 3) return false;
        
        bool inside = false;
        int j = Polygon.Count - 1;
        
        for (int i = 0; i < Polygon.Count; i++)
        {
            if ((Polygon[i].Longitude > longitude) != (Polygon[j].Longitude > longitude) &&
                latitude < (Polygon[j].Latitude - Polygon[i].Latitude) * 
                (longitude - Polygon[i].Longitude) / 
                (Polygon[j].Longitude - Polygon[i].Longitude) + Polygon[i].Latitude)
            {
                inside = !inside;
            }
            j = i;
        }
        
        return inside;
    }
}

/// <summary>
/// Forest area
/// </summary>
public class ForestObject : RenderableObject3D
{
    public double AreaSquareKm { get; set; }
    public string ForestType { get; set; } = "Mixed"; // Deciduous, Coniferous, Mixed
    public List<GeoCoordinate> Polygon { get; set; } = new();
}

/// <summary>
/// Building object
/// </summary>
public class BuildingObject : RenderableObject3D
{
    public double HeightMeters { get; set; }
    public string BuildingType { get; set; } = ""; // Residential, Commercial, etc.
    public int Floors { get; set; }
    public List<GeoCoordinate> FootprintPolygon { get; set; } = new();
}

/// <summary>
/// 3D Bounding Box
/// </summary>
public class BoundingBox3D
{
    public double MinLatitude { get; set; }
    public double MaxLatitude { get; set; }
    public double MinLongitude { get; set; }
    public double MaxLongitude { get; set; }
    public double MinElevation { get; set; }
    public double MaxElevation { get; set; }
    
    public bool Intersects(BoundingBox3D other)
    {
        return !(MaxLatitude < other.MinLatitude ||
                 MinLatitude > other.MaxLatitude ||
                 MaxLongitude < other.MinLongitude ||
                 MinLongitude > other.MaxLongitude ||
                 MaxElevation < other.MinElevation ||
                 MinElevation > other.MaxElevation);
    }
    
    public bool Contains(double latitude, double longitude, double elevation = 0)
    {
        return latitude >= MinLatitude && latitude <= MaxLatitude &&
               longitude >= MinLongitude && longitude <= MaxLongitude &&
               elevation >= MinElevation && elevation <= MaxElevation;
    }
}

public class GeoCoordinate
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double Elevation { get; set; }
}

public class ProtectedAreaInfo
{
    public string Name { get; set; } = "";
    public LayerType Type { get; set; }
    public string Description { get; set; } = "";
    public decimal CostPenalty { get; set; }
    public BoundingBox3D BoundingBox { get; set; } = new();
    
    public string GetTypeName() => Type switch
    {
        LayerType.Protected_FFH => "FFH-Gebiet (Natura 2000)",
        LayerType.Protected_NatureReserve => "Naturschutzgebiet",
        LayerType.Protected_LandscapeProtection => "Landschaftsschutzgebiet",
        LayerType.Protected_WaterProtection => "Wasserschutzgebiet",
        LayerType.Protected_Monuments => "Denkmalgeschütztes Objekt",
        _ => "Schutzgebiet"
    };
}

// ============================================================================
// CAD-Style Display Modes and Events
// ============================================================================

/// <summary>
/// Display modes (like SolidWorks view modes)
/// </summary>
public enum DisplayMode
{
    Wireframe,              // Show only edges
    HiddenLineRemoved,      // Wireframe with hidden lines removed
    HiddenLineVisible,      // Wireframe with hidden lines gray
    Shaded,                 // Solid shading
    ShadedWithEdges,        // Shaded + black edges
    RealView,               // Realistic rendering with materials
    Shadows,                // Shaded with shadows
    Ambient,                // Ambient occlusion
    Technical,              // Technical illustration style
    Xray                    // Semi-transparent view
}

public class LayerEventArgs : EventArgs
{
    public LayerType LayerType { get; set; }
    public bool IsVisible { get; set; }
}

public class DisplayModeEventArgs : EventArgs
{
    public DisplayMode OldMode { get; set; }
    public DisplayMode NewMode { get; set; }
}

