/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FeatureTreeManager.cs                              ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     744                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 61a060e76  2025-12-15  Add CAD-style 3D Layer System with Feature Tree and Desig... ║
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
/// CAD-Style Feature Tree Manager (inspired by SolidWorks)
/// Hierarchical feature-based modeling system for railway infrastructure
/// </summary>
public class FeatureTreeManager
{
    private readonly ObservableCollection<Feature> _rootFeatures = new();
    private readonly Dictionary<string, Feature> _featureById = new();
    private readonly List<DesignConfiguration> _configurations = new();
    private string _activeConfiguration = "Default";
    
    public ObservableCollection<Feature> RootFeatures => _rootFeatures;
    public event EventHandler<FeatureEventArgs>? FeatureAdded;
    public event EventHandler<FeatureEventArgs>? FeatureModified;
    public event EventHandler<FeatureEventArgs>? FeatureDeleted;
    public event EventHandler? TreeRebuilt;

    public FeatureTreeManager()
    {
        // Create default configuration
        _configurations.Add(new DesignConfiguration 
        { 
            Name = "Default",
            Description = "Standard-Konfiguration"
        });
    }

    /// <summary>
    /// Add a feature to the tree
    /// </summary>
    public void AddFeature(Feature feature, Feature? parent = null)
    {
        feature.Id = Guid.NewGuid().ToString();
        feature.CreatedDate = DateTime.Now;
        feature.ModifiedDate = DateTime.Now;
        
        if (parent == null)
        {
            _rootFeatures.Add(feature);
        }
        else
        {
            parent.Children.Add(feature);
            feature.Parent = parent;
        }
        
        _featureById[feature.Id] = feature;
        FeatureAdded?.Invoke(this, new FeatureEventArgs { Feature = feature });
        
        // Rebuild if feature affects geometry
        if (feature.AffectsGeometry)
        {
            RebuildFromFeature(feature);
        }
    }

    /// <summary>
    /// Suppress/Unsuppress feature (like SolidWorks)
    /// </summary>
    public void SetFeatureSuppressed(string featureId, bool suppressed)
    {
        if (_featureById.TryGetValue(featureId, out var feature))
        {
            feature.IsSuppressed = suppressed;
            feature.ModifiedDate = DateTime.Now;
            
            // Cascade to children
            foreach (var child in GetAllChildren(feature))
            {
                child.IsSuppressed = suppressed;
            }
            
            FeatureModified?.Invoke(this, new FeatureEventArgs { Feature = feature });
            
            if (feature.AffectsGeometry)
            {
                RebuildFromFeature(feature);
            }
        }
    }

    /// <summary>
    /// Edit feature (triggers rebuild from this feature onward)
    /// </summary>
    public void EditFeature(string featureId, Action<Feature> editAction)
    {
        if (_featureById.TryGetValue(featureId, out var feature))
        {
            editAction(feature);
            feature.ModifiedDate = DateTime.Now;
            
            FeatureModified?.Invoke(this, new FeatureEventArgs { Feature = feature });
            
            if (feature.AffectsGeometry)
            {
                RebuildFromFeature(feature);
            }
        }
    }

    /// <summary>
    /// Delete feature and all children
    /// </summary>
    public void DeleteFeature(string featureId)
    {
        if (_featureById.TryGetValue(featureId, out var feature))
        {
            // Remove from parent or root
            if (feature.Parent != null)
            {
                feature.Parent.Children.Remove(feature);
            }
            else
            {
                _rootFeatures.Remove(feature);
            }
            
            // Remove from index
            _featureById.Remove(featureId);
            
            // Recursively delete children
            foreach (var child in feature.Children.ToList())
            {
                DeleteFeature(child.Id);
            }
            
            FeatureDeleted?.Invoke(this, new FeatureEventArgs { Feature = feature });
            RebuildFromFeature(feature.Parent);
        }
    }

