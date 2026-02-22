/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentLockCleanupService.cs                      ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     175                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Services;
using Timer = System.Timers.Timer;

namespace Themis.DocumentManager.Infrastructure.BackgroundJobs;

/// <summary>
/// Background Service für automatisches Cleanup abgelaufener Dokumenten-Sperren.
/// WPF-kompatible Timer-basierte Implementierung.
/// Phase 2 Sprint 5-6 - Background Jobs.
/// </summary>
public class DocumentLockCleanupService : IDisposable
{
    private readonly IDocumentLockingService _lockingService;
    private readonly ILogger<DocumentLockCleanupService> _logger;
    private readonly Timer _cleanupTimer;
    private readonly DocumentLockCleanupConfiguration _configuration;
    private bool _isRunning;

    public DocumentLockCleanupService(
        IDocumentLockingService lockingService,
        ILogger<DocumentLockCleanupService> logger,
        DocumentLockCleanupConfiguration? configuration = null)
    {
        _lockingService = lockingService;
        _logger = logger;
        _configuration = configuration ?? new DocumentLockCleanupConfiguration();

        _cleanupTimer = new Timer(_configuration.CleanupInterval.TotalMilliseconds);
        _cleanupTimer.Elapsed += OnCleanupTimerElapsed;
        _cleanupTimer.AutoReset = true;
    }

    /// <summary>
    /// Startet den Cleanup Service.
    /// </summary>
    public void Start()
    {
        if (_isRunning)
        {
            _logger.LogWarning("Document Lock Cleanup Service is already running");
            return;
        }

        if (!_configuration.Enabled)
        {
            _logger.LogInformation("Document Lock Cleanup Service is disabled in configuration");
            return;
        }

        _logger.LogInformation("Starting Document Lock Cleanup Service. Cleanup interval: {Interval}", 
            _configuration.CleanupInterval);

        _isRunning = true;
        _cleanupTimer.Start();

        // Optional: Initial cleanup nach InitialDelay
        if (_configuration.InitialDelay > TimeSpan.Zero)
        {
            Task.Delay(_configuration.InitialDelay).ContinueWith(async _ =>
            {
                if (_isRunning)
                {
                    await CleanupExpiredLocksAsync();
                }
            });
        }
    }

    /// <summary>
    /// Stoppt den Cleanup Service.
    /// </summary>
    public void Stop()
    {
        if (!_isRunning)
            return;

        _logger.LogInformation("Stopping Document Lock Cleanup Service");
        
        _cleanupTimer.Stop();
        _isRunning = false;
    }

    private async void OnCleanupTimerElapsed(object? sender, ElapsedEventArgs e)
    {
        if (!_isRunning)
            return;

        await CleanupExpiredLocksAsync();
    }

    private async Task CleanupExpiredLocksAsync()
    {
        try
        {
            _logger.LogDebug("Starting cleanup of expired locks");

            await _lockingService.CleanupExpiredLocksAsync();

            _logger.LogDebug("Lock cleanup completed successfully");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to cleanup expired locks");
            // Continue running despite errors
        }
    }

    public void Dispose()
    {
        Stop();
        _cleanupTimer?.Dispose();
    }
}

/// <summary>
/// Konfiguration für DocumentLockCleanupService.
/// </summary>
public class DocumentLockCleanupConfiguration
{
    /// <summary>
    /// Intervall zwischen Cleanup-Durchläufen.
    /// Default: 5 Minuten
    /// </summary>
    public TimeSpan CleanupInterval { get; set; } = TimeSpan.FromMinutes(5);

    /// <summary>
    /// Aktiviert/Deaktiviert den Cleanup Service.
    /// Default: true
    /// </summary>
    public bool Enabled { get; set; } = true;

    /// <summary>
    /// Maximale Anzahl an Locks die pro Cleanup-Zyklus entfernt werden.
    /// Default: 100 (0 = unbegrenzt)
    /// </summary>
    public int MaxLocksPerCycle { get; set; } = 100;

    /// <summary>
    /// Delay beim Service-Start bevor erster Cleanup durchgeführt wird.
    /// Default: 30 Sekunden
    /// </summary>
    public TimeSpan InitialDelay { get; set; } = TimeSpan.FromSeconds(30);
}

