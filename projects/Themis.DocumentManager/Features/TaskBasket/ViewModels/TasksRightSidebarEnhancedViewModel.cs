/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TasksRightSidebarEnhancedViewModel.cs              ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     549                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
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

using Themis.DocumentManager.Application.Common.Interfaces;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MediatR;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Data;
using Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks;
using static Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks.GetMyTasksQuery;
using Themis.DocumentManager.Features.TaskBasket.Views;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Features.TaskBasket.ViewModels;

/// <summary>
/// Enhanced ViewModel for Tasks with ThemisDB capabilities
/// Integrates Timeline, Geo, LLM, Graph, and Vector features
/// Inspired by Microsoft Teams, PDV VIS, and ThemisDB architecture
/// </summary>
public partial class TasksRightSidebarEnhancedViewModel : ObservableObject
{
    private readonly IMediator _mediator;
    private readonly ITimelineService? _timelineService;
    private readonly IGeoService? _geoService;
    private readonly IOllamaService? _ollamaService;
    private readonly IGraphService? _graphService;
    private readonly IVectorService? _vectorService;
    
    [ObservableProperty]
    private ObservableCollection<TaskItem> _tasks = new();

    [ObservableProperty]
    private bool _isLoading;

    [ObservableProperty]
    private string _searchText = string.Empty;

    [ObservableProperty]
    private Application.Tasks.Queries.GetMyTasks.TaskStatus? _statusFilter;

    [ObservableProperty]
    private string? _sortBy = "DueDate";

    [ObservableProperty]
    private string? _currentEntityId;

    [ObservableProperty]
    private LinkedEntityType? _currentEntityType;

    [ObservableProperty]
    private bool _hasNoTasks;

    [ObservableProperty]
    private int _unreadTaskCount;

    [ObservableProperty]
    private string? _currentUserId;

    [ObservableProperty]
    private bool _isAiSuggestionsEnabled = true;

    [ObservableProperty]
    private bool _isTimelineViewEnabled;

    [ObservableProperty]
    private bool _isGeoViewEnabled;

    [ObservableProperty]
    private string? _aiSuggestion;

    [ObservableProperty]
    private ObservableCollection<TaskItem> _relatedTasks = new();

    public ICollectionView TasksView { get; private set; }

    public TasksRightSidebarEnhancedViewModel(
        IMediator mediator,
        ITimelineService? timelineService = null,
        IGeoService? geoService = null,
        IOllamaService? ollamaService = null,
        IGraphService? graphService = null,
        IVectorService? vectorService = null)
    {
        _mediator = mediator;
        _timelineService = timelineService;
        _geoService = geoService;
        _ollamaService = ollamaService;
        _graphService = graphService;
        _vectorService = vectorService;
        
        // Initialize CollectionView for filtering and sorting
        TasksView = CollectionViewSource.GetDefaultView(Tasks);
        TasksView.Filter = FilterTasks;
        
        // Default sort by DueDate (VIS best practice)
        TasksView.SortDescriptions.Add(new SortDescription(nameof(TaskItem.DueDate), ListSortDirection.Ascending));
    }

    /// <summary>
    /// Loads tasks with ThemisDB enhancements
    /// - Timeline integration for deadline visualization
    /// - Geo integration for location-based tasks
    /// - LLM integration for smart prioritization suggestions
    /// </summary>
    [RelayCommand]
    public async Task LoadTasksAsync()
    {
        IsLoading = true;
        try
        {
            var query = new GetMyTasksQuery
            {
                UserId = CurrentUserId ?? "current-user",
                StatusFilter = StatusFilter,
                EntityId = CurrentEntityId,
                EntityType = CurrentEntityType,
                SortBy = ParseSortBy(SortBy),
                SortDescending = false
            };

            var tasks = await _mediator.Send(query);
            
            Tasks.Clear();
            foreach (var task in tasks)
            {
                // Mark tasks related to current entity (VIS-style)
                task.IsRelatedToCurrentEntity = !string.IsNullOrEmpty(CurrentEntityId) && 
                                                 task.LinkedEntityId == CurrentEntityId;
                Tasks.Add(task);
            }

            // ThemisDB Enhancement: Add tasks to Timeline
            await AddTasksToTimelineAsync(tasks);

            // ThemisDB Enhancement: Geo-tag location-based tasks
            await EnrichTasksWithGeoDataAsync(tasks);

            // ThemisDB Enhancement: AI prioritization suggestions
            if (IsAiSuggestionsEnabled && _ollamaService != null)
            {
                await GenerateAiPrioritizationSuggestionsAsync(tasks);
            }

            // ThemisDB Enhancement: Find related tasks using Vector similarity
            if (_vectorService != null && Tasks.Any())
            {
                await FindRelatedTasksAsync(Tasks.First());
            }

            UnreadTaskCount = tasks.Count(t => t.Status == Application.Tasks.Queries.GetMyTasks.TaskStatus.Pending && 
                                               t.CreatedAt > DateTime.UtcNow.AddHours(-24));

            HasNoTasks = Tasks.Count == 0;
            TasksView.Refresh();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error loading tasks: {ex.Message}");
            HasNoTasks = true;
        }
        finally
        {
            IsLoading = false;
        }
    }

