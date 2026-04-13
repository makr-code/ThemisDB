/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ImpactAnalysisService.cs                           ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     151                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Net.Http;
using System.Net.Http.Json;
using System.Text.Json;
using Themis.ImpactAnalysisViewer.Models;

namespace Themis.ImpactAnalysisViewer.Services;

/// <summary>
/// Service for communicating with the GPU Impact Analysis Plugin
/// </summary>
public class ImpactAnalysisService
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;
    private readonly JsonSerializerOptions _jsonOptions;

    public ImpactAnalysisService(string baseUrl)
    {
        _baseUrl = baseUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(baseUrl),
            Timeout = TimeSpan.FromMinutes(5)
        };
        
        _jsonOptions = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            WriteIndented = true
        };
    }

    /// <summary>
    /// Analyze impact of a document change
    /// </summary>
    public async Task<ImpactAnalysisResult?> AnalyzeImpactAsync(
        DocumentChange change,
        Dictionary<string, object>? config = null)
    {
        var request = new
        {
            document_change = new
            {
                document_id = change.DocumentId,
                change_type = change.ChangeType,
                magnitude = change.Magnitude,
                source_layer = change.SourceLayer
            },
            config = config ?? new Dictionary<string, object>()
        };

        var response = await _httpClient.PostAsJsonAsync(
            "/api/analytics/gpu-impact/analyze",
            request,
            _jsonOptions);

        response.EnsureSuccessStatusCode();
        return await response.Content.ReadFromJsonAsync<ImpactAnalysisResult>(_jsonOptions);
    }

    /// <summary>
    /// Analyze multi-layer impact
    /// </summary>
    public async Task<ImpactAnalysisResult?> AnalyzeMultiLayerImpactAsync(
        DocumentChange change,
        string[] targetLayers,
        Dictionary<string, object>? config = null)
    {
        var request = new
        {
            document_change = new
            {
                document_id = change.DocumentId,
                change_type = change.ChangeType,
                magnitude = change.Magnitude,
                source_layer = change.SourceLayer
            },
            target_layers = targetLayers,
            config = config ?? new Dictionary<string, object>
            {
                ["fem"] = new
                {
                    enable_cross_layer_propagation = true,
                    layer_damping_factors = new Dictionary<string, double>
                    {
                        ["api"] = 0.95,
                        ["process"] = 0.85,
                        ["database"] = 0.80,
                        ["ui"] = 0.75
                    }
                }
            }
        };

        var response = await _httpClient.PostAsJsonAsync(
            "/api/analytics/gpu-impact/analyze-multi-layer",
            request,
            _jsonOptions);

        response.EnsureSuccessStatusCode();
        return await response.Content.ReadFromJsonAsync<ImpactAnalysisResult>(_jsonOptions);
    }

    /// <summary>
    /// Test connection to the plugin
    /// </summary>
    public async Task<bool> TestConnectionAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/analytics/gpu-impact/health");
            return response.IsSuccessStatusCode;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Get plugin information
    /// </summary>
    public async Task<Dictionary<string, object>?> GetPluginInfoAsync()
    {
        var response = await _httpClient.GetAsync("/api/analytics/gpu-impact/info");
        response.EnsureSuccessStatusCode();
        return await response.Content.ReadFromJsonAsync<Dictionary<string, object>>(_jsonOptions);
    }
}
