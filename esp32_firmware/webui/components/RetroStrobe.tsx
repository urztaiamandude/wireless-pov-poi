
import React, { useState, useCallback } from 'react';
import { Zap, ToggleLeft, ToggleRight } from 'lucide-react';
import { hapticImpact } from '../nativeFeatures';

function getDeviceBase(ip: string): string {
  const isLocalhost = typeof window !== 'undefined' &&
    (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1');
  if (isLocalhost) return '';
  const protocol = window.location.protocol === 'https:' ? 'https:' : 'http:';
  return ip ? `${protocol}//${ip}` : window.location.origin;
}

/** Default device IP for the POI leader. */
const DEFAULT_IP = '192.168.4.1';

/** Pattern type ID for Retro Strobe on the Teensy. */
const RETRO_STROBE_TYPE = 18;

/**
 * Encode the speed byte for the Retro Strobe pattern.
 *
 * Bit layout:
 *   bit 7      : sub-mode flag  (1 = RGB White, 0 = Dual Color)
 *   bits 6..0  : timing index   strobeMicros = value * 5 + 100  (100–735 μs)
 *
 * Default timing index 40 → 300 μs → ~3 333 Hz show rate.
 */
function encodeSpeed(rgbMode: boolean, timingIndex: number): number {
  const clamped = Math.max(0, Math.min(127, Math.round(timingIndex)));
  return (rgbMode ? 0x80 : 0x00) | clamped;
}

/** Convert a hex color string (#RRGGBB) to {r, g, b}. */
function hexToRgb(hex: string): { r: number; g: number; b: number } {
  const v = parseInt(hex.replace('#', ''), 16);
  return { r: (v >> 16) & 0xFF, g: (v >> 8) & 0xFF, b: v & 0xFF };
}

const RetroStrobe: React.FC = () => {
  const [rgbMode, setRgbMode] = useState(true);
  const [colorA, setColorA] = useState('#ff0000');
  const [colorB, setColorB] = useState('#0000ff');
  const [timingIndex, setTimingIndex] = useState(40); // 300 μs default
  const [sending, setSending] = useState(false);
  const [status, setStatus] = useState<string | null>(null);

  const strobeMicros = timingIndex * 5 + 100;
  const showRateHz = Math.round(1_000_000 / strobeMicros);

  const activate = useCallback(async () => {
    setSending(true);
    setStatus(null);
    hapticImpact('medium');

    const base = getDeviceBase(DEFAULT_IP);
    const speed = encodeSpeed(rgbMode, timingIndex);
    const c1 = hexToRgb(colorA);
    const c2 = hexToRgb(colorB);

    try {
      // 1. Upload pattern configuration
      const resPattern = await fetch(`${base}/api/pattern`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          index: RETRO_STROBE_TYPE,
          type: RETRO_STROBE_TYPE,
          color1: c1,
          color2: c2,
          speed,
        }),
      });
      if (!resPattern.ok) throw new Error(`Pattern: ${resPattern.status}`);

      // 2. Switch to pattern mode with retro strobe index
      const resMode = await fetch(`${base}/api/mode`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: 2, index: RETRO_STROBE_TYPE }),
      });
      if (!resMode.ok) throw new Error(`Mode: ${resMode.status}`);

      setStatus('Active');
    } catch (err) {
      setStatus(`Error: ${err instanceof Error ? err.message : String(err)}`);
    } finally {
      setSending(false);
    }
  }, [rgbMode, timingIndex, colorA, colorB]);

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center gap-3">
        <div className="p-2.5 bg-orange-500/20 rounded-xl">
          <Zap className="text-orange-400 w-7 h-7" />
        </div>
        <div>
          <h2 className="text-xl font-bold text-white">Retro Strobe</h2>
          <p className="text-xs text-slate-400">
            Temporal color interleaving — emulates classic RGB ball poi
          </p>
        </div>
      </div>

      {/* Explanation */}
      <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4 text-xs text-slate-400 space-y-1">
        <p>
          <strong className="text-slate-300">Stationary:</strong> High-speed flashing blends into a single solid bar
          (White in RGB mode, mixed color in Dual Color mode).
        </p>
        <p>
          <strong className="text-slate-300">Spinning:</strong> POV reveals distinct bars of each component color
          separated by black gaps.
        </p>
      </div>

      {/* Mode toggle */}
      <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4 space-y-4">
        <div className="flex items-center justify-between">
          <span className="text-sm font-semibold text-slate-300">Mode</span>
          <button
            onClick={() => setRgbMode(prev => !prev)}
            className="flex items-center gap-2 px-4 py-2 rounded-lg text-sm font-bold transition-all active:scale-95 border bg-slate-800 border-slate-700 hover:bg-slate-700"
          >
            {rgbMode
              ? <><ToggleRight size={18} className="text-cyan-400" /> <span className="text-cyan-400">RGB (White)</span></>
              : <><ToggleLeft size={18} className="text-orange-400" /> <span className="text-orange-400">Dual Color</span></>
            }
          </button>
        </div>

        {rgbMode ? (
          <p className="text-[10px] text-slate-500">
            Cycle: <span className="text-red-400">Red</span> → Black →{' '}
            <span className="text-green-400">Green</span> → Black →{' '}
            <span className="text-blue-400">Blue</span> → Black
          </p>
        ) : (
          <p className="text-[10px] text-slate-500">
            Cycle: <span style={{ color: colorA }}>Color A</span> → Black →{' '}
            <span style={{ color: colorB }}>Color B</span> → Black
          </p>
        )}
      </div>

      {/* Color pickers — Dual Color mode only */}
      {!rgbMode && (
        <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4 space-y-4">
          <div className="text-[9px] font-black text-slate-500 uppercase tracking-widest">Custom Colors</div>
          <div className="grid grid-cols-2 gap-4">
            <label className="space-y-1.5">
              <span className="text-xs text-slate-400">Color A</span>
              <div className="flex items-center gap-2">
                <input
                  type="color"
                  value={colorA}
                  onChange={e => setColorA(e.target.value)}
                  className="w-10 h-10 rounded-lg border border-slate-700 bg-transparent cursor-pointer"
                />
                <span className="text-[10px] text-slate-500 font-mono">{colorA}</span>
              </div>
            </label>
            <label className="space-y-1.5">
              <span className="text-xs text-slate-400">Color B</span>
              <div className="flex items-center gap-2">
                <input
                  type="color"
                  value={colorB}
                  onChange={e => setColorB(e.target.value)}
                  className="w-10 h-10 rounded-lg border border-slate-700 bg-transparent cursor-pointer"
                />
                <span className="text-[10px] text-slate-500 font-mono">{colorB}</span>
              </div>
            </label>
          </div>
        </div>
      )}

      {/* Timing / bar width slider */}
      <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4 space-y-3">
        <div className="flex items-center justify-between">
          <span className="text-[9px] font-black text-slate-500 uppercase tracking-widest">Bar Width</span>
          <span className="text-[10px] text-cyan-400 font-mono">
            {strobeMicros} μs &middot; {showRateHz.toLocaleString()} Hz
          </span>
        </div>
        <input
          type="range"
          min={0}
          max={127}
          value={timingIndex}
          onChange={e => setTimingIndex(Number(e.target.value))}
          className="w-full accent-cyan-500"
        />
        <div className="flex justify-between text-[9px] text-slate-600">
          <span>Thin (100 μs)</span>
          <span>Default (300 μs)</span>
          <span>Wide (735 μs)</span>
        </div>
      </div>

      {/* Activate button */}
      <button
        onClick={activate}
        disabled={sending}
        className={`w-full py-3.5 rounded-xl text-sm font-bold transition-all active:scale-[0.98] border ${
          sending
            ? 'bg-slate-700 border-slate-600 text-slate-400 cursor-wait'
            : 'bg-orange-600 hover:bg-orange-500 border-orange-400/30 text-white'
        }`}
      >
        {sending ? 'Sending…' : '⚡ Activate Retro Strobe'}
      </button>

      {/* Status */}
      {status && (
        <div className={`text-center text-xs font-mono ${status.startsWith('Error') ? 'text-red-400' : 'text-green-400'}`}>
          {status}
        </div>
      )}
    </div>
  );
};

export default RetroStrobe;
