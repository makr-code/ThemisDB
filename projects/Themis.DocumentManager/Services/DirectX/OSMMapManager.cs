/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OSMMapManager.cs                                   ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     529                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// OpenStreetMap (OSM) Integration für Geo-Visualization
/// </summary>
public class OSMMapManager
{
    private readonly HttpClient _httpClient;
    private Dictionary<string, OSMTile> _tileCache = new();
    private OSMMapConfig _config;
    private int _nextTileId = 0;

    public OSMMapManager(OSMMapConfig? config = null)
    {
        _config = config ?? new OSMMapConfig();
        _httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(10) };
    }

    /// <summary>
    /// Load OSM Tiles für Geographic Area
    /// </summary>
    public async Task<List<OSMTile>> LoadMapTilesAsync(double minLat, double maxLat, double minLon, double maxLon, int zoomLevel)
    {
        var tiles = new List<OSMTile>();

        try
        {
            // Calculate tile indices
            var (minTileX, minTileY) = LatLonToTile(minLat, minLon, zoomLevel);
            var (maxTileX, maxTileY) = LatLonToTile(maxLat, maxLon, zoomLevel);

            System.Diagnostics.Debug.WriteLine(
                $"Loading OSM tiles: Zoom={zoomLevel} Area=({minTileX},{minTileY}) to ({maxTileX},{maxTileY})");

            // Load tiles within bounds
            for (int tileX = minTileX; tileX <= maxTileX; tileX++)
            {
                for (int tileY = minTileY; tileY <= maxTileY; tileY++)
                {
                    string tileKey = $"{zoomLevel}_{tileX}_{tileY}";

                    // Check cache first
                    if (_tileCache.ContainsKey(tileKey))
                    {
                        tiles.Add(_tileCache[tileKey]);
                        continue;
                    }

                    // Load from server
                    var tile = await LoadTileAsync(zoomLevel, tileX, tileY);
                    if (tile != null)
                    {
                        tile.Id = _nextTileId++;
                        tile.CacheKey = tileKey;
                        _tileCache[tileKey] = tile;
                        tiles.Add(tile);
                    }
                }
            }

            System.Diagnostics.Debug.WriteLine(
                $"Loaded {tiles.Count} OSM tiles (Cache size: {_tileCache.Count})");

            return tiles;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error loading OSM tiles: {ex.Message}");
            return tiles;
        }
    }

    /// <summary>
    /// Load Single OSM Tile
    /// </summary>
    private async Task<OSMTile?> LoadTileAsync(int zoom, int tileX, int tileY)
    {
        try
        {
            // Use public OSM tile server
            string url = $"https://tile.openstreetmap.org/{zoom}/{tileX}/{tileY}.png";

            var response = await _httpClient.GetAsync(url);
            if (response.IsSuccessStatusCode)
            {
                var imageData = await response.Content.ReadAsByteArrayAsync();

                return new OSMTile
                {
                    Zoom = zoom,
                    TileX = tileX,
                    TileY = tileY,
                    ImageData = imageData,
                    LoadedAt = DateTime.UtcNow
                };
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error loading tile {zoom}/{tileX}/{tileY}: {ex.Message}");
        }

        return null;
    }

    /// <summary>
    /// Convert Lat/Lon to Tile Coordinates
    /// </summary>
    private (int TileX, int TileY) LatLonToTile(double lat, double lon, int zoom)
    {
        int tileX = (int)Math.Floor((lon + 180) / 360 * (1 << zoom));
        int tileY = (int)Math.Floor((1 - Math.Log(Math.Tan(lat * Math.PI / 180) + 1 / Math.Cos(lat * Math.PI / 180)) / Math.PI) / 2 * (1 << zoom));

        return (tileX, tileY);
    }

    /// <summary>
    /// Convert Tile Coordinates to Lat/Lon
    /// </summary>
    public (double Lat, double Lon) TileToLatLon(int tileX, int tileY, int zoom)
    {
        double lon = tileX / (double)(1 << zoom) * 360 - 180;
        double lat = Math.Atan(Math.Sinh(Math.PI * (1 - 2 * tileY / (double)(1 << zoom)))) * 180 / Math.PI;

        return (lat, lon);
    }

    /// <summary>
    /// Get Tile Statistics
    /// </summary>
    public OSMTileStatistics GetStatistics()
    {
        long totalImageBytes = _tileCache.Values.Sum(t => t.ImageData?.Length ?? 0);

        return new OSMTileStatistics
        {
            CachedTiles = _tileCache.Count,
            TotalImageBytes = totalImageBytes,
            AverageTileSize = _tileCache.Count > 0 ? totalImageBytes / _tileCache.Count : 0,
            OldestTile = _tileCache.Values.OrderBy(t => t.LoadedAt).FirstOrDefault()?.LoadedAt ?? DateTime.UtcNow
        };
    }

    /// <summary>
    /// Clear Tile Cache
    /// </summary>
    public void ClearCache()
    {
        _tileCache.Clear();
        System.Diagnostics.Debug.WriteLine("OSM tile cache cleared");
    }

    /// <summary>
    /// Get Tile from Cache
    /// </summary>
    public OSMTile? GetCachedTile(int zoom, int tileX, int tileY)
    {
        string key = $"{zoom}_{tileX}_{tileY}";
        return _tileCache.ContainsKey(key) ? _tileCache[key] : null;
    }
}

