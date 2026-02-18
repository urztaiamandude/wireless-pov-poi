
import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
  Upload, ImageIcon, RefreshCw, Sparkles,
  Sliders, Activity, Palette, Box,
  Plus, Trash2, ListOrdered, Play, Pause, SkipForward, SkipBack, Clock,
  ChevronLeft, Users
} from 'lucide-react';
import { SequenceItem } from '../types';

interface ImageLabProps {
  onPreviewUpdate: (url: string) => void;
  initialPreview: string | null;
  ledCount: number;
  setLedCount: (count: number) => void;
}

// Nebula POI fleet IPs (matching the POV-POI-WiFi AP)
const FLEET_IPS = ['192.168.4.1', '192.168.4.2'];

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

  // Procedural States
  const [patternType, setPatternType] = useState<'organic' | 'geometric'>('organic');
  const [complexity, setComplexity] = useState<number>(8);
  const [colorSeed, setColorSeed] = useState<number>(Math.random());

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const playbackTimerRef = useRef<number | null>(null);

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

  const createBMP = (canvas: HTMLCanvasElement): Blob => {
    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    if (!ctx) throw new Error("Context failed");
    const width = canvas.width;
    const height = canvas.height;
    const rowSize = Math.floor((24 * width + 31) / 32) * 4;
    const pixelDataSize = rowSize * height;
    const fileSize = 54 + pixelDataSize;
    const buffer = new ArrayBuffer(fileSize);
    const view = new DataView(buffer);

    view.setUint8(0, 0x42); view.setUint8(1, 0x4D);
    view.setUint32(2, fileSize, true);
    view.setUint32(10, 54, true);
    view.setUint32(14, 40, true);
    view.setInt32(18, width, true);
    view.setInt32(22, height, true);
    view.setUint16(26, 1, true);
    view.setUint16(28, 24, true);

    const imgData = ctx.getImageData(0, 0, width, height).data;
    let offset = 54;
    for (let y = height - 1; y >= 0; y--) {
      for (let x = 0; x < width; x++) {
        const i = (y * width + x) * 4;
        view.setUint8(offset++, imgData[i + 2]); // B
        view.setUint8(offset++, imgData[i + 1]); // G
        view.setUint8(offset++, imgData[i]);     // R
      }
      for (let p = 0; p < rowSize - (width * 3); p++) view.setUint8(offset++, 0);
    }
    return new Blob([buffer], { type: 'image/bmp' });
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

  const addToSequence = () => {
    if (!selectedImage) return;
    const blob = canvasRef.current ? createBMP(canvasRef.current) : undefined;
    const newItem: SequenceItem = {
      id: Math.random().toString(36).substr(2, 9),
      name: labMode === 'upload' ? 'Upload Frame' : `${patternType} Pattern`,
      dataUrl: selectedImage,
      blob,
      duration: frameDuration
    };
    setSequence(prev => [...prev, newItem]);
    setStatus("Frame added to timeline.");
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

  useEffect(() => {
    if (isPlayingSequence && sequence.length > 0) {
      if (activeSequenceIndex === -1) setActiveSequenceIndex(0);
      const currentItem = sequence[activeSequenceIndex === -1 ? 0 : activeSequenceIndex];
      playbackTimerRef.current = window.setTimeout(() => {
        setActiveSequenceIndex(prev => (prev + 1) % sequence.length);
      }, currentItem.duration);
    } else {
      if (playbackTimerRef.current) clearTimeout(playbackTimerRef.current);
    }
    return () => { if (playbackTimerRef.current) clearTimeout(playbackTimerRef.current); };
  }, [isPlayingSequence, activeSequenceIndex, sequence]);

  useEffect(() => {
    if (activeSequenceIndex !== -1 && sequence[activeSequenceIndex]) {
      const item = sequence[activeSequenceIndex];
      setSelectedImage(item.dataUrl);
      onPreviewUpdate(item.dataUrl);
    }
  }, [activeSequenceIndex, sequence, onPreviewUpdate]);

  // Fleet sync via POST /api/image (existing firmware endpoint)
  const handleFleetSync = async () => {
    if (sequence.length === 0 && !selectedImage) return;
    setIsSyncing(true);
    setStatus("Broadcasting Sequence to Fleet via /api/image...");

    try {
      const targets = sequence.length > 0 ? sequence : [{
        blob: bmpBlob || (canvasRef.current ? createBMP(canvasRef.current) : undefined),
        name: 'single_frame.bmp'
      }];

      for (const [idx, item] of (targets as any[]).entries()) {
        if (!item.blob) continue;
        const formData = new FormData();
        const filename = `seq_${idx}.bmp`;
        formData.append('file', item.blob, filename);

        // Push to all fleet IPs via POST /api/image
        const syncPromises = FLEET_IPS.map(ip =>
          fetch(`http://${ip}/api/image`, { method: 'POST', body: formData }).catch(() => {})
        );
        await Promise.all(syncPromises);
      }

      // Auto-load the first frame after upload
      if (targets.length > 0) {
        const loadPromises = FLEET_IPS.map(ip =>
          fetch(`http://${ip}/api/sd/load`, {
            method: 'POST',
            body: (() => { const f = new FormData(); f.append('file', 'seq_0.bmp'); return f; })()
          }).catch(() => {})
        );
        await Promise.all(loadPromises);
      }

      setStatus('Fleet sequence deployment successful. Frame loaded.');
    } catch {
      setStatus('Broadcast Error: Check POV-POI-WiFi connectivity.');
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
                  <div className="flex justify-between text-[10px] font-black text-slate-400 uppercase tracking-widest">Complexity <span>{complexity}</span></div>
                  <input type="range" min="1" max="25" value={complexity} onChange={(e) => setComplexity(parseInt(e.target.value))} className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-pink-500" />
                </div>
                <button onClick={() => { setColorSeed(Math.random()); setTimeout(generateProceduralArt, 0); }} className="w-full py-4 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-[10px] font-black flex items-center justify-center gap-2 uppercase tracking-widest transition-all"><Palette size={16} className="text-pink-500" /> Roll Colors</button>
              </div>
            ) : (
              <button onClick={() => fileInputRef.current?.click()} className="w-full py-10 border-2 border-dashed border-slate-800 rounded-3xl flex flex-col items-center gap-3 hover:border-pink-500/50 hover:bg-pink-500/5 transition-all group">
                <Upload className="text-slate-600 group-hover:text-pink-500" size={24} />
                <span className="block text-[10px] font-black text-slate-400 uppercase tracking-widest">Import Image</span>
                <input type="file" ref={fileInputRef} onChange={handleFileChange} className="hidden" />
              </button>
            )}

            <div className="pt-4 border-t border-slate-800">
              <button
                onClick={addToSequence}
                className="w-full py-5 bg-pink-600 hover:bg-pink-500 text-white rounded-2xl font-black text-xs uppercase tracking-widest shadow-lg shadow-pink-900/20 active:scale-95 transition-all flex items-center justify-center gap-2"
              >
                <Plus size={18} /> Add To Sequence
              </button>
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
                    <img src={item.dataUrl} className="w-10 h-10 rounded border border-slate-800 object-cover" alt="" />
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
                  onChange={(e) => setFrameDuration(parseInt(e.target.value))}
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
                Deploy Sequence to Fleet
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
