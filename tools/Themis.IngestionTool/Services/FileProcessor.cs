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
                : null,
            Geo = ExtractGeoMetadata(filePath, content),
            Process = ExtractProcessMetadata(filePath, content)
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

    private GeoMetadata? ExtractGeoMetadata(string filePath, object? content)
    {
        if (content is not JsonElement jsonElement || jsonElement.ValueKind != JsonValueKind.Object)
            return null;

        var geoFields = new Dictionary<string, List<string>>
        {
            ["latitude"] = new() { "latitude", "lat", "y", "coord_lat" },
            ["longitude"] = new() { "longitude", "lon", "lng", "x", "coord_lon", "coord_lng" },
            ["address"] = new() { "address", "addr", "street_address", "location" },
            ["city"] = new() { "city", "town", "municipality" },
            ["postal_code"] = new() { "postal_code", "zip", "zipcode", "plz", "postcode" },
            ["country"] = new() { "country", "nation", "land" },
            ["geometry"] = new() { "geometry", "_geometry", "geom", "shape" },
            ["coordinates"] = new() { "coordinates", "coords", "point" }
        };

        var geoMeta = new Dictionary<string, object>();
        bool foundGeo = false;

        foreach (var property in jsonElement.EnumerateObject())
        {
            var lowerKey = property.Name.ToLowerInvariant();
            foreach (var (fieldType, fieldNames) in geoFields)
            {
                if (fieldNames.Contains(lowerKey))
                {
                    geoMeta[fieldType] = property.Value.ValueKind switch
                    {
                        JsonValueKind.String => property.Value.GetString() ?? string.Empty,
                        JsonValueKind.Number => property.Value.GetDouble(),
                        JsonValueKind.Array => property.Value,
                        JsonValueKind.Object => property.Value,
                        _ => property.Value.ToString()
                    };
                    foundGeo = true;
                }
            }
        }

        if (!foundGeo)
            return null;

        var result = new Dictionary<string, object>();
        bool hasGeometry = false;
        var coordinateFields = new Dictionary<string, object>();
        var addressFields = new Dictionary<string, object>();

        // Extract coordinates
        if (geoMeta.ContainsKey("latitude") && geoMeta.ContainsKey("longitude"))
        {
            try
            {
                var lat = Convert.ToDouble(geoMeta["latitude"]);
                var lon = Convert.ToDouble(geoMeta["longitude"]);
                hasGeometry = true;
                coordinateFields["latitude"] = lat;
                coordinateFields["longitude"] = lon;
                coordinateFields["srid"] = 4326; // WGS84
                result["geometry_wkt"] = $"POINT({lon} {lat})";
            }
            catch { }
        }

        // Extract geometry field
        if (geoMeta.ContainsKey("geometry"))
        {
            var geom = geoMeta["geometry"];
            if (geom is string geomStr)
            {
                hasGeometry = true;
                result["geometry_raw"] = geomStr;
                if (geomStr.StartsWith("POINT", StringComparison.OrdinalIgnoreCase) ||
                    geomStr.StartsWith("LINESTRING", StringComparison.OrdinalIgnoreCase) ||
                    geomStr.StartsWith("POLYGON", StringComparison.OrdinalIgnoreCase))
                {
                    result["geometry_format"] = "WKT";
                }
                else if (geomStr.StartsWith("{") && geomStr.Contains("type"))
                {
                    result["geometry_format"] = "GeoJSON";
                }
            }
            else if (geom is JsonElement geomJson)
            {
                hasGeometry = true;
                result["geometry_raw"] = geomJson;
                result["geometry_format"] = "GeoJSON";
            }
        }

        // Extract coordinates array
        if (geoMeta.ContainsKey("coordinates") && geoMeta["coordinates"] is JsonElement coordsJson)
        {
            if (coordsJson.ValueKind == JsonValueKind.Array && coordsJson.GetArrayLength() >= 2)
            {
                try
                {
                    var arr = coordsJson.EnumerateArray().ToArray();
                    if (arr[0].ValueKind == JsonValueKind.Number)
                    {
                        var lon = arr[0].GetDouble();
                        var lat = arr[1].GetDouble();
                        hasGeometry = true;
                        coordinateFields["longitude"] = lon;
                        coordinateFields["latitude"] = lat;
                        coordinateFields["srid"] = 4326;
                        result["geometry_wkt"] = $"POINT({lon} {lat})";
                    }
                }
                catch { }
            }
        }

        // Extract address information
        foreach (var field in new[] { "address", "city", "postal_code", "country" })
        {
            if (geoMeta.ContainsKey(field))
            {
                addressFields[field] = geoMeta[field];
            }
        }

        if (addressFields.Count > 0)
        {
            var addressParts = new List<string>();
            if (addressFields.ContainsKey("address"))
                addressParts.Add(addressFields["address"].ToString() ?? "");
            if (addressFields.ContainsKey("postal_code"))
                addressParts.Add(addressFields["postal_code"].ToString() ?? "");
            if (addressFields.ContainsKey("city"))
                addressParts.Add(addressFields["city"].ToString() ?? "");
            if (addressFields.ContainsKey("country"))
                addressParts.Add(addressFields["country"].ToString() ?? "");
            result["full_address"] = string.Join(", ", addressParts.Where(p => !string.IsNullOrEmpty(p)));
        }

        if (!hasGeometry && addressFields.Count == 0)
            return null;

        return new GeoMetadata
        {
            HasGeometry = hasGeometry,
            CoordinateFields = coordinateFields,
            AddressFields = addressFields,
            GeometryWkt = result.ContainsKey("geometry_wkt") ? result["geometry_wkt"].ToString() : null,
            GeometryFormat = result.ContainsKey("geometry_format") ? result["geometry_format"].ToString() : null,
            GeometryRaw = result.ContainsKey("geometry_raw") ? result["geometry_raw"] : null,
            FullAddress = result.ContainsKey("full_address") ? result["full_address"].ToString() : null,
            SpatialIndexRequired = hasGeometry,
            IndexType = hasGeometry ? "R-Tree" : null
        };
    }

    private ProcessMetadata? ExtractProcessMetadata(string filePath, object? content)
    {
        if (content is not JsonElement jsonElement || jsonElement.ValueKind != JsonValueKind.Object)
            return null;

        var processIndicators = new Dictionary<string, List<string>>
        {
            ["state"] = new() { "state", "status", "_state", "current_state", "process_state" },
            ["activity"] = new() { "activity", "task", "action", "step", "phase" },
            ["case_id"] = new() { "case_id", "process_id", "instance_id", "workflow_id" },
            ["timestamp"] = new() { "timestamp", "time", "date", "created_at", "updated_at" },
            ["resource"] = new() { "resource", "user", "actor", "assignee", "owner" },
            ["variables"] = new() { "variables", "_variables", "data", "context" },
            ["tokens"] = new() { "tokens", "_tokens", "positions" },
            ["transitions"] = new() { "transitions", "edges", "flows" },
            ["process_type"] = new() { "type", "_type", "process_type", "workflow_type" }
        };

        var processMeta = new Dictionary<string, object>();
        bool foundProcess = false;

        foreach (var property in jsonElement.EnumerateObject())
        {
            var lowerKey = property.Name.ToLowerInvariant();
            foreach (var (fieldType, fieldNames) in processIndicators)
            {
                if (fieldNames.Contains(lowerKey))
                {
                    processMeta[fieldType] = property.Value.ValueKind switch
                    {
                        JsonValueKind.String => property.Value.GetString() ?? string.Empty,
                        JsonValueKind.Number => property.Value.GetInt64(),
                        JsonValueKind.Array => property.Value,
                        JsonValueKind.Object => property.Value,
                        JsonValueKind.True => true,
                        JsonValueKind.False => false,
                        _ => property.Value.ToString()
                    };
                    foundProcess = true;
                }
            }
        }

        if (!foundProcess)
            return null;

        var processFields = new Dictionary<string, object>();
        bool hasState = false, hasVariables = false, hasTokens = false;
        bool isProcessInstance = false;

        if (processMeta.ContainsKey("state"))
        {
            processFields["state"] = processMeta["state"];
            hasState = true;
        }

        if (processMeta.ContainsKey("activity"))
            processFields["activity"] = processMeta["activity"];

        if (processMeta.ContainsKey("case_id"))
        {
            processFields["case_id"] = processMeta["case_id"];
            isProcessInstance = true;
        }

        if (processMeta.ContainsKey("timestamp"))
            processFields["timestamp"] = processMeta["timestamp"];

        if (processMeta.ContainsKey("resource"))
            processFields["resource"] = processMeta["resource"];

        if (processMeta.ContainsKey("variables"))
        {
            processFields["variables"] = processMeta["variables"];
            hasVariables = true;
        }

        if (processMeta.ContainsKey("tokens"))
        {
            processFields["tokens"] = processMeta["tokens"];
            hasTokens = true;
        }

        // Check for BPMN fields
        bool isBpmn = false;
        var bpmnFields = new[] { "bpmn", "flowNode", "sequenceFlow", "gateway", "event", "task" };
        foreach (var property in jsonElement.EnumerateObject())
        {
            if (bpmnFields.Contains(property.Name, StringComparer.OrdinalIgnoreCase))
            {
                isBpmn = true;
                break;
            }
        }

        bool isStateMachine = processMeta.ContainsKey("transitions");

        bool processMiningReady = processMeta.ContainsKey("case_id") &&
                                  processMeta.ContainsKey("activity") &&
                                  processMeta.ContainsKey("timestamp");

        string? suggestedCollection = null;
        if (isProcessInstance)
            suggestedCollection = "_process_instances";
        else if (isBpmn)
            suggestedCollection = "_process_definitions";

        return new ProcessMetadata
        {
            IsProcessAware = true,
            ProcessFields = processFields,
            HasState = hasState,
            HasVariables = hasVariables,
            HasTokens = hasTokens,
            IsProcessInstance = isProcessInstance,
            IsBpmn = isBpmn,
            IsStateMachine = isStateMachine,
            ProcessType = processMeta.ContainsKey("process_type") ? processMeta["process_type"].ToString() : null,
            Format = isBpmn ? "BPMN" : null,
            ProcessMiningReady = processMiningReady,
            SuggestedCollection = suggestedCollection,
            Transitions = processMeta.ContainsKey("transitions") ? processMeta["transitions"] : null
        };
    }
}
