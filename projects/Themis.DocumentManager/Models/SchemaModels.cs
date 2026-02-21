/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SchemaModels.cs                                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     138                                            ║
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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Represents a database schema containing entities and relationships
/// </summary>
public class DatabaseSchema
{
    public string Name { get; set; } = "ThemisDB";
    public ObservableCollection<EntityDefinition> Entities { get; set; } = new();
    public ObservableCollection<RelationshipDefinition> Relationships { get; set; } = new();
    public DateTime LastUpdated { get; set; } = DateTime.Now;
}

/// <summary>
/// Represents an entity (table/collection) in the database schema
/// </summary>
public class EntityDefinition
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = "Collection"; // Collection, Table, Document
    public ObservableCollection<AttributeDefinition> Attributes { get; set; } = new();
    public string Description { get; set; } = string.Empty;
    
    // Visual positioning for ERD
    public double X { get; set; }
    public double Y { get; set; }
    public double Width { get; set; } = 200;
    public double Height { get; set; } = 100;
}

/// <summary>
/// Represents an attribute (field/column) of an entity
/// </summary>
public class AttributeDefinition
{
    public string Name { get; set; } = string.Empty;
    public string DataType { get; set; } = "string";
    public bool IsPrimaryKey { get; set; }
    public bool IsRequired { get; set; }
    public bool IsIndexed { get; set; }
    public string? DefaultValue { get; set; }
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// Represents a relationship between entities
/// </summary>
public class RelationshipDefinition
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public string SourceEntityId { get; set; } = string.Empty;
    public string TargetEntityId { get; set; } = string.Empty;
    public string SourceAttribute { get; set; } = string.Empty;
    public string TargetAttribute { get; set; } = string.Empty;
    public RelationshipType Type { get; set; } = RelationshipType.OneToMany;
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// Types of relationships between entities
/// </summary>
public enum RelationshipType
{
    OneToOne,
    OneToMany,
    ManyToOne,
    ManyToMany
}

/// <summary>
/// Represents a saved query
/// </summary>
public class SavedQuery
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string QueryText { get; set; } = string.Empty;
    public QueryLanguage Language { get; set; } = QueryLanguage.AQL;
    public DateTime Created { get; set; } = DateTime.Now;
    public DateTime LastModified { get; set; } = DateTime.Now;
    public string CreatedBy { get; set; } = Environment.UserName;
}

/// <summary>
/// Query language types
/// </summary>
public enum QueryLanguage
{
    AQL,        // ArangoDB Query Language (ThemisDB uses this)
    SQL,        // For relational queries
    GraphQL     // For graph queries
}

/// <summary>
/// Query execution result
/// </summary>
public class QueryResult
{
    public bool Success { get; set; }
    public string? ErrorMessage { get; set; }
    public List<Dictionary<string, object>> Results { get; set; } = new();
    public int RowCount { get; set; }
    public TimeSpan ExecutionTime { get; set; }
    public string Query { get; set; } = string.Empty;
}
