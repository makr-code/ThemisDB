# ThemisDB Documentation Assistant

## Overview

The ThemisDB Documentation Assistant is an LLM-powered feature that helps administrators configure and troubleshoot ThemisDB using the integrated llama.cpp LLM and a pre-compiled documentation database.

## Features

- **Configuration Assistance**: Get help with ThemisDB configuration options and settings
- **Troubleshooting Support**: Receive guidance for resolving errors and issues
- **Documentation Search**: Quickly find relevant documentation using semantic search
- **RAG-powered Answers**: Context-aware responses based on actual documentation
- **Offline Operation**: Works entirely offline with no external API calls

## Architecture

The documentation assistant consists of three main components:

1. **Documentation Database**: Pre-compiled JSON database containing all documentation from `./docs` and `./compendium` directories
2. **DocsAssistant Class**: C++ component that loads and searches the documentation database
3. **LLM Integration**: Uses llama.cpp to generate context-aware answers using RAG (Retrieval Augmented Generation)

## Usage

### Via REST API

```bash
# Query the documentation assistant
curl -X POST http://localhost:8765/api/v1/llm/docs/query \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "query": "How do I configure sharding in ThemisDB?"
  }'

# Get configuration help
curl -X POST http://localhost:8765/api/v1/llm/docs/config \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "topic": "replication"
  }'

# Get troubleshooting help
curl -X POST http://localhost:8765/api/v1/llm/docs/troubleshoot \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "error": "Server hangs at 'Adaptive Index Manager initialized'"
  }'
```

### Via AQL

```sql
-- Query documentation
SELECT llm.docs_query('How do I enable LLM support?') AS answer;

-- Get configuration help
SELECT llm.docs_config_help('security') AS help;

-- Get troubleshooting help
SELECT llm.docs_troubleshoot('Connection timeout errors') AS solution;
```

### Via C++ API

```cpp
#include "llm/docs_assistant.h"

// Create documentation assistant
themis::llm::DocsAssistantConfig config;
config.docs_database_path = "data/docs_database.json";
config.max_context_docs = 5;

themis::llm::DocsAssistant assistant(config);

// Load documentation database
if (!assistant.loadDatabase()) {
    std::cerr << "Failed to load documentation database\n";
    return;
}

// Query for help
auto result = assistant.query("How do I configure RAID sharding?");

std::cout << "Answer: " << result.generated_answer << "\n";
std::cout << "Confidence: " << (result.confidence_score * 100) << "%\n";
std::cout << "Documents used: " << result.docs_included_in_context << "\n";
```

## Configuration

The documentation assistant can be configured using environment variables or configuration file:

### Environment Variables

- `THEMIS_DOCS_DATABASE_PATH`: Path to documentation database (default: `data/docs_database.json`)
- `THEMIS_DOCS_MAX_CONTEXT`: Maximum number of documents to include in context (default: 5)
- `THEMIS_DOCS_PREVIEW_LENGTH`: Characters to include per document (default: 1000)
- `THEMIS_ENABLE_DOCS_ASSISTANT`: Enable/disable documentation assistant (default: true if database exists)

### Configuration File (themis.yaml)

```yaml
llm:
  docs_assistant:
    enabled: true
    database_path: data/docs_database.json
    max_context_docs: 5
    context_preview_length: 1000
    enable_semantic_search: true
    enable_caching: true
```

## Building the Documentation Database

The documentation database is automatically generated during the build process. To manually generate or update it:

```bash
# Generate documentation database with default settings
python3 scripts/generate_docs_database.py

# Generate with custom output path
python3 scripts/generate_docs_database.py --output /path/to/docs_db.json

# Generate with docs only (exclude compendium)
python3 scripts/generate_docs_database.py --no-compendium

# Include example documentation
python3 scripts/generate_docs_database.py --include-examples
```

## Release Integration

The documentation database is automatically included in ThemisDB releases:

1. **Generation**: During the build process, `scripts/generate_docs_database.py` is run
2. **Packaging**: The generated `data/docs_database.json` is included in release archives
3. **Size**: The database is approximately 4-5 MB (compressed to ~1 MB in archives)

## Performance

- **Database Loading**: ~10-50ms (one-time on startup)
- **Document Search**: ~5-20ms (depending on database size)
- **Answer Generation**: ~500-2000ms (depending on LLM model and context size)
- **Total Query Time**: Typically <3 seconds for most queries

## Examples

### Configuration Questions

**Q**: "How do I enable sharding?"

**A**: To enable sharding in ThemisDB, you need to set the `THEMIS_ENABLE_SHARDING=true` environment variable. Additionally, configure the following:

- `THEMIS_SHARD_COUNT`: Number of shards (e.g., 3, 6, 9)
- `THEMIS_SHARD_REPLICATION_FACTOR`: Replication factor (1, 5, 6 for RAID 0, 5, 6)
- Configure shard endpoints in `config/shards.yaml`

Refer to: `docs/en/features/RAID_SHARDING.md` for complete configuration guide.

### Troubleshooting Questions

**Q**: "Server hangs at 'Adaptive Index Manager initialized'"

**A**: This is a known issue with RAID sharding when AdaptiveIndexManager attempts to open Column Families before ShardingManager is initialized. Solution:

1. Update to v1.3.4 or later (includes hotfix)
2. Ensure `THEMIS_ENABLE_SHARDING=true` is set correctly
3. Check docker-compose port mappings (should be `808X:8765` not `808X:8080`)

Refer to: `docs/RAID_SHARDING_DEADLOCK_HOTFIX.md`

## Limitations

1. **No Real-time Updates**: Database must be regenerated to include new documentation
2. **Keyword-based Search**: Currently uses simple keyword matching (vector embeddings planned for future)
3. **Context Window**: Limited to configured number of documents (default: 5)
4. **LLM Dependency**: Requires llama.cpp to be enabled (`THEMIS_ENABLE_LLM=ON`)

## Future Enhancements

- [ ] Vector embeddings for semantic search
- [ ] Multi-language support
- [ ] Code example extraction and execution
- [ ] Interactive configuration wizard
- [ ] Automatic documentation updates
- [ ] Integration with voice assistant

## License

Same as ThemisDB - MIT License

## Contributing

To improve the documentation assistant:

1. Improve documentation in `./docs` and `./compendium`
2. Regenerate database: `python3 scripts/generate_docs_database.py`
3. Test with various queries
4. Submit improvements via pull request

## Support

For issues with the documentation assistant:

1. Check that `data/docs_database.json` exists
2. Verify LLM is enabled (`THEMIS_ENABLE_LLM=ON`)
3. Check logs for errors: `logs/themis_server.log`
4. Report issues on GitHub: https://github.com/makr-code/ThemisDB/issues
