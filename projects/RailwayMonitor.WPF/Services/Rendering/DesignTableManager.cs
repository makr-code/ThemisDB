/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DesignTableManager.cs                              ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     513                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Data;

namespace RailwayMonitor.WPF.Services.Rendering;

/// <summary>
/// Design Table Manager (like SolidWorks Design Tables)
/// Allows Excel-like parametric control of features
/// </summary>
public class DesignTableManager
{
    private readonly Dictionary<string, DesignTable> _tables = new();
    private readonly FeatureTreeManager _featureTree;
    
    public DesignTableManager(FeatureTreeManager featureTree)
    {
        _featureTree = featureTree;
    }

    /// <summary>
    /// Create a new design table
    /// </summary>
    public DesignTable CreateTable(string name, string description = "")
    {
        var table = new DesignTable
        {
            Id = Guid.NewGuid().ToString(),
            Name = name,
            Description = description
        };
        
        _tables[table.Id] = table;
        return table;
    }

    /// <summary>
    /// Add a column (parameter) to track
    /// </summary>
    public void AddColumn(string tableId, string featureId, string parameterName)
    {
        if (_tables.TryGetValue(tableId, out var table))
        {
            table.Columns.Add(new DesignTableColumn
            {
                FeatureId = featureId,
                ParameterName = parameterName,
                DisplayName = $"{featureId}@{parameterName}"
            });
        }
    }

    /// <summary>
    /// Add a configuration row
    /// </summary>
    public void AddConfiguration(string tableId, string configName, Dictionary<string, object> values)
    {
        if (_tables.TryGetValue(tableId, out var table))
        {
            var row = new DesignTableRow
            {
                ConfigurationName = configName
            };
            
            foreach (var kvp in values)
            {
                row.Values[kvp.Key] = kvp.Value;
            }
            
            table.Rows.Add(row);
        }
    }

    /// <summary>
    /// Apply a configuration from the design table
    /// </summary>
    public void ApplyConfiguration(string tableId, string configName)
    {
        if (!_tables.TryGetValue(tableId, out var table))
            return;
            
        var row = table.Rows.FirstOrDefault(r => r.ConfigurationName == configName);
        if (row == null)
            return;
            
        // Apply each parameter value
        foreach (var column in table.Columns)
        {
            if (row.Values.TryGetValue(column.DisplayName, out var value))
            {
                ApplyParameterValue(column.FeatureId, column.ParameterName, value);
            }
        }
        
        // Trigger rebuild
        // _featureTree.RebuildFromFeature(null);
    }

    private void ApplyParameterValue(string featureId, string parameterName, object value)
    {
        _featureTree.EditFeature(featureId, feature =>
        {
            if (feature.Parameters.TryGetValue(parameterName, out var param))
            {
                if (value is double dVal)
                    param.Value = dVal;
                else if (value is int iVal)
                    param.Value = iVal;
                else if (value is string sVal && double.TryParse(sVal, out var parsed))
                    param.Value = parsed;
            }
        });
    }

    /// <summary>
    /// Export table to CSV
    /// </summary>
    public string ExportToCsv(string tableId)
    {
        if (!_tables.TryGetValue(tableId, out var table))
            return "";
            
        var csv = new System.Text.StringBuilder();
        
        // Header row
        csv.Append("Configuration");
        foreach (var col in table.Columns)
        {
            csv.Append($",{col.DisplayName}");
        }
        csv.AppendLine();
        
        // Data rows
        foreach (var row in table.Rows)
        {
            csv.Append(row.ConfigurationName);
            foreach (var col in table.Columns)
            {
                var value = row.Values.GetValueOrDefault(col.DisplayName, "");
                csv.Append($",{value}");
            }
            csv.AppendLine();
        }
        
        return csv.ToString();
    }

    /// <summary>
    /// Import table from CSV
    /// </summary>
    public void ImportFromCsv(string tableId, string csvContent)
    {
        if (!_tables.TryGetValue(tableId, out var table))
            return;
            
        var lines = csvContent.Split('\n');
        if (lines.Length < 2)
            return;
            
        // Parse header
        var headers = lines[0].Split(',').Select(h => h.Trim()).ToArray();
        
        // Clear existing data
        table.Rows.Clear();
        table.Columns.Clear();
        
        // Recreate columns (skip first column "Configuration")
        for (int i = 1; i < headers.Length; i++)
        {
            var parts = headers[i].Split('@');
            if (parts.Length == 2)
            {
                table.Columns.Add(new DesignTableColumn
                {
                    FeatureId = parts[0],
                    ParameterName = parts[1],
                    DisplayName = headers[i]
                });
            }
        }
        
        // Parse data rows
        for (int i = 1; i < lines.Length; i++)
        {
            var values = lines[i].Split(',').Select(v => v.Trim()).ToArray();
            if (values.Length < 2)
                continue;
                
            var row = new DesignTableRow
            {
                ConfigurationName = values[0]
            };
            
            for (int j = 1; j < values.Length && j < headers.Length; j++)
            {
                row.Values[headers[j]] = values[j];
            }
            
            table.Rows.Add(row);
        }
    }
}

