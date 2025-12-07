using Microsoft.Extensions.DependencyInjection;
using System.Windows;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Views;

namespace Themis.DocumentManager;

/// <summary>
/// Interaction logic for App.xaml
/// </summary>
public partial class App : Application
{
    private ServiceProvider? _serviceProvider;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        var serviceCollection = new ServiceCollection();
        ConfigureServices(serviceCollection);
        _serviceProvider = serviceCollection.BuildServiceProvider();

        var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
        mainWindow.Show();
    }

    private void ConfigureServices(IServiceCollection services)
    {
        // Services
        services.AddSingleton<IThemisApiClient, ThemisApiClient>();
        services.AddSingleton<IDocumentService, DocumentService>();
        services.AddSingleton<ISearchService, SearchService>();
        services.AddSingleton<IMetadataService, MetadataService>();
        services.AddSingleton<IGeoService, GeoService>();
        services.AddSingleton<ITimelineService, TimelineService>();
        services.AddSingleton<IVectorService, VectorService>();
        services.AddSingleton<IGraphService, GraphService>();
        services.AddSingleton<IRevisionService, RevisionService>();
        services.AddSingleton<IOfficeIntegrationService, OfficeIntegrationService>();
        
        // Administrative Structure Services (NEW)
        services.AddSingleton<IProcessTimelineService, ProcessTimelineService>();
        services.AddSingleton<IAdministrativeStructureService, AdministrativeStructureService>();

        // ViewModels
        services.AddTransient<MainViewModel>();
        services.AddTransient<DocumentBrowserViewModel>();
        services.AddTransient<DocumentDetailViewModel>();
        services.AddTransient<SearchViewModel>();
        services.AddTransient<GeoViewModel>();
        services.AddTransient<TimelineViewModel>();
        services.AddTransient<GraphViewModel>();

        // Views
        services.AddSingleton<MainWindow>();
    }

    protected override void OnExit(ExitEventArgs e)
    {
        _serviceProvider?.Dispose();
        base.OnExit(e);
    }
}
