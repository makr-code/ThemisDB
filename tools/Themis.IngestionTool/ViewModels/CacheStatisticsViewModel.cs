/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CacheStatisticsViewModel.cs                        ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:57:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     227                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.ObjectModel;
using System.Windows.Input;
using System.Windows.Threading;
using Themis.IngestionTool.Services;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.ViewModels
{
    /// <summary>
    /// ViewModel für Cache Statistiken und Performance Monitoring
    /// </summary>
    public class CacheStatisticsViewModel : ViewModelBase
    {
        private readonly ICacheService _cacheService;
        private readonly ILoggerService _loggerService;
        private readonly DispatcherTimer _refreshTimer;

        // Cache Statistics
        private int _embeddingCacheSize = 0;
        private int _llmResponseCacheSize = 0;
        private double _embeddingHitRate = 0;
        private double _llmResponseHitRate = 0;
        private long _totalEmbeddingsGenerated = 0;
        private long _totalLLMResponsesGenerated = 0;

        // UI State
        private bool _isMonitoring = false;
        private string _monitoringStatus = "Bereit";
        private int _refreshIntervalSeconds = 5;

        // Commands
        public ICommand StartMonitoringCommand { get; }
        public ICommand StopMonitoringCommand { get; }
        public ICommand ClearCacheCommand { get; }
        public ICommand RefreshStatsCommand { get; }

        // History for charting
        public ObservableCollection<CacheStatistic> HitRateHistory { get; } = new();

        public CacheStatisticsViewModel(
            ICacheService cacheService,
            ILoggerService loggerService)
        {
            _cacheService = cacheService;
            _loggerService = loggerService;

            _refreshTimer = new DispatcherTimer();
            _refreshTimer.Interval = TimeSpan.FromSeconds(RefreshIntervalSeconds);
            _refreshTimer.Tick += async (s, e) => await RefreshStatistics();

            StartMonitoringCommand = new RelayCommand(() => StartMonitoring());
            StopMonitoringCommand = new RelayCommand(() => StopMonitoring());
            ClearCacheCommand = new RelayCommand(async () => await ExecuteClearCache());
            RefreshStatsCommand = new RelayCommand(async () => await RefreshStatistics());
        }

        #region Properties

        public int EmbeddingCacheSize
        {
            get => _embeddingCacheSize;
            set { SetProperty(ref _embeddingCacheSize, value); }
        }

        public int LLMResponseCacheSize
        {
            get => _llmResponseCacheSize;
            set { SetProperty(ref _llmResponseCacheSize, value); }
        }

        public double EmbeddingHitRate
        {
            get => _embeddingHitRate;
            set { SetProperty(ref _embeddingHitRate, value); }
        }

        public double LLMResponseHitRate
        {
            get => _llmResponseHitRate;
            set { SetProperty(ref _llmResponseHitRate, value); }
        }

        public long TotalEmbeddingsGenerated
        {
            get => _totalEmbeddingsGenerated;
            set { SetProperty(ref _totalEmbeddingsGenerated, value); }
        }

        public long TotalLLMResponsesGenerated
        {
            get => _totalLLMResponsesGenerated;
            set { SetProperty(ref _totalLLMResponsesGenerated, value); }
        }

        public bool IsMonitoring
        {
            get => _isMonitoring;
            set { SetProperty(ref _isMonitoring, value); }
        }

        public string MonitoringStatus
        {
            get => _monitoringStatus;
            set { SetProperty(ref _monitoringStatus, value); }
        }

        public int RefreshIntervalSeconds
        {
            get => _refreshIntervalSeconds;
            set
            {
                if (SetProperty(ref _refreshIntervalSeconds, value))
                {
                    _refreshTimer.Interval = TimeSpan.FromSeconds(value);
                }
            }
        }

        #endregion

        #region Methods

        private void StartMonitoring()
        {
            IsMonitoring = true;
            MonitoringStatus = "🟢 Monitoring aktiv";
            _refreshTimer.Start();
            _loggerService.LogInfo("Cache Monitoring gestartet");
        }

        private void StopMonitoring()
        {
            IsMonitoring = false;
            MonitoringStatus = "🔴 Monitoring gestoppt";
            _refreshTimer.Stop();
            _loggerService.LogInfo("Cache Monitoring gestoppt");
        }

        private async Task RefreshStatistics()
        {
            try
            {
                var stats = _cacheService.GetStatistics();

                EmbeddingCacheSize = stats.EmbeddingCacheSize;
                LLMResponseCacheSize = stats.LLMResponseCacheSize;
                EmbeddingHitRate = stats.EmbeddingHitRate * 100;  // Convert to percentage
                LLMResponseHitRate = stats.LLMResponseHitRate * 100;
                TotalEmbeddingsGenerated = stats.TotalEmbeddingsGenerated;
                TotalLLMResponsesGenerated = stats.TotalLLMResponsesGenerated;

                // Add to history
                HitRateHistory.Add(new CacheStatistic
                {
                    Timestamp = DateTime.Now,
                    EmbeddingHitRate = EmbeddingHitRate,
                    LLMResponseHitRate = LLMResponseHitRate,
                    EmbeddingCacheSize = EmbeddingCacheSize,
                    LLMResponseCacheSize = LLMResponseCacheSize
                });

                // Keep only last 100 data points
                while (HitRateHistory.Count > 100)
                    HitRateHistory.RemoveAt(0);

                MonitoringStatus = $"🟢 Aktualisiert: {DateTime.Now:HH:mm:ss}";
            }
            catch (Exception ex)
            {
                MonitoringStatus = $"⚠️ Fehler beim Abrufen: {ex.Message}";
                _loggerService.LogError($"Cache Stats Fehler: {ex.Message}");
            }
        }

        private async Task ExecuteClearCache()
        {
            try
            {
                _cacheService.Clear();
                await RefreshStatistics();
                MonitoringStatus = "✅ Cache geleert";
                _loggerService.LogInfo("Cache geleert");
            }
            catch (Exception ex)
            {
                MonitoringStatus = $"❌ Fehler beim Löschen: {ex.Message}";
                _loggerService.LogError($"Clear Cache Fehler: {ex.Message}");
            }
        }

        #endregion
    }

    /// <summary>
    /// Model für Cache-Statistik-Zeitreihe
    /// </summary>
    public class CacheStatistic
    {
        public DateTime Timestamp { get; set; }
        public double EmbeddingHitRate { get; set; }
        public double LLMResponseHitRate { get; set; }
        public int EmbeddingCacheSize { get; set; }
        public int LLMResponseCacheSize { get; set; }
    }
}
