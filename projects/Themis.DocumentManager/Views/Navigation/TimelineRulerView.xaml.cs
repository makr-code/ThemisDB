/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineRulerView.xaml.cs                          ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:56:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
