/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLogViewerWindow.xaml.cs                       ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Views.Windows
{
    public partial class AuditLogViewerWindow : Window
    {
        public AuditLogViewerWindow()
        {
            InitializeComponent();
            var vm = App.GetService<AuditLogViewerViewModel>();
            DataContext = vm;
            Loaded += async (_, __) =>
            {
                if (vm != null)
                    await vm.LoadAsync();
            };
        }
    }
}
