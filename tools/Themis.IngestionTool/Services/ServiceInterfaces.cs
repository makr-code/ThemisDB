/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ServiceInterfaces.cs                               ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     95                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
