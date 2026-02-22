/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DatabaseSchema.cs                                  ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-22 11:29:15                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c97d719  2026-02-22  Add parallel multi-source BFS/DFS implementation (graph/p... ║
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
