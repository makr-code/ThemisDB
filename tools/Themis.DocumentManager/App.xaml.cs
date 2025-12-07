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
        // Core Services
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
        
        // Administrative Structure Services
        services.AddSingleton<IProcessTimelineService, ProcessTimelineService>();
        services.AddSingleton<IAdministrativeStructureService, AdministrativeStructureService>();
        
        // Phase 1 VIS Features Services
        services.AddSingleton<INotificationService, NotificationService>();
        services.AddSingleton<IInboxService, InboxService>();
        services.AddSingleton<IReminderService, ReminderService>();
        services.AddSingleton<ICosigningService, CosigningService>();
        services.AddSingleton<IProcessLogService, ProcessLogService>();
        services.AddSingleton<IFilingPlanService, FilingPlanService>();
        
        // Geo Services (OSM Layer Support)
        services.AddSingleton<IMapConfigurationService, MapConfigurationService>();
        services.AddSingleton<IGeoLayerService, GeoLayerService>();
        services.AddSingleton<IGeoFeatureService, GeoFeatureService>();
        services.AddSingleton<IGeocodingService, GeocodingService>();
        services.AddSingleton<IGeoDocumentService, GeoDocumentService>();
        
        // LLM Services (Native AI Support) - Interfaces only, implementations TBD
        // services.AddSingleton<ILLMService, LLMService>();
        // services.AddSingleton<IDocumentAnalysisService, DocumentAnalysisService>();
        // services.AddSingleton<ISemanticSearchService, SemanticSearchService>();
        // services.AddSingleton<IChatAssistantService, ChatAssistantService>();
        // services.AddSingleton<IDocumentGenerationService, DocumentGenerationService>();
        // services.AddSingleton<IDocumentComparisonService, DocumentComparisonService>();
        // services.AddSingleton<IComplianceCheckService, ComplianceCheckService>();
        // services.AddSingleton<ITranslationService, TranslationService>();
        // services.AddSingleton<IAutoClassificationService, AutoClassificationService>();
        // services.AddSingleton<IRedactionService, RedactionService>();
        // services.AddSingleton<IQualityAssuranceService, QualityAssuranceService>();
        // services.AddSingleton<ISmartRoutingService, SmartRoutingService>();
        // services.AddSingleton<IKnowledgeBaseService, KnowledgeBaseService>();
        // services.AddSingleton<IOCRService, OCRService>();
        
        // Metadata Badge Services (Smart Input with Semantic Similarity)
        services.AddSingleton<IBadgePatternService, BadgePatternService>();
        services.AddSingleton<IAbbreviationService, AbbreviationService>();
        services.AddSingleton<IMetadataBadgeService, MetadataBadgeService>();
        services.AddSingleton<ISmartSuggestionService, SmartSuggestionService>();
        services.AddSingleton<ISmartInputValidatorService, SmartInputValidatorService>();
        
        // Process Watch & Timeline Services
        services.AddSingleton<IProcessWatchService, ProcessWatchService>();
        services.AddSingleton<ITimelineAggregationService, TimelineAggregationService>();
        services.AddSingleton<IGanttService, GanttService>();
        
        // Outbox Service
        services.AddSingleton<IOutboxService, OutboxService>();
        
        // Phase 2 Services (Email, Scan, OCR, Search, Notifications, Forms)
        services.AddSingleton<IEmailIntegrationService, EmailIntegrationService>();
        services.AddSingleton<IScanService, ScanService>();
        services.AddSingleton<IOCRService, OCRService>();
        services.AddSingleton<IFullTextSearchService, FullTextSearchService>();
        services.AddSingleton<IEnhancedNotificationService, EnhancedNotificationService>();
        services.AddSingleton<IFormManagementService, FormManagementService>();
        
        // Email Threading Services
        services.AddSingleton<IEmailHeaderService, EmailHeaderService>();
        
        // Seamless Integration Services (Messenger, Calendar, Tasks)
        services.AddSingleton<IMessengerIntegrationService, MessengerIntegrationService>();
        services.AddSingleton<ICalendarIntegrationService, CalendarIntegrationService>();
        services.AddSingleton<IOutlookTaskService, OutlookTaskService>();
        services.AddSingleton<ISeamlessIntegrationOrchestrator, SeamlessIntegrationOrchestrator>();

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
