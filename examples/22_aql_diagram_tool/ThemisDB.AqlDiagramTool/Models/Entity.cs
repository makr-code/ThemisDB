/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Entity.cs                                          ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace ThemisDB.AqlDiagramTool.Models;

/// <summary>
/// Represents an entity in the database schema (Collection/Table)
/// </summary>
public class Entity
{
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public List<Attribute> Attributes { get; set; } = new();
    public EntityType Type { get; set; } = EntityType.Collection;

    public Entity() { }

    public Entity(string name, string description = "")
    {
        Name = name;
        Description = description;
    }

    public void AddAttribute(Attribute attribute)
    {
        Attributes.Add(attribute);
    }

    public void AddAttribute(string name, string dataType, bool isKey = false, bool isRequired = false)
    {
        Attributes.Add(new Attribute
        {
            Name = name,
            DataType = dataType,
            IsKey = isKey,
            IsRequired = isRequired
        });
    }
}

/// <summary>
/// Represents an attribute/field of an entity
/// </summary>
public class Attribute
{
    public string Name { get; set; } = string.Empty;
    public string DataType { get; set; } = string.Empty;
    public bool IsKey { get; set; }
    public bool IsRequired { get; set; }
    public bool IsUnique { get; set; }
    public string? DefaultValue { get; set; }
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// Type of entity in the database
/// </summary>
public enum EntityType
{
    Collection,
    EdgeCollection,
    View,
    Index
}
