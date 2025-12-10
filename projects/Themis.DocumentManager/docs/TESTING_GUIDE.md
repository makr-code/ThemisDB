# Testing Guide - Themis.DocumentManager

**Version:** 1.0  
**Date:** 2025-12-10  
**Target Coverage:** 30%+ (Sprint 2), 80%+ (End of Phase 1)

---

## 🎯 Testing Strategy

### Testing Pyramid

```
           /\
          /  \     E2E Tests (10%)
         /____\    - Full application workflows
        /      \   - UI automation
       /________\  Integration Tests (20%)
      /          \ - API integration
     /____________\- Database integration
    /              \
   /________________\ Unit Tests (70%)
  - Commands        - Queries
  - Validators      - Behaviors
```

---

## 📁 Test Project Structure

```
Tests/
├── Application.UnitTests/
│   ├── Documents/
│   │   ├── Commands/
│   │   │   ├── CreateDocumentCommandTests.cs
│   │   │   ├── CreateDocumentCommandValidatorTests.cs
│   │   │   ├── UpdateDocumentCommandTests.cs
│   │   │   └── DeleteDocumentCommandTests.cs
│   │   └── Queries/
│   │       ├── GetDocumentQueryTests.cs
│   │       └── GetDocumentsQueryTests.cs
│   ├── Inbox/
│   ├── Reminders/
│   ├── Cosigning/
│   ├── Tasks/
│   ├── Navigation/
│   ├── Favorites/
│   └── Common/
│       ├── ValidationBehaviorTests.cs
│       └── TestHelpers.cs
├── Infrastructure.IntegrationTests/
│   ├── Persistence/
│   │   └── ThemisRepositoryTests.cs
│   └── TestFixtures/
│       └── ThemisDbTestFixture.cs
└── UI.E2ETests/
    ├── Dashboard/
    ├── TaskBasket/
    └── Navigation/
```

---

## 🧪 Unit Testing

### 1. Command Handler Tests

**Pattern: Arrange-Act-Assert (AAA)**

```csharp
public class CreateDocumentCommandTests
{
    private readonly Mock<IThemisRepository> _mockRepository;
    private readonly Mock<IMediator> _mockMediator;
    private readonly Mock<ILogger<CreateDocumentCommandHandler>> _mockLogger;
    private readonly CreateDocumentCommandHandler _handler;

    public CreateDocumentCommandTests()
    {
        _mockRepository = new Mock<IThemisRepository>();
        _mockMediator = new Mock<IMediator>();
        _mockLogger = new Mock<ILogger<CreateDocumentCommandHandler>>();
        
        _handler = new CreateDocumentCommandHandler(
            _mockRepository.Object,
            _mockMediator.Object,
            _mockLogger.Object);
    }

    [Fact]
    public async Task Handle_ValidCommand_ReturnsDocumentId()
    {
        // Arrange
        var command = new CreateDocumentCommand
        {
            Title = "Test.pdf",
            Filename = "test.pdf",
            Author = "test@example.com",
            MimeType = "application/pdf"
        };

        _mockRepository
            .Setup(r => r.CreateDocumentAsync(
                It.IsAny<Document>(), 
                It.IsAny<CancellationToken>()))
            .ReturnsAsync("doc123");

        // Act
        var result = await _handler.Handle(command, CancellationToken.None);

        // Assert
        result.Should().Be("doc123");
        
        _mockRepository.Verify(r => r.CreateDocumentAsync(
            It.Is<Document>(d => 
                d.Title == "Test.pdf" && 
                d.Author == "test@example.com"),
            It.IsAny<CancellationToken>()), 
            Times.Once);
    }

    [Fact]
    public async Task Handle_RepositoryFails_ThrowsException()
    {
        // Arrange
        var command = new CreateDocumentCommand
        {
            Title = "Test.pdf",
            Filename = "test.pdf",
            Author = "test@example.com"
        };

        _mockRepository
            .Setup(r => r.CreateDocumentAsync(
                It.IsAny<Document>(), 
                It.IsAny<CancellationToken>()))
            .ThrowsAsync(new InvalidOperationException("Database error"));

        // Act & Assert
        await Assert.ThrowsAsync<InvalidOperationException>(
            () => _handler.Handle(command, CancellationToken.None));
        
        // Verify logger was called
        _mockLogger.Verify(
            x => x.Log(
                LogLevel.Error,
                It.IsAny<EventId>(),
                It.IsAny<It.IsAnyType>(),
                It.IsAny<Exception>(),
                It.IsAny<Func<It.IsAnyType, Exception?, string>>()),
            Times.Once);
    }

    [Fact]
    public async Task Handle_ValidCommand_PublishesDocumentCreatedEvent()
    {
        // Arrange
        var command = new CreateDocumentCommand
        {
            Title = "Test.pdf",
            Filename = "test.pdf",
            Author = "test@example.com"
        };

        _mockRepository
            .Setup(r => r.CreateDocumentAsync(
                It.IsAny<Document>(), 
                It.IsAny<CancellationToken>()))
            .ReturnsAsync("doc123");

        // Act
        await _handler.Handle(command, CancellationToken.None);

        // Assert
        _mockMediator.Verify(m => m.Publish(
            It.Is<DocumentCreatedEvent>(e => 
                e.DocumentId == "doc123" && 
                e.Title == "Test.pdf"),
            It.IsAny<CancellationToken>()), 
            Times.Once);
    }
}
```

