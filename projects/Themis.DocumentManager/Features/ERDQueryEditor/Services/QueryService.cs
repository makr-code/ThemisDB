/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            QueryService.cs                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     358                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.ERDQueryEditor.Services;

/// <summary>
/// Service interface for query operations
/// </summary>
public interface IQueryService
{
    Task<QueryResult> ExecuteQueryAsync(string query, QueryLanguage language = QueryLanguage.AQL, CancellationToken cancellationToken = default);
    Task<List<SavedQuery>> GetSavedQueriesAsync(CancellationToken cancellationToken = default);
    Task<SavedQuery> SaveQueryAsync(SavedQuery query, CancellationToken cancellationToken = default);
    Task<bool> DeleteSavedQueryAsync(string queryId, CancellationToken cancellationToken = default);
    Task<string> ValidateQueryAsync(string query, QueryLanguage language = QueryLanguage.AQL);
}

/// <summary>
/// Service for managing and executing database queries
/// </summary>
public class QueryService : IQueryService
{
    private readonly IThemisApiClient _apiClient;
    private readonly string _savedQueriesPath;
    private List<SavedQuery> _savedQueries = new();

    public QueryService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient ?? throw new ArgumentNullException(nameof(apiClient));
        
        // Initialize saved queries storage
        var appDataPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "ThemisDB", "DocumentManager");
        
        Directory.CreateDirectory(appDataPath);
        _savedQueriesPath = Path.Combine(appDataPath, "saved_queries.json");
        
        LoadSavedQueries();
    }

    public async Task<QueryResult> ExecuteQueryAsync(string query, QueryLanguage language = QueryLanguage.AQL, CancellationToken cancellationToken = default)
    {
        var stopwatch = Stopwatch.StartNew();
        var result = new QueryResult
        {
            Query = query,
            Success = false
        };

        try
        {
            // Validate query first
            var validationError = await ValidateQueryAsync(query, language);
            if (!string.IsNullOrEmpty(validationError))
            {
                result.ErrorMessage = validationError;
                result.ExecutionTime = stopwatch.Elapsed;
                return result;
            }

            // Execute based on language
            switch (language)
            {
                case QueryLanguage.AQL:
                    result = await ExecuteAqlQueryAsync(query, cancellationToken);
                    break;
                case QueryLanguage.SQL:
                    result.ErrorMessage = "SQL queries are not yet supported. Please use AQL.";
                    break;
                case QueryLanguage.GraphQL:
                    result.ErrorMessage = "GraphQL queries are not yet supported. Please use AQL.";
                    break;
                default:
                    result.ErrorMessage = $"Unsupported query language: {language}";
                    break;
            }

            result.ExecutionTime = stopwatch.Elapsed;
        }
        catch (Exception ex)
        {
            result.Success = false;
            result.ErrorMessage = $"Query execution error: {ex.Message}";
            result.ExecutionTime = stopwatch.Elapsed;
        }

        return result;
    }

    private async Task<QueryResult> ExecuteAqlQueryAsync(string query, CancellationToken cancellationToken)
    {
        var result = new QueryResult
        {
            Query = query,
            Success = false
        };

        try
        {
            // Execute AQL query via ThemisDB API
            var response = await _apiClient.ExecuteAqlAsync<Dictionary<string, object>>(query, null, cancellationToken);
            
            if (response != null)
            {
                result.Results = response;
                result.RowCount = response.Count;
                result.Success = true;
            }
            else
            {
                result.ErrorMessage = "No results returned from query";
            }
        }
        catch (Exception ex)
        {
            result.ErrorMessage = $"AQL execution error: {ex.Message}";
        }

        return result;
    }

    public async Task<string> ValidateQueryAsync(string query, QueryLanguage language = QueryLanguage.AQL)
    {
        await Task.CompletedTask; // Keep async signature for future enhancements
        
        if (string.IsNullOrWhiteSpace(query))
        {
            return "Query cannot be empty";
        }

        switch (language)
        {
            case QueryLanguage.AQL:
                return ValidateAqlQuery(query);
            case QueryLanguage.SQL:
                return "SQL validation not yet implemented";
            case QueryLanguage.GraphQL:
                return "GraphQL validation not yet implemented";
            default:
                return $"Unknown query language: {language}";
        }
    }

    private string ValidateAqlQuery(string query)
    {
        var trimmedQuery = query.Trim();

        // Basic AQL syntax validation
        var validKeywords = new[] { "FOR", "RETURN", "FILTER", "SORT", "LIMIT", "COLLECT", "INSERT", "UPDATE", "REPLACE", "REMOVE", "UPSERT", "LET" };
        var firstWord = trimmedQuery.Split(' ', StringSplitOptions.RemoveEmptyEntries).FirstOrDefault()?.ToUpper();

        if (firstWord == null || !validKeywords.Contains(firstWord))
        {
            return $"Query must start with a valid AQL keyword: {string.Join(", ", validKeywords)}";
        }

        // Check for basic syntax errors
        var openBraces = trimmedQuery.Count(c => c == '{');
        var closeBraces = trimmedQuery.Count(c => c == '}');
        if (openBraces != closeBraces)
        {
            return "Mismatched braces in query";
        }

        var openParens = trimmedQuery.Count(c => c == '(');
        var closeParens = trimmedQuery.Count(c => c == ')');
        if (openParens != closeParens)
        {
            return "Mismatched parentheses in query";
        }

        // FOR queries should have RETURN
        if (firstWord == "FOR" && !trimmedQuery.ToUpper().Contains("RETURN"))
        {
            return "FOR queries must include a RETURN statement";
        }

        return string.Empty; // Valid
    }

    public async Task<List<SavedQuery>> GetSavedQueriesAsync(CancellationToken cancellationToken = default)
    {
        await Task.CompletedTask;
        return _savedQueries.ToList();
    }

    public async Task<SavedQuery> SaveQueryAsync(SavedQuery query, CancellationToken cancellationToken = default)
    {
        // Update existing or add new
        var existing = _savedQueries.FirstOrDefault(q => q.Id == query.Id);
        if (existing != null)
        {
            existing.Name = query.Name;
            existing.Description = query.Description;
            existing.QueryText = query.QueryText;
            existing.Language = query.Language;
            existing.LastModified = DateTime.Now;
        }
        else
        {
            query.Id = Guid.NewGuid().ToString();
            query.Created = DateTime.Now;
            query.LastModified = DateTime.Now;
            _savedQueries.Add(query);
        }

        await PersistSavedQueries();
        return existing ?? query;
    }

    public async Task<bool> DeleteSavedQueryAsync(string queryId, CancellationToken cancellationToken = default)
    {
        var query = _savedQueries.FirstOrDefault(q => q.Id == queryId);
        if (query != null)
        {
            _savedQueries.Remove(query);
            await PersistSavedQueries();
            return true;
        }
        return false;
    }

    private void LoadSavedQueries()
    {
        try
        {
            if (File.Exists(_savedQueriesPath))
            {
                var json = File.ReadAllText(_savedQueriesPath);
                _savedQueries = JsonSerializer.Deserialize<List<SavedQuery>>(json) ?? new List<SavedQuery>();
            }
            else
            {
                // Create some example queries
                _savedQueries = CreateExampleQueries();
                // Persist asynchronously with error handling
                Task.Run(async () =>
                {
                    try
                    {
                        await PersistSavedQueries();
                    }
                    catch (Exception persistEx)
                    {
                        System.Diagnostics.Debug.WriteLine($"Error persisting default queries: {persistEx.Message}");
                    }
                });
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error loading saved queries: {ex.Message}");
            _savedQueries = CreateExampleQueries();
        }
    }

    private async Task PersistSavedQueries()
    {
        try
        {
            var options = new JsonSerializerOptions { WriteIndented = true };
            var json = JsonSerializer.Serialize(_savedQueries, options);
            await File.WriteAllTextAsync(_savedQueriesPath, json);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error saving queries: {ex.Message}");
        }
    }

    private List<SavedQuery> CreateExampleQueries()
    {
        return new List<SavedQuery>
        {
            new SavedQuery
            {
                Name = "All Documents",
                Description = "Retrieve all documents from the database",
                QueryText = "FOR doc IN documents\n  SORT doc.created_at DESC\n  LIMIT 100\n  RETURN doc",
                Language = QueryLanguage.AQL
            },
            new SavedQuery
            {
                Name = "Recent Documents",
                Description = "Get documents created in the last 7 days",
                QueryText = "FOR doc IN documents\n  FILTER doc.created_at >= DATE_SUBTRACT(DATE_NOW(), 7, 'days')\n  SORT doc.created_at DESC\n  RETURN doc",
                Language = QueryLanguage.AQL
            },
            new SavedQuery
            {
                Name = "Documents by Tag",
                Description = "Find documents with a specific tag",
                QueryText = "FOR doc IN documents\n  FILTER 'important' IN doc.tags\n  RETURN { id: doc._key, title: doc.title, tags: doc.tags }",
                Language = QueryLanguage.AQL
            },
            new SavedQuery
            {
                Name = "Document with Revisions",
                Description = "Get a document with all its revisions",
                QueryText = "FOR doc IN documents\n  LET revisions = (\n    FOR rev IN document_revisions\n      FILTER rev.document_id == doc._key\n      SORT rev.revision_number DESC\n      RETURN rev\n  )\n  LIMIT 10\n  RETURN { document: doc, revisions: revisions }",
                Language = QueryLanguage.AQL
            },
            new SavedQuery
            {
                Name = "User Activity",
                Description = "Get timeline events for a specific user",
                QueryText = "FOR event IN timeline_events\n  FILTER event.user == 'current_user'\n  SORT event.timestamp DESC\n  LIMIT 50\n  RETURN event",
                Language = QueryLanguage.AQL
            },
            new SavedQuery
            {
                Name = "Documents by Process",
                Description = "Get all documents for a specific process",
                QueryText = "FOR doc IN documents\n  FILTER doc.process_id == 'PROCESS_ID_HERE'\n  SORT doc.created_at DESC\n  RETURN doc",
                Language = QueryLanguage.AQL
            }
        };
    }
}


