using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels;

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