/// <summary>
/// OSM Tile Data
/// </summary>
public class OSMTile
{
    public int Id { get; set; }
    public string CacheKey { get; set; } = "";
    public int Zoom { get; set; }
    public int TileX { get; set; }
    public int TileY { get; set; }
    public byte[]? ImageData { get; set; }
    public DateTime LoadedAt { get; set; }

    public long GetMemorySize()
    {
        return ImageData?.Length ?? 0;
    }

    public override string ToString()
    {
        return $"OSM Tile({Zoom}/{TileX}/{TileY}, {GetMemorySize() / 1024}KB)";
    }
}

/// <summary>
/// OSM Map Configuration
/// </summary>
public class OSMMapConfig
{
    public string TileServer { get; set; } = "https://tile.openstreetmap.org";
    public int MaxCachedTiles { get; set; } = 100;
    public int DefaultZoomLevel { get; set; } = 10;
    public int MinZoomLevel { get; set; } = 2;
    public int MaxZoomLevel { get; set; } = 18;
    public int TileSize { get; set; } = 256;  // 256×256 pixels
}

/// <summary>
/// OSM Tile Cache Statistics
/// </summary>
public class OSMTileStatistics
{
    public int CachedTiles { get; set; }
    public long TotalImageBytes { get; set; }
    public long AverageTileSize { get; set; }
    public DateTime OldestTile { get; set; }

    public override string ToString()
    {
        return $"OSM Tiles: {CachedTiles} cached | {TotalImageBytes / 1024}KB total | " +
               $"Avg {AverageTileSize / 1024}KB per tile";
    }
}

/// <summary>
/// Geo-Location Data Point
/// </summary>
public class GeoPoint
{
    public string Id { get; set; } = "";
    public string Label { get; set; } = "";
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double? Altitude { get; set; }
    public string Color { get; set; } = "#FF0000";
    public float Radius { get; set; } = 5.0f;
    public Dictionary<string, object>? Metadata { get; set; }

    public override string ToString()
    {
        return $"{Label} ({Latitude:F4}, {Longitude:F4})";
    }
}

