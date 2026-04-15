/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RealIngestionService.cs                            ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:33:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     353                                            ║
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
using System.Linq;
using System.Threading.Tasks;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Real-Time Ingestion zu ThemisDB via HTTP
    /// Speichert Daten direkt in Entities, Vectors und TimeSeries Tables
    /// </summary>
    public interface IRealIngestionService
    {
        Task<RealIngestionResult> IngestFilesAsync(
            List<FileAnalysisResult> files,
            bool useBatch = true,
            int batchSize = 100,
            IProgress<RealIngestionProgress>? progress = null);
    }

    public class RealIngestionService : IRealIngestionService
    {
        private readonly IThemisApiService _themisApiService;
        private readonly ILoggerService _loggerService;
        private readonly ISettingsService _settingsService;

        public RealIngestionService(
            IThemisApiService themisApiService,
            ILoggerService loggerService,
            ISettingsService settingsService)
        {
            _themisApiService = themisApiService;
            _loggerService = loggerService;
            _settingsService = settingsService;
        }

        /// <summary>
        /// Ingeste Dateien direkt zu ThemisDB
        /// </summary>
        public async Task<RealIngestionResult> IngestFilesAsync(
            List<FileAnalysisResult> files,
            bool useBatch = true,
            int batchSize = 100,
            IProgress<RealIngestionProgress>? progress = null)
        {
            var result = new RealIngestionResult
            {
                StartTime = DateTime.Now,
                TotalFiles = files.Count,
                Settings = _settingsService.LoadSettings()
            };

            if (string.IsNullOrEmpty(result.Settings.ThemisApiUrl))
            {
                _loggerService.LogWarning("ThemisDB API URL is not configured. Skipping ingestion.");
                result.ErrorMessage = "ThemisDB API URL not configured";
                return result;
            }

            _loggerService.LogInfo($"Starting real ingestion of {files.Count} files to ThemisDB");

            try
            {
                if (useBatch)
                {
                    // Batch-Processing
                    await IngestInBatchesAsync(files, batchSize, result, progress);
                }
                else
                {
                    // Sequential Processing
                    await IngestSequentialAsync(files, result, progress);
                }

                result.Success = true;
                result.EndTime = DateTime.Now;

                _loggerService.LogInfo(
                    $"Ingestion completed: {result.StoredEntities} entities, " +
                    $"{result.StoredVectors} vectors, {result.StoredTimeSeries} metrics");
            }
            catch (Exception ex)
            {
                result.Success = false;
                result.ErrorMessage = ex.Message;
                _loggerService.LogError($"Ingestion failed: {ex.Message}");
            }

            return result;
        }

        /// <summary>
        /// Batch-Processing: sendet mehrere Dateien gleichzeitig
        /// </summary>
        private async Task IngestInBatchesAsync(
            List<FileAnalysisResult> files,
            int batchSize,
            RealIngestionResult result,
            IProgress<RealIngestionProgress>? progress)
        {
            var batches = files
                .Select((file, index) => new { file, index })
                .GroupBy(x => x.index / batchSize)
                .Select(g => g.Select(x => x.file).ToList())
                .ToList();

            _loggerService.LogInfo($"Processing {batches.Count} batches (size: {batchSize})");

            for (int batchIndex = 0; batchIndex < batches.Count; batchIndex++)
            {
                var batch = batches[batchIndex];
                var batchTasks = new List<Task>();

                // Parallel ingestion within batch
                foreach (var file in batch)
                {
                    if (file.IsProcessed && !file.IsDuplicate)
                    {
                        var task = IngestFileAsync(file, result);
                        batchTasks.Add(task);
                    }
                    else
                    {
                        result.SkippedFiles++;
                    }
                }

                // Wait for batch to complete
                await Task.WhenAll(batchTasks);

                // Report progress
                var progressData = new RealIngestionProgress
                {
                    BatchIndex = batchIndex + 1,
                    TotalBatches = batches.Count,
                    ProcessedFiles = result.StoredEntities + result.FailedFiles + result.SkippedFiles,
                    StoredEntities = result.StoredEntities,
                    StoredVectors = result.StoredVectors,
                    StoredTimeSeries = result.StoredTimeSeries,
                    FailedFiles = result.FailedFiles,
                    CurrentBatchSize = batch.Count,
                    Message = $"Batch {batchIndex + 1}/{batches.Count} completed ({batch.Count} files)"
                };

                progress?.Report(progressData);
                _loggerService.LogInfo($"Batch {batchIndex + 1}/{batches.Count} completed");
            }
        }

        /// <summary>
        /// Sequential Processing: sendet Dateien nacheinander
        /// </summary>
        private async Task IngestSequentialAsync(
            List<FileAnalysisResult> files,
            RealIngestionResult result,
            IProgress<RealIngestionProgress>? progress)
        {
            for (int i = 0; i < files.Count; i++)
            {
                var file = files[i];

                if (file.IsProcessed && !file.IsDuplicate)
                {
                    await IngestFileAsync(file, result);
                }
                else
                {
                    result.SkippedFiles++;
                }

                // Report progress
                var progressData = new RealIngestionProgress
                {
                    ProcessedFiles = i + 1,
                    TotalFiles = files.Count,
                    StoredEntities = result.StoredEntities,
                    StoredVectors = result.StoredVectors,
                    StoredTimeSeries = result.StoredTimeSeries,
                    FailedFiles = result.FailedFiles,
                    Message = $"Ingesting: {file.FileName} ({i + 1}/{files.Count})"
                };

                progress?.Report(progressData);
            }
        }

        /// <summary>
        /// Ingeste eine einzelne Datei zu ThemisDB
        /// </summary>
        private async Task IngestFileAsync(FileAnalysisResult file, RealIngestionResult result)
        {
            try
            {
                var fileKey = $"file:{file.ContentHash}";

                // 1. Store Entity
                var entityStored = await _themisApiService.StoreEntityAsync(file);
                if (entityStored)
                {
                    result.StoredEntities++;
                }
                else
                {
                    result.FailedFiles++;
                    return;
                }

                // 2. Store Vectors (if enabled)
                if (result.Settings.StoreVectors && !string.IsNullOrEmpty(file.Summary))
                {
                    var embedding = _themisApiService.GenerateEmbedding(file.Summary);
                    var vectorMetadata = new Dictionary<string, object>
                    {
                        { "filename", file.FileName },
                        { "relevance", file.RelevanceScore },
                        { "language", file.Language }
                    };

                    var vectorStored = await _themisApiService.StoreVectorAsync(fileKey, embedding, vectorMetadata);
                    if (vectorStored)
                    {
                        result.StoredVectors++;
                    }
                }

                // 3. Store TimeSeries (if enabled)
                if (result.Settings.TrackTimeSeries)
                {
                    var tags = new Dictionary<string, string>
                    {
                        { "file", file.FileName },
                        { "type", file.FileType },
                        { "language", file.Language }
                    };

                    var timeSeriesStored = await _themisApiService.StoreTimeSeriesAsync(
                        $"metric:quality:{file.ContentHash}",
                        file.QualityScore,
                        file.AnalysisTimestamp,
                        tags);

                    if (timeSeriesStored)
                    {
                        result.StoredTimeSeries++;
                    }
                }

                _loggerService.LogInfo($"Ingested: {file.FileName} -> {fileKey}");
            }
            catch (Exception ex)
            {
                result.FailedFiles++;
                _loggerService.LogError($"Failed to ingest {file.FileName}: {ex.Message}");
            }
        }
    }

    /// <summary>
    /// Ergebnis der echten Ingestion
    /// </summary>
    public class RealIngestionResult
    {
        public bool Success { get; set; }
        public DateTime StartTime { get; set; }
        public DateTime EndTime { get; set; }
        public TimeSpan Duration => EndTime - StartTime;

        public int TotalFiles { get; set; }
        public int StoredEntities { get; set; }
        public int StoredVectors { get; set; }
        public int StoredTimeSeries { get; set; }
        public int FailedFiles { get; set; }
        public int SkippedFiles { get; set; }

        public string ErrorMessage { get; set; } = string.Empty;
        public AppSettings Settings { get; set; } = new();

        public override string ToString()
        {
            return $@"
╔═══════════════════════════════════════════════════════════════════════════════╗
║                      REAL INGESTION RESULT                                   ║
╚═══════════════════════════════════════════════════════════════════════════════╝

Status:           {(Success ? "✅ SUCCESS" : "❌ FAILED")}
Duration:         {Duration.TotalSeconds:F2} seconds
Total Files:      {TotalFiles}

ThemisDB Storage:
  Entities:       {StoredEntities} files stored
  Vectors:        {StoredVectors} embeddings stored
  TimeSeries:     {StoredTimeSeries} metrics stored
  
Results:
  Failed:         {FailedFiles}
  Skipped:        {SkippedFiles}
  Success Rate:   {(TotalFiles > 0 ? (StoredEntities * 100.0 / TotalFiles) : 0):F1}%

Configuration:
  Host:           {Settings.ThemisHost}:{Settings.ThemisPort}
  Store Vectors:  {Settings.StoreVectors}
  Track Series:   {Settings.TrackTimeSeries}

═══════════════════════════════════════════════════════════════════════════════
";
        }
    }

    /// <summary>
    /// Progress-Bericht während Ingestion
    /// </summary>
    public class RealIngestionProgress
    {
        public int ProcessedFiles { get; set; }
        public int TotalFiles { get; set; }
        public int StoredEntities { get; set; }
        public int StoredVectors { get; set; }
        public int StoredTimeSeries { get; set; }
        public int FailedFiles { get; set; }
        public string Message { get; set; } = string.Empty;

        // For Batch Processing
        public int BatchIndex { get; set; }
        public int TotalBatches { get; set; }
        public int CurrentBatchSize { get; set; }

        public double ProgressPercent =>
            TotalFiles > 0 ? (ProcessedFiles * 100.0 / TotalFiles) : 0;
    }
}
