/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Relationship.cs                                    ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:55:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace ThemisDB.AqlDiagramTool.Models;

/// <summary>
/// Represents a relationship between two entities
/// </summary>
public class Relationship
{
    public string Name { get; set; } = string.Empty;
    public Entity FromEntity { get; set; } = null!;
    public Entity ToEntity { get; set; } = null!;
    public RelationshipType Type { get; set; } = RelationshipType.OneToMany;
    public string Description { get; set; } = string.Empty;
    public string? ForeignKeyAttribute { get; set; }
    public string? EdgeCollection { get; set; }

    public Relationship() { }

    public Relationship(string name, Entity from, Entity to, RelationshipType type)
    {
        Name = name;
        FromEntity = from;
        ToEntity = to;
        Type = type;
    }
}

/// <summary>
/// Type of relationship between entities
/// </summary>
public enum RelationshipType
{
    OneToOne,
    OneToMany,
    ManyToOne,
    ManyToMany
}
