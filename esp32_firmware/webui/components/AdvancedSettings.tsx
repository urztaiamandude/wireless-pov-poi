
import React, { useState, useEffect, useCallback } from 'react';
import { Settings2, RefreshCw, Grid3X3, Palette, Info, RotateCw, Zap, CircuitBoard, Wifi, Lock, Trash2, Check } from 'lucide-react';
import { useDebounce } from '../hooks';

interface AdvancedSettingsProps {
  ledCount: number;
  setLedCount: (count: number) => void;
}

interface WifiStatus {
  apIp: string;
  apSsid: string;
  staConnected: boolean;
  staIp: string;
  savedSsid: string;
}

const AdvancedSettings: React.FC<AdvancedSettingsProps> = ({ ledCount, setLedCount }) => {
  const [refreshRate, setRefreshRate] = useState(60);
  const [refreshRateStatus, setRefreshRateStatus] = useState<string | null>(null);
  const [pixelDensity, setPixelDensity] = useState(144);
  const [colorDepth, setColorDepth] = useState(24);

  // WiFi network (connect to existing network to access web UI over that network)
  const [wifiStatus, setWifiStatus] = useState<WifiStatus | null>(null);
  const [wifiLoading, setWifiLoading] = useState(true);
  const [wifiConnectLoading, setWifiConnectLoading] = useState(false);
  const [wifiError, setWifiError] = useState<string | null>(null);
  const [wifiSsid, setWifiSsid] = useState('');
  const [wifiPassword, setWifiPassword] = useState('');

  const baseUrl = '';

  const fetchWifiStatus = useCallback(async () => {
    try {
      const res = await fetch(`${baseUrl}/api/wifi/status`, { signal: AbortSignal.timeout(3000) });
      if (res.ok) {
        const data = await res.json();
        setWifiStatus(data);
        setWifiError(null);
      }
    } catch {
      setWifiStatus(null);
      setWifiError('Could not reach device.');
    } finally {
      setWifiLoading(false);
    }
  }, [baseUrl]);

  useEffect(() => {
    fetchWifiStatus();
    const t = setInterval(fetchWifiStatus, 10000);
    return () => clearInterval(t);
  }, [fetchWifiStatus]);

  // Send refresh rate to Teensy via ESP32's /api/framerate endpoint (command 0x07)
  const debouncedFrameRateUpdate = useDebounce(
    useCallback(async (fps: number) => {
      try {
        const res = await fetch(`${baseUrl}/api/framerate`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ framerate: fps }),
          signal: AbortSignal.timeout(3000)
        });
        if (res.ok) {
          setRefreshRateStatus(`Applied ${fps} FPS to Teensy.`);
        } else {
          setRefreshRateStatus(`Failed to set frame rate (HTTP ${res.status}).`);
        }
      } catch {
        setRefreshRateStatus('Could not reach device.');
      }
    }, [baseUrl]),
    300
  );

  const handleRefreshRateChange = (value: number) => {
    setRefreshRate(value);
    setRefreshRateStatus(null);
    debouncedFrameRateUpdate(value);
  };

  const handleWifiConnect = async () => {
    if (!wifiSsid.trim()) {
      setWifiError('Enter a network name (SSID).');
      return;
    }
    setWifiConnectLoading(true);
    setWifiError(null);
    try {
      const res = await fetch(`${baseUrl}/api/wifi/connect`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid: wifiSsid.trim(), password: wifiPassword }),
        signal: AbortSignal.timeout(5000)
      });
      const data = await res.json().catch(() => ({}));
      if (!res.ok) {
        setWifiError(data.error || 'Connect failed.');
        return;
      }
      setWifiError(null);
      for (let i = 0; i < 15; i++) {
        await new Promise((r) => setTimeout(r, 2000));
        const r2 = await fetch(`${baseUrl}/api/wifi/status`, { signal: AbortSignal.timeout(3000) });
        if (r2.ok) {
          const s = await r2.json();
          setWifiStatus(s);
          if (s.staConnected) break;
        }
      }
    } catch {
      setWifiError('Request failed. Try again.');
    } finally {
      setWifiConnectLoading(false);
    }
  };

  const handleWifiDisconnect = async () => {
    setWifiConnectLoading(true);
    setWifiError(null);
    try {
      await fetch(`${baseUrl}/api/wifi/disconnect`, {
        method: 'POST',
        signal: AbortSignal.timeout(3000)
      });
      await fetchWifiStatus();
      setWifiSsid('');
      setWifiPassword('');
    } catch {
      setWifiError('Disconnect request failed.');
    } finally {
      setWifiConnectLoading(false);
    }
  };

  return (
    <div className="space-y-8 animate-fadeIn">
      <header>
        <div>
          <h2 className="text-3xl font-bold text-white mb-2 flex items-center gap-3">
            <Settings2 className="text-cyan-400" /> Display Configuration
          </h2>
          <p className="text-slate-400">Reference settings for the Teensy 4.1 POV engine. Hardware pins and LED count are configured in Teensy firmware.</p>
        </div>
      </header>

      <div className="bg-slate-900 border border-slate-800 rounded-2xl p-6">
        <h3 className="text-white font-semibold mb-6 flex items-center gap-2">
          <Wifi size={18} className="text-cyan-400" /> WiFi Network
        </h3>
        <p className="text-slate-400 text-sm mb-4">
          Connect the device to your home/router WiFi to access the web UI from any device on that network. The device always runs its own access point (POV-POI-WiFi) as well.
        </p>
        {wifiLoading && !wifiStatus && (
          <p className="text-slate-500 text-sm flex items-center gap-2">
            <RefreshCw className="animate-spin" size={16} /> Loading…
          </p>
        )}
        {wifiStatus && (
          <>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-6">
              <div className="bg-slate-950 border border-slate-700 rounded-xl p-4">
                <div className="text-xs font-bold text-slate-500 uppercase tracking-widest mb-1">Access point</div>
                <div className="font-mono text-cyan-400">{wifiStatus.apSsid}</div>
                <div className="text-sm text-slate-400">Open <span className="font-mono text-white">{wifiStatus.apIp}</span> when connected to this network</div>
              </div>
              <div className="bg-slate-950 border border-slate-700 rounded-xl p-4">
                <div className="text-xs font-bold text-slate-500 uppercase tracking-widest mb-1">Home network</div>
                {wifiStatus.staConnected ? (
                  <>
                    <div className="font-mono text-green-400">Connected</div>
                    <div className="text-sm text-slate-400">Use <span className="font-mono text-white">{wifiStatus.staIp}</span> from your LAN</div>
                    {wifiStatus.savedSsid && <div className="text-xs text-slate-500 mt-1">Saved: {wifiStatus.savedSsid}</div>}
                  </>
                ) : (
                  <>
                    <div className="text-amber-400">Not connected</div>
                    {wifiStatus.savedSsid ? (
                      <div className="text-sm text-slate-400">Saved network: {wifiStatus.savedSsid} (reconnecting…)</div>
                    ) : (
                      <div className="text-sm text-slate-400">Connect below to access over your WiFi</div>
                    )}
                  </>
                )}
              </div>
            </div>
            <div className="flex flex-wrap gap-4 items-end">
              <div className="flex-1 min-w-[140px]">
                <label className="text-xs font-bold text-slate-500 uppercase tracking-widest block mb-1">Network name (SSID)</label>
                <input
                  type="text"
                  value={wifiSsid}
                  onChange={(e) => setWifiSsid(e.target.value)}
                  placeholder="Your WiFi name"
                  className="w-full bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-white font-mono focus:ring-1 focus:ring-cyan-500 outline-none"
                />
              </div>
              <div className="flex-1 min-w-[140px]">
                <label className="text-xs font-bold text-slate-500 uppercase tracking-widest block mb-1 flex items-center gap-1">
                  <Lock size={12} /> Password
                </label>
                <input
                  type="password"
                  value={wifiPassword}
                  onChange={(e) => setWifiPassword(e.target.value)}
                  placeholder="WiFi password"
                  className="w-full bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-white font-mono focus:ring-1 focus:ring-cyan-500 outline-none"
                />
              </div>
              <div className="flex gap-2">
                <button
                  onClick={handleWifiConnect}
                  disabled={wifiConnectLoading}
                  className="flex items-center gap-2 px-5 py-2.5 bg-cyan-600 hover:bg-cyan-500 disabled:bg-slate-800 text-white rounded-xl font-semibold transition-all"
                >
                  {wifiConnectLoading ? <RefreshCw className="animate-spin" size={18} /> : <Wifi size={18} />}
                  {wifiConnectLoading ? 'Connecting…' : 'Connect'}
                </button>
                {wifiStatus.savedSsid && (
                  <button
                    onClick={handleWifiDisconnect}
                    disabled={wifiConnectLoading}
                    className="flex items-center gap-2 px-5 py-2.5 bg-slate-700 hover:bg-slate-600 text-white rounded-xl font-semibold transition-all"
                  >
                    <Trash2 size={18} /> Disconnect
                  </button>
                )}
              </div>
            </div>
            {wifiError && <p className="text-red-400 text-sm mt-3">{wifiError}</p>}
          </>
        )}
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-2xl p-6">
        <h3 className="text-white font-semibold mb-6 flex items-center gap-2">
          <CircuitBoard size={18} className="text-cyan-400" /> LED Hardware Interface
        </h3>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
          <div className="space-y-2">
            <label className="text-xs font-bold text-slate-500 uppercase tracking-widest">Global LED Count (Height)</label>
            <div className="flex items-center gap-4">
              <input
                type="number"
                value={ledCount}
                onChange={(e) => setLedCount(parseInt(e.target.value) || 1)}
                className="bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-white font-mono w-full focus:ring-1 focus:ring-cyan-500 outline-none transition-all"
              />
              <Zap size={18} className="text-yellow-500 shrink-0" />
            </div>
            <p className="text-[10px] text-slate-500 italic">Vertical resolution ({ledCount}px). Sync with Teensy NUM_LEDS.</p>
          </div>

          <div className="space-y-2">
            <label className="text-xs font-bold text-slate-500 uppercase tracking-widest">Data Pin (MOSI)</label>
            <div className="bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-slate-400 font-mono">
              Pin 11
            </div>
            <p className="text-[10px] text-slate-500 italic">Hardwired on Teensy 4.1 (not configurable from ESP32).</p>
          </div>

          <div className="space-y-2">
            <label className="text-xs font-bold text-slate-500 uppercase tracking-widest">Clock Pin (SCK)</label>
            <div className="bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-slate-400 font-mono">
              Pin 13
            </div>
            <p className="text-[10px] text-slate-500 italic">Hardwired on Teensy 4.1 (not configurable from ESP32).</p>
          </div>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <ConfigCard
          icon={<RotateCw className="text-blue-400" />}
          title="Refresh Rate"
          value={`${refreshRate} FPS`}
          description="Frame rate sent to Teensy 4.1 via /api/framerate. The Teensy controls actual LED timing — the ESP32 only relays this value."
        >
          <input
            type="range"
            min="10"
            max="250"
            step="5"
            value={refreshRate}
            onChange={(e) => { const v = parseInt(e.target.value); if (!isNaN(v)) handleRefreshRateChange(v); }}
            className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-blue-400"
          />
          <div className="flex justify-between text-[10px] text-slate-500 mt-2 font-mono">
            <span>10 FPS</span>
            <span>120 FPS</span>
            <span>250 FPS</span>
          </div>
          {refreshRateStatus && (
            <div className={`flex items-center gap-1 mt-2 text-[10px] font-mono ${refreshRateStatus.startsWith('Applied') ? 'text-green-400' : 'text-red-400'}`}>
              {refreshRateStatus.startsWith('Applied') && <Check size={10} />}
              {refreshRateStatus}
            </div>
          )}
        </ConfigCard>

        <ConfigCard
          icon={<Grid3X3 className="text-purple-400" />}
          title="Angular Resolution"
          value={`${pixelDensity} PX`}
          description="Total vertical slices in a full 360° rotation. Matches your encoder resolution."
        >
          <div className="grid grid-cols-4 gap-2">
            {[72, 144, 288, 576].map((val) => (
              <button
                key={val}
                onClick={() => setPixelDensity(val)}
                className={`py-2 rounded-lg text-xs font-bold border transition-all ${
                  pixelDensity === val
                    ? 'bg-purple-500/10 border-purple-500 text-purple-400 shadow-[0_0_10px_rgba(168,85,247,0.2)]'
                    : 'bg-slate-800 border-slate-700 text-slate-500 hover:border-slate-600'
                }`}
              >
                {val}
              </button>
            ))}
          </div>
        </ConfigCard>

        <ConfigCard
          icon={<Palette className="text-pink-400" />}
          title="Color Depth"
          value={`${colorDepth} BIT`}
          description="Bit-depth per color channel. Lowering can improve frame rates on high-density displays."
        >
          <div className="flex gap-4">
            {[8, 16, 24].map((val) => (
              <label key={val} className="flex-1 cursor-pointer">
                <input
                  type="radio"
                  name="depth"
                  className="hidden"
                  checked={colorDepth === val}
                  onChange={() => setColorDepth(val)}
                />
                <div className={`py-3 rounded-xl text-center border transition-all font-bold text-sm ${
                  colorDepth === val
                    ? 'bg-pink-500/10 border-pink-500 text-pink-400 shadow-lg shadow-pink-900/10'
                    : 'bg-slate-800 border-slate-700 text-slate-500'
                }`}>
                  {val === 24 ? 'TRUE' : val + 'b'}
                </div>
              </label>
            ))}
          </div>
        </ConfigCard>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-2xl p-6">
        <h3 className="text-white font-semibold mb-4 flex items-center gap-2">
          <Info size={18} className="text-cyan-400" /> Advanced Engine Logic
        </h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
          <div className="space-y-4 text-sm text-slate-400">
            <p>
              The <strong>Teensy 4.1</strong> uses a high-precision hardware timer (GPT) to trigger LED updates.
              The <code className="text-cyan-300">Refresh Rate</code> setting modulates the period of this timer.
            </p>
            <p>
              When <strong>LED Count</strong> is changed (currently {ledCount}), the Teensy driver
              re-allocates the <code>leds</code> array in PSRAM. The Nebula POI supports up to 64 LEDs
              with PSRAM enabled.
            </p>
          </div>
          <div className="space-y-4 text-sm text-slate-400">
            <div className="bg-black/40 border border-slate-700 p-4 rounded-xl font-mono text-[11px]">
              <div className="text-slate-500 mb-2">// Hardware Allocation (Teensy 4.1)</div>
              <div className="text-cyan-400">#define NUM_LEDS {ledCount}</div>
              <div className="text-cyan-400">CRGB leds[NUM_LEDS];</div>
              <div className="text-purple-400">FastLED.addLeds&lt;APA102, 11, 13&gt;(leds, NUM_LEDS);</div>
              <div className="text-slate-300 mt-2">// Configured in Teensy firmware (not via ESP32)</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

interface ConfigCardProps {
  icon: React.ReactNode;
  title: string;
  value: string;
  description: string;
  children: React.ReactNode;
}

const ConfigCard: React.FC<ConfigCardProps> = ({ icon, title, value, description, children }) => (
  <div className="bg-slate-900 border border-slate-800 rounded-2xl p-6 flex flex-col h-full hover:border-slate-700 transition-colors">
    <div className="flex items-center justify-between mb-2">
      <div className="flex items-center gap-3">
        {icon}
        <h3 className="font-bold text-white uppercase tracking-wider text-xs">{title}</h3>
      </div>
      <span className="text-xl font-black font-mono text-white">{value}</span>
    </div>
    <p className="text-xs text-slate-500 mb-6 leading-relaxed flex-1">
      {description}
    </p>
    <div className="mt-auto">
      {children}
    </div>
  </div>
);

export default AdvancedSettings;
