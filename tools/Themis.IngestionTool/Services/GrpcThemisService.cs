/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GrpcThemisService.cs                               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     329                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Grpc.Net.Client;
using Themis.IngestionTool.Grpc;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// gRPC-basierter Service für Kommunikation mit ThemisDB
    /// Ersetzt die HTTP-REST-basierte Kommunikation mit Grpc für bessere Performance
    /// </summary>
    public interface IGrpcThemisService
    {
        Task<bool> HealthCheckAsync();
        Task<bool> CreateEntityAsync(string key, Dictionary<string, string> data, Dictionary<string, string>? metadata = null);
        Task<bool> CreateRelationshipAsync(string fromKey, string toKey, string relationshipType, Dictionary<string, string>? properties = null);
        Task<bool> UpsertVectorAsync(string collectionName, string objectKey, double[] vector, Dictionary<string, string>? metadata = null);
        Task<bool> InsertTimeSeriesAsync(string key, List<TimeSeriesPoint> points);
        Task<(bool success, List<VectorResultDto> results)> QueryVectorAsync(string collectionName, double[] queryVector, int limit = 10, float threshold = 0.5f);
        string GetServerInfo();
        Task DisconnectAsync();
    }

    public class GrpcThemisService : IGrpcThemisService
    {
        private readonly GrpcChannel _channel;
        private readonly ThemisService.ThemisServiceClient _client;
        private readonly string _host;
        private readonly int _port;
        private readonly ILoggerService _loggerService;

        public GrpcThemisService(string host, int port, ILoggerService loggerService)
        {
            _host = host;
            _port = port;
            _loggerService = loggerService;

            try
            {
                // Erstelle gRPC Channel mit AppContext Flag für HTTP/2 ohne TLS
                AppContext.SetSwitch("System.Net.Http.SocketsHttpHandler.Http2UnencryptedSupport", true);

                _channel = GrpcChannel.ForAddress($"http://{host}:{port}", new GrpcChannelOptions
                {
                    MaxReceiveMessageSize = 10 * 1024 * 1024, // 10 MB max
                    MaxSendMessageSize = 10 * 1024 * 1024     // 10 MB max
                });

                _client = new ThemisService.ThemisServiceClient(_channel);
                _loggerService.LogInfo($"gRPC Channel zu {host}:{port} hergestellt");
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Erstellen des gRPC Channels: {ex.Message}");
                throw;
            }
        }

        /// <summary>
        /// Health Check - testet Verbindung zu Themis
        /// </summary>
        public async Task<bool> HealthCheckAsync()
        {
            try
            {
                var request = new HealthCheckRequest { Service = "ThemisDB" };
                var response = await _client.HealthCheckAsync(request);
                _loggerService.LogInfo($"Health Check erfolgreich: {response.Message}");
                return response.Healthy;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Health Check fehlgeschlagen: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Entity erstellen
        /// </summary>
        public async Task<bool> CreateEntityAsync(string key, Dictionary<string, string> data, Dictionary<string, string>? metadata = null)
        {
            try
            {
                var request = new CreateEntityRequest
                {
                    Key = key,
                    Metadata = { metadata ?? new Dictionary<string, string>() }
                };

                foreach (var kvp in data)
                {
                    request.Data[kvp.Key] = kvp.Value;
                }

                var response = await _client.CreateEntityAsync(request);
                
                if (!response.Success)
                {
                    _loggerService.LogError($"Entity erstellen fehlgeschlagen: {response.Error}");
                    return false;
                }

                _loggerService.LogInfo($"Entity erstellt: {key}");
                return true;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Erstellen der Entity: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Relationship erstellen
        /// </summary>
        public async Task<bool> CreateRelationshipAsync(string fromKey, string toKey, string relationshipType, Dictionary<string, string>? properties = null)
        {
            try
            {
                var request = new CreateRelationshipRequest
                {
                    FromKey = fromKey,
                    ToKey = toKey,
                    RelationshipType = relationshipType,
                    Properties = { properties ?? new Dictionary<string, string>() }
                };

                var response = await _client.CreateRelationshipAsync(request);

                if (!response.Success)
                {
                    _loggerService.LogError($"Relationship erstellen fehlgeschlagen: {response.Error}");
                    return false;
                }

                _loggerService.LogInfo($"Relationship erstellt: {fromKey} -> {toKey} ({relationshipType})");
                return true;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Erstellen der Relationship: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Vector in Collection einfügen oder aktualisieren
        /// </summary>
        public async Task<bool> UpsertVectorAsync(string collectionName, string objectKey, double[] vector, Dictionary<string, string>? metadata = null)
        {
            try
            {
                var request = new UpsertVectorRequest
                {
                    CollectionName = collectionName,
                    ObjectKey = objectKey,
                    Metadata = { metadata ?? new Dictionary<string, string>() }
                };

                request.Vector.AddRange(vector);

                var response = await _client.UpsertVectorAsync(request);

                if (!response.Success)
                {
                    _loggerService.LogError($"Vector upsert fehlgeschlagen: {response.Error}");
                    return false;
                }

                _loggerService.LogInfo($"Vector upserted: {objectKey} in {collectionName}");
                return true;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Vector upsert: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// TimeSeries-Punkte einfügen
        /// </summary>
        public async Task<bool> InsertTimeSeriesAsync(string key, List<TimeSeriesPoint> points)
        {
            try
            {
                var request = new InsertTimeSeriesRequest { Key = key };

                foreach (var point in points)
                {
                    var grpcPoint = new Themis.IngestionTool.Grpc.TimeSeriesPoint
                    {
                        Timestamp = point.Timestamp,
                        Value = point.Value,
                        Tags = { point.Tags ?? new Dictionary<string, string>() }
                    };
                    request.Points.Add(grpcPoint);
                }

                var response = await _client.InsertTimeSeriesAsync(request);

                if (!response.Success)
                {
                    _loggerService.LogError($"TimeSeries Insert fehlgeschlagen: {response.Error}");
                    return false;
                }

                _loggerService.LogInfo($"TimeSeries eingefügt: {response.PointsInserted} Punkte für {key}");
                return true;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim TimeSeries Insert: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Vector Query - sucht nach ähnlichen Vectors
        /// </summary>
        public async Task<(bool success, List<VectorResultDto> results)> QueryVectorAsync(string collectionName, double[] queryVector, int limit = 10, float threshold = 0.5f)
        {
            try
            {
                var request = new QueryVectorRequest
                {
                    CollectionName = collectionName,
                    Limit = limit,
                    Threshold = threshold
                };

                request.QueryVector.AddRange(queryVector);

                var response = await _client.QueryVectorAsync(request);

                if (!response.Success)
                {
                    _loggerService.LogError($"Vector Query fehlgeschlagen: {response.Error}");
                    return (false, new List<VectorResultDto>());
                }

                var results = response.Results.Select(r => new VectorResultDto
                {
                    Key = r.Key,
                    Similarity = r.Similarity,
                    Metadata = r.Metadata.ToDictionary(x => x.Key, x => x.Value)
                }).ToList();

                _loggerService.LogInfo($"Vector Query erfolgreich: {results.Count} Ergebnisse");
                return (true, results);
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Vector Query: {ex.Message}");
                return (false, new List<VectorResultDto>());
            }
        }

        /// <summary>
        /// Server-Informationen abrufen
        /// </summary>
        public string GetServerInfo()
        {
            return $"gRPC Server: {_host}:{_port}";
        }

        /// <summary>
        /// Cleanup
        /// </summary>
        public async Task DisconnectAsync()
        {
            if (_channel != null)
            {
                await _channel.ShutdownAsync();
            }
        }

        ~GrpcThemisService()
        {
            DisconnectAsync().Wait(TimeSpan.FromSeconds(5));
        }
    }

    /// <summary>
    /// DTO für Vector Query Ergebnisse
    /// </summary>
    public class VectorResultDto
    {
        public string Key { get; set; } = string.Empty;
        public double Similarity { get; set; }
        public Dictionary<string, string> Metadata { get; set; } = new();
    }

    /// <summary>
    /// DTO für TimeSeries Punkte
    /// </summary>
    public class TimeSeriesPoint
    {
        public long Timestamp { get; set; }
        public double Value { get; set; }
        public Dictionary<string, string>? Tags { get; set; }
    }
}
