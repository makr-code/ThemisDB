/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineRulerView.xaml.cs                          ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     41                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows.Controls;

namespace Themis.DocumentManager.Views.Navigation;

public partial class TimelineRulerView : UserControl
{
    public TimelineRulerView()
    {
        InitializeComponent();
        
        // Inject TimelineRulerViewModel
        var viewModel = App.GetService<ViewModels.TimelineRulerViewModel>();
        if (viewModel != null)
        {
            DataContext = viewModel;
            _ = viewModel.InitializeAsync();
        }
    }
}
