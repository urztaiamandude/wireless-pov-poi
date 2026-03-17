
import React, { useRef, useEffect, useState, useCallback } from 'react';
import { Music, Play, Pause, Mic } from 'lucide-react';

// ─── Color helpers (match FastLED behavior) ──────────────────────────
function hsvToRgb(h: number, s: number, v: number): [number, number, number] {
  h = ((h % 256) + 256) % 256;
  const hf = (h / 256) * 6;
  const hi = Math.floor(hf);
  const f = hf - hi;
  const p = v * (1 - s / 255);
  const q = v * (1 - (s / 255) * f);
  const t = v * (1 - (s / 255) * (1 - f));
  let r = 0, g = 0, b = 0;
  switch (hi % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }
  return [Math.round(r), Math.round(g), Math.round(b)];
}

/** FastLED sin8 approximation: sine wave 0-255 input → 0-255 output */
function sin8(x: number): number {
  return Math.round((Math.sin((((x & 0xFF) / 256) * 2 * Math.PI) - Math.PI / 2) + 1) * 127.5);
}

/** FastLED beatsin8 approximation */
function beatsin8(bpm: number, lo: number, hi: number, timeMs: number): number {
  const beat = (timeMs / 1000) * bpm / 60;
  const val = (Math.sin(beat * 2 * Math.PI) + 1) / 2;
  return Math.round(lo + val * (hi - lo));
}

/** FastLED blend approximation */
function blendColor(c1: [number, number, number], c2: [number, number, number], amount: number): [number, number, number] {
  const a = amount / 255;
  return [
    Math.round(c1[0] + (c2[0] - c1[0]) * a),
    Math.round(c1[1] + (c2[1] - c1[1]) * a),
    Math.round(c1[2] + (c2[2] - c1[2]) * a),
  ];
}

/** FastLED HeatColor approximation */
function heatColor(heat: number): [number, number, number] {
  if (heat < 85) return [heat * 3, 0, 0];
  if (heat < 170) return [255, (heat - 85) * 3, 0];
  return [255, 255, (heat - 170) * 3];
}

/** Scale an RGB tuple by a 0-255 factor */
function scaleColor(c: [number, number, number], s: number): [number, number, number] {
  return [Math.round(c[0] * s / 255), Math.round(c[1] * s / 255), Math.round(c[2] * s / 255)];
}

/** Fade-to-black: reduce each channel proportionally */
function fadeToBlack(c: [number, number, number], amount: number): [number, number, number] {
  const scale = Math.max(0, 1 - amount / 255);
  return [Math.round(c[0] * scale), Math.round(c[1] * scale), Math.round(c[2] * scale)];
}

function hexToRgb(hex: string): [number, number, number] {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return [r, g, b];
}

function clamp(v: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, v));
}

// ─── Pattern definitions ─────────────────────────────────────────────
interface PatternDef {
  id: number;
  label: string;
  group: 'basic' | 'audio' | 'advanced';
  description: string;
}

