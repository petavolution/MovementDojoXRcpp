# MovementDojoXRcpp - VR Engine Framework Roadmap

## Overview

This roadmap aligns MovementDojoXRcpp with a **4-stage open-source VR engine framework** using OpenXR, SteamVR, glTF, USD, and a custom minimal C++ engine.

**Core Stack:**
- **Runtime**: OpenXR via SteamVR (PCVR) or Monado (Linux native)
- **Graphics**: Vulkan (primary), OpenGL (fallback)
- **Physics**: Bullet Physics (open-source)
- **Assets**: glTF 2.0 (runtime) + USD (authoring/metadata)
- **Audio**: miniaudio or OpenAL-soft

---

## Current State Assessment

### What's Implemented (Stage 3-4 Level)

| System | Completion | Notes |
|--------|------------|-------|
| OpenXR Session Management | 80% | Lifecycle, states, extensions ready |
| OpenXR Input/Actions | 85% | Pose, grip, trigger, haptic actions |
| USD Scene Loading | 70% | Variants, layers, mesh extraction |
| Physics Engine | 75% | Bullet integration, CCD support |
| Movement Analytics | 90% | Voxelization, patterns, trails |
| Training Modes (6) | 85% | All modes architected |
| Progression System | 90% | Levels, achievements, challenges |
| Haptic Feedback | 70% | Patterns, collision feedback |

### Critical Gaps (Stage 0-1 Level)

| System | Status | Priority |
|--------|--------|----------|
| **Vulkan Rendering** | 5% stubs | CRITICAL |
| **glTF Loading** | 0% | CRITICAL |
| **Texture System** | 0% | HIGH |
| **Audio Engine** | 0% | MEDIUM |
| **UI/HUD Rendering** | 0% | MEDIUM |

---

## Stage 1: OpenXR Handshake & glTF/USD Skeleton

**Motto:** "Prove the XR pipeline and external assets actually talk to each other."

### 1.1 Goals

- Stable OpenXR session with SteamVR/Monado
- Tracked controllers visualized in 3D
- glTF test mesh rendered in VR
- USD stage loaded with mesh extraction
- 90 Hz frame rate maintained

### 1.2 Vulkan Implementation Tasks

```
src/graphics/
├── VulkanContext.cpp/h       # Week 1-2
│   ├── VkInstance with XR extensions
│   ├── Physical device selection (XR compatible)
│   ├── Logical device + graphics queue
│   └── Command pool and buffer management
│
├── VulkanXRBinding.cpp/h     # Week 2-3
│   ├── XrGraphicsBindingVulkanKHR setup
│   ├── XrSwapchain creation with VkImage
│   ├── Swapchain image acquisition/release
│   └── Frame synchronization
│
├── VulkanRenderPass.cpp/h    # Week 3
│   ├── Render pass for XR composition
│   ├── Framebuffer per swapchain image
│   └── Clear and present operations
│
└── VulkanPipeline.cpp/h      # Week 4
    ├── Shader loading (SPIR-V)
    ├── Basic PBR pipeline
    ├── Vertex input configuration
    └── Descriptor set layouts
```

### 1.3 glTF Loader Implementation

```
src/assets/GLTFLoader.cpp/h   # Week 2-3
├── cgltf integration (header-only)
├── Buffer/accessor parsing
├── Mesh extraction
│   ├── Vertices (position, normal, UV)
│   ├── Indices
│   └── Primitive assembly
├── Material extraction
│   ├── PBR metallic-roughness
│   ├── Texture references
│   └── Factor values
├── Scene hierarchy
│   ├── Node traversal
│   ├── Transform composition
│   └── Mesh instantiation
└── Integration with SceneObject
```

### 1.4 Asset Pipeline Strategy

```
Authoring (Blender)
       │
       ├── Export glTF 2.0 → Runtime meshes/materials
       │
       └── Export USD → Scene graph, variants, metadata
              │
              └── USD custom attributes for training data
                  - exercise:type
                  - exercise:duration
                  - spawn:pattern
                  - path:waypoints
```

### 1.5 Stage 1 Deliverables

- [ ] `VulkanContext` initializes with OpenXR requirements
- [ ] `XRSession::beginFrame()/endFrame()` renders to HMD
- [ ] Controller poses rendered as cubes/spheres
- [ ] `GLTFLoader` loads test mesh (saber handle)
- [ ] `USDLoader` loads `stage1_cube.usda` with mesh
- [ ] Ground plane + skybox rendering
- [ ] 90 Hz on GTX 1070+ equivalent

### 1.6 Architecture (Stage 1)

