using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Microsoft.Extensions.Logging;
using Themis.IngestionTool.Models;
using YamlDotNet.Serialization;

namespace Themis.IngestionTool.Services;

/// <summary>
/// Prozessor für Dateien - extrahiert Metadaten und generiert ThemisDB-Metadaten
/// </summary>
public class FileProcessor
{
    private readonly IngestionConfig _config;
    private readonly ILogger<FileProcessor> _logger;
    private static readonly Dictionary<string, string> MimeTypes = new()
    {
        { ".json", "application/json" },
        { ".yaml", "application/x-yaml" },
        { ".yml", "application/x-yaml" },
        { ".txt", "text/plain" },
        { ".md", "text/markdown" },
        { ".csv", "text/csv" },
        { ".xml", "application/xml" },
        { ".html", "text/html" },
        { ".htm", "text/html" },
        { ".pdf", "application/pdf" },
        { ".doc", "application/msword" },
        { ".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" }
    };

    public FileProcessor(IngestionConfig config, ILogger<FileProcessor> logger)
    {
        _config = config;
        _logger = logger;
    }

    public string ComputeHash(string filePath)
    {
        try
        {
            using var sha256 = SHA256.Create();
            using var stream = File.OpenRead(filePath);
            var hash = sha256.ComputeHash(stream);
            return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error computing hash for {FilePath}", filePath);
            return string.Empty;
        }
    }

    public string GetMimeType(string filePath)
    {
        var extension = Path.GetExtension(filePath).ToLowerInvariant();
        return MimeTypes.TryGetValue(extension, out var mimeType) 
            ? mimeType 
            : "application/octet-stream";
    }

    public FileMetadata? ProcessFile(string filePath)
    {
        try
        {
            // Hash berechnen
            var fileHash = ComputeHash(filePath);
            if (string.IsNullOrEmpty(fileHash))
                return null;

            // Dateiinformationen
            var fileInfo = new FileInfo(filePath);
            var mimeType = GetMimeType(filePath);

            // Basis-Metadaten extrahieren
            var metadata = new Dictionary<string, object>
            {
                ["file_name"] = fileInfo.Name,
                ["file_extension"] = fileInfo.Extension,
                ["created_time"] = fileInfo.CreationTimeUtc.ToString("O"),
                ["modified_time"] = fileInfo.LastWriteTimeUtc.ToString("O")
            };

            // Content laden
            var content = LoadContent(filePath);

            // ThemisDB-Metadaten generieren
            var themisMetadata = ExtractThemisMetadata(filePath, content, mimeType);

            return new FileMetadata
            {
                FilePath = filePath,
                FileHash = fileHash,
                FileSize = fileInfo.Length,
                MimeType = mimeType,
                IngestionTime = DateTime.UtcNow,
                Metadata = metadata,
                ThemisMetadata = themisMetadata
            };
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error processing file {FilePath}", filePath);
            return null;
        }
    }

    private object? LoadContent(string filePath)
    {
        var extension = Path.GetExtension(filePath).ToLowerInvariant();

        try
        {
            if (extension == ".json")
            {
                var text = File.ReadAllText(filePath);
                return JsonSerializer.Deserialize<object>(text);
            }
            else if (extension is ".yaml" or ".yml")
            {
                var text = File.ReadAllText(filePath);
                var deserializer = new DeserializerBuilder().Build();
                return deserializer.Deserialize<object>(text);
            }
            else if (extension is ".txt" or ".md" or ".log" or ".csv")
            {
                return File.ReadAllText(filePath);
            }

            return null;
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Could not load content from {FilePath}", filePath);
            return null;
        }
    }

    private ThemisMetadata ExtractThemisMetadata(string filePath, object? content, string mimeType)
    {
        return new ThemisMetadata
        {
            Graph = _config.GenerateGraphMetadata 
                ? ExtractGraphMetadata(filePath, content) 
                : null,
            Vector = _config.GenerateVectorMetadata 
                ? ExtractVectorMetadata(filePath, content, mimeType) 
                : null,
            Relational = _config.GenerateRelationalMetadata 
                ? ExtractRelationalMetadata(filePath, content, mimeType) 
                : null
        };
    }

