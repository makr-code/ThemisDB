/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Relationship.cs                                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     62                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