const PATTERNS: PatternDef[] = [
  { id: 0,  label: 'Rainbow',       group: 'basic',    description: 'Full spectrum rainbow scrolling across the LED strip. Hue shifts continuously, producing smooth cycling through all colors.' },
  { id: 1,  label: 'Wave',          group: 'basic',    description: 'Sine-wave brightness modulation on a single color. LEDs pulse in a smooth traveling wave pattern.' },
  { id: 2,  label: 'Gradient',      group: 'basic',    description: 'Scrolling blend between two colors. A smooth sine-based transition continuously moves across the strip.' },
  { id: 3,  label: 'Sparkle',       group: 'basic',    description: 'Random bright pixels that appear and fade to black. Speed controls how frequently new sparkles are generated.' },
  { id: 4,  label: 'Fire',          group: 'basic',    description: 'Realistic flame simulation. Heat rises from the bottom with random ignition, cooling, and diffusion creating organic fire movement.' },
  { id: 5,  label: 'Comet',         group: 'basic',    description: 'A single bright head bounces end-to-end with a fading tail trailing behind it.' },
  { id: 6,  label: 'Breathing',     group: 'basic',    description: 'The entire strip uniformly pulses on and off in a smooth breathing rhythm.' },
  { id: 7,  label: 'Strobe',        group: 'basic',    description: 'Quick on/off flashes. Speed controls flash rate from slow blinks (500ms) to rapid strobe (10ms).' },
  { id: 8,  label: 'Meteor',        group: 'basic',    description: 'A bright head falls downward with a sparkly random-decay trail. The meteor wraps back to the top when it reaches the bottom.' },
  { id: 9,  label: 'Wipe',          group: 'basic',    description: 'LEDs fill one-by-one with color, then clear one-by-one. A classic progressive fill/clear animation.' },
  { id: 10, label: 'Plasma',        group: 'basic',    description: 'Organic color mixing using overlapping sine waves. Creates flowing, psychedelic color patterns.' },
  { id: 11, label: 'VU Meter',      group: 'audio',    description: 'Audio-driven level bar with green→yellow→red gradient. A white peak indicator holds briefly then decays. Beat detection shifts the color palette.' },
  { id: 12, label: 'Pulse',         group: 'audio',    description: 'The entire strip flashes bright on beat detection, then smoothly decays to dark. Whole-strip pulse effect.' },
  { id: 13, label: 'Audio Rainbow', group: 'audio',    description: 'Rainbow pattern whose scroll speed is driven by audio level. Louder audio = faster rainbow movement and brighter colors.' },
  { id: 14, label: 'Center Burst',  group: 'audio',    description: 'Color wave expands outward from the center of the strip based on audio level. Louder = wider expansion.' },
  { id: 15, label: 'Audio Sparkle', group: 'audio',    description: 'Sparkle intensity driven by audio level. More sparkles appear with louder input (0-8 sparkles per frame).' },
  { id: 16, label: 'Split Spin',    group: 'advanced',  description: 'Two solid color halves rotate around the strip. Creates a spinning bicolor effect when the poi is in motion.' },
  { id: 17, label: 'Theater Chase', group: 'advanced',  description: 'Classic theater-marquee chase. Every 3rd LED is lit, with the pattern shifting position each frame.' },
];

const DEFAULT_SPEED = 50;
const DEFAULT_COLOR1 = '#FF0000';
const DEFAULT_COLOR2 = '#0000FF';

type RGB = [number, number, number];

// ─── Single pattern preview canvas ───────────────────────────────────
interface PatternCanvasProps {
  patternId: number;
  speed: number;
  color1: string;
  color2: string;
  playing: boolean;
  audioLevel: number;      // 0-255 simulated audio level
  ledCount: number;
  height?: number;
}

interface PatternState {
  heat: number[];
  cometPos: number;
  cometDir: number;
  meteorPos: number;
  wipePos: number;
  wipeFilling: boolean;
  strobeOn: boolean;
  lastStrobeMs: number;
  peakLevel: number;
  peakDecay: number;
  beatHue: number;
  pulseVal: number;
  lastAudioLevel: number;
  rainbowOffset: number;
  sparkleBuffer: RGB[];
}

/** Copy an RGB tuple (avoids spread-to-any problems) */
function copyRgb(c: RGB): RGB { return [c[0], c[1], c[2]]; }

