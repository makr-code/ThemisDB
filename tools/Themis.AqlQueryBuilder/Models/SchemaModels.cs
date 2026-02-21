/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SchemaModels.cs                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:46:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • efd303db0  2025-11-17  Add comprehensive multi-model design and enhanced UI with... ║
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
