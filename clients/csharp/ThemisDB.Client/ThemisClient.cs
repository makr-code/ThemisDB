/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisClient.cs                                    ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     740                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Net.Http.Json;
using System.Text;
using System.Text.Json;

namespace ThemisDB.Client;

/// <summary>
/// Client for interacting with ThemisDB
/// </summary>
public class ThemisClient : IDisposable
{
    private readonly HttpClient _httpClient;
    private readonly List<string> _endpoints;
    private readonly SemaphoreSlim _endpointLock = new(1, 1);
    private int _currentEndpointIndex;
    private bool _disposed;
    private readonly int _maxRetries;
    private readonly CircuitBreaker? _circuitBreaker;
    private readonly bool _loggingEnabled;
    private readonly bool _logRequests;
    private readonly bool _logResponses;
    private readonly Action<string, ClientConfig.LogLevel>? _logger;

    /// <summary>
    /// Initializes a new instance of the <see cref="ThemisClient"/> class
    /// </summary>
    /// <param name="endpoints">List of ThemisDB server endpoints</param>
    /// <param name="timeout">Optional HTTP timeout</param>
    /// <exception cref="ArgumentException">Thrown when endpoints list is null or empty</exception>
    public ThemisClient(IEnumerable<string> endpoints, TimeSpan? timeout = null)
        : this(endpoints, null)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ThemisClient"/> class with full configuration
    /// </summary>
    /// <param name="endpoints">List of ThemisDB server endpoints</param>
    /// <param name="config">Client configuration</param>
    /// <exception cref="ArgumentException">Thrown when endpoints list is null or empty</exception>
    public ThemisClient(IEnumerable<string> endpoints, ClientConfig? config)
    {
        if (endpoints == null || !endpoints.Any())
        {
            throw new ArgumentException("At least one endpoint must be provided", nameof(endpoints));
        }

        _endpoints = endpoints.ToList();
        
        // Apply configuration or use defaults
        config ??= new ClientConfig();
        _maxRetries = config.MaxRetries;

        // Configure HTTP client with connection pooling
        var handler = new SocketsHttpHandler
        {
            PooledConnectionLifetime = config.ConnectionPool?.KeepAliveTimeout ?? TimeSpan.FromSeconds(60),
            PooledConnectionIdleTimeout = config.ConnectionPool?.IdleTimeout ?? TimeSpan.FromSeconds(30),
            MaxConnectionsPerServer = config.ConnectionPool?.MaxConnectionsPerEndpoint ?? 50
        };

        _httpClient = new HttpClient(handler)
        {
            Timeout = config.Timeout
        };

        // Initialize circuit breaker if enabled
        if (config.CircuitBreaker?.Enabled == true)
        {
            _circuitBreaker = new CircuitBreaker(
                config.CircuitBreaker.FailureThreshold,
                config.CircuitBreaker.ResetTimeout,
                config.CircuitBreaker.HalfOpenMaxRequests
            );
        }

        // Initialize logging
        if (config.Logging?.Enabled == true)
        {
            _loggingEnabled = true;
            _logRequests = config.Logging.LogRequests;
            _logResponses = config.Logging.LogResponses;
            _logger = config.Logging.Logger ?? DefaultLogger;
        }
    }

    /// <summary>
    /// Default logger that writes to console
    /// </summary>
    private static void DefaultLogger(string message, ClientConfig.LogLevel level)
    {
        Console.WriteLine($"[ThemisDB] [{level}] {message}");
    }

    /// <summary>
    /// Gets the circuit breaker state
    /// </summary>
    public string? GetCircuitBreakerState()
    {
        return _circuitBreaker?.State.ToString();
    }

    /// <summary>
    /// Gets the current endpoint being used
    /// </summary>
    public async Task<string> GetCurrentEndpointAsync()
    {
        await _endpointLock.WaitAsync();
        try
        {
            return _endpoints[_currentEndpointIndex];
        }
        finally
        {
            _endpointLock.Release();
        }
    }

