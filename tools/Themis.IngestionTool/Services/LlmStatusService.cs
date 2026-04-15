/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmStatusService.cs                                ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     291                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Interface für die Überwachung des llama.cpp LLM-Status.
    /// Prüft die Verfügbarkeit des Ollama-Services und des geladenen Modells.
    /// </summary>
    public interface ILlmStatusService
    {
        /// <summary>
        /// Prüft die Verbindung zum llama.cpp/Ollama-Service.
        /// </summary>
        Task<bool> CheckConnectionAsync();

        /// <summary>
        /// Ruft den aktuellen Status des llama.cpp/Ollama-Services ab.
        /// </summary>
        Task<LlmStatus> GetLlmStatusAsync();

        /// <summary>
        /// Event wird ausgelöst, wenn sich der LLM-Status ändert.
        /// </summary>
        event EventHandler<LlmStatusChangedEventArgs>? StatusChanged;
    }

    /// <summary>
    /// Implementierung des LLM-Status-Services.
    /// </summary>
    public class LlmStatusService : ILlmStatusService
    {
        private readonly ISettingsService _settingsService;
        private readonly ILoggerService _loggerService;
        private System.Threading.Timer? _heartbeatTimer;
        private LlmStatus? _lastStatus;
        private bool _isDisposed = false;

        public event EventHandler<LlmStatusChangedEventArgs>? StatusChanged;

        public LlmStatusService(ISettingsService settingsService, ILoggerService loggerService)
        {
            _settingsService = settingsService;
            _loggerService = loggerService;

            // Starte automatischen Status-Check alle 10 Sekunden
            _heartbeatTimer = new System.Threading.Timer(
                async _ => await PerformHeartbeatCheckAsync(),
                null,
                TimeSpan.FromSeconds(2),
                TimeSpan.FromSeconds(10));
        }

        public async Task<bool> CheckConnectionAsync()
        {
            try
            {
                var status = await GetLlmStatusAsync();
                return status.IsAvailable;
            }
            catch
            {
                return false;
            }
        }

        public async Task<LlmStatus> GetLlmStatusAsync()
        {
            try
            {
                var settings = _settingsService.LoadSettings();
                using (var client = new HttpClient { Timeout = TimeSpan.FromSeconds(5) })
                {
                    // Versuche die Ollama Tags zu laden, um verfügbare Modelle zu prüfen
                    var response = await client.GetAsync($"http://{settings.OllamaHost}:{settings.OllamaPort}/api/tags");
                    
                    if (!response.IsSuccessStatusCode)
                    {
                        return new LlmStatus
                        {
                            IsAvailable = false,
                            IsModelLoaded = false,
                            LoadedModel = null,
                            Error = $"HTTP {response.StatusCode}"
                        };
                    }

                    var content = await response.Content.ReadAsStringAsync();
                    using (var doc = JsonDocument.Parse(content))
                    {
                        var root = doc.RootElement;
                        var status = new LlmStatus { IsAvailable = true };

                        // Prüfe, ob das konfigurierte Modell in der Liste ist
                        if (root.TryGetProperty("models", out var modelsArray))
                        {
                            foreach (var model in modelsArray.EnumerateArray())
                            {
                                if (model.TryGetProperty("name", out var nameElement))
                                {
                                    var modelName = nameElement.GetString();
                                    if (modelName?.StartsWith(settings.LlamaModel) == true)
                                    {
                                        status.IsModelLoaded = true;
                                        status.LoadedModel = modelName;
                                        
                                        // Zusätzliche Informationen wenn vorhanden
                                        if (model.TryGetProperty("size", out var sizeElement))
                                        {
                                            status.ModelSize = sizeElement.GetInt64();
                                        }
                                        if (model.TryGetProperty("modified_at", out var modifiedElement))
                                        {
                                            status.LastModified = modifiedElement.GetString();
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        if (!status.IsModelLoaded)
                        {
                            status.Error = $"Modell '{settings.LlamaModel}' ist nicht geladen";
                        }

                        return status;
                    }
                }
            }
            catch (HttpRequestException ex)
            {
                _loggerService.LogError($"LLM Status Fehler: {ex.Message}");
                return new LlmStatus
                {
                    IsAvailable = false,
                    IsModelLoaded = false,
                    Error = "Verbindung fehlgeschlagen"
                };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Abrufen des LLM-Status: {ex.Message}");
                return new LlmStatus
                {
                    IsAvailable = false,
                    IsModelLoaded = false,
                    Error = ex.Message
                };
            }
        }

        private async Task PerformHeartbeatCheckAsync()
        {
            if (_isDisposed)
                return;

            try
            {
                var currentStatus = await GetLlmStatusAsync();

                // Vergleiche mit dem letzten bekannten Status
                if (_lastStatus == null || 
                    _lastStatus.IsAvailable != currentStatus.IsAvailable ||
                    _lastStatus.IsModelLoaded != currentStatus.IsModelLoaded ||
                    _lastStatus.LoadedModel != currentStatus.LoadedModel)
                {
                    _lastStatus = currentStatus;
                    StatusChanged?.Invoke(this, new LlmStatusChangedEventArgs
                    {
                        Status = currentStatus,
                        Timestamp = DateTime.Now
                    });
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Heartbeat-Check fehlgeschlagen: {ex.Message}");
            }
        }

        public void Dispose()
        {
            _isDisposed = true;
            _heartbeatTimer?.Dispose();
        }
    }

    /// <summary>
    /// Repräsentiert den Status des llama.cpp/Ollama-LLM-Services.
    /// </summary>
    public class LlmStatus
    {
        /// <summary>
        /// Gibt an, ob der Ollama-Service verfügbar ist.
        /// </summary>
        public bool IsAvailable { get; set; }

        /// <summary>
        /// Gibt an, ob das konfigurierte Modell geladen ist.
        /// </summary>
        public bool IsModelLoaded { get; set; }

        /// <summary>
        /// Name des geladenen Modells.
        /// </summary>
        public string? LoadedModel { get; set; }

        /// <summary>
        /// Größe des Modells in Bytes.
        /// </summary>
        public long? ModelSize { get; set; }

        /// <summary>
        /// Zeitstempel der letzten Änderung des Modells.
        /// </summary>
        public string? LastModified { get; set; }

        /// <summary>
        /// Fehlermeldung, falls vorhanden.
        /// </summary>
        public string? Error { get; set; }

        /// <summary>
        /// Gibt eine benutzerfreundliche Statusbeschreibung zurück.
        /// </summary>
        public string GetStatusDescription()
        {
            if (!IsAvailable)
            {
                return $"Ollama Offline: {Error ?? "Verbindung fehlgeschlagen"}";
            }

            if (!IsModelLoaded)
            {
                return $"Modell nicht geladen: {Error ?? "Unbekannter Fehler"}";
            }

            var sizeText = ModelSize.HasValue ? $" ({FormatFileSize(ModelSize.Value)})" : "";
            return $"Modell aktiv: {LoadedModel}{sizeText}";
        }

        private static string FormatFileSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB" };
            double len = bytes;
            int order = 0;
            while (len >= 1024 && order < sizes.Length - 1)
            {
                order++;
                len = len / 1024;
            }
            return $"{len:0.##} {sizes[order]}";
        }
    }

    /// <summary>
    /// Event-Argumente für LLM-Status-Änderungen.
    /// </summary>
    public class LlmStatusChangedEventArgs : EventArgs
    {
        public LlmStatus Status { get; set; } = new();
        public DateTime Timestamp { get; set; }
    }
}
