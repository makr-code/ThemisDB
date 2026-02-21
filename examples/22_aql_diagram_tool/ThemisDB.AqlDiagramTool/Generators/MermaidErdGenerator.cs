/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MermaidErdGenerator.cs                             ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text;
using ThemisDB.AqlDiagramTool.Models;

namespace ThemisDB.AqlDiagramTool.Generators;

/// <summary>
/// Generates Entity-Relationship Diagrams in Mermaid format
/// </summary>
public class MermaidErdGenerator
{
    /// <summary>
    /// Generate an ERD from a database schema
    /// </summary>
    public string Generate(DatabaseSchema schema)
    {
        var sb = new StringBuilder();
        
        sb.AppendLine("erDiagram");
        
        // Generate entities with attributes
        foreach (var entity in schema.Entities)
        {
            sb.AppendLine($"    {SanitizeName(entity.Name)} {{");
            
            foreach (var attr in entity.Attributes)
            {
                var keyMarker = attr.IsKey ? "PK" : "";
                var requiredMarker = attr.IsRequired ? "NOT NULL" : "";
                var markers = string.Join(" ", new[] { keyMarker, requiredMarker }.Where(m => !string.IsNullOrEmpty(m)));
                
                sb.AppendLine($"        {attr.DataType} {SanitizeName(attr.Name)} {markers}".TrimEnd());
            }
            
            sb.AppendLine("    }");
        }
        
        // Generate relationships
        foreach (var rel in schema.Relationships)
        {
            var relationshipSymbol = GetRelationshipSymbol(rel.Type);
            var label = string.IsNullOrEmpty(rel.Name) ? "" : $" : \"{rel.Name}\"";
            
            sb.AppendLine($"    {SanitizeName(rel.FromEntity.Name)} {relationshipSymbol} {SanitizeName(rel.ToEntity.Name)}{label}");
        }
        
        return sb.ToString();
    }

    private string GetRelationshipSymbol(RelationshipType type)
    {
        return type switch
        {
            RelationshipType.OneToOne => "||--||",
            RelationshipType.OneToMany => "||--o{",
            RelationshipType.ManyToOne => "}o--||",
            RelationshipType.ManyToMany => "}o--o{",
            _ => "||--||"
        };
    }

    private string SanitizeName(string name)
    {
        // Replace invalid characters for Mermaid
        return name.Replace(" ", "_").Replace("-", "_");
    }
}
