/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisConnectionServiceGrpc.cs                     ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:23:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     262                                            ║
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
using System.IO;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Erweiterte ThemisConnectionService mit gRPC-Unterstützung
    /// Wechselt automatisch zwischen gRPC und HTTP basierend auf Konfiguration
    /// </summary>
    public class ThemisConnectionServiceGrpc : IThemisConnectionService
    {
        private string _host = "localhost";
        private int _port = 18765;  // Standard ThemisDB Port
        private int _grpcPort = 18765;  // gRPC auf gleichem Port wie HTTP
        private bool _useGrpc = false;  // HTTP als Default
        private System.Threading.Timer? _heartbeatTimer;
        private bool _lastConnectionState = false;
        private IGrpcThemisService? _grpcService;
        private readonly ILoggerService _loggerService;
        private readonly ISettingsService _settingsService;

        public event EventHandler<ConnectionStatusChangedEventArgs>? ConnectionStatusChanged;

        public void UpdateConnectionSettings(string host, int port)
        {
            _host = host;
            _port = port;
            _grpcPort = port;  // Nutze gleichen Port für gRPC
            System.Diagnostics.Debug.WriteLine($"[CONNECTION-GRPC] Settings updated: {_host}:{_port}");
            
            // Aktualisiere gRPC Service falls aktiviert
            if (_useGrpc)
            {
                try
                {
                    _grpcService = new GrpcThemisService(_host, _grpcPort, _loggerService);
                    _loggerService.LogInfo("gRPC Service neu initialisiert mit neuen Einstellungen");
                }
                catch (Exception ex)
                {
                    _loggerService.LogError($"Fehler bei gRPC Service Re-Initialisierung: {ex.Message}");
                    _grpcService = null;
                }
            }
            
            // Starte oder restart den Heartbeat-Timer nach Settings-Update
            if (_heartbeatTimer == null)
            {
                _heartbeatTimer = new System.Threading.Timer(
                    async _ =>
                    {
                        try
                        {
                            await PerformHeartbeatCheckAsync();
                        }
                        catch (Exception ex)
                        {
                            _loggerService.LogError($"Heartbeat-Fehler: {ex.Message}");
                        }
                    },
                    null,
                    TimeSpan.FromSeconds(2),
                    TimeSpan.FromSeconds(5));
            }
        }

        public ThemisConnectionServiceGrpc(ISettingsService settingsService, ILoggerService loggerService)
        {
            _loggerService = loggerService;
            _settingsService = settingsService;
            
            // Lade Settings wenn Service bereit ist
            try
            {
                LoadSettings();
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Laden der Connection-Settings: {ex.Message}");
            }
        }

        /// <summary>
        /// Laden von Einstellungen
        /// </summary>
        private void LoadSettings()
        {
            var settings = _settingsService.LoadSettings();
            _host = settings.ThemisHost;
            _port = settings.ThemisPort;
            _grpcPort = settings.ThemisGrpcPort;
            _useGrpc = settings.UseGrpc;

            _loggerService.LogInfo($"ThemisConnection Settings geladen - Host: {_host}, HTTP-Port: {_port}, gRPC-Port: {_grpcPort}, UseGrpc: {_useGrpc}");

            // Initialisiere gRPC Service wenn gRPC aktiviert ist
            if (_useGrpc)
            {
                try
                {
                    _grpcService = new GrpcThemisService(_host, _grpcPort, _loggerService);
                    _loggerService.LogInfo("gRPC Service initialisiert");
                }
                catch (Exception ex)
                {
                    _loggerService.LogError($"Fehler bei gRPC Service Initialisierung: {ex.Message}");
                    _grpcService = null;
                }
            }
        }

        private async Task PerformHeartbeatCheckAsync()
        {
            bool currentState = await CheckConnectionAsync();
            if (currentState != _lastConnectionState)
            {
                _lastConnectionState = currentState;
                ConnectionStatusChanged?.Invoke(this, new ConnectionStatusChangedEventArgs
                {
                    IsConnected = currentState,
                    Timestamp = DateTime.Now,
                    Message = currentState ? "Mit Themis verbunden" : "Verbindung zu Themis unterbrochen"
                });
            }
        }

        /// <summary>
        /// Prüfe Verbindung - zuerst gRPC, dann Fallback zu HTTP
        /// </summary>
        public async Task<bool> CheckConnectionAsync()
        {
            if (_useGrpc && _grpcService != null)
            {
                try
                {
                    return await _grpcService.HealthCheckAsync();
                }
                catch (Exception ex)
                {
                    _loggerService.LogWarning($"gRPC HealthCheck fehlgeschlagen: {ex.Message}, Fallback zu HTTP");
                    // Fallback zu HTTP
                    return await TestConnectionAsyncHttp(_host, _port);
                }
            }
            else
            {
                return await TestConnectionAsyncHttp(_host, _port);
            }
        }

        /// <summary>
        /// Test der Verbindung mit Fallback-Logik
        /// </summary>
        public async Task<bool> TestConnectionAsync(string host, int port)
        {
            _host = host;
            _port = port;

            if (_useGrpc && _grpcService != null)
            {
                try
                {
                    return await _grpcService.HealthCheckAsync();
                }
                catch (Exception ex)
                {
                    _loggerService.LogWarning($"gRPC Connection Test fehlgeschlagen: {ex.Message}");
                }
            }

            // Fallback zu HTTP
            return await TestConnectionAsyncHttp(host, port);
        }

        /// <summary>
        /// HTTP-basierter Connection Test (Fallback)
        /// </summary>
        private async Task<bool> TestConnectionAsyncHttp(string host, int port)
        {
            try
            {
                using (var client = new HttpClient { Timeout = TimeSpan.FromSeconds(3) })
                {
                    var response = await client.GetAsync($"http://{host}:{port}/health");
                    bool isConnected = response.IsSuccessStatusCode;
                    
                    ConnectionStatusChanged?.Invoke(this, new ConnectionStatusChangedEventArgs
                    {
                        IsConnected = isConnected,
                        Message = isConnected ? "HTTP Verbindung erfolgreich" : "HTTP Verbindung fehlgeschlagen"
                    });
                    
                    return isConnected;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"HTTP Connection Test fehler: {ex.Message}");
                
                ConnectionStatusChanged?.Invoke(this, new ConnectionStatusChangedEventArgs
                {
                    IsConnected = false,
                    Message = "Verbindung zu Themis fehlgeschlagen"
                });
                
                return false;
            }
        }

        /// <summary>
        /// Gib die gRPC Service Instanz zurück wenn verfügbar
        /// </summary>
        public IGrpcThemisService? GetGrpcService()
        {
            return _grpcService;
        }

        /// <summary>
        /// Cleanup
        /// </summary>
        public void Dispose()
        {
            _heartbeatTimer?.Dispose();
            if (_grpcService != null)
            {
                _grpcService.DisconnectAsync().Wait(TimeSpan.FromSeconds(5));
            }
        }

        ~ThemisConnectionServiceGrpc()
        {
            Dispose();
        }
    }
}
