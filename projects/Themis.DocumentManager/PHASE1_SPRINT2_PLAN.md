# Phase 1 - Sprint 2: Extended CQRS, Validation Pipeline, Logging & Testing

**Start Date:** 2025-12-10 (nach Sprint 1)  
**Duration:** 2 Wochen  
**Ziel:** Erweiterte CQRS Commands, Validation Pipeline, Serilog, Unit Tests

---

## Sprint 2 Tasks

### 1. Additional CQRS Commands ✅

#### Documents
- [ ] Application/Documents/Commands/UpdateDocument/
  - UpdateDocumentCommand.cs
  - UpdateDocumentCommandHandler.cs
  - UpdateDocumentCommandValidator.cs
- [ ] Application/Documents/Commands/DeleteDocument/
  - DeleteDocumentCommand.cs
  - DeleteDocumentCommandHandler.cs
  - DeleteDocumentCommandValidator.cs

#### VIS Workflows
- [ ] Application/Inbox/Commands/AssignInboxItem/
  - AssignInboxItemCommand.cs
  - AssignInboxItemCommandHandler.cs
  - AssignInboxItemCommandValidator.cs
- [ ] Application/Reminders/Commands/CompleteReminder/
  - CompleteReminderCommand.cs
  - CompleteReminderCommandHandler.cs
  - CompleteReminderCommandValidator.cs
- [ ] Application/Cosigning/Commands/RejectCosigningStep/
  - RejectCosigningStepCommand.cs
  - RejectCosigningStepCommandHandler.cs
  - RejectCosigningStepCommandValidator.cs

### 2. MediatR Validation Pipeline ✅

- [ ] Application/Common/Behaviors/ValidationBehavior.cs
- [ ] Application/Common/Exceptions/ValidationException.cs
- [ ] Update App.xaml.cs to register pipeline behavior

**Implementation:**
```csharp
public class ValidationBehavior<TRequest, TResponse> : IPipelineBehavior<TRequest, TResponse>
    where TRequest : IRequest<TResponse>
{
    private readonly IEnumerable<IValidator<TRequest>> _validators;

    public ValidationBehavior(IEnumerable<IValidator<TRequest>> validators)
    {
        _validators = validators;
    }

    public async Task<TResponse> Handle(
        TRequest request,
        RequestHandlerDelegate<TResponse> next,
        CancellationToken cancellationToken)
    {
        if (!_validators.Any())
            return await next();

        var context = new ValidationContext<TRequest>(request);
        
        var validationResults = await Task.WhenAll(
            _validators.Select(v => v.ValidateAsync(context, cancellationToken)));
        
        var failures = validationResults
            .SelectMany(r => r.Errors)
            .Where(f => f != null)
            .ToList();

        if (failures.Any())
            throw new ValidationException(failures);

        return await next();
    }
}
```

**Registration in App.xaml.cs:**
```csharp
services.AddMediatR(cfg => 
{
    cfg.RegisterServicesFromAssembly(typeof(App).Assembly);
    cfg.AddBehavior(typeof(IPipelineBehavior<,>), typeof(ValidationBehavior<,>));
});
```

### 3. Serilog Integration ✅

**NuGet Packages:**
- [ ] Serilog v3.1.1
- [ ] Serilog.Sinks.Console v5.0.1
- [ ] Serilog.Sinks.File v5.0.0
- [ ] Serilog.Extensions.Logging v8.0.0
- [ ] Serilog.Enrichers.Environment v2.3.0

**Files:**
- [ ] Infrastructure/Logging/SerilogConfiguration.cs
- [ ] Update App.xaml.cs for Serilog initialization

**Configuration:**
```csharp
Log.Logger = new LoggerConfiguration()
    .MinimumLevel.Debug()
    .MinimumLevel.Override("Microsoft", LogEventLevel.Information)
    .Enrich.FromLogContext()
    .Enrich.WithProperty("Application", "ThemisDB.DocumentManager")
    .Enrich.WithMachineName()
    .Enrich.WithEnvironmentName()
    .WriteTo.Console(
        outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}] {Message:lj}{NewLine}{Exception}")
    .WriteTo.File(
        path: "logs/themisdb-.log",
        rollingInterval: RollingInterval.Day,
        retainedFileCountLimit: 30)
    .CreateLogger();
```