/// <summary>
/// Geo-Location Track/Path
/// </summary>
public class GeoTrack
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public List<GeoPoint> Points { get; set; } = new();
    public string Color { get; set; } = "#0000FF";
    public float StrokeWidth { get; set; } = 2.0f;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    public double GetTotalDistance()
    {
        double distance = 0;
        for (int i = 1; i < Points.Count; i++)
        {
            distance += CalculateDistance(Points[i - 1], Points[i]);
        }
        return distance;
    }

    private static double CalculateDistance(GeoPoint p1, GeoPoint p2)
    {
        const double EarthRadiusKm = 6371;

        double lat1Rad = p1.Latitude * Math.PI / 180;
        double lat2Rad = p2.Latitude * Math.PI / 180;
        double deltaLat = (p2.Latitude - p1.Latitude) * Math.PI / 180;
        double deltaLon = (p2.Longitude - p1.Longitude) * Math.PI / 180;

        double a = Math.Sin(deltaLat / 2) * Math.Sin(deltaLat / 2) +
                   Math.Cos(lat1Rad) * Math.Cos(lat2Rad) *
                   Math.Sin(deltaLon / 2) * Math.Sin(deltaLon / 2);

        double c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));

        return EarthRadiusKm * c;
    }
}

/// <summary>
/// Geo-Fence Region
/// </summary>
public class GeoFence
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public List<GeoPoint> Boundary { get; set; } = new();
    public string Color { get; set; } = "#00FF00";
    public bool IsActive { get; set; } = true;

    public bool IsPointInside(GeoPoint point)
    {
        // Ray casting algorithm for point-in-polygon
        int intersections = 0;
        for (int i = 0; i < Boundary.Count; i++)
        {
            var p1 = Boundary[i];
            var p2 = Boundary[(i + 1) % Boundary.Count];

            if ((p1.Latitude <= point.Latitude && point.Latitude < p2.Latitude ||
                 p2.Latitude <= point.Latitude && point.Latitude < p1.Latitude) &&
                point.Longitude < (p2.Longitude - p1.Longitude) * (point.Latitude - p1.Latitude) /
                                   (p2.Latitude - p1.Latitude) + p1.Longitude)
            {
                intersections++;
            }
        }

        return intersections % 2 == 1;
    }
}

/// <summary>
/// Map Layer Manager
/// </summary>
public class MapLayerManager
{
    private Dictionary<string, MapLayer> _layers = new();

    public void AddLayer(string name, MapLayer layer)
    {
        _layers[name] = layer;
        System.Diagnostics.Debug.WriteLine($"Added layer: {name}");
    }

    public MapLayer? GetLayer(string name)
    {
        return _layers.ContainsKey(name) ? _layers[name] : null;
    }

    public void RemoveLayer(string name)
    {
        if (_layers.Remove(name))
            System.Diagnostics.Debug.WriteLine($"Removed layer: {name}");
    }

    public void SetLayerVisibility(string name, bool visible)
    {
        if (_layers.ContainsKey(name))
        {
            _layers[name].IsVisible = visible;
            System.Diagnostics.Debug.WriteLine($"Layer {name} visibility: {visible}");
        }
    }

    public List<MapLayer> GetVisibleLayers()
    {
        return _layers.Values.Where(l => l.IsVisible).ToList();
    }

    public List<string> GetLayerNames()
    {
        return _layers.Keys.ToList();
    }
}

/// <summary>
/// Map Layer
/// </summary>
public class MapLayer
{
    public string Name { get; set; } = "";
    public string Type { get; set; } = "";  // "Points", "Tracks", "Fences", "Tiles"
    public bool IsVisible { get; set; } = true;
    public float Opacity { get; set; } = 1.0f;
    public int ZIndex { get; set; } = 0;
    public List<object> Features { get; set; } = new();

    public void AddFeature(object feature)
    {
        Features.Add(feature);
    }

    public void RemoveFeature(object feature)
    {
        Features.Remove(feature);
    }

    public int GetFeatureCount()
    {
        return Features.Count;
    }
}

