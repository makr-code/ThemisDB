/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DsmInMemoryStores.cs                               ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     195                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Einfache In-Memory-Implementierungen der DSM-Store-Interfaces.
/// Hinweis: Nicht dauerhaft – dient als Stub/Adapter, bis ThemisDB-Adapter implementiert ist.
/// </summary>
public class DsmInMemoryMetadataStore : IDsmMetadataStore
{
    private readonly ConcurrentDictionary<string, DsmMetadataTemplate> _templates = new();
    private readonly ConcurrentDictionary<(string entityType, string entityId), List<DsmMetadataValue>> _values = new();

    public Task<DsmMetadataTemplate?> GetTemplateAsync(string templateId)
    {
        _templates.TryGetValue(templateId, out var t);
        return Task.FromResult(t);
    }

    public Task<IEnumerable<DsmMetadataTemplate>> ListTemplatesAsync(string? entityType = null)
    {
        var res = _templates.Values.AsEnumerable();
        if (!string.IsNullOrWhiteSpace(entityType))
        {
            res = res.Where(t => t.EntityTypes.Contains(entityType));
        }
        return Task.FromResult(res);
    }

    public Task<DsmMetadataTemplate> UpsertTemplateAsync(DsmMetadataTemplate template)
    {
        _templates[template.Id] = template;
        return Task.FromResult(template);
    }

    public Task<DsmMetadataValue?> GetCurrentValuesAsync(string entityType, string entityId)
    {
        var key = (entityType, entityId);
        if (_values.TryGetValue(key, out var list))
        {
            return Task.FromResult<DsmMetadataValue?>(list.OrderByDescending(v => v.Version).FirstOrDefault());
        }
        return Task.FromResult<DsmMetadataValue?>(null);
    }

    public Task<IEnumerable<DsmMetadataValue>> GetAllValuesAsync(string entityType, string entityId)
    {
        var key = (entityType, entityId);
        if (_values.TryGetValue(key, out var list))
        {
            return Task.FromResult<IEnumerable<DsmMetadataValue>>(list.OrderByDescending(v => v.Version));
        }
        return Task.FromResult<IEnumerable<DsmMetadataValue>>(Enumerable.Empty<DsmMetadataValue>());
    }

    public Task<DsmMetadataValue> UpsertValuesAsync(DsmMetadataValue value)
    {
        var key = (value.EntityType, value.EntityId);
        var list = _values.GetOrAdd(key, _ => new List<DsmMetadataValue>());

        // Markiere ältere Versionen als nicht current, neue als current
        foreach (var v in list)
        {
            if (v.IsCurrent) v.IsCurrent = false;
        }
        value.IsCurrent = true;

        list.Add(value);
        return Task.FromResult(value);
    }

    public Task AddAuditAsync(DsmMetadataAudit audit)
    {
        // In-Memory: nichts weiter zu tun; in DB-Version speichern
        return Task.CompletedTask;
    }
}

public class DsmInMemoryGraphStore : IDsmGraphStore
{
    private readonly ConcurrentBag<DsmEntityGraphEdge> _edges = new();

    public Task<DsmEntityGraphEdge> AddEdgeAsync(DsmEntityGraphEdge edge)
    {
        _edges.Add(edge);
        return Task.FromResult(edge);
    }

    public Task<IEnumerable<DsmEntityGraphEdge>> GetEdgesAsync(string entityType, string entityId)
    {
        var res = _edges.Where(e => (e.SourceType == entityType && e.SourceId == entityId) || (e.TargetType == entityType && e.TargetId == entityId));
        return Task.FromResult(res);
    }
}

public class DsmInMemoryVectorStore : IDsmVectorStore
{
    private readonly ConcurrentBag<DsmEntityVector> _vectors = new();

    public Task<DsmEntityVector> AddVectorAsync(DsmEntityVector vector)
    {
        _vectors.Add(vector);
        return Task.FromResult(vector);
    }

    public Task<IEnumerable<DsmEntityVector>> GetVectorsAsync(string entityType, string entityId)
    {
        var res = _vectors.Where(v => v.EntityType == entityType && v.EntityId == entityId);
        return Task.FromResult(res);
    }
}

public class DsmInMemoryGeoStore : IDsmGeoStore
{
    private readonly ConcurrentDictionary<(string entityType, string entityId), DsmEntityGeo> _geo = new();

    public Task<DsmEntityGeo> UpsertGeoAsync(DsmEntityGeo geo)
    {
        _geo[(geo.EntityType, geo.EntityId)] = geo;
        return Task.FromResult(geo);
    }

    public Task<DsmEntityGeo?> GetGeoAsync(string entityType, string entityId)
    {
        _geo.TryGetValue((entityType, entityId), out var g);
        return Task.FromResult(g);
    }
}

public class DsmInMemoryTimelineStore : IDsmTimelineStore
{
    private readonly ConcurrentBag<DsmTimelineItem> _items = new();

    public Task<DsmTimelineItem> AddItemAsync(DsmTimelineItem item)
    {
        _items.Add(item);
        return Task.FromResult(item);
    }

    public Task<IEnumerable<DsmTimelineItem>> GetItemsAsync(string entityType, string entityId)
    {
        var res = _items.Where(i => i.EntityType == entityType && i.EntityId == entityId)
                        .OrderBy(i => i.Timestamp)
                        .ToList()
                        .AsEnumerable();
        return Task.FromResult(res);
    }
}

public class DsmInMemoryProcessLinkStore : IDsmProcessLinkStore
{
    private readonly ConcurrentBag<DsmProcessLink> _links = new();

    public Task AddLinkAsync(DsmProcessLink link)
    {
        _links.Add(link);
        return Task.CompletedTask;
    }

    public Task<IEnumerable<DsmProcessLink>> GetLinksAsync(string processId)
    {
        var res = _links.Where(l => l.ProcessId == processId);
        return Task.FromResult(res);
    }
}
