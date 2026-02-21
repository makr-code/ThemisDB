/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineViewImproved.xaml.cs                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
