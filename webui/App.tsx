
import React, { useState } from 'react';
import { ViewMode } from './types';
import { getESP32Sketch, getTeensySketch } from './constants';
import Dashboard from './components/Dashboard';
import CodeViewer from './components/CodeViewer';
import WiringGuide from './components/WiringGuide';
import AIAssistant from './components/AIAssistant';
import FirmwareManager from './components/FirmwareManager';
import AdvancedSettings from './components/AdvancedSettings';
import ImageLab from './components/ImageLab';
import {
  LayoutDashboard,
  Cpu,
  CircuitBoard,
  Wrench,
  Sparkles,
  Github,
  CloudUpload,
  Settings2,
  ImageIcon
} from 'lucide-react';

const App: React.FC = () => {
  const [view, setView] = useState<ViewMode>(ViewMode.DASHBOARD);
  const [globalPreviewUrl, setGlobalPreviewUrl] = useState<string | null>(null);
  const [ledCount, setLedCount] = useState<number>(32);

  const renderContent = () => {
    switch (view) {
      case ViewMode.DASHBOARD:
        return <Dashboard previewUrl={globalPreviewUrl} />;
      case ViewMode.IMAGE_LAB:
        return <ImageLab onPreviewUpdate={setGlobalPreviewUrl} initialPreview={globalPreviewUrl} ledCount={ledCount} setLedCount={setLedCount} />;
      case ViewMode.ADVANCED_SETTINGS:
        return <AdvancedSettings ledCount={ledCount} setLedCount={setLedCount} />;
      case ViewMode.CODE_ESP32:
        return <CodeViewer sketch={getESP32Sketch(ledCount)} />;
      case ViewMode.CODE_TEENSY:
        return <CodeViewer sketch={getTeensySketch(ledCount)} />;
      case ViewMode.WIRING:
        return <WiringGuide />;
      case ViewMode.AI_ASSISTANT:
        return <AIAssistant />;
      case ViewMode.FIRMWARE:
        return <FirmwareManager />;
      default:
        return <Dashboard previewUrl={globalPreviewUrl} />;
    }
  };

  return (
    <div className="flex flex-col lg:flex-row min-h-screen">
      {/* Sidebar Navigation */}
      <nav className="w-full lg:w-64 bg-slate-900 border-b lg:border-r border-slate-800 p-4 space-y-1 lg:fixed lg:h-full z-20 overflow-y-auto custom-scrollbar">
        <div className="flex items-center gap-2 px-2 py-4 mb-4">
          <CircuitBoard className="text-cyan-400 w-8 h-8" />
          <div>
            <h1 className="text-xl font-bold bg-gradient-to-r from-cyan-400 to-purple-400 bg-clip-text text-transparent">
              POV Architect
            </h1>
            <p className="text-[9px] text-slate-600 font-mono">Nebula POI • wireless-pov-poi</p>
          </div>
        </div>

        <NavItem
          active={view === ViewMode.DASHBOARD}
          onClick={() => setView(ViewMode.DASHBOARD)}
          icon={<LayoutDashboard size={20} />}
          label="UI Dashboard"
        />
        <NavItem
          active={view === ViewMode.IMAGE_LAB}
          onClick={() => setView(ViewMode.IMAGE_LAB)}
          icon={<ImageIcon size={20} className="text-pink-400" />}
          label="Image Lab"
        />
        <NavItem
          active={view === ViewMode.ADVANCED_SETTINGS}
          onClick={() => setView(ViewMode.ADVANCED_SETTINGS)}
          icon={<Settings2 size={20} />}
          label="Display Config"
        />
        <div className="h-4" />
        <NavItem
          active={view === ViewMode.CODE_ESP32}
          onClick={() => setView(ViewMode.CODE_ESP32)}
          icon={<Cpu size={20} />}
          label="ESP32 Sketch"
        />
        <NavItem
          active={view === ViewMode.CODE_TEENSY}
          onClick={() => setView(ViewMode.CODE_TEENSY)}
          icon={<Cpu size={20} />}
          label="Teensy Sketch"
        />
        <NavItem
          active={view === ViewMode.WIRING}
          onClick={() => setView(ViewMode.WIRING)}
          icon={<Wrench size={20} />}
          label="Wiring Guide"
        />
        <NavItem
          active={view === ViewMode.FIRMWARE}
          onClick={() => setView(ViewMode.FIRMWARE)}
          icon={<CloudUpload size={20} />}
          label="Firmware (OTA)"
        />
        <div className="pt-4 mt-4 border-t border-slate-800">
          <NavItem
            active={view === ViewMode.AI_ASSISTANT}
            onClick={() => setView(ViewMode.AI_ASSISTANT)}
            icon={<Sparkles size={20} className="text-purple-400" />}
            label="AI Expansions"
          />
        </div>

        <div className="pt-8 px-4 hidden lg:block">
          <a
            href="https://github.com/urztaiamandude/wireless-pov-poi"
            target="_blank"
            rel="noreferrer"
            className="flex items-center gap-2 text-xs text-slate-500 hover:text-slate-300 transition-colors"
          >
            <Github size={14} />
            wireless-pov-poi on GitHub
          </a>
        </div>
      </nav>

      {/* Main Content Area */}
      <main className="flex-1 lg:ml-64 p-4 md:p-8 bg-slate-950">
        <div className="max-w-5xl mx-auto animate-fadeIn">
          {renderContent()}
        </div>
      </main>
    </div>
  );
};

const NavItem = React.memo<{ active: boolean, onClick: () => void, icon: React.ReactNode, label: string }>(({ active, onClick, icon, label }) => (
  <button
    onClick={onClick}
    className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 ${
      active
        ? 'bg-slate-800 text-white shadow-lg ring-1 ring-slate-700'
        : 'text-slate-400 hover:bg-slate-800/50 hover:text-slate-200'
    }`}
  >
    {icon}
    <span className="font-medium">{label}</span>
  </button>
));

NavItem.displayName = 'NavItem';

export default App;