    /// <summary>
    /// Rebuild geometry from a specific feature onward (like SolidWorks rebuild)
    /// </summary>
    private void RebuildFromFeature(Feature? startFeature)
    {
        // Collect all features that need rebuilding
        var featuresToRebuild = new List<Feature>();
        
        if (startFeature == null)
        {
            // Full rebuild
            featuresToRebuild = GetAllFeaturesInOrder();
        }
        else
        {
            // Partial rebuild from this feature
            featuresToRebuild = GetFeaturesAfter(startFeature);
        }
        
        foreach (var feature in featuresToRebuild)
        {
            if (!feature.IsSuppressed && feature.AffectsGeometry)
            {
                feature.State = FeatureState.Rebuilding;
                
                try
                {
                    // Execute feature's build logic
                    feature.ExecuteBuild();
                    feature.State = FeatureState.Built;
                }
                catch (Exception ex)
                {
                    feature.State = FeatureState.Error;
                    feature.ErrorMessage = ex.Message;
                }
            }
        }
        
        TreeRebuilt?.Invoke(this, EventArgs.Empty);
    }

    /// <summary>
    /// Get all features in evaluation order (depth-first)
    /// </summary>
    private List<Feature> GetAllFeaturesInOrder()
    {
        var result = new List<Feature>();
        
        foreach (var root in _rootFeatures)
        {
            AddFeatureAndChildren(root, result);
        }
        
        return result;
    }

    private void AddFeatureAndChildren(Feature feature, List<Feature> list)
    {
        list.Add(feature);
        foreach (var child in feature.Children)
        {
            AddFeatureAndChildren(child, list);
        }
    }

    /// <summary>
    /// Get all features that come after a given feature in the tree
    /// </summary>
    private List<Feature> GetFeaturesAfter(Feature startFeature)
    {
        var allFeatures = GetAllFeaturesInOrder();
        var index = allFeatures.IndexOf(startFeature);
        
        return index >= 0 ? allFeatures.Skip(index).ToList() : new List<Feature>();
    }

    /// <summary>
    /// Get all children recursively
    /// </summary>
    private List<Feature> GetAllChildren(Feature feature)
    {
        var result = new List<Feature>();
        
        foreach (var child in feature.Children)
        {
            result.Add(child);
            result.AddRange(GetAllChildren(child));
        }
        
        return result;
    }

    /// <summary>
    /// Roll back to a specific feature (suppress all features after it)
    /// </summary>
    public void RollbackToFeature(string featureId)
    {
        if (_featureById.TryGetValue(featureId, out var targetFeature))
        {
            var allFeatures = GetAllFeaturesInOrder();
            var index = allFeatures.IndexOf(targetFeature);
            
            // Suppress all features after target
            for (int i = index + 1; i < allFeatures.Count; i++)
            {
                allFeatures[i].IsSuppressed = true;
            }
            
            RebuildFromFeature(targetFeature);
        }
    }

    /// <summary>
    /// Create a new configuration (like SolidWorks configurations)
    /// </summary>
    public void CreateConfiguration(string name, string description = "")
    {
        var config = new DesignConfiguration
        {
            Name = name,
            Description = description
        };
        
        // Copy current suppression states
        foreach (var feature in GetAllFeaturesInOrder())
        {
            config.FeatureStates[feature.Id] = new FeatureConfigState
            {
                IsSuppressed = feature.IsSuppressed,
                IsVisible = feature.IsVisible
            };
        }
        
        _configurations.Add(config);
    }

    /// <summary>
    /// Switch to a different configuration
    /// </summary>
    public void ActivateConfiguration(string configName)
    {
        var config = _configurations.FirstOrDefault(c => c.Name == configName);
        if (config != null)
        {
            _activeConfiguration = configName;
            
            // Apply configuration states
            foreach (var kvp in config.FeatureStates)
            {
                if (_featureById.TryGetValue(kvp.Key, out var feature))
                {
                    feature.IsSuppressed = kvp.Value.IsSuppressed;
                    feature.IsVisible = kvp.Value.IsVisible;
                }
            }
            
            RebuildFromFeature(null); // Full rebuild
        }
    }

