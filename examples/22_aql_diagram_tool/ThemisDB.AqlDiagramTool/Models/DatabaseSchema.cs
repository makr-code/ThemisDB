/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DatabaseSchema.cs                                  ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace ThemisDB.AqlDiagramTool.Models;

/// <summary>
/// Represents a complete database schema with entities and relationships
/// </summary>
public class DatabaseSchema
{
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public List<Entity> Entities { get; set; } = new();
    public List<Relationship> Relationships { get; set; } = new();

    public DatabaseSchema() { }

    public DatabaseSchema(string name, string description = "")
    {
        Name = name;
        Description = description;
    }

    public void AddEntity(Entity entity)
    {
        Entities.Add(entity);
    }

    public void AddRelationship(Relationship relationship)
    {
        Relationships.Add(relationship);
    }

    public Entity? GetEntity(string name)
    {
        return Entities.FirstOrDefault(e => e.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
    }
}
