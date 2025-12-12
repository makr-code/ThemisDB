using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Views;

public partial class ProcessLinkingDialog : UserControl
{
    public ProcessLinkingDialog()
    {
        InitializeComponent();
    }

    private void TemplateItem_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is Border border && border.DataContext is ProcessTemplateViewModel template)
        {
            var vm = this.DataContext as ProcessLinkingDialogViewModel;
            if (vm != null)
            {
                vm.SelectedTemplate = template;
            }
        }
    }
}
