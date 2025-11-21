# Movement Dojo: XR Proprioception & Movement Exploration

A VR application for **movement exploration and body awareness training** - a serious game that tracks, visualizes, and encourages full-body movement. Inspired by yoga, qi gong, meditation practices, and the 'Vader Dojo' from Vader Immortal.

## Vision

**Record EVERYTHING. Visualize EVERYTHING. Explore movements you NEVER make in daily life.**

Most people use only a fraction of their body's movement potential. This application:
- **Records all movement data** (hands, head, full tracking)
- **Visualizes movement patterns** (trails, heatmaps, coverage maps)
- **Tracks "movement space coverage"** - what % of your reachable space have you explored?
- **Guides you to uncommon positions** - movements you rarely or never make
- **Gamifies exploration** with achievements, levels, and challenges

Can run standalone or as an **OpenXR overlay** on top of other VR apps!

## Core Features

- **Movement Analytics**: Complete recording and analysis of all movement data
- **Movement Space Coverage**: Voxelized tracking of explored vs unexplored areas
- **OpenXR Overlay Mode**: Track movements while playing any VR game
- **Flow/Meditation Modes**: Yoga, qi gong, breathing-synchronized exercises
- **Serious Game Progression**: XP, levels, achievements, daily/weekly challenges
- **USD Scene Definition**: Data-driven environments using Pixar's Universal Scene Description
- **PCVR Streaming**: Optimized for Quest 3/Pico 4 via ALVR or Virtual Desktop
- **CLI-First Development**: Pure code workflow with no GUI dependencies

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │   Training   │  │  Progression │  │   Overlay System     │  │
│  │   Modes      │  │   System     │  │   (XR_EXTX_overlay)  │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐                            │
│  │   Flow/      │  │  Movement    │                            │
│  │   Meditation │  │  Analytics   │                            │
│  └──────────────┘  └──────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                       Core Systems                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  XR Session  │  │   Rendering  │  │   Physics Engine     │  │
│  │  Manager     │  │   Engine     │  │   (Bullet)           │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │    Input &   │  │    USD       │  │   Haptic Feedback    │  │
│  │ Action Mgr   │  │   Loader     │  │   Manager            │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                     Platform Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │   OpenXR     │  │   Vulkan/    │  │   SteamVR/Oculus     │  │
│  │   Loader     │  │   OpenGL     │  │   Runtime            │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Project Structure

```
project4/
├── README.md                 # This file
├── CMakeLists.txt           # Main CMake build configuration
├── conanfile.txt            # Conan dependency specification
├── src/                     # C++ source code
│   ├── main.cpp             # Application entry point
│   ├── core/                # Core systems
│   │   ├── XRSession.cpp/h  # OpenXR session management
│   │   ├── Renderer.cpp/h   # Vulkan/OpenGL rendering
│   │   ├── Input.cpp/h      # Input and action management
│   │   └── Math.cpp         # Math utilities
│   ├── usd/                 # USD integration
│   │   └── USDLoader.cpp/h  # USD scene loading
│   ├── analytics/           # Movement analytics (NEW)
│   │   └── MovementAnalytics.cpp/h  # Full movement recording & analysis
│   ├── training/            # Training modules
│   │   ├── TrainingModule.cpp/h
│   │   └── FlowModes.cpp/h  # Meditation, qi gong, yoga modes
│   ├── progression/         # Serious game progression (NEW)
│   │   └── ProgressionSystem.cpp/h  # XP, levels, achievements
│   ├── overlay/             # OpenXR overlay mode (NEW)
│   │   └── OverlaySystem.cpp/h  # Overlay on other VR apps
│   ├── haptics/             # Haptic feedback
│   │   └── HapticManager.cpp/h
│   └── physics/             # Physics
│       └── PhysicsEngine.cpp/h
├── include/                 # Public headers
│   └── Types.h              # Core types (Vec3, Quat, Transform, etc.)
├── scripts/                 # Python and shell scripts
│   ├── generate_scene.py    # USD scene generation
│   ├── validate_usd.py      # USD validation
│   ├── build.sh             # Build script
│   └── run.sh               # Run script
├── scenes/                  # USD scene files
│   ├── stage1_cube.usda     # Stage 1: Basic cube
│   ├── dojo_basic.usda      # Stage 2: Basic dojo
│   └── dojo_full.usda       # Stage 3+: Full dojo
├── assets/                  # 3D models, textures, sounds
├── tests/                   # Unit and integration tests
│   ├── CMakeLists.txt
│   ├── test_math.cpp
│   └── test_usd_loader.cpp
├── docker/                  # Docker configuration
│   └── Dockerfile
└── .github/                 # CI/CD configuration
    └── workflows/
        └── ci.yml
```

