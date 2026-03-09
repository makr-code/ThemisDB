/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ERDView.xaml.cs                                    ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Features.ERDQueryEditor.ViewModels;

namespace Themis.DocumentManager.Features.ERDQueryEditor.Views;

/// <summary>
/// Interaction logic for ERDView.xaml
/// </summary>
public partial class ERDView : UserControl
{
    public ERDView()
    {
        InitializeComponent();
        
        // Injiziere ERDViewModel als DataContext
        var viewModel = App.GetService<ERDViewModel>();
        if (viewModel != null)
        {
            DataContext = viewModel;
        }
    }
}


