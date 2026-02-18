
import React from 'react';
import { ArrowRightLeft, Zap, Layers } from 'lucide-react';

const WiringGuide: React.FC = () => {
  return (
    <div className="space-y-8">
      <header>
        <h2 className="text-3xl font-bold text-white mb-2 text-center lg:text-left">Inter-Board Wiring Guide</h2>
        <p className="text-slate-400 text-center lg:text-left">Proper electrical connections for stable serial communication.</p>
      </header>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <WiringCard
          icon={<ArrowRightLeft className="text-cyan-400" />}
          title="UART Bridge"
          details={[
            { from: "ESP32-S3 TX2 (GPIO 17)", to: "Teensy 4.1 RX1 (Pin 0)" },
            { from: "ESP32-S3 RX2 (GPIO 18)", to: "Teensy 4.1 TX1 (Pin 1)" }
          ]}
        />
        <WiringCard
          icon={<Zap className="text-yellow-400" />}
          title="Power Logic"
          details={[
            { from: "Common GND", to: "Essential for Serial logic" },
            { from: "5V Power", to: "Direct to LED Strip VCC" },
            { from: "Teensy VIN", to: "5V (High Current)" }
          ]}
        />
        <WiringCard
          icon={<Layers className="text-purple-400" />}
          title="LED Interface"
          details={[
            { from: "Teensy SCK (Pin 13)", to: "APA102 CLK" },
            { from: "Teensy MOSI (Pin 11)", to: "APA102 DATA" }
          ]}
        />
      </div>

      <div className="bg-slate-900/50 border border-slate-800 rounded-xl p-8 relative overflow-hidden">
        <h3 className="text-xl font-bold text-white mb-6">Circuit Diagram Simulation</h3>

        <div className="relative h-64 border border-slate-700/50 rounded-lg bg-black/40 flex items-center justify-center gap-12 lg:gap-32">
          {/* ESP32 Box */}
          <div className="w-40 h-32 bg-slate-800 rounded-lg border-2 border-cyan-500 flex flex-col items-center justify-center p-4 shadow-[0_0_20px_rgba(6,182,212,0.1)]">
            <span className="text-cyan-400 font-bold">ESP32-S3</span>
            <span className="text-[10px] text-slate-500 mt-2 italic">(The Brain)</span>
          </div>

          {/* UART Lines */}
          <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
            <div className="w-20 lg:w-40 h-0.5 bg-gradient-to-r from-cyan-500 via-slate-700 to-purple-500" />
            <div className="w-20 lg:w-40 h-0.5 bg-gradient-to-r from-purple-500 via-slate-700 to-cyan-500" />
          </div>

          {/* Teensy Box */}
          <div className="w-40 h-32 bg-slate-800 rounded-lg border-2 border-purple-500 flex flex-col items-center justify-center p-4 shadow-[0_0_20px_rgba(168,85,247,0.1)]">
            <span className="text-purple-400 font-bold">Teensy 4.1</span>
            <span className="text-[10px] text-slate-500 mt-2 italic">(The Muscle)</span>
          </div>
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl p-6">
        <h4 className="text-red-400 font-bold mb-2">CRITICAL SAFETY:</h4>
        <p className="text-slate-400 text-sm">
          High-density APA102 LED strips can pull significant amperage. <strong>Do not power the LEDs directly from the Teensy 5V pin.</strong>
          Use a dedicated 5V power supply and share the ground with both boards. For portable POV systems, use 18650 cells with a boost converter.
        </p>
      </div>
    </div>
  );
};

const WiringCard: React.FC<{ icon: React.ReactNode, title: string, details: {from: string, to: string}[] }> = ({ icon, title, details }) => (
  <div className="bg-slate-900 border border-slate-800 rounded-xl p-6 hover:border-slate-700 transition-all">
    <div className="flex items-center gap-2 mb-4">
      {icon}
      <h3 className="font-bold text-white">{title}</h3>
    </div>
    <div className="space-y-3">
      {details.map((d, i) => (
        <div key={i} className="text-xs">
          <div className="text-slate-500">{d.from}</div>
          <div className="flex items-center gap-2 text-slate-300">
            <div className="w-full h-px bg-slate-700" />
            <span className="whitespace-nowrap font-mono">{d.to}</span>
          </div>
        </div>
      ))}
    </div>
  </div>
);

export default WiringGuide;
