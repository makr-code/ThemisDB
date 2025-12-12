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
