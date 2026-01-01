using System.Diagnostics;
using System.Text.Json;
using Microsoft.Extensions.Logging;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services;

/// <summary>
/// Hauptengine für den Ingestion-Prozess
/// </summary>
public class IngestionEngine
{
    private readonly IngestionConfig _config;
    private readonly IngestionTracker _tracker;
    private readonly FileProcessor _processor;
    private readonly ILogger<IngestionEngine> _logger;

    public IngestionEngine(
        IngestionConfig config,
        IngestionTracker tracker,
        FileProcessor processor,
        ILogger<IngestionEngine> logger)
    {
        _config = config;
        _tracker = tracker;
        _processor = processor;
        _logger = logger;
    }

    public bool ShouldProcessFile(string filePath)
    {
        // Prüfe Ausschlussmuster
        foreach (var pattern in _config.ExcludePatterns)
        {
            if (filePath.Contains(pattern, StringComparison.OrdinalIgnoreCase))
                return false;
        }

        // Prüfe Dateierweiterung
        var extension = Path.GetExtension(filePath).ToLowerInvariant();

        if (_config.IncludeExtensions.Count > 0)
        {
            if (!_config.IncludeExtensions.Contains(extension))
                return false;
        }

        if (_config.ExcludeExtensions.Contains(extension))
            return false;

        // Prüfe Dateigröße
        try
        {
            var fileInfo = new FileInfo(filePath);
            var maxSizeBytes = _config.MaxFileSizeMb * 1024 * 1024;
            if (fileInfo.Length > maxSizeBytes)
            {
                _logger.LogWarning("Skipping {FilePath}: file too large ({Size} MB)",
                    filePath, fileInfo.Length / 1024.0 / 1024.0);
                return false;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error checking file size for {FilePath}", filePath);
            return false;
        }

        return true;
    }

    public List<string> ScanDirectory(string directory)
    {
        var files = new List<string>();
        _logger.LogInformation("Scanning directory: {Directory}", directory);

        try
        {
            var allFiles = Directory.GetFiles(directory, "*", SearchOption.AllDirectories);
            
            foreach (var file in allFiles)
            {
                if (ShouldProcessFile(file))
                {
                    files.Add(file);
                }
            }

            _logger.LogInformation("Found {Count} files to process", files.Count);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error scanning directory {Directory}", directory);
        }

        return files;
    }

    public async Task<IngestionResult> IngestAsync(IProgress<(int current, int total, int skipped)>? progress = null)
    {
        _logger.LogInformation("Starting ingestion process");
        var stopwatch = Stopwatch.StartNew();

        var stats = new IngestionStats();
        var ingestedFiles = new List<FileMetadata>();

        // Verzeichnis scannen
        var files = ScanDirectory(_config.SourceDir);
        stats.TotalFilesScanned = files.Count;

        // Dateien verarbeiten
        for (int i = 0; i < files.Count; i++)
        {
            var filePath = files[i];
            progress?.Report((i + 1, files.Count, stats.FilesSkipped));

            try
            {
                // Hash zuerst berechnen für Duplikatprüfung
                var fileHash = _processor.ComputeHash(filePath);
                if (string.IsNullOrEmpty(fileHash))
                {
                    stats.FilesFailed++;
                    continue;
                }

                // Prüfe ob bereits ingestiert
                if (_tracker.IsIngested(filePath, fileHash))
                {
                    stats.FilesSkipped++;
                    continue;
                }

                // Datei verarbeiten
                var fileMetadata = _processor.ProcessFile(filePath);
                if (fileMetadata != null)
                {
                    _tracker.AddFile(fileMetadata);
                    ingestedFiles.Add(fileMetadata);
                    stats.FilesProcessed++;
                    stats.TotalSizeBytes += fileMetadata.FileSize;
                }
                else
                {
                    stats.FilesFailed++;
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error processing file {FilePath}", filePath);
                stats.FilesFailed++;
            }
        }

        stopwatch.Stop();
        stats.ElapsedSeconds = stopwatch.Elapsed.TotalSeconds;

        // Statistik loggen
        _logger.LogInformation("Ingestion complete!");
        _logger.LogInformation("Files scanned: {Count}", stats.TotalFilesScanned);
        _logger.LogInformation("Files processed: {Count}", stats.FilesProcessed);
        _logger.LogInformation("Files skipped (already ingested): {Count}", stats.FilesSkipped);
        _logger.LogInformation("Files failed: {Count}", stats.FilesFailed);
        _logger.LogInformation("Total size: {Size:F2} MB", stats.TotalSizeBytes / 1024.0 / 1024.0);
        _logger.LogInformation("Elapsed time: {Time:F2} seconds", stats.ElapsedSeconds);

        // Ergebnis zusammenstellen
        var result = new IngestionResult
        {
            Metadata = new IngestionMetadataInfo
            {
                IngestionTime = DateTime.UtcNow,
                SourceDirectory = _config.SourceDir,
                Config = _config
            },
            Statistics = stats,
            IngestedFiles = ingestedFiles
        };

        // Ausgabedatei schreiben
        var json = JsonSerializer.Serialize(result, new JsonSerializerOptions
        {
            WriteIndented = true,
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase
        });
        await File.WriteAllTextAsync(_config.OutputFile, json);
        _logger.LogInformation("Output saved to: {OutputFile}", _config.OutputFile);

        return result;
    }
}