**Usage in Handlers:**
```csharp
public class CreateDocumentCommandHandler
{
    private readonly ILogger<CreateDocumentCommandHandler> _logger;

    public async Task<string> Handle(CreateDocumentCommand request, ...)
    {
        _logger.LogInformation(
            "Creating document {Title} by {Author}", 
            request.Title, 
            request.Author);

        try
        {
            var documentId = await _repository.CreateDocumentAsync(...);
            
            _logger.LogInformation(
                "Document {DocumentId} created successfully", 
                documentId);
            
            return documentId;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, 
                "Failed to create document {Title}", 
                request.Title);
            throw;
        }
    }
}
```

### 4. Unit Tests ✅

**Test Project:**
- [ ] Tests/Application.UnitTests/Application.UnitTests.csproj

**NuGet Packages:**
- [ ] xUnit v2.6.4
- [ ] xUnit.runner.visualstudio v2.5.6
- [ ] Moq v4.20.70
- [ ] FluentAssertions v6.12.0
- [ ] Microsoft.NET.Test.Sdk v17.8.0

**Test Files (12 files):**
- [ ] Tests/Application.UnitTests/Documents/Commands/CreateDocumentCommandTests.cs
- [ ] Tests/Application.UnitTests/Documents/Commands/CreateDocumentCommandValidatorTests.cs
- [ ] Tests/Application.UnitTests/Documents/Commands/UpdateDocumentCommandTests.cs
- [ ] Tests/Application.UnitTests/Documents/Commands/DeleteDocumentCommandTests.cs
- [ ] Tests/Application.UnitTests/Documents/Queries/GetDocumentQueryTests.cs
- [ ] Tests/Application.UnitTests/Documents/Queries/GetDocumentsQueryTests.cs
- [ ] Tests/Application.UnitTests/Inbox/CreateInboxItemCommandTests.cs
- [ ] Tests/Application.UnitTests/Inbox/AssignInboxItemCommandTests.cs
- [ ] Tests/Application.UnitTests/Reminders/CreateReminderCommandTests.cs
- [ ] Tests/Application.UnitTests/Reminders/CompleteReminderCommandTests.cs
- [ ] Tests/Application.UnitTests/Cosigning/ApproveCosigningStepCommandTests.cs
- [ ] Tests/Application.UnitTests/Common/ValidationBehaviorTests.cs

**Example Test:**
```csharp
public class CreateDocumentCommandTests
{
    [Fact]
    public async Task Handle_ValidCommand_ReturnsDocumentId()
    {
        // Arrange
        var mockRepo = new Mock<IThemisRepository>();
        mockRepo.Setup(r => r.CreateDocumentAsync(
            It.IsAny<Document>(), 
            It.IsAny<CancellationToken>()))
            .ReturnsAsync("doc123");
        
        var mockMediator = new Mock<IMediator>();
        var handler = new CreateDocumentCommandHandler(
            mockRepo.Object, 
            mockMediator.Object);
        
        var command = new CreateDocumentCommand
        {
            Title = "Test.pdf",
            Filename = "test.pdf",
            Author = "test@example.com"
        };

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.Equal("doc123", result);
        mockRepo.Verify(r => r.CreateDocumentAsync(
            It.Is<Document>(d => d.Title == "Test.pdf"), 
            It.IsAny<CancellationToken>()), Times.Once);
    }

    [Fact]
    public async Task Handle_RepositoryThrows_PropagatesException()
    {
        // Arrange
        var mockRepo = new Mock<IThemisRepository>();
        mockRepo.Setup(r => r.CreateDocumentAsync(
            It.IsAny<Document>(), 
            It.IsAny<CancellationToken>()))
            .ThrowsAsync(new InvalidOperationException("DB error"));
        
        var mockMediator = new Mock<IMediator>();
        var handler = new CreateDocumentCommandHandler(
            mockRepo.Object, 
            mockMediator.Object);
        
        var command = new CreateDocumentCommand
        {
            Title = "Test.pdf",
            Filename = "test.pdf",
            Author = "test@example.com"
        };

        // Act & Assert
        await Assert.ThrowsAsync<InvalidOperationException>(
            () => handler.Handle(command, CancellationToken.None));
    }
}

public class CreateDocumentCommandValidatorTests
{
    [Theory]
    [InlineData("")]
    [InlineData(null)]
    public async Task Validate_EmptyTitle_ShouldHaveValidationError(string title)
    {
        // Arrange
        var validator = new CreateDocumentCommandValidator();
        var command = new CreateDocumentCommand { Title = title };

        // Act
        var result = await validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().Contain(e => e.PropertyName == "Title");
    }

    [Fact]
    public async Task Validate_TitleTooLong_ShouldHaveValidationError()
    {
        // Arrange
        var validator = new CreateDocumentCommandValidator();
        var command = new CreateDocumentCommand 
        { 
            Title = new string('a', 201) // > 200 chars
        };

        // Act
        var result = await validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().Contain(e => 
            e.PropertyName == "Title" && 
            e.ErrorMessage.Contains("200"));
    }

    [Fact]
    public async Task Validate_ValidCommand_ShouldNotHaveValidationError()
    {
        // Arrange
        var validator = new CreateDocumentCommandValidator();
        var command = new CreateDocumentCommand
        {
            Title = "Valid Document.pdf",
            Filename = "document.pdf",
            Author = "user@example.com",
            MimeType = "application/pdf"
        };

        // Act
        var result = await validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeTrue();
    }
}
```

