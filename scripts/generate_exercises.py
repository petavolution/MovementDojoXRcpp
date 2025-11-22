#!/usr/bin/env python3
"""
Procedural Exercise Generator for Movement Dojo

Generates exercise definitions in JSON format that can be loaded at runtime.
All exercises are defined in code - no GUI authoring needed.

Usage:
    python3 generate_exercises.py --output exercises/
    python3 generate_exercises.py --difficulty beginner --output exercises/beginner.json
"""

import argparse
import json
import math
import os
from datetime import datetime
from typing import List, Dict, Any


# =============================================================================
# Exercise Building Blocks
# =============================================================================

def generate_circle_path(radius: float, height: float, segments: int = 16) -> List[Dict[str, float]]:
    """Generate points along a circular path."""
    points = []
    for i in range(segments):
        angle = (2 * math.pi * i) / segments
        points.append({
            "x": radius * math.cos(angle),
            "y": height,
            "z": radius * math.sin(angle),
            "timestamp": i * 0.5  # 0.5 seconds between points
        })
    return points


def generate_figure8_path(width: float, height: float, segments: int = 32) -> List[Dict[str, float]]:
    """Generate a figure-8 (lemniscate) path."""
    points = []
    for i in range(segments):
        t = (2 * math.pi * i) / segments
        x = width * math.sin(t)
        y = height
        z = width * math.sin(t) * math.cos(t)
        points.append({
            "x": x,
            "y": y + 0.2 * math.sin(t * 2),
            "z": z - 1.0,  # Offset in front of player
            "timestamp": i * 0.3
        })
    return points


def generate_vertical_slice_path(width: float, height: float, segments: int = 8) -> List[Dict[str, float]]:
    """Generate a vertical slicing path."""
    points = []
    for i in range(segments):
        t = i / (segments - 1)
        points.append({
            "x": width * (t - 0.5) * 2,
            "y": height + 0.5 * math.sin(t * math.pi),
            "z": -1.2,
            "timestamp": i * 0.15
        })
    return points


def generate_spiral_path(radius_start: float, radius_end: float, height_start: float,
                         height_end: float, turns: float = 2, segments: int = 32) -> List[Dict[str, float]]:
    """Generate an ascending/descending spiral path."""
    points = []
    for i in range(segments):
        t = i / (segments - 1)
        angle = turns * 2 * math.pi * t
        radius = radius_start + (radius_end - radius_start) * t
        height = height_start + (height_end - height_start) * t
        points.append({
            "x": radius * math.cos(angle),
            "y": height,
            "z": radius * math.sin(angle) - 1.0,
            "timestamp": i * 0.25
        })
    return points


def generate_random_targets(count: int, bounds: Dict[str, tuple], seed: int = 42) -> List[Dict[str, float]]:
    """Generate pseudo-random target positions within bounds (deterministic for reproducibility)."""
    import random
    rng = random.Random(seed)

    targets = []
    for i in range(count):
        targets.append({
            "x": rng.uniform(bounds["x"][0], bounds["x"][1]),
            "y": rng.uniform(bounds["y"][0], bounds["y"][1]),
            "z": rng.uniform(bounds["z"][0], bounds["z"][1]),
            "radius": 0.08,
            "id": i
        })
    return targets


# =============================================================================
# Exercise Definitions
# =============================================================================

def create_beginner_exercises() -> List[Dict[str, Any]]:
    """Create beginner-level exercises focused on basic movements."""
    exercises = []

    # Exercise 1: Simple horizontal circle
    exercises.append({
        "id": "beginner_circle_1",
        "name": "Basic Circle",
        "description": "Trace a simple circle at chest height",
        "difficulty": "beginner",
        "duration_seconds": 30,
        "hand": "dominant",
        "type": "path_follow",
        "path": generate_circle_path(radius=0.4, height=1.2, segments=16),
        "scoring": {
            "accuracy_weight": 0.7,
            "smoothness_weight": 0.3,
            "time_bonus": False
        },
        "feedback": {
            "on_complete": "Great job! You traced a basic circle.",
            "on_perfect": "Perfect circle! Excellent control."
        }
    })

    # Exercise 2: Vertical figure-8
    exercises.append({
        "id": "beginner_figure8_1",
        "name": "Gentle Figure-8",
        "description": "Follow a slow figure-8 pattern",
        "difficulty": "beginner",
        "duration_seconds": 45,
        "hand": "dominant",
        "type": "path_follow",
        "path": generate_figure8_path(width=0.3, height=1.2, segments=24),
        "scoring": {
            "accuracy_weight": 0.6,
            "smoothness_weight": 0.4,
            "time_bonus": False
        }
    })

    # Exercise 3: Static target touch
    exercises.append({
        "id": "beginner_targets_1",
        "name": "Target Touch",
        "description": "Touch each glowing target in sequence",
        "difficulty": "beginner",
        "duration_seconds": 60,
        "hand": "dominant",
        "type": "target_sequence",
        "targets": generate_random_targets(8, {
            "x": (-0.6, 0.6),
            "y": (0.8, 1.6),
            "z": (-1.5, -0.8)
        }, seed=1),
        "sequence": "ordered",
        "scoring": {
            "accuracy_weight": 1.0,
            "time_bonus": True,
            "max_time_per_target": 5.0
        }
    })

    return exercises


