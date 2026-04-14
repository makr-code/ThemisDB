/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            JsonSchemaParser.cs                                ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     201                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text.Json;
using ThemisDB.AqlDiagramTool.Models;

namespace ThemisDB.AqlDiagramTool.Parsers;

/// <summary>
/// Parses database schema from JSON format
/// </summary>
public class JsonSchemaParser
{
    /// <summary>
    /// Parse schema from JSON string
    /// </summary>
    public DatabaseSchema Parse(string json)
    {
        try
        {
            var jsonDoc = JsonDocument.Parse(json);
            var root = jsonDoc.RootElement;
            
            var schema = new DatabaseSchema
            {
                Name = root.GetProperty("name").GetString() ?? "Database",
                Description = GetStringProperty(root, "description")
            };
            
            // Parse entities
            if (root.TryGetProperty("entities", out var entitiesElement))
            {
                foreach (var entityElement in entitiesElement.EnumerateArray())
                {
                    var entity = ParseEntity(entityElement);
                    schema.AddEntity(entity);
                }
            }
            
            // Parse relationships
            if (root.TryGetProperty("relationships", out var relationshipsElement))
            {
                foreach (var relElement in relationshipsElement.EnumerateArray())
                {
                    var relationship = ParseRelationship(relElement, schema);
                    if (relationship != null)
                    {
                        schema.AddRelationship(relationship);
                    }
                }
            }
            
            return schema;
        }
        catch (Exception ex)
        {
            throw new ArgumentException($"Failed to parse JSON schema: {ex.Message}", ex);
        }
    }

    /// <summary>
    /// Parse schema from JSON file
    /// </summary>
    public DatabaseSchema ParseFile(string filePath)
    {
        if (!File.Exists(filePath))
        {
            throw new FileNotFoundException($"Schema file not found: {filePath}");
        }
        
        var json = File.ReadAllText(filePath);
        return Parse(json);
    }

    private Entity ParseEntity(JsonElement element)
    {
        var entity = new Entity
        {
            Name = element.GetProperty("name").GetString() ?? "UnnamedEntity",
            Description = GetStringProperty(element, "description"),
            Type = ParseEntityType(GetStringProperty(element, "type"))
        };
        
        if (element.TryGetProperty("attributes", out var attributesElement))
        {
            foreach (var attrElement in attributesElement.EnumerateArray())
            {
                var attribute = ParseAttribute(attrElement);
                entity.AddAttribute(attribute);
            }
        }
        
        return entity;
    }

    private Models.Attribute ParseAttribute(JsonElement element)
    {
        return new Models.Attribute
        {
            Name = element.GetProperty("name").GetString() ?? "unnamed",
            DataType = GetStringProperty(element, "type", "string"),
            IsKey = GetBoolProperty(element, "isKey"),
            IsRequired = GetBoolProperty(element, "required"),
            IsUnique = GetBoolProperty(element, "unique"),
            DefaultValue = GetStringProperty(element, "default"),
            Description = GetStringProperty(element, "description")
        };
    }

    private Relationship? ParseRelationship(JsonElement element, DatabaseSchema schema)
    {
        var fromName = GetStringProperty(element, "from");
        var toName = GetStringProperty(element, "to");
        
        if (string.IsNullOrEmpty(fromName) || string.IsNullOrEmpty(toName))
        {
            return null;
        }
        
        var fromEntity = schema.GetEntity(fromName);
        var toEntity = schema.GetEntity(toName);
        
        if (fromEntity == null || toEntity == null)
        {
            Console.WriteLine($"Warning: Could not find entities for relationship from '{fromName}' to '{toName}'");
            return null;
        }
        
        return new Relationship
        {
            Name = GetStringProperty(element, "name", $"{fromName}_{toName}"),
            FromEntity = fromEntity,
            ToEntity = toEntity,
            Type = ParseRelationshipType(GetStringProperty(element, "type", "one-to-many")),
            Description = GetStringProperty(element, "description"),
            ForeignKeyAttribute = GetStringProperty(element, "foreignKey"),
            EdgeCollection = GetStringProperty(element, "edgeCollection")
        };
    }

    private EntityType ParseEntityType(string? type)
    {
        return type?.ToLower() switch
        {
            "edge" or "edgecollection" => EntityType.EdgeCollection,
            "view" => EntityType.View,
            "index" => EntityType.Index,
            _ => EntityType.Collection
        };
    }

    private RelationshipType ParseRelationshipType(string type)
    {
        return type.ToLower() switch
        {
            "one-to-one" or "1:1" => RelationshipType.OneToOne,
            "one-to-many" or "1:n" => RelationshipType.OneToMany,
            "many-to-one" or "n:1" => RelationshipType.ManyToOne,
            "many-to-many" or "n:m" or "m:n" => RelationshipType.ManyToMany,
            _ => RelationshipType.OneToMany
        };
    }

    private string GetStringProperty(JsonElement element, string propertyName, string defaultValue = "")
    {
        if (element.TryGetProperty(propertyName, out var prop))
        {
            return prop.GetString() ?? defaultValue;
        }
        return defaultValue;
    }

    private bool GetBoolProperty(JsonElement element, string propertyName, bool defaultValue = false)
    {
        if (element.TryGetProperty(propertyName, out var prop))
        {
            return prop.GetBoolean();
        }
        return defaultValue;
    }
}
