/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineViewImproved.xaml.cs                       ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
