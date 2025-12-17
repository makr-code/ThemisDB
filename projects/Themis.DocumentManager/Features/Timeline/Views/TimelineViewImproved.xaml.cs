using System.Windows;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Features.Timeline.Views;

public partial class TimelineViewImproved : UserControl
{
    public TimelineViewImproved()
    {
        InitializeComponent();
        
        // Injiziere TimelineViewModel als DataContext
        var viewModel = App.GetService<TimelineViewModel>();
        if (viewModel != null)
        {
            DataContext = viewModel;
        }
    }

    private void TimelineRuler_SizeChanged(object sender, SizeChangedEventArgs e)
    {
        if (DataContext is TimelineViewModel viewModel && e.NewSize.Width > 0)
        {
            viewModel.CanvasWidth = e.NewSize.Width;
        }
    }
}
