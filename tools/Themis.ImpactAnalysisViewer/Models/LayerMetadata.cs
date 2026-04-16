/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LayerMetadata.cs                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.ImpactAnalysisViewer.Models;

/// <summary>
/// Layer types supported by the impact analysis
/// </summary>
public enum LayerType
{
    Document,
    Process,
    Api,
    Database,
    UI,
    Infrastructure,
    Custom
}

/// <summary>
/// Metadata associated with a layer
/// </summary>
public class LayerMetadata
{
    public LayerType LayerType { get; set; }
    public string LayerName { get; set; } = string.Empty;
    public double Criticality { get; set; }
    public Dictionary<string, object> LayerProperties { get; set; } = new();
    
    // Visualization properties
    public string Color { get; set; } = "#808080";
    public double YPosition { get; set; }  // For 3D layer separation
    public bool Visible { get; set; } = true;
}

/// <summary>
/// Layer configuration for visualization
/// </summary>
public class LayerConfiguration
{
    public Dictionary<string, LayerVisualSettings> LayerSettings { get; set; } = new()
    {
        ["document"] = new() { Color = "#4A90E2", YPosition = 0.0, DisplayName = "Document Layer" },
        ["process"] = new() { Color = "#F5A623", YPosition = 1.0, DisplayName = "Process Layer" },
        ["api"] = new() { Color = "#7ED321", YPosition = 2.0, DisplayName = "API Layer" },
        ["database"] = new() { Color = "#BD10E0", YPosition = 3.0, DisplayName = "Database Layer" },
        ["ui"] = new() { Color = "#50E3C2", YPosition = 4.0, DisplayName = "UI Layer" },
        ["infrastructure"] = new() { Color = "#B8E986", YPosition = 5.0, DisplayName = "Infrastructure Layer" },
        ["custom"] = new() { Color = "#9013FE", YPosition = 6.0, DisplayName = "Custom Layer" }
    };
}

/// <summary>
/// Visual settings for a layer
/// </summary>
public class LayerVisualSettings
{
    public string Color { get; set; } = "#808080";
    public double YPosition { get; set; }
    public string DisplayName { get; set; } = string.Empty;
    public bool Visible { get; set; } = true;
    public double Opacity { get; set; } = 1.0;
}
