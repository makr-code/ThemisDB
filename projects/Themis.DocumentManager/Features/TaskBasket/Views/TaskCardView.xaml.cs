/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TaskCardView.xaml.cs                               ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     198                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks;

namespace Themis.DocumentManager.Features.TaskBasket.Views;

/// <summary>
/// Interaction logic for TaskCardView.xaml
/// Card-based task display optimized for sidebar display
/// Supports drag & drop for task distribution (inspired by Microsoft Teams and PDV VIS)
/// </summary>
public partial class TaskCardView : UserControl
{
    private Point _startPoint;
    private bool _isDragging;

    public TaskCardView()
    {
        InitializeComponent();
    }

    /// <summary>
    /// Initiates drag operation when user clicks and drags a task card
    /// Similar to Microsoft Teams task card drag behavior
    /// </summary>
    private void TaskCard_MouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left && sender is Border border)
        {
            _startPoint = e.GetPosition(null);
            _isDragging = false;

            // Start monitoring mouse movement for drag
            border.MouseMove += TaskCard_MouseMove;
            border.MouseUp += TaskCard_MouseUp;
        }
    }

    /// <summary>
    /// Handles mouse movement to detect drag threshold
    /// </summary>
    private void TaskCard_MouseMove(object sender, MouseEventArgs e)
    {
        if (e.LeftButton == MouseButtonState.Pressed && !_isDragging && sender is Border border)
        {
            Point currentPosition = e.GetPosition(null);
            Vector diff = _startPoint - currentPosition;

            // Check if drag threshold is exceeded (similar to Teams behavior)
            if (Math.Abs(diff.X) > SystemParameters.MinimumHorizontalDragDistance ||
                Math.Abs(diff.Y) > SystemParameters.MinimumVerticalDragDistance)
            {
                _isDragging = true;

                // Get the task item from the border's Tag
                if (border.Tag is TaskItem taskItem)
                {
                    // Create drag data
                    var dragData = new DataObject("TaskItem", taskItem);
                    dragData.SetData("TaskId", taskItem.Id);

                    // Visual feedback during drag
                    border.Opacity = 0.7;

                    // Start drag & drop operation
                    DragDrop.DoDragDrop(border, dragData, DragDropEffects.Move | DragDropEffects.Copy);

                    // Restore opacity
                    border.Opacity = 1.0;
                }

                // Clean up event handlers
                border.MouseMove -= TaskCard_MouseMove;
                border.MouseUp -= TaskCard_MouseUp;
            }
        }
    }

    /// <summary>
    /// Cleans up mouse event handlers
    /// </summary>
    private void TaskCard_MouseUp(object sender, MouseButtonEventArgs e)
    {
        if (sender is Border border)
        {
            border.MouseMove -= TaskCard_MouseMove;
            border.MouseUp -= TaskCard_MouseUp;
        }
        _isDragging = false;
    }

    /// <summary>
    /// Handles drag enter event to provide visual feedback
    /// Similar to Teams/VIS drop zone highlighting
    /// </summary>
    private void TaskCard_DragEnter(object sender, DragEventArgs e)
    {
        if (sender is Border border && e.Data.GetDataPresent("TaskItem"))
        {
            // Highlight drop target
            border.BorderBrush = new System.Windows.Media.SolidColorBrush(
                System.Windows.Media.Color.FromRgb(59, 130, 246)); // Blue highlight
            border.BorderThickness = new Thickness(2);
            e.Effects = DragDropEffects.Move;
        }
        else
        {
            e.Effects = DragDropEffects.None;
        }
        e.Handled = true;
    }

    /// <summary>
    /// Removes highlight when drag leaves the drop zone
    /// </summary>
    private void TaskCard_DragLeave(object sender, DragEventArgs e)
    {
        if (sender is Border border)
        {
            // Reset border to default
            border.BorderBrush = new System.Windows.Media.SolidColorBrush(
                System.Windows.Media.Color.FromRgb(226, 232, 240));
            border.BorderThickness = new Thickness(1);
        }
        e.Handled = true;
    }

    /// <summary>
    /// Handles drop event to reassign/redistribute tasks
    /// Implements collaborative task distribution like Teams
    /// </summary>
    private void TaskCard_Drop(object sender, DragEventArgs e)
    {
        if (sender is Border border)
        {
            // Reset border
            border.BorderBrush = new System.Windows.Media.SolidColorBrush(
                System.Windows.Media.Color.FromRgb(226, 232, 240));
            border.BorderThickness = new Thickness(1);

            if (e.Data.GetDataPresent("TaskItem"))
            {
                var draggedTask = e.Data.GetData("TaskItem") as TaskItem;
                var targetTask = border.Tag as TaskItem;

                if (draggedTask != null && targetTask != null && draggedTask.Id != targetTask.Id)
                {
                    // Notify ViewModel of task reordering/grouping
                    if (DataContext is ViewModels.TasksRightSidebarViewModel viewModel)
                    {
                        viewModel.HandleTaskDropCommand?.Execute(new TaskDropInfo
                        {
                            DraggedTask = draggedTask,
                            TargetTask = targetTask
                        });
                    }
                }
            }
        }
        e.Handled = true;
    }
}

/// <summary>
/// Information about a task drop operation
/// </summary>
public class TaskDropInfo
{
    public TaskItem? DraggedTask { get; set; }
    public TaskItem? TargetTask { get; set; }
}