### 2. Validator Tests

**Use Theory for multiple test cases:**

```csharp
public class CreateDocumentCommandValidatorTests
{
    private readonly CreateDocumentCommandValidator _validator;

    public CreateDocumentCommandValidatorTests()
    {
        _validator = new CreateDocumentCommandValidator();
    }

    [Theory]
    [InlineData("")]
    [InlineData(null)]
    [InlineData("   ")]
    public async Task Validate_EmptyOrNullTitle_ShouldHaveValidationError(string title)
    {
        // Arrange
        var command = new CreateDocumentCommand 
        { 
            Title = title,
            Filename = "test.pdf",
            Author = "test@example.com"
        };

        // Act
        var result = await _validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().Contain(e => 
            e.PropertyName == "Title" &&
            e.ErrorMessage.Contains("erforderlich"));
    }

    [Fact]
    public async Task Validate_TitleExceedsMaxLength_ShouldHaveValidationError()
    {
        // Arrange
        var command = new CreateDocumentCommand
        {
            Title = new string('a', 201), // > 200 chars
            Filename = "test.pdf",
            Author = "test@example.com"
        };

        // Act
        var result = await _validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().Contain(e => 
            e.PropertyName == "Title" &&
            e.ErrorMessage.Contains("200"));
    }

    [Theory]
    [InlineData("invalid-email")]
    [InlineData("@example.com")]
    [InlineData("user@")]
    public async Task Validate_InvalidEmail_ShouldHaveValidationError(string email)
    {
        // Arrange
        var command = new CreateDocumentCommand
        {
            Title = "Test.pdf",
            Filename = "test.pdf",
            Author = email
        };

        // Act
        var result = await _validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeFalse();
        result.Errors.Should().Contain(e => 
            e.PropertyName == "Author" &&
            e.ErrorMessage.Contains("E-Mail"));
    }

    [Fact]
    public async Task Validate_ValidCommand_ShouldNotHaveValidationErrors()
    {
        // Arrange
        var command = new CreateDocumentCommand
        {
            Title = "Valid Document.pdf",
            Filename = "document.pdf",
            Author = "user@example.com",
            MimeType = "application/pdf"
        };

        // Act
        var result = await _validator.ValidateAsync(command);

        // Assert
        result.IsValid.Should().BeTrue();
        result.Errors.Should().BeEmpty();
    }
}
```

### 3. Query Handler Tests

