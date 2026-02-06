#!/usr/bin/env python3
"""
Test script for BLE Protocol Command Translation

This script tests the BLE protocol command translation logic without requiring
physical hardware. It verifies that BLE commands are correctly formatted and
translated to internal protocol commands.
"""

# BLE Protocol markers
BLE_CMD_START = 0xD0
BLE_CMD_END = 0xD1

# Internal Protocol markers
INTERNAL_CMD_START = 0xFF
INTERNAL_CMD_END = 0xFE

# Command codes
CC_SET_BRIGHTNESS = 0x02
CC_SET_SPEED = 0x03
CC_SET_PATTERN = 0x04
CC_SET_PATTERN_SLOT = 0x05
CC_SET_PATTERN_ALL = 0x06
CC_SET_SEQUENCER = 0x0E
CC_START_SEQUENCER = 0x0F

# Internal command codes
INTERNAL_SET_MODE = 0x01
INTERNAL_UPLOAD_PATTERN = 0x03
INTERNAL_UPLOAD_SEQUENCE = 0x04
INTERNAL_SET_BRIGHTNESS = 0x06
INTERNAL_SET_FRAMERATE = 0x07


def format_ble_command(command_code, data=None):
    """
    Format a BLE command with proper markers.
    
    Args:
        command_code: Command code (int)
        data: Optional data bytes (list of int)
    
    Returns:
        list of int: Formatted BLE command
    """
    cmd = [BLE_CMD_START, command_code]
    if data:
        cmd.extend(data)
    cmd.append(BLE_CMD_END)
    return cmd


def translate_ble_to_internal(ble_cmd):
    """
    Translate BLE command to internal protocol.
    Mimics the ESP32 BLE bridge translation logic.
    
    Args:
        ble_cmd: BLE command bytes (list of int)
    
    Returns:
        list of int: Internal protocol command
    """
    if len(ble_cmd) < 3:
        return None
    
    if ble_cmd[0] != BLE_CMD_START or ble_cmd[-1] != BLE_CMD_END:
        return None
    
    ble_code = ble_cmd[1]
    data = ble_cmd[2:-1] if len(ble_cmd) > 3 else []
    
    # Command mapping
    if ble_code == CC_SET_BRIGHTNESS:
        # Map to internal brightness command
        internal_code = INTERNAL_SET_BRIGHTNESS
        internal_cmd = [INTERNAL_CMD_START, internal_code, len(data)] + data + [INTERNAL_CMD_END]
        
    elif ble_code == CC_SET_SPEED:
        # Map to internal frame rate command
        internal_code = INTERNAL_SET_FRAMERATE
        internal_cmd = [INTERNAL_CMD_START, internal_code, len(data)] + data + [INTERNAL_CMD_END]
        
    elif ble_code == CC_SET_PATTERN:
        # Map to internal pattern upload
        internal_code = INTERNAL_UPLOAD_PATTERN
        internal_cmd = [INTERNAL_CMD_START, internal_code, len(data)] + data + [INTERNAL_CMD_END]
        
    elif ble_code == CC_SET_PATTERN_SLOT:
        # Map to set mode (pattern mode with slot)
        slot = data[0] if data else 0
        internal_cmd = [INTERNAL_CMD_START, INTERNAL_SET_MODE, 2, 0x02, slot, INTERNAL_CMD_END]
        
    elif ble_code == CC_SET_PATTERN_ALL:
        # Map to set mode (auto-cycle all patterns)
        internal_cmd = [INTERNAL_CMD_START, INTERNAL_SET_MODE, 2, 0x02, 0xFF, INTERNAL_CMD_END]
        
    elif ble_code == CC_SET_SEQUENCER:
        # Map to internal sequence upload
        internal_code = INTERNAL_UPLOAD_SEQUENCE
        internal_cmd = [INTERNAL_CMD_START, internal_code, len(data)] + data + [INTERNAL_CMD_END]
        
    elif ble_code == CC_START_SEQUENCER:
        # Map to set mode (sequence mode)
        seq_idx = data[0] if data else 0
        internal_cmd = [INTERNAL_CMD_START, INTERNAL_SET_MODE, 2, 0x03, seq_idx, INTERNAL_CMD_END]
        
    else:
        # Unknown command
        return None
    
    return internal_cmd


def print_command(label, cmd):
    """Print a command in hex format."""
    hex_str = ' '.join([f'{b:02X}' for b in cmd])
    print(f"{label:30s}: {hex_str}")


