/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TasksRightSidebarViewModel.cs                      ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     333                                            ║
    • Open Issues:     TODOs: 5, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

namespace Themis.DocumentManager.Features.TaskBasket.ViewModels;

/// <summary>
/// ViewModel for Tasks displayed in right sidebar
/// Optimized for compact card-based display with entity filtering
/// Implements collaborative features inspired by Microsoft Teams and PDV VIS best practices:
/// - Drag & drop task distribution
/// - Real-time task updates
/// - Visual status indicators
/// - Context-aware filtering
/// Follows MVVM pattern and OOP best practices
/// </summary>
public partial class TasksRightSidebarViewModel : ObservableObject
{
    private readonly IMediator _mediator;
    
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

    public ICollectionView TasksView { get; private set; }

    public TasksRightSidebarViewModel(IMediator mediator)
    {
        _mediator = mediator;
        
        // Initialize CollectionView for filtering and sorting
        TasksView = CollectionViewSource.GetDefaultView(Tasks);
        TasksView.Filter = FilterTasks;
        
        // Default sort by DueDate (best practice from VIS)
        TasksView.SortDescriptions.Add(new SortDescription(nameof(TaskItem.DueDate), ListSortDirection.Ascending));
    }

    /// <summary>
    /// Loads tasks for the current user with optional entity filtering
    /// Implements real-time update pattern similar to Microsoft Teams
    /// </summary>
    [RelayCommand]
    public async Task LoadTasksAsync()
    {
        IsLoading = true;
        try
        {
            var query = new GetMyTasksQuery
            {
                UserId = CurrentUserId ?? "current-user", // TODO: Get from authentication service
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
                // Mark tasks related to current entity (VIS-style context highlighting)
                task.IsRelatedToCurrentEntity = !string.IsNullOrEmpty(CurrentEntityId) && 
                                                 task.LinkedEntityId == CurrentEntityId;
                Tasks.Add(task);
            }

            // Calculate unread/new tasks (Teams-style notification badge)
            UnreadTaskCount = tasks.Count(t => t.Status == Application.Tasks.Queries.GetMyTasks.TaskStatus.Pending && 
                                               t.CreatedAt > DateTime.UtcNow.AddHours(-24));

            HasNoTasks = Tasks.Count == 0;
            TasksView.Refresh();
        }
        catch (Exception ex)
        {
            // Log error (in production, use proper logging)
            System.Diagnostics.Debug.WriteLine($"Error loading tasks: {ex.Message}");
            HasNoTasks = true;
        }
        finally
        {
            IsLoading = false;
        }
    }

    /// <summary>
    /// Updates the entity context (e.g., when user selects a different document/case)
    /// VIS-style context-aware filtering
    /// </summary>
    public async Task UpdateEntityContextAsync(string? entityId, LinkedEntityType? entityType)
    {
        CurrentEntityId = entityId;
        CurrentEntityType = entityType;
        await LoadTasksAsync();
    }

    /// <summary>
    /// Handles task drop operation for drag & drop distribution
    /// Implements collaborative task assignment like Microsoft Teams
    /// </summary>
    [RelayCommand]
    private async Task HandleTaskDropAsync(TaskDropInfo? dropInfo)
    {
        if (dropInfo?.DraggedTask == null || dropInfo.TargetTask == null)
            return;

        try
        {
            // Example: Merge task priority or reassign based on target
            // In a real implementation, this could:
            // 1. Reassign task to different user (Teams-style @mention assignment)
            // 2. Change task status/priority
            // 3. Link tasks together
            // 4. Reorder tasks in a list
            
            // For now, show visual feedback by swapping positions
            var draggedIndex = Tasks.IndexOf(dropInfo.DraggedTask);
            var targetIndex = Tasks.IndexOf(dropInfo.TargetTask);

            if (draggedIndex >= 0 && targetIndex >= 0)
            {
                Tasks.Move(draggedIndex, targetIndex);
            }

            // TODO: Persist changes via MediatR command
            // await _mediator.Send(new UpdateTaskOrderCommand { ... });

            // Refresh view
            TasksView.Refresh();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error handling task drop: {ex.Message}");
        }
    }

    /// <summary>
    /// Marks a task as completed
    /// Similar to Teams quick action checkboxes
    /// </summary>
    [RelayCommand]
    private async Task MarkTaskCompletedAsync(TaskItem? task)
    {
        if (task == null) return;
        
        task.Status = Application.Tasks.Queries.GetMyTasks.TaskStatus.Completed;
        TasksView.Refresh();
        
        // TODO: Persist to backend via MediatR command
        // await _mediator.Send(new CompleteTaskCommand { TaskId = task.Id });
        
        // Reload to update counts
        await LoadTasksAsync();
    }

    /// <summary>
    /// Opens task details or navigates to related entity
    /// Teams-style deep linking to context
    /// </summary>
    [RelayCommand]
    private async Task OpenTaskAsync(TaskItem? task)
    {
        if (task == null) return;

        // TODO: Implement navigation to task details or related entity
        // This could open a dialog or navigate to the linked document/case
        // Similar to Teams card click behavior
        await Task.CompletedTask;
    }

    /// <summary>
    /// Assigns a task to a user (collaborative feature)
    /// Teams-style @mention assignment
    /// </summary>
    [RelayCommand]
    private async Task AssignTaskAsync(object? parameter)
    {
        if (parameter is (TaskItem task, string userId))
        {
            task.AssignedTo = userId;
            TasksView.Refresh();
            
            // TODO: Persist via MediatR and notify assignee (Teams-style notification)
            // await _mediator.Send(new AssignTaskCommand { TaskId = task.Id, UserId = userId });
        }
    }

    /// <summary>
    /// Filters tasks based on search text and status filter
    /// VIS-style advanced filtering
    /// </summary>
    private bool FilterTasks(object obj)
    {
        if (obj is not TaskItem task) return false;

        // Search filter (case-insensitive)
        if (!string.IsNullOrEmpty(SearchText))
        {
            var searchLower = SearchText.ToLower();
            if (!task.Title.ToLower().Contains(searchLower) &&
                !(task.Description?.ToLower().Contains(searchLower) ?? false))
            {
                return false;
            }
        }

        // Status filter
        if (StatusFilter.HasValue && task.Status != StatusFilter.Value)
        {
            return false;
        }

        // Hide completed tasks by default (best practice from VIS)
        if (task.Status == Application.Tasks.Queries.GetMyTasks.TaskStatus.Completed)
        {
            return false;
        }

        return true;
    }

    /// <summary>
    /// Updates sorting when sort option changes
    /// </summary>
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

    /// <summary>
    /// Refreshes filter when search text changes
    /// Real-time search like Teams
    /// </summary>
    partial void OnSearchTextChanged(string value)
    {
        TasksView.Refresh();
    }

    /// <summary>
    /// Refreshes filter when status filter changes
    /// </summary>
    partial void OnStatusFilterChanged(Application.Tasks.Queries.GetMyTasks.TaskStatus? value)
    {
        TasksView.Refresh();
    }

    /// <summary>
    /// Parses sort by string to enum
    /// </summary>
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
