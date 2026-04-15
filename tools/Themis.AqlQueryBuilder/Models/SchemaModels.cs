/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SchemaModels.cs                                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:19:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     94                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AqlQueryBuilder.Models;

/// <summary>
/// Represents a database collection/table in the schema explorer
/// </summary>
public class SchemaCollection
{
    public string Name { get; set; } = string.Empty;
    public CollectionType Type { get; set; }
    public List<SchemaField> Fields { get; set; } = new();
    public int EstimatedDocumentCount { get; set; }
    public bool HasVectorIndex { get; set; }
    public bool HasGeoIndex { get; set; }
    public bool HasGraphEdges { get; set; }
}

/// <summary>
/// Type of collection
/// </summary>
public enum CollectionType
{
    Relational,    // 📊 Standard table
    Graph,         // 🕸️ Graph nodes/edges
    Vector,        // 🔢 Has vector embeddings
    Geo,           // 📍 Spatial data
    Hybrid         // Multiple types
}

/// <summary>
/// Represents a field/column in a collection
/// </summary>
public class SchemaField
{
    public string Name { get; set; } = string.Empty;
    public FieldDataType DataType { get; set; }
    public bool IsIndexed { get; set; }
    public bool IsRequired { get; set; }
    public bool IsVectorField { get; set; }
    public bool IsGeoField { get; set; }
    public int? VectorDimension { get; set; }
}

/// <summary>
/// Field data types
/// </summary>
public enum FieldDataType
{
    String,
    Integer,
    Float,
    Boolean,
    Date,
    DateTime,
    Object,
    Array,
    Vector,
    GeoPoint,
    GeoPolygon,
    GeoLineString
}

/// <summary>
/// Query type selector
/// </summary>
public enum QueryType
{
    Relational,
    Graph,
    Vector,
    Geo,
    Hybrid
}
