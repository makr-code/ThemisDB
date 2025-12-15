using System.Windows.Controls;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Views;

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