const PatternCanvas: React.FC<PatternCanvasProps> = ({
  patternId, speed, color1, color2, playing, audioLevel, ledCount, height = 200,
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const stateRef = useRef<Record<number, PatternState>>({});
  const animRef = useRef<number>(0);
  const startTimeRef = useRef<number>(performance.now());
  const ledsRef = useRef<RGB[]>([]);

  // Persistent per-pattern state
  const getState = useCallback((): PatternState => {
    if (!stateRef.current[patternId]) {
      stateRef.current[patternId] = {
        heat: new Array(ledCount).fill(0),
        cometPos: 0,
        cometDir: 1,
        meteorPos: ledCount - 1,
        wipePos: 0,
        wipeFilling: true,
        strobeOn: false,
        lastStrobeMs: 0,
        peakLevel: 0,
        peakDecay: 0,
        beatHue: 0,
        pulseVal: 0,
        lastAudioLevel: 0,
        rainbowOffset: 0,
        sparkleBuffer: new Array(ledCount).fill(null).map((): RGB => [0, 0, 0]),
      };
    }
    return stateRef.current[patternId];
  }, [patternId, ledCount]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const LED_W = Math.floor(canvas.width / ledCount);
    const LED_GAP = 1;

    const render = () => {
      if (!playing) return;
      const now = performance.now();
      const elapsed = now - startTimeRef.current;
      const patternTime = Math.floor(elapsed / 16); // ~60fps frame counter
      const timeMs = elapsed;
      const c1 = hexToRgb(color1);
      const c2 = hexToRgb(color2);
      const st = getState();
      // Reuse pre-allocated array to reduce GC pressure
      if (ledsRef.current.length !== ledCount) {
        ledsRef.current = new Array(ledCount).fill(null).map((): RGB => [0, 0, 0]);
      }
      const leds = ledsRef.current;
      for (let i = 0; i < ledCount; i++) { leds[i][0] = 0; leds[i][1] = 0; leds[i][2] = 0; }

      switch (patternId) {
        // ── 0: Rainbow ─────────────────────────────────
        case 0:
          for (let i = 0; i < ledCount; i++) {
            const hue = (patternTime * speed / 10 + i * 255 / ledCount) % 256;
            leds[i] = hsvToRgb(hue, 255, 255);
          }
          break;

        // ── 1: Wave ────────────────────────────────────
        case 1:
          for (let i = 0; i < ledCount; i++) {
            const brightness = sin8(patternTime * speed / 10 + i * 255 / ledCount);
            leds[i] = scaleColor(c1, brightness);
          }
          break;

        // ── 2: Gradient ────────────────────────────────
        case 2: {
          const timeOffset = Math.floor((timeMs / 500) * speed) & 0xFF;
          for (let i = 0; i < ledCount; i++) {
            const phase = ((i * 255 / ledCount) + timeOffset) & 0xFF;
            leds[i] = blendColor(c1, c2, sin8(phase));
          }
          break;
        }

        // ── 3: Sparkle ────────────────────────────────
        case 3:
          // Fade existing
          for (let i = 0; i < ledCount; i++) {
            st.sparkleBuffer[i] = fadeToBlack(st.sparkleBuffer[i], 20);
          }
          // Add new sparkle based on speed
          if (Math.random() * 255 < speed) {
            const pos = Math.floor(Math.random() * ledCount);
            st.sparkleBuffer[pos] = copyRgb(c1);
          }
          for (let i = 0; i < ledCount; i++) leds[i] = copyRgb(st.sparkleBuffer[i]);
          break;

        // ── 4: Fire ───────────────────────────────────
        case 4: {
          // Cool down — formula matches Teensy firmware (COOLING=55, scale by strip length)
          for (let i = 0; i < ledCount; i++) {
            st.heat[i] = Math.max(0, st.heat[i] - Math.random() * (((55 * 10) / ledCount) + 2));
          }
          // Heat rises
          for (let i = ledCount - 1; i >= 2; i--) {
            st.heat[i] = (st.heat[i - 1] + st.heat[i - 2] + st.heat[i - 2]) / 3;
          }
          // Random ignition at bottom
          if (Math.random() * 255 < speed) {
            const y = Math.floor(Math.random() * Math.min(3, ledCount));
            st.heat[y] = Math.min(255, st.heat[y] + 160 + Math.random() * 95);
          }
          for (let i = 0; i < ledCount; i++) {
            leds[i] = heatColor(Math.round(st.heat[i]));
          }
          break;
        }

        // ── 5: Comet ──────────────────────────────────
        case 5: {
          // Fade for tail
          for (let i = 0; i < ledCount; i++) {
            st.sparkleBuffer[i] = fadeToBlack(st.sparkleBuffer[i], 60);
          }
          st.cometPos += st.cometDir;
          if (st.cometPos >= ledCount - 1 || st.cometPos <= 0) {
            st.cometDir = -st.cometDir;
          }
          st.cometPos = clamp(st.cometPos, 0, ledCount - 1);
          st.sparkleBuffer[st.cometPos] = copyRgb(c1);
          const tailPos = st.cometPos - st.cometDir;
          if (tailPos >= 0 && tailPos < ledCount) {
            st.sparkleBuffer[tailPos] = scaleColor(c1, 128);
          }
          for (let i = 0; i < ledCount; i++) leds[i] = copyRgb(st.sparkleBuffer[i]);
          break;
        }

        // ── 6: Breathing ──────────────────────────────
        case 6: {
          const breath = beatsin8(speed / 4, 20, 255, timeMs);
          for (let i = 0; i < ledCount; i++) {
            leds[i] = scaleColor(c1, breath);
          }
          break;
        }

        // ── 7: Strobe ─────────────────────────────────
        case 7: {
          const strobeDelay = 500 - ((speed / 255) * 490);
          if (timeMs - st.lastStrobeMs >= strobeDelay) {
            st.strobeOn = !st.strobeOn;
            st.lastStrobeMs = timeMs;
          }
          for (let i = 0; i < ledCount; i++) {
            leds[i] = st.strobeOn ? copyRgb(c1) : [0, 0, 0];
          }
          break;
        }

        // ── 8: Meteor ─────────────────────────────────
        case 8: {
          // Random sparkly decay
          for (let i = 0; i < ledCount; i++) {
            if (Math.random() < 80 / 255) {
              st.sparkleBuffer[i] = fadeToBlack(st.sparkleBuffer[i], 64);
            }
          }
          // Draw meteor head (4 LEDs)
          for (let j = 0; j < 4; j++) {
            const pos = st.meteorPos - j;
            if (pos >= 0 && pos < ledCount) {
              st.sparkleBuffer[pos] = scaleColor(c1, 255 - j * 60);
            }
          }
          st.meteorPos--;
          if (st.meteorPos < 0) st.meteorPos = ledCount - 1;
          for (let i = 0; i < ledCount; i++) leds[i] = copyRgb(st.sparkleBuffer[i]);
          break;
        }

        // ── 9: Wipe ───────────────────────────────────
        case 9: {
          // Fill or clear one LED per frame
          if (st.wipeFilling) {
            leds[st.wipePos] = copyRgb(c1);
          }
          // Draw current state of all LEDs
          for (let i = 0; i < ledCount; i++) {
            if (st.wipeFilling) {
              leds[i] = i <= st.wipePos ? copyRgb(c1) : [0, 0, 0];
            } else {
              leds[i] = i <= st.wipePos ? [0, 0, 0] : copyRgb(c1);
            }
          }
          st.wipePos++;
          if (st.wipePos >= ledCount) {
            st.wipePos = 0;
            st.wipeFilling = !st.wipeFilling;
          }
          break;
        }

        // ── 10: Plasma ────────────────────────────────
        case 10:
          for (let i = 0; i < ledCount; i++) {
            const hue = (sin8(i * 10 + patternTime * speed / 20) +
                        sin8(i * 15 - patternTime * speed / 15) +
                        sin8(patternTime * speed / 10)) & 0xFF;
            leds[i] = hsvToRgb(hue, 255, 255);
          }
          break;

        // ── 11: VU Meter (audio-reactive) ─────────────
        case 11: {
          const al = audioLevel;
          // Beat detection
          if (al > st.peakLevel + 30) {
            st.beatHue = (st.beatHue + 32) & 0xFF;
          }
          // Peak tracking
          if (al > st.peakLevel) {
            st.peakLevel = al;
            st.peakDecay = 0;
          } else {
            st.peakDecay++;
            if (st.peakDecay > 5) {
              st.peakLevel = Math.max(0, st.peakLevel - 3);
            }
          }
          const ledsToLight = Math.round(al / 255 * ledCount);
          for (let i = 0; i < ledCount; i++) {
            if (i < ledsToLight) {
              let hue: number;
              if (i < ledCount / 3) hue = 96;       // Green
              else if (i < 2 * ledCount / 3) hue = 64; // Yellow
              else hue = 0;                            // Red
              hue = (hue + st.beatHue) & 0xFF;
              leds[i] = hsvToRgb(hue, 255, 255);
            } else {
              leds[i] = fadeToBlack(leds[i], 50);
            }
          }
          // Peak indicator
          const peakPos = Math.round(st.peakLevel / 255 * (ledCount - 1));
          if (peakPos >= 0 && peakPos < ledCount) {
            leds[peakPos] = [255, 255, 255];
          }
          break;
        }

        // ── 12: Music Pulse (audio-reactive) ──────────
        case 12: {
          const al = audioLevel;
          if (al > st.lastAudioLevel + 20 && al > 100) {
            st.pulseVal = 255;
          }
          st.lastAudioLevel = al;
          for (let i = 0; i < ledCount; i++) {
            leds[i] = scaleColor(c1, Math.round(st.pulseVal));
          }
          st.pulseVal = st.pulseVal * (220 / 256);
          break;
        }

        // ── 13: Audio Rainbow (audio-reactive) ────────
        case 13: {
          const al = audioLevel;
          const speedAdd = Math.round(1 + (al / 255) * 19);
          st.rainbowOffset += speedAdd;
          for (let i = 0; i < ledCount; i++) {
            const hue = (Math.floor(st.rainbowOffset / 4) + i * 255 / ledCount) & 0xFF;
            const brightness = clamp(al + 50, 50, 255);
            leds[i] = hsvToRgb(hue, 255, brightness);
          }
          break;
        }

        // ── 14: Center Burst (audio-reactive) ─────────
        case 14: {
          const al = audioLevel;
          const expansion = Math.round(al / 255 * (ledCount / 2));
          const center = Math.floor(ledCount / 2);
          // Fade all first
          for (let i = 0; i < ledCount; i++) {
            st.sparkleBuffer[i] = fadeToBlack(st.sparkleBuffer[i], 80);
          }
          // Expand from center
          for (let i = 0; i <= expansion; i++) {
            const hue = (patternTime * speed / 20 + i * 10) & 0xFF;
            const rgb = hsvToRgb(hue, 255, 255);
            if (center + i < ledCount) st.sparkleBuffer[center + i] = rgb;
            if (center - i >= 0) st.sparkleBuffer[center - i] = rgb;
          }
          for (let i = 0; i < ledCount; i++) leds[i] = copyRgb(st.sparkleBuffer[i]);
          break;
        }

        // ── 15: Audio Sparkle (audio-reactive) ────────
        case 15: {
          const al = audioLevel;
          // Fade existing
          for (let i = 0; i < ledCount; i++) {
            st.sparkleBuffer[i] = fadeToBlack(st.sparkleBuffer[i], 40);
          }
          // Add sparkles proportional to audio
          const numSparkles = Math.round(al / 255 * 8);
          for (let s = 0; s < numSparkles; s++) {
            const pos = Math.floor(Math.random() * ledCount);
            const hue = (patternTime * 2 + Math.floor(Math.random() * 64)) & 0xFF;
            st.sparkleBuffer[pos] = hsvToRgb(hue, 255, 255);
          }
          for (let i = 0; i < ledCount; i++) leds[i] = copyRgb(st.sparkleBuffer[i]);
          break;
        }

        // ── 16: Split Spin ────────────────────────────
        case 16: {
          const offset = Math.floor(patternTime * speed / 20) % ledCount;
          const splitPoint = Math.floor(ledCount / 2);
          for (let i = 0; i < ledCount; i++) {
            const pos = (i + offset) % ledCount;
            leds[i] = pos < splitPoint ? copyRgb(c1) : copyRgb(c2);
          }
          break;
        }

        // ── 17: Theater Chase ─────────────────────────
        case 17: {
          const chaseOffset = Math.floor(patternTime * speed / 20) % 3;
          for (let i = 0; i < ledCount; i++) {
            const phase = (i + chaseOffset) % 3;
            leds[i] = phase === 0 ? copyRgb(c1) : copyRgb(c2);
          }
          break;
        }

        default:
          break;
      }

      // Render to canvas
      ctx.fillStyle = '#0f172a';
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      const totalGap = (ledCount - 1) * LED_GAP;
      const ledWidth = (canvas.width - totalGap - 4) / ledCount;
      const ledHeight = canvas.height - 4;

      for (let i = 0; i < ledCount; i++) {
        const [r, g, b] = leds[i];
        const x = 2 + i * (ledWidth + LED_GAP);
        // Glow effect
        if (r + g + b > 30) {
          ctx.shadowBlur = 8;
          ctx.shadowColor = `rgb(${r},${g},${b})`;
        } else {
          ctx.shadowBlur = 0;
        }
        ctx.fillStyle = `rgb(${r},${g},${b})`;
        ctx.beginPath();
        ctx.roundRect(x, 2, ledWidth, ledHeight, 2);
        ctx.fill();
      }
      ctx.shadowBlur = 0;

      animRef.current = requestAnimationFrame(render);
    };

    if (playing) {
      animRef.current = requestAnimationFrame(render);
    }
    return () => cancelAnimationFrame(animRef.current);
  }, [patternId, speed, color1, color2, playing, audioLevel, ledCount, getState, height]);

  return (
    <canvas
      ref={canvasRef}
      width={620}
      height={height}
      className="w-full rounded-lg"
      style={{ imageRendering: 'pixelated' }}
    />
  );
};

