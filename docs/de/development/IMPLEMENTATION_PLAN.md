# Railway Monitor - Gap Closure Implementation Plan

## Status: IN PROGRESS

### Overview
Based on comprehensive gap analysis, implementing critical & high priority improvements to achieve AAA-game quality.

**Timeline**: 27 weeks (6.5 months)
**Team**: 3 developers recommended  
**Budget**: ~€150K

---

## Phase 1: Unit Testing Infrastructure ⏳ CURRENT (Weeks 1-6)

### Objectives
- Achieve 80%+ code coverage
- Establish testing best practices
- Enable confident refactoring for future improvements

### Tasks

#### Week 1-2: Test Framework Setup ✅ NEXT
- [x] Create test project structure
- [ ] Add NuGet packages (xUnit, FluentAssertions, Moq, Coverlet)
- [ ] Configure test runners
- [ ] Setup code coverage reporting

#### Week 3-4: Core Service Tests
- [ ] ThemisDbService tests (REST/AQL integration)
- [ ] EnergyManagementService tests (power calculations)
- [ ] ChangeFeedService tests (SSE streaming)
- [ ] OllamaService tests (LLM integration)
- [ ] TrainSimulatorService tests (process lifecycle)

#### Week 5-6: Advanced Service Tests
- [ ] GeoSpatialAnalyzer tests (A* pathfinding)
- [ ] RealDataProvider tests (data integration)
- [ ] CrossingAnalyzer tests (traffic detection)
- [ ] SettlementAnalyzer tests (urban constraints)
- [ ] Data Pipeline tests (download/cache)
- [ ] ML Service tests (prediction accuracy)

### Deliverables
- Test projects with 80%+ coverage
- Automated test execution
- Coverage reports
- Testing documentation

---

## Phase 2: Veldrid Graphics Backend (Weeks 7-10)

### Objectives
- Cross-platform DirectX 11/12 + Vulkan support
- GPU instancing for massive entity counts
- Modern shader pipeline

### Tasks
- [ ] NuGet packages (Veldrid, Veldrid.StartupUtilities, Veldrid.SPIRV)
- [ ] GraphicsDevice abstraction layer
- [ ] Shader compilation pipeline (SPIR-V)
- [ ] GPU buffer management (vertices, indices, uniforms)
- [ ] Texture loading and sampling
- [ ] Instanced rendering implementation
- [ ] Integration with RailwayMapRenderer
- [ ] Performance benchmarks

### Deliverables
- Veldrid-based rendering backend
- DirectX 11/12 + Vulkan support
- 10x rendering performance improvement
- Shader library (vertex, fragment, compute)

---

## Phase 3: ECS Migration (Weeks 11-16)

### Objectives
- Data-oriented architecture
- 10x entity processing performance
- Support 500K+ entities @ 60 FPS

### Tasks
- [ ] NuGet package (Arch)
- [ ] Component definitions (Position, Velocity, Renderable, etc.)
- [ ] System definitions (MovementSystem, RenderSystem, etc.)
- [ ] Entity creation/destruction API
- [ ] Query optimization
- [ ] Migration from OOP classes
- [ ] Performance benchmarks

### Deliverables
- Arch ECS implementation
- 500K+ entities @ 60 FPS
- Component/System architecture
- Migration guide

---

## Phase 4: PostGIS Integration (Weeks 17-18)

### Objectives
- High-performance spatial queries
- <10ms for millions of polygons
- Professional GIS capabilities

### Tasks
- [ ] NuGet packages (Npgsql, Npgsql.NetTopologySuite)
- [ ] Docker Compose (PostgreSQL + PostGIS)
- [ ] Schema migration from SQLite
- [ ] GIST spatial indices
- [ ] Spatial query optimization
- [ ] Connection pooling
- [ ] Performance benchmarks

### Deliverables
- PostgreSQL + PostGIS database
- Optimized spatial queries
- 100x query performance improvement
- Migration scripts

---

## Phase 5: MLOps Platform (Weeks 19-21)