/// <summary>
/// Geo-Spatial Query Engine
/// </summary>
public class GeoSpatialQueryEngine
{
    /// <summary>
    /// Find Points within Bounding Box
    /// </summary>
    public static List<GeoPoint> QueryByBoundingBox(List<GeoPoint> points, double minLat, double maxLat, double minLon, double maxLon)
    {
        return points
            .Where(p => p.Latitude >= minLat && p.Latitude <= maxLat &&
                       p.Longitude >= minLon && p.Longitude <= maxLon)
            .ToList();
    }

    /// <summary>
    /// Find Points within Radius
    /// </summary>
    public static List<GeoPoint> QueryByRadius(List<GeoPoint> points, GeoPoint center, double radiusKm)
    {
        const double EarthRadiusKm = 6371;

        return points
            .Where(p =>
            {
                double lat1 = center.Latitude * Math.PI / 180;
                double lat2 = p.Latitude * Math.PI / 180;
                double deltaLat = (p.Latitude - center.Latitude) * Math.PI / 180;
                double deltaLon = (p.Longitude - center.Longitude) * Math.PI / 180;

                double a = Math.Sin(deltaLat / 2) * Math.Sin(deltaLat / 2) +
                           Math.Cos(lat1) * Math.Cos(lat2) *
                           Math.Sin(deltaLon / 2) * Math.Sin(deltaLon / 2);

                double c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));
                double distance = EarthRadiusKm * c;

                return distance <= radiusKm;
            })
            .ToList();
    }

    /// <summary>
    /// Find Nearest Point
    /// </summary>
    public static GeoPoint? FindNearest(List<GeoPoint> points, GeoPoint reference)
    {
        if (points.Count == 0) return null;

        return points
            .Select(p => new { Point = p, Distance = CalculateDistance(reference, p) })
            .OrderBy(x => x.Distance)
            .First()
            .Point;
    }

    private static double CalculateDistance(GeoPoint p1, GeoPoint p2)
    {
        const double EarthRadiusKm = 6371;

        double lat1 = p1.Latitude * Math.PI / 180;
        double lat2 = p2.Latitude * Math.PI / 180;
        double deltaLat = (p2.Latitude - p1.Latitude) * Math.PI / 180;
        double deltaLon = (p2.Longitude - p1.Longitude) * Math.PI / 180;

        double a = Math.Sin(deltaLat / 2) * Math.Sin(deltaLat / 2) +
                   Math.Cos(lat1) * Math.Cos(lat2) *
                   Math.Sin(deltaLon / 2) * Math.Sin(deltaLon / 2);

        double c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));

        return EarthRadiusKm * c;
    }
}

/// <summary>
/// Map Viewport Manager
/// </summary>
public class MapViewport
{
    public double CenterLat { get; set; }
    public double CenterLon { get; set; }
    public int ZoomLevel { get; set; }
    public double MinLat { get; set; }
    public double MaxLat { get; set; }
    public double MinLon { get; set; }
    public double MaxLon { get; set; }

    public void UpdateBounds(int screenWidth, int screenHeight)
    {
        // Calculate bounds based on zoom level and center
        double latSpan = 85.051129 / Math.Pow(2, ZoomLevel);
        double lonSpan = 180 / Math.Pow(2, ZoomLevel);

        double aspectRatio = screenWidth / (double)screenHeight;

        MinLat = CenterLat - latSpan / 2;
        MaxLat = CenterLat + latSpan / 2;
        MinLon = CenterLon - (lonSpan / 2) * aspectRatio;
        MaxLon = CenterLon + (lonSpan / 2) * aspectRatio;
    }

    public bool IsPointVisible(GeoPoint point)
    {
        return point.Latitude >= MinLat && point.Latitude <= MaxLat &&
               point.Longitude >= MinLon && point.Longitude <= MaxLon;
    }

    public override string ToString()
    {
        return $"Viewport: Zoom={ZoomLevel} Center=({CenterLat:F4},{CenterLon:F4}) " +
               $"Bounds=({MinLat:F4},{MaxLat:F4})/({MinLon:F4},{MaxLon:F4})";
    }
}
