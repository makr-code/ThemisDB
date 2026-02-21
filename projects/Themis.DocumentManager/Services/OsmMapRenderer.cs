/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OsmMapRenderer.cs                                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     252                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// OSM-Map Renderer Service mit Leaflet.js WebView-Integration
/// Erstellt HTML/JavaScript für interaktive Kartenvisualisierung
/// </summary>
/// 
public interface IOsmMapRenderer
{
    string GenerateMapHtml(MapConfiguration config, List<GeoLayer> layers);
    string GenerateLayerJs(GeoLayer layer);
    string GenerateFeaturePopup(GeoFeature feature);
    Task<string> GenerateBaseLayerUrlAsync(string layerType);
}

public class OsmMapRenderer : IOsmMapRenderer
{
    /// <summary>
    /// Generiert vollständige HTML/JavaScript für Leaflet-basierte Karte
    /// </summary>
    public string GenerateMapHtml(MapConfiguration config, List<GeoLayer> layers)
    {
        var sb = new StringBuilder();

        sb.AppendLine("<!DOCTYPE html>");
        sb.AppendLine("<html>");
        sb.AppendLine("<head>");
        sb.AppendLine("    <meta charset='utf-8' />");
        sb.AppendLine("    <meta name='viewport' content='width=device-width, initial-scale=1.0'>");
        sb.AppendLine("    <title>OSM Map - Themis</title>");
        sb.AppendLine("    <link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/leaflet.min.css' />");
        sb.AppendLine("    <script src='https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/leaflet.min.js'></script>");
        sb.AppendLine("    <script src='https://cdnjs.cloudflare.com/ajax/libs/leaflet.markercluster/1.4.1/leaflet.markercluster.min.js'></script>");
        sb.AppendLine("    <link href='https://cdnjs.cloudflare.com/ajax/libs/leaflet.markercluster/1.4.1/MarkerCluster.css' rel='stylesheet' />");
        sb.AppendLine("    <style>");
        sb.AppendLine("        body { margin: 0; padding: 0; }");
        sb.AppendLine("        #map { position: absolute; top: 0; bottom: 0; width: 100%; }");
        sb.AppendLine("        .legend { background: white; padding: 10px; border-radius: 5px; box-shadow: 0 0 15px rgba(0,0,0,0.2); }");
        sb.AppendLine("        .legend h4 { margin: 0 0 10px 0; }");
        sb.AppendLine("        .legend-item { margin: 5px 0; }");
        sb.AppendLine("    </style>");
        sb.AppendLine("</head>");
        sb.AppendLine("<body>");
        sb.AppendLine("    <div id='map'></div>");
        sb.AppendLine("    <script>");
        sb.AppendLine($"        var map = L.map('map').setView([{config.DefaultCenter.Latitude}, {config.DefaultCenter.Longitude}], {config.DefaultZoom});");

        // Base Layer
        var baseLayerUrl = GetBaseLayerUrl(config.BaseLayerType);
        sb.AppendLine($"        L.tileLayer('{baseLayerUrl}', {{");
        sb.AppendLine($"            attribution: '{config.Attribution}',");
        sb.AppendLine($"            minZoom: {config.MinZoom},");
        sb.AppendLine($"            maxZoom: {config.MaxZoom}");
        sb.AppendLine("        }}).addTo(map);");

        // Layer Groups
        var layerControl = new Dictionary<string, string>();

        if (config.EnableClustering)
        {
            sb.AppendLine("        var markerClusterGroup = L.markerClusterGroup();");
        }

        // Layer-Rendering
        int layerIndex = 0;
        foreach (var layer in config.Layers)
        {
            if (!layer.IsVisible) continue;

            var layerVarName = $"layer{layerIndex}";
            sb.AppendLine($"        var {layerVarName} = L.featureGroup();");

            // Layer-Styling und Daten
            sb.AppendLine(GenerateLayerJs(layer));

            sb.AppendLine($"        {layerVarName}.addTo(map);");
            layerControl[layer.Name] = layerVarName;

            layerIndex++;
        }

        // Layer Control
        if (layerControl.Count > 0)
        {
            sb.AppendLine("        var layerControl = L.control.layers({}, {");
            var layerEntries = new List<string>();
            foreach (var lc in layerControl)
            {
                layerEntries.Add($"            '{lc.Key}': {lc.Value}");
            }
            sb.AppendLine(string.Join(",\n", layerEntries));
            sb.AppendLine("        }).addTo(map);");
        }

        // Legend
        if (config.Layers.Count > 0)
        {
            sb.AppendLine("        var legend = L.control({position: 'bottomright'});");
            sb.AppendLine("        legend.onAdd = function(map) {");
            sb.AppendLine("            var div = L.DomUtil.create('div', 'legend');");
            sb.AppendLine("            div.innerHTML = '<h4>Layer</h4>';");
            foreach (var layer in config.Layers)
            {
                sb.AppendLine($"            div.innerHTML += '<div class=\"legend-item\"><input type=\"checkbox\" checked /> {layer.Name}</div>';");
            }
            sb.AppendLine("            return div;");
            sb.AppendLine("        };");
            sb.AppendLine("        legend.addTo(map);");
        }

        sb.AppendLine("    </script>");
        sb.AppendLine("</body>");
        sb.AppendLine("</html>");

        return sb.ToString();
    }

