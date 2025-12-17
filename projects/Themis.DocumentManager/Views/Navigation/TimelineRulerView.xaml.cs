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
