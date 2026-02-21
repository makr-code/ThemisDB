/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MermaidErdGenerator.cs                             ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