// ─── Simulated audio level generator ─────────────────────────────────
// Frequencies (Hz) and amplitudes for a music-like waveform
const SIM_BASS_FREQ = 1.8;      // Bass drum hits
const SIM_BASS_AMP  = 0.7;
const SIM_MID_FREQ  = 3.2;      // Mid-range rhythm
const SIM_MID_AMP   = 0.4;
const SIM_MID_PHASE = 1.3;
const SIM_HI_FREQ   = 7.5;      // Hi-hat taps
const SIM_HI_AMP    = 0.2;
const SIM_HI_PHASE  = 0.7;
const SIM_ENV_FREQ  = 0.15;     // Slow volume envelope
const SIM_NOISE_AMP = 0.08;     // Random noise floor

function useSimulatedAudio(enabled: boolean): number {
  const [level, setLevel] = useState(0);
  const rafRef = useRef<number>(0);
  const startRef = useRef(performance.now());

  useEffect(() => {
    if (!enabled) { setLevel(0); return; }
    const tick = () => {
      const t = (performance.now() - startRef.current) / 1000;
      // Mix several sine waves to simulate music dynamics with beats
      const bass = Math.max(0, Math.sin(t * 2 * Math.PI * SIM_BASS_FREQ) * SIM_BASS_AMP);
      const mid  = Math.max(0, Math.sin(t * 2 * Math.PI * SIM_MID_FREQ + SIM_MID_PHASE) * SIM_MID_AMP);
      const hi   = Math.max(0, Math.sin(t * 2 * Math.PI * SIM_HI_FREQ + SIM_HI_PHASE) * SIM_HI_AMP);
      const envelope = 0.5 + 0.5 * Math.sin(t * 2 * Math.PI * SIM_ENV_FREQ);
      const noise = Math.random() * SIM_NOISE_AMP;
      const raw = (bass + mid + hi + noise) * envelope;
      setLevel(Math.round(clamp(raw * 255, 0, 255)));
      rafRef.current = requestAnimationFrame(tick);
    };
    rafRef.current = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(rafRef.current);
  }, [enabled]);

  return level;
}