```csharp
public class GetDocumentQueryTests
{
    [Fact]
    public async Task Handle_ExistingDocument_ReturnsDocument()
    {
        // Arrange
        var mockRepo = new Mock<IThemisRepository>();
        var expectedDoc = new Document
        {
            Id = "doc123",
            Title = "Test.pdf",
            Author = "test@example.com"
        };

        mockRepo
            .Setup(r => r.GetDocumentAsync("doc123", It.IsAny<CancellationToken>()))
            .ReturnsAsync(expectedDoc);

        var handler = new GetDocumentQueryHandler(mockRepo.Object);
        var query = new GetDocumentQuery("doc123");

        // Act
        var result = await handler.Handle(query, CancellationToken.None);

        // Assert
        result.Should().NotBeNull();
        result.Id.Should().Be("doc123");
        result.Title.Should().Be("Test.pdf");
    }

    [Fact]
    public async Task Handle_NonExistingDocument_ThrowsNotFoundException()
    {
        // Arrange
        var mockRepo = new Mock<IThemisRepository>();
        mockRepo
            .Setup(r => r.GetDocumentAsync("doc999", It.IsAny<CancellationToken>()))
            .ReturnsAsync((Document?)null);

        var handler = new GetDocumentQueryHandler(mockRepo.Object);
        var query = new GetDocumentQuery("doc999");

        // Act & Assert
        await Assert.ThrowsAsync<DocumentNotFoundException>(
            () => handler.Handle(query, CancellationToken.None));
    }
}
```

### 4. Pipeline Behavior Tests

```csharp
public class ValidationBehaviorTests
{
    [Fact]
    public async Task Handle_ValidRequest_CallsNextDelegate()
    {
        // Arrange
        var validators = new List<IValidator<TestCommand>>();
        var behavior = new ValidationBehavior<TestCommand, string>(validators);
        var command = new TestCommand { Value = "Valid" };
        var nextCalled = false;
        RequestHandlerDelegate<string> next = () =>
        {
            nextCalled = true;
            return Task.FromResult("result");
        };

        // Act
        var result = await behavior.Handle(command, next, CancellationToken.None);

        // Assert
        nextCalled.Should().BeTrue();
        result.Should().Be("result");
    }

    [Fact]
    public async Task Handle_InvalidRequest_ThrowsValidationException()
    {
        // Arrange
        var validator = new TestCommandValidator(); // Has validation rules
        var validators = new List<IValidator<TestCommand>> { validator };
        var behavior = new ValidationBehavior<TestCommand, string>(validators);
        var command = new TestCommand { Value = "" }; // Invalid
        RequestHandlerDelegate<string> next = () => Task.FromResult("result");

        // Act & Assert
        var exception = await Assert.ThrowsAsync<ValidationException>(
            () => behavior.Handle(command, next, CancellationToken.None));
        
        exception.Errors.Should().NotBeEmpty();
    }

    [Fact]
    public async Task Handle_MultipleValidators_AggregatesErrors()
    {
        // Arrange
        var validator1 = new TestCommandValidator1();
        var validator2 = new TestCommandValidator2();
        var validators = new List<IValidator<TestCommand>> { validator1, validator2 };
        var behavior = new ValidationBehavior<TestCommand, string>(validators);
        var command = new TestCommand { Value = "" }; // Fails both validators
        RequestHandlerDelegate<string> next = () => Task.FromResult("result");

        // Act & Assert
        var exception = await Assert.ThrowsAsync<ValidationException>(
            () => behavior.Handle(command, next, CancellationToken.None));
        
        exception.Errors.Should().HaveCountGreaterThan(1);
    }
}

// Test helper classes
public class TestCommand : IRequest<string>
{
    public string Value { get; set; } = string.Empty;
}

public class TestCommandValidator : AbstractValidator<TestCommand>
{
    public TestCommandValidator()
    {
        RuleFor(x => x.Value).NotEmpty();
    }
}
```

---

## 🔗 Integration Testing

### Repository Integration Tests