def create_intermediate_exercises() -> List[Dict[str, Any]]:
    """Create intermediate exercises with more complex patterns."""
    exercises = []

    # Exercise 1: Dual-hand circles
    exercises.append({
        "id": "intermediate_dual_circles",
        "name": "Mirror Circles",
        "description": "Trace circles with both hands simultaneously",
        "difficulty": "intermediate",
        "duration_seconds": 45,
        "hand": "both",
        "type": "dual_path_follow",
        "paths": {
            "left": generate_circle_path(radius=0.35, height=1.2, segments=16),
            "right": generate_circle_path(radius=0.35, height=1.2, segments=16)
        },
        "mirror_mode": "symmetric",
        "scoring": {
            "accuracy_weight": 0.5,
            "synchronization_weight": 0.3,
            "smoothness_weight": 0.2
        }
    })

    # Exercise 2: Ascending spiral
    exercises.append({
        "id": "intermediate_spiral_1",
        "name": "Ascending Spiral",
        "description": "Follow a spiral path from low to high",
        "difficulty": "intermediate",
        "duration_seconds": 40,
        "hand": "dominant",
        "type": "path_follow",
        "path": generate_spiral_path(
            radius_start=0.5, radius_end=0.2,
            height_start=0.6, height_end=1.8,
            turns=2, segments=32
        ),
        "scoring": {
            "accuracy_weight": 0.6,
            "smoothness_weight": 0.3,
            "verticality_bonus": 0.1
        }
    })

    # Exercise 3: Speed targets
    exercises.append({
        "id": "intermediate_speed_targets",
        "name": "Quick Strike",
        "description": "Hit targets as quickly as possible",
        "difficulty": "intermediate",
        "duration_seconds": 45,
        "hand": "dominant",
        "type": "target_sequence",
        "targets": generate_random_targets(12, {
            "x": (-0.8, 0.8),
            "y": (0.6, 1.8),
            "z": (-2.0, -0.6)
        }, seed=42),
        "sequence": "random",
        "scoring": {
            "accuracy_weight": 0.4,
            "speed_weight": 0.6,
            "time_bonus": True,
            "max_time_per_target": 2.0
        }
    })

    return exercises


def create_advanced_exercises() -> List[Dict[str, Any]]:
    """Create advanced exercises requiring high precision and coordination."""
    exercises = []

    # Exercise 1: Complex dual-hand figure-8
    exercises.append({
        "id": "advanced_dual_figure8",
        "name": "Interleaved Figure-8",
        "description": "Trace offset figure-8 patterns with both hands",
        "difficulty": "advanced",
        "duration_seconds": 60,
        "hand": "both",
        "type": "dual_path_follow",
        "paths": {
            "left": generate_figure8_path(width=0.4, height=1.0, segments=32),
            "right": generate_figure8_path(width=0.4, height=1.4, segments=32)
        },
        "phase_offset": 0.5,  # Right hand is 50% behind left
        "scoring": {
            "accuracy_weight": 0.4,
            "synchronization_weight": 0.4,
            "smoothness_weight": 0.2
        }
    })

    # Exercise 2: Precision cutting
    exercises.append({
        "id": "advanced_precision_cut",
        "name": "Precision Cuts",
        "description": "Make precise cuts through narrow gates",
        "difficulty": "advanced",
        "duration_seconds": 90,
        "hand": "dominant",
        "type": "gate_sequence",
        "gates": [
            {"center": {"x": 0.0, "y": 1.2, "z": -1.2}, "normal": {"x": 1, "y": 0, "z": 0}, "width": 0.15},
            {"center": {"x": 0.3, "y": 1.0, "z": -1.4}, "normal": {"x": 0, "y": 1, "z": 0}, "width": 0.12},
            {"center": {"x": -0.2, "y": 1.4, "z": -1.0}, "normal": {"x": 0.7, "y": 0.7, "z": 0}, "width": 0.10},
            {"center": {"x": 0.0, "y": 0.8, "z": -1.3}, "normal": {"x": 0, "y": 0, "z": 1}, "width": 0.15},
            {"center": {"x": 0.4, "y": 1.3, "z": -1.5}, "normal": {"x": 0.5, "y": 0.5, "z": 0.7}, "width": 0.08},
        ],
        "min_velocity": 2.0,  # Minimum saber velocity to count as a cut
        "scoring": {
            "accuracy_weight": 0.6,
            "velocity_weight": 0.3,
            "angle_accuracy_weight": 0.1
        }
    })

    # Exercise 3: Endurance flow
    exercises.append({
        "id": "advanced_endurance_flow",
        "name": "Endurance Flow",
        "description": "Maintain smooth, continuous movement for extended duration",
        "difficulty": "advanced",
        "duration_seconds": 180,
        "hand": "both",
        "type": "flow_mode",
        "flow_config": {
            "min_velocity": 0.3,
            "max_pause_duration": 1.0,
            "coverage_zones": [
                {"center": {"x": -0.5, "y": 1.0, "z": -1.0}, "radius": 0.3},
                {"center": {"x": 0.5, "y": 1.0, "z": -1.0}, "radius": 0.3},
                {"center": {"x": 0.0, "y": 1.5, "z": -1.0}, "radius": 0.3},
                {"center": {"x": 0.0, "y": 0.7, "z": -1.2}, "radius": 0.3},
            ]
        },
        "scoring": {
            "continuity_weight": 0.4,
            "coverage_weight": 0.4,
            "smoothness_weight": 0.2
        }
    })

    return exercises


