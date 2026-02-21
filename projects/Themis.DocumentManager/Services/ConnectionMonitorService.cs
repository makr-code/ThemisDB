/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ConnectionMonitorService.cs                        ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     170                                            ║
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
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

#nullable enable

/// <summary>
/// Service for monitoring ThemisDB connection status and quality
/// </summary>
public interface IConnectionMonitorService
{
    Task<ConnectionStatus> GetStatusAsync(CancellationToken cancellationToken = default);
    Task<int> MeasureLatencyAsync(CancellationToken cancellationToken = default);
    event EventHandler<ConnectionStatus>? StatusChanged;
}

public class ConnectionMonitorService : IConnectionMonitorService, IDisposable
{
    private readonly IThemisDbClient _db;
    private readonly ILogger<ConnectionMonitorService> _logger;
    private readonly Timer _monitorTimer;
    private ConnectionStatus _currentStatus;
    
    public event EventHandler<ConnectionStatus>? StatusChanged;
    
    public ConnectionMonitorService(IThemisDbClient db, ILogger<ConnectionMonitorService> logger)
    {
        _db = db ?? throw new ArgumentNullException(nameof(db));
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        
        _currentStatus = new ConnectionStatus
        {
            State = ConnectionState.Disconnected,
            Quality = ConnectionQuality.Poor,
            LastChecked = DateTime.UtcNow
        };
        
        // Monitor connection every 30 seconds
        _monitorTimer = new Timer(
            _ => MonitorConnectionAsync().GetAwaiter().GetResult(),
            null,
            TimeSpan.Zero,
            TimeSpan.FromSeconds(30)
        );
    }
    
    public Task<ConnectionStatus> GetStatusAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(_currentStatus);
    }
    
    public async Task<int> MeasureLatencyAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var start = DateTime.UtcNow;
            
            // Simple ping query
            var query = "RETURN 1";
            await _db.QueryAsync<int>(query, null, cancellationToken);
            
            var latency = (int)(DateTime.UtcNow - start).TotalMilliseconds;
            return latency;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error measuring latency");
            return int.MaxValue;
        }
    }
    
    private async Task MonitorConnectionAsync()
    {
        try
        {
            var newStatus = new ConnectionStatus
            {
                LastChecked = DateTime.UtcNow
            };
            
            // Measure latency
            var latency = await MeasureLatencyAsync();
            newStatus.Latency = latency;
            
            if (latency == int.MaxValue)
            {
                newStatus.State = ConnectionState.Disconnected;
                newStatus.Quality = ConnectionQuality.Poor;
                newStatus.ErrorMessage = "Connection failed";
            }
            else
            {
                newStatus.State = ConnectionState.Connected;
                
                // Determine quality based on latency
                newStatus.Quality = latency switch
                {
                    < 50 => ConnectionQuality.Excellent,
                    < 150 => ConnectionQuality.Good,
                    < 500 => ConnectionQuality.Fair,
                    _ => ConnectionQuality.Poor
                };
                
                // Get server version
                try
                {
                    var versionQuery = "RETURN DATABASE_VERSION()";
                    var result = await _db.QueryAsync<string>(versionQuery);
                    newStatus.ServerVersion = await result.FirstOrDefaultAsync() ?? "Unknown";
                }
                catch
                {
                    newStatus.ServerVersion = "Unknown";
                }
            }
            
            // Check if status changed
            if (newStatus.State != _currentStatus.State ||
                newStatus.Quality != _currentStatus.Quality)
            {
                _logger.LogInformation("Connection status changed: {State}, Quality: {Quality}, Latency: {Latency}ms",
                    newStatus.State, newStatus.Quality, newStatus.Latency);
                
                StatusChanged?.Invoke(this, newStatus);
            }
            
            _currentStatus = newStatus;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error monitoring connection");
        }
    }
    
    public void Dispose()
    {
        _monitorTimer?.Dispose();
    }
}
