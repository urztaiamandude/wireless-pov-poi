import React, { useCallback, useEffect, useRef, useState } from 'react';
import { Eraser } from 'lucide-react';
import { useDebounce } from '../hooks';
import { getDeviceBase } from '../deviceAddress';

interface RgbPixel {
  r: number;
  g: number;
  b: number;
}

const DEFAULT_DISPLAY_LEDS = 32;
const BLACK: RgbPixel = { r: 0, g: 0, b: 0 };

function hexToRgb(hex: string): RgbPixel {
  let h = hex.replace('#', '').toLowerCase();
  if (h.length === 3) {
    h = h[0] + h[0] + h[1] + h[1] + h[2] + h[2];
  }
  h = h.padEnd(6, '0').substring(0, 6);
  return {
    r: parseInt(h.substring(0, 2), 16) || 0,
    g: parseInt(h.substring(2, 4), 16) || 0,
    b: parseInt(h.substring(4, 6), 16) || 0,
  };
}

function rgbCss(pixel: RgbPixel): string {
  return `rgb(${pixel.r}, ${pixel.g}, ${pixel.b})`;
}

function blankStrip(count: number): RgbPixel[] {
  return Array.from({ length: count }, () => ({ ...BLACK }));
}

/**
 * Paints the strip and posts it to POST /api/live.
 * The Teensy shows liveBuffer only while display mode is 4 (Live).
 * Pixel 0 is the first display LED, which is the bottom of the image.
 */
const LiveDraw: React.FC = () => {
  const [ledCount, setLedCount] = useState(DEFAULT_DISPLAY_LEDS);
  const [color, setColor] = useState('#ff44aa');
  const [pixels, setPixels] = useState<RgbPixel[]>(() => blankStrip(DEFAULT_DISPLAY_LEDS));
  const [status, setStatus] = useState<string | null>(null);
  const colorRef = useRef(color);
  const pixelsRef = useRef(pixels);
  const paintingRef = useRef(false);

  useEffect(() => {
    colorRef.current = color;
  }, [color]);

  useEffect(() => {
    pixelsRef.current = pixels;
  }, [pixels]);

  const postFrame = useCallback(async (frame: RgbPixel[]) => {
    try {
      const res = await fetch(`${getDeviceBase()}/api/live`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pixels: frame }),
        signal: AbortSignal.timeout(1500),
      });
      if (res.ok) {
        setStatus('Frame sent to /api/live.');
      } else {
        setStatus(`Live frame rejected (HTTP ${res.status}).`);
      }
    } catch {
      setStatus('Could not reach the poi. Live frames use POST /api/live.');
    }
  }, []);

  const debouncedPost = useDebounce(postFrame, 40);

  useEffect(() => {
    let cancelled = false;
    fetch(`${getDeviceBase()}/api/hardware/leds`, { signal: AbortSignal.timeout(3000) })
      .then(res => (res.ok ? res.json() : Promise.reject(res.status)))
      .then(data => {
        if (cancelled) return;
        const count = Number(data.displayLeds);
        if (Number.isInteger(count) && count >= 1 && count <= DEFAULT_DISPLAY_LEDS) {
          setLedCount(count);
          setPixels(blankStrip(count));
        }
      })
      .catch(() => {
        /* Offline preview still paints a 32-LED strip. */
      });
    return () => {
      cancelled = true;
    };
  }, []);

  const paintIndex = (index: number) => {
    if (index < 0 || index >= pixelsRef.current.length) return;
    const rgb = hexToRgb(colorRef.current);
    const next = pixelsRef.current.map((pixel, i) => (i === index ? rgb : pixel));
    pixelsRef.current = next;
    setPixels(next);
    debouncedPost(next);
  };

  const clearStrip = () => {
    const next = blankStrip(ledCount);
    pixelsRef.current = next;
    setPixels(next);
    void postFrame(next);
  };

  return (
    <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4 space-y-3">
      <div className="flex items-center justify-between gap-3">
        <div>
          <div className="text-[9px] font-black text-slate-500 uppercase tracking-widest">Live draw</div>
          <p className="text-[10px] text-slate-500 mt-1">
            Paints the display LEDs through POST /api/live. The left cell is the bottom of the image.
          </p>
        </div>
        <label className="flex items-center gap-2 text-[10px] text-slate-400">
          Color
          <input
            type="color"
            value={color}
            onChange={e => setColor(e.target.value)}
            className="w-10 h-8 rounded-lg cursor-pointer border border-slate-700 bg-transparent"
            aria-label="Live draw color"
          />
        </label>
      </div>
      <div
        className="flex flex-nowrap gap-1 overflow-x-auto touch-none pb-1"
        onPointerLeave={() => { paintingRef.current = false; }}
        onPointerUp={() => { paintingRef.current = false; }}
      >
        {pixels.map((pixel, index) => (
          <button
            key={index}
            type="button"
            aria-label={`LED ${index}`}
            className="w-5 h-10 shrink-0 rounded border border-slate-700"
            style={{ backgroundColor: rgbCss(pixel) }}
            onPointerDown={event => {
              event.preventDefault();
              paintingRef.current = true;
              paintIndex(index);
            }}
            onPointerEnter={() => {
              if (paintingRef.current) paintIndex(index);
            }}
          />
        ))}
      </div>
      <div className="flex items-center justify-between gap-3">
        <button
          type="button"
          onClick={clearStrip}
          className="px-3 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-[10px] font-bold border border-slate-700 flex items-center gap-1.5"
        >
          <Eraser size={12} /> Clear
        </button>
        <span className="text-[10px] text-slate-500 font-mono">{ledCount} LEDs{status ? ` · ${status}` : ''}</span>
      </div>
    </div>
  );
};

export default LiveDraw;
