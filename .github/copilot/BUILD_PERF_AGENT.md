# @BuildPerfCpp Agent Configuration

This guide defines how to run build-performance analysis and optimization tasks for ThemisDB C++ builds.

## Purpose

Use `@BuildPerfCpp` to identify and reduce avoidable build-time costs without changing runtime semantics.

## Primary Analysis Inputs

- ETL trace outputs from compiler/build profiling runs
- CMake target graph and transitive include structure
- Compile command diagnostics and per-target compilation hotspots

## Analysis Goals

Focus on identifying:

- Expensive header inclusion patterns and high fan-out include chains
- Slow template instantiation hotspots
- Inefficient function generation/translation-unit structure
- Rebuild amplification from unstable dependency boundaries

## Output Requirements

Each run should provide:

- Ranked bottleneck list with evidence
- Proposed remediations ordered by cost/impact
- Scope of affected targets/modules
- Verification plan (baseline vs. post-change timing)

## Integration with Existing CMake Targets

Use existing project targets and presets for measurements and verification. Prefer incremental target-level validation before full workspace rebuilds.

## Safety Constraints

- No speculative optimization without measured bottlenecks
- Preserve behavior and ABI expectations unless explicitly approved
- Keep optimization changes small and reviewable
- Update relevant docs when build architecture assumptions change