```
┌─────────────────────────────────────────────────────────────┐
│                     XRSessionManager                         │
│  - OpenXR instance/session lifecycle                        │
│  - Frame timing (xrWaitFrame/BeginFrame/EndFrame)          │
│  - View configuration and projection matrices              │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     XRInputManager                           │
│  - Action sets and bindings                                 │
│  - Controller pose polling                                  │
│  - Haptic action triggers                                   │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                       Renderer                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │   Vulkan    │  │   Scene     │  │    Debug Draw       │ │
│  │   Context   │  │   Graph     │  │    (cubes/lines)    │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      AssetLoader                             │
│  ┌─────────────────────┐  ┌─────────────────────────────┐  │
│  │    GLTFLoader       │  │      USDLoader              │  │
│  │  - Meshes           │  │  - Scene graph              │  │
│  │  - Materials        │  │  - Variants                 │  │
│  │  - Textures         │  │  - Metadata                 │  │
│  └─────────────────────┘  └─────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Stage 2: Lightsaber Loop & glTF Dojo

**Motto:** "Swing the saber, hit something, and feel it."

### 2.1 Goals

- glTF dojo environment rendered
- Lightsaber attached to controller with physics
- Collision detection with targets/environment
- Haptic feedback on collisions
- Basic "Hit N Targets" training module

### 2.2 Physics Integration Tasks

```
Connect existing PhysicsEngine:
├── Static bodies for dojo environment
├── Kinematic body for saber handle (follows controller)
├── Dynamic bodies for targets
├── Capsule collider for saber blade
├── CCD enabled for fast swings
├── Collision callbacks → HapticManager
└── Collision callbacks → TrainingModule events
```

### 2.3 Lightsaber Implementation

```cpp
// Saber state machine
enum class SaberState { Retracted, Extending, Extended, Retracting };

// Visual components
- Handle mesh (glTF)
- Blade mesh (emissive material)
- Blade trail (line segments or particle ribbon)

// Physics components
- Blade capsule collider
- CCD motion threshold: 0.001 (for fast swings)
- Collision layers: BLADE, TARGET, ENVIRONMENT
```

### 2.4 Training Module: HitTargets

```cpp
class HitTargetsModule : public TrainingModuleBase {
    // Configuration
    int targetCount = 10;
    float spawnRadius = 1.5f;
    float targetSize = 0.1f;

    // State
    std::vector<Target> activeTargets;
    int targetsHit = 0;
    float sessionTime = 0;

    // Events
    void onTargetHit(const CollisionEvent& e) {
        playSound(SFX_HIT);
        triggerHaptic(HAPTIC_HIT_TARGET);
        destroyTarget(e.targetId);
        targetsHit++;
        awardXP(10);
    }
};
```

### 2.5 Stage 2 Deliverables

- [ ] Dojo environment from `dojo_basic.usda` + glTF meshes
- [ ] Saber handle follows dominant controller
- [ ] Blade extends/retracts with input
- [ ] Blade collides with environment (sparks + haptics)
- [ ] Targets spawn and can be destroyed
- [ ] "Hit N Targets" module fully playable
- [ ] Basic scoring and session stats

---

## Stage 3: Proprioception & Movement Fidelity

**Motto:** "Turn a toy into a proprioception trainer."

### 3.1 Goals

- High-quality PBR rendering
- All 6 FlowModes fully functional
- Movement analytics visualization
- USD metadata drives training parameters
- Performance profiled and optimized

### 3.2 Visual Fidelity Tasks

```
Rendering upgrades:
├── Full PBR material system
│   ├── Albedo, normal, metallic, roughness, AO maps
│   └── IBL (image-based lighting)
├── Post-processing
│   ├── Bloom (for saber blade)
│   ├── FXAA/TAA
│   └── Vignette (comfort)
├── Dynamic lighting
│   ├── Point lights (saber glow)
│   └── Shadow mapping (optional)
└── LOD system
    └── USD VariantSets for detail levels
```

### 3.3 FlowMode Integration

Your existing FlowModes need rendering integration:

| Mode | Visualization Needed |
|------|---------------------|
| FreeExplorationMode | Coverage spheres, trail rendering |
| GuidedStretchMode | Ghost target poses, alignment indicators |
| BreathingSyncMode | Breath phase visualizer, rhythm guides |
| MirrorMode | Ghost playback rendering |
| MovementMeditationMode | Awareness cue rendering, speed indicators |
| FlowStateMode | Dynamic target rendering, flow score HUD |

### 3.4 Movement Analytics Visualization

```
Connect MovementAnalytics → Renderer:
├── Trail rendering
│   ├── Line segments from MovementVisualizer::generateTrailGeometry()
│   ├── Color mapping (velocity, time, zone)
│   └── Fade over time
├── Coverage visualization
│   ├── Voxel grid overlay (optional)
│   └── Coverage sphere rendering
├── Heatmap overlay
│   ├── 3D heatmap from generateHeatmap()
│   └── Transparency-based intensity
└── Ghost playback
    ├── Recorded movement sequences
    └── Semi-transparent controller/hand models
