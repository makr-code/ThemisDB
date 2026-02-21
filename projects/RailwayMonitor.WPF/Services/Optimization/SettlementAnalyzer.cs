/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SettlementAnalyzer.cs                              ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     360                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Services.Optimization;

/// <summary>
/// Analyzes settlements and urban areas to determine routing constraints.
/// Prevents unrealistic routes through dense urban areas without tunnels.
/// </summary>
public class SettlementAnalyzer
{
    private readonly HttpClient _httpClient;
    private readonly string _overpassUrl = "https://overpass-api.de/api/interpreter";

    public SettlementAnalyzer(HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    /// <summary>
    /// Analyzes settlement at given location.
    /// </summary>
    public async Task<SettlementInfo> AnalyzeSettlementAsync(
        GeoPoint point,
        double radiusKm = 1.0)
    {
        try
        {
            // Query OSM for buildings
            var buildings = await QueryBuildingsAsync(point, radiusKm);
            
            // Classify settlement type
            var type = ClassifySettlement(buildings, radiusKm);
            
            // Count specific amenities
            var schools = buildings.Count(b => IsSchool(b));
            var hospitals = buildings.Count(b => IsHospital(b));
            var churches = buildings.Count(b => IsChurch(b));
            var residential = buildings.Count(b => IsResidential(b));
            
            // Estimate population
            var population = EstimatePopulation(buildings, type);
            
            return new SettlementInfo
            {
                Location = point,
                Type = type,
                RadiusKm = radiusKm,
                BuildingCount = buildings.Count,
                BuildingDensity = buildings.Count / (Math.PI * radiusKm * radiusKm),
                PopulationEstimate = population,
                Schools = schools,
                Hospitals = hospitals,
                Churches = churches,
                ResidentialBuildings = residential,
                AllowSurfaceRoute = type <= SettlementType.Village,
                RequiresTunnel = type >= SettlementType.City,
                EstimatedPenaltyCost = CalculateUrbanPenalty(type, radiusKm)
            };
        }
        catch
        {
            // Fallback to rural classification if API fails
            return new SettlementInfo
            {
                Location = point,
                Type = SettlementType.Rural,
                RadiusKm = radiusKm,
                AllowSurfaceRoute = true,
                RequiresTunnel = false
            };
        }
    }

    /// <summary>
    /// Queries Overpass API for buildings in radius.
    /// </summary>
    private async Task<List<OsmBuilding>> QueryBuildingsAsync(GeoPoint center, double radiusKm)
    {
        var radiusMeters = (int)(radiusKm * 1000);
        
        var query = $@"
[out:json][timeout:25];
(
  way[""building""]
    (around:{radiusMeters},{center.Latitude},{center.Longitude});
  relation[""building""]
    (around:{radiusMeters},{center.Latitude},{center.Longitude});
);
out tags;
";

        var content = new FormUrlEncodedContent(new[]
        {
            new KeyValuePair<string, string>("data", query)
        });

        var response = await _httpClient.PostAsync(_overpassUrl, content);
        response.EnsureSuccessStatusCode();
        
        var json = await response.Content.ReadAsStringAsync();
        var result = JsonSerializer.Deserialize<OverpassResult>(json);
        
        return result?.Elements
            .Select(e => new OsmBuilding
            {
                Id = e.Id,
                Tags = e.Tags
            }).ToList() ?? new List<OsmBuilding>();
    }

    /// <summary>
    /// Classifies settlement type based on building density and count.
    /// </summary>
    private SettlementType ClassifySettlement(List<OsmBuilding> buildings, double radiusKm)
    {
        var count = buildings.Count;
        var density = count / (Math.PI * radiusKm * radiusKm); // buildings per km²
        
        // Classification based on German settlement patterns
        if (count < 50 || density < 100)
            return SettlementType.Rural;
        
        if (count < 200 || density < 500)
            return SettlementType.Village;
        
        if (count < 1000 || density < 2000)
            return SettlementType.SmallTown;
        
        if (count < 5000 || density < 5000)
            return SettlementType.MediumTown;
        
        if (count < 20000 || density < 10000)
            return SettlementType.City;
        
        return SettlementType.MetroArea;
    }

    /// <summary>
    /// Estimates population from building count and type.
    /// </summary>
    private int EstimatePopulation(List<OsmBuilding> buildings, SettlementType type)
    {
        // Average people per building varies by settlement type
        var avgPeoplePerBuilding = type switch
        {
            SettlementType.Rural => 2.5,        // Single family homes
            SettlementType.Village => 3.0,      // Mix of homes
            SettlementType.SmallTown => 4.0,    // More apartments
            SettlementType.MediumTown => 6.0,   // Apartment buildings
            SettlementType.City => 8.0,         // High-rises
            SettlementType.MetroArea => 12.0,   // Dense urban
            _ => 3.0
        };
        
        // Count only residential buildings
        var residentialCount = buildings.Count(IsResidential);
        
        return (int)(residentialCount * avgPeoplePerBuilding);
    }

    /// <summary>
    /// Calculates urban penalty cost for routing through settlement.
    /// </summary>
    private decimal CalculateUrbanPenalty(SettlementType type, double lengthKm)
    {
        var penaltyPerKm = type switch
        {
            SettlementType.Rural => 0m,                    // No penalty
            SettlementType.Village => 2_000_000m,          // Lärmschutz
            SettlementType.SmallTown => 10_000_000m,       // Enteignungen
            SettlementType.MediumTown => 50_000_000m,      // Massive Enteignungen
            SettlementType.City => 200_000_000m,           // Tunnel praktisch erforderlich
            SettlementType.MetroArea => 500_000_000m,      // Stuttgart 21-Level
            _ => 0m
        };
        
        return penaltyPerKm * (decimal)lengthKm;
    }

    // Helper methods to classify buildings
    private bool IsSchool(OsmBuilding b) =>
        b.Tags.GetValueOrDefault("amenity") == "school" ||
        b.Tags.GetValueOrDefault("building") == "school";

    private bool IsHospital(OsmBuilding b) =>
        b.Tags.GetValueOrDefault("amenity") == "hospital" ||
        b.Tags.GetValueOrDefault("building") == "hospital";

    private bool IsChurch(OsmBuilding b) =>
        b.Tags.GetValueOrDefault("amenity") == "place_of_worship" ||
        b.Tags.GetValueOrDefault("building") == "church";

    private bool IsResidential(OsmBuilding b)
    {
        var building = b.Tags.GetValueOrDefault("building");
        return building == "residential" ||
               building == "house" ||
               building == "apartments" ||
               building == "detached" ||
               building == "terrace";
    }
}

/// <summary>
/// Urban routing constraints for pathfinding algorithm.
/// </summary>
public class UrbanConstraints
{
    /// <summary>
    /// Determines if surface route is allowed through settlement.
    /// </summary>
    public bool AllowSurfaceRoute(SettlementType type)
    {
        return type switch
        {
            SettlementType.Rural => true,
            SettlementType.Village => true,      // Allowed with noise barriers
            SettlementType.SmallTown => false,   // Surface only with very high cost
            SettlementType.MediumTown => false,  // Tunnel preferred
            SettlementType.City => false,        // Tunnel required
            SettlementType.MetroArea => false,   // Tunnel absolutely required
            _ => false
        };
    }

