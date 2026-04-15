/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CircuitBreaker.cs                                  ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     172                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace ThemisDB.Client;

/// <summary>
/// Circuit breaker implementation for fault tolerance
/// Prevents cascading failures by temporarily blocking requests when a service is experiencing issues
/// </summary>
public class CircuitBreaker
{
    private readonly int _failureThreshold;
    private readonly TimeSpan _resetTimeout;
    private readonly int _halfOpenMaxRequests;
    private readonly SemaphoreSlim _lock = new(1, 1);

    private CircuitBreakerState _state = CircuitBreakerState.Closed;
    private int _failureCount;
    private int _successCount;
    private DateTime _nextAttemptTime;

    /// <summary>
    /// Circuit breaker states
    /// </summary>
    public enum CircuitBreakerState
    {
        Closed,
        Open,
        HalfOpen
    }

    /// <summary>
    /// Initializes a new circuit breaker
    /// </summary>
    public CircuitBreaker(int failureThreshold, TimeSpan resetTimeout, int halfOpenMaxRequests)
    {
        _failureThreshold = failureThreshold;
        _resetTimeout = resetTimeout;
        _halfOpenMaxRequests = halfOpenMaxRequests;
        _nextAttemptTime = DateTime.MinValue;
    }

    /// <summary>
    /// Current state of the circuit breaker
    /// </summary>
    public CircuitBreakerState State => _state;

    /// <summary>
    /// Check if a request can be executed
    /// </summary>
    public async Task<bool> CanExecuteAsync()
    {
        await _lock.WaitAsync();
        try
        {
            if (_state == CircuitBreakerState.Closed)
            {
                return true;
            }

            if (_state == CircuitBreakerState.Open)
            {
                if (DateTime.UtcNow >= _nextAttemptTime)
                {
                    _state = CircuitBreakerState.HalfOpen;
                    _successCount = 0;
                    return true;
                }
                return false;
            }

            // Half-open state
            return _successCount < _halfOpenMaxRequests;
        }
        finally
        {
            _lock.Release();
        }
    }

    /// <summary>
    /// Record a successful request
    /// </summary>
    public async Task RecordSuccessAsync()
    {
        await _lock.WaitAsync();
        try
        {
            _failureCount = 0;

            if (_state == CircuitBreakerState.HalfOpen)
            {
                _successCount++;
                if (_successCount >= _halfOpenMaxRequests)
                {
                    _state = CircuitBreakerState.Closed;
                }
            }
        }
        finally
        {
            _lock.Release();
        }
    }

    /// <summary>
    /// Record a failed request
    /// </summary>
    public async Task RecordFailureAsync()
    {
        await _lock.WaitAsync();
        try
        {
            _failureCount++;
            _successCount = 0;

            if (_failureCount >= _failureThreshold)
            {
                _state = CircuitBreakerState.Open;
                _nextAttemptTime = DateTime.UtcNow.Add(_resetTimeout);
            }
        }
        finally
        {
            _lock.Release();
        }
    }

    /// <summary>
    /// Execute a function with circuit breaker protection
    /// </summary>
    public async Task<T> ExecuteAsync<T>(Func<Task<T>> action)
    {
        if (!await CanExecuteAsync())
        {
            throw new InvalidOperationException("Circuit breaker is OPEN");
        }

        try
        {
            var result = await action();
            await RecordSuccessAsync();
            return result;
        }
        catch
        {
            await RecordFailureAsync();
            throw;
        }
    }
}
