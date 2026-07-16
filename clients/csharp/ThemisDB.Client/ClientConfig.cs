/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClientConfig.cs                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     144                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace ThemisDB.Client;

/// <summary>
/// Configuration for ThemisDB client
/// </summary>
public class ClientConfig
{
    /// <summary>
    /// Maximum number of retry attempts
    /// </summary>
    public int MaxRetries { get; set; } = 3;

    /// <summary>
    /// HTTP request timeout
    /// </summary>
    public TimeSpan Timeout { get; set; } = TimeSpan.FromSeconds(30);

    /// <summary>
    /// Circuit breaker configuration
    /// </summary>
    public CircuitBreakerConfig? CircuitBreaker { get; set; }

    /// <summary>
    /// Logging configuration
    /// </summary>
    public LoggingConfig? Logging { get; set; }

    /// <summary>
    /// Connection pool configuration
    /// </summary>
    public ConnectionPoolConfig? ConnectionPool { get; set; }

    /// <summary>
    /// Circuit breaker configuration
    /// </summary>
    public class CircuitBreakerConfig
    {
        /// <summary>
        /// Enable circuit breaker
        /// </summary>
        public bool Enabled { get; set; } = true;

        /// <summary>
        /// Number of failures before opening circuit
        /// </summary>
        public int FailureThreshold { get; set; } = 5;

        /// <summary>
        /// Time to wait before attempting reset
        /// </summary>
        public TimeSpan ResetTimeout { get; set; } = TimeSpan.FromSeconds(60);

        /// <summary>
        /// Maximum requests in half-open state
        /// </summary>
        public int HalfOpenMaxRequests { get; set; } = 3;
    }

    /// <summary>
    /// Logging configuration
    /// </summary>
    public class LoggingConfig
    {
        /// <summary>
        /// Enable logging
        /// </summary>
        public bool Enabled { get; set; } = true;

        /// <summary>
        /// Log HTTP requests
        /// </summary>
        public bool LogRequests { get; set; } = true;

        /// <summary>
        /// Log HTTP responses
        /// </summary>
        public bool LogResponses { get; set; } = true;

        /// <summary>
        /// Custom logger function
        /// </summary>
        public Action<string, LogLevel>? Logger { get; set; }
    }

    /// <summary>
    /// Connection pool configuration
    /// </summary>
    public class ConnectionPoolConfig
    {
        /// <summary>
        /// Maximum connections
        /// </summary>
        public int MaxConnections { get; set; } = 100;

        /// <summary>
        /// Maximum connections per endpoint
        /// </summary>
        public int MaxConnectionsPerEndpoint { get; set; } = 50;

        /// <summary>
        /// Connection idle timeout
        /// </summary>
        public TimeSpan IdleTimeout { get; set; } = TimeSpan.FromSeconds(30);

        /// <summary>
        /// Keep-alive timeout
        /// </summary>
        public TimeSpan KeepAliveTimeout { get; set; } = TimeSpan.FromSeconds(60);
    }

    /// <summary>
    /// Log levels
    /// </summary>
    public enum LogLevel
    {
        Info,
        Warning,
        Error
    }
}