def test_brightness_command():
    """Test brightness command (0-255)."""
    print("\n=== Test: Set Brightness to 128 (50%) ===")
    ble_cmd = format_ble_command(CC_SET_BRIGHTNESS, [128])
    print_command("BLE Command", ble_cmd)
    
    internal_cmd = translate_ble_to_internal(ble_cmd)
    print_command("Internal Command", internal_cmd)
    
    # Verify
    assert internal_cmd[0] == INTERNAL_CMD_START
    assert internal_cmd[1] == INTERNAL_SET_BRIGHTNESS
    assert internal_cmd[3] == 128
    assert internal_cmd[-1] == INTERNAL_CMD_END
    print("✓ Test passed")


def test_speed_command():
    """Test speed/frame rate command."""
    print("\n=== Test: Set Speed to 20ms (50 FPS) ===")
    ble_cmd = format_ble_command(CC_SET_SPEED, [20])
    print_command("BLE Command", ble_cmd)
    
    internal_cmd = translate_ble_to_internal(ble_cmd)
    print_command("Internal Command", internal_cmd)
    
    # Verify
    assert internal_cmd[0] == INTERNAL_CMD_START
    assert internal_cmd[1] == INTERNAL_SET_FRAMERATE
    assert internal_cmd[3] == 20
    assert internal_cmd[-1] == INTERNAL_CMD_END
    print("✓ Test passed")


def test_pattern_slot_command():
    """Test pattern slot selection."""
    print("\n=== Test: Select Pattern Slot 3 ===")
    ble_cmd = format_ble_command(CC_SET_PATTERN_SLOT, [3])
    print_command("BLE Command", ble_cmd)
    
    internal_cmd = translate_ble_to_internal(ble_cmd)
    print_command("Internal Command", internal_cmd)
    
    # Verify
    assert internal_cmd[0] == INTERNAL_CMD_START
    assert internal_cmd[1] == INTERNAL_SET_MODE
    assert internal_cmd[3] == 0x02  # Mode 2 = pattern mode
    assert internal_cmd[4] == 3     # Slot 3
    assert internal_cmd[-1] == INTERNAL_CMD_END
    print("✓ Test passed")


def test_pattern_all_command():
    """Test auto-cycle all patterns."""
    print("\n=== Test: Auto-Cycle All Patterns ===")
    ble_cmd = format_ble_command(CC_SET_PATTERN_ALL)
    print_command("BLE Command", ble_cmd)
    
    internal_cmd = translate_ble_to_internal(ble_cmd)
    print_command("Internal Command", internal_cmd)
    
    # Verify
    assert internal_cmd[0] == INTERNAL_CMD_START
    assert internal_cmd[1] == INTERNAL_SET_MODE
    assert internal_cmd[3] == 0x02  # Mode 2 = pattern mode
    assert internal_cmd[4] == 0xFF  # 0xFF = auto-cycle
    assert internal_cmd[-1] == INTERNAL_CMD_END
    print("✓ Test passed")


def test_start_sequencer_command():
    """Test start sequencer."""
    print("\n=== Test: Start Sequencer (slot 0) ===")
    ble_cmd = format_ble_command(CC_START_SEQUENCER, [0])
    print_command("BLE Command", ble_cmd)
    
    internal_cmd = translate_ble_to_internal(ble_cmd)
    print_command("Internal Command", internal_cmd)
    
    # Verify
    assert internal_cmd[0] == INTERNAL_CMD_START
    assert internal_cmd[1] == INTERNAL_SET_MODE
    assert internal_cmd[3] == 0x03  # Mode 3 = sequence mode
    assert internal_cmd[4] == 0     # Sequence 0
    assert internal_cmd[-1] == INTERNAL_CMD_END
    print("✓ Test passed")


def test_pattern_upload_command():
    """Test pattern upload with data."""
    print("\n=== Test: Upload Rainbow Pattern ===")
    # Pattern: type=0 (rainbow), color1=(255,0,0), color2=(0,255,0), speed=50
    pattern_data = [0, 255, 0, 0, 0, 255, 0, 50]
    ble_cmd = format_ble_command(CC_SET_PATTERN, pattern_data)
    print_command("BLE Command", ble_cmd)
    
    internal_cmd = translate_ble_to_internal(ble_cmd)
    print_command("Internal Command", internal_cmd)
    
    # Verify
    assert internal_cmd[0] == INTERNAL_CMD_START
    assert internal_cmd[1] == INTERNAL_UPLOAD_PATTERN
    assert internal_cmd[2] == len(pattern_data)
    assert internal_cmd[-1] == INTERNAL_CMD_END
    print("✓ Test passed")


def main():
    """Run all tests."""
    print("=" * 60)
    print("BLE Protocol Command Translation Tests")
    print("=" * 60)
    
    test_brightness_command()
    test_speed_command()
    test_pattern_slot_command()
    test_pattern_all_command()
    test_start_sequencer_command()
    test_pattern_upload_command()
    
    print("\n" + "=" * 60)
    print("✓ All tests passed!")
    print("=" * 60)


if __name__ == "__main__":
    main()