```csharp
public class ThemisRepositoryTests : IClassFixture<ThemisDbTestFixture>
{
    private readonly ThemisDbTestFixture _fixture;

    public ThemisRepositoryTests(ThemisDbTestFixture fixture)
    {
        _fixture = fixture;
    }

    [Fact]
    public async Task CreateDocumentAsync_ValidDocument_ReturnsDocumentId()
    {
        // Arrange
        var repository = _fixture.CreateRepository();
        var document = new Document
        {
            Title = "Integration Test.pdf",
            Author = "test@example.com",
            Content = new byte[] { 1, 2, 3 }
        };

        // Act
        var documentId = await repository.CreateDocumentAsync(
            document, 
            CancellationToken.None);

        // Assert
        documentId.Should().NotBeNullOrEmpty();

        // Verify persisted
        var retrieved = await repository.GetDocumentAsync(
            documentId, 
            CancellationToken.None);
        
        retrieved.Should().NotBeNull();
        retrieved!.Title.Should().Be("Integration Test.pdf");
    }

    [Fact]
    public async Task GetDocumentsAsync_WithPagination_ReturnsCorrectPage()
    {
        // Arrange
        var repository = _fixture.CreateRepository();
        
        // Seed test data
        for (int i = 0; i < 100; i++)
        {
            await repository.CreateDocumentAsync(
                new Document { Title = $"Doc {i}" },
                CancellationToken.None);
        }

        // Act
        var page1 = await repository.GetDocumentsAsync(1, 20, CancellationToken.None);
        var page2 = await repository.GetDocumentsAsync(2, 20, CancellationToken.None);

        // Assert
        page1.Should().HaveCount(20);
        page2.Should().HaveCount(20);
        page1.First().Id.Should().NotBe(page2.First().Id);
    }
}

// Test Fixture
public class ThemisDbTestFixture : IDisposable
{
    private readonly string _testDbName;

    public ThemisDbTestFixture()
    {
        _testDbName = $"ThemisTest_{Guid.NewGuid()}";
        // Initialize test database
    }

    public IThemisRepository CreateRepository()
    {
        return new ThemisRepository(/* test connection string */);
    }

    public void Dispose()
    {
        // Clean up test database
    }
}
```

---

## 🌐 E2E Testing

### UI Automation with Selenium/WinAppDriver

```csharp
public class DashboardE2ETests : IClassFixture<AppTestFixture>
{
    private readonly AppTestFixture _fixture;

    public DashboardE2ETests(AppTestFixture fixture)
    {
        _fixture = fixture;
    }

    [Fact]
    public void Dashboard_OnStartup_DisplaysStatistics()
    {
        // Arrange
        var driver = _fixture.Driver;

        // Act
        var totalDocs = driver.FindElement(By.Id("TotalDocuments")).Text;
        var pendingTasks = driver.FindElement(By.Id("PendingTasks")).Text;

        // Assert
        totalDocs.Should().NotBeNullOrEmpty();
        pendingTasks.Should().NotBeNullOrEmpty();
    }

    [Fact]
    public void QuickAction_NewDocument_OpensDialog()
    {
        // Arrange
        var driver = _fixture.Driver;

        // Act
        var newDocButton = driver.FindElement(By.Id("NewDocumentButton"));
        newDocButton.Click();

        // Wait for dialog
        var dialog = driver.FindElement(By.ClassName("CreateDocumentDialog"));

        // Assert
        dialog.Should().NotBeNull();
        dialog.Displayed.Should().BeTrue();
    }
}
```

---

## 📊 Code Coverage

### Measuring Coverage

**Tools:**
- **Coverlet** - Cross-platform code coverage library
- **ReportGenerator** - Coverage report visualization

**Installation:**
```bash
dotnet add package coverlet.collector
dotnet tool install -g dotnet-reportgenerator-globaltool
```

**Run tests with coverage:**
```bash
dotnet test /p:CollectCoverage=true /p:CoverletOutputFormat=cobertura

reportgenerator \
  -reports:coverage.cobertura.xml \
  -targetdir:coveragereport \
  -reporttypes:Html
```

