/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AqlQueryHelper.cs                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     202                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text;
using ThemisDB.AqlDiagramTool.Models;

namespace ThemisDB.AqlDiagramTool.Generators;

/// <summary>
/// Generates AQL query templates and suggestions based on schema
/// </summary>
public class AqlQueryHelper
{
    /// <summary>
    /// Generate sample CRUD queries for an entity
    /// </summary>
    public List<string> GenerateCrudQueries(Entity entity)
    {
        var queries = new List<string>();
        var collectionName = entity.Name;
        
        // CREATE (INSERT)
        queries.Add($@"// Insert new {entity.Name}
INSERT {{
    {string.Join(",\n    ", entity.Attributes.Where(a => !a.IsKey).Select(a => $"{a.Name}: <{a.DataType}>"))}
}} INTO {collectionName}");
        
        // READ (SELECT)
        queries.Add($@"// Get all {entity.Name}
FOR doc IN {collectionName}
    RETURN doc");
        
        // READ with filter
        var firstAttr = entity.Attributes.FirstOrDefault();
        if (firstAttr != null)
        {
            queries.Add($@"// Filter {entity.Name} by {firstAttr.Name}
FOR doc IN {collectionName}
    FILTER doc.{firstAttr.Name} == <value>
    RETURN doc");
        }
        
        // UPDATE
        var keyAttr = entity.Attributes.FirstOrDefault(a => a.IsKey);
        var keyField = keyAttr?.Name ?? "_key";
        queries.Add($@"// Update {entity.Name}
UPDATE {{
    {keyField}: <key_value>
}} WITH {{
    {string.Join(",\n    ", entity.Attributes.Where(a => !a.IsKey).Take(2).Select(a => $"{a.Name}: <new_value>"))}
}} IN {collectionName}");
        
        // DELETE
        queries.Add($@"// Delete {entity.Name}
REMOVE {{
    {keyField}: <key_value>
}} IN {collectionName}");
        
        return queries;
    }

    /// <summary>
    /// Generate join queries based on relationships
    /// </summary>
    public List<string> GenerateJoinQueries(DatabaseSchema schema)
    {
        var queries = new List<string>();
        
        foreach (var relationship in schema.Relationships)
        {
            var fromEntity = relationship.FromEntity.Name;
            var toEntity = relationship.ToEntity.Name;
            var relName = relationship.Name;
            
            if (relationship.Type == RelationshipType.OneToMany || relationship.Type == RelationshipType.ManyToOne)
            {
                var fkAttr = relationship.ForeignKeyAttribute ?? $"{toEntity}_id";
                queries.Add($@"// Join {fromEntity} with {toEntity} ({relName})
FOR from_doc IN {fromEntity}
    FOR to_doc IN {toEntity}
        FILTER to_doc._key == from_doc.{fkAttr}
        RETURN {{
            {fromEntity}: from_doc,
            {toEntity}: to_doc
        }}");
            }
            else if (relationship.Type == RelationshipType.ManyToMany && !string.IsNullOrEmpty(relationship.EdgeCollection))
            {
                queries.Add($@"// Join {fromEntity} with {toEntity} via {relationship.EdgeCollection}
FOR from_doc IN {fromEntity}
    FOR edge IN {relationship.EdgeCollection}
        FILTER edge._from == from_doc._id
        FOR to_doc IN {toEntity}
            FILTER to_doc._id == edge._to
            RETURN {{
                {fromEntity}: from_doc,
                {toEntity}: to_doc,
                relationship: edge
            }}");
            }
        }
        
        return queries;
    }

    /// <summary>
    /// Generate graph traversal queries for related entities
    /// </summary>
    public List<string> GenerateGraphTraversalQueries(DatabaseSchema schema)
    {
        var queries = new List<string>();
        
        foreach (var entity in schema.Entities)
        {
            if (entity.Type == EntityType.EdgeCollection)
            {
                queries.Add($@"// Traverse outbound from a document via {entity.Name}
FOR vertex IN 1..3 OUTBOUND 'collection/document_key' {entity.Name}
    RETURN vertex");
                
                queries.Add($@"// Traverse inbound to a document via {entity.Name}
FOR vertex IN 1..3 INBOUND 'collection/document_key' {entity.Name}
    RETURN vertex");
            }
        }
        
        return queries;
    }

    /// <summary>
    /// Generate aggregation queries for entities
    /// </summary>
    public List<string> GenerateAggregationQueries(Entity entity)
    {
        var queries = new List<string>();
        var collectionName = entity.Name;
        
        // Count
        queries.Add($@"// Count {entity.Name}
RETURN COUNT(
    FOR doc IN {collectionName}
        RETURN 1
)");
        
        // Find numeric attributes for aggregation
        var numericTypes = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "int", "integer", "int32", "int64", "long", "short",
            "float", "double", "decimal", "number", "numeric"
        };
        var numericAttrs = entity.Attributes.Where(a => 
            numericTypes.Contains(a.DataType)
        ).ToList();
        
        if (numericAttrs.Any())
        {
            var numAttr = numericAttrs.First();
            queries.Add($@"// Aggregate {entity.Name} by {numAttr.Name}
FOR doc IN {collectionName}
    COLLECT
    AGGREGATE 
        total = SUM(doc.{numAttr.Name}),
        avg = AVG(doc.{numAttr.Name}),
        min = MIN(doc.{numAttr.Name}),
        max = MAX(doc.{numAttr.Name})
    RETURN {{ total, avg, min, max }}");
        }
        
        // Group by first non-key attribute
        var groupByAttr = entity.Attributes.FirstOrDefault(a => !a.IsKey);
        if (groupByAttr != null)
        {
            queries.Add($@"// Group {entity.Name} by {groupByAttr.Name}
FOR doc IN {collectionName}
    COLLECT group = doc.{groupByAttr.Name}
    AGGREGATE count = COUNT(1)
    RETURN {{ {groupByAttr.Name}: group, count: count }}");
        }
        
        return queries;
    }
}