/// <summary>
/// Design Table (like Excel table in SolidWorks)
/// </summary>
public class DesignTable
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public string Description { get; set; } = "";
    public List<DesignTableColumn> Columns { get; set; } = new();
    public List<DesignTableRow> Rows { get; set; } = new();
    public DateTime CreatedDate { get; set; } = DateTime.Now;
    public DateTime ModifiedDate { get; set; } = DateTime.Now;
}

public class DesignTableColumn
{
    public string FeatureId { get; set; } = "";
    public string ParameterName { get; set; } = "";
    public string DisplayName { get; set; } = "";
    public string Unit { get; set; } = "";
}

public class DesignTableRow
{
    public string ConfigurationName { get; set; } = "";
    public Dictionary<string, object> Values { get; set; } = new();
}

/// <summary>
/// CAD-Style Property Manager (Parameter Panel like SolidWorks)
/// </summary>
public class PropertyManager
{
    private Feature? _selectedFeature;
    
    public event EventHandler<PropertyChangedEventArgs>? PropertyChanged;
    
    public void SelectFeature(Feature feature)
    {
        _selectedFeature = feature;
    }
    
    public List<PropertyGroup> GetPropertyGroups()
    {
        if (_selectedFeature == null)
            return new List<PropertyGroup>();
            
        var groups = new List<PropertyGroup>();
        
        // Feature Info Group
        groups.Add(new PropertyGroup
        {
            Name = "Feature-Information",
            Properties = new List<Property>
            {
                new Property { Name = "Name", Value = _selectedFeature.Name, IsEditable = true },
                new Property { Name = "Typ", Value = _selectedFeature.Type.ToString(), IsEditable = false },
                new Property { Name = "Status", Value = _selectedFeature.State.ToString(), IsEditable = false },
                new Property { Name = "Erstellt", Value = _selectedFeature.CreatedDate, IsEditable = false },
                new Property { Name = "Geändert", Value = _selectedFeature.ModifiedDate, IsEditable = false }
            }
        });
        
        // Parameters Group
        if (_selectedFeature.Parameters.Any())
        {
            var paramGroup = new PropertyGroup
            {
                Name = "Parameter",
                Properties = new List<Property>()
            };
            
            foreach (var param in _selectedFeature.Parameters.Values)
            {
                paramGroup.Properties.Add(new Property
                {
                    Name = param.Name,
                    Value = param.Value,
                    Unit = param.Unit,
                    IsEditable = param.IsEditable,
                    MinValue = param.MinValue,
                    MaxValue = param.MaxValue
                });
            }
            
            groups.Add(paramGroup);
        }
        
        // Feature-specific properties
        groups.AddRange(GetFeatureSpecificProperties(_selectedFeature));
        
        return groups;
    }
    
    private List<PropertyGroup> GetFeatureSpecificProperties(Feature feature)
    {
        var groups = new List<PropertyGroup>();
        
        switch (feature)
        {
            case RailwayTrackFeature track:
                groups.Add(new PropertyGroup
                {
                    Name = "Gleiseigenschaften",
                    Properties = new List<Property>
                    {
                        new Property { Name = "Gleisanzahl", Value = track.NumberOfTracks, IsEditable = true },
                        new Property { Name = "Spurweite (m)", Value = track.TrackGauge, IsEditable = true },
                        new Property { Name = "Wegpunkte", Value = track.Centerline.Count, IsEditable = false }
                    }
                });
                break;
                
            case TunnelFeature tunnel:
                groups.Add(new PropertyGroup
                {
                    Name = "Tunneleigenschaften",
                    Properties = new List<Property>
                    {
                        new Property { Name = "Tunneltyp", Value = tunnel.TunnelType.ToString(), IsEditable = true },
                        new Property { Name = "Querschnittsfläche (m²)", Value = tunnel.CrossSectionArea, IsEditable = true }
                    }
                });
                break;
                
            case BridgeFeature bridge:
                groups.Add(new PropertyGroup
                {
                    Name = "Brückeneigenschaften",
                    Properties = new List<Property>
                    {
                        new Property { Name = "Brückentyp", Value = bridge.BridgeType.ToString(), IsEditable = true },
                        new Property { Name = "Fahrbahnbreite (m)", Value = bridge.DeckWidth, IsEditable = true },
                        new Property { Name = "Pfeiler", Value = bridge.Piers.Count, IsEditable = false }
                    }
                });
                break;
        }
        
        return groups;
    }
    
