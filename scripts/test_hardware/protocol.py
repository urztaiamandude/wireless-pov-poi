"""
Serial protocol helpers for Wireless POV Poi.

Encodes/decodes the internal protocol (0xFF...0xFE) used between ESP32 and
Teensy, and the BLE protocol (0xD0...0xD1) used by the Flutter app.
"""

import struct
from dataclasses import dataclass
from enum import IntEnum
from typing import Optional


# ---------------------------------------------------------------------------
# Internal protocol (ESP32 ↔ Teensy)
# ---------------------------------------------------------------------------
INTERNAL_START = 0xFF
INTERNAL_END = 0xFE

class Cmd(IntEnum):
    SET_MODE       = 0x01
    UPLOAD_IMAGE   = 0x02
    UPLOAD_PATTERN = 0x03
    UPLOAD_SEQ     = 0x04
    LIVE_FRAME     = 0x05
    SET_BRIGHTNESS = 0x06
    SET_FRAMERATE  = 0x07
    STATUS_REQ     = 0x10
    SD_SAVE        = 0x20
    SD_LOAD        = 0x21
    SD_LIST        = 0x22
    SD_DELETE      = 0x23

class Resp(IntEnum):
    ACK    = 0xAA
    STATUS = 0xBB
    LIST   = 0xCC

class Mode(IntEnum):
    IDLE     = 0
    IMAGE    = 1
    PATTERN  = 2
    SEQUENCE = 3
    LIVE     = 4


@dataclass
class StatusResponse:
    mode: int
    index: int
    sd_present: bool


def build_packet(cmd: int, data: bytes = b"") -> bytes:
    """Build an internal-protocol packet: FF CMD LEN DATA... FE"""
    length = len(data)
    if cmd == Cmd.UPLOAD_IMAGE:
        # Image upload uses 16-bit length
        return bytes([INTERNAL_START, cmd, (length >> 8) & 0xFF, length & 0xFF]) + data + bytes([INTERNAL_END])
    if length > 0xFF:
        raise ValueError(
            f"Payload too large for 8-bit length field: {length} bytes (max 255)"
        )
    return bytes([INTERNAL_START, cmd, length & 0xFF]) + data + bytes([INTERNAL_END])


def set_mode(mode: int, index: int = 0) -> bytes:
    return build_packet(Cmd.SET_MODE, bytes([mode, index]))


def set_brightness(value: int) -> bytes:
    return build_packet(Cmd.SET_BRIGHTNESS, bytes([value & 0xFF]))


def set_framerate(fps: int) -> bytes:
    """Build a frame-rate command using the 2-byte (uint16_t) FPS protocol."""
    return build_packet(Cmd.SET_FRAMERATE, bytes([(fps >> 8) & 0xFF, fps & 0xFF]))


def set_framerate_legacy(delay_ms: int) -> bytes:
    """Build a frame-rate command using the legacy 1-byte delay protocol."""
    return build_packet(Cmd.SET_FRAMERATE, bytes([delay_ms & 0xFF]))


def request_status() -> bytes:
    return build_packet(Cmd.STATUS_REQ)


def upload_pattern(index: int, ptype: int,
                   color1: tuple = (255, 0, 0),
                   color2: tuple = (0, 0, 255),
                   speed: int = 50) -> bytes:
    data = bytes([index, ptype,
                  color1[0], color1[1], color1[2],
                  color2[0], color2[1], color2[2],
                  speed])
    return build_packet(Cmd.UPLOAD_PATTERN, data)


def live_frame(pixels: list[tuple[int, int, int]]) -> bytes:
    """pixels: list of 31 (R,G,B) tuples."""
    data = b""
    for r, g, b in pixels[:31]:
        data += bytes([r & 0xFF, g & 0xFF, b & 0xFF])
    return build_packet(Cmd.LIVE_FRAME, data)


def parse_response(data: bytes) -> Optional[tuple[int, bytes]]:
    """
    Extract the first complete response frame from *data*.
    Returns (response_code, payload) or None.
    """
    start = data.find(bytes([INTERNAL_START]))
    if start == -1:
        return None
    end = data.find(bytes([INTERNAL_END]), start + 1)
    if end == -1:
        return None
    frame = data[start:end + 1]
    if len(frame) < 3:
        return None
    resp_code = frame[1]
    payload = frame[2:-1]  # everything between code and end marker
    return (resp_code, payload)


def parse_status(data: bytes) -> Optional[StatusResponse]:
    """Parse a status response (0xBB) frame."""
    result = parse_response(data)
    if result is None:
        return None
    code, payload = result
    # Teensy status frame is: 0xFF 0xBB <mode> <index> 0xFE
    # Some firmware versions may optionally append an SD-present flag byte.
    # Require at least mode and index, treat sd_present as optional.
    if code != Resp.STATUS or len(payload) < 2:
        return None
    sd_present = bool(payload[2]) if len(payload) >= 3 else False
    return StatusResponse(
        mode=payload[0],
        index=payload[1],
        sd_present=sd_present,
    )


def is_ack(data: bytes) -> bool:
    """Return True if *data* contains an ACK (0xAA) response."""
    result = parse_response(data)
    if result is None:
        return False
    return result[0] == Resp.ACK


# ---------------------------------------------------------------------------
# BLE protocol (Flutter App ↔ ESP32)
# ---------------------------------------------------------------------------
BLE_START = 0xD0
BLE_END = 0xD1

class BleCmd(IntEnum):
    SET_BRIGHTNESS   = 0x02
    SET_SPEED        = 0x03
    SET_PATTERN      = 0x04
    SET_PATTERN_SLOT = 0x05
    SET_PATTERN_ALL  = 0x06
    SET_SEQUENCER    = 0x0E
    START_SEQUENCER  = 0x0F


def build_ble_packet(cmd: int, data: bytes = b"") -> bytes:
    return bytes([BLE_START, cmd]) + data + bytes([BLE_END])
