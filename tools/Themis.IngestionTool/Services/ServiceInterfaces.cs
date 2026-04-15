/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ServiceInterfaces.cs                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:57:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    public interface IIngestionService
    {
        Task<IngestionResult> StartIngestionAsync(string sourceFolder, string outputFile);
        void CancelIngestion();
        event EventHandler<ProgressEventArgs>? ProgressChanged;
    }

    public interface IThemisConnectionService
    {
        Task<bool> CheckConnectionAsync();
        Task<bool> TestConnectionAsync(string host, int port);
        void UpdateConnectionSettings(string host, int port);
        event EventHandler<ConnectionStatusChangedEventArgs>? ConnectionStatusChanged;
    }

    public interface ISettingsService
    {
        AppSettings LoadSettings();
        void SaveSettings(AppSettings settings);
        string GetThemisApiUrl();  // Get API URL for graph/vector queries
        bool UseGrpc { get; }  // Bestimmt ob gRPC oder HTTP verwendet wird
    }

    public interface ILoggerService
    {
        void LogInfo(string message);
        void LogWarning(string message);
        void LogError(string message);
        event EventHandler<LogEventArgs>? LogMessageReceived;
    }

    public class IngestionResult
    {
        public int ProcessedFiles { get; set; }
        public int DuplicatesFound { get; set; }
        public int Errors { get; set; }
    }

    public class ProgressEventArgs : EventArgs
    {
        public int Current { get; set; }
        public int Total { get; set; }
        public string CurrentFile { get; set; } = string.Empty;
    }

    public class ConnectionStatusChangedEventArgs : EventArgs
    {
        public bool IsConnected { get; set; }
        public string Message { get; set; } = string.Empty;
        public DateTime Timestamp { get; set; } = DateTime.Now;
    }

    public class LogEventArgs : EventArgs
    {
        public string Message { get; set; } = string.Empty;
        public LogLevel Level { get; set; }
    }

    public enum LogLevel
    {
        Info,
        Warning,
        Error
    }
}
