
import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
  Square, Sun, Smartphone,
  Wifi, Upload, Terminal, Dices, Sparkles, Monitor, Activity,
  Crown, Users, Zap, Battery, BatteryLow,
  Music, Layers, SkipBack, SkipForward, Gauge, Image
} from 'lucide-react';
import { Device, PowerMode } from '../types';
import { useDebounce } from '../hooks';

interface DashboardProps {
  previewUrl: string | null;
}

function getDeviceBase(ip: string): string {
  if (typeof window !== 'undefined' && (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1')) {
    return ''; // Vite proxy handles /api -> ESP32
  }
  return window.location.origin; // deployed on ESP32 itself
}

const DEFAULT_DEVICES: Device[] = [
  { id: 'A1B2C3', name: 'POV_LEADER', ip: '192.168.4.1', status: 'Sync Leader', type: 'success', isPlaying: false, brightness: 128 },
  { id: 'D4E5F6', name: 'POV_FOLLOWER', ip: '192.168.4.2', status: 'Awaiting Sync', type: 'info', isPlaying: false, brightness: 128 }
];

const DISPLAY_MODES = [
  { id: 0, label: 'Idle', icon: Square },
  { id: 1, label: 'Image', icon: Image },
  { id: 2, label: 'Pattern', icon: Sparkles },
  { id: 3, label: 'Sequence', icon: Layers },
  { id: 4, label: 'Live', icon: Activity },
];

const PATTERNS = [
  { id: 0,  label: 'Rainbow',       group: 'basic' },
  { id: 1,  label: 'Wave',          group: 'basic' },
  { id: 2,  label: 'Gradient',      group: 'basic' },
  { id: 3,  label: 'Sparkle',       group: 'basic' },
  { id: 4,  label: 'Fire',          group: 'basic' },
  { id: 5,  label: 'Comet',         group: 'basic' },
  { id: 6,  label: 'Breathing',     group: 'basic' },
  { id: 7,  label: 'Strobe',        group: 'basic' },
  { id: 8,  label: 'Meteor',        group: 'basic' },
  { id: 9,  label: 'Wipe',          group: 'basic' },
  { id: 10, label: 'Plasma',        group: 'basic' },
  { id: 11, label: 'VU Meter',      group: 'audio' },
  { id: 12, label: 'Pulse',         group: 'audio' },
  { id: 13, label: 'Audio Rainbow', group: 'audio' },
  { id: 14, label: 'Center Burst',  group: 'audio' },
  { id: 15, label: 'Audio Sparkle', group: 'audio' },
  { id: 16, label: 'Split Spin',    group: 'advanced' },
  { id: 17, label: 'Theater Chase', group: 'advanced' },
];

const POWER_MODES: { id: PowerMode; label: string; sub: string; icon: React.ElementType; color: string }[] = [
  { id: 'performance', label: 'Performance', sub: '240 MHz', icon: Zap,       color: 'bg-yellow-600 hover:bg-yellow-500' },
  { id: 'balanced',    label: 'Balanced',    sub: '160 MHz', icon: Gauge,     color: 'bg-blue-600   hover:bg-blue-500'   },
  { id: 'powersave',   label: 'Power Save',  sub: '80 MHz',  icon: Battery,   color: 'bg-green-700  hover:bg-green-600'  },
  { id: 'ultrasave',   label: 'Ultra Save',  sub: '80 MHz*', icon: BatteryLow,color: 'bg-slate-700  hover:bg-slate-600'  },
];

const Dashboard: React.FC<DashboardProps> = ({ previewUrl }) => {
  const [devices, setDevices] = useState<Device[]>(DEFAULT_DEVICES);
  const [selectedDeviceId, setSelectedDeviceId] = useState<string | null>(devices[0].id);
  const [isSyncMode, setIsSyncMode] = useState(true);
  const [filename, setFilename] = useState('pattern.bmp');
  const [isUploading, setIsUploading] = useState(false);
  const [isOnline, setIsOnline] = useState(false);
  const [logs, setLogs] = useState<{time: string, msg: string, color: string}[]>([]);
  const [localBrightness, setLocalBrightness] = useState<number>(128);

  // New control state
  const [currentMode, setCurrentMode] = useState<number>(0);
  const [contentIndex, setContentIndex] = useState<number>(0);
  const [currentPattern, setCurrentPattern] = useState<number>(0);
  const [localFrameRate, setLocalFrameRate] = useState<number>(60);
  const [powerMode, setPowerModeState] = useState<PowerMode>('balanced');
  const [maxContentIndex, setMaxContentIndex] = useState<number>(49);

  const fileInputRef = useRef<HTMLInputElement>(null);
  const lastBrightnessInteraction = useRef<number>(0);
  const lastFrameRateInteraction = useRef<number>(0);
  const lastModeInteraction = useRef<number>(0);

  const activeDevice = devices.find(d => d.id === selectedDeviceId) || devices[0];

  const isSyncModeRef = useRef(isSyncMode);
  const devicesRef = useRef(devices);
  const activeDeviceRef = useRef(activeDevice);

  useEffect(() => {
    isSyncModeRef.current = isSyncMode;
  }, [isSyncMode]);

  useEffect(() => {
    devicesRef.current = devices;
  }, [devices]);

  useEffect(() => {
    activeDeviceRef.current = activeDevice;
  }, [activeDevice]);

  useEffect(() => {
    setLocalBrightness(activeDevice.brightness);
  }, [activeDevice.brightness]);

  useEffect(() => {
    const checkConnection = async () => {
      try {
        const base = getDeviceBase(activeDevice.ip);
        const res = await fetch(`${base}/api/status`, { signal: AbortSignal.timeout(1500) });
        if (res.ok) {
          const data = await res.json();
          setIsOnline(true);
          const now = Date.now();
          if (typeof data.mode === 'number' && now - lastModeInteraction.current > 1000) setCurrentMode(data.mode);
          if (typeof data.brightness === 'number' && now - lastBrightnessInteraction.current > 1000) setLocalBrightness(data.brightness);
          if (typeof data.framerate === 'number' && now - lastFrameRateInteraction.current > 1000) setLocalFrameRate(data.framerate);
          if (typeof data.count === 'number' && data.count > 0) setMaxContentIndex(data.count - 1);
        } else {
          setIsOnline(false);
        }
      } catch {
        setIsOnline(false);
      }
    };
    checkConnection();
    const interval = setInterval(checkConnection, 5000);
    return () => clearInterval(interval);
  }, [activeDevice.ip]);

  const addLog = (msg: string, color: string = 'text-slate-400') => {
    const now = new Date();
    const time = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
    setLogs(prev => [{ time, msg, color }, ...prev].slice(0, 50));
  };

  const updateDevice = (id: string, updates: Partial<Device>) => {
    setDevices(prev => prev.map(d => d.id === id ? { ...d, ...updates } : d));
  };

  const debouncedBrightnessUpdate = useDebounce(
    useCallback((brightness: number) => {
      handleGlobalAction('brightness', brightness);
    }, []),
    200
  );

  const debouncedFrameRateUpdate = useDebounce(
    useCallback(async (fps: number) => {
      const targets = isSyncModeRef.current ? [devicesRef.current[0]] : [activeDeviceRef.current];
      for (const dev of targets) {
        try {
          const base = getDeviceBase(dev.ip);
          await fetch(`${base}/api/framerate`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ framerate: fps }),
          });
          addLog(`[OK] Frame rate set to ${fps} FPS on ${dev.name}`, 'text-green-400');
        } catch {
          addLog(`[Error] Failed to set frame rate on ${dev.name}`, 'text-red-400');
        }
      }
    }, []),
    300
  );

  const handleBrightnessChange = (value: number) => {
    lastBrightnessInteraction.current = Date.now();
    setLocalBrightness(value);
    updateDevice(activeDevice.id, { brightness: value });
    debouncedBrightnessUpdate(value);
  };

  const handleFrameRateChange = (value: number) => {
    lastFrameRateInteraction.current = Date.now();
    setLocalFrameRate(value);
    debouncedFrameRateUpdate(value);
  };

  const handleModeSelect = async (mode: number) => {
    lastModeInteraction.current = Date.now();
    setCurrentMode(mode);
    const targets = isSyncMode ? [devices[0]] : [activeDevice];
    for (const dev of targets) {
      try {
        const base = getDeviceBase(dev.ip);
        await fetch(`${base}/api/mode`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode, index: mode === 2 ? currentPattern : contentIndex }),
        });
        addLog(`[OK] Mode → ${DISPLAY_MODES[mode]?.label} on ${dev.name}`, 'text-cyan-400');
      } catch {
        addLog(`[Error] Mode change failed on ${dev.name}`, 'text-red-400');
      }
    }
  };

  const handlePatternSelect = async (patternId: number) => {
    setCurrentPattern(patternId);
    setCurrentMode(2);
    const targets = isSyncMode ? [devices[0]] : [activeDevice];
    for (const dev of targets) {
      try {
        const base = getDeviceBase(dev.ip);
        await fetch(`${base}/api/pattern`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ type: patternId, speed: 50 }),
        });
        await fetch(`${base}/api/mode`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode: 2, index: patternId }),
        });
        addLog(`[OK] Pattern → ${PATTERNS[patternId]?.label} on ${dev.name}`, 'text-purple-400');
      } catch {
        addLog(`[Error] Pattern change failed on ${dev.name}`, 'text-red-400');
      }
    }
  };

  const handleContentNav = async (delta: number) => {
    const next = Math.max(0, Math.min(maxContentIndex, contentIndex + delta));
    setContentIndex(next);
    const targets = isSyncMode ? [devices[0]] : [activeDevice];
    for (const dev of targets) {
      try {
        const base = getDeviceBase(dev.ip);
        await fetch(`${base}/api/mode`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode: currentMode, index: next }),
        });
        addLog(`[OK] Content index → ${next} on ${dev.name}`, 'text-cyan-400');
      } catch {
        addLog(`[Error] Content nav failed on ${dev.name}`, 'text-red-400');
      }
    }
  };

  const handlePowerMode = async (mode: PowerMode) => {
    setPowerModeState(mode);
    const targets = isSyncMode ? [devices[0]] : [activeDevice];
    for (const dev of targets) {
      try {
        const base = getDeviceBase(dev.ip);
        await fetch(`${base}/api/power/mode`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode }),
        });
        addLog(`[OK] Power → ${mode} on ${dev.name}`, 'text-yellow-400');
      } catch {
        addLog(`[Error] Power mode failed on ${dev.name}`, 'text-red-400');
      }
    }
  };

  const handleFileUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    const targets = isSyncMode ? devices : [activeDevice];
    setIsUploading(true);

    for (const target of targets) {
      addLog(`[Sync] Pushing ${file.name} to ${target.name}...`, 'text-amber-400');
      const formData = new FormData();
      formData.append('file', file, file.name);

      try {
        const base = getDeviceBase(target.ip);
        const res = await fetch(`${base}/api/image`, { method: 'POST', body: formData });
        if (res.ok) {
          updateDevice(target.id, { status: `Synced: ${file.name}`, type: 'success' });
          addLog(`[OK] ${target.name} received ${file.name}`, 'text-green-400');
        } else {
          throw new Error(`HTTP ${res.status}`);
        }
      } catch (err) {
        addLog(`[Error] Failed to sync to ${target.name}`, 'text-red-500');
        updateDevice(target.id, { status: 'Upload failed', type: 'error' });
      }
    }

    setIsUploading(false);
    setFilename(file.name);
    addLog(`[Fleet] Upload sequence complete.`, 'text-green-400');
  };

  const handleGlobalAction = async (action: 'play' | 'stop' | 'brightness' | 'load' | 'random', value?: string | number) => {
    const targets = isSyncMode ? [devices[0]] : [activeDevice];

    for (const dev of targets) {
      addLog(`[CMD] ${action.toUpperCase()} -> ${dev.name}`, 'text-cyan-400');
      try {
        let method = 'POST';

        if (action === 'play' || action === 'stop' || action === 'random') {
          let mode = 0;
          let index = 0;

          if (action === 'stop') {
            mode = 0;
            index = 0;
          } else if (action === 'play') {
            mode = 3;
            index = 0;
          } else if (action === 'random') {
            mode = 2;
            index = 0;
          }

          setCurrentMode(mode);
          const base = getDeviceBase(dev.ip);
          await fetch(`${base}/api/mode`, {
            method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode, index }),
          });
        } else if (action === 'brightness') {
          const base = getDeviceBase(dev.ip);
          await fetch(`${base}/api/brightness`, {
            method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ brightness: Number(value) })
          });
        } else if (action === 'load') {
          const base = getDeviceBase(dev.ip);
          const body = new FormData();
          body.append('file', String(value));
          await fetch(`${base}/api/sd/load`, { method, body });
        }

        if (isSyncMode && action !== 'brightness') {
          try {
            await fetch(`${getDeviceBase(dev.ip)}/api/sync/execute`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ action, val: value ?? '' })
            });
          } catch { /* best-effort */ }
        }

        if (isSyncMode) {
          devices.forEach(d => {
            if (action === 'play') updateDevice(d.id, { isPlaying: true, status: 'Synced Play' });
            else if (action === 'stop') updateDevice(d.id, { isPlaying: false, status: 'Synced Stop' });
            else if (action === 'brightness') updateDevice(d.id, { brightness: value as number });
          });
        } else {
          if (action === 'play') updateDevice(dev.id, { isPlaying: true, status: 'Solo Running' });
          else if (action === 'stop') updateDevice(dev.id, { isPlaying: false, status: 'Solo Stopped' });
        }

        addLog(`[OK] ${dev.name} acknowledged.`, 'text-green-400');
      } catch {
        addLog(`[Network] ${dev.name} connection failed.`, 'text-red-400');
        updateDevice(dev.id, { status: 'Unreachable', type: 'error' });
      }
    }
  };

  const activePowerMode = POWER_MODES.find(p => p.id === powerMode);

  return (
    <div className="space-y-6 pb-20 lg:pb-0">
      <header className="flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <h2 className="text-2xl lg:text-3xl font-bold text-white leading-none">Fleet Control</h2>
            <div className={`px-2 py-0.5 rounded-full text-[8px] font-black uppercase tracking-widest flex items-center gap-1 ${isOnline ? 'bg-green-500/10 text-green-500' : 'bg-red-500/10 text-red-500'}`}>
              <Activity size={10} className={isOnline ? 'animate-pulse' : ''} />
              {isOnline ? 'Active' : 'Offline'}
            </div>
          </div>
          <p className="text-slate-500 text-xs font-medium">Managing {devices.length} POV Units • SSID: POV-POI-WiFi</p>
        </div>
        <div className="flex bg-slate-900 border border-slate-800 rounded-xl p-1 shadow-inner">
          <button
            onClick={() => setIsSyncMode(true)}
            className={`px-4 py-2 rounded-lg text-[10px] font-black uppercase tracking-widest transition-all flex items-center gap-2 ${isSyncMode ? 'bg-purple-600 text-white shadow-lg' : 'text-slate-500'}`}
          >
            <Users size={14} /> Synchronized
          </button>
          <button
            onClick={() => setIsSyncMode(false)}
            className={`px-4 py-2 rounded-lg text-[10px] font-black uppercase tracking-widest transition-all flex items-center gap-2 ${!isSyncMode ? 'bg-slate-700 text-white shadow-lg' : 'text-slate-500'}`}
          >
            <Smartphone size={14} /> Independent
          </button>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        <div className="lg:col-span-4 space-y-4">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl p-2 space-y-1">
            {devices.map((device, idx) => (
              <button
                key={device.id}
                onClick={() => setSelectedDeviceId(device.id)}
                className={`w-full text-left p-4 rounded-xl transition-all border ${
                  selectedDeviceId === device.id
                    ? 'bg-slate-800 border-cyan-500/30'
                    : 'bg-transparent border-transparent opacity-60 grayscale'
                }`}
              >
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-3">
                    {idx === 0 ? <Crown size={14} className="text-yellow-500" /> : <Smartphone size={14} className="text-slate-500" />}
                    <span className="text-sm font-bold text-slate-200">{device.name}</span>
                  </div>
                  <div className={`w-2 h-2 rounded-full ${device.isPlaying ? 'bg-green-500 animate-pulse' : 'bg-slate-600'}`} />
                </div>
                <div className="text-[10px] mt-1 text-slate-500 font-mono">{device.ip} • {device.status}</div>
              </button>
            ))}
          </div>

          <div className="bg-black border border-slate-800 rounded-2xl overflow-hidden shadow-2xl">
            <div className="p-3 bg-slate-900/80 border-b border-slate-800 flex items-center justify-between">
              <span className="text-[8px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
                <Monitor size={12} className="text-pink-500" /> Mirror Output
              </span>
              {isSyncMode && <div className="px-1.5 py-0.5 bg-purple-500/10 text-purple-400 text-[7px] font-black rounded border border-purple-500/20 uppercase tracking-widest">Dual-Link</div>}
            </div>
            <div className="aspect-[4/1] bg-black relative">
              {previewUrl && <img src={previewUrl} alt="Preview" className="w-full h-full object-cover opacity-90 blur-[1px]" />}
              <div className="absolute inset-0 bg-gradient-to-t from-black via-transparent to-transparent opacity-60" />
            </div>
          </div>

          {/* Power Mode Card */}
          <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4 space-y-3">
            <div className="text-[9px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
              {activePowerMode && <activePowerMode.icon size={12} className="text-yellow-400" />}
              Power Mode
            </div>
            <div className="grid grid-cols-2 gap-2">
              {POWER_MODES.map(pm => (
                <button
                  key={pm.id}
                  onClick={() => handlePowerMode(pm.id)}
                  className={`p-2.5 rounded-xl text-white text-[10px] font-bold flex flex-col items-center gap-1 transition-all active:scale-95 border ${
                    powerMode === pm.id
                      ? `${pm.color} border-white/20 shadow-lg`
                      : 'bg-slate-800 border-slate-700 opacity-60'
                  }`}
                >
                  <pm.icon size={14} />
                  <span>{pm.label}</span>
                  <span className="text-[8px] opacity-70 font-mono">{pm.sub}</span>
                </button>
              ))}
            </div>
          </div>
        </div>

        <div className="lg:col-span-8 space-y-4">
          {/* Mode Selector */}
          <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4">
            <div className="text-[9px] font-black text-slate-500 uppercase tracking-widest mb-3">Display Mode</div>
            <div className="flex gap-2 flex-wrap">
              {DISPLAY_MODES.map(m => (
                <button
                  key={m.id}
                  onClick={() => handleModeSelect(m.id)}
                  className={`flex-1 min-w-[80px] py-3 px-2 rounded-xl text-[11px] font-bold flex flex-col items-center gap-1.5 transition-all active:scale-95 border ${
                    currentMode === m.id
                      ? 'bg-cyan-600 border-cyan-400/30 text-white shadow-lg shadow-cyan-900/20'
                      : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                  }`}
                >
                  <m.icon size={16} />
                  {m.label}
                </button>
              ))}
            </div>
          </div>

          {/* Content Navigation (Image / Sequence mode) */}
          {(currentMode === 1 || currentMode === 3) && (
            <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4">
              <div className="text-[9px] font-black text-slate-500 uppercase tracking-widest mb-3">
                {currentMode === 1 ? 'Image' : 'Sequence'} Navigation
              </div>
              <div className="flex items-center gap-3">
                <button
                  onClick={() => handleContentNav(-1)}
                  disabled={contentIndex === 0}
                  className="p-3 bg-slate-800 hover:bg-slate-700 disabled:opacity-30 text-white rounded-xl transition-all active:scale-95 border border-slate-700"
                >
                  <SkipBack size={18} />
                </button>
                <div className="flex-1 text-center">
                  <span className="text-slate-500 text-[9px] uppercase tracking-widest block">Index</span>
                  <span className="text-white font-mono text-xl font-bold">{contentIndex}</span>
                </div>
                <button
                  onClick={() => handleContentNav(1)}
                  disabled={contentIndex >= maxContentIndex}
                  className="p-3 bg-slate-800 hover:bg-slate-700 disabled:opacity-30 text-white rounded-xl transition-all active:scale-95 border border-slate-700"
                >
                  <SkipForward size={18} />
                </button>
              </div>
            </div>
          )}

          {/* Pattern Panel (Pattern mode) */}
          {currentMode === 2 && (
            <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-4">
              <div className="flex items-center justify-between mb-3">
                <div className="text-[9px] font-black text-slate-500 uppercase tracking-widest">Patterns</div>
                <div className="text-[9px] text-cyan-400 font-mono">{PATTERNS.find(p => p.id === currentPattern)?.label ?? '—'}</div>
              </div>

              <div className="space-y-3">
                <div>
                  <div className="text-[8px] text-slate-600 uppercase tracking-widest mb-1.5">Basic</div>
                  <div className="grid grid-cols-4 gap-1.5">
                    {PATTERNS.filter(p => p.group === 'basic').map(p => (
                      <button
                        key={p.id}
                        onClick={() => handlePatternSelect(p.id)}
                        className={`py-2 px-1 rounded-lg text-[9px] font-bold transition-all active:scale-95 border ${
                          currentPattern === p.id
                            ? 'bg-indigo-600 border-indigo-400/30 text-white'
                            : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                        }`}
                      >
                        {p.label}
                      </button>
                    ))}
                  </div>
                </div>

                <div>
                  <div className="text-[8px] text-slate-600 uppercase tracking-widest mb-1.5 flex items-center gap-1">
                    <Music size={9} /> Audio Reactive
                  </div>
                  <div className="grid grid-cols-3 gap-1.5">
                    {PATTERNS.filter(p => p.group === 'audio').map(p => (
                      <button
                        key={p.id}
                        onClick={() => handlePatternSelect(p.id)}
                        className={`py-2 px-1 rounded-lg text-[9px] font-bold transition-all active:scale-95 border ${
                          currentPattern === p.id
                            ? 'bg-pink-600 border-pink-400/30 text-white'
                            : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                        }`}
                      >
                        {p.label}
                      </button>
                    ))}
                  </div>
                </div>

                <div>
                  <div className="text-[8px] text-slate-600 uppercase tracking-widest mb-1.5">Advanced</div>
                  <div className="grid grid-cols-3 gap-1.5">
                    {PATTERNS.filter(p => p.group === 'advanced').map(p => (
                      <button
                        key={p.id}
                        onClick={() => handlePatternSelect(p.id)}
                        className={`py-2 px-1 rounded-lg text-[9px] font-bold transition-all active:scale-95 border ${
                          currentPattern === p.id
                            ? 'bg-violet-600 border-violet-400/30 text-white'
                            : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                        }`}
                      >
                        {p.label}
                      </button>
                    ))}
                  </div>
                </div>
              </div>
            </div>
          )}

          <div className="bg-slate-900/60 border border-slate-800 rounded-3xl p-6 lg:p-8 backdrop-blur-xl">
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div className="space-y-4">
                <div className="flex gap-3">
                  <button
                    onClick={() => handleGlobalAction('play')}
                    className="flex-1 py-8 bg-green-600 hover:bg-green-500 text-white rounded-2xl font-black text-xl shadow-xl shadow-green-900/20 active:scale-95 transition-all"
                  >
                    PLAY
                  </button>
                  <button
                    onClick={() => handleGlobalAction('stop')}
                    className="flex-1 py-8 bg-red-600 hover:bg-red-500 text-white rounded-2xl font-black text-xl shadow-xl shadow-red-900/20 active:scale-95 transition-all"
                  >
                    STOP
                  </button>
                </div>
                <button
                  onClick={() => handleGlobalAction('random')}
                  className="w-full py-5 bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 rounded-2xl font-bold flex items-center justify-center gap-3 active:scale-95 transition-all"
                >
                  <Dices size={20} className="text-indigo-400" /> SYNCED PATTERN CYCLE
                </button>
              </div>

              <div className="space-y-4">
                {/* Brightness */}
                <div className="bg-slate-950/50 p-5 rounded-2xl border border-slate-800">
                  <div className="flex justify-between items-center mb-3">
                    <label className="text-[10px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
                      <Sun size={14} className="text-yellow-500" /> Brightness
                    </label>
                    <span className="text-cyan-400 font-mono text-sm">{Math.round((localBrightness / 255) * 100)}%</span>
                  </div>
                  <input
                    type="range" min="0" max="255" value={localBrightness}
                    onChange={(e) => handleBrightnessChange(parseInt(e.target.value))}
                    className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-cyan-500"
                  />
                </div>

                {/* Frame Rate */}
                <div className="bg-slate-950/50 p-5 rounded-2xl border border-slate-800">
                  <div className="flex justify-between items-center mb-3">
                    <label className="text-[10px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
                      <Gauge size={14} className="text-blue-400" /> Frame Rate
                    </label>
                    <span className="text-cyan-400 font-mono text-sm">{localFrameRate} FPS</span>
                  </div>
                  <input
                    type="range" min="10" max="120" value={localFrameRate}
                    onChange={(e) => handleFrameRateChange(parseInt(e.target.value))}
                    className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-blue-500"
                  />
                </div>

                {/* Upload */}
                <div className="bg-slate-950/50 p-4 rounded-2xl border border-slate-800 flex items-center gap-4">
                  <div className="flex-1">
                    <div className="text-[8px] font-black text-slate-600 uppercase mb-1">Active Asset</div>
                    <div className="text-xs text-slate-300 font-mono truncate max-w-[120px]">{filename}</div>
                  </div>
                  <button
                    onClick={() => fileInputRef.current?.click()}
                    disabled={isUploading}
                    className="w-12 h-12 bg-purple-600 hover:bg-purple-500 disabled:opacity-50 text-white rounded-xl flex items-center justify-center shadow-lg shadow-purple-900/20"
                  >
                    {isUploading ? <Wifi size={18} className="animate-pulse" /> : <Upload size={20} />}
                  </button>
                  <input type="file" ref={fileInputRef} onChange={handleFileUpload} className="hidden" accept=".bmp,.png,.jpg,.jpeg" />
                </div>
              </div>
            </div>
          </div>

          <div className="bg-black/80 rounded-2xl p-4 font-mono text-[10px] h-32 flex flex-col border border-slate-800">
            <div className="flex items-center justify-between mb-2 text-slate-600 border-b border-slate-900 pb-2">
              <span className="flex items-center gap-1"><Terminal size={10} /> FLEET_SYNC_MONITOR</span>
              <span>{isSyncMode ? 'MULTICAST_ON • /api/sync/execute' : 'P2P_ONLY'}</span>
            </div>
            <div className="overflow-y-auto flex-1 custom-scrollbar space-y-1">
              {logs.length === 0 && (
                <div className="text-slate-700">Connect to POV-POI-WiFi and start issuing commands...</div>
              )}
              {logs.map((log, i) => (
                <div key={i} className={`${log.color} flex gap-2`}>
                  <span className="text-slate-700 shrink-0">{log.time}</span>
                  <span>{log.msg}</span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Dashboard;
