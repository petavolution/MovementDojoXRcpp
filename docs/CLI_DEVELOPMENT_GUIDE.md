# Movement Dojo: Pure CLI Development Guide

A complete guide to developing, building, and testing the Movement Dojo VR application
using only command-line tools on Debian Linux. No GUIs required.

## Philosophy

- **Everything is Code**: Worlds, scenes, training content are generated procedurally
- **Text-Based Assets**: USD (.usda) files are human-readable text
- **Open Source Stack**: Vulkan, OpenXR, Bullet Physics, USD (Apache 2.0)
- **Reproducible Builds**: Scripts generate all content deterministically
- **CI/CD Ready**: Entire pipeline runs in headless mode

## Required Tools (Debian/Ubuntu)

```bash
# Core build tools
sudo apt install build-essential cmake ninja-build git git-lfs

# Graphics & XR
sudo apt install libvulkan-dev vulkan-tools
sudo apt install libopenxr-dev openxr-layer-api-dump

# Physics
sudo apt install libbullet-dev

# Window/Input (for development)
sudo apt install libglfw3-dev libglm-dev

# USD (build from source or use prebuilt)
# See: https://github.com/PixarAnimationStudios/USD

# Optional: Testing
sudo apt install libgtest-dev
```

## Project Structure

```
project/
├── CMakeLists.txt              # Build configuration
├── conanfile.txt               # Package management (optional)
├── scripts/
│   ├── generate_scenes.py      # Procedural USD scene generation
│   ├── generate_exercises.py   # Training exercise data
│   ├── generate_meshes.py      # Procedural mesh generation
│   ├── validate_scenes.sh      # Scene validation
│   └── run_tests.sh            # Test runner
├── src/                        # C++ source code
├── include/                    # Headers
├── tests/                      # Unit/integration tests
├── scenes/                     # Generated .usda files
│   ├── dojo_basic.usda
│   ├── dojo_advanced.usda
│   └── training_guides.usda
├── assets/
│   ├── generated/              # Procedurally generated
│   └── external/               # 3D models, sounds (placeholders)
└── docs/
```

## Build Commands

```bash
# Configure (Release)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Configure (Debug with tests)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build
cmake --build build -j$(nproc)

# Run tests
ctest --test-dir build --output-on-failure

# Install
cmake --install build --prefix /opt/movement-dojo
```

## Scene Generation Pipeline

### 1. Generate USD Scenes (Python)

```bash
# Generate all training dojo variants
python3 scripts/generate_scenes.py --output scenes/

# Generate specific variant
python3 scripts/generate_scenes.py --variant advanced --output scenes/dojo_advanced.usda
```

### 2. Generate Training Exercises

```bash
# Generate exercise definitions (JSON -> embedded in USD)
python3 scripts/generate_exercises.py --output scenes/exercises/

# Generate path-following splines
python3 scripts/generate_exercises.py --type paths --difficulty hard
```

### 3. Validate Scenes

```bash
# Check all .usda files for errors
./scripts/validate_scenes.sh scenes/

# Validate specific scene
usdcat scenes/dojo_basic.usda > /dev/null && echo "Valid"

# Check references
usdresolve scenes/dojo_basic.usda
```

## Procedural Content Generation

### USD Scene Template (Python)

```python
#!/usr/bin/env python3
# scripts/generate_scenes.py

def generate_dojo(variant="basic"):
    """Generate a complete dojo scene in USDA format."""

    usda = '''#usda 1.0
(
    defaultPrim = "World"
    upAxis = "Y"
    metersPerUnit = 1.0
    doc = "Movement Dojo - {variant} variant"
)

def Xform "World" (
    variants = {{
        string dojoLayout = "{variant}"
    }}
    prepend variantSets = "dojoLayout"
)
{{
    # Floor
    def Cube "Floor"
    {{
        double size = 1.0
        double3 xformOp:scale = (10, 0.1, 10)
        double3 xformOp:translate = (0, -0.05, 0)
        uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:scale"]
        color3f[] primvars:displayColor = [(0.2, 0.2, 0.25)]
    }}

    # Training area boundary
    def Xform "TrainingArea"
    {{
{generate_boundary_posts(8, 3.0)}
    }}

    # Lighting
    def DomeLight "AmbientLight"
    {{
        float inputs:intensity = 500
        color3f inputs:color = (0.8, 0.85, 1.0)
    }}

    def DistantLight "KeyLight"
    {{
        float inputs:intensity = 1000
        float3 xformOp:rotateXYZ = (-45, 30, 0)
        uniform token[] xformOpOrder = ["xformOp:rotateXYZ"]
    }}

{generate_variant_content(variant)}
}}
'''
    return usda.format(variant=variant)
```

