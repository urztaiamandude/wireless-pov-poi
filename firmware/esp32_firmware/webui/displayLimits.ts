/** POV image and sequence bounds shared by Image Lab and the Dashboard. */

export const POV_IMAGE_MIN_HEIGHT = 1;
export const POV_IMAGE_MAX_HEIGHT = 32;
export const POV_IMAGE_MAX_WIDTH = 400;

/** Firmware stores each sequence step duration as a uint16 and rejects values under 100 ms. */
export const SEQUENCE_DURATION_MIN_MS = 100;
export const SEQUENCE_DURATION_MAX_MS = 65535;

export const RETRO_STROBE_PATTERN_ID = 18;

/**
 * Frame rate sent to POST /api/framerate.
 * The ESP32 handler stores a uint16 and does not clamp. The Teensy sets
 * frameDelay = 1000 / fps and raises a zero delay to 1 ms, so 1000 FPS is
 * the fastest rate it runs. The on-device page and docs/API.md use 10–1000.
 * Below 10, the ESP32's uint8 cachedFrameDelay (1000/fps) overflows.
 */
export const FRAME_RATE_MIN_FPS = 10;
export const FRAME_RATE_MAX_FPS = 1000;

/** Pattern speed byte is 1–255. Retro Strobe keeps bit 7 for the RGB flag. */
export const PATTERN_SPEED_MIN = 1;
export const PATTERN_SPEED_MAX = 255;
export const RETRO_STROBE_SPEED_MAX = 127;

export function clampLedHeight(value: number): number {
  if (!Number.isFinite(value)) return POV_IMAGE_MIN_HEIGHT;
  return Math.max(POV_IMAGE_MIN_HEIGHT, Math.min(POV_IMAGE_MAX_HEIGHT, Math.round(value)));
}

export function clampSequenceDuration(ms: number): number {
  if (!Number.isFinite(ms)) return SEQUENCE_DURATION_MIN_MS;
  return Math.max(SEQUENCE_DURATION_MIN_MS, Math.min(SEQUENCE_DURATION_MAX_MS, Math.round(ms)));
}

/**
 * Fit a source image into the POV frame: height follows the LED count (1–32),
 * and width keeps the aspect ratio up to 400 columns.
 */
export function fitPovDimensions(
  sourceWidth: number,
  sourceHeight: number,
  ledHeight: number,
): { width: number; height: number } {
  const height = clampLedHeight(ledHeight);
  const safeHeight = sourceHeight > 0 ? sourceHeight : 1;
  const safeWidth = sourceWidth > 0 ? sourceWidth : 1;
  let width = Math.max(1, Math.round(height * (safeWidth / safeHeight)));
  let fittedHeight = height;
  if (width > POV_IMAGE_MAX_WIDTH) {
    const scale = POV_IMAGE_MAX_WIDTH / width;
    width = POV_IMAGE_MAX_WIDTH;
    fittedHeight = Math.max(1, Math.round(height * scale));
  }
  return { width, height: fittedHeight };
}

export function clampFrameRate(fps: number): number {
  if (!Number.isFinite(fps)) return 60;
  return Math.max(FRAME_RATE_MIN_FPS, Math.min(FRAME_RATE_MAX_FPS, Math.round(fps)));
}

/**
 * Speed byte sent to /api/pattern.
 * Retro Strobe uses bit 7 as the RGB-mode flag, so the Dashboard slider
 * may only occupy bits 6..0 (1–127). Other patterns keep the 1–255 slider.
 */
export function clampPatternSpeed(patternId: number, speed: number): number {
  const max = patternId === RETRO_STROBE_PATTERN_ID ? RETRO_STROBE_SPEED_MAX : PATTERN_SPEED_MAX;
  if (!Number.isFinite(speed)) return PATTERN_SPEED_MIN;
  return Math.max(PATTERN_SPEED_MIN, Math.min(max, Math.round(speed)));
}
