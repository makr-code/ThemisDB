/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLogViewerWindow.xaml.cs                       ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
