# USD-Based OpenXR Lightsaber Proprioception Trainer

A high-fidelity, single-player OpenXR/SteamVR application where users engage in meditative lightsaber-based proprioception enhancement training within immersive environments defined by `.usda` scene files. The experience aims for the quality and interaction depth seen in simulations like the 'Vader Dojo' training room from 'Vader Immortal'.

## Core Features

- **OpenXR First**: Cross-platform VR support via OpenXR standard
- **USD Scene Definition**: Data-driven environments using Pixar's Universal Scene Description
- **PCVR Streaming**: Optimized for Quest 3/Pico 4 via ALVR or Virtual Desktop
- **Proprioception Training**: Meditative lightsaber exercises with haptic feedback
- **CLI-First Development**: Pure code workflow with no GUI dependencies

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │   Training   │  │    UI/UX     │  │   User Profile/      │  │
│  │   Modules    │  │   Manager    │  │   Progress           │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
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
│   │   └── Physics.cpp/h    # Bullet physics integration
│   ├── usd/                 # USD integration
│   │   └── USDLoader.cpp/h  # USD scene loading
│   ├── training/            # Training modules
│   │   └── TrainingModule.cpp/h
│   └── haptics/             # Haptic feedback
│       └── HapticManager.cpp/h
├── include/                 # Public headers
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
│   └── placeholder/         # Placeholder assets
├── tests/                   # Unit and integration tests
│   ├── CMakeLists.txt
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

## Training Modules

Training exercises are defined via custom USD schemas:

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