    /// <summary>
    /// Get feature statistics
    /// </summary>
    public FeatureTreeStatistics GetStatistics()
    {
        var allFeatures = GetAllFeaturesInOrder();
        
        return new FeatureTreeStatistics
        {
            TotalFeatures = allFeatures.Count,
            ActiveFeatures = allFeatures.Count(f => !f.IsSuppressed),
            SuppressedFeatures = allFeatures.Count(f => f.IsSuppressed),
            ErrorFeatures = allFeatures.Count(f => f.State == FeatureState.Error),
            SketchFeatures = allFeatures.Count(f => f.Type == FeatureType.Sketch),
            GeometryFeatures = allFeatures.Count(f => f.Type == FeatureType.Geometry),
            ConstraintFeatures = allFeatures.Count(f => f.Type == FeatureType.Constraint)
        };
    }

    /// <summary>
    /// Find features by type
    /// </summary>
    public List<Feature> FindFeaturesByType(FeatureType type)
    {
        return GetAllFeaturesInOrder().Where(f => f.Type == type).ToList();
    }

    /// <summary>
    /// Find features by name pattern
    /// </summary>
    public List<Feature> FindFeaturesByName(string pattern)
    {
        return GetAllFeaturesInOrder()
            .Where(f => f.Name.Contains(pattern, StringComparison.OrdinalIgnoreCase))
            .ToList();
    }
}

// ============================================================================
// Feature Classes (CAD-Style)
// ============================================================================

/// <summary>
/// Base Feature class (like SolidWorks Feature)
/// </summary>
public class Feature
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public FeatureType Type { get; set; }
    public FeatureState State { get; set; } = FeatureState.NotBuilt;
    public bool IsSuppressed { get; set; }
    public bool IsVisible { get; set; } = true;
    public bool AffectsGeometry { get; set; } = true;
    public DateTime CreatedDate { get; set; }
    public DateTime ModifiedDate { get; set; }
    public string ErrorMessage { get; set; } = "";
    
    public Feature? Parent { get; set; }
    public ObservableCollection<Feature> Children { get; set; } = new();
    
    // Feature parameters (like SolidWorks dimensions)
    public Dictionary<string, FeatureParameter> Parameters { get; set; } = new();
    
    // Constraints
    public List<Constraint> Constraints { get; set; } = new();
    
    /// <summary>
    /// Execute the feature's build logic
    /// </summary>
    public virtual void ExecuteBuild()
    {
        // Override in derived classes
    }
    
    /// <summary>
    /// Get icon for feature tree display
    /// </summary>
    public virtual string GetIcon() => Type switch
    {
        FeatureType.Sketch => "✏️",
        FeatureType.Geometry => "📦",
        FeatureType.Assembly => "🔧",
        FeatureType.Constraint => "🔗",
        FeatureType.Pattern => "⊞",
        FeatureType.Reference => "📐",
        _ => "•"
    };
}

/// <summary>
/// Sketch Feature (2D drawing that becomes basis for 3D geometry)
/// </summary>
public class SketchFeature : Feature
{
    public SketchFeature()
    {
        Type = FeatureType.Sketch;
        Name = "Sketch";
    }
    
    public SketchPlane Plane { get; set; } = new();
    public List<SketchEntity> Entities { get; set; } = new();
    public List<SketchConstraint> SketchConstraints { get; set; } = new();
    
    public override void ExecuteBuild()
    {
        // Solve sketch constraints
        foreach (var constraint in SketchConstraints)
        {
            constraint.Apply();
        }
    }
}

/// <summary>
/// Railway Track Feature
/// </summary>
public class RailwayTrackFeature : Feature
{
    public RailwayTrackFeature()
    {
        Type = FeatureType.Geometry;
        Name = "Bahnstrecke";
        
        Parameters["Length"] = new FeatureParameter { Name = "Länge", Value = 10.0, Unit = "km" };
        Parameters["Width"] = new FeatureParameter { Name = "Breite", Value = 3.0, Unit = "m" };
        Parameters["NumberOfTracks"] = new FeatureParameter { Name = "Gleisanzahl", Value = 2.0, Unit = "" };
    }
    
