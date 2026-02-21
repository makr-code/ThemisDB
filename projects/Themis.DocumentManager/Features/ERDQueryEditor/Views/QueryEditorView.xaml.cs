/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            QueryEditorView.xaml.cs                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
/// Interaction logic for QueryEditorView.xaml
/// </summary>
public partial class QueryEditorView : UserControl
{
    public QueryEditorView()
    {
        InitializeComponent();
        
        // Injiziere QueryEditorViewModel als DataContext
        var viewModel = App.GetService<QueryEditorViewModel>();
        if (viewModel != null)
        {
            DataContext = viewModel;
        }
    }
}


