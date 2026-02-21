/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SchemaService.cs                                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     387                                            ║
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

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.ERDQueryEditor.Services;

/// <summary>
/// Service interface for database schema operations
/// </summary>
public interface ISchemaService
{
    Task<DatabaseSchema> GetSchemaAsync(CancellationToken cancellationToken = default);
    Task<List<EntityDefinition>> GetEntitiesAsync(CancellationToken cancellationToken = default);
    Task<EntityDefinition?> GetEntityAsync(string entityName, CancellationToken cancellationToken = default);
    Task RefreshSchemaAsync(CancellationToken cancellationToken = default);
}

/// <summary>
/// Service for introspecting and managing database schema
/// </summary>
public class SchemaService : ISchemaService
{
    private readonly IThemisApiClient _apiClient;
    private DatabaseSchema? _cachedSchema;
    private DateTime? _lastRefresh;
    private readonly TimeSpan _cacheExpiry = TimeSpan.FromMinutes(5);

    public SchemaService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient ?? throw new ArgumentNullException(nameof(apiClient));
    }

    public async Task<DatabaseSchema> GetSchemaAsync(CancellationToken cancellationToken = default)
    {
        // Return cached schema if still valid
        if (_cachedSchema != null && _lastRefresh != null && 
            DateTime.Now - _lastRefresh.Value < _cacheExpiry)
        {
            return _cachedSchema;
        }

        await RefreshSchemaAsync(cancellationToken);
        return _cachedSchema ?? CreateDefaultSchema();
    }

    public async Task<List<EntityDefinition>> GetEntitiesAsync(CancellationToken cancellationToken = default)
    {
        var schema = await GetSchemaAsync(cancellationToken);
        return schema.Entities.ToList();
    }

    public async Task<EntityDefinition?> GetEntityAsync(string entityName, CancellationToken cancellationToken = default)
    {
        var schema = await GetSchemaAsync(cancellationToken);
        return schema.Entities.FirstOrDefault(e => 
            e.Name.Equals(entityName, StringComparison.OrdinalIgnoreCase));
    }

    public async Task RefreshSchemaAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            // Try to get schema from ThemisDB
            // Since ThemisDB doesn't have a dedicated schema endpoint yet,
            // we'll infer the schema from known collections
            var schema = await InferSchemaFromCollectionsAsync(cancellationToken);
            
            _cachedSchema = schema;
            _lastRefresh = DateTime.Now;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Schema refresh failed: {ex.Message}");
            // Fall back to default schema if API call fails
            _cachedSchema = CreateDefaultSchema();
            _lastRefresh = DateTime.Now;
        }
    }

    private async Task<DatabaseSchema> InferSchemaFromCollectionsAsync(CancellationToken cancellationToken)
    {
        var schema = new DatabaseSchema
        {
            Name = "ThemisDB",
            LastUpdated = DateTime.Now
        };

        // Define known entities based on the DocumentManager implementation
        var entities = new List<EntityDefinition>
        {
            CreateDocumentEntity(),
            CreateDocumentRevisionEntity(),
            CreateTimelineEventEntity(),
            CreateUserEntity(),
            CreateProcessEntity(),
            CreateTaskEntity(),
            CreateFavoriteEntity(),
            CreateAuditLogEntity()
        };

        foreach (var entity in entities)
        {
            schema.Entities.Add(entity);
        }

        // Define relationships
        var relationships = new List<RelationshipDefinition>
        {
            new RelationshipDefinition
            {
                Name = "document_revisions",
                SourceEntityId = entities[0].Id, // Document
                TargetEntityId = entities[1].Id, // DocumentRevision
                SourceAttribute = "_key",
                TargetAttribute = "document_id",
                Type = RelationshipType.OneToMany,
                Description = "A document can have multiple revisions"
            },
            new RelationshipDefinition
            {
                Name = "document_timeline",
                SourceEntityId = entities[0].Id, // Document
                TargetEntityId = entities[2].Id, // TimelineEvent
                SourceAttribute = "_key",
                TargetAttribute = "entity_id",
                Type = RelationshipType.OneToMany,
                Description = "A document generates timeline events"
            },
            new RelationshipDefinition
            {
                Name = "user_documents",
                SourceEntityId = entities[3].Id, // User
                TargetEntityId = entities[0].Id, // Document
                SourceAttribute = "_key",
                TargetAttribute = "created_by",
                Type = RelationshipType.OneToMany,
                Description = "A user can create multiple documents"
            },
            new RelationshipDefinition
            {
                Name = "process_documents",
                SourceEntityId = entities[4].Id, // Process
                TargetEntityId = entities[0].Id, // Document
                SourceAttribute = "_key",
                TargetAttribute = "process_id",
                Type = RelationshipType.OneToMany,
                Description = "A process can contain multiple documents"
            },
            new RelationshipDefinition
            {
                Name = "user_favorites",
                SourceEntityId = entities[3].Id, // User
                TargetEntityId = entities[6].Id, // Favorite
                SourceAttribute = "_key",
                TargetAttribute = "user_id",
                Type = RelationshipType.OneToMany,
                Description = "A user can have multiple favorites"
            }
        };

        foreach (var relationship in relationships)
        {
            schema.Relationships.Add(relationship);
        }

        return schema;
    }

    private DatabaseSchema CreateDefaultSchema()
    {
        var schema = new DatabaseSchema
        {
            Name = "ThemisDB (Default)",
            LastUpdated = DateTime.Now
        };

        schema.Entities.Add(CreateDocumentEntity());
        schema.Entities.Add(CreateDocumentRevisionEntity());
        schema.Entities.Add(CreateTimelineEventEntity());

        return schema;
    }

    private EntityDefinition CreateDocumentEntity()
    {
        return new EntityDefinition
        {
            Name = "documents",
            Type = "Collection",
            Description = "Main document collection",
            X = 100, Y = 100,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Document ID" },
                new() { Name = "title", DataType = "string", IsRequired = true, Description = "Document title" },
                new() { Name = "file_path", DataType = "string", IsRequired = true, Description = "File system path" },
                new() { Name = "file_type", DataType = "string", Description = "File MIME type" },
                new() { Name = "created_at", DataType = "datetime", IsRequired = true, IsIndexed = true, Description = "Creation timestamp" },
                new() { Name = "modified_at", DataType = "datetime", IsRequired = true, IsIndexed = true, Description = "Last modification timestamp" },
                new() { Name = "created_by", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Creator user ID" },
                new() { Name = "tags", DataType = "array", Description = "Document tags" },
                new() { Name = "metadata", DataType = "object", Description = "Custom metadata" },
                new() { Name = "process_id", DataType = "string", IsIndexed = true, Description = "Associated process ID" }
            }
        };
    }

    private EntityDefinition CreateDocumentRevisionEntity()
    {
        return new EntityDefinition
        {
            Name = "document_revisions",
            Type = "Collection",
            Description = "Document revision history",
            X = 400, Y = 100,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Revision ID" },
                new() { Name = "document_id", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Parent document ID" },
                new() { Name = "revision_number", DataType = "integer", IsRequired = true, Description = "Revision sequence number" },
                new() { Name = "file_path", DataType = "string", IsRequired = true, Description = "Revision file path" },
                new() { Name = "file_hash", DataType = "string", IsRequired = true, Description = "SHA256 file hash" },
                new() { Name = "created_at", DataType = "datetime", IsRequired = true, IsIndexed = true, Description = "Revision timestamp" },
                new() { Name = "created_by", DataType = "string", IsRequired = true, Description = "User who created revision" },
                new() { Name = "comment", DataType = "string", Description = "Revision comment" }
            }
        };
    }

    private EntityDefinition CreateTimelineEventEntity()
    {
        return new EntityDefinition
        {
            Name = "timeline_events",
            Type = "Collection",
            Description = "Timeline events for documents and processes",
            X = 100, Y = 300,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Event ID" },
                new() { Name = "entity_type", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Type of entity (document, process, etc.)" },
                new() { Name = "entity_id", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Entity ID" },
                new() { Name = "event_type", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Event type (created, modified, etc.)" },
                new() { Name = "timestamp", DataType = "datetime", IsRequired = true, IsIndexed = true, Description = "Event timestamp" },
                new() { Name = "user", DataType = "string", IsRequired = true, Description = "User who triggered event" },
                new() { Name = "description", DataType = "string", Description = "Event description" },
                new() { Name = "metadata", DataType = "object", Description = "Additional event data" }
            }
        };
    }

    private EntityDefinition CreateUserEntity()
    {
        return new EntityDefinition
        {
            Name = "users",
            Type = "Collection",
            Description = "User accounts",
            X = 400, Y = 300,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "User ID" },
                new() { Name = "username", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Username" },
                new() { Name = "email", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Email address" },
                new() { Name = "full_name", DataType = "string", Description = "Full name" },
                new() { Name = "role", DataType = "string", IsRequired = true, Description = "User role" },
                new() { Name = "created_at", DataType = "datetime", IsRequired = true, Description = "Account creation date" }
            }
        };
    }

    private EntityDefinition CreateProcessEntity()
    {
        return new EntityDefinition
        {
            Name = "processes",
            Type = "Collection",
            Description = "Business processes",
            X = 700, Y = 100,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Process ID" },
                new() { Name = "name", DataType = "string", IsRequired = true, Description = "Process name" },
                new() { Name = "description", DataType = "string", Description = "Process description" },
                new() { Name = "status", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Process status" },
                new() { Name = "created_at", DataType = "datetime", IsRequired = true, Description = "Creation date" },
                new() { Name = "created_by", DataType = "string", IsRequired = true, Description = "Creator user ID" }
            }
        };
    }

    private EntityDefinition CreateTaskEntity()
    {
        return new EntityDefinition
        {
            Name = "tasks",
            Type = "Collection",
            Description = "Tasks and assignments",
            X = 700, Y = 300,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Task ID" },
                new() { Name = "title", DataType = "string", IsRequired = true, Description = "Task title" },
                new() { Name = "description", DataType = "string", Description = "Task description" },
                new() { Name = "status", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Task status" },
                new() { Name = "assignee", DataType = "string", IsIndexed = true, Description = "Assigned user ID" },
                new() { Name = "due_date", DataType = "datetime", IsIndexed = true, Description = "Due date" },
                new() { Name = "created_at", DataType = "datetime", IsRequired = true, Description = "Creation date" }
            }
        };
    }

    private EntityDefinition CreateFavoriteEntity()
    {
        return new EntityDefinition
        {
            Name = "favorites",
            Type = "Collection",
            Description = "User favorites",
            X = 1000, Y = 300,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Favorite ID" },
                new() { Name = "user_id", DataType = "string", IsRequired = true, IsIndexed = true, Description = "User ID" },
                new() { Name = "entity_type", DataType = "string", IsRequired = true, Description = "Favorited entity type" },
                new() { Name = "entity_id", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Favorited entity ID" },
                new() { Name = "created_at", DataType = "datetime", IsRequired = true, Description = "Favorite creation date" }
            }
        };
    }

    private EntityDefinition CreateAuditLogEntity()
    {
        return new EntityDefinition
        {
            Name = "audit_log",
            Type = "Collection",
            Description = "Audit trail for all operations",
            X = 100, Y = 500,
            Attributes = new System.Collections.ObjectModel.ObservableCollection<AttributeDefinition>
            {
                new() { Name = "_key", DataType = "string", IsPrimaryKey = true, IsRequired = true, Description = "Log entry ID" },
                new() { Name = "timestamp", DataType = "datetime", IsRequired = true, IsIndexed = true, Description = "Event timestamp" },
                new() { Name = "user", DataType = "string", IsRequired = true, IsIndexed = true, Description = "User who performed action" },
                new() { Name = "action", DataType = "string", IsRequired = true, IsIndexed = true, Description = "Action performed" },
                new() { Name = "entity_type", DataType = "string", IsIndexed = true, Description = "Affected entity type" },
                new() { Name = "entity_id", DataType = "string", IsIndexed = true, Description = "Affected entity ID" },
                new() { Name = "success", DataType = "boolean", IsRequired = true, IsIndexed = true, Description = "Whether action succeeded" },
                new() { Name = "details", DataType = "object", Description = "Additional details" }
            }
        };
    }
}


