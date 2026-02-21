/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineViewImproved.xaml.cs                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     28                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