---

## Success Metrics Sprint 2

- [ ] 5 zusätzliche Commands implementiert (Update, Delete, Assign, Complete, Reject)
- [ ] Validation Pipeline für alle Commands aktiv
- [ ] Serilog konfiguriert mit Console + File Sinks
- [ ] 12+ Unit Tests erstellt
- [ ] Test Coverage mindestens 30%
- [ ] Alle Tests grün (100% Pass Rate)

---

## Command Overview (Nach Sprint 2)

### Documents (5 Commands/Queries)
- ✅ CreateDocumentCommand
- [ ] UpdateDocumentCommand
- [ ] DeleteDocumentCommand
- ✅ GetDocumentQuery
- ✅ GetDocumentsQuery

### Inbox - VIS (3 Commands/Queries)
- ✅ CreateInboxItemCommand
- [ ] AssignInboxItemCommand
- ✅ GetInboxItemsQuery

### Reminders - VIS (3 Commands/Queries)
- ✅ CreateReminderCommand
- [ ] CompleteReminderCommand
- ✅ GetDueRemindersQuery

### Cosigning - VIS (2 Commands)
- ✅ ApproveCosigningStepCommand
- [ ] RejectCosigningStepCommand

### Tasks (1 Query)
- ✅ GetMyTasksQuery

### Navigation (2 Queries)
- ✅ GetNavigationPathQuery
- ✅ GetRelatedEntitiesQuery

### Favorites (4 Commands/Queries)
- ✅ AddToFavoritesCommand
- ✅ RemoveFromFavoritesCommand
- ✅ GetFavoritesQuery
- ✅ IsFavoriteQuery

**Total: 23 Commands/Queries** (18 ✅ + 5 🆕)

---

## Implementation Order

1. **Week 1:**
   - Day 1-2: Additional Commands (Update, Delete, Assign, Complete, Reject)
   - Day 3: Validation Pipeline Behavior
   - Day 4: Serilog Integration
   - Day 5: First Unit Tests (Commands)

2. **Week 2:**
   - Day 1-2: More Unit Tests (Queries, Validators)
   - Day 3: Validation Behavior Tests
   - Day 4: Documentation (TESTING_GUIDE.md, LOGGING_GUIDE.md)
   - Day 5: Code Review & Sprint Abschluss

---

## Next: Sprint 3-4 (Plugin System)

Nach Abschluss von Sprint 2 folgt:
- Plugin Architecture (IDocumentPlugin Interface)
- Plugin Manager mit Dependency Injection
- Core Plugins: OCR, Auto-Classification, PDF Watermark
- Hot-Reload Funktionalität
- Plugin Marketplace (optional)