    /// <summary>
    /// Generiert JavaScript für einzelnen Layer
    /// </summary>
    public string GenerateLayerJs(GeoLayer layer)
    {
        var sb = new StringBuilder();

        switch (layer.Type)
        {
            case LayerType.Markers:
                sb.AppendLine($"        // Marker Layer: {layer.Name}");
                sb.AppendLine($"        var marker = L.marker([51.5, -0.09]).addTo(layer0);");
                sb.AppendLine($"        marker.bindPopup('<b>{layer.Name}</b>');");
                break;

            case LayerType.GeoJSON:
                sb.AppendLine($"        // GeoJSON Layer: {layer.Name}");
                sb.AppendLine($"        var geojsonData = {{}};");
                sb.AppendLine($"        L.geoJSON(geojsonData, {{");
                sb.AppendLine($"            style: {{");
                sb.AppendLine($"                color: '{layer.Style.Polygon.StrokeColor}',");
                sb.AppendLine($"                weight: {layer.Style.Polygon.StrokeWeight},");
                sb.AppendLine($"                opacity: {layer.Style.Polygon.StrokeOpacity},");
                sb.AppendLine($"                fillColor: '{layer.Style.Polygon.FillColor}',");
                sb.AppendLine($"                fillOpacity: {layer.Style.Polygon.FillOpacity}");
                sb.AppendLine($"            }}");
                sb.AppendLine($"        }}).addTo(layer0);");
                break;

            case LayerType.Heatmap:
                sb.AppendLine($"        // Heatmap Layer: {layer.Name}");
                sb.AppendLine($"        var heatmapPoints = [];");
                sb.AppendLine($"        // Load heatmap data from data source");
                break;

            default:
                sb.AppendLine($"        // Generic Layer: {layer.Name}");
                break;
        }

        return sb.ToString();
    }

    /// <summary>
    /// Generiert PopUp HTML für Feature
    /// </summary>
    public string GenerateFeaturePopup(GeoFeature feature)
    {
        var sb = new StringBuilder();

        sb.AppendLine("<div class='feature-popup'>");
        if (!string.IsNullOrEmpty(feature.Title))
        {
            sb.AppendLine($"  <h3>{feature.Title}</h3>");
        }
        if (!string.IsNullOrEmpty(feature.Description))
        {
            sb.AppendLine($"  <p>{feature.Description}</p>");
        }
        if (feature.Properties.Count > 0)
        {
            sb.AppendLine("  <dl>");
            foreach (var prop in feature.Properties)
            {
                sb.AppendLine($"    <dt>{prop.Key}:</dt>");
                sb.AppendLine($"    <dd>{prop.Value}</dd>");
            }
            sb.AppendLine("  </dl>");
        }
        sb.AppendLine("</div>");

        return sb.ToString();
    }

    /// <summary>
    /// Ermittelt Base-Layer URL basierend auf Typ
    /// </summary>
    public async Task<string> GenerateBaseLayerUrlAsync(string layerType)
    {
        return layerType switch
        {
            "OpenStreetMap" => "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
            "Satellite" => "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}",
            "Terrain" => "https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/{z}/{y}/{x}",
            "CartoDB" => "https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png",
            "CartoDB Dark" => "https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png",
            _ => "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
        };
    }

    private string GetBaseLayerUrl(string layerType)
    {
        return layerType switch
        {
            "OpenStreetMap" => "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
            "Satellite" => "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}",
            "Terrain" => "https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/{z}/{y}/{x}",
            "CartoDB" => "https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png",
            "CartoDB Dark" => "https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png",
            _ => "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
        };
    }
}
