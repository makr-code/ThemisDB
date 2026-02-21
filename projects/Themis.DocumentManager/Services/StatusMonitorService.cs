/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            StatusMonitorService.cs                            ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     244                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service zur kontinuierlichen Überwachung von ThemisDB und Ollama LLM Status
/// </summary>
public class StatusMonitorService : INotifyPropertyChanged
{
    private readonly IThemisApiClient _themisClient;
    private readonly IOllamaService _ollamaService;
    private Timer? _monitorTimer;
    
    private bool _isThemisDbOnline;
    private bool _isOllamaOnline;
    private string _themisDbStatus = "Prüfe...";
    private string _ollamaStatus = "Prüfe...";
    private DateTime _lastThemisCheck = DateTime.MinValue;
    private DateTime _lastOllamaCheck = DateTime.MinValue;

    public event PropertyChangedEventHandler? PropertyChanged;

    public StatusMonitorService(IThemisApiClient themisClient, IOllamaService ollamaService)
    {
        _themisClient = themisClient ?? throw new ArgumentNullException(nameof(themisClient));
        _ollamaService = ollamaService ?? throw new ArgumentNullException(nameof(ollamaService));
    }

    #region Properties

    public bool IsThemisDbOnline
    {
        get => _isThemisDbOnline;
        private set
        {
            if (_isThemisDbOnline != value)
            {
                _isThemisDbOnline = value;
                OnPropertyChanged();
                UpdateThemisDbStatus();
            }
        }
    }

    public bool IsOllamaOnline
    {
        get => _isOllamaOnline;
        private set
        {
            if (_isOllamaOnline != value)
            {
                _isOllamaOnline = value;
                OnPropertyChanged();
                UpdateOllamaStatus();
            }
        }
    }

    public string ThemisDbStatus
    {
        get => _themisDbStatus;
        private set
        {
            if (_themisDbStatus != value)
            {
                _themisDbStatus = value;
                OnPropertyChanged();
            }
        }
    }

    public string OllamaStatus
    {
        get => _ollamaStatus;
        private set
        {
            if (_ollamaStatus != value)
            {
                _ollamaStatus = value;
                OnPropertyChanged();
            }
        }
    }

    public DateTime LastThemisCheck
    {
        get => _lastThemisCheck;
        private set
        {
            _lastThemisCheck = value;
            OnPropertyChanged();
        }
    }

    public DateTime LastOllamaCheck
    {
        get => _lastOllamaCheck;
        private set
        {
            _lastOllamaCheck = value;
            OnPropertyChanged();
        }
    }

    #endregion

    /// <summary>
    /// Startet kontinuierliches Monitoring (alle 10 Sekunden)
    /// </summary>
    public void StartMonitoring(int intervalSeconds = 10)
    {
        // Initiale Prüfung
        _ = CheckStatusAsync();

        // Timer für regelmäßige Prüfungen
        _monitorTimer = new Timer(
            async _ => await CheckStatusAsync(),
            null,
            TimeSpan.FromSeconds(intervalSeconds),
            TimeSpan.FromSeconds(intervalSeconds)
        );
    }

    /// <summary>
    /// Stoppt das Monitoring
    /// </summary>
    public void StopMonitoring()
    {
        _monitorTimer?.Dispose();
        _monitorTimer = null;
    }

    /// <summary>
    /// Prüft Status von ThemisDB und Ollama
    /// </summary>
    public async Task CheckStatusAsync()
    {
        // ThemisDB prüfen
        await CheckThemisDbAsync();

        // Ollama prüfen
        await CheckOllamaAsync();
    }

    /// <summary>
    /// Prüft ThemisDB-Verfügbarkeit
    /// </summary>
    private async Task CheckThemisDbAsync()
    {
        try
        {
            var isOnline = await _themisClient.CheckHealthAsync();
            IsThemisDbOnline = isOnline;
            LastThemisCheck = DateTime.Now;
        }
        catch
        {
            IsThemisDbOnline = false;
            LastThemisCheck = DateTime.Now;
        }
    }

    /// <summary>
    /// Prüft Ollama LLM-Verfügbarkeit
    /// </summary>
    private async Task CheckOllamaAsync()
    {
        try
        {
            var models = await _ollamaService.GetAvailableModelsAsync();
            IsOllamaOnline = models.Count > 0;
            LastOllamaCheck = DateTime.Now;
        }
        catch
        {
            IsOllamaOnline = false;
            LastOllamaCheck = DateTime.Now;
        }
    }

    private void UpdateThemisDbStatus()
    {
        if (IsThemisDbOnline)
        {
            ThemisDbStatus = "🟢 ThemisDB: Online";
        }
        else
        {
            ThemisDbStatus = "🔴 ThemisDB: Offline";
        }
    }

    private void UpdateOllamaStatus()
    {
        if (IsOllamaOnline)
        {
            OllamaStatus = "🟢 Ollama: Online";
        }
        else
        {
            OllamaStatus = "🔴 Ollama: Offline";
        }
    }

    protected void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    public void Dispose()
    {
        StopMonitoring();
    }
}
