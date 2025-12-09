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

    public async Task<T?> GetAsync<T>(string endpoint)
    {
        try
        {
            var response = await _httpClient.GetAsync(endpoint);
            response.EnsureSuccessStatusCode();
            return await response.Content.ReadFromJsonAsync<T>();
        }
        catch (Exception ex)
        {
            // Log error
            Console.WriteLine($"GET request failed: {ex.Message}");
            return default;
        }
    }

    public async Task<TResponse?> PostAsync<TRequest, TResponse>(string endpoint, TRequest data)
    {
        try
        {
            var json = JsonSerializer.Serialize(data);
            var content = new StringContent(json, Encoding.UTF8, "application/json");
            
            var response = await _httpClient.PostAsync(endpoint, content);
            response.EnsureSuccessStatusCode();
            
            return await response.Content.ReadFromJsonAsync<TResponse>();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"POST request failed: {ex.Message}");
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
        catch (Exception ex)
        {
            Console.WriteLine($"PUT request failed: {ex.Message}");
            return default;
        }
    }

    public async Task<bool> DeleteAsync(string endpoint)
    {
        try
        {
            var response = await _httpClient.DeleteAsync(endpoint);
            return response.IsSuccessStatusCode;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"DELETE request failed: {ex.Message}");
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
        catch (Exception ex)
        {
            Console.WriteLine($"AQL query execution failed: {ex.Message}");
            return new List<T>();
        }
    }

    public void Dispose()
    {
        _httpClient?.Dispose();
    }
}