    public List<GeoCoordinate> Centerline { get; set; } = new();
    public int NumberOfTracks { get; set; } = 2;
    public double TrackGauge { get; set; } = 1.435; // meters (Standard gauge)
    
    public override void ExecuteBuild()
    {
        // Generate track geometry from centerline
        // Apply banking/cant in curves
        // Add ballast, sleepers, rails
    }
}

/// <summary>
/// Tunnel Feature
/// </summary>
public class TunnelFeature : Feature
{
    public TunnelFeature()
    {
        Type = FeatureType.Geometry;
        Name = "Tunnel";
        
        Parameters["Length"] = new FeatureParameter { Name = "Länge", Value = 1.0, Unit = "km" };
        Parameters["Diameter"] = new FeatureParameter { Name = "Durchmesser", Value = 10.0, Unit = "m" };
        Parameters["Depth"] = new FeatureParameter { Name = "Tiefe", Value = 50.0, Unit = "m" };
    }
    
    public GeoCoordinate StartPoint { get; set; } = new();
    public GeoCoordinate EndPoint { get; set; } = new();
    public TunnelType TunnelType { get; set; } = TunnelType.Bored;
    public double CrossSectionArea { get; set; } = 78.5; // m²
    
    public override void ExecuteBuild()
    {
        // Generate tunnel tube geometry
        // Add portal structures
        // Add ventilation shafts if needed
    }
}

/// <summary>
/// Bridge Feature
/// </summary>
public class BridgeFeature : Feature
{
    public BridgeFeature()
    {
        Type = FeatureType.Geometry;
        Name = "Brücke";
        
        Parameters["Length"] = new FeatureParameter { Name = "Länge", Value = 100.0, Unit = "m" };
        Parameters["Height"] = new FeatureParameter { Name = "Höhe", Value = 20.0, Unit = "m" };
        Parameters["Spans"] = new FeatureParameter { Name = "Felder", Value = 3.0, Unit = "" };
    }
    
    public BridgeType BridgeType { get; set; } = BridgeType.Beam;
    public List<GeoCoordinate> Piers { get; set; } = new();
    public double DeckWidth { get; set; } = 12.0; // meters
    
    public override void ExecuteBuild()
    {
        // Generate bridge deck, piers, abutments
        // Add railings, cable stays (if cable-stayed)
    }
}

/// <summary>
/// Protected Area Feature (virtual box)
/// </summary>
public class ProtectedAreaFeature : Feature
{
    public ProtectedAreaFeature()
    {
        Type = FeatureType.Reference;
        Name = "Schutzgebiet";
        AffectsGeometry = false; // Only visual/reference
    }
    
    public LayerType ProtectionType { get; set; }
    public List<GeoCoordinate> Boundary { get; set; } = new();
    public double HeightMeters { get; set; } = 100.0;
    public decimal CostPenalty { get; set; }
    
    public override void ExecuteBuild()
    {
        // Generate bounding box visualization
    }
}

/// <summary>
/// Pattern Feature (like Linear/Circular Pattern in SolidWorks)
/// </summary>
public class PatternFeature : Feature
{
    public PatternFeature()
    {
        Type = FeatureType.Pattern;
        Name = "Muster";
    }
    
    public Feature? SourceFeature { get; set; }
    public PatternType PatternType { get; set; }
    public int Count { get; set; } = 5;
    public double Spacing { get; set; } = 10.0; // meters or degrees
    
    public override void ExecuteBuild()
    {
        // Replicate source feature according to pattern
    }
}

/// <summary>
/// Assembly Feature (grouping of features)
/// </summary>
public class AssemblyFeature : Feature
{
    public AssemblyFeature()
    {
        Type = FeatureType.Assembly;
        Name = "Baugruppe";
        AffectsGeometry = false;
    }
    
    public List<AssemblyMate> Mates { get; set; } = new();
}

// ============================================================================
// Supporting Classes
// ============================================================================