    /// <summary>
    /// Gets additional cost for routing through urban area.
    /// </summary>
    public decimal GetUrbanPenaltyCost(SettlementType type, double lengthKm)
    {
        var penaltyPerKm = type switch
        {
            SettlementType.Rural => 0m,
            SettlementType.Village => 2_000_000m,          // Noise barriers: 2 Mio €/km
            SettlementType.SmallTown => 10_000_000m,       // Land acquisition + noise: 10 Mio €/km
            SettlementType.MediumTown => 50_000_000m,      // Massive acquisition: 50 Mio €/km
            SettlementType.City => 200_000_000m,           // Forces tunnel: 200 Mio €/km
            SettlementType.MetroArea => 500_000_000m,      // Stuttgart 21 level: 500 Mio €/km
            _ => 0m
        };
        
        return penaltyPerKm * (decimal)lengthKm;
    }

    /// <summary>
    /// Gets noise barrier requirements.
    /// </summary>
    public NoiseBarrierRequirement GetNoiseBarrierRequirement(SettlementType type, int residentialBuildings)
    {
        if (type <= SettlementType.Rural)
            return NoiseBarrierRequirement.None;
        
        if (type == SettlementType.Village && residentialBuildings < 10)
            return NoiseBarrierRequirement.Minimal; // 1 Mio €/km
        
        if (type <= SettlementType.SmallTown)
            return NoiseBarrierRequirement.Standard; // 2 Mio €/km
        
        return NoiseBarrierRequirement.Enhanced; // 4 Mio €/km
    }
}

// ============================================================================
// Data Models
// ============================================================================

public enum SettlementType
{
    Rural = 0,          // <500 inhabitants: Full throughfare OK
    Village = 1,        // 500-5,000: Throughfare with noise barriers
    SmallTown = 2,      // 5k-20k: Bypass preferred, tunnel possible
    MediumTown = 3,     // 20k-100k: Bypass strongly recommended
    City = 4,           // 100k-500k: Tunnel only or large bypass
    MetroArea = 5       // >500k: Tunnel exclusively (Stuttgart 21 style)
}

public enum NoiseBarrierRequirement
{
    None,       // 0 €/km
    Minimal,    // 1 Mio €/km
    Standard,   // 2 Mio €/km
    Enhanced    // 4 Mio €/km
}

public class SettlementInfo
{
    public GeoPoint Location { get; set; } = null!;
    public SettlementType Type { get; set; }
    public double RadiusKm { get; set; }
    public int BuildingCount { get; set; }
    public double BuildingDensity { get; set; }
    public int PopulationEstimate { get; set; }
    public int Schools { get; set; }
    public int Hospitals { get; set; }
    public int Churches { get; set; }
    public int ResidentialBuildings { get; set; }
    public bool AllowSurfaceRoute { get; set; }
    public bool RequiresTunnel { get; set; }
    public decimal EstimatedPenaltyCost { get; set; }
    
    public string GetDescription() => Type switch
    {
        SettlementType.Rural => $"Ländlich ({PopulationEstimate} Einw.)",
        SettlementType.Village => $"Dorf ({PopulationEstimate} Einw., {BuildingCount} Gebäude)",
        SettlementType.SmallTown => $"Kleinstadt ({PopulationEstimate} Einw., {BuildingCount} Gebäude)",
        SettlementType.MediumTown => $"Mittelstadt ({PopulationEstimate} Einw., {BuildingCount} Gebäude)",
        SettlementType.City => $"Großstadt ({PopulationEstimate} Einw., {BuildingCount} Gebäude)",
        SettlementType.MetroArea => $"Metropolregion ({PopulationEstimate} Einw., {BuildingCount} Gebäude)",
        _ => "Unbekannt"
    };
}

public class OsmBuilding
{
    public long Id { get; set; }
    public Dictionary<string, string> Tags { get; set; } = new();
}

// Overpass API response models
internal class OverpassResult
{
    public List<OverpassElement> Elements { get; set; } = new();
}

internal class OverpassElement
{
    public long Id { get; set; }
    public Dictionary<string, string> Tags { get; set; } = new();
}
