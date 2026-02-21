/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GeoModels.cs                                       ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     596                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e35bb0178  2025-12-10  Phase 25: Complete UI implementation (GeoView, GraphView,... ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • 172a52a7e  2025-12-07  Implement Phase 1 VIS features, native LLM support, and O... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// OSM Layer-basierte Geo-Visualisierung
/// Frei konfigurierbare Layer für geo-referenzierte Dokumente und Informationen
/// </summary>

#region Map Configuration

/// <summary>
/// Karten-Konfiguration
/// URN: urn:themis:geo:mapconfig:{id}
/// </summary>
public class MapConfiguration
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:geo:mapconfig:{Id}";
    
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    // Karteneinstellungen
    public MapCenter DefaultCenter { get; set; } = new();
    public int DefaultZoom { get; set; } = 6;
    public int MinZoom { get; set; } = 1;
    public int MaxZoom { get; set; } = 18;
    
    // Basis-Layer
    public string BaseLayerType { get; set; } = "OpenStreetMap"; // OSM, Satellite, Hybrid, etc.
    public string TileServerUrl { get; set; } = "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png";
    public string Attribution { get; set; } = "© OpenStreetMap contributors";
    
    // Layer
    public List<GeoLayer> Layers { get; set; } = new();
    
    // Interaktion
    public bool EnableClustering { get; set; } = true;
    public bool EnablePopups { get; set; } = true;
    public bool EnableDrawing { get; set; } = false;
    
    // UI Display
    public bool EnableHeatmap { get; set; } = false;
    public bool ShowLegend { get; set; } = true;
    public bool ShowLayerControl { get; set; } = true;
    
    public DateTime CreatedAt { get; set; }
    public string CreatedBy { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public class MapCenter
{
    public double Latitude { get; set; } = 51.1657; // Deutschland Zentrum
    public double Longitude { get; set; } = 10.4515;
}

#endregion

#region Geo Layer

/// <summary>
/// Konfigurierbarer Geo-Layer
/// URN: urn:themis:geo:layer:{id}
/// </summary>
public class GeoLayer
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:geo:layer:{Id}";
    
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public LayerType Type { get; set; }
    
    // Sichtbarkeit
    public bool IsVisible { get; set; } = true;
    public bool IsBaseLayer { get; set; } = false;
    public int ZIndex { get; set; } = 0;
    public int DisplayOrder { get; set; } = 0;
    public double Opacity { get; set; } = 1.0;
    
    // Zoom-Grenzen
    public int? MinZoom { get; set; }
    public int? MaxZoom { get; set; }
    
    // Datenquelle
    public DataSourceType SourceType { get; set; }
    public string SourceQuery { get; set; } = string.Empty; // AQL Query für ThemisDB
    public string SourceUrl { get; set; } = string.Empty; // Externe WMS/WFS/GeoJSON URL
    
    // Styling
    public LayerStyle Style { get; set; } = new();
    
    // Filter
    public LayerFilter Filter { get; set; } = new();
    
    // Interaktion
    public bool IsClickable { get; set; } = true;
    public bool ShowTooltip { get; set; } = true;
    public string TooltipTemplate { get; set; } = string.Empty;
    public string PopupTemplate { get; set; } = string.Empty;
    
    // Clustering
    public bool EnableClustering { get; set; } = false;
    public int ClusterRadius { get; set; } = 80;
    public int MaxClusterZoom { get; set; } = 15;
    
    // Daten-Caching
    public TimeSpan CacheDuration { get; set; } = TimeSpan.FromMinutes(5);
    public DateTime? LastUpdated { get; set; }
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public enum LayerType
{
    Markers,        // Einzelne Marker/Pins
    Heatmap,        // Heatmap
    Choropleth,     // Choroplethenkarte (gefärbte Flächen)
    Polygons,       // Polygone/Flächen
    Lines,          // Linien
    Vector,         // Vektordaten (Linien, Polygone)
    Raster,         // Rasterdaten (Tiles)
    GeoJSON,        // GeoJSON Layer
    WMS,            // Web Map Service
    WFS,            // Web Feature Service
    Custom          // Benutzerdefiniert
}

public enum DataSourceType
{
    ThemisDB,       // Daten aus ThemisDB
    GeoJSON,        // GeoJSON Datei/URL
    WMS,            // Web Map Service
    WFS,            // Web Feature Service
    Custom          // Custom Datenquelle
}

#endregion

#region Layer Styling

/// <summary>
/// Layer-Styling-Konfiguration
/// </summary>
public class LayerStyle
{
    // Marker-Stil
    public MarkerStyle Marker { get; set; } = new();
    
    // Linien-Stil
    public LineStyle Line { get; set; } = new();
    
    // Flächen-Stil
    public PolygonStyle Polygon { get; set; } = new();
    
    // Heatmap-Stil
    public HeatmapStyle Heatmap { get; set; } = new();
    
    // Cluster-Stil
    public ClusterStyle Cluster { get; set; } = new();
    
    // Convenience Properties (direct access)
    public string Color 
    { 
        get => Line.Color; 
        set => Line.Color = value; 
    }
    
    public string FillColor 
    { 
        get => Polygon.FillColor; 
        set => Polygon.FillColor = value; 
    }
    
    public double Opacity 
    { 
        get => Line.Opacity; 
        set { Line.Opacity = value; Polygon.FillOpacity = value; } 
    }
    
    public string IconUrl 
    { 
        get => Marker.IconUrl; 
        set => Marker.IconUrl = value; 
    }
    
    public int[] IconSize 
    { 
        get => new[] { Marker.IconSize, Marker.IconSize }; 
        set => Marker.IconSize = value.Length > 0 ? value[0] : 25; 
    }
    
    public int Weight 
    { 
        get => Line.Weight; 
        set { Line.Weight = value; Polygon.StrokeWeight = value; } 
    }
}

public class MarkerStyle
{
    public string IconUrl { get; set; } = string.Empty;
    public string IconType { get; set; } = "default"; // default, custom, svg, emoji
    public string IconColor { get; set; } = "#3388ff";
    public int IconSize { get; set; } = 25;
    public string IconShape { get; set; } = "circle"; // circle, square, pin, custom
    
    // Icon basierend auf Eigenschaften
    public bool UseConditionalIcons { get; set; }
    public List<ConditionalStyle> ConditionalIcons { get; set; } = new();
}

public class LineStyle
{
    public string Color { get; set; } = "#3388ff";
    public int Weight { get; set; } = 3;
    public double Opacity { get; set; } = 0.8;
    public string DashPattern { get; set; } = ""; // z.B. "5,10" für gestrichelt
    public string LineCap { get; set; } = "round";
    public string LineJoin { get; set; } = "round";
}

public class PolygonStyle
{
    public string FillColor { get; set; } = "#3388ff";
    public double FillOpacity { get; set; } = 0.3;
    public string StrokeColor { get; set; } = "#3388ff";
    public int StrokeWeight { get; set; } = 2;
    public double StrokeOpacity { get; set; } = 0.8;
}

public class HeatmapStyle
{
    public double Radius { get; set; } = 25;
    public double Blur { get; set; } = 15;
    public double MaxIntensity { get; set; } = 1.0;
    public List<HeatmapGradient> Gradient { get; set; } = new();
}

public class HeatmapGradient
{
    public double Value { get; set; } // 0.0 - 1.0
    public string Color { get; set; } = string.Empty;
}

public class ClusterStyle
{
    public string BackgroundColor { get; set; } = "#3388ff";
    public string TextColor { get; set; } = "#ffffff";
    public int MinSize { get; set; } = 30;
    public int MaxSize { get; set; } = 60;
    public string Shape { get; set; } = "circle";
}

public class ConditionalStyle
{
    public string Condition { get; set; } = string.Empty; // z.B. "priority == 'High'"
    public string IconUrl { get; set; } = string.Empty;
    public string IconColor { get; set; } = string.Empty;
    public int? IconSize { get; set; }
}

#endregion

#region Layer Filter

/// <summary>
/// Layer-Filter-Konfiguration
/// </summary>
public class LayerFilter
{
    public bool IsActive { get; set; } = false;
    
    // Zeitfilter
    public DateTime? StartDate { get; set; }
    public DateTime? EndDate { get; set; }
    
    // Attributfilter
    public Dictionary<string, object> AttributeFilters { get; set; } = new();
    
    // Bounding Box
    public GeoBounds? Bounds { get; set; }
    
    // Kategorien
    public List<string> IncludeCategories { get; set; } = new();
    public List<string> ExcludeCategories { get; set; } = new();
    
    // Sicherheitsstufe
    public List<SecurityClassification> AllowedSecurityLevels { get; set; } = new();
    
    // Custom Filter (AQL WHERE Clause)
    public string CustomFilter { get; set; } = string.Empty;
}

public class GeoBounds
{
    public double North { get; set; }
    public double South { get; set; }
    public double East { get; set; }
    public double West { get; set; }
}

#endregion

#region Geo Features

/// <summary>
/// Geo-Feature (GeoJSON kompatibel)
/// URN: urn:themis:geo:feature:{id}
/// </summary>
public class GeoFeature
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:geo:feature:{Id}";
    
    public FeatureType Type { get; set; }
    public GeoGeometry Geometry { get; set; } = new();
    public Dictionary<string, object> Properties { get; set; } = new();
    
    // ThemisDB-Integration
    public string DocumentId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    
    // Layer Association
    public string LayerId { get; set; } = string.Empty;
    
    // Metadaten
    public string Name { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    public string CreatedBy { get; set; } = string.Empty;
}

public enum FeatureType
{
    Point,
    LineString,
    Polygon,
    MultiPoint,
    MultiLineString,
    MultiPolygon
}

public class GeoGeometry
{
    public string Type { get; set; } = "Point";
    public object Coordinates { get; set; } = new object(); // [lon, lat] für Point, komplexer für andere
}

#endregion

#region Geo Document

/// <summary>
/// Geo-referenziertes Dokument
/// </summary>
public class GeoDocument
{
    public string DocumentId { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    // Geo-Referenz
    public GeoReferenceType ReferenceType { get; set; }
    public GeoPoint? Point { get; set; }
    public GeoPolygon? Area { get; set; }
    public GeoLine? Route { get; set; }
    
    // Adress-Information
    public GeoAddress? Address { get; set; }
    
    // Layer-Zuordnung
    public List<string> LayerIds { get; set; } = new();
    
    // Eigenschaften für Visualisierung
    public Dictionary<string, object> DisplayProperties { get; set; } = new();
}

public enum GeoReferenceType
{
    Point,          // Einzelpunkt
    Area,           // Fläche
    Route,          // Linie/Route
    MultiPoint,     // Mehrere Punkte
    Address         // Adresse (wird zu Point aufgelöst)
}

public class GeoPoint
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double? Elevation { get; set; }
    public double? Accuracy { get; set; } // in Metern
}

public class GeoPolygon
{
    public List<List<GeoPoint>> Rings { get; set; } = new(); // Outer ring + optional inner rings (holes)
}

public class GeoLine
{
    public List<GeoPoint> Points { get; set; } = new();
}

public class GeoAddress
{
    public string Street { get; set; } = string.Empty;
    public string HouseNumber { get; set; } = string.Empty;
    public string PostalCode { get; set; } = string.Empty;
    public string City { get; set; } = string.Empty;
    public string State { get; set; } = string.Empty;
    public string Country { get; set; } = "Deutschland";
    
    // Geocodierte Koordinaten
    public GeoPoint? Coordinates { get; set; }
    public double? GeocodingConfidence { get; set; }
}

#endregion

#region Map Interaction

/// <summary>
/// Karten-Interaktions-Event
/// </summary>
public class MapInteraction
{
    public string Id { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; }
    public string UserId { get; set; } = string.Empty;
    
    public MapInteractionType Type { get; set; }
    
    public GeoPoint? Location { get; set; }
    public string? FeatureId { get; set; }
    public string? LayerId { get; set; }
    
    public Dictionary<string, object> Data { get; set; } = new();
}

public enum MapInteractionType
{
    Click,
    DoubleClick,
    MarkerClick,
    PopupOpen,
    PopupClose,
    ZoomChange,
    BoundsChange,
    LayerToggle,
    FeatureSelect,
    DrawStart,
    DrawEnd,
    MeasureStart,
    MeasureEnd
}

#endregion

#region Drawing & Measurement

/// <summary>
/// Zeichnungs-Feature
/// </summary>
public class DrawingFeature
{
    public string Id { get; set; } = string.Empty;
    public FeatureType Type { get; set; }
    public GeoGeometry Geometry { get; set; } = new();
    
    public string Label { get; set; } = string.Empty;
    public string Color { get; set; } = "#ff0000";
    public int StrokeWidth { get; set; } = 2;
    
    public DateTime CreatedAt { get; set; }
    public string CreatedBy { get; set; } = string.Empty;
    
    // Verknüpfung
    public string? LinkedDocumentId { get; set; }
    public string? LinkedProcessId { get; set; }
}

/// <summary>
/// Mess-Ergebnis
/// </summary>
public class Measurement
{
    public string Id { get; set; } = string.Empty;
    public MeasurementType Type { get; set; }
    
    public double Value { get; set; }
    public string Unit { get; set; } = string.Empty;
    
    public GeoGeometry Geometry { get; set; } = new();
    public DateTime CreatedAt { get; set; }
}

public enum MeasurementType
{
    Distance,       // Entfernung
    Area,           // Fläche
    Perimeter       // Umfang
}

#endregion

#region Layer Groups

/// <summary>
/// Layer-Gruppe für Organisation
/// </summary>
public class LayerGroup
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    public List<string> LayerIds { get; set; } = new();
    
    public bool IsExpanded { get; set; } = true;
    public bool IsVisible { get; set; } = true;
    
    // Hierarchie
    public string? ParentGroupId { get; set; }
}

#endregion

#region Geocoding

/// <summary>
/// Geocoding-Ergebnis
/// </summary>
public class GeocodingResult
{
    public string InputAddress { get; set; } = string.Empty;
    public List<GeocodingMatch> Matches { get; set; } = new();
}

public class GeocodingMatch
{
    public GeoAddress Address { get; set; } = new();
    public GeoPoint Coordinates { get; set; } = new();
    public double Confidence { get; set; }
    public string Source { get; set; } = "Nominatim"; // OSM Nominatim, Google, etc.
    public GeoBounds? Bounds { get; set; }
}

#endregion

#region Statistics

/// <summary>
/// Geo-Statistiken für Layer
/// </summary>
public class LayerStatistics
{
    public string LayerId { get; set; } = string.Empty;
    public int TotalFeatures { get; set; }
    public int VisibleFeatures { get; set; }
    
    public GeoBounds Bounds { get; set; } = new();
    
    // Heatmap-Daten
    public Dictionary<string, int> CategoryDistribution { get; set; } = new();
    public Dictionary<string, int> TypeDistribution { get; set; } = new();
    
    public DateTime LastUpdated { get; set; }
}

#endregion