public enum FeatureType
{
    Sketch,          // 2D sketch
    Geometry,        // 3D geometry (extrude, revolve, etc.)
    Assembly,        // Group of features
    Constraint,      // Dimensional or geometric constraint
    Pattern,         // Linear/circular pattern
    Reference,       // Reference geometry (planes, axes, points)
    Annotation       // Notes, dimensions for display only
}

public enum FeatureState
{
    NotBuilt,
    Rebuilding,
    Built,
    Error,
    Suppressed
}

public enum TunnelType
{
    Bored,           // TBM tunnel
    CutAndCover,     // Cut-and-cover tunnel
    Immersed         // Immersed tube tunnel
}

public enum BridgeType
{
    Beam,            // Beam bridge
    Arch,            // Arch bridge
    Suspension,      // Suspension bridge
    CableStayed,     // Cable-stayed bridge
    Truss            // Truss bridge
}

public enum PatternType
{
    Linear,          // Linear pattern (e.g., telegraph poles)
    Circular,        // Circular pattern
    Path             // Pattern along a path
}

public class FeatureParameter
{
    public string Name { get; set; } = "";
    public double Value { get; set; }
    public string Unit { get; set; } = "";
    public double MinValue { get; set; } = double.MinValue;
    public double MaxValue { get; set; } = double.MaxValue;
    public bool IsEditable { get; set; } = true;
}

public class Constraint
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public ConstraintType Type { get; set; }
    public List<string> TargetFeatureIds { get; set; } = new();
    public Dictionary<string, double> Parameters { get; set; } = new();
}

public enum ConstraintType
{
    Coincident,      // Two points coincide
    Parallel,        // Two lines parallel
    Perpendicular,   // Two lines perpendicular
    Tangent,         // Curve tangent to line
    Distance,        // Fixed distance
    Angle,           // Fixed angle
    Symmetric,       // Symmetric about a centerline
    Concentric       // Two circles concentric
}

public class SketchPlane
{
    public string Name { get; set; } = "XY Plane";
    public GeoCoordinate Origin { get; set; } = new();
    public double RotationX { get; set; }
    public double RotationY { get; set; }
    public double RotationZ { get; set; }
}

public class SketchEntity
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public SketchEntityType Type { get; set; }
    public List<GeoCoordinate> Points { get; set; } = new();
    public Dictionary<string, double> Parameters { get; set; } = new();
}

public enum SketchEntityType
{
    Line,
    Arc,
    Circle,
    Spline,
    Rectangle,
    Polygon
}

public class SketchConstraint
{
    public ConstraintType Type { get; set; }
    public List<string> EntityIds { get; set; } = new();
    public double Value { get; set; }
    
    public void Apply()
    {
        // Constraint solver logic
    }
}

public class AssemblyMate
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public MateType Type { get; set; }
    public string Feature1Id { get; set; } = "";
    public string Feature2Id { get; set; } = "";
    public Dictionary<string, double> Parameters { get; set; } = new();
}

public enum MateType
{
    Coincident,      // Two surfaces coincide
    Parallel,        // Two faces parallel
    Perpendicular,   // Two faces perpendicular
    Tangent,         // Two faces tangent
    Concentric,      // Two cylindrical faces concentric
    Distance,        // Fixed distance between faces
    Angle            // Fixed angle between faces
}

public class DesignConfiguration
{
    public string Name { get; set; } = "";
    public string Description { get; set; } = "";
    public Dictionary<string, FeatureConfigState> FeatureStates { get; set; } = new();
    public Dictionary<string, double> ParameterOverrides { get; set; } = new();
}

public class FeatureConfigState
{
    public bool IsSuppressed { get; set; }
    public bool IsVisible { get; set; } = true;
}

public class FeatureTreeStatistics
{
    public int TotalFeatures { get; set; }
    public int ActiveFeatures { get; set; }
    public int SuppressedFeatures { get; set; }
    public int ErrorFeatures { get; set; }
    public int SketchFeatures { get; set; }
    public int GeometryFeatures { get; set; }
    public int ConstraintFeatures { get; set; }
}

public class FeatureEventArgs : EventArgs
{
    public Feature Feature { get; set; } = null!;
}
