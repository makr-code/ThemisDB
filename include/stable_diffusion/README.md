# include stable_diffusion module

Public headers for Stable Diffusion plugin integration.

## Headers
- `sd_plugin.h`
- `sd_generator.h`
- `sd_config.h`
- `sd_prompt_sanitizer.h`

## Exposed API
- `SDPlugin` implementing `IImageGenerationBackend`
- Generator abstraction (`ISDGenerator`) with stub/in-memory implementations
- Prompt policy sanitizer and JSON config contract