def create_master_exercises() -> List[Dict[str, Any]]:
    """Create master-level exercises for experts."""
    exercises = []

    # Exercise 1: Full body integration
    exercises.append({
        "id": "master_full_body",
        "name": "Full Body Integration",
        "description": "Coordinate hands with head movement and spatial awareness",
        "difficulty": "master",
        "duration_seconds": 120,
        "hand": "both",
        "type": "full_body_flow",
        "requirements": {
            "hand_coordination": True,
            "head_tracking": True,
            "spatial_coverage": 0.8
        },
        "zones": generate_random_targets(20, {
            "x": (-1.2, 1.2),
            "y": (0.4, 2.0),
            "z": (-2.0, 0.5)
        }, seed=999),
        "scoring": {
            "coverage_weight": 0.3,
            "coordination_weight": 0.3,
            "flow_weight": 0.2,
            "exploration_weight": 0.2
        }
    })

    # Exercise 2: Blindfolded proprioception
    exercises.append({
        "id": "master_proprioception",
        "name": "Pure Proprioception",
        "description": "Complete targets with minimal visual feedback",
        "difficulty": "master",
        "duration_seconds": 90,
        "hand": "dominant",
        "type": "proprioception_test",
        "visual_fade": {
            "start_opacity": 1.0,
            "end_opacity": 0.1,
            "fade_duration": 30.0
        },
        "targets": generate_random_targets(15, {
            "x": (-0.6, 0.6),
            "y": (0.8, 1.5),
            "z": (-1.4, -0.8)
        }, seed=777),
        "haptic_feedback": True,
        "audio_feedback": True,
        "scoring": {
            "accuracy_weight": 0.8,
            "confidence_weight": 0.2
        }
    })

    return exercises


# =============================================================================
# Output Functions
# =============================================================================

def generate_exercise_catalog() -> Dict[str, Any]:
    """Generate complete exercise catalog with all difficulties."""

    catalog = {
        "metadata": {
            "generator": "generate_exercises.py",
            "version": "1.0.0",
            "generated": datetime.now().isoformat(),
            "description": "Movement Dojo Exercise Catalog - Procedurally Generated"
        },
        "difficulties": ["beginner", "intermediate", "advanced", "master"],
        "exercises": {
            "beginner": create_beginner_exercises(),
            "intermediate": create_intermediate_exercises(),
            "advanced": create_advanced_exercises(),
            "master": create_master_exercises()
        },
        "progression": {
            "unlock_requirements": {
                "intermediate": {"beginner_completed": 3, "min_average_score": 70},
                "advanced": {"intermediate_completed": 3, "min_average_score": 75},
                "master": {"advanced_completed": 3, "min_average_score": 80}
            }
        }
    }

    # Add total counts
    total = sum(len(ex) for ex in catalog["exercises"].values())
    catalog["metadata"]["total_exercises"] = total

    return catalog


def main():
    parser = argparse.ArgumentParser(
        description="Generate exercise definitions for Movement Dojo"
    )
    parser.add_argument(
        "--output", "-o",
        default="exercises/",
        help="Output directory or file path"
    )
    parser.add_argument(
        "--difficulty", "-d",
        choices=["beginner", "intermediate", "advanced", "master", "all"],
        default="all",
        help="Difficulty level to generate"
    )
    parser.add_argument(
        "--format", "-f",
        choices=["json", "pretty"],
        default="pretty",
        help="Output format (json=compact, pretty=indented)"
    )

    args = parser.parse_args()

    # Generate catalog
    catalog = generate_exercise_catalog()

    # Prepare output
    if args.output.endswith('.json'):
        output_path = args.output
        os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    else:
        os.makedirs(args.output, exist_ok=True)
        output_path = os.path.join(args.output, "exercise_catalog.json")

    # Filter by difficulty if needed
    if args.difficulty != "all":
        filtered_catalog = {
            "metadata": catalog["metadata"],
            "difficulties": [args.difficulty],
            "exercises": {args.difficulty: catalog["exercises"][args.difficulty]}
        }
        catalog = filtered_catalog

    # Write output
    indent = 2 if args.format == "pretty" else None
    with open(output_path, "w") as f:
        json.dump(catalog, f, indent=indent)

    print(f"Generated: {output_path}")
    print(f"Total exercises: {sum(len(ex) for ex in catalog['exercises'].values())}")


if __name__ == "__main__":
    main()