    public void SetPropertyValue(string propertyName, object value)
    {
        if (_selectedFeature == null)
            return;
            
        // Handle different property types
        if (propertyName == "Name")
        {
            _selectedFeature.Name = value.ToString() ?? "";
        }
        else if (_selectedFeature.Parameters.TryGetValue(propertyName, out var param))
        {
            if (value is double dVal)
                param.Value = dVal;
        }
        
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs { PropertyName = propertyName, NewValue = value });
    }
}

public class PropertyGroup
{
    public string Name { get; set; } = "";
    public bool IsExpanded { get; set; } = true;
    public List<Property> Properties { get; set; } = new();
}

public class Property
{
    public string Name { get; set; } = "";
    public object? Value { get; set; }
    public string Unit { get; set; } = "";
    public bool IsEditable { get; set; }
    public double MinValue { get; set; } = double.MinValue;
    public double MaxValue { get; set; } = double.MaxValue;
    public PropertyType Type { get; set; } = PropertyType.Text;
}

public enum PropertyType
{
    Text,
    Number,
    Boolean,
    Dropdown,
    Color,
    File
}

public class PropertyChangedEventArgs : EventArgs
{
    public string PropertyName { get; set; } = "";
    public object? OldValue { get; set; }
    public object? NewValue { get; set; }
}

/// <summary>
/// Measurement and Analysis Tools (like SolidWorks Measure)
/// </summary>
public class MeasurementTools
{
    public double MeasureDistance(GeoCoordinate point1, GeoCoordinate point2)
    {
        // Haversine formula for great circle distance
        var R = 6371000; // Earth radius in meters
        var lat1 = point1.Latitude * Math.PI / 180;
        var lat2 = point2.Latitude * Math.PI / 180;
        var dLat = (point2.Latitude - point1.Latitude) * Math.PI / 180;
        var dLon = (point2.Longitude - point1.Longitude) * Math.PI / 180;
        
        var a = Math.Sin(dLat / 2) * Math.Sin(dLat / 2) +
                Math.Cos(lat1) * Math.Cos(lat2) *
                Math.Sin(dLon / 2) * Math.Sin(dLon / 2);
        var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));
        
        return R * c;
    }
    
    public double MeasureAngle(GeoCoordinate vertex, GeoCoordinate point1, GeoCoordinate point2)
    {
        var v1x = point1.Longitude - vertex.Longitude;
        var v1y = point1.Latitude - vertex.Latitude;
        var v2x = point2.Longitude - vertex.Longitude;
        var v2y = point2.Latitude - vertex.Latitude;
        
        var dot = v1x * v2x + v1y * v2y;
        var mag1 = Math.Sqrt(v1x * v1x + v1y * v1y);
        var mag2 = Math.Sqrt(v2x * v2x + v2y * v2y);
        
        var angleRad = Math.Acos(dot / (mag1 * mag2));
        return angleRad * 180 / Math.PI;
    }
    
    public double CalculateArea(List<GeoCoordinate> polygon)
    {
        if (polygon.Count < 3)
            return 0;
            
        // Shoelace formula (approximate for small areas)
        double area = 0;
        for (int i = 0; i < polygon.Count; i++)
        {
            var j = (i + 1) % polygon.Count;
            area += polygon[i].Longitude * polygon[j].Latitude;
            area -= polygon[j].Longitude * polygon[i].Latitude;
        }
        
        area = Math.Abs(area) / 2.0;
        
        // Convert to square meters (approximate)
        var metersPerDegree = 111320;
        return area * metersPerDegree * metersPerDegree;
    }
    
    public MeasurementResult GetDetailedMeasurement(GeoCoordinate point1, GeoCoordinate point2)
    {
        var distance = MeasureDistance(point1, point2);
        var elevationChange = Math.Abs(point2.Elevation - point1.Elevation);
        var horizontalDistance = Math.Sqrt(distance * distance - elevationChange * elevationChange);
        var gradient = horizontalDistance > 0 ? (elevationChange / horizontalDistance) * 100 : 0;
        
        return new MeasurementResult
        {
            TotalDistance = distance,
            HorizontalDistance = horizontalDistance,
            ElevationChange = elevationChange,
            GradientPercent = gradient,
            StartPoint = point1,
            EndPoint = point2
        };
    }
}

public class MeasurementResult
{
    public double TotalDistance { get; set; }
    public double HorizontalDistance { get; set; }
    public double ElevationChange { get; set; }
    public double GradientPercent { get; set; }
    public GeoCoordinate StartPoint { get; set; } = new();
    public GeoCoordinate EndPoint { get; set; } = new();
}
