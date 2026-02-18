
import React, { useState } from 'react';
import { Settings2, RefreshCw, Grid3X3, Palette, Info, Save, RotateCw, Cpu, Zap, CircuitBoard } from 'lucide-react';

interface AdvancedSettingsProps {
  ledCount: number;
  setLedCount: (count: number) => void;
}

const DEVICE_IP = '192.168.4.1'; // POV-POI-WiFi leader default

const AdvancedSettings: React.FC<AdvancedSettingsProps> = ({ ledCount, setLedCount }) => {
  const [refreshRate, setRefreshRate] = useState(60);
  const [pixelDensity, setPixelDensity] = useState(144);
  const [colorDepth, setColorDepth] = useState(24);
  const [dataPin, setDataPin] = useState(11);
  const [clkPin, setClkPin] = useState(13);
  const [isSaving, setIsSaving] = useState(false);
  const [saveStatus, setSaveStatus] = useState<string | null>(null);

  // Deploy config to the firmware via POST /api/device/config
  // NOTE: Current firmware implementation only accepts deviceName, syncGroup, and autoSync.
  // Hardware parameters (ledCount, pins, etc.) are not yet supported by the backend.
  // This is a UI-ready implementation pending firmware support.
  const handleSave = async () => {
    setIsSaving(true);
    setSaveStatus(null);
    const payload = { ledCount, dataPin, clkPin, refreshRate, pixelDensity, colorDepth };
    try {
      const res = await fetch(`http://${DEVICE_IP}/api/device/config`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
        signal: AbortSignal.timeout(3000)
      });
      if (res.ok) {
        setSaveStatus('Config deployed successfully.');
      } else {
        const text = await res.text();
        setSaveStatus(`Warning: Endpoint exists but may not support all parameters. Response: ${text.substring(0, 50)}`);
      }
    } catch {
      setSaveStatus('Could not reach device. Verify POV-POI-WiFi connection.');
    } finally {
      setIsSaving(false);
    }
  };

  return (
    <div className="space-y-8 animate-fadeIn">
      <header className="flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <h2 className="text-3xl font-bold text-white mb-2 flex items-center gap-3">
            <Settings2 className="text-cyan-400" /> Display Configuration
          </h2>
          <p className="text-slate-400">Fine-tune hardware pins and display performance. Deploys to <span className="font-mono text-cyan-400">{DEVICE_IP}/api/device/config</span>.</p>
        </div>
        <div className="flex flex-col items-end gap-2">
          <button
            onClick={handleSave}
            disabled={isSaving}
            className="flex items-center gap-2 px-8 py-3 bg-cyan-600 hover:bg-cyan-500 disabled:bg-slate-800 text-white rounded-xl font-bold shadow-lg shadow-cyan-900/20 transition-all active:scale-95"
          >
            {isSaving ? <RefreshCw className="animate-spin" size={20} /> : <Save size={20} />}
            {isSaving ? 'Syncing...' : 'Deploy Settings'}
          </button>
          {saveStatus && (
            <span className={`text-[10px] font-mono ${saveStatus.startsWith('Error') || saveStatus.startsWith('Could') ? 'text-red-400' : 'text-green-400'}`}>
              {saveStatus}
            </span>
          )}
        </div>
      </header>

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
            <div className="flex items-center gap-4">
              <input
                type="number"
                value={dataPin}
                onChange={(e) => setDataPin(parseInt(e.target.value))}
                className="bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-white font-mono w-full focus:ring-1 focus:ring-cyan-500 outline-none"
              />
              <Cpu size={18} className="text-cyan-500 shrink-0" />
            </div>
          </div>

          <div className="space-y-2">
            <label className="text-xs font-bold text-slate-500 uppercase tracking-widest">Clock Pin (SCK)</label>
            <div className="flex items-center gap-4">
              <input
                type="number"
                value={clkPin}
                onChange={(e) => setClkPin(parseInt(e.target.value))}
                className="bg-slate-950 border border-slate-700 rounded-lg px-4 py-2 text-white font-mono w-full focus:ring-1 focus:ring-cyan-500 outline-none"
              />
              <Cpu size={18} className="text-purple-500 shrink-0" />
            </div>
          </div>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <ConfigCard
          icon={<RotateCw className="text-blue-400" />}
          title="Refresh Rate"
          value={`${refreshRate} Hz`}
          description="Number of frame slices rendered per rotation. Higher values reduce flicker but increase CPU load."
        >
          <input
            type="range"
            min="30"
            max="240"
            step="10"
            value={refreshRate}
            onChange={(e) => setRefreshRate(parseInt(e.target.value))}
            className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-blue-400"
          />
          <div className="flex justify-between text-[10px] text-slate-500 mt-2 font-mono">
            <span>30Hz</span>
            <span>120Hz</span>
            <span>240Hz</span>
          </div>
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
              <div className="text-purple-400">FastLED.addLeds&lt;APA102, {dataPin}, {clkPin}&gt;(leds, NUM_LEDS);</div>
              <div className="text-slate-300 mt-2">// POST /api/device/config to sync</div>
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
