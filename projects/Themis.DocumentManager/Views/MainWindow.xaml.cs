using System.Windows;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Views;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;
    private readonly IOfficeIntegrationService _officeService;

    public MainWindow(MainViewModel viewModel, IOfficeIntegrationService officeService)
    {
        InitializeComponent();
        _viewModel = viewModel;
        _officeService = officeService;
        DataContext = _viewModel;

        _viewModel.PropertyChanged += ViewModel_PropertyChanged;
        
        // Load initial view
        LoadView("DocumentBrowser");
    }

    private void ViewModel_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(MainViewModel.CurrentView))
        {
            LoadView(_viewModel.CurrentView);
        }
        else if (e.PropertyName == nameof(MainViewModel.IsLoading))
        {
            LoadingOverlay.Visibility = _viewModel.IsLoading ? Visibility.Visible : Visibility.Collapsed;
        }
    }

    private void LoadView(string viewName)
    {
        UserControl? view = viewName switch
        {
            "DocumentBrowser" => new DocumentBrowserView(),
            "Search" => new SearchView(),
            "GeoView" => new GeoView(),
            "Timeline" => new TimelineView(),
            "GraphView" => new GraphView(),
            _ => null
        };

        if (view != null)
        {
            MainContentControl.Content = view;
        }
    }

    private void SearchBox_QuerySubmitted(ModernWpf.Controls.AutoSuggestBox sender, ModernWpf.Controls.AutoSuggestBoxQuerySubmittedEventArgs args)
    {
        if (!string.IsNullOrWhiteSpace(args.QueryText))
        {
            _viewModel.SearchQuery = args.QueryText;
            _viewModel.SearchCommand.Execute(null);
        }
    }

    #region Office Integration Event Handlers

    private async void NewWordDocument_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await _officeService.CreateNewWordDocumentAsync();
            if (result.Success)
            {
                MessageBox.Show($"Word document created: {result.DocumentPath}", 
                    "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show($"Failed to create Word document: {result.ErrorMessage}", 
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error: {ex.Message}", "Error", 
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private async void NewExcelWorkbook_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await _officeService.CreateNewExcelWorkbookAsync();
            if (result.Success)
            {
                MessageBox.Show($"Excel workbook created: {result.DocumentPath}", 
                    "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show($"Failed to create Excel workbook: {result.ErrorMessage}", 
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error: {ex.Message}", "Error", 
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private async void NewOutlookEmail_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await _officeService.CreateNewOutlookEmailAsync();
            if (result.Success)
            {
                MessageBox.Show("Outlook email draft created", 
                    "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show($"Failed to create Outlook email: {result.ErrorMessage}", 
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error: {ex.Message}", "Error", 
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private async void NewPowerPoint_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await _officeService.CreateNewPowerPointPresentationAsync();
            if (result.Success)
            {
                MessageBox.Show($"PowerPoint presentation created: {result.DocumentPath}", 
                    "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show($"Failed to create PowerPoint presentation: {result.ErrorMessage}", 
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error: {ex.Message}", "Error", 
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private async void NewOneNotePage_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await _officeService.CreateNewOneNotePageAsync();
            if (result.Success)
            {
                MessageBox.Show($"OneNote page created: {result.DocumentPath}", 
                    "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show($"Failed to create OneNote page: {result.ErrorMessage}", 
                    "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error: {ex.Message}", "Error", 
                MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    #endregion
}
