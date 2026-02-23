/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DashboardViewModel.cs                              ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   74.0/100                                       ║
    • Total Lines:     198                                            ║
    • Open Issues:     TODOs: 8, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MediatR;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Application.Documents.Queries.GetDocuments;
using Themis.DocumentManager.Application.Inbox.Queries.GetInboxItems;
using Themis.DocumentManager.Application.Reminders.Queries.GetDueReminders;
using Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks;

namespace Themis.DocumentManager.Features.Dashboard.ViewModels;

/// <summary>
/// ViewModel for Dashboard - Starting point with quick actions
/// </summary>
public partial class DashboardViewModel : ObservableObject
{
    private readonly IMediator _mediator;

    [ObservableProperty]
    private int _totalDocuments;

    [ObservableProperty]
    private int _pendingTasks;

    [ObservableProperty]
    private int _overdueReminders;

    [ObservableProperty]
    private int _unreadInbox;

    [ObservableProperty]
    private ObservableCollection<RecentActivity> _recentActivities = new();

    [ObservableProperty]
    private bool _isLoading;

    public DashboardViewModel(IMediator mediator)
    {
        _mediator = mediator;
    }

    [RelayCommand]
    private async Task LoadDashboardDataAsync()
    {
        IsLoading = true;
        try
        {
            // Load document count
            var documentsQuery = new GetDocumentsQuery { Page = 1, PageSize = 1 };
            var documents = await _mediator.Send(documentsQuery);
            TotalDocuments = documents.Count;

            // Load pending tasks count
            var tasksQuery = new GetMyTasksQuery
            {
                UserId = "current-user", // TODO: Get from auth service
                StatusFilter = Application.Tasks.Queries.GetMyTasks.TaskStatus.Pending
            };
            var tasks = await _mediator.Send(tasksQuery);
            PendingTasks = tasks.Count;

            // Load overdue reminders
            var remindersQuery = new GetDueRemindersQuery { UpToDate = DateTime.UtcNow };
            var reminders = await _mediator.Send(remindersQuery);
            OverdueReminders = reminders.Count(r => r.DueDate < DateTime.UtcNow);

            // Load unread inbox count
            var inboxQuery = new GetInboxItemsQuery { Status = Models.InboxStatus.New };
            var inboxItems = await _mediator.Send(inboxQuery);
            UnreadInbox = inboxItems.Count;

            // Load recent activities (mock for now)
            LoadRecentActivities();
        }
        finally
        {
            IsLoading = false;
        }
    }

    private void LoadRecentActivities()
    {
        RecentActivities.Clear();
        
        // Mock data - would be loaded from actual activity log
        RecentActivities.Add(new RecentActivity
        {
            Icon = "📄",
            Title = "Vertrag.pdf erstellt",
            Description = "Neues Dokument hochgeladen",
            TimeAgo = "vor 5 Minuten"
        });
        
        RecentActivities.Add(new RecentActivity
        {
            Icon = "✓",
            Title = "Aufgabe erledigt",
            Description = "Genehmigung für Projekt A abgeschlossen",
            TimeAgo = "vor 1 Stunde"
        });
        
        RecentActivities.Add(new RecentActivity
        {
            Icon = "📥",
            Title = "Neuer Posteingang",
            Description = "Antrag auf Baugenehmigung eingegangen",
            TimeAgo = "vor 2 Stunden"
        });
        
        RecentActivities.Add(new RecentActivity
        {
            Icon = "📅",
            Title = "Frist gesetzt",
            Description = "Wiedervorlage für Projekt B",
            TimeAgo = "gestern"
        });
    }

    [RelayCommand]
    private void NewDocument()
    {
        // Navigate to new document creation
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void OpenTaskBasket()
    {
        // Navigate to task basket
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void OpenSearch()
    {
        // Navigate to search view
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void OpenInbox()
    {
        // Navigate to inbox view
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void OpenTimeline()
    {
        // Navigate to timeline view
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void OpenGeoView()
    {
        // Navigate to geo view
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void OpenGraphView()
    {
        // Navigate to graph view
        // TODO: Implement navigation
    }
}

public class RecentActivity
{
    public string Icon { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string TimeAgo { get; set; } = string.Empty;
}

