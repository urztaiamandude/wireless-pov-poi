
import React, { useState, useRef, useEffect } from 'react';
import {
  Play, Square, Sun, Smartphone,
  Wifi, Upload, Terminal, Dices, Sparkles, Monitor, Activity,
  Crown, Users
} from 'lucide-react';
import { Device } from '../types';

interface DashboardProps {
  previewUrl: string | null;
}

// Default devices matching the Nebula POI fleet (POV-POI-WiFi AP addresses)
const DEFAULT_DEVICES: Device[] = [
  { id: 'A1B2C3', name: 'POV_LEADER', ip: '192.168.4.1', status: 'Sync Leader', type: 'success', isPlaying: false, brightness: 128 },
  { id: 'D4E5F6', name: 'POV_FOLLOWER', ip: '192.168.4.2', status: 'Awaiting Sync', type: 'info', isPlaying: false, brightness: 128 }
];

const Dashboard: React.FC<DashboardProps> = ({ previewUrl }) => {
  const [devices, setDevices] = useState<Device[]>(DEFAULT_DEVICES);
  const [selectedDeviceId, setSelectedDeviceId] = useState<string | null>(devices[0].id);
  const [isSyncMode, setIsSyncMode] = useState(true);
  const [filename, setFilename] = useState('pattern.bmp');
  const [isUploading, setIsUploading] = useState(false);
  const [isOnline, setIsOnline] = useState(false);
  const [logs, setLogs] = useState<{time: string, msg: string, color: string}[]>([]);

  const fileInputRef = useRef<HTMLInputElement>(null);
  const activeDevice = devices.find(d => d.id === selectedDeviceId) || devices[0];

  // Poll connection status using the firmware's /api/status endpoint
  useEffect(() => {
    const checkConnection = async () => {
      try {
        const res = await fetch(`http://${activeDevice.ip}/api/status`, { signal: AbortSignal.timeout(1500) });
        setIsOnline(res.ok);
      } catch {
        setIsOnline(false);
      }
    };
    checkConnection();
    const interval = setInterval(checkConnection, 5000);
    return () => clearInterval(interval);
  }, [activeDevice.ip]);

  const addLog = (msg: string, color: string = 'text-slate-400') => {
    const time = new Date().toLocaleTimeString([], { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' });
    setLogs(prev => [{ time, msg, color }, ...prev].slice(0, 50));
  };

  const updateDevice = (id: string, updates: Partial<Device>) => {
    setDevices(prev => prev.map(d => d.id === id ? { ...d, ...updates } : d));
  };

  // Upload BMP image via POST /api/image (matches existing firmware)
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
        const res = await fetch(`http://${target.ip}/api/image`, { method: 'POST', body: formData });
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

  // Send control commands to the firmware's REST API endpoints
  const handleGlobalAction = async (action: 'play' | 'stop' | 'brightness' | 'load' | 'random', value?: string | number) => {
    const targets = isSyncMode ? [devices[0]] : [activeDevice];

    for (const dev of targets) {
      addLog(`[CMD] ${action.toUpperCase()} -> ${dev.name}`, 'text-cyan-400');
      try {
        let url = '';
        let method = 'POST';

        if (action === 'play' || action === 'stop' || action === 'random') {
          // Map to /api/mode endpoint with mode parameter
          const modeVal = action === 'stop' ? 'idle' : action === 'random' ? 'random' : 'playback';
          url = `http://${dev.ip}/api/mode`;
          const body = new FormData();
          body.append('mode', modeVal);
          await fetch(url, { method, body });
        } else if (action === 'brightness') {
          // POST /api/brightness with val param
          url = `http://${dev.ip}/api/brightness`;
          const body = new FormData();
          body.append('val', String(value));
          await fetch(url, { method, body });
        } else if (action === 'load') {
          // POST /api/sd/load with file param
          url = `http://${dev.ip}/api/sd/load`;
          const body = new FormData();
          body.append('file', String(value));
          await fetch(url, { method, body });
        }

        // In sync mode, also trigger the UDP broadcast endpoint
        if (isSyncMode && action !== 'brightness') {
          try {
            await fetch(`http://${dev.ip}/api/sync/execute`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ action, val: value ?? '' })
            });
          } catch { /* best-effort sync broadcast */ }
        }

        // Update local UI state
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
        </div>

        <div className="lg:col-span-8 space-y-6">
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

              <div className="space-y-6">
                <div className="bg-slate-950/50 p-6 rounded-2xl border border-slate-800">
                  <div className="flex justify-between items-center mb-4">
                    <label className="text-[10px] font-black text-slate-500 uppercase tracking-widest flex items-center gap-2">
                      <Sun size={14} className="text-yellow-500" /> Global Intensity
                    </label>
                    <span className="text-cyan-400 font-mono text-sm">{Math.round((activeDevice.brightness / 255) * 100)}%</span>
                  </div>
                  <input
                    type="range" min="0" max="255" value={activeDevice.brightness}
                    onChange={(e) => handleGlobalAction('brightness', parseInt(e.target.value))}
                    className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-cyan-500"
                  />
                </div>

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
                  <input type="file" ref={fileInputRef} onChange={handleFileUpload} className="hidden" accept=".bmp" />
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
                <div key={i} className="flex gap-2">
                  <span className="text-slate-800 shrink-0">{log.time}</span>
                  <span className={log.color}>{log.msg}</span>
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
