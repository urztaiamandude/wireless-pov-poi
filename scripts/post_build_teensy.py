"""
Post-build script for Teensy 4.1 firmware
Generates HEX file for use with Teensy Loader application
"""
from typing import Any

try:
    from SCons.Script import Import  # type: ignore[reportMissingImports]
except Exception:  # pragma: no cover - SCons available at build time
    def Import(name: str) -> Any:
        raise ImportError("SCons is required to run this script.")

Import("env")

env = globals().get("env")
if env is None:
    raise RuntimeError("SCons environment 'env' not available.")
import shutil
import os

def post_build_action(source, target, env):
    """Copy the HEX file to a convenient location after build"""
    
    # Get the firmware name from the build
    firmware_source = target[0].get_abspath()
    firmware_dir = os.path.dirname(firmware_source)
    
    # The .hex file should be generated alongside the .elf file
    hex_source = firmware_source.replace(".elf", ".hex")
    
    # Create output directory in project root
    project_dir = env.get("PROJECT_DIR")
    output_dir = os.path.join(project_dir, "build_output")
    os.makedirs(output_dir, exist_ok=True)
    
    # Copy HEX file to output directory with a clear name
    hex_destination = os.path.join(output_dir, "teensy41_firmware.hex")
    
    if os.path.exists(hex_source):
        shutil.copy(hex_source, hex_destination)
        print("=" * 60)
        print("✓ HEX file generated successfully!")
        print(f"  Location: {hex_destination}")
        print("=" * 60)
        print("To load with Teensy Loader:")
        print("  1. Open Teensy Loader application")
        print("  2. File > Open HEX File")
        print(f"  3. Select: {hex_destination}")
        print("  4. Press the button on your Teensy board")
        print("  5. Click 'Program' in Teensy Loader")
        print("=" * 60)
    else:
        print(f"Warning: HEX file not found at {hex_source}")

# Register the post-build action
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", post_build_action)