## Development Stages

### Stage 1: Foundation - "See the Controller, Load a Cube"
- Basic OpenXR session initialization
- HMD and controller tracking
- Simple scene rendering (ground plane, skybox)
- Controller visualization (placeholder cubes)
- Minimal USD scene loading (single cube)

### Stage 2: Core Interaction - "Swing the Saber, Hit the Target"
- Full USD scene loading (dojo environment)
- Lightsaber model attachment and activation
- Basic physics integration (Bullet)
- Collision detection and response
- Basic haptic feedback

### Stage 3: Fidelity & Proprioception - "Feel the Force"
- High-fidelity PBR rendering
- Advanced physics and collision
- Proprioception training modules
- Nuanced haptic patterns
- Performance optimization

### Stage 4: Polish & Content - "Master the Dojo"
- USD variants and layers
- Multiple training exercises
- Full UI/UX implementation
- CI/CD pipeline
- Content extensibility

## Prerequisites

### System Requirements
- Linux (Debian/Ubuntu recommended) or Windows
- NVIDIA GPU with Vulkan support
- VR headset (Quest 3, Pico 4, etc.) with ALVR or Virtual Desktop

### Dependencies
```bash
# Debian/Ubuntu
sudo apt update && sudo apt install -y \
  build-essential cmake ninja-build pkg-config git \
  libglfw3-dev libx11-dev libxrandr-dev libxinerama-dev \
  libopenxr-dev libvulkan-dev libglm-dev \
  python3-pip python3-venv

# Python dependencies
pip3 install conan usd-core
```

## Building

### Quick Start
```bash
# Clone and enter project
cd project4

# Install dependencies via Conan
conan install . --output-folder=build --build=missing

# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Validate USD scenes
python3 scripts/validate_usd.py scenes/

# Run the application
./build/lightsaber_trainer --scene scenes/stage1_cube.usda
```

### Using Build Script
```bash
./scripts/build.sh        # Full build
./scripts/build.sh debug  # Debug build
./scripts/build.sh clean  # Clean build
```

## Running

### Basic Execution
```bash
# With SteamVR
./build/lightsaber_trainer --scene scenes/dojo_basic.usda

# Headless validation mode
./build/lightsaber_trainer --scene scenes/dojo_basic.usda --validate-only

# With specific runtime
XR_RUNTIME_JSON=/path/to/runtime.json ./build/lightsaber_trainer
```

### PCVR Streaming Setup

#### ALVR
1. Install ALVR server on PC
2. Install ALVR client on Quest 3/Pico 4
3. Configure: H.264/HEVC, ~45 Mbps, 5 GHz WiFi
4. Run the application with SteamVR

#### Virtual Desktop
1. Install Virtual Desktop Streamer on PC
2. Install Virtual Desktop on headset
3. Set OpenXR runtime to VDXR
4. Run the application

## USD Scene Format

Scenes are authored in USD ASCII format (`.usda`):

```usda
#usda 1.0
(
    defaultPrim = "World"
    upAxis = "Y"
)

def Xform "World"
{
    def Xform "Dojo"
    {
        def Mesh "Floor" {
            # Floor geometry
        }

        def Xform "TrainingArea" {
            # Training equipment
        }
    }

    def "Lights" {
        def DistantLight "Sun" {
            float inputs:intensity = 1.0
        }
    }
}
```

## Training Modes

### Flow & Meditation Modes

The application includes multiple mindful movement modes:

| Mode | Description |
|------|-------------|
| **Free Exploration** | Freely explore movement space with visual coverage feedback |
| **Guided Stretch** | Follow guided yoga/stretch sequences with target positions |
| **Breathing Sync** | Synchronize movement with breathing rhythm (inhale/exhale) |
| **Mirror Mode** | Follow a ghost guide through recorded or procedural sequences |
| **Movement Meditation** | Slow, mindful movement with awareness cues |
| **Flow State** | Encourage continuous movement flow with dynamic targets |