    /// <summary>
    /// Rotates to the next available endpoint
    /// </summary>
    private async Task RotateEndpointAsync()
    {
        await _endpointLock.WaitAsync();
        try
        {
            _currentEndpointIndex = (_currentEndpointIndex + 1) % _endpoints.Count;
        }
        finally
        {
            _endpointLock.Release();
        }
    }

    /// <summary>
    /// Execute an HTTP request with retry logic, circuit breaker, and logging
    /// </summary>
    private async Task<HttpResponseMessage> ExecuteWithRetryAsync(
        Func<Task<HttpResponseMessage>> requestFunc,
        string method,
        string url,
        CancellationToken cancellationToken)
    {
        Exception? lastException = null;

        for (int attempt = 0; attempt <= _maxRetries; attempt++)
        {
            try
            {
                // Check circuit breaker
                if (_circuitBreaker != null && !await _circuitBreaker.CanExecuteAsync())
                {
                    throw new InvalidOperationException("Circuit breaker is OPEN");
                }

                // Log request
                if (_loggingEnabled && _logRequests)
                {
                    _logger?.Invoke($"{method} {url} (attempt {attempt + 1}/{_maxRetries + 1})", ClientConfig.LogLevel.Info);
                }

                // Execute request
                var response = await requestFunc();

                // Log response
                if (_loggingEnabled && _logResponses)
                {
                    _logger?.Invoke($"{method} {url} -> {(int)response.StatusCode} {response.StatusCode}", ClientConfig.LogLevel.Info);
                }

                // Record success in circuit breaker
                if (_circuitBreaker != null)
                {
                    await _circuitBreaker.RecordSuccessAsync();
                }

                response.EnsureSuccessStatusCode();
                return response;
            }
            catch (Exception ex)
            {
                lastException = ex;

                // Record failure in circuit breaker
                if (_circuitBreaker != null)
                {
                    await _circuitBreaker.RecordFailureAsync();
                }

                // Log error
                if (_loggingEnabled)
                {
                    _logger?.Invoke($"{method} {url} failed: {ex.Message}", ClientConfig.LogLevel.Error);
                }

                // Don't retry if circuit breaker is open or if it's the last attempt
                if (attempt >= _maxRetries || (_circuitBreaker != null && _circuitBreaker.State == CircuitBreaker.CircuitBreakerState.Open))
                {
                    break;
                }

                // Exponential backoff
                var delay = TimeSpan.FromMilliseconds(Math.Pow(2, attempt) * 100);
                await Task.Delay(delay, cancellationToken);
            }
        }

        throw lastException ?? new Exception("Request failed after all retry attempts");
    }

    /// <summary>
    /// Gets a document from the database
    /// </summary>
    /// <typeparam name="T">Type of the document</typeparam>
    /// <param name="model">Model name (e.g., "relational", "document", "graph")</param>
    /// <param name="collection">Collection name</param>
    /// <param name="uuid">Document UUID</param>
    /// <param name="cancellationToken">Cancellation token</param>
    /// <returns>The retrieved document</returns>
    public async Task<T?> GetAsync<T>(string model, string collection, string uuid, CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/api/{model}/{collection}/{uuid}";
        
        var response = await ExecuteWithRetryAsync(
            () => _httpClient.GetAsync(url, cancellationToken),
            "GET",
            url,
            cancellationToken);
        
        return await response.Content.ReadFromJsonAsync<T>(cancellationToken: cancellationToken);
    }

