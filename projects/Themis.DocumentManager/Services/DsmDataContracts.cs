/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DsmDataContracts.cs                                ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Datenmodelle für dynamische Metadaten, Graph, Vektor, Geo, Timeline, Prozesse – passend zum DSM-Entwurf.
/// Diese Klassen dienen als Contracts für Persistenzservices (ThemisDB) und können auf Collections gemappt werden.
/// </summary>
public static class DsmContracts { }

#region Metadata Templates / Values / Audit

public class DsmMetadataTemplate
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public int Version { get; set; }
    public List<string> EntityTypes { get; set; } = new();
    public List<DsmMetadataFieldDescriptor> Fields { get; set; } = new();
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
}

public class DsmMetadataFieldDescriptor
{
    public string Key { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public string DataType { get; set; } = "string"; // string|number|bool|date|datetime|list|json|lookup:<key>|ref:<entity>|file
    public bool IsRequired { get; set; }
    public Dictionary<string, object> Options { get; set; } = new(); // e.g. lookup values, min/max, regex
    public string? Group { get; set; }
    public int Order { get; set; }
}

public class DsmMetadataValue
{
    public string Id { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public string? TemplateId { get; set; }
    public int Version { get; set; } = 1;
    public Dictionary<string, object> Values { get; set; } = new();
    public bool IsCurrent { get; set; } = true;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
    public DateTime? UpdatedAt { get; set; }
    public string? UpdatedBy { get; set; }
}

public class DsmMetadataAudit
{
    public string Id { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public int Version { get; set; }
    public DateTime ChangedAt { get; set; } = DateTime.UtcNow;
    public string ChangedBy { get; set; } = string.Empty;
    public string DiffJson { get; set; } = string.Empty; // JSON Patch oder Diff
}

#endregion

#region Graph

public class DsmEntityGraphEdge
{
    public string Id { get; set; } = string.Empty;
    public string SourceType { get; set; } = string.Empty;
    public string SourceId { get; set; } = string.Empty;
    public string TargetType { get; set; } = string.Empty;
    public string TargetId { get; set; } = string.Empty;
    public string Relation { get; set; } = string.Empty; // references/derives_from/is_part_of/blocks/duplicates
    public double Weight { get; set; } = 1.0;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public Dictionary<string, object> Properties { get; set; } = new();
}

#endregion

#region Vector

public class DsmEntityVector
{
    public string Id { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public string Model { get; set; } = string.Empty;
    public int Dimensions { get; set; }
    public float[] Embedding { get; set; } = Array.Empty<float>();
    public string? ChunkId { get; set; }
    public (int Start, int End)? ChunkRange { get; set; }
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

#endregion

#region Geo

public class DsmEntityGeo
{
    public string Id { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public string GeoJson { get; set; } = string.Empty; // Point/Polygon
    public int Srid { get; set; } = 4326;
    public DateTime UpdatedAt { get; set; } = DateTime.UtcNow;
}

#endregion

#region Timeline

public class DsmTimelineItem
{
    public string Id { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; }
    public string Icon { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty; // milestone/statuschange/note
    public string? ProcessId { get; set; }
    public string PayloadJson { get; set; } = string.Empty;
}

#endregion

#region Prozesse / Links

public class DsmProcessLink
{
    public string ProcessId { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
}

public class DsmProcessStep
{
    public string Id { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string StepKey { get; set; } = string.Empty;
    public string Status { get; set; } = string.Empty;
    public string? Assignee { get; set; }
    public DateTime? StartedAt { get; set; }
    public DateTime? EndedAt { get; set; }
    public string PayloadJson { get; set; } = string.Empty;
}

#endregion

#region Service-Schnittstellen (Persistenz-Contracts)

public interface IDsmMetadataStore
{
    Task<DsmMetadataTemplate?> GetTemplateAsync(string templateId);
    Task<IEnumerable<DsmMetadataTemplate>> ListTemplatesAsync(string? entityType = null);
    Task<DsmMetadataTemplate> UpsertTemplateAsync(DsmMetadataTemplate template);

    Task<DsmMetadataValue?> GetCurrentValuesAsync(string entityType, string entityId);
    Task<IEnumerable<DsmMetadataValue>> GetAllValuesAsync(string entityType, string entityId);
    Task<DsmMetadataValue> UpsertValuesAsync(DsmMetadataValue value);

    Task AddAuditAsync(DsmMetadataAudit audit);
}

public interface IDsmGraphStore
{
    Task<DsmEntityGraphEdge> AddEdgeAsync(DsmEntityGraphEdge edge);
    Task<IEnumerable<DsmEntityGraphEdge>> GetEdgesAsync(string entityType, string entityId);
}

public interface IDsmVectorStore
{
    Task<DsmEntityVector> AddVectorAsync(DsmEntityVector vector);
    Task<IEnumerable<DsmEntityVector>> GetVectorsAsync(string entityType, string entityId);
}

public interface IDsmGeoStore
{
    Task<DsmEntityGeo> UpsertGeoAsync(DsmEntityGeo geo);
    Task<DsmEntityGeo?> GetGeoAsync(string entityType, string entityId);
}

public interface IDsmTimelineStore
{
    Task<DsmTimelineItem> AddItemAsync(DsmTimelineItem item);
    Task<IEnumerable<DsmTimelineItem>> GetItemsAsync(string entityType, string entityId);
}

public interface IDsmProcessLinkStore
{
    Task AddLinkAsync(DsmProcessLink link);
    Task<IEnumerable<DsmProcessLink>> GetLinksAsync(string processId);
}

#endregion