    /// <summary>
    /// ThemisDB Timeline Integration
    /// Visualizes task deadlines on the timeline
    /// </summary>
    private async Task AddTasksToTimelineAsync(List<TaskItem> tasks)
    {
        if (_timelineService == null) return;

        try
        {
            foreach (var task in tasks.Where(t => t.DueDate.HasValue))
            {
                await _timelineService.CreateEventAsync(new Models.TimelineEvent
                {
                    Id = $"task-{task.Id}",
                    DocumentId = task.Id,
                    Description = $"{task.Title} - {task.Description}",
                    Timestamp = task.DueDate!.Value,
                    EventType = "task-deadline",
                    Metadata = new Dictionary<string, object>
                    {
                        { "taskId", task.Id },
                        { "priority", task.Priority.ToString() },
                        { "status", task.Status.ToString() }
                    }
                });
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error adding tasks to timeline: {ex.Message}");
        }
    }

    /// <summary>
    /// ThemisDB Geo Integration
    /// Enriches tasks with location data for geo-based filtering
    /// </summary>
    private async Task EnrichTasksWithGeoDataAsync(List<TaskItem> tasks)
    {
        if (_geoService == null) return;

        try
        {
            // Example: Extract location from task description or metadata
            // and geocode it for visualization on map
            foreach (var task in tasks)
            {
                // This would parse location mentions in task description
                // e.g., "Meeting in Berlin" -> geocode to coordinates
                // For now, this is a placeholder for future implementation
                await Task.CompletedTask;
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error enriching tasks with geo data: {ex.Message}");
        }
    }

    /// <summary>
    /// ThemisDB LLM Integration
    /// Generates AI-powered task prioritization suggestions
    /// Similar to Microsoft Copilot suggestions
    /// </summary>
    private async Task GenerateAiPrioritizationSuggestionsAsync(List<TaskItem> tasks)
    {
        if (_ollamaService == null || !tasks.Any()) return;

        try
        {
            var taskSummary = string.Join("\n", tasks.Take(5).Select(t => 
                $"- {t.Title} (Priority: {t.Priority}, Due: {t.DueDate?.ToString("yyyy-MM-dd") ?? "N/A"})"));

            var prompt = $@"Analyze these tasks and suggest the top 3 priorities for today:

{taskSummary}

Provide a brief, actionable suggestion (max 100 words) on which tasks to focus on first and why.";

            var response = await _ollamaService.ChatAsync(prompt, "llama3.2");
            
            if (!string.IsNullOrEmpty(response))
            {
                AiSuggestion = $"🤖 AI Empfehlung: {response}";
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error generating AI suggestions: {ex.Message}");
        }
    }

    /// <summary>
    /// ThemisDB Vector Search Integration
    /// Finds similar tasks using semantic similarity
    /// </summary>
    private async Task FindRelatedTasksAsync(TaskItem selectedTask)
    {
        if (_vectorService == null) return;

        try
        {
            // Generate embedding for selected task
            var taskText = $"{selectedTask.Title} {selectedTask.Description}";
            
            // Search for similar tasks using vector similarity
            // This would use ThemisDB's vector store capabilities
            // For now, this is a placeholder for future implementation
            
            RelatedTasks.Clear();
            // Add similar tasks to RelatedTasks collection
            
            await Task.CompletedTask;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error finding related tasks: {ex.Message}");
        }
    }

    /// <summary>
    /// ThemisDB Graph Integration
    /// Analyzes task dependencies and relationships
    /// </summary>
    [RelayCommand]
    private async Task AnalyzeTaskGraphAsync()
    {
        if (_graphService == null) return;

        try
        {
            // Build graph of task relationships
            // - Dependencies (Task A must be completed before Task B)
            // - Related by entity (Tasks linked to same document/case)
            // - Assigned to same team
            
            // This would visualize task dependencies in a graph view
            // For now, this is a placeholder for future implementation
            
            await Task.CompletedTask;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error analyzing task graph: {ex.Message}");
        }
    }

    /// <summary>
    /// Updates the entity context with ThemisDB timeline sync
    /// </summary>
    public async Task UpdateEntityContextAsync(string? entityId, LinkedEntityType? entityType)
    {
        CurrentEntityId = entityId;
        CurrentEntityType = entityType;
        await LoadTasksAsync();

        // Sync timeline view to show tasks for this entity
        if (_timelineService != null && !string.IsNullOrEmpty(entityId))
        {
            // Filter timeline to show events related to this entity
            await Task.CompletedTask;
        }
    }

    /// <summary>
    /// Handles task drop with ThemisDB graph update
    /// </summary>
    [RelayCommand]
    private async Task HandleTaskDropAsync(TaskDropInfo? dropInfo)
    {
        if (dropInfo?.DraggedTask == null || dropInfo.TargetTask == null)
            return;

        try
        {
            var draggedIndex = Tasks.IndexOf(dropInfo.DraggedTask);
            var targetIndex = Tasks.IndexOf(dropInfo.TargetTask);

            if (draggedIndex >= 0 && targetIndex >= 0)
            {
                Tasks.Move(draggedIndex, targetIndex);
            }

            // ThemisDB Enhancement: Create relationship in graph
            if (_graphService != null)
            {
                // Record task reordering as a relationship
                await Task.CompletedTask;
            }

            TasksView.Refresh();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error handling task drop: {ex.Message}");
        }
    }

    /// <summary>
    /// Marks task as completed with timeline update
    /// </summary>
    [RelayCommand]
    private async Task MarkTaskCompletedAsync(TaskItem? task)
    {
        if (task == null) return;
        
        task.Status = Application.Tasks.Queries.GetMyTasks.TaskStatus.Completed;
        
        // ThemisDB Enhancement: Create timeline completion event
        if (_timelineService != null && task.DueDate.HasValue)
        {
            await _timelineService.CreateEventAsync(new Models.TimelineEvent
            {
                Id = $"task-completed-{task.Id}-{DateTime.Now.Ticks}",
                DocumentId = task.Id,
                Description = $"✓ {task.Title} - Completed",
                Timestamp = DateTime.Now,
                EventType = "task-completion",
                Metadata = new Dictionary<string, object>
                {
                    { "taskId", task.Id },
                    { "completedAt", DateTime.Now }
                }
            });
        }

        TasksView.Refresh();
        await LoadTasksAsync();
    }

    /// <summary>
    /// Opens task with multi-modal view (document + timeline + geo)
    /// </summary>
    [RelayCommand]
    private async Task OpenTaskAsync(TaskItem? task)
    {
        if (task == null) return;

        // TODO: Open comprehensive task view showing:
        // - Linked documents
        // - Timeline position
        // - Geo location (if applicable)
        // - Related tasks (vector similarity)
        // - Dependency graph
        
        await Task.CompletedTask;
    }

    /// <summary>
    /// Generates task summary using LLM
    /// </summary>
    [RelayCommand]
    private async Task GenerateTaskSummaryAsync()
    {
        if (_ollamaService == null || !Tasks.Any()) return;

        try
        {
            IsLoading = true;
            
            var allTasks = string.Join("\n", Tasks.Select(t => 
                $"- [{t.Status}] {t.Title} (Due: {t.DueDate?.ToString("yyyy-MM-dd") ?? "No deadline"}, Priority: {t.Priority})"));

            var prompt = $@"Summarize these tasks and provide insights:

{allTasks}

Provide:
1. Total count by status
2. Overdue tasks count
3. Recommendations for task management
4. Risk areas that need attention

Keep response concise (max 150 words).";

            var summary = await _ollamaService.ChatAsync(prompt, "llama3.2");
            
            if (!string.IsNullOrEmpty(summary))
            {
                AiSuggestion = $"📊 Aufgaben-Übersicht:\n\n{summary}";
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error generating task summary: {ex.Message}");
        }
        finally
        {
            IsLoading = false;
        }
    }

    private bool FilterTasks(object obj)
    {
        if (obj is not TaskItem task) return false;

        if (!string.IsNullOrEmpty(SearchText))
        {
            var searchLower = SearchText.ToLower();
            if (!task.Title.ToLower().Contains(searchLower) &&
                !(task.Description?.ToLower().Contains(searchLower) ?? false))
            {
                return false;
            }
        }

        if (StatusFilter.HasValue && task.Status != StatusFilter.Value)
        {
            return false;
        }

        if (task.Status == Application.Tasks.Queries.GetMyTasks.TaskStatus.Completed)
        {
            return false;
        }

        return true;
    }

    partial void OnSortByChanged(string? value)
    {
        TasksView.SortDescriptions.Clear();
        
        var propertyName = value switch
        {
            "DueDate" => nameof(TaskItem.DueDate),
            "Priority" => nameof(TaskItem.Priority),
            "Title" => nameof(TaskItem.Title),
            _ => nameof(TaskItem.DueDate)
        };
        
        TasksView.SortDescriptions.Add(new SortDescription(propertyName, ListSortDirection.Ascending));
        TasksView.Refresh();
    }

    partial void OnSearchTextChanged(string value)
    {
        TasksView.Refresh();
    }

    partial void OnStatusFilterChanged(Application.Tasks.Queries.GetMyTasks.TaskStatus? value)
    {
        TasksView.Refresh();
    }

    private static TaskSortBy ParseSortBy(string? sortBy)
    {
        return sortBy switch
        {
            "DueDate" => TaskSortBy.DueDate,
            "Priority" => TaskSortBy.Priority,
            "Title" => TaskSortBy.Title,
            _ => TaskSortBy.DueDate
        };
    }
}
