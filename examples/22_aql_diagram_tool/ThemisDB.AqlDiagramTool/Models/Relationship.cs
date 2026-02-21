/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Relationship.cs                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:31:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 44d16f6b5  2025-12-31  Add AQL Diagram Tool with ERD, ER, and DFD generators ║
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