```

### 3.5 USD Training Metadata

Extend USD schemas for training:

```usda
def "TrainingZone_PostureHold" (
    kind = "TrainingExercise"
)
{
    # Exercise type
    string exercise:type = "posture_hold"

    # Parameters
    float exercise:duration = 30.0
    float exercise:toleranceDegrees = 5.0

    # Target pose
    vector3f exercise:targetPosition = (0, 1.2, 0.5)
    quatf exercise:targetOrientation = (1, 0, 0, 0)

    # Scoring
    float exercise:perfectBonus = 1.5
    int exercise:baseXP = 50

    # Difficulty variants
    variantSet "Difficulty" = {
        "Easy" {
            float exercise:toleranceDegrees = 10.0
        }
        "Hard" {
            float exercise:toleranceDegrees = 2.0
        }
    }
}
```

### 3.6 Stage 3 Deliverables

- [ ] PBR materials with normal/roughness maps
- [ ] Bloom effect on saber blade
- [ ] All 6 FlowModes playable with visualization
- [ ] Movement trails render in real-time
- [ ] Coverage percentage displayed
- [ ] Ghost playback in MirrorMode
- [ ] USD training metadata parsed and applied
- [ ] 90 Hz maintained with full effects

---

## Stage 4: Tooling, Modding & Extensibility

**Motto:** "Make it robust, moddable, and future-proof."

### 4.1 Goals

- Non-developers can create content
- USD variants/layers for content management
- Data-driven training definitions
- CLI tools for asset pipeline
- CI/CD pipeline complete
- Overlay mode functional

### 4.2 USD Variants & Layers

```
Layer architecture:
├── base.usd           # Core geometry and materials
├── training.usd       # Training zones, spawn points, paths
├── session.usd        # Runtime: ghost trails, heatmaps
└── user_override.usd  # User customizations

VariantSets:
├── /World/Dojo [Layout]
│   ├── "Beginner"    # Open space, few obstacles
│   ├── "Intermediate" # More structures
│   └── "Advanced"    # Complex environment
├── /World/Saber [Style]
│   ├── "Classic"
│   ├── "Crossguard"
│   └── "Dual"
└── /World/Targets [Pattern]
    ├── "Static"
    ├── "Orbiting"
    └── "Random"
```

### 4.3 Data-Driven Training

Training definition schema (JSON or USD):

```json
{
  "id": "posture_series_01",
  "name": "Basic Stances",
  "exercises": [
    {
      "type": "posture_hold",
      "usdPrim": "/World/Training/Pose_High",
      "duration": 15.0,
      "tolerance": 8.0
    },
    {
      "type": "path_follow",
      "usdPrim": "/World/Training/Path_Circle",
      "speed": 0.5,
      "repetitions": 3
    }
  ],
  "rewards": {
    "completion": 100,
    "perfect": 50
  }
}
```

### 4.4 CLI Tools

```bash
# Content validation
./tools/validate_content --scene dojo.usd --training exercises.json

# Asset conversion
./tools/convert_assets --input model.gltf --output model.usd

# Content packaging
./tools/pack_content --name "Advanced Dojo" --output advanced_dojo.pak

# Performance testing
./tools/benchmark --scene dojo_full.usda --duration 60
```

### 4.5 CI/CD Pipeline

```yaml
# .github/workflows/ci.yml
stages:
  - build:
      - cmake configure + build
      - unit tests
      - USD validation
  - content:
      - validate all .usda scenes
      - validate training definitions
      - asset integrity checks
  - package:
      - create release builds
      - package content packs
  - deploy:
      - upload to release channel
```

### 4.6 Overlay Mode

Complete the overlay system for running atop other VR apps:

```cpp
// OverlaySystem activation
if (XRSession::supportsOverlay()) {
    // Minimal rendering mode
    - Movement trails only
    - Coverage indicator (corner HUD)
    - Uncommon area guides (subtle orbs)

    // Reduced resource usage
    - Lower trail resolution
    - Simplified shaders
    - Shared GPU resources
}
```

### 4.7 Stage 4 Deliverables

- [ ] Content creation without code changes
- [ ] USD variants switch layouts at runtime
- [ ] Training definitions load from JSON/USD
- [ ] CLI tools for validation and conversion
- [ ] CI/CD pipeline runs on all commits
- [ ] Overlay mode works with SteamVR games
- [ ] Documentation for content creators
- [ ] Performance targets met on min-spec hardware

---

## Dependency Additions

### Required for Stage 1

```cmake
# CMakeLists.txt additions