    private GraphMetadata ExtractGraphMetadata(string filePath, object? content)
    {
        var fileName = Path.GetFileName(filePath);
        var metadata = new GraphMetadata
        {
            EntityType = "Document",
            EntityId = fileName,
            Properties = new Dictionary<string, object>
            {
                ["source_file"] = filePath,
                ["file_type"] = Path.GetExtension(filePath)
            },
            Relationships = new List<Relationship>()
        };

        // Prüfe auf Beziehungen in strukturierten Daten
        if (content is JsonElement jsonElement && jsonElement.ValueKind == JsonValueKind.Object)
        {
            foreach (var property in jsonElement.EnumerateObject())
            {
                var key = property.Name.ToLowerInvariant();
                if (key is "references" or "links" or "related" or "dependencies")
                {
                    var targets = new List<string>();
                    if (property.Value.ValueKind == JsonValueKind.Array)
                    {
                        foreach (var item in property.Value.EnumerateArray())
                        {
                            if (item.ValueKind == JsonValueKind.String)
                            {
                                targets.Add(item.GetString() ?? string.Empty);
                            }
                        }
                    }

                    metadata.Relationships.Add(new Relationship
                    {
                        Type = key,
                        Targets = targets
                    });
                }
            }
        }

        return metadata;
    }

    private VectorMetadata ExtractVectorMetadata(string filePath, object? content, string mimeType)
    {
        var fileName = Path.GetFileName(filePath);
        string? textContent = null;
        int contentLength = 0;

        if (content is string text)
        {
            contentLength = text.Length;
            textContent = _config.ExtractTextPreview && text.Length > _config.PreviewLength
                ? text[.._config.PreviewLength]
                : text;
        }
        else if (content is JsonElement jsonElement)
        {
            var jsonText = JsonSerializer.Serialize(jsonElement);
            contentLength = jsonText.Length;
            textContent = _config.ExtractTextPreview && jsonText.Length > _config.PreviewLength
                ? jsonText[.._config.PreviewLength]
                : jsonText;
        }

        return new VectorMetadata
        {
            ObjectName = "documents",
            DocumentId = fileName,
            ContentType = mimeType,
            EmbeddingRequired = true,
            TextContent = textContent,
            ContentLength = contentLength
        };
    }

    private RelationalMetadata ExtractRelationalMetadata(string filePath, object? content, string mimeType)
    {
        var fileName = Path.GetFileName(filePath);
        var schema = new Dictionary<string, string>
        {
            ["id"] = "TEXT PRIMARY KEY",
            ["file_path"] = "TEXT NOT NULL",
            ["file_name"] = "TEXT",
            ["content_type"] = "TEXT",
            ["ingestion_date"] = "TIMESTAMP"
        };

        var record = new Dictionary<string, object>
        {
            ["id"] = fileName,
            ["file_path"] = filePath,
            ["file_name"] = fileName,
            ["content_type"] = mimeType,
            ["ingestion_date"] = DateTime.UtcNow.ToString("O")
        };

        // Strukturierte Daten hinzufügen
        if (content is JsonElement jsonElement && jsonElement.ValueKind == JsonValueKind.Object)
        {
            foreach (var property in jsonElement.EnumerateObject())
            {
                var key = property.Name;
                var value = property.Value;

                if (value.ValueKind == JsonValueKind.String)
                {
                    schema[key] = "TEXT";
                    record[key] = value.GetString() ?? string.Empty;
                }
                else if (value.ValueKind == JsonValueKind.Number)
                {
                    if (value.TryGetInt64(out var intValue))
                    {
                        schema[key] = "INTEGER";
                        record[key] = intValue;
                    }
                    else if (value.TryGetDouble(out var doubleValue))
                    {
                        schema[key] = "REAL";
                        record[key] = doubleValue;
                    }
                }
                else if (value.ValueKind == JsonValueKind.True || value.ValueKind == JsonValueKind.False)
                {
                    schema[key] = "BOOLEAN";
                    record[key] = value.GetBoolean();
                }
            }
        }

        return new RelationalMetadata
        {
            TableName = "ingested_documents",
            Schema = schema,
            Record = record
        };
    }
}