### Movement Space Zones

The application tracks exploration of different movement zones:

- **Overhead**: Above your head (uncommon in daily life)
- **Behind High/Low**: Behind your back (rarely used)
- **Floor Front/Side**: Near floor positions
- **Side High/Mid**: Reaching to the sides
- **Front High/Mid/Low**: Standard frontal positions

### Training Exercises (USD-defined)

Training exercises can also be defined via custom USD schemas:

```usda
def "Exercise_HoldSteady" (
    kind = "TrainingExercise"
)
{
    string exercise:type = "posture_hold"
    float exercise:duration = 30.0
    vector3f exercise:targetPosition = (0, 1.2, 0.5)
    quatf exercise:targetOrientation = (1, 0, 0, 0)
    float exercise:toleranceDegrees = 5.0
}
```

## Movement Analytics

The MovementAnalytics system records ALL movement data:

```cpp
struct MovementSample {
    double timestamp;
    Vec3 headPosition, headVelocity;
    Vec3 leftHandPosition, rightHandPosition;
    Vec3 leftHandRelative, rightHandRelative;  // Head-relative
    Quat leftHandOrientation, rightHandOrientation;
    Vec3 leftHandVelocity, rightHandVelocity;
    float leftGripStrength, rightGripStrength;
};
```

### Key Metrics

- **Movement Space Coverage**: 0-100% of reachable space explored
- **Uncommon Position Time**: Time spent in rarely-used positions
- **Movement Symmetry**: Left/right balance analysis
- **Flow Score**: Continuous movement quality (0-1)
- **Velocity Patterns**: Speed and acceleration analysis

## Progression System

### Level System

| Level | Title | XP Required | Unlocks |
|-------|-------|-------------|---------|
| 1 | Newcomer | 0 | - |
| 2 | Explorer | 100 | Free Exploration mode |
| 3 | Seeker | 350 | Guided Stretch, trail colors |
| 4 | Practitioner | 850 | Breathing Sync, heatmap view |
| 5 | Adept | 1600 | Mirror Mode, ghost playback |
| 6 | Journeyman | 2600 | Meditation Mode, custom sequences |
| 7 | Expert | 4100 | Flow State, movement analysis |
| 8 | Master | 6100 | Advanced stats |
| 9 | Grand Master | 9100 | Mentor mode |
| 10 | Movement Sage | 14100 | All features |

### Achievements

- **Exploration**: Coverage milestones (10%, 25%, 50%, 75%)
- **Discovery**: Uncommon position discoveries
- **Flow**: Flow state duration records
- **Consistency**: Daily/weekly streak achievements
- **Mastery**: Body symmetry and movement variety

### Daily/Weekly Challenges

The system generates dynamic challenges:
- "Increase coverage by 5%"
- "Discover 3 uncommon positions"
- "Maintain flow state for 30 seconds"

## Overlay Mode

Run as an OpenXR overlay on top of ANY VR game:

```bash
./lightsaber_trainer --overlay --minimal
```

### Overlay Elements

- **Movement Trails**: Colorized hand movement visualization
- **Coverage Indicator**: Current exploration percentage
- **Uncommon Area Guides**: Orbs highlighting unexplored areas
- **Stats HUD**: Session timer, coverage, discoveries
- **Breathing Guide**: Visual breathing rhythm indicator

### Overlay Presets

- `minimal`: Just trails, low opacity
- `standard`: Trails + stats HUD
- `exploration`: Full coverage visualization
- `meditation`: Trails + breathing guide
- `full`: All elements enabled

## Testing

```bash
# Run all tests
cd build && ctest --output-on-failure

# Run specific test
./build/tests/test_usd_loader

# USD validation
python3 scripts/validate_usd.py scenes/
```

## License

MIT License - See LICENSE file for details.

## References

- [OpenXR Specification](https://www.khronos.org/openxr/)
- [USD Documentation](https://openusd.org/docs/)
- [Bullet Physics](https://pybullet.org/)
- [ALVR](https://github.com/alvr-org/ALVR)
- [Virtual Desktop](https://www.vrdesktop.net/)