### Procedural Mesh Generation (C++)

```cpp
// Already in src/core/Math.cpp - Mesh::createSphere, createCube, etc.
// Add more procedural generators:

Mesh Mesh::createCylinder(float radius, float height, int segments) {
    Mesh mesh;
    // ... procedural generation
    return mesh;
}

Mesh Mesh::createTorus(float majorRadius, float minorRadius, int segments) {
    Mesh mesh;
    // ... procedural generation
    return mesh;
}

Mesh Mesh::createArrow(float length, float headSize) {
    Mesh mesh;
    // ... for path visualization
    return mesh;
}
```

## Headless Testing

### Run Without Display

```bash
# Use software rendering for CI
export LIBGL_ALWAYS_SOFTWARE=1
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json

# Run tests
./build/bin/lightsaber_tests

# Validate USD loading
./build/bin/lightsaber_trainer --validate-scenes --headless
```

### Mock XR Runtime

```cpp
// For testing without VR hardware
class MockXRSession : public XRSession {
    // Returns simulated poses, views, etc.
};
```

## Development Workflow (CLI Only)

### Daily Workflow

```bash
# 1. Pull latest
git pull origin main

# 2. Regenerate procedural content
python3 scripts/generate_scenes.py --output scenes/

# 3. Build
cmake --build build -j$(nproc)

# 4. Run tests
ctest --test-dir build

# 5. Run application (if VR hardware available)
./build/bin/lightsaber_trainer --scene scenes/dojo_basic.usda

# 6. Commit changes
git add -A
git commit -m "Description"
git push
```

### Editor Setup (Vim/Neovim)

```vim
" .vimrc additions for C++/USD development
set path+=include/**
set path+=src/**

" USD syntax (add to ~/.vim/syntax/usda.vim)
au BufRead,BufNewFile *.usda set filetype=usda

" Build shortcut
nnoremap <F5> :!cmake --build build -j$(nproc)<CR>

" Run tests
nnoremap <F6> :!ctest --test-dir build --output-on-failure<CR>
```

## CI/CD Pipeline (GitHub Actions)

```yaml
# .github/workflows/build.yml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential cmake ninja-build \
            libvulkan-dev libopenxr-dev \
            libbullet-dev libglfw3-dev libglm-dev

      - name: Generate scenes
        run: python3 scripts/generate_scenes.py --output scenes/

      - name: Configure
        run: cmake -B build -G Ninja -DBUILD_TESTS=ON

      - name: Build
        run: cmake --build build

      - name: Test
        run: ctest --test-dir build --output-on-failure

      - name: Validate USD scenes
        run: |
          for f in scenes/*.usda; do
            echo "Validating $f..."
            ./build/bin/lightsaber_trainer --validate "$f" || exit 1
          done
```

## Placeholder Assets

For 3D models and sounds that will be replaced later:

```bash
# Create placeholder directory structure
mkdir -p assets/external/{models,sounds,textures}

# Placeholder model (just a marker file)
echo "# Placeholder: Replace with actual lightsaber.glb" > assets/external/models/lightsaber.stub

# Placeholder sound
echo "# Placeholder: Replace with saber_hum.ogg" > assets/external/sounds/saber_hum.stub
```

The code uses procedural geometry until real assets are added:

```cpp
// In SessionManager or asset loading:
Mesh getSaberMesh() {
    // Check for external asset first
    if (fileExists("assets/external/models/lightsaber.glb")) {
        return loadGLTF("assets/external/models/lightsaber.glb");
    }
    // Fall back to procedural
    return Mesh::createCylinder(0.02f, 1.0f, 16);
}
```

## Summary

This workflow enables:
- Full development using only `vim`, `cmake`, `python`, and shell scripts- All content generated programmatically
- Reproducible builds in CI/CD
- Testing without VR hardware
- Gradual replacement of stubs with real assets