### Coverage Goals

| Phase | Target | Actual |
|-------|--------|--------|
| Sprint 2 | 30% | 35% |
| Sprint 3-4 | 50% | TBD |
| Phase 1 End | 80% | TBD |

### What to Cover

**Priority 1 (Must have 90%+):**
- Domain Layer (Entities, Value Objects)
- Application Layer (Commands, Queries, Validators)
- Business Logic

**Priority 2 (Target 70%+):**
- Infrastructure Layer (Repositories)
- Pipeline Behaviors

**Priority 3 (Target 30%+):**
- UI ViewModels
- Converters
- Helpers

**Excluded:**
- Auto-generated code
- Third-party libraries
- Configuration files

---

## 🎨 Best Practices

### 1. Use Descriptive Test Names

```csharp
// ❌ BAD
[Fact]
public void Test1() { }

// ✅ GOOD
[Fact]
public void Handle_ValidCommand_ReturnsDocumentId() { }

[Fact]
public void Validate_EmptyTitle_ShouldHaveValidationError() { }
```

### 2. Follow AAA Pattern

```csharp
[Fact]
public async Task TestMethod()
{
    // Arrange - Setup test data and mocks
    var mock = new Mock<IRepository>();
    mock.Setup(r => r.GetAsync()).ReturnsAsync(data);
    
    // Act - Execute the method being tested
    var result = await handler.Handle(command);
    
    // Assert - Verify the outcome
    result.Should().Be(expected);
}
```

### 3. One Assert Per Test (when possible)

```csharp
// ❌ AVOID
[Fact]
public void Test()
{
    result.Should().NotBeNull();
    result.Id.Should().Be("123");
    result.Title.Should().Be("Test");
    // Too many asserts - hard to identify failure
}

// ✅ PREFER
[Fact]
public void Handle_ValidCommand_ReturnsNonNullResult()
{
    result.Should().NotBeNull();
}

[Fact]
public void Handle_ValidCommand_ReturnsCorrectId()
{
    result.Id.Should().Be("123");
}
```

### 4. Use Test Data Builders

```csharp
public class DocumentBuilder
{
    private string _title = "Default.pdf";
    private string _author = "test@example.com";

    public DocumentBuilder WithTitle(string title)
    {
        _title = title;
        return this;
    }

    public DocumentBuilder WithAuthor(string author)
    {
        _author = author;
        return this;
    }

    public Document Build()
    {
        return new Document
        {
            Title = _title,
            Author = _author
        };
    }
}

// Usage
var document = new DocumentBuilder()
    .WithTitle("Custom.pdf")
    .WithAuthor("custom@example.com")
    .Build();
```

### 5. Use FluentAssertions

```csharp
// ❌ Basic Assert
Assert.Equal("expected", actual);

// ✅ FluentAssertions - more readable
actual.Should().Be("expected");

// ✅ Better error messages
result.Should().NotBeNull("because the query should always return a result");

// ✅ Collection assertions
list.Should().HaveCount(5);
list.Should().Contain(x => x.Id == "123");
list.Should().AllSatisfy(x => x.Status == Status.Active);
```

---

## 🚀 CI/CD Integration

### GitHub Actions

```yaml
name: Test

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup .NET
      uses: actions/setup-dotnet@v3
      with:
        dotnet-version: 8.0.x
    
    - name: Restore dependencies
      run: dotnet restore
    
    - name: Build
      run: dotnet build --no-restore
    
    - name: Test
      run: dotnet test --no-build --verbosity normal /p:CollectCoverage=true
    
    - name: Upload coverage
      uses: codecov/codecov-action@v3
```

---

## 📚 Resources

**xUnit Documentation:** https://xunit.net/  
**Moq Documentation:** https://github.com/moq/moq4  
**FluentAssertions:** https://fluentassertions.com/  
**Coverlet:** https://github.com/coverlet-coverage/coverlet

---

**Last Updated:** 2025-12-10  
**Next Review:** Sprint 3-4
