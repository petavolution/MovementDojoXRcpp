# Movement Dojo - Roadmap Improvement Plan

Based on the USD-based OpenXR Lightsaber Proprioception Trainer roadmap, here's a
prioritized plan to elevate the project from current Stage 2 to a full Stage 4 application.

## Current Status Assessment

| Component | Current | Target | Gap |
|-----------|---------|--------|-----|
| OpenXR Integration | 85% | 100% | Overlay extension, comfort options |
| USD Loading | 60% | 95% | VariantSets, Layers, textures |
| Rendering | 5% | 90% | Full Vulkan pipeline |
| Physics | 40% | 85% | CCD, slicing, constraints |
| Haptics | 80% | 95% | Pattern library |
| Training Modules | 70% | 95% | Path following, blocking |
| UI/UX | 10% | 90% | VR menus, text rendering |
| Progression | 80% | 95% | Cloud sync, stats |

## Phase 1: Critical Foundations (2-4 weeks)

### 1.1 Enable Physics CCD (Priority: CRITICAL, Effort: 1 day)
- Enable Bullet Physics Continuous Collision Detection
- Prevents fast-moving controllers from phasing through objects
- File: `src/physics/PhysicsEngine.cpp`

### 1.2 Vulkan Rendering Pipeline (Priority: CRITICAL, Effort: 4-6 weeks)
- Create `VulkanPipeline` class for actual GPU rendering
- Implement basic PBR shaders
- Support textures from USD materials
- Add bloom post-processing for saber/trails

### 1.3 Text Rendering (Priority: HIGH, Effort: 3-5 days)
- Integrate font rendering library (stb_truetype or FreeType)
- Implement signed distance field text for VR clarity
- Enable HUD element text display

## Phase 2: Enhanced Features (4-6 weeks)

### 2.1 Advanced USD Integration
- **VariantSets**: Switch between dojo layouts at runtime
  - `dojoVariantSet = {"Basic", "Advanced", "Master"}`
- **Layers**: Overlay training guides without modifying base scene
- **Textures**: Full UsdPreviewSurface material support

### 2.2 In-World VR UI System
- Floating menu panels in 3D space
- Laser pointer interaction from controllers
- Menu hierarchy: Main -> Training -> Settings -> Profile

### 2.3 Path Following Exercises
- 3D spline paths users trace with saber/hands
- Visual guides (glowing path, arrows)
- Accuracy scoring and haptic feedback

## Phase 3: Polish & Performance (4-6 weeks)

### 3.1 Advanced Rendering
- Shadow mapping (directional + spot)
- Screen-space reflections
- Environment probes for metallic materials
- Particle effects for saber trail

### 3.2 VR Comfort Options
- Snap turn / smooth turn toggle
- Vignette during movement
- Seated/standing mode
- Guardian boundary integration

### 3.3 Performance Optimization
- GPU instancing for repeated geometry
- View frustum culling
- Level of detail (LOD) system
- Async asset loading

## Phase 4: Robustness & Extensibility (4-6 weeks)

### 4.1 User Profiles
- Cloud sync option
- Multiple profile support
- Statistics dashboard

### 4.2 CI/CD Pipeline
- Automated builds (CMake + GitHub Actions)
- USD scene validation tests
- Performance regression tests

### 4.3 Content Extensibility
- Data-driven exercise definitions (JSON/USD)
- Modular saber/environment assets
- Community content support

## Quick Wins (Can implement now)

1. **Physics CCD** - 1 day, huge impact
2. **USD VariantSet parsing** - 2 days, enables dynamic scenes
3. **Haptic pattern library** - 1 day, better feedback
4. **Exercise data files** - 2 days, easier content creation

## Architecture Additions Needed

```
src/
├── rendering/
│   ├── VulkanPipeline.h/.cpp      # GPU rendering
│   ├── ShaderManager.h/.cpp        # Shader compilation
│   ├── TextRenderer.h/.cpp         # SDF text
│   └── PostProcess.h/.cpp          # Bloom, etc.
├── ui/
│   ├── VRMenuSystem.h/.cpp         # 3D menus
│   ├── MenuPanel.h/.cpp            # UI panels
│   └── LaserPointer.h/.cpp         # Controller interaction
├── usd/
│   ├── VariantSetManager.h/.cpp    # Dynamic variants
│   └── LayerManager.h/.cpp         # Layer composition
└── exercises/
    ├── PathFollowing.h/.cpp        # Trace paths
    ├── TargetBlocking.h/.cpp       # Deflect projectiles
    └── ExerciseLoader.h/.cpp       # Data-driven loading
```

## Estimated Timeline

| Phase | Duration | Milestone |
|-------|----------|-----------|
| Phase 1 | 4-6 weeks | Visible rendering, basic interactions |
| Phase 2 | 4-6 weeks | Full training experience |
| Phase 3 | 4-6 weeks | Polished product |
| Phase 4 | 4-6 weeks | Production-ready |

**Total: 4-6 months to production-ready**

## Next Steps

1. Enable Physics CCD immediately (quick win)
2. Create VulkanPipeline skeleton
3. Add USD VariantSet support
4. Design VR UI system architecture
