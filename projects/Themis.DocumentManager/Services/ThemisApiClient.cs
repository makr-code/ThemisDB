/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisApiClient.cs                                 ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     257                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// ThemisDB REST API Client implementation
/// </summary>
public class ThemisApiClient : IThemisApiClient, IDisposable
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;
    private string? _authToken;

    public ThemisApiClient()
    {
        _baseUrl = "http://localhost:8765"; // Default ThemisDB server
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromSeconds(30)
        };
    }

    public ThemisApiClient(string baseUrl)
    {
        _baseUrl = baseUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromSeconds(30)
        };
    }

    /// <summary>
    /// Prüft Health-Status der ThemisDB API
    /// </summary>
    public async Task<bool> CheckHealthAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var response = await _httpClient.GetAsync("/health", cancellationToken);
            return response.IsSuccessStatusCode;
        }
        catch
        {
            return false;
        }
    }

    public async Task<T?> GetAsync<T>(string endpoint, CancellationToken cancellationToken = default)
    {
        try
        {
            using var request = new HttpRequestMessage(HttpMethod.Get, endpoint);
            if (!string.IsNullOrEmpty(_authToken))
                request.Headers.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", _authToken);
            
            var response = await _httpClient.SendAsync(request, cancellationToken);
            response.EnsureSuccessStatusCode();
            return await response.Content.ReadFromJsonAsync<T>(cancellationToken: cancellationToken);
        }
        catch (HttpRequestException)
        {
            // Server connection error - silent fail
            return default;
        }
        catch (TaskCanceledException)
        {
            // Timeout - silent fail
            return default;
        }
        catch (Exception)
        {
            // Other errors
            return default;
        }
    }

    public async Task<TResponse?> PostAsync<TRequest, TResponse>(string endpoint, TRequest data, CancellationToken cancellationToken = default)
    {
        try
        {
            var json = JsonSerializer.Serialize(data);
            var content = new StringContent(json, Encoding.UTF8, "application/json");
            
            using var request = new HttpRequestMessage(HttpMethod.Post, endpoint) { Content = content };
            if (!string.IsNullOrEmpty(_authToken))
                request.Headers.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", _authToken);
            
            var response = await _httpClient.SendAsync(request, cancellationToken);
            response.EnsureSuccessStatusCode();
            
            return await response.Content.ReadFromJsonAsync<TResponse>(cancellationToken: cancellationToken);
        }
        catch (HttpRequestException)
        {
            // Server connection error - silent fail for optional operations
            return default;
        }
        catch (TaskCanceledException)
        {
            // Timeout - silent fail for optional operations
            return default;
        }
        catch (Exception)
        {
            // Other errors (parse, serialize, etc.)
            return default;
        }
    }

    public async Task<TResponse?> PutAsync<TRequest, TResponse>(string endpoint, TRequest data, CancellationToken cancellationToken = default)
    {
        try
        {
            var json = JsonSerializer.Serialize(data);
            var content = new StringContent(json, Encoding.UTF8, "application/json");
            
            var response = await _httpClient.PutAsync(endpoint, content, cancellationToken);
            response.EnsureSuccessStatusCode();
            
            return await response.Content.ReadFromJsonAsync<TResponse>(cancellationToken: cancellationToken);
        }
        catch (HttpRequestException)
        {
            // Server connection error
            return default;
        }
        catch (TaskCanceledException)
        {
            // Timeout
            return default;
        }
        catch (Exception)
        {
            // Other errors
            return default;
        }
    }

    public async Task<bool> DeleteAsync(string endpoint, CancellationToken cancellationToken = default)
    {
        try
        {
            using var request = new HttpRequestMessage(HttpMethod.Delete, endpoint);
            if (!string.IsNullOrEmpty(_authToken))
                request.Headers.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", _authToken);
            
            var response = await _httpClient.SendAsync(request, cancellationToken);
            return response.IsSuccessStatusCode;
        }
        catch (HttpRequestException)
        {
            // Server connection error
            return false;
        }
        catch (TaskCanceledException)
        {
            // Timeout
            return false;
        }
        catch (Exception)
        {
            // Other errors
            return false;
        }
    }

    public async Task<List<T>> ExecuteAqlAsync<T>(string query, object? bindVars = null, CancellationToken cancellationToken = default)
    {
        try
        {
            var payload = new
            {
                query = query,
                bindVars = bindVars ?? new { }
            };

            var response = await _httpClient.PostAsJsonAsync("/query", payload, cancellationToken);
            response.EnsureSuccessStatusCode();
            
            var result = await response.Content.ReadFromJsonAsync<dynamic>(cancellationToken: cancellationToken);
            
            // Extract results from response
            var resultList = new List<T>();
            if (result?.result is System.Text.Json.JsonElement element)
            {
                foreach (var item in element.EnumerateArray())
                {
                    var value = JsonSerializer.Deserialize<T>(item.GetRawText());
                    if (value != null)
                        resultList.Add(value);
                }
            }
            
            return resultList;
        }
        catch (HttpRequestException)
        {
            // Server connection error
            return new List<T>();
        }
        catch (TaskCanceledException)
        {
            // Timeout
            return new List<T>();
        }
        catch (Exception)
        {
            // Parse or serialize errors
            return new List<T>();
        }
    }

    public void SetAuthToken(string? token)
    {
        _authToken = token;
    }

    public void Dispose()
    {
        _httpClient?.Dispose();
    }
}