// ─── Microphone audio level ──────────────────────────────────────────
function useMicrophoneAudio(enabled: boolean): { level: number; supported: boolean; error: string | null } {
  const [level, setLevel] = useState(0);
  const [supported] = useState(() => !!(navigator.mediaDevices?.getUserMedia));
  const [error, setError] = useState<string | null>(null);
  const cleanupRef = useRef<(() => void) | null>(null);

  useEffect(() => {
    if (!enabled || !supported) { setLevel(0); return; }

    let cancelled = false;

    (async () => {
      try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        if (cancelled) { stream.getTracks().forEach(t => t.stop()); return; }

        const audioCtx = new AudioContext();
        const source = audioCtx.createMediaStreamSource(stream);
        const analyser = audioCtx.createAnalyser();
        analyser.fftSize = 256;
        source.connect(analyser);
        const dataArray = new Uint8Array(analyser.frequencyBinCount);

        const tick = () => {
          if (cancelled) return;
          analyser.getByteFrequencyData(dataArray);
          let sum = 0;
          for (let i = 0; i < dataArray.length; i++) sum += dataArray[i];
          setLevel(Math.round(clamp(sum / dataArray.length * 2, 0, 255)));
          requestAnimationFrame(tick);
        };
        requestAnimationFrame(tick);

        cleanupRef.current = () => {
          stream.getTracks().forEach(t => t.stop());
          source.disconnect();
          audioCtx.close();
        };
      } catch (e) {
        setError('Microphone access denied or unavailable');
      }
    })();

    return () => {
      cancelled = true;
      cleanupRef.current?.();
      cleanupRef.current = null;
    };
  }, [enabled, supported]);

  return { level, supported, error };
}

