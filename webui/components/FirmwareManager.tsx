
import React, { useState } from 'react';
import { Upload, Cpu, Zap, ShieldCheck, AlertTriangle, Loader2, CheckCircle2 } from 'lucide-react';

// Firmware upload endpoints on the existing ESP32 firmware
const OTA_ENDPOINTS: Record<string, string> = {
  ESP32: 'http://192.168.4.1/api/ota/esp32',
  Teensy: 'http://192.168.4.1/api/ota/teensy'
};

const FirmwareManager: React.FC = () => {
  const [target, setTarget] = useState<'ESP32' | 'Teensy'>('ESP32');
  const [isUploading, setIsUploading] = useState(false);
  const [progress, setProgress] = useState(0);
  const [status, setStatus] = useState<string | null>(null);
  const fileInputRef = React.useRef<HTMLInputElement>(null);

  const handleFirmwareSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    setIsUploading(true);
    setProgress(0);
    setStatus(`Preparing ${target} update: ${file.name}...`);

    // Simulate progress ticks while uploading via XHR for real progress events
    const xhr = new XMLHttpRequest();
    const formData = new FormData();
    formData.append('firmware', file, file.name);

    xhr.upload.onprogress = (ev) => {
      if (ev.lengthComputable) {
        setProgress(Math.round((ev.loaded / ev.total) * 95));
      }
    };

    xhr.onload = () => {
      setProgress(100);
      setIsUploading(false);
      if (xhr.status >= 200 && xhr.status < 300) {
        setStatus(`Update successful! ${target} is rebooting...`);
      } else {
        setStatus(`Upload failed: HTTP ${xhr.status}. Check firmware format.`);
      }
    };

    xhr.onerror = () => {
      setIsUploading(false);
      setStatus(`Network error. Ensure you are connected to POV-POI-WiFi.`);
    };

    xhr.open('POST', OTA_ENDPOINTS[target]);
    xhr.send(formData);
  };

  return (
    <div className="space-y-8 animate-fadeIn">
      <header>
        <h2 className="text-3xl font-bold text-white mb-2">Firmware & OTA</h2>
        <p className="text-slate-400">Push wireless updates to your POV system components via <span className="font-mono text-cyan-400">POV-POI-WiFi</span>.</p>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-8">
        <div className="lg:col-span-5 space-y-6">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl p-6">
            <h3 className="text-white font-semibold mb-6">Select Target Hardware</h3>
            <div className="grid grid-cols-2 gap-4">
              <button
                onClick={() => setTarget('ESP32')}
                className={`p-4 rounded-xl border flex flex-col items-center gap-3 transition-all ${
                  target === 'ESP32' ? 'bg-cyan-500/10 border-cyan-500 text-cyan-400' : 'bg-slate-800/50 border-slate-700 text-slate-500'
                }`}
              >
                <Cpu size={32} />
                <span className="font-bold text-sm">ESP32-S3</span>
                <span className="text-[9px] font-mono opacity-60">.bin</span>
              </button>
              <button
                onClick={() => setTarget('Teensy')}
                className={`p-4 rounded-xl border flex flex-col items-center gap-3 transition-all ${
                  target === 'Teensy' ? 'bg-purple-500/10 border-purple-500 text-purple-400' : 'bg-slate-800/50 border-slate-700 text-slate-500'
                }`}
              >
                <Zap size={32} />
                <span className="font-bold text-sm">Teensy 4.1</span>
                <span className="text-[9px] font-mono opacity-60">.hex</span>
              </button>
            </div>

            <div className="mt-8 space-y-4">
              <div className="flex items-start gap-3 p-3 bg-slate-800/30 rounded-lg">
                <ShieldCheck className="text-green-500 shrink-0" size={18} />
                <p className="text-xs text-slate-400">Endpoint: <span className="text-slate-200 font-mono">{OTA_ENDPOINTS[target]}</span></p>
              </div>
              <div className="flex items-start gap-3 p-3 bg-amber-500/10 rounded-lg">
                <AlertTriangle className="text-amber-500 shrink-0" size={18} />
                <p className="text-xs text-amber-200/70 leading-relaxed">Ensure a stable 5V power supply. Disconnecting during the write phase will result in a hard brick.</p>
              </div>
            </div>
          </div>
        </div>

        <div className="lg:col-span-7">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl p-8 h-full flex flex-col justify-center items-center text-center">
            {isUploading ? (
              <div className="w-full max-w-md space-y-8 py-12">
                <Loader2 className="mx-auto text-cyan-400 animate-spin" size={48} />
                <div className="space-y-4">
                  <div className="flex justify-between text-xs font-mono text-slate-500 uppercase tracking-widest">
                    <span>Writing Flash...</span>
                    <span>{progress}%</span>
                  </div>
                  <div className="w-full h-3 bg-slate-800 rounded-full overflow-hidden">
                    <div
                      className={`h-full transition-all duration-300 rounded-full ${target === 'ESP32' ? 'bg-cyan-500' : 'bg-purple-500'}`}
                      style={{ width: `${progress}%` }}
                    />
                  </div>
                </div>
                <p className="text-slate-400 italic text-sm animate-pulse">{status}</p>
              </div>
            ) : status?.includes('successful') ? (
              <div className="space-y-6 py-12">
                <div className="w-20 h-20 bg-green-500/10 rounded-full flex items-center justify-center mx-auto">
                  <CheckCircle2 size={48} className="text-green-500" />
                </div>
                <div className="space-y-2">
                  <h3 className="text-2xl font-bold text-white">Update Complete</h3>
                  <p className="text-slate-400">{status}</p>
                </div>
                <button
                  onClick={() => { setStatus(null); setProgress(0); }}
                  className="px-6 py-2 bg-slate-800 hover:bg-slate-700 rounded-lg text-sm font-medium transition-all"
                >
                  Return to Manager
                </button>
              </div>
            ) : (
              <div className="space-y-8">
                <div
                  className="w-24 h-24 border-2 border-dashed border-slate-700 rounded-full flex items-center justify-center mx-auto hover:border-cyan-500 transition-colors cursor-pointer"
                  onClick={() => fileInputRef.current?.click()}
                >
                  <Upload size={32} className="text-slate-500" />
                </div>
                <div className="space-y-2">
                  <h3 className="text-xl font-bold text-white">Upload {target} Firmware</h3>
                  <p className="text-slate-400 text-sm max-w-sm">
                    Select the compiled binary (<code className="text-slate-200">{target === 'ESP32' ? '.bin' : '.hex'}</code>) to initiate OTA.
                    {status && !status.includes('successful') && (
                      <span className="block mt-2 text-red-400">{status}</span>
                    )}
                  </p>
                </div>
                <button
                  onClick={() => fileInputRef.current?.click()}
                  className={`px-8 py-3 rounded-xl font-bold transition-all shadow-lg ${
                    target === 'ESP32'
                      ? 'bg-cyan-600 hover:bg-cyan-500 shadow-cyan-900/20'
                      : 'bg-purple-600 hover:bg-purple-500 shadow-purple-900/20'
                  }`}
                >
                  Select {target} Firmware
                </button>
                <input
                  ref={fileInputRef}
                  type="file"
                  className="hidden"
                  accept={target === 'ESP32' ? '.bin' : '.hex'}
                  onChange={handleFirmwareSelect}
                />
              </div>
            )}
          </div>
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6">
        <h4 className="text-white font-semibold mb-4">Firmware Pipeline Architecture</h4>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-8 text-sm text-slate-400 leading-relaxed">
          <div className="space-y-3">
            <div className="flex items-center gap-2 text-cyan-400 font-bold">
              <div className="w-1.5 h-1.5 rounded-full bg-cyan-400" />
              ESP32 OTA Mechanism
            </div>
            <p>Uses the <code className="text-slate-200">Update.h</code> library. The web interface streams the binary to the second OTA partition. Upon completion, the bootloader swaps partitions and restarts the SOC.</p>
          </div>
          <div className="space-y-3">
            <div className="flex items-center gap-2 text-purple-400 font-bold">
              <div className="w-1.5 h-1.5 rounded-full bg-purple-400" />
              Teensy Relay Mechanism
            </div>
            <p>Since Teensy has no radio, the ESP32 acts as a transparent relay. Firmware data is buffered on the shared SD card or streamed over UART. Teensy uses <code className="text-slate-200">FlasherX</code> logic to write to its program memory from RAM.</p>
          </div>
        </div>
      </div>
    </div>
  );
};

export default FirmwareManager;
