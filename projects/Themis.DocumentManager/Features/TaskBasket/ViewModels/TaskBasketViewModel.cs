/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TaskBasketViewModel.cs                             ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     323                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
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

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MediatR;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Data;
using Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks;
using static Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks.GetMyTasksQuery;

namespace Themis.DocumentManager.Features.TaskBasket.ViewModels;

/// <summary>
/// ViewModel for Task Basket (Aufgaben-Korb)
/// Supports grouping, filtering, sorting, and customizable UI layout
/// Best practice: MVVM pattern with IMediator for CQRS
/// </summary>
public partial class TaskBasketViewModel : ObservableObject
{
    private readonly IMediator _mediator;
    
    [ObservableProperty]
    private ObservableCollection<TaskItem> _tasks = new();

    [ObservableProperty]
    private ObservableCollection<TaskItem> _selectedTasks = new();

    [ObservableProperty]
    private TaskItem? _selectedTask;

    [ObservableProperty]
    private bool _isLoading;

    [ObservableProperty]
    private string _searchText = string.Empty;

    [ObservableProperty]
    private Application.Tasks.Queries.GetMyTasks.TaskStatus? _statusFilter;

    [ObservableProperty]
    private TaskPriority? _priorityFilter;

    [ObservableProperty]
    private string? _categoryFilter;

    [ObservableProperty]
    private string? _entityId;

    [ObservableProperty]
    private LinkedEntityType? _entityType;

    [ObservableProperty]
    private TaskSortBy _sortBy = TaskSortBy.DueDate;

    [ObservableProperty]
    private bool _sortDescending = false;

    [ObservableProperty]
    private TaskGroupBy _groupBy = TaskGroupBy.Category;

    [ObservableProperty]
    private bool _showCompleted = false;

    // UI Layout properties (for customizable interface)
    [ObservableProperty]
    private double _filterPanelWidth = 250;

    [ObservableProperty]
    private double _detailPanelWidth = 300;

    [ObservableProperty]
    private bool _isFilterPanelVisible = true;

    [ObservableProperty]
    private bool _isDetailPanelVisible = true;

    [ObservableProperty]
    private bool _isTreeViewMode = true; // TreeView vs ListView

    [ObservableProperty]
    private int _unreadTasksCount;

    [ObservableProperty]
    private bool _hasUnreadTasks;

    public ICollectionView TasksView { get; private set; }

    public TaskBasketViewModel(IMediator mediator)
    {
        _mediator = mediator;
        
        // Initialize CollectionView for filtering and grouping
        TasksView = CollectionViewSource.GetDefaultView(Tasks);
        TasksView.Filter = FilterTasks;
        TasksView.GroupDescriptions.Add(new PropertyGroupDescription(nameof(TaskItem.Category)));
    }

    [RelayCommand]
    public async Task LoadTasksAsync()
    {
        IsLoading = true;
        try
        {
            var query = new GetMyTasksQuery
            {
                UserId = "current-user", // TODO: Get from authentication service
                StatusFilter = ShowCompleted ? null : StatusFilter,
                PriorityFilter = PriorityFilter,
                CategoryFilter = CategoryFilter,
                EntityId = EntityId,
                EntityType = EntityType,
                SortBy = SortBy,
                SortDescending = SortDescending
            };

            var tasks = await _mediator.Send(query);
            
            Tasks.Clear();
            foreach (var task in tasks)
            {
                Tasks.Add(task);
            }

            // Calculate unread tasks count (new tasks that haven't been viewed)
            UnreadTasksCount = tasks.Count(t => t.Status == Application.Tasks.Queries.GetMyTasks.TaskStatus.Pending && t.CreatedAt > DateTime.UtcNow.AddDays(-1));
            HasUnreadTasks = UnreadTasksCount > 0;

            TasksView.Refresh();
        }
        finally
        {
            IsLoading = false;
        }
    }

    /// <summary>
    /// Setzt Entity-Filter (z.B. Dokument/Vorgang/Akte) für die Aufgabenliste
    /// </summary>
    public void ApplyEntityFilter(string? entityId, LinkedEntityType? entityType)
    {
        EntityId = entityId;
        EntityType = entityType;
    }

    [RelayCommand]
    private void ApplyFilters()
    {
        TasksView.Refresh();
    }

    [RelayCommand]
    private void ClearFilters()
    {
        StatusFilter = null;
        PriorityFilter = null;
        CategoryFilter = null;
        SearchText = string.Empty;
        TasksView.Refresh();
    }

    [RelayCommand]
    private void ToggleGroupBy()
    {
        TasksView.GroupDescriptions.Clear();

        var propertyName = GroupBy switch
        {
            TaskGroupBy.Category => nameof(TaskItem.Category),
            TaskGroupBy.Priority => nameof(TaskItem.Priority),
            TaskGroupBy.Status => nameof(TaskItem.Status),
            TaskGroupBy.DueDate => nameof(TaskItem.DueDate),
            _ => nameof(TaskItem.Category)
        };

        TasksView.GroupDescriptions.Add(new PropertyGroupDescription(propertyName));
        TasksView.Refresh();
    }

    [RelayCommand]
    private void ToggleSortOrder()
    {
        SortDescending = !SortDescending;
        _ = LoadTasksAsync();
    }

    [RelayCommand]
    private void ToggleViewMode()
    {
        IsTreeViewMode = !IsTreeViewMode;
    }

    [RelayCommand]
    private void ToggleFilterPanel()
    {
        IsFilterPanelVisible = !IsFilterPanelVisible;
    }

    [RelayCommand]
    private void ToggleDetailPanel()
    {
        IsDetailPanelVisible = !IsDetailPanelVisible;
    }

    [RelayCommand]
    private void MarkTasksCompleted()
    {
        foreach (var task in SelectedTasks)
        {
            task.Status = Application.Tasks.Queries.GetMyTasks.TaskStatus.Completed;
        }
        TasksView.Refresh();
    }

    [RelayCommand]
    private void DeleteSelectedTasks()
    {
        foreach (var task in SelectedTasks.ToList())
        {
            Tasks.Remove(task);
        }
        SelectedTasks.Clear();
    }

    [RelayCommand]
    private async Task OpenTaskAsync(TaskItem? task)
    {
        if (task == null) return;

        // Navigate to appropriate view based on task type
        // TODO: Implement navigation
    }

    private bool FilterTasks(object obj)
    {
        if (obj is not TaskItem task) return false;

        // Search filter
        if (!string.IsNullOrEmpty(SearchText))
        {
            var searchLower = SearchText.ToLower();
            if (!task.Title.ToLower().Contains(searchLower) &&
                !task.Description.ToLower().Contains(searchLower))
            {
                return false;
            }
        }

        // Status filter
        if (StatusFilter.HasValue && task.Status != StatusFilter.Value)
        {
            return false;
        }

        // Priority filter
        if (PriorityFilter.HasValue && task.Priority != PriorityFilter.Value)
        {
            return false;
        }

        // Category filter
        if (!string.IsNullOrEmpty(CategoryFilter) && task.Category != CategoryFilter)
        {
            return false;
        }

        // Hide completed tasks if not showing them
        if (!ShowCompleted && task.Status == Application.Tasks.Queries.GetMyTasks.TaskStatus.Completed)
        {
            return false;
        }

        return true;
    }

    partial void OnSearchTextChanged(string value)
    {
        TasksView.Refresh();
    }

    partial void OnGroupByChanged(TaskGroupBy value)
    {
        ToggleGroupBy();
    }
}

public enum TaskGroupBy
{
    None,
    Category,
    Priority,
    Status,
    DueDate
}