    /// <summary>
    /// Puts (creates or updates) a document in the database
    /// </summary>
    /// <typeparam name="T">Type of the document</typeparam>
    /// <param name="model">Model name</param>
    /// <param name="collection">Collection name</param>
    /// <param name="uuid">Document UUID</param>
    /// <param name="data">Document data</param>
    /// <param name="cancellationToken">Cancellation token</param>
    public async Task PutAsync<T>(string model, string collection, string uuid, T data, CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/api/{model}/{collection}/{uuid}";
        
        await ExecuteWithRetryAsync(
            () => _httpClient.PutAsJsonAsync(url, data, cancellationToken),
            "PUT",
            url,
            cancellationToken);
    }

    /// <summary>
    /// Deletes a document from the database
    /// </summary>
    /// <param name="model">Model name</param>
    /// <param name="collection">Collection name</param>
    /// <param name="uuid">Document UUID</param>
    /// <param name="cancellationToken">Cancellation token</param>
    public async Task DeleteAsync(string model, string collection, string uuid, CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/api/{model}/{collection}/{uuid}";
        
        await ExecuteWithRetryAsync(
            () => _httpClient.DeleteAsync(url, cancellationToken),
            "DELETE",
            url,
            cancellationToken);
    }

    /// <summary>
    /// Executes an AQL query
    /// </summary>
    /// <typeparam name="T">Type of the query results</typeparam>
    /// <param name="aql">AQL query string</param>
    /// <param name="cancellationToken">Cancellation token</param>
    /// <returns>Query results</returns>
    public async Task<List<T>> QueryAsync<T>(string aql, CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/query";
        
        var content = new StringContent(
            JsonSerializer.Serialize(new { aql }),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        return await response.Content.ReadFromJsonAsync<List<T>>(cancellationToken: cancellationToken) ?? new List<T>();
    }

    /// <summary>
    /// Begins a new transaction
    /// </summary>
    /// <param name="options">Transaction options</param>
    /// <param name="cancellationToken">Cancellation token</param>
    /// <returns>A new transaction instance</returns>
    public async Task<Transaction> BeginTransactionAsync(TransactionOptions? options = null, CancellationToken cancellationToken = default)
    {
        options ??= new TransactionOptions();
        
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/transaction/begin";
        
        var content = new StringContent(
            JsonSerializer.Serialize(new 
            { 
                isolation_level = options.IsolationLevel.ToString().ToUpperInvariant(),
                timeout = options.Timeout?.TotalSeconds 
            }),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        var result = await response.Content.ReadFromJsonAsync<TransactionBeginResponse>(cancellationToken: cancellationToken);
        if (result == null || string.IsNullOrEmpty(result.TransactionId))
        {
            throw new InvalidOperationException("Failed to begin transaction");
        }
        
        return new Transaction(this, result.TransactionId, _httpClient);
    }

    /// <summary>
    /// Disposes the client and releases resources
    /// </summary>
    public void Dispose()
    {
        if (_disposed) return;
        
        _httpClient?.Dispose();
        _endpointLock?.Dispose();
        _disposed = true;
        
        GC.SuppressFinalize(this);
    }

    // ==================== Graph API ====================

    /// <summary>
    /// Traverses a graph starting from a node
    /// </summary>
    public async Task<GraphTraverseResult> GraphTraverseAsync(
        string startNode,
        int maxDepth = 3,
        string? edgeType = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/graph/traverse";
        
        var body = new Dictionary<string, object>
        {
            ["start"] = startNode,
            ["max_depth"] = maxDepth
        };
        if (!string.IsNullOrEmpty(edgeType))
        {
            body["edge_type"] = edgeType;
        }
        
        var content = new StringContent(
            JsonSerializer.Serialize(body),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        return await response.Content.ReadFromJsonAsync<GraphTraverseResult>(cancellationToken: cancellationToken) 
            ?? new GraphTraverseResult();
    }

    /// <summary>
    /// Finds the shortest path between two nodes
    /// </summary>
    public async Task<List<string>> ShortestPathAsync(
        string from,
        string to,
        string? edgeType = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/graph/shortest-path";
        
        var body = new Dictionary<string, object>
        {
            ["from"] = from,
            ["to"] = to
        };
        if (!string.IsNullOrEmpty(edgeType))
        {
            body["edge_type"] = edgeType;
        }
        
        var content = new StringContent(
            JsonSerializer.Serialize(body),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        var result = await response.Content.ReadFromJsonAsync<ShortestPathResult>(cancellationToken: cancellationToken);
        return result?.Path ?? new List<string>();
    }

    /// <summary>
    /// Gets the neighbors of a node
    /// </summary>
    public async Task<List<string>> NeighborsAsync(
        string nodeId,
        string? direction = null,
        string? edgeType = null,
        int? limit = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/graph/neighbors";
        
        var body = new Dictionary<string, object>
        {
            ["node"] = nodeId
        };
        if (!string.IsNullOrEmpty(direction))
        {
            body["direction"] = direction;
        }
        if (!string.IsNullOrEmpty(edgeType))
        {
            body["edge_type"] = edgeType;
        }
        if (limit.HasValue)
        {
            body["limit"] = limit.Value;
        }
        
        var content = new StringContent(
            JsonSerializer.Serialize(body),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        var result = await response.Content.ReadFromJsonAsync<NeighborsResult>(cancellationToken: cancellationToken);
        return result?.Neighbors ?? new List<string>();
    }

    // ==================== Vector API ====================

    /// <summary>
    /// Performs a vector similarity search
    /// </summary>
    public async Task<VectorSearchResult> VectorSearchAsync(
        double[] embedding,
        int topK = 10,
        Dictionary<string, object>? filter = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/vector/search";
        
        var body = new Dictionary<string, object>
        {
            ["vector"] = embedding,
            ["k"] = topK
        };
        if (filter != null)
        {
            body["filter"] = filter;
        }
        
        var content = new StringContent(
            JsonSerializer.Serialize(body),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        return await response.Content.ReadFromJsonAsync<VectorSearchResult>(cancellationToken: cancellationToken) 
            ?? new VectorSearchResult();
    }

    /// <summary>
    /// Upserts a vector with metadata
    /// </summary>
    public async Task VectorUpsertAsync(
        string id,
        double[] embedding,
        Dictionary<string, object>? metadata = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/vector/upsert";
        
        var body = new Dictionary<string, object>
        {
            ["id"] = id,
            ["vector"] = embedding
        };
        if (metadata != null)
        {
            body["metadata"] = metadata;
        }
        
        var content = new StringContent(
            JsonSerializer.Serialize(body),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
    }

    /// <summary>
    /// Deletes a vector by ID
    /// </summary>
    public async Task VectorDeleteAsync(string id, CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/vector/{id}";
        
        var response = await _httpClient.DeleteAsync(url, cancellationToken);
        response.EnsureSuccessStatusCode();
    }

    private class TransactionBeginResponse
    {
        public string TransactionId { get; set; } = string.Empty;
    }
    
    // ==================== LLM API ====================
    
    /// <summary>
    /// Creates an LLM interaction
    /// </summary>
    /// <param name="model">LLM model name (e.g., "gpt-4o", "llama-3.1")</param>
    /// <param name="messages">List of conversation messages</param>
    /// <param name="reasoningSteps">Optional reasoning steps</param>
    /// <param name="metadata">Optional metadata</param>
    /// <param name="cancellationToken">Cancellation token</param>
    /// <returns>Result containing interaction ID and success status</returns>
    public async Task<Llm.LlmInteractionResult> LlmInteractionAsync(
        string model,
        List<Llm.LlmMessage> messages,
        List<Llm.ReasoningStep>? reasoningSteps = null,
        Dictionary<string, object>? metadata = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/llm/interaction";
        
        var body = new Dictionary<string, object>
        {
            ["model"] = model,
            ["messages"] = messages
        };
        
        if (reasoningSteps != null)
        {
            body["reasoning_steps"] = reasoningSteps;
        }
        
        if (metadata != null)
        {
            body["metadata"] = metadata;
        }
        
        var content = new StringContent(
            JsonSerializer.Serialize(body),
            Encoding.UTF8,
            "application/json"
        );
        
        var response = await _httpClient.PostAsync(url, content, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        return await response.Content.ReadFromJsonAsync<Llm.LlmInteractionResult>(cancellationToken: cancellationToken)
            ?? new Llm.LlmInteractionResult();
    }
    
    /// <summary>
    /// Gets a specific LLM interaction by ID
    /// </summary>
    /// <param name="interactionId">The interaction ID</param>
    /// <param name="cancellationToken">Cancellation token</param>
    /// <returns>The LLM interaction or null if not found</returns>
    public async Task<Llm.LlmInteraction?> GetLlmInteractionAsync(
        string interactionId,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var url = $"{endpoint}/llm/interaction/{interactionId}";
        
        var response = await _httpClient.GetAsync(url, cancellationToken);
        
        if (response.StatusCode == System.Net.HttpStatusCode.NotFound)
        {
            return null;
        }
        
        response.EnsureSuccessStatusCode();
        
        return await response.Content.ReadFromJsonAsync<Llm.LlmInteraction>(cancellationToken: cancellationToken);
    }
    
    /// <summary>
    /// Lists LLM interactions with optional filtering
    /// </summary>
    /// <param name="model">Optional model name filter</param>
    /// <param name="limit">Maximum number of results</param>
    /// <param name="offset">Result offset for pagination</param>
    /// <param name="cancellationToken">Cancellation token</param>
    /// <returns>List of LLM interactions</returns>
    public async Task<List<Llm.LlmInteraction>> ListLlmInteractionsAsync(
        string? model = null,
        int? limit = null,
        int? offset = null,
        CancellationToken cancellationToken = default)
    {
        var endpoint = await GetCurrentEndpointAsync();
        var queryParams = new List<string>();
        
        if (!string.IsNullOrEmpty(model))
        {
            queryParams.Add($"model={Uri.EscapeDataString(model)}");
        }
        
        if (limit.HasValue)
        {
            queryParams.Add($"limit={limit.Value}");
        }
        
        if (offset.HasValue)
        {
            queryParams.Add($"offset={offset.Value}");
        }
        
        var url = $"{endpoint}/llm/interaction";
        if (queryParams.Any())
        {
            url += "?" + string.Join("&", queryParams);
        }
        
        var response = await _httpClient.GetAsync(url, cancellationToken);
        response.EnsureSuccessStatusCode();
        
        var result = await response.Content.ReadFromJsonAsync<LlmInteractionsResponse>(cancellationToken: cancellationToken);
        return result?.Interactions ?? new List<Llm.LlmInteraction>();
    }
    
    private class LlmInteractionsResponse
    {
        [JsonPropertyName("interactions")]
        public List<Llm.LlmInteraction> Interactions { get; set; } = new();
    }
}

/// <summary>
/// Result of a graph traversal
/// </summary>
public class GraphTraverseResult
{
    public List<string> Nodes { get; set; } = new();
    public List<string> Visited { get; set; } = new();
}

/// <summary>
/// Result of a shortest path query
/// </summary>
public class ShortestPathResult
{
    public List<string> Path { get; set; } = new();
}

/// <summary>
/// Result of a neighbors query
/// </summary>
public class NeighborsResult
{
    public List<string> Neighbors { get; set; } = new();
}

/// <summary>
/// Result of a vector search
/// </summary>
public class VectorSearchResult
{
    public List<VectorSearchHit> Results { get; set; } = new();
}

/// <summary>
/// A single hit from a vector search
/// </summary>
public class VectorSearchHit
{
    public string Id { get; set; } = string.Empty;
    public double Score { get; set; }
    public double? Distance { get; set; }
    public Dictionary<string, object>? Metadata { get; set; }
}
