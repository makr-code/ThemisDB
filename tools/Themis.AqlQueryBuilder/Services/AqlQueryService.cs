/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AqlQueryService.cs                                 ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     109                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Themis.AqlQueryBuilder.Infrastructure;

namespace Themis.AqlQueryBuilder.Services;

/// <summary>
/// Implementation of AQL Query Service using HTTP REST API
/// </summary>
public class AqlQueryService : IAqlQueryService
{
    private readonly HttpClient _httpClient;
    private readonly string _serverUrl;

    public AqlQueryService(HttpClient httpClient, string serverUrl)
    {
        _httpClient = httpClient;
        _serverUrl = serverUrl;
    }

    public async Task<Result<string>> ExecuteQueryAsync(string aql, CancellationToken ct = default)
    {
        try
        {
            var requestBody = JsonSerializer.Serialize(new { query = aql });
            var content = new StringContent(requestBody, Encoding.UTF8, "application/json");
            
            var response = await _httpClient.PostAsync($"{_serverUrl}/api/query/aql", content, ct).ConfigureAwait(false);
            
            if (!response.IsSuccessStatusCode)
            {
                var errorContent = await response.Content.ReadAsStringAsync(ct).ConfigureAwait(false);
                return Result.Failure<string>($"Query failed: {response.StatusCode} - {errorContent}");
            }
            
            var result = await response.Content.ReadAsStringAsync(ct).ConfigureAwait(false);
            return Result.Success(result);
        }
        catch (TaskCanceledException)
        {
            return Result.Failure<string>("Query execution cancelled");
        }
        catch (HttpRequestException ex)
        {
            return Result.Failure<string>($"Connection error: {ex.Message}");
        }
        catch (Exception ex)
        {
            return Result.Failure<string>($"Unexpected error: {ex.Message}");
        }
    }

    public async Task<Result> TestConnectionAsync(CancellationToken ct = default)
    {
        try
        {
            var response = await _httpClient.GetAsync($"{_serverUrl}/api/health", ct).ConfigureAwait(false);
            
            if (response.IsSuccessStatusCode)
            {
                return Result.Success();
            }
            
            return Result.Failure($"Connection test failed: {response.StatusCode}");
        }
        catch (TaskCanceledException)
        {
            return Result.Failure("Connection test cancelled");
        }
        catch (HttpRequestException ex)
        {
            return Result.Failure($"Cannot connect to server: {ex.Message}");
        }
        catch (Exception ex)
        {
            return Result.Failure($"Unexpected error: {ex.Message}");
        }
    }
}
