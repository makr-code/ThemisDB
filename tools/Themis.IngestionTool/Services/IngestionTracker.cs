using System.Data;
using System.Data.SQLite;
using System.Text.Json;
using Microsoft.Extensions.Logging;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services;

/// <summary>
/// SQLite-basierter Tracker für ingestierte Dateien
/// </summary>
public class IngestionTracker : IDisposable
{
    private readonly string _dbPath;
    private readonly ILogger<IngestionTracker> _logger;
    private SQLiteConnection? _connection;

    public IngestionTracker(string dbPath, ILogger<IngestionTracker> logger)
    {
        _dbPath = dbPath;
        _logger = logger;
        InitializeDatabase();
    }

    private void InitializeDatabase()
    {
        _connection = new SQLiteConnection($"Data Source={_dbPath};Version=3;");
        _connection.Open();

        var createTableCommand = _connection.CreateCommand();
        createTableCommand.CommandText = @"
            CREATE TABLE IF NOT EXISTS ingested_files (
                file_path TEXT PRIMARY KEY,
                file_hash TEXT NOT NULL,
                file_size INTEGER,
                mime_type TEXT,
                ingestion_time TEXT,
                metadata TEXT,
                themis_metadata TEXT
            )";
        createTableCommand.ExecuteNonQuery();

        var createIndexCommand = _connection.CreateCommand();
        createIndexCommand.CommandText = @"
            CREATE INDEX IF NOT EXISTS idx_file_hash ON ingested_files(file_hash)";
        createIndexCommand.ExecuteNonQuery();

        _logger.LogInformation("Ingestion tracker database initialized: {DbPath}", _dbPath);
    }

    public bool IsIngested(string filePath, string fileHash)
    {
        if (_connection == null) return false;

        var command = _connection.CreateCommand();
        command.CommandText = @"
            SELECT file_hash FROM ingested_files 
            WHERE file_path = @filePath OR file_hash = @fileHash";
        command.Parameters.AddWithValue("@filePath", filePath);
        command.Parameters.AddWithValue("@fileHash", fileHash);

        using var reader = command.ExecuteReader();
        return reader.Read();
    }

    public void AddFile(FileMetadata fileMetadata)
    {
        if (_connection == null) return;

        var command = _connection.CreateCommand();
        command.CommandText = @"
            INSERT OR REPLACE INTO ingested_files 
            (file_path, file_hash, file_size, mime_type, ingestion_time, metadata, themis_metadata)
            VALUES (@filePath, @fileHash, @fileSize, @mimeType, @ingestionTime, @metadata, @themisMetadata)";
        
        command.Parameters.AddWithValue("@filePath", fileMetadata.FilePath);
        command.Parameters.AddWithValue("@fileHash", fileMetadata.FileHash);
        command.Parameters.AddWithValue("@fileSize", fileMetadata.FileSize);
        command.Parameters.AddWithValue("@mimeType", fileMetadata.MimeType);
        command.Parameters.AddWithValue("@ingestionTime", fileMetadata.IngestionTime.ToString("O"));
        command.Parameters.AddWithValue("@metadata", JsonSerializer.Serialize(fileMetadata.Metadata));
        command.Parameters.AddWithValue("@themisMetadata", JsonSerializer.Serialize(fileMetadata.ThemisMetadata));

        command.ExecuteNonQuery();
    }

    public (int TotalFiles, long TotalSizeBytes) GetStats()
    {
        if (_connection == null) return (0, 0);

        var command = _connection.CreateCommand();
        command.CommandText = @"
            SELECT COUNT(*) as count, COALESCE(SUM(file_size), 0) as size 
            FROM ingested_files";

        using var reader = command.ExecuteReader();
        if (reader.Read())
        {
            return (reader.GetInt32(0), reader.GetInt64(1));
        }

        return (0, 0);
    }

    public void Dispose()
    {
        _connection?.Close();
        _connection?.Dispose();
    }
}
