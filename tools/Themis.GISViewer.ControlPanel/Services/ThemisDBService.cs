/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisDBService.cs                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     126                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Net.Http;
using System.Net.Http.Json;
using Themis.AdminTools.Shared.ApiClient;

namespace Themis.GISViewer.ControlPanel.Services;

public interface IThemisDBService
{
    Task<bool> TestConnectionAsync();
    Task<T?> ExecuteAQLAsync<T>(string query, Dictionary<string, object>? parameters = null);
    Task<List<GeoBuilding>> QueryBuildingsAsync(double lat, double lon, double radiusKm);
}

public class ThemisDBService : IThemisDBService
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;

    public ThemisDBService(Microsoft.Extensions.Options.IOptions<ThemisDBConfiguration> config)
    {
        _baseUrl = config.Value.ApiUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromSeconds(config.Value.Timeout)
        };
    }

    public async Task<bool> TestConnectionAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/health");
            return response.IsSuccessStatusCode;
        }
        catch
        {
            return false;
        }
    }

    public async Task<T?> ExecuteAQLAsync<T>(string query, Dictionary<string, object>? parameters = null)
    {
        var request = new
        {
            query = query,
            bindVars = parameters ?? new Dictionary<string, object>()
        };

        var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
        response.EnsureSuccessStatusCode();

        return await response.Content.ReadFromJsonAsync<T>();
    }

    public async Task<List<GeoBuilding>> QueryBuildingsAsync(double lat, double lon, double radiusKm)
    {
        var aql = @"
            FOR building IN osm_buildings
              FILTER GEO_DISTANCE(
                building.location,
                GEO_POINT(@longitude, @latitude)
              ) <= @radius
              RETURN {
                id: building._key,
                footprint: building.footprint,
                height: building.height,
                type: building.tags.building
              }
        ";

        var parameters = new Dictionary<string, object>
        {
            { "latitude", lat },
            { "longitude", lon },
            { "radius", radiusKm * 1000 } // Convert to meters
        };

        var result = await ExecuteAQLAsync<AQLQueryResult<GeoBuilding>>(aql, parameters);
        return result?.Results ?? new List<GeoBuilding>();
    }
}

public class GeoBuilding
{
    public string Id { get; set; } = "";
    public List<GeoCoordinate> Footprint { get; set; } = new();
    public double Height { get; set; }
    public string Type { get; set; } = "";
}

public class GeoCoordinate
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double Altitude { get; set; }
}

public class AQLQueryResult<T>
{
    public List<T> Results { get; set; } = new();
    public bool HasMore { get; set; }
    public int Count { get; set; }
}
