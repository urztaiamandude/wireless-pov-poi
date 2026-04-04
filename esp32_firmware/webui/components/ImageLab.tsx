
import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
  Upload, ImageIcon, RefreshCw, Sparkles,
  Sliders, Activity, Palette, Box,
  Plus, Trash2, ListOrdered, Play, Pause, SkipForward, SkipBack, Clock,
  ChevronLeft, Users, Zap, Camera
} from 'lucide-react';
import { SequenceItem } from '../types';
import { useDebounce } from '../hooks';
import { isNativePlatform, takePhoto, pickPhoto, hapticImpact } from '../nativeFeatures';

// Pattern definitions — must match Teensy firmware pattern IDs
const PATTERN_LIST = [
  { id: 0,  label: 'Rainbow' },
  { id: 1,  label: 'Wave' },
  { id: 2,  label: 'Gradient' },
  { id: 3,  label: 'Sparkle' },
  { id: 4,  label: 'Fire' },
  { id: 5,  label: 'Comet' },
  { id: 6,  label: 'Breathing' },
  { id: 7,  label: 'Strobe' },
  { id: 8,  label: 'Meteor' },
  { id: 9,  label: 'Wipe' },
  { id: 10, label: 'Plasma' },
  { id: 11, label: 'VU Meter' },
  { id: 12, label: 'Pulse' },
  { id: 13, label: 'Audio Rainbow' },
  { id: 14, label: 'Center Burst' },
  { id: 15, label: 'Audio Sparkle' },
  { id: 16, label: 'Split Spin' },
  { id: 17, label: 'Theater Chase' },
  { id: 18, label: 'Retro Strobe' },
];

interface ImageLabProps {
  onPreviewUpdate: (url: string) => void;
  initialPreview: string | null;
  ledCount: number;
  setLedCount: (count: number) => void;
}