# Vulkan Memory Allocator (header-only)
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.0.1
)

# cgltf (header-only glTF loader)
FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.13
)

# stb_image (header-only image loading)
# Single header from https://github.com/nothings/stb
```

### Optional for Stage 3+

```cmake
# miniaudio (header-only audio)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)

# Dear ImGui (optional debug UI)
# nuklear (alternative minimal UI)
```

---

## File Structure After All Stages

```
MovementDojoXRcpp/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── XRSession.cpp/h         # OpenXR management
│   │   ├── Input.cpp/h             # Input handling
│   │   └── Math.cpp                # Math utilities
│   ├── graphics/                   # NEW - Stage 1
│   │   ├── VulkanContext.cpp/h     # Vulkan initialization
│   │   ├── VulkanXRBinding.cpp/h   # OpenXR integration
│   │   ├── VulkanPipeline.cpp/h    # Render pipelines
│   │   ├── VulkanBuffer.cpp/h      # GPU buffers
│   │   ├── VulkanTexture.cpp/h     # Texture management
│   │   └── Renderer.cpp/h          # High-level rendering
│   ├── assets/                     # NEW - Stage 1
│   │   ├── GLTFLoader.cpp/h        # glTF 2.0 loading
│   │   ├── TextureLoader.cpp/h     # Image loading
│   │   └── AssetManager.cpp/h      # Asset caching
│   ├── usd/
│   │   └── USDLoader.cpp/h         # Existing, enhanced
│   ├── physics/
│   │   └── PhysicsEngine.cpp/h     # Existing
│   ├── haptics/
│   │   └── HapticManager.cpp/h     # Existing
│   ├── audio/                      # NEW - Stage 3
│   │   └── AudioEngine.cpp/h       # Spatial audio
│   ├── analytics/
│   │   └── MovementAnalytics.cpp/h # Existing
│   ├── visualization/
│   │   └── MovementVisualizer.cpp/h # Existing, connected
│   ├── training/
│   │   ├── TrainingModule.cpp/h    # Existing
│   │   └── FlowModes.cpp/h         # Existing
│   ├── progression/
│   │   └── ProgressionSystem.cpp/h # Existing
│   └── overlay/
│       └── OverlaySystem.cpp/h     # Existing, completed
├── shaders/                        # NEW - Stage 1
│   ├── pbr.vert.glsl
│   ├── pbr.frag.glsl
│   ├── trail.vert.glsl
│   ├── trail.frag.glsl
│   └── compile_shaders.sh
├── scenes/
│   ├── stage1_cube.usda            # Existing
│   ├── dojo_basic.usda             # Existing
│   └── dojo_full.usda              # Existing
├── content/                        # NEW - Stage 4
│   ├── training/
│   │   └── exercises.json
│   └── dojos/
│       └── advanced/
├── tools/                          # NEW - Stage 4
│   ├── validate_content.py
│   ├── convert_assets.py
│   └── pack_content.py
├── tests/
│   └── ...                         # Existing
├── CMakeLists.txt
├── README.md
└── ROADMAP.md                      # This file
```

---

## Success Metrics

### Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| Frame rate | 90 Hz stable | SteamVR frame timing |
| Frame time | < 11ms | GPU profiler |
| Load time | < 5s (dojo) | Stopwatch |
| Memory | < 2GB VRAM | GPU-Z |

### Quality Targets

| Metric | Target |
|--------|--------|
| Tracking latency | < 20ms motion-to-photon |
| Haptic response | < 5ms collision-to-feedback |
| Analytics accuracy | Voxel: 5cm resolution |

### Content Targets

| Stage | Environments | Training Modules |
|-------|--------------|------------------|
| 1 | 1 (test cube) | 0 |
| 2 | 1 (basic dojo) | 1 (HitTargets) |
| 3 | 2 (basic + full) | 6+ (all FlowModes) |
| 4 | 4+ (user content) | 10+ (data-driven) |

---

## References

- [OpenXR Specification](https://www.khronos.org/openxr/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [glTF 2.0 Specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)
- [USD Documentation](https://openusd.org/docs/)
- [cgltf Documentation](https://github.com/jkuhlmann/cgltf)
- [Vulkan Memory Allocator](https://gpuopen.com/vulkan-memory-allocator/)
- [SteamVR OpenXR Runtime](https://store.steampowered.com/app/250820/SteamVR/)
