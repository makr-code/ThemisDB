/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            PollyHttpResilienceService.cs                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;
using Polly;
using Polly.CircuitBreaker;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Advanced HTTP Error Handling mit Polly Resilience Patterns
    /// </summary>
    public interface IHttpResilienceService
    {
        Task<T> ExecuteAsync<T>(Func<Task<T>> operation, string operationName);
        Task<HttpResponseMessage> PostWithResilienceAsync(string url, HttpContent content);
        Task<HttpResponseMessage> GetWithResilienceAsync(string url);
    }

    public class PollyHttpResilienceService : IHttpResilienceService
    {
        private readonly HttpClient _httpClient;
        private readonly ILoggerService _loggerService;
        private readonly IAsyncPolicy<HttpResponseMessage> _retryPolicy;
        private readonly IAsyncPolicy<HttpResponseMessage> _circuitBreakerPolicy;
        private readonly IAsyncPolicy<HttpResponseMessage> _combinedPolicy;

        public PollyHttpResilienceService(HttpClient httpClient, ILoggerService loggerService)
        {
            _httpClient = httpClient;
            _loggerService = loggerService;

            // Retry Policy: 3x mit exponential backoff (1s, 2s, 4s)
            _retryPolicy = Policy
                .Handle<HttpRequestException>()
                .Or<TaskCanceledException>()
                .OrResult<HttpResponseMessage>(r => 
                    r.StatusCode == System.Net.HttpStatusCode.RequestTimeout ||
                    r.StatusCode == System.Net.HttpStatusCode.BadGateway ||
                    r.StatusCode == System.Net.HttpStatusCode.ServiceUnavailable)
                .WaitAndRetryAsync(
                    retryCount: 3,
                    sleepDurationProvider: retryAttempt =>
                        TimeSpan.FromSeconds(Math.Pow(2, retryAttempt)),
                    onRetry: (outcome, timespan, retryCount, context) =>
                    {
                        _loggerService.LogWarning(
                            $"Retry {retryCount} after {timespan.TotalSeconds}s: {outcome.Exception?.Message ?? outcome.Result?.StatusCode.ToString()}");
                    });

            // Circuit Breaker: Bricht nach 5 Fehlern für 30s ab
            _circuitBreakerPolicy = Policy
                .Handle<HttpRequestException>()
                .Or<TaskCanceledException>()
                .OrResult<HttpResponseMessage>(r => !r.IsSuccessStatusCode)
                .CircuitBreakerAsync<HttpResponseMessage>(
                    handledEventsAllowedBeforeBreaking: 5,
                    durationOfBreak: TimeSpan.FromSeconds(30),
                    onBreak: (outcome, timespan) =>
                    {
                        _loggerService.LogError(
                            $"Circuit breaker opened for {timespan.TotalSeconds}s due to: {outcome.Exception?.Message ?? outcome.Result?.StatusCode.ToString()}");
                    },
                    onReset: () =>
                    {
                        _loggerService.LogInfo("Circuit breaker reset");
                    });

            // Kombiniere Retry + CircuitBreaker
            _combinedPolicy = Policy.WrapAsync(_retryPolicy, _circuitBreakerPolicy);
        }

        /// <summary>
        /// Generische Operation mit Resilience Handling
        /// </summary>
        public async Task<T> ExecuteAsync<T>(Func<Task<T>> operation, string operationName)
        {
            try
            {
                _loggerService.LogInfo($"Executing: {operationName}");
                
                var result = await Policy
                    .Handle<Exception>()
                    .WaitAndRetryAsync(
                        retryCount: 2,
                        sleepDurationProvider: attempt => TimeSpan.FromSeconds(Math.Pow(2, attempt)),
                        onRetry: (exception, timespan, retryCount, context) =>
                        {
                            _loggerService.LogWarning(
                                $"Retry {retryCount} for {operationName} after {timespan.TotalSeconds}s: {exception.Message}");
                        })
                    .ExecuteAsync(operation);

                _loggerService.LogInfo($"Completed: {operationName}");
                return result;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Failed: {operationName} - {ex.Message}");
                throw;
            }
        }

        /// <summary>
        /// POST mit Retry + Circuit Breaker
        /// </summary>
        public async Task<HttpResponseMessage> PostWithResilienceAsync(string url, HttpContent content)
        {
            try
            {
                return await _combinedPolicy.ExecuteAsync(async () =>
                {
                    var request = new HttpRequestMessage(HttpMethod.Post, url)
                    {
                        Content = content
                    };
                    return await _httpClient.SendAsync(request);
                });
            }
            catch (BrokenCircuitException ex)
            {
                _loggerService.LogError($"Circuit breaker is open for POST {url}: {ex.Message}");
                throw;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"POST {url} failed: {ex.Message}");
                throw;
            }
        }

        /// <summary>
        /// GET mit Retry + Circuit Breaker
        /// </summary>
        public async Task<HttpResponseMessage> GetWithResilienceAsync(string url)
        {
            try
            {
                return await _combinedPolicy.ExecuteAsync(async () =>
                    await _httpClient.GetAsync(url));
            }
            catch (BrokenCircuitException ex)
            {
                _loggerService.LogError($"Circuit breaker is open for GET {url}: {ex.Message}");
                throw;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"GET {url} failed: {ex.Message}");
                throw;
            }
        }
    }

    /// <summary>
    /// Fault Tolerance Helper für graceful degradation
    /// </summary>
    public static class FaultToleranceExtensions
    {
        /// <summary>
        /// Führt Operation mit Fallback aus
        /// </summary>
        public static async Task<T> ExecuteWithFallbackAsync<T>(
            Func<Task<T>> primary,
            Func<Task<T>> fallback,
            ILoggerService logger,
            string operationName)
        {
            try
            {
                return await primary();
            }
            catch (Exception ex)
            {
                logger.LogWarning($"{operationName} primary failed, using fallback: {ex.Message}");
                try
                {
                    return await fallback();
                }
                catch (Exception fallbackEx)
                {
                    logger.LogError($"{operationName} fallback also failed: {fallbackEx.Message}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Retry mit exponential backoff
        /// </summary>
        public static async Task<T> RetryAsync<T>(
            Func<Task<T>> operation,
            int maxRetries,
            ILoggerService logger,
            string operationName)
        {
            int attemptNumber = 0;
            while (true)
            {
                try
                {
                    return await operation();
                }
                catch (Exception ex) when (++attemptNumber < maxRetries)
                {
                    var delay = TimeSpan.FromSeconds(Math.Pow(2, attemptNumber));
                    logger.LogWarning($"{operationName} attempt {attemptNumber} failed, retrying in {delay.TotalSeconds}s: {ex.Message}");
                    await Task.Delay(delay);
                }
            }
        }
    }
}