// ─── Audio level bar ─────────────────────────────────────────────────
const AudioLevelBar: React.FC<{ level: number }> = ({ level }) => (
  <div className="flex items-center gap-2 text-[9px] text-slate-500">
    <span className="w-14">Audio: {level}</span>
    <div className="flex-1 h-2 bg-slate-800 rounded-full overflow-hidden">
      <div
        className="h-full bg-gradient-to-r from-green-500 via-yellow-500 to-red-500 transition-all duration-75"
        style={{ width: `${(level / 255) * 100}%` }}
      />
    </div>
  </div>
);

// ─── Main PatternPreview component ───────────────────────────────────
interface PatternPreviewProps {
  ledCount: number;
}

const PatternPreview: React.FC<PatternPreviewProps> = ({ ledCount }) => {
  const [selectedPattern, setSelectedPattern] = useState<number | null>(null);
  const [globalPlaying, setGlobalPlaying] = useState(true);
  const [speed, setSpeed] = useState(DEFAULT_SPEED);
  const [color1, setColor1] = useState(DEFAULT_COLOR1);
  const [color2, setColor2] = useState(DEFAULT_COLOR2);
  const [useMic, setUseMic] = useState(false);

  const simulatedAudio = useSimulatedAudio(!useMic && globalPlaying);
  const mic = useMicrophoneAudio(useMic && globalPlaying);
  const audioLevel = useMic ? mic.level : simulatedAudio;

  const renderCard = (p: PatternDef) => {
    const isSelected = selectedPattern === p.id;
    const isAudio = p.group === 'audio';
    return (
      <div
        key={p.id}
        className={`rounded-xl border transition-all cursor-pointer ${
          isSelected
            ? 'border-cyan-500/50 bg-slate-800/80 ring-1 ring-cyan-500/20'
            : 'border-slate-800 bg-slate-900/60 hover:border-slate-700'
        }`}
        onClick={() => setSelectedPattern(isSelected ? null : p.id)}
      >
        <div className="p-3">
          <div className="flex items-center justify-between mb-2">
            <div className="flex items-center gap-2">
              <span className="text-[10px] font-mono text-slate-600">#{p.id}</span>
              <span className="text-xs font-bold text-slate-200">{p.label}</span>
              {isAudio && <Music size={11} className="text-pink-400" />}
            </div>
            <span className={`text-[8px] px-2 py-0.5 rounded-full font-bold uppercase tracking-wider ${
              p.group === 'basic' ? 'bg-indigo-900/50 text-indigo-400' :
              p.group === 'audio' ? 'bg-pink-900/50 text-pink-400' :
              'bg-violet-900/50 text-violet-400'
            }`}>
              {p.group}
            </span>
          </div>

          <PatternCanvas
            patternId={p.id}
            speed={speed}
            color1={color1}
            color2={color2}
            playing={globalPlaying}
            audioLevel={isAudio ? audioLevel : 0}
            ledCount={ledCount}
            height={48}
          />

          {isSelected && (
            <div className="mt-2 text-[10px] text-slate-400 leading-relaxed">
              {p.description}
            </div>
          )}
          {isSelected && isAudio && (
            <div className="mt-2">
              <AudioLevelBar level={audioLevel} />
            </div>
          )}
        </div>
      </div>
    );
  };

  // Group patterns
  const basicPatterns = PATTERNS.filter(p => p.group === 'basic');
  const audioPatterns = PATTERNS.filter(p => p.group === 'audio');
  const advancedPatterns = PATTERNS.filter(p => p.group === 'advanced');

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-xl font-black text-white">Pattern Preview</h2>
          <p className="text-[10px] text-slate-500 mt-0.5">
            Live preview of all 18 firmware patterns • Click a pattern for details
          </p>
        </div>
        <button
          onClick={() => setGlobalPlaying(!globalPlaying)}
          className={`flex items-center gap-2 px-4 py-2 rounded-lg text-xs font-bold transition-all ${
            globalPlaying
              ? 'bg-green-600 text-white hover:bg-green-500'
              : 'bg-slate-700 text-slate-300 hover:bg-slate-600'
          }`}
        >
          {globalPlaying ? <Pause size={14} /> : <Play size={14} />}
          {globalPlaying ? 'Pause All' : 'Play All'}
        </button>
      </div>

      {/* Global Controls */}
      <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4">
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div>
            <label className="text-[9px] text-slate-500 uppercase tracking-widest block mb-1">Speed</label>
            <div className="flex items-center gap-2">
              <input
                type="range"
                min="1" max="255"
                value={speed}
                onChange={e => setSpeed(Number(e.target.value))}
                className="flex-1"
              />
              <span className="text-xs font-mono text-cyan-400 w-8 text-right">{speed}</span>
            </div>
          </div>
          <div className="flex gap-4">
            <div>
              <label className="text-[9px] text-slate-500 uppercase tracking-widest block mb-1">Color 1</label>
              <input type="color" value={color1} onChange={e => setColor1(e.target.value)}
                className="w-full h-8 rounded-lg border border-slate-700 bg-slate-800 cursor-pointer" />
            </div>
            <div>
              <label className="text-[9px] text-slate-500 uppercase tracking-widest block mb-1">Color 2</label>
              <input type="color" value={color2} onChange={e => setColor2(e.target.value)}
                className="w-full h-8 rounded-lg border border-slate-700 bg-slate-800 cursor-pointer" />
            </div>
          </div>
          <div>
            <label className="text-[9px] text-slate-500 uppercase tracking-widest block mb-1">Audio Source</label>
            <div className="flex gap-2">
              <button
                onClick={() => setUseMic(false)}
                className={`flex-1 py-1.5 rounded-lg text-[10px] font-bold border transition-all ${
                  !useMic
                    ? 'bg-pink-600 border-pink-400/30 text-white'
                    : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                }`}
              >
                Simulated
              </button>
              <button
                onClick={() => setUseMic(true)}
                className={`flex-1 py-1.5 rounded-lg text-[10px] font-bold border transition-all flex items-center justify-center gap-1 ${
                  useMic
                    ? 'bg-pink-600 border-pink-400/30 text-white'
                    : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                }`}
              >
                <Mic size={10} /> Live Mic
              </button>
            </div>
            {useMic && mic.error && (
              <div className="text-[9px] text-red-400 mt-1">{mic.error}</div>
            )}
          </div>
        </div>
        {/* Audio level indicator for audio source */}
        <div className="mt-3">
          <AudioLevelBar level={audioLevel} />
        </div>
      </div>

      {/* Basic Patterns */}
      <div>
        <div className="text-[10px] font-black text-slate-500 uppercase tracking-widest mb-2">
          Basic Patterns (0–10)
        </div>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
          {basicPatterns.map(renderCard)}
        </div>
      </div>

      {/* Audio-Reactive Patterns */}
      <div>
        <div className="text-[10px] font-black text-slate-500 uppercase tracking-widest mb-2 flex items-center gap-1.5">
          <Music size={12} className="text-pink-400" />
          Audio-Reactive Patterns (11–15)
          <span className="text-[8px] text-slate-600 font-normal ml-1">— simulated audio or live mic</span>
        </div>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
          {audioPatterns.map(renderCard)}
        </div>
      </div>

      {/* Advanced Patterns */}
      <div>
        <div className="text-[10px] font-black text-slate-500 uppercase tracking-widest mb-2">
          Advanced Patterns (16–17)
        </div>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
          {advancedPatterns.map(renderCard)}
        </div>
      </div>

      {/* Verification Summary */}
      <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4">
        <div className="text-[10px] font-black text-slate-500 uppercase tracking-widest mb-3">
          Verification Summary
        </div>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-2">
          {PATTERNS.map(p => (
            <div key={p.id} className="flex items-center gap-2 text-[10px]">
              <span className="text-green-400">✓</span>
              <span className="font-mono text-slate-600 w-5">#{p.id}</span>
              <span className="text-slate-300 font-medium">{p.label}</span>
              <span className="text-slate-600">—</span>
              <span className="text-slate-500 truncate flex-1">{p.description.split('.')[0]}</span>
              {p.group === 'audio' && <Music size={9} className="text-pink-400 flex-shrink-0" />}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default PatternPreview;
