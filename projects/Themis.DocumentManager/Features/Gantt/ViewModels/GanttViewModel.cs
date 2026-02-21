/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GanttViewModel.cs                                  ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Features.Gantt.ViewModels;

/// <summary>
/// ViewModel für Gantt-Chart Ansicht
/// Verwaltet Projekt-Tasks mit Start/End Datum auf Timeline
/// </summary>
public partial class GanttViewModel : ObservableObject
{
    [ObservableProperty]
    private ObservableCollection<GanttTask> tasks = new();
    
    [ObservableProperty]
    private DateTime startDate = DateTime.Now.AddMonths(-1);
    
    [ObservableProperty]
    private DateTime endDate = DateTime.Now.AddMonths(3);
    
    [ObservableProperty]
    private string filterText = string.Empty;
    
    [ObservableProperty]
    private bool isBusy = false;

    public GanttViewModel()
    {
        // Initialize with sample data for demo
        LoadSampleData();
    }

    [RelayCommand]
    public async Task LoadTasksAsync()
    {
        IsBusy = true;
        try
        {
            // TODO: Load actual tasks from service
            await Task.Delay(500);
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand]
    public void AddTask()
    {
        Tasks.Add(new GanttTask
        {
            Id = Guid.NewGuid().ToString(),
            Title = "New Task",
            StartDate = DateTime.Now,
            EndDate = DateTime.Now.AddDays(3),
            Progress = 0,
            ResourceName = "Unassigned"
        });
    }

    [RelayCommand]
    public void DeleteTask(string taskId)
    {
        var task = Tasks.FirstOrDefault(t => t.Id == taskId);
        if (task != null)
        {
            Tasks.Remove(task);
        }
    }

    private void LoadSampleData()
    {
        Tasks = new ObservableCollection<GanttTask>
        {
            new GanttTask
            {
                Id = "1",
                Title = "Projektplanung",
                StartDate = DateTime.Now,
                EndDate = DateTime.Now.AddDays(7),
                Progress = 100,
                ResourceName = "Alice"
            },
            new GanttTask
            {
                Id = "2",
                Title = "Design",
                StartDate = DateTime.Now.AddDays(7),
                EndDate = DateTime.Now.AddDays(21),
                Progress = 75,
                ResourceName = "Bob"
            },
            new GanttTask
            {
                Id = "3",
                Title = "Entwicklung",
                StartDate = DateTime.Now.AddDays(14),
                EndDate = DateTime.Now.AddDays(42),
                Progress = 45,
                ResourceName = "Charlie"
            },
            new GanttTask
            {
                Id = "4",
                Title = "Testing",
                StartDate = DateTime.Now.AddDays(40),
                EndDate = DateTime.Now.AddDays(50),
                Progress = 0,
                ResourceName = "Diana"
            }
        };
    }
}

/// <summary>
/// Gantt-Chart Task Model
/// </summary>
public class GanttTask
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Title { get; set; } = string.Empty;
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public int Progress { get; set; } // 0-100%
    public string ResourceName { get; set; } = string.Empty;
    
    public int DurationDays => (EndDate - StartDate).Days;
    public double ProgressPercentage => Progress / 100.0;
}