function getDeviceBase(): string {
  if (typeof window !== 'undefined' && (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1')) {
    return '';
  }
  return window.location.origin;
}

// Teensy sequence slot for user-defined sequences (slot 0 is the preloaded demo sequence)
const USER_SEQUENCE_SLOT = 1;
// First image slot available for user uploads (slots 0-4 are preloaded demo images).
// The sequence protocol limits image indices to 7 bits (0-127), so uploads wrap at 127.
const FIRST_USER_IMAGE_SLOT = 5;
const MAX_USER_IMAGE_SLOT = 127;

const ImageLab: React.FC<ImageLabProps> = ({ onPreviewUpdate, initialPreview, ledCount, setLedCount }) => {
  const [labMode, setLabMode] = useState<'upload' | 'procedural'>('upload');
  const [selectedImage, setSelectedImage] = useState<string | null>(initialPreview);
  const [isSyncing, setIsSyncing] = useState(false);
  const [status, setStatus] = useState<string | null>(null);
  const [bmpBlob, setBmpBlob] = useState<Blob | null>(null);
  const [processedDimensions, setProcessedDimensions] = useState({ w: 0, h: 0 });

  // Sequence State
  const [sequence, setSequence] = useState<SequenceItem[]>([]);
  const [activeSequenceIndex, setActiveSequenceIndex] = useState<number>(-1);
  const [isPlayingSequence, setIsPlayingSequence] = useState(false);
  const [frameDuration, setFrameDuration] = useState(2000);

  // Pattern-in-sequence state
  const [showPatternPicker, setShowPatternPicker] = useState(false);
  const [patternPickerDuration, setPatternPickerDuration] = useState(2000);

  // Procedural States
  const [patternType, setPatternType] = useState<'organic' | 'geometric'>('organic');
  const [complexity, setComplexity] = useState<number>(8);
  const [localComplexity, setLocalComplexity] = useState<number>(8); // Local state for slider
  const [colorSeed, setColorSeed] = useState<number>(Math.random());

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const playbackTimerRef = useRef<number | null>(null);
  const sequenceRef = useRef<SequenceItem[]>([]);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (event) => {
        const result = event.target?.result as string;
        setSelectedImage(result);
        onPreviewUpdate(result);
        setBmpBlob(null);
        setStatus(null);
        setLabMode('upload');
      };
      reader.readAsDataURL(file);
    }
  };

  /** Capture a photo via the native camera and load it into the editor. */
  const handleCameraCapture = useCallback(async () => {
    await hapticImpact('light');
    const photo = await takePhoto();
    if (photo) {
      setSelectedImage(photo.dataUrl);
      onPreviewUpdate(photo.dataUrl);
      setBmpBlob(null);
      setStatus('Photo captured — resize & deploy when ready');
      setLabMode('upload');
    }
  }, [onPreviewUpdate]);

  /** Pick a photo from the native gallery and load it into the editor. */
  const handleGalleryPick = useCallback(async () => {
    await hapticImpact('light');
    const photo = await pickPhoto();
    if (photo) {
      setSelectedImage(photo.dataUrl);
      onPreviewUpdate(photo.dataUrl);
      setBmpBlob(null);
      setStatus('Photo selected — resize & deploy when ready');
      setLabMode('upload');
    }
  }, [onPreviewUpdate]);

  const createBMP = (canvas: HTMLCanvasElement): Blob => {
    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    if (!ctx) throw new Error("Context failed");
    const width = canvas.width;
    const height = canvas.height;
    
    // BMP row padding: Each row must be padded to a 4-byte boundary
    // Formula: ((bitsPerPixel * width + 31) / 32) * 4
    // For 24-bit BMPs: ((24 * width + 31) / 32) * 4
    const rowSize = Math.floor((24 * width + 31) / 32) * 4;
    const pixelDataSize = rowSize * height;
    const fileSize = 54 + pixelDataSize;
    const buffer = new ArrayBuffer(fileSize);
    const view = new DataView(buffer);

    // BMP Header
    view.setUint8(0, 0x42); view.setUint8(1, 0x4D); // "BM"
    view.setUint32(2, fileSize, true);
    view.setUint32(10, 54, true); // Pixel data offset
    
    // DIB Header (BITMAPINFOHEADER)
    view.setUint32(14, 40, true); // DIB header size
    view.setInt32(18, width, true);
    view.setInt32(22, height, true);
    view.setUint16(26, 1, true); // Color planes
    view.setUint16(28, 24, true); // Bits per pixel

    // Pixel data (bottom-up, BGR format)
    const imgData = ctx.getImageData(0, 0, width, height).data;
    let offset = 54;
    for (let y = height - 1; y >= 0; y--) {
      for (let x = 0; x < width; x++) {
        const i = (y * width + x) * 4;
        view.setUint8(offset++, imgData[i + 2]); // B
        view.setUint8(offset++, imgData[i + 1]); // G
        view.setUint8(offset++, imgData[i]);     // R
      }
      // Add row padding to align to 4-byte boundary
      for (let p = 0; p < rowSize - (width * 3); p++) view.setUint8(offset++, 0);
    }
    return new Blob([buffer], { type: 'image/bmp' });
  };

  // Creates a raw RGB blob (R, G, B per pixel, top-to-bottom) suitable for the firmware's
  // /api/image endpoint. Filename must be image_WxH.rgb for dimension parsing.
  const createRawRGB = (canvas: HTMLCanvasElement): { blob: Blob; filename: string } => {
    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    if (!ctx) throw new Error("Context failed");
    const width = canvas.width;
    const height = canvas.height;
    const imgData = ctx.getImageData(0, 0, width, height).data;
    const buffer = new Uint8Array(width * height * 3);
    let offset = 0;
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const i = (y * width + x) * 4;
        buffer[offset++] = imgData[i];     // R
        buffer[offset++] = imgData[i + 1]; // G
        buffer[offset++] = imgData[i + 2]; // B
      }
    }
    return {
      blob: new Blob([buffer], { type: 'application/octet-stream' }),
      filename: `image_${width}x${height}.rgb`,
    };
  };

  const generateProceduralArt = useCallback(() => {
    if (!canvasRef.current) return;
    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const targetH = ledCount;
    const targetW = targetH * 4;
    canvas.width = targetW;
    canvas.height = targetH;
    setProcessedDimensions({ w: targetW, h: targetH });

    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, targetW, targetH);
    const hueStart = colorSeed * 360;

    if (patternType === 'organic') {
      for (let i = 0; i < complexity; i++) {
        const xOffset = Math.random() * targetW;
        const freq = 0.01 + (Math.random() * 0.04);
        const amp = (targetH / 4) + Math.random() * (targetH / 2);
        const hue = (hueStart + (i * (360 / complexity))) % 360;
        ctx.beginPath();
        ctx.strokeStyle = `hsla(${hue}, 90%, 60%, 0.7)`;
        ctx.lineWidth = 2 + Math.random() * 8;
        for (let x = 0; x <= targetW; x++) {
          const y = (targetH / 2) + Math.sin(x * freq + xOffset) * amp + Math.cos(x * freq * 0.5) * (amp * 0.3);
          if (x === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }
    } else {
      const cols = Math.max(4, Math.floor(complexity * 2));
      const cellSize = targetW / cols;
      const rows = Math.floor(targetH / cellSize) || 1;
      for (let x = 0; x < cols; x++) {
        for (let y = 0; y < rows; y++) {
          if (Math.random() > 0.4) {
            const hue = (hueStart + (Math.random() * 60)) % 360;
            ctx.fillStyle = `hsla(${hue}, 90%, 50%, 0.9)`;
            const px = x * cellSize;
            const py = y * cellSize;
            const size = cellSize * (0.5 + Math.random() * 0.4);
            const shape = Math.floor(Math.random() * 3);
            if (shape === 0) ctx.fillRect(px + (cellSize - size) / 2, py + (cellSize - size) / 2, size, size);
            else if (shape === 1) { ctx.beginPath(); ctx.arc(px + cellSize / 2, py + cellSize / 2, size / 2, 0, Math.PI * 2); ctx.fill(); }
            else { ctx.strokeStyle = ctx.fillStyle; ctx.lineWidth = 3; ctx.beginPath(); ctx.moveTo(px, py); ctx.lineTo(px + cellSize, py + cellSize); ctx.stroke(); }
          }
        }
      }
    }
    const dataUrl = canvas.toDataURL();
    setSelectedImage(dataUrl);
    onPreviewUpdate(dataUrl);
    setBmpBlob(null);
    setStatus(`Engine Ready: ${patternType.toUpperCase()}`);
  }, [ledCount, complexity, colorSeed, patternType, onPreviewUpdate]);

  // Debounced complexity update - regenerate pattern only after user stops moving slider
  const debouncedComplexityUpdate = useDebounce(
    useCallback((value: number) => {
      setComplexity(value);
    }, []),
    300 // 300ms debounce delay
  );

  const handleComplexityChange = (value: number) => {
    setLocalComplexity(value); // Update slider immediately
    debouncedComplexityUpdate(value); // Debounce the actual complexity state change
  };

  const addToSequence = () => {
    if (!selectedImage) return;
    const blob = canvasRef.current ? createBMP(canvasRef.current) : undefined;
    const newItem: SequenceItem = {
      id: crypto.randomUUID(),
      name: labMode === 'upload' ? 'Upload Frame' : `${patternType} Pattern`,
      dataUrl: selectedImage,
      blob,
      duration: frameDuration,
      kind: 'image',
    };
    setSequence(prev => [...prev, newItem]);
    setStatus("Frame added to timeline.");
  };

  const addPatternToSequence = (patternId: number) => {
    const pat = PATTERN_LIST.find(p => p.id === patternId);
    if (!pat) return;
    const newItem: SequenceItem = {
      id: crypto.randomUUID(),
      name: `Pattern: ${pat.label}`,
      dataUrl: '',           // no preview image for patterns
      duration: patternPickerDuration,
      kind: 'pattern',
      patternId,
    };
    setSequence(prev => [...prev, newItem]);
    setShowPatternPicker(false);
    setStatus(`Pattern "${pat.label}" added to timeline.`);
  };

  const removeFromSequence = (id: string) => {
    setSequence(prev => prev.filter(item => item.id !== id));
  };

  const moveItem = (index: number, direction: 'up' | 'down') => {
    const newSeq = [...sequence];
    const targetIndex = direction === 'up' ? index - 1 : index + 1;
    if (targetIndex < 0 || targetIndex >= newSeq.length) return;
    [newSeq[index], newSeq[targetIndex]] = [newSeq[targetIndex], newSeq[index]];
    setSequence(newSeq);
  };

  const togglePlayback = () => {
    if (sequence.length === 0) return;
    setIsPlayingSequence(!isPlayingSequence);
  };

  // Keep sequenceRef in sync with sequence state
  useEffect(() => {
    sequenceRef.current = sequence;
  }, [sequence]);

  // Playback timer - uses sequenceRef to avoid resetting when sequence content changes
  useEffect(() => {
    if (isPlayingSequence && sequenceRef.current.length > 0) {
      if (activeSequenceIndex === -1) setActiveSequenceIndex(0);
      const currentItem = sequenceRef.current[activeSequenceIndex === -1 ? 0 : activeSequenceIndex];
      playbackTimerRef.current = window.setTimeout(() => {
        setActiveSequenceIndex(prev => (prev + 1) % sequenceRef.current.length);
      }, currentItem.duration);
    } else {
      if (playbackTimerRef.current) clearTimeout(playbackTimerRef.current);
    }
    return () => { if (playbackTimerRef.current) clearTimeout(playbackTimerRef.current); };
  }, [isPlayingSequence, activeSequenceIndex]);

  // Update preview when active sequence index changes
  useEffect(() => {
    if (activeSequenceIndex !== -1 && sequenceRef.current[activeSequenceIndex]) {
      const item = sequenceRef.current[activeSequenceIndex];
      setSelectedImage(item.dataUrl);
      onPreviewUpdate(item.dataUrl);
    }
  }, [activeSequenceIndex, onPreviewUpdate]);

  // Deploy sequence/image to hardware via POST /api/image (for images) + POST /api/sequence
  const handleFleetSync = async () => {
    if (sequence.length === 0 && !selectedImage) return;
    setIsSyncing(true);
    const base = getDeviceBase();

    // Single-image mode (no sequence built): just upload and display
    if (sequence.length === 0 && selectedImage) {
      setStatus('Uploading image to hardware...');
      try {
        const blob = bmpBlob || (canvasRef.current ? createBMP(canvasRef.current) : null);
        if (!blob) { setStatus('No image data to upload.'); return; }
        const { blob: rgbBlob, filename } = createRawRGB(canvasRef.current!);
        const formData = new FormData();
        formData.append('file', rgbBlob, filename);
        const res = await fetch(`${base}/api/image`, { method: 'POST', body: formData });
        if (res.ok) {
          setStatus('Image uploaded and displaying on hardware.');
        } else {
          setStatus('Image upload failed. Check device connection.');
        }
      } catch {
        setStatus('Upload error. Check POV-POI-WiFi connectivity.');
      } finally {
        setIsSyncing(false);
      }
      return;
    }

    // Multi-item sequence mode: upload images, then push sequence definition
    setStatus(`Uploading ${sequence.length} items to hardware...`);
    try {
      // Build sequence items with assigned slots
      interface HardwareItem {
        kind: 'image' | 'pattern';
        index: number;  // image slot or pattern id
        duration: number;
      }
      const hwItems: HardwareItem[] = [];
      let uploadErrors = 0;

      for (const item of sequence) {
        if (item.kind === 'pattern') {
          // Pattern item — no upload needed, reference by patternId directly
          hwItems.push({
            kind: 'pattern',
            index: item.patternId ?? 0,
            duration: item.duration,
          });
          continue;
        }

        // Image item — must have blob or dataUrl; skip with error if neither is present
        if (!item.blob && !item.dataUrl) {
          console.warn('[ImageLab] Image item missing blob and dataUrl; skipping', item.name);
          uploadErrors++;
          continue;
        }

        // Image item — upload to device and get assigned slot
        try {
          let rgbBlob: Blob;
          let filename: string;

          if (item.blob) {
            // Re-draw stored blob to an offscreen canvas for raw RGB export
            const offscreen = document.createElement('canvas');
            const url = URL.createObjectURL(item.blob);
            await new Promise<void>(resolve => {
              const img = new window.Image();
              img.onload = () => {
                offscreen.width = img.naturalWidth;
                offscreen.height = img.naturalHeight;
                const ctx = offscreen.getContext('2d');
                ctx?.drawImage(img, 0, 0);
                URL.revokeObjectURL(url);
                resolve();
              };
              img.onerror = () => { URL.revokeObjectURL(url); resolve(); };
              img.src = url;
            });
            const raw = createRawRGB(offscreen);
            rgbBlob = raw.blob;
            filename = raw.filename;
          } else if (canvasRef.current) {
            const raw = createRawRGB(canvasRef.current);
            rgbBlob = raw.blob;
            filename = raw.filename;
          } else {
            uploadErrors++;
            continue;
          }

          const formData = new FormData();
          formData.append('file', rgbBlob, filename);
          const res = await fetch(`${base}/api/image`, { method: 'POST', body: formData });
          if (res.ok) {
            const json = await res.json() as { slot?: number };
            if (typeof json.slot !== 'number') {
              console.warn('[ImageLab] /api/image response missing slot; using fallback', FIRST_USER_IMAGE_SLOT);
            }
            // Clamp to MAX_USER_IMAGE_SLOT (127) — the sequence protocol only has 7 bits for image index
            const rawSlot = typeof json.slot === 'number' ? json.slot : FIRST_USER_IMAGE_SLOT;
            const assignedSlot = Math.min(rawSlot, MAX_USER_IMAGE_SLOT);
            hwItems.push({ kind: 'image', index: assignedSlot, duration: item.duration });
          } else {
            uploadErrors++;
          }
        } catch {
          uploadErrors++;
        }
      }

      if (hwItems.length === 0) {
        setStatus('No items to sequence. Check uploads.');
        return;
      }

      // Push sequence definition to Teensy via /api/sequence
      const seqBody = {
        index: USER_SEQUENCE_SLOT,   // slot 1 = user sequence (slot 0 = preloaded demo)
        loop: true,
        items: hwItems,
      };
      const seqRes = await fetch(`${base}/api/sequence`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(seqBody),
      });

      if (seqRes.ok) {
        // Start sequence playback (mode=3, user sequence slot)
        await fetch(`${base}/api/mode`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode: 3, index: USER_SEQUENCE_SLOT }),
        });
        const errMsg = uploadErrors > 0 ? ` (${uploadErrors} upload error(s))` : '';
        setStatus(`Sequence deployed to hardware (${hwItems.length} items).${errMsg}`);
      } else {
        setStatus('Sequence push failed. Check device connection.');
      }
    } catch {
      setStatus('Deployment error. Check POV-POI-WiFi connectivity.');
    } finally {
      setIsSyncing(false);
    }
  };

  const setAndGenerate = (type: 'organic' | 'geometric') => {
    setPatternType(type);
    setTimeout(generateProceduralArt, 0);
  };

  useEffect(() => {
    if (labMode === 'upload' && selectedImage && canvasRef.current) {
      const canvas = canvasRef.current;
      const ctx = canvas.getContext('2d');
      const img = new Image();
      img.onload = () => {
        const targetH = ledCount;
        const ratio = img.width / img.height;
        const targetW = Math.max(1, Math.round(targetH * ratio));
        canvas.width = targetW;
        canvas.height = targetH;
        setProcessedDimensions({ w: targetW, h: targetH });
        ctx?.clearRect(0, 0, canvas.width, canvas.height);
        ctx?.drawImage(img, 0, 0, targetW, targetH);
        onPreviewUpdate(canvas.toDataURL());
      };
      img.src = selectedImage;
    }
  }, [selectedImage, ledCount, labMode, onPreviewUpdate]);

  return (
    <div className="space-y-6 animate-fadeIn pb-12">
      <header className="flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <h2 className="text-3xl font-bold text-white flex items-center gap-3">
            <ImageIcon className="text-pink-500" /> POV Image Lab
          </h2>
          <p className="text-slate-400 font-medium">Design and sequence hardware-ready visuals.</p>
        </div>
        <div className="flex bg-slate-900 border border-slate-800 rounded-2xl p-1 shadow-inner">
          <button
            onClick={() => setLabMode('upload')}
            className={`px-6 py-2 rounded-xl text-[10px] font-black uppercase tracking-widest transition-all flex items-center gap-2 ${labMode === 'upload' ? 'bg-slate-800 text-white' : 'text-slate-500 hover:text-slate-400'}`}
          >
            <Upload size={14} /> Upload
          </button>
          <button
            onClick={() => { setLabMode('procedural'); generateProceduralArt(); }}
            className={`px-6 py-2 rounded-xl text-[10px] font-black uppercase tracking-widest transition-all flex items-center gap-2 ${labMode === 'procedural' ? 'bg-slate-800 text-white' : 'text-slate-500 hover:text-slate-400'}`}
          >
            <Sparkles size={14} className="text-pink-400" /> Procedural
          </button>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left Toolbar */}
        <div className="lg:col-span-4 space-y-4">
          <div className="bg-slate-900 border border-slate-800 rounded-3xl p-5 shadow-xl space-y-6">
            <h3 className="text-[10px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
              <Sliders size={14} className="text-cyan-400" /> Parameters
            </h3>

            <div className="space-y-2">
              <label className="text-[10px] font-black text-slate-400 uppercase tracking-widest px-1">Global Height (LEDs)</label>
              <input
                type="number"
                value={ledCount}
                onChange={(e) => setLedCount(parseInt(e.target.value) || 1)}
                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-4 py-3 text-white font-mono outline-none focus:ring-1 focus:ring-pink-500/50 transition-all"
              />
            </div>

            {labMode === 'procedural' ? (
              <div className="space-y-6 animate-fadeIn">
                <div className="grid grid-cols-2 gap-2">
                  <button onClick={() => setAndGenerate('organic')} className={`py-4 rounded-2xl text-xs font-bold border transition-all flex flex-col items-center gap-2 ${patternType === 'organic' ? 'bg-cyan-500/10 border-cyan-500 text-cyan-400' : 'bg-slate-800/50 border-slate-700 text-slate-500'}`}><Activity size={18} /> Organic</button>
                  <button onClick={() => setAndGenerate('geometric')} className={`py-4 rounded-2xl text-xs font-bold border transition-all flex flex-col items-center gap-2 ${patternType === 'geometric' ? 'bg-purple-500/10 border-purple-500 text-purple-400' : 'bg-slate-800/50 border-slate-700 text-slate-500'}`}><Box size={18} /> Geometric</button>
                </div>
                <div className="space-y-3">
                  <div className="flex justify-between text-[10px] font-black text-slate-400 uppercase tracking-widest">Complexity <span>{localComplexity}</span></div>
                  <input type="range" min="1" max="25" value={localComplexity} onChange={(e) => handleComplexityChange(parseInt(e.target.value))} className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-pink-500" />
                </div>
                <button onClick={() => { setColorSeed(Math.random()); setTimeout(generateProceduralArt, 0); }} className="w-full py-4 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-[10px] font-black flex items-center justify-center gap-2 uppercase tracking-widest transition-all"><Palette size={16} className="text-pink-500" /> Roll Colors</button>
              </div>
            ) : (
              <div className="space-y-3">
                <button onClick={() => fileInputRef.current?.click()} className="w-full py-10 border-2 border-dashed border-slate-800 rounded-3xl flex flex-col items-center gap-3 hover:border-pink-500/50 hover:bg-pink-500/5 transition-all group">
                  <Upload className="text-slate-600 group-hover:text-pink-500" size={24} />
                  <span className="block text-[10px] font-black text-slate-400 uppercase tracking-widest">Import Image</span>
                  <input type="file" ref={fileInputRef} onChange={handleFileChange} className="hidden" />
                </button>
                {isNativePlatform() && (
                  <div className="grid grid-cols-2 gap-2">
                    <button onClick={handleCameraCapture} className="py-4 bg-pink-900/30 hover:bg-pink-800/40 border border-pink-700/40 rounded-2xl text-xs font-bold text-pink-300 flex flex-col items-center gap-2 transition-all active:scale-95">
                      <Camera size={18} /> Take Photo
                    </button>
                    <button onClick={handleGalleryPick} className="py-4 bg-cyan-900/30 hover:bg-cyan-800/40 border border-cyan-700/40 rounded-2xl text-xs font-bold text-cyan-300 flex flex-col items-center gap-2 transition-all active:scale-95">
                      <ImageIcon size={18} /> Gallery
                    </button>
                  </div>
                )}
              </div>
            )}

            <div className="pt-4 border-t border-slate-800 space-y-2">
              <button
                onClick={addToSequence}
                disabled={!selectedImage}
                className="w-full py-5 bg-pink-600 hover:bg-pink-500 disabled:opacity-40 text-white rounded-2xl font-black text-xs uppercase tracking-widest shadow-lg shadow-pink-900/20 active:scale-95 transition-all flex items-center justify-center gap-2"
              >
                <Plus size={18} /> Add Image To Sequence
              </button>
              <button
                onClick={() => setShowPatternPicker(p => !p)}
                className="w-full py-3 bg-purple-900/40 hover:bg-purple-800/60 border border-purple-700/40 text-purple-300 rounded-2xl font-black text-xs uppercase tracking-widest active:scale-95 transition-all flex items-center justify-center gap-2"
              >
                <Zap size={14} /> Add Pattern To Sequence
              </button>
              {showPatternPicker && (
                <div className="bg-slate-950 border border-purple-700/40 rounded-2xl p-3 space-y-2">
                  <div className="flex items-center gap-2">
                    <label className="text-[9px] font-black uppercase tracking-widest text-slate-500 flex-shrink-0">Duration</label>
                    <input type="number" min={100} step={100} value={patternPickerDuration}
                      onChange={e => setPatternPickerDuration(Math.max(100, parseInt(e.target.value) || 100))}
                      className="w-20 bg-slate-900 border border-slate-700 rounded-lg px-2 py-1 text-white font-mono text-xs outline-none" />
                    <span className="text-[9px] text-slate-500">ms</span>
                  </div>
                  <div className="grid grid-cols-2 gap-1 max-h-48 overflow-y-auto custom-scrollbar">
                    {PATTERN_LIST.map(p => (
                      <button key={p.id} onClick={() => addPatternToSequence(p.id)}
                        className="py-2 px-2 bg-slate-800 hover:bg-purple-900/40 border border-slate-700 hover:border-purple-500/50 text-slate-300 hover:text-purple-300 rounded-lg text-[10px] font-bold text-left transition-all">
                        {p.label}
                      </button>
                    ))}
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* Sequence Navigator */}
          <div className="bg-slate-900 border border-slate-800 rounded-3xl p-5 shadow-xl space-y-4">
            <h3 className="text-[10px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
              <ListOrdered size={14} className="text-amber-500" /> Timeline Editor ({sequence.length})
            </h3>

            <div className="space-y-2 max-h-[300px] overflow-y-auto custom-scrollbar pr-1">
              {sequence.length === 0 ? (
                <div className="py-8 text-center text-[10px] text-slate-600 font-black uppercase tracking-widest opacity-30 border border-dashed border-slate-800 rounded-xl">Sequence Empty</div>
              ) : (
                sequence.map((item, idx) => (
                  <div key={item.id} className={`flex items-center gap-3 p-2 rounded-xl border transition-all ${activeSequenceIndex === idx ? 'bg-slate-800 border-cyan-500/50' : 'bg-slate-950/50 border-slate-800'}`}>
                    {item.kind === 'pattern' ? (
                      <div className="w-10 h-10 rounded border border-purple-700/40 bg-purple-900/20 flex items-center justify-center flex-shrink-0">
                        <Zap size={16} className="text-purple-400" />
                      </div>
                    ) : (
                      item.dataUrl ? (
                        <img src={item.dataUrl} className="w-10 h-10 rounded border border-slate-800 object-cover flex-shrink-0" alt="" />
                      ) : (
                        <div className="w-10 h-10 rounded border border-slate-800 bg-slate-900 flex-shrink-0" />
                      )
                    )}
                    <div className="flex-1 min-w-0">
                      <div className="text-[10px] font-bold text-slate-300 truncate">{item.name}</div>
                      <div className="text-[8px] text-slate-500 font-mono">{item.duration}ms</div>
                    </div>
                    <div className="flex gap-1">
                      <button onClick={() => moveItem(idx, 'up')} className="p-1 text-slate-500 hover:text-white"><ChevronLeft size={14} className="rotate-90" /></button>
                      <button onClick={() => removeFromSequence(item.id)} className="p-1 text-slate-500 hover:text-red-500"><Trash2 size={14} /></button>
                    </div>
                  </div>
                ))
              )}
            </div>
          </div>
        </div>

        {/* Main Workspace */}
        <div className="lg:col-span-8 space-y-6">
          <div className="bg-slate-900 border border-slate-800 rounded-[2.5rem] p-6 lg:p-8 flex flex-col h-full shadow-2xl relative overflow-hidden">
            <div className="flex items-center justify-between mb-6">
              <h3 className="text-white font-black uppercase tracking-tighter text-xl flex items-center gap-3">
                <RefreshCw size={24} className="text-purple-400" />
                {isPlayingSequence ? 'PLAYING SEQUENCE' : 'FRAME PREVIEW'}
              </h3>
              <div className="flex items-center gap-3 bg-black/40 px-4 py-2 rounded-full border border-slate-800 shadow-inner">
                <Clock size={14} className="text-pink-400" />
                <input
                  type="number" step="100" min="100" value={frameDuration}
                  onChange={(e) => {
                    const raw = e.target.value;
                    if (raw === '') {
                      setFrameDuration(100);
                      return;
                    }
                    const v = parseInt(raw, 10);
                    if (isNaN(v)) {
                      setFrameDuration(100);
                    } else {
                      setFrameDuration(Math.max(100, v));
                    }
                  }}
                  className="bg-transparent text-[10px] font-mono text-cyan-400 outline-none w-12 text-center"
                />
                <span className="text-[8px] font-black text-slate-500 uppercase">ms/frame</span>
              </div>
            </div>

            <div className="flex-1 bg-black rounded-[2rem] border border-slate-800 flex items-center justify-center p-4 min-h-[350px] relative overflow-hidden group shadow-inner">
              <canvas ref={canvasRef} className="max-w-full shadow-2xl rounded-sm border border-slate-900 image-pixelated transition-all duration-300" />
              {sequence.length > 0 && (
                <div className="absolute bottom-4 left-1/2 -translate-x-1/2 flex items-center gap-4 bg-slate-900/80 backdrop-blur border border-slate-700 p-3 rounded-2xl shadow-2xl">
                  <button onClick={() => setActiveSequenceIndex(prev => (prev - 1 + sequence.length) % sequence.length)} className="p-2 text-slate-400 hover:text-white transition-all"><SkipBack size={20} /></button>
                  <button
                    onClick={togglePlayback}
                    className="w-12 h-12 bg-cyan-600 hover:bg-cyan-500 text-white rounded-full flex items-center justify-center shadow-lg transition-all active:scale-90"
                  >
                    {isPlayingSequence ? <Pause size={24} fill="currentColor" /> : <Play size={24} fill="currentColor" className="translate-x-0.5" />}
                  </button>
                  <button onClick={() => setActiveSequenceIndex(prev => (prev + 1) % sequence.length)} className="p-2 text-slate-400 hover:text-white transition-all"><SkipForward size={20} /></button>
                </div>
              )}
            </div>

            <div className="mt-6 grid grid-cols-1 md:grid-cols-2 gap-4">
              <div className="bg-slate-950/40 p-4 rounded-2xl border border-slate-800 flex items-center gap-4">
                <div className="w-10 h-10 rounded-full bg-cyan-500/10 flex items-center justify-center"><Activity size={18} className="text-cyan-400" /></div>
                <div>
                  <div className="text-[8px] font-black text-slate-500 uppercase tracking-widest">Dimensions</div>
                  <div className="text-[10px] font-mono text-slate-300">{processedDimensions.w}W x {processedDimensions.h}H px</div>
                </div>
              </div>

              <button
                onClick={handleFleetSync}
                disabled={isSyncing || (sequence.length === 0 && !selectedImage)}
                className="flex items-center justify-center gap-3 py-6 bg-purple-600 hover:bg-purple-500 text-white rounded-[1.5rem] font-black text-xs uppercase tracking-widest shadow-xl shadow-purple-900/30 transition-all disabled:opacity-30 active:scale-95 border border-purple-400/20"
              >
                {isSyncing ? <RefreshCw className="animate-spin" size={18} /> : <Users size={18} />}
                {sequence.length > 0 ? `Deploy Sequence (${sequence.length} items)` : 'Upload & Display'}
              </button>
            </div>

            {status && (
              <div className="mt-4 flex items-center justify-center gap-2">
                <div className="w-1.5 h-1.5 rounded-full bg-cyan-500 animate-pulse" />
                <p className="text-[10px] font-black text-cyan-400 uppercase tracking-[0.1em] text-center">{status}</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};

export default ImageLab;