### Objectives
- Model versioning and registry
- 2-5x faster ML inference
- A/B testing capabilities

### Tasks
- [ ] NuGet package (Microsoft.ML.OnnxRuntime)
- [ ] Model registry setup
- [ ] ONNX model conversion (ML.NET → ONNX)
- [ ] A/B testing framework
- [ ] Performance monitoring
- [ ] Model versioning
- [ ] Rollback mechanisms

### Deliverables
- ONNX Runtime integration
- Model registry
- 2-5x inference speedup
- A/B testing framework

---

## Phase 6: Track Network Graph (Weeks 22-24)

### Objectives
- Graph-based pathfinding
- Signal/interlock integration
- Route conflict detection

### Tasks
- [ ] Graph data structure (nodes, edges)
- [ ] Dijkstra/A* on track graph
- [ ] Signal state integration
- [ ] Interlock logic
- [ ] Route conflict detection
- [ ] Timetable optimization

### Deliverables
- Track network graph
- Graph-based pathfinding
- Signal/interlock system
- Conflict detection

---

## Phase 7: Physics Engine (Weeks 25-28)

### Objectives
- Realistic train dynamics
- Accurate simulation
- Industry-standard physics

### Tasks
- [ ] NuGet package (BepuPhysics, BepuUtilities)
- [ ] Train dynamics model
- [ ] Tractive effort curves
- [ ] Davis resistance formula
- [ ] Pneumatic brake simulation
- [ ] Grade/curve restrictions
- [ ] Performance benchmarks

### Deliverables
- BEPUphysics v2 integration
- Realistic train dynamics
- Physics simulation
- Validation against real data

---

## Success Metrics

### Performance Targets
| Metric | Current | Target | Improvement |
|--------|---------|--------|-------------|
| Entities | 50K @ 30 FPS | 500K+ @ 60 FPS | 10x |
| Spatial Queries | Variable | <10ms | 100x |
| ML Inference | <10ms | <5ms | 2x |
| Cache Hit Rate | 85% | 90%+ | +5% |

### Quality Targets
- Test Coverage: 0% → 80%+
- Code Quality: B+ → A
- AAA Readiness: 70% → 95%+

---

## Risk Mitigation

### Technical Risks
1. **ECS Migration Complexity**: Phased migration, fallback to OOP
2. **Veldrid Learning Curve**: Extensive documentation, examples
3. **PostGIS Performance**: Indexing strategy, query optimization
4. **ONNX Compatibility**: Model validation, fallback to ML.NET

### Schedule Risks
1. **Underestimated Effort**: 20% buffer built into timeline
2. **Resource Availability**: Cross-training team members
3. **Dependency Issues**: Regular dependency updates

---

## Next Steps

**Immediate (This Week)**:
1. ✅ Setup test project structure
2. Add xUnit + FluentAssertions + Moq
3. Write first test suite (ThemisDbService)
4. Configure code coverage

**Short-term (Next 2 Weeks)**:
5. Complete core service tests
6. Achieve 40% coverage milestone
7. Setup automated test execution
8. Document testing patterns

**Medium-term (Weeks 3-6)**:
9. Complete all service tests
10. Achieve 80% coverage target
11. Performance benchmarks
12. Prepare for Veldrid integration

---

## Resources

### Documentation
- xUnit: https://xunit.net/
- FluentAssertions: https://fluentassertions.com/
- Moq: https://github.com/moq/moq4
- Veldrid: https://veldrid.dev/
- Arch ECS: https://github.com/genaray/Arch
- PostGIS: https://postgis.net/
- ONNX Runtime: https://onnxruntime.ai/
- BEPUphysics: https://github.com/bepu/bepuphysics2

### Team Contacts
- Lead Developer: TBD
- Test Engineer: TBD
- DevOps Engineer: TBD

---

**Last Updated**: 2026-04-06
**Status**: Phase 1 (Unit Testing) in progress
**Progress**: 5% complete (framework setup)